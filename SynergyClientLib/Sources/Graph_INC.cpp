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

	Node& newFetchedNode;
	{
		size_t i;
		for (i = 0; i < sizeof(FetchedNodes) / sizeof(Node); i++)
		{
			if (FetchedNodes[i].ID == SNODE_INVALID_ID)
			{
				fetched = FetchedNodes[i];
				break;
			}
		}

		if (i == sizeof(FetchedNodes) / sizeof(Node))
		{
			// ASSERT Not enough room to fetch more nodes.
			return nullptr;
		}

		newFetchedNode = FetchedNodes[i];
	}
	
	SNodeDef fetchedDef = TargetGraph->GetNodeDef(NodeID);

	if (fetchedDef.id != NodeID)
	{
		// ASSERT Failed to get node def from graph.
		return nullptr;
	}

	fetched.ID = NodeID;
	fetched.Parent = nullptr; // Fetched nodes are not assigned a parent in the transaction's internal hierarchy.
	fetched.NodeDef = fetchedDef;
	fetched.bDeleted = false;

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
		Connection& newFetchedConnection;
		{
			size_t i;
			for (i = 0; i < sizeof(FetchedConnections) / sizeof(Connection); i++)
			{
				if (FetchedConnections[i].Src == nullptr)
				{
					fetched = FetchedConnections[i];
					break;
				}
			}

			if (i == sizeof(FetchedConnections) / sizeof(Connection))
			{
				// ASSERT Not enough room to fetch more nodes.
				return nullptr;
			}
		}

		newFetchedConnection.AccessLevel = connection.accessLevel;
		newFetchedConnection.Src = &newFetchedNode;
		newFetchedConnection.Dest = targetNode;
		newFetchedConnection.Def = connection;
	}

	// Add a new operation to the operation collection.
	GraphEditOp_Fetch* newFetchOp = TransactionOperationsMemory.Allocate<GraphEditOp_Fetch>();
	newFetchOp->type = EditOperationType::FETCH;
	newFetchOp->fetchedNodeID = newFetchedNode.ID;

	AddOperation(newFetchOp);

	return &fetched;
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

	return nullptr;
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