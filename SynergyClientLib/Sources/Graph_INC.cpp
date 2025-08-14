SOURCE_INC_FILE()

// Implementation source include file for symbols related to Graph management on the Client.

#include "SynergyCore.h"
#include "ClientGraph.h"

ClientGraphEditTransaction::Node* ClientGraphEditTransaction::FetchGraphNode(SNodeGUID NodeID)
{
	if (TargetGraph == nullptr)
	{
		// ASSERT TargetGraph was not assigned.
		return nullptr;
	}

	/*
		Find the Node with the given ID in the graph and "deploy" it in the FetchedNodes collection.
		Along with the node itself, find all of its outgoing connections and place them in the Connections collection.
		Add a corresponding FETCH operation in the operation collection.
	*/

	size_t fetchedNodeIndex;
	for (fetchedNodeIndex = 0; fetchedNodeIndex < sizeof(FetchedNodes) / sizeof(Node); fetchedNodeIndex++)
	{
		if (FetchedNodes[fetchedNodeIndex].ID == SNODE_INVALID_ID)
		{
			break;
		}
	}

	if (fetchedNodeIndex == sizeof(FetchedNodes) / sizeof(Node))
	{
		// ASSERT Not enough room to fetch more nodes.
		return nullptr;
	}
	
	Node& newFetchedNode = FetchedNodes[fetchedNodeIndex];
	SNodeDef fetchedDef = TargetGraph->GetNodeDef(NodeID);

	if (fetchedDef.id != NodeID)
	{
		// ASSERT Failed to get node def from graph.
		return nullptr;
	}

	newFetchedNode.ID = NodeID;
	newFetchedNode.Parent = nullptr; // Fetched nodes are not assigned a parent in the transaction's internal hierarchy.
	newFetchedNode.NodeDef = fetchedDef;
	newFetchedNode.bDeleted = false;

	// Fetch this node's connections from data store and set them up in the transaction data, 
	// updating existing nodes in the transaction as well if any of them define a parent - child relationship.
	
	SNodeConnectionDef connectionsBuffer[32]; // For simplicity we just assume no node has more than 32 outgoing connections.
	size_t connectionCount = TargetGraph->GetNodeConnections(NodeID, connectionsBuffer, sizeof(connectionsBuffer) / sizeof(SNodeConnectionDef));

	for (size_t connectionIndex = 0; connectionIndex < connectionCount; connectionIndex++)
	{
		SNodeConnectionDef& connection = connectionsBuffer[connectionIndex];
		SNodeGUID targetNodeID = connection.nodeID_Dest;
		
		// See if the target node was fetched beforehand. If it was, related info can be updated.
		Node* targetNode = nullptr;
		{

			for (Node& fetchedNode : FetchedNodes)
			{
				if (fetchedNode.ID == connection.nodeID_Dest)
				{
					targetNode = &fetchedNode;
					break;
				}
			}

			// Update parentage
			if (targetNode != nullptr && connection.bIsParentChildConnection)
			{
				if (fetchedDef.parentID == targetNode->ID)
				{
					fetched.Parent = targetNode;
				}
				else
				{
					targetNode->Parent = &fetched;
				}
			}
		}

		// Register the connection in the transaction.
		
		size_t fetchedConnectionIndex;
		for (fetchedConnectionIndex = 0; fetchedConnectionIndex < sizeof(FetchedConnections) / sizeof(Connection); fetchedConnectionIndex++)
		{
			if (FetchedConnections[fetchedConnectionIndex].Src == nullptr)
			{
				break;
			}
		}

		if (fetchedConnectionIndex == sizeof(FetchedConnections) / sizeof(Connection))
		{
			// ASSERT Not enough room to fetch more nodes.
			return nullptr;
		}
		
		Connection& newFetchedConnection = FetchedConnections[fetchedConnectionIndex];

		newFetchedConnection.AccessLevel = connection.accessLevel;
		newFetchedConnection.Src = &newFetchedNode;
		newFetchedConnection.Dest = targetNode;
		newFetchedConnection.FetchedDef = connection;
	}

	// Add a new operation to the operation collection.
	GraphEditOp_Fetch* newFetchOp = TransactionOperationsMemory.Allocate<GraphEditOp_Fetch>();
	newFetchOp->type = EditOperationType::FETCH;
	newFetchOp->fetchedNodeID = newFetchedNode.ID;

	AddOperation(newFetchOp);

	return &newFetchedNode;
}

ClientGraphEditTransaction::Node* ClientGraphEditTransaction::CreateNode(SNodeDef NewNodeDef, ClientGraphEditTransaction::Node* Parent)
{
	if (TargetGraph == nullptr)
	{
		// ASSERT TargetGraph was not assigned.
		return nullptr;
	}

	/*
		Allocate a new node in the CreatedNode collection and build it using the passed parameters and return its address.
		Add a corresponding NODE_CREATE operation in the operation collection.
	*/

	size_t createdNodeIndex;
	for (createdNodeIndex = 0; createdNodeIndex < sizeof(CreatedNodes) / sizeof(Node); createdNodeIndex++)
	{
		if (CreatedNodes[createdNodeIndex].ID == SNODE_INVALID_ID)
		{
			break;
		}
	}

	if (createdNodeIndex == sizeof(CreatedNodes) / sizeof(Node))
	{
		// ASSERT Not enough room to fetch more nodes.
		return nullptr;
	}

	Node& newCreatedNode = CreatedNodes[createdNodeIndex];

	newCreatedNode.ID = SNODE_INVALID_ID; // Created Node don't get an ID, it will get assigned as the transaction is processed.
	newCreatedNode.bDeleted = false;
	newCreatedNode.Parent = Parent;
	newCreatedNode.NodeDef = NewNodeDef;

	// Set minimum access levels to and from parent.
	newCreatedNode.AccessLevelFromParent = SNodeConnectionAccessLevel::PRIVATE;
	newCreatedNode.AccessLevelToParent = SNodeConnectionAccessLevel::PUBLIC;

	// Adding actual Connection objects for parent-child relationship
	// to the transaction is not needed as they are implicitly created when creating the node.

	// Add New node operation to the operation container.
	GraphEditOp_NodeNewEdit* newNewOp = TransactionOperationsMemory.Allocate<GraphEditOp_NodeNewEdit>();
	newNewOp->type = EditOperationType::NODE_NEW;
	newNewOp->def = newCreatedNode.NodeDef;
	newNewOp->createdNodeIndex = createdNodeIndex;

	AddOperation(newNewOp);

	return &newCreatedNode;
}

bool ClientGraphEditTransaction::EditNode(SNodeDef NewNodeDef, ClientGraphEditTransaction::Node* TargetNode, ClientGraphEditTransaction::Node* NewParent)
{
	if (TargetGraph == nullptr)
	{
		// ASSERT TargetGraph was not assigned.
		return false;
	}

	/*
		Change the target node's data with the passed Def parameter and Parent (if non null).
		Add a corresponding NODE_EDIT operation in the operation collection.
	*/

	if (TargetNode == nullptr)
	{
		// ASSERT Target node was not assigned. It must be created or fetched first !
		return false;
	}

	Node& editedNode = *TargetNode;

	// If changing the parent, check that it is a valid operation and delete existing parent - child connection if need be.
	if (NewParent != nullptr)
	{
		if (editedNode.ID == TargetGraph->RootNodeID)
		{
			// ASSERT The root node cannot be assigned a parent !
			return false;
		}

		if (editedNode.bFetched)
		{
			// Changing the parentage of a fetched node requires the previous AND new parent to be fetched as well.
			if (editedNode.Parent == nullptr)
			{
				// ASSERT The previous parent needs to be fetched !
			}
			else
			{
				// Delete connection between the two parent - child fetched nodes directly.
				DeleteConnection(TargetNode, editedNode.Parent); // Deleting a parent / child connection deletes the symmetrical connection as well.
			}
		}
		editedNode.Parent = NewParent;
	}

	// Assign new definition data.
	editedNode.NodeDef = NewNodeDef;

	// Add Edit operation to the transaction operation container.
	GraphEditOp_NodeNewEdit* newEditOp = TransactionOperationsMemory.Allocate<GraphEditOp_NodeNewEdit>();
	newEditOp->type = EditOperationType::NODE_EDIT;
	newEditOp->def = NewNodeDef;

	AddOperation(newEditOp);

	return true;
}

bool ClientGraphEditTransaction::DeleteNode(ClientGraphEditTransaction::Node* ToBeDeleted)
{
	if (TargetGraph == nullptr)
	{
		// ASSERT TargetGraph was not assigned.
		return false;
	}
	/*
		Set the bIsDeleted flag of the target node to true.
		Add a corresponding NODE_DELETE operation in the operation collection.
	*/
}

bool ClientGraphEditTransaction::AddOrEditConnection(ClientGraphEditTransaction::Node* SourceNode,
							ClientGraphEditTransaction::Node* TargetNode, SNodeConnectionDef ConnectionDef)
{
	if (TargetGraph == nullptr)
	{
		// ASSERT TargetGraph was not assigned.
		return false;
	}

	/*
		Create a new connection or edit an existing one from a source to a target node based on the passed Connection Def.
		Add a corresponding CONNECTION_CREATE_EDIT operation in the operation collection.
	*/

	// Find existing connection. If it doesn't already exist, or only implicitly in the case of new parent - child connections, create it.

	Node* connectionPtr = nullptr;
	bool bIsFetchedConnection = false;

	// Look first among fetched connections as most edit operations will likely target them.
	size_t fetchedConnectionIndex;
	for (fetchedConnectionIndex = 0; fetchedConnectionIndex < sizeof(FetchedConnections) / sizeof(Connection); fetchedConnectionIndex++)
	{
		if (FetchedNodes[fetchedConnectionIndex].ID == SNODE_INVALID_ID)
		{
			connectionPtr = &FetchedNodes[fetchedConnectionIndex];
			bIsFetchedConnection = true;
			break;
		}
	}

	// If the connection to edit wasn't found among the fetched, look among the created connections.
	if (connectionPtr == nullptr)
	{
		size_t createdConnectionIndex;
		for (createdConnectionIndex = 0; createdConnectionIndex < sizeof(CreatedConnections) / sizeof(Connection); createdConnectionIndex++)
		{
			if (CreatedConnections[createdConnectionIndex].Src == SourceNode)
			{
				connectionPtr = &CreatedNodes[createdConnectionIndex];
				break;
			}
		}
	}
}

bool ClientGraphEditTransaction::DeleteConnection(ClientGraphEditTransaction::Node* SourceNode, ClientGraphEditTransaction::Node* TargetNode)
{
	if (TargetGraph == nullptr)
	{
		// ASSERT TargetGraph was not assigned.
		return false;
	}

	/*
		Delete the connection from a source to a target node.
		Add a corresponding CONNECTION_DELETE operation in the operation collection.
	*/
}

void ClientGraphEditTransaction::AddOperation(GraphEditOp_Base* NewOp)
{
	if (OperationsListBegin == nullptr)
	{
		OperationsListBegin = NewOp;
		OperationsListEnd = NewOp;
	}
	else
	{
		GraphEditOp_Base* previousEnd = OperationsListEnd;
		previousEnd->next = NewOp;
		NewOp->previous = previousEnd;
		OperationsListEnd = NewOp;
	}

	OperationsCount++;
}

SNodeDef ClientGraph::GetNodeDef(SNodeGUID NodeID, SNodeGUID StartNodeID)
{
	if (NodeID > DataStore.GetMaxNodeCount())
	{
		return {};
	}
	NodeCoreData& coreData = DataStore._StaticNodeCoreDataStore[NodeID];

	SNodeDef def = {
		NodeID,
		SNODE_INVALID_ID, // TODO search for parent-child connection related to node and use it to find parent ID.
	};

	strcpy_s(def.name, sizeof(def.name), coreData.name);

	return def;
}

SNodeDef ClientGraph::GetNodeDef(NodeName Name, SNodeGUID StartNodeID)
{
	// Linear search should data store.
	// This only works so long as Node GUIDs can be directly translated into Index of datastore element.
	for (SNodeGUID NodeID = 0; NodeID < DataStore.GetMaxNodeCount(); NodeID++)
	{
		if (strcmp(Name, DataStore._StaticNodeCoreDataStore[NodeID].name) == 0)
		{
			return GetNodeDef(NodeID);
		}
	}

	return {};
}

size_t ClientGraph::GetNodeConnections(SNodeGUID NodeID, SNodeConnectionDef* NodeConnectionsBuffer, size_t NodeConnectionsBufferSize)
{
	// Go through access level matrix, counting connections. If buffer pointer is given, also fill it in with appropriate structured data
	// until it is full. Keep counting so that the function's user can notice that the given buffer wasn't large enough to hold them all.

	bool bBufferPassed = NodeConnectionsBuffer != nullptr && NodeConnectionsBufferSize > 0;
	size_t connectionsCount = 0;

	// Matrix stores the access level from one node to all others line-wise.
	for (SNodeGUID OtherNodeID = 0; OtherNodeID < DataStore.GetMaxNodeCount(); OtherNodeID++)
	{
		const SNodeConnectionAccessLevel& accessLevel = DataStore._StaticAccessLevelMatrix[NodeID * DataStore.GetMaxNodeCount() + OtherNodeID];
		if (accessLevel > SNodeConnectionAccessLevel::NONE)
		{
			if (bBufferPassed && connectionsCount < NodeConnectionsBufferSize)
			{
				NodeConnectionsBuffer[connectionsCount] =
				{
					NodeID, OtherNodeID,
					accessLevel
				};
			}
			connectionsCount++;
		}
	}

	return connectionsCount;
}

bool ClientGraph::ApplyEditTransaction(ClientGraphEditTransaction& TransactionToApply)
{
	if (TransactionToApply.TargetGraph != this)
	{
		// ASSERT: Transaction was applied to the wrong graph.
		return false;
	}

	/*	Determine and run prerequisite checks :
		 - Do the fetched nodes exist ?
		 - ... (Add other prerequisites here later in plain english)
		If any of the checks fail, fail the transaction immediately.
	*/

	/*	
		Run transaction, applying each operation one by one and checking their success.
		If any operation fails, undo all previous operations and fail the transaction.
	*/

	/*
		Run postrequisite checks :
			- Do the created nodes now exist in the graph ?
			- Are the deleted nodes now absent from the graph ?
			- Do the edited and created nodes have the correct contents ?
			- Are the connections of edited and created nodes in the correct state ?
				- This includes connections to deleted nodes that should be gone as well.
		If any check fails, undo the entire transaction and fail it.
	*/

	return true;
}