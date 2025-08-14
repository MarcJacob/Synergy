SOURCE_INC_FILE()

// Implementation source include file for symbols related to Graph management on the Client.

#include "SynergyCore.h"
#include "ClientGraph.h"

GraphEditNode* ClientGraphEditTransaction::FetchGraphNode(SNodeGUID NodeID)
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
	for (fetchedNodeIndex = 0; fetchedNodeIndex < sizeof(FetchedNodes) / sizeof(GraphEditNode); fetchedNodeIndex++)
	{
		if (FetchedNodes[fetchedNodeIndex].ID == SNODE_INVALID_ID)
		{
			break;
		}
	}

	if (fetchedNodeIndex == sizeof(FetchedNodes) / sizeof(GraphEditNode))
	{
		// ASSERT Not enough room to fetch more nodes.
		return nullptr;
	}
	
	GraphEditNode& newFetchedNode = FetchedNodes[fetchedNodeIndex];
	SNodeDef fetchedDef = TargetGraph->GetNodeDef(NodeID);

	if (fetchedDef.id != NodeID)
	{
		// ASSERT Failed to get node def from graph.
		return nullptr;
	}

	newFetchedNode.ID = NodeID;
	newFetchedNode.Parent = nullptr; // Fetched nodes are not assigned a parent in the transaction's internal hierarchy until their parent node gets fetched as well.
	newFetchedNode.NodeDef = fetchedDef;
	newFetchedNode.bDeleted = false;

	// Fetch this node's connections from data store and set them up in the transaction data, 
	// updating existing nodes in the transaction as well if any of them define a parent - child relationship.
	
	SNodeConnectionDef connectionsBuffer[32]; // For simplicity we just assume no node has more than 32 outgoing AND incoming connections, meaning
	// we support a maximum of 16 connected nodes including parent and children.

	size_t connectionCount = TargetGraph->GetNodeConnections_Bidirectional(NodeID, connectionsBuffer, sizeof(connectionsBuffer) / sizeof(SNodeConnectionDef));

	for (size_t connectionIndex = 0; connectionIndex < connectionCount; connectionIndex++)
	{
		SNodeConnectionDef& connectionDef = connectionsBuffer[connectionIndex];
		
		// See if the partner node was fetched beforehand. If it was, the connection should already have been fetched and will be updated in the next step.
		GraphEditNode* partnerNode = nullptr;
		bool bIncomingConnection = false;
		for (GraphEditNode& fetchedNode : FetchedNodes)
		{
			// OUTGOING connection.
			if (fetchedNode.ID == connectionDef.nodeID_Dest)
			{
				partnerNode = &fetchedNode;
				break;
			}

			// INCOMING connection.
			if (fetchedNode.ID == connectionDef.nodeID_Src)
			{
				partnerNode = &fetchedNode;
				bIncomingConnection = true;
				break;
			}
		}

		// If partner node was fetched beforehand and the connection is a parent / child connection, update parent - child relationship data accordingly.
		if (partnerNode != nullptr && connectionDef.bIsParentChildConnection)
		{
			if (newFetchedNode.NodeDef.parentID == partnerNode->ID)
			{
				newFetchedNode.Parent = partnerNode;
				newFetchedNode.AccessLevelToParent = connectionDef.accessLevel;
			}
			else
			{
				partnerNode->Parent = &newFetchedNode;
				partnerNode->AccessLevelFromParent = connectionDef.accessLevel;
			}
		}
		

		// If however the partner node wasn't already fetched, then this is a brand new connection from the Transaction's perpective and needs
		// to be recorded.
		if (partnerNode == nullptr)
		{
			size_t fetchedConnectionIndex;
			for (fetchedConnectionIndex = 0; fetchedConnectionIndex < sizeof(FetchedConnections) / sizeof(GraphEditConnection); fetchedConnectionIndex++)
			{
				if (FetchedConnections[fetchedConnectionIndex].Src == nullptr)
				{
					break;
				}
			}

			if (fetchedConnectionIndex == sizeof(FetchedConnections) / sizeof(GraphEditConnection))
			{
				// ASSERT Not enough room to fetch more nodes.
				return nullptr;
			}
			
			GraphEditConnection& newFetchedConnection = FetchedConnections[fetchedConnectionIndex];
			newFetchedConnection.Src = bIncomingConnection ? partnerNode : &newFetchedNode;
			newFetchedConnection.Dest = bIncomingConnection ? &newFetchedNode : partnerNode;
			newFetchedConnection.Def = connectionDef;
		}
	}

	// Update fetched connections in case they point to the newly fetched node as destination or source.
	for(GraphEditConnection& connection : FetchedConnections)
	{
		if (connection.Def.nodeID_Dest == newFetchedNode.ID)
		{
			connection.Dest = &newFetchedNode;
		}
		else if (connection.Def.nodeID_Src == newFetchedNode.ID)
		{
			connection.Src = &newFetchedNode;
		}
	}

	// Add a new operation to the operation collection.
	GraphEditOp_Fetch* newFetchOp = TransactionOperationsMemory.Allocate<GraphEditOp_Fetch>();
	newFetchOp->type = EditOperationType::FETCH;
	newFetchOp->fetchedGraphNode = &newFetchedNode;

	_AddOperation(*newFetchOp);

	return &newFetchedNode;
}

GraphEditNode* ClientGraphEditTransaction::CreateNode(SNodeDef NewNodeDef, GraphEditNode* Parent)
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
	for (createdNodeIndex = 0; createdNodeIndex < sizeof(CreatedNodes) / sizeof(GraphEditNode); createdNodeIndex++)
	{
		if (CreatedNodes[createdNodeIndex].ID == SNODE_INVALID_ID
		&& CreatedNodes[createdNodeIndex].AccessLevelFromParent == SNodeConnectionAccessLevel::NONE)
		{
			break;
		}
	}

	if (createdNodeIndex == sizeof(CreatedNodes) / sizeof(GraphEditNode))
	{
		// ASSERT Not enough room to fetch more nodes.
		return nullptr;
	}

	GraphEditNode& newCreatedNode = CreatedNodes[createdNodeIndex];

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
	newNewOp->target = &newCreatedNode;
	newNewOp->parent = Parent;

	_AddOperation(*newNewOp);

	return &newCreatedNode;
}

bool ClientGraphEditTransaction::EditNode( GraphEditNode& TargetNode, SNodeDef NewNodeDef, GraphEditNode* NewParent,
					SNodeConnectionAccessLevel AccessToParent, SNodeConnectionAccessLevel AccessFromParent)
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

	// If changing the parent, check that it is a valid operation.
	if (NewParent != nullptr)
	{
		if (TargetNode.ID == TargetGraph->RootNodeID)
		{
			// ASSERT The root node cannot be assigned a parent !
			return false;
		}

		TargetNode.Parent = NewParent;
		TargetNode.AccessLevelFromParent = AccessFromParent;
		TargetNode.AccessLevelToParent = AccessToParent;

		// Updating connections is not necessary as they will be implicitly deleted and created when the operation is processed.
	}

	// Assign new definition data.
	TargetNode.NodeDef = NewNodeDef;
	TargetNode.NodeDef.parentID = TargetNode.Parent != nullptr ? TargetNode.Parent->ID : SNODE_INVALID_ID; 

	// Add Edit operation to the transaction operation container.
	GraphEditOp_NodeNewEdit* newEditOp = TransactionOperationsMemory.Allocate<GraphEditOp_NodeNewEdit>();
	newEditOp->type = EditOperationType::NODE_EDIT;
	newEditOp->def = TargetNode.NodeDef;
	newEditOp->target = &TargetNode;
	newEditOp->parent = NewParent;
	
	_AddOperation(*newEditOp);

	return true;
}

bool ClientGraphEditTransaction::DeleteNode(GraphEditNode& ToBeDeleted)
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

	ToBeDeleted.bDeleted = true;

	// Perform a recursive delete operation on all child nodes. Fetch them if necessary.
	{
		for (GraphEditConnection& connection : FetchedConnections)
		{
			if (connection.Def.bIsParentChildConnection
			&& connection.Src == &ToBeDeleted
			&& ToBeDeleted.NodeDef.parentID != connection.Def.nodeID_Dest) // To-Child connection check.
			{
				GraphEditNode* childNode = connection.Dest;
				if (childNode == nullptr)
				{
					// Fetch child node.
					childNode = FetchGraphNode(connection.Def.nodeID_Dest);
				}
				DeleteNode(*childNode);
			}
		}
	}
	
	// Delete all connections to and from this node as well.
	{
		for (GraphEditConnection& connection : FetchedConnections)
		{
			// Incoming connection.
			if (connection.Dest == &ToBeDeleted)
			{
				if (connection.Src == nullptr)
				{
					// Fetch the source node of this connection so it can be part of the transaction, as it is about to lose a connection !
					FetchGraphNode(connection.Def.nodeID_Src);
				}
				_DeleteConnection(connection);
			}
			// Outgoing connection.
			else if (connection.Src == &ToBeDeleted)
			{
				_DeleteConnection(connection);
			}
		}
	}

	// Add Node Delete operation.
	GraphEditOp_NodeDelete* newNodeDeleteOp = TransactionOperationsMemory.Allocate<GraphEditOp_NodeDelete>();
	newNodeDeleteOp->type = EditOperationType::NODE_DELETE;
	newNodeDeleteOp->deletedNode = &ToBeDeleted;

	_AddOperation(*newNodeDeleteOp);
	return true;
}

bool ClientGraphEditTransaction::AddOrEditConnection(GraphEditNode& SourceNode, GraphEditNode& DestNode, SNodeConnectionDef ConnectionDef)
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

	GraphEditConnection* connectionPtr = nullptr;
	bool bIsFetchedConnection = false;

	// Look first among fetched connections as most edit operations will likely target them.
	size_t fetchedConnectionIndex;
	for (fetchedConnectionIndex = 0; fetchedConnectionIndex < sizeof(FetchedConnections) / sizeof(GraphEditConnection); fetchedConnectionIndex++)
	{
		if (FetchedConnections[fetchedConnectionIndex].Src == &SourceNode
		&& FetchedConnections[fetchedConnectionIndex].Dest == &DestNode)
		{
			connectionPtr = &FetchedConnections[fetchedConnectionIndex];
			bIsFetchedConnection = true;
			break;
		}
	}

	// If the connection to edit wasn't found among the fetched, look among the created connections.
	if (connectionPtr == nullptr)
	{
		size_t createdConnectionIndex;
		size_t availableSpot = ~0;
		for (createdConnectionIndex = 0; createdConnectionIndex < sizeof(CreatedConnections) / sizeof(GraphEditConnection); createdConnectionIndex++)
		{
			if (CreatedConnections[createdConnectionIndex].Src == &SourceNode
			&& CreatedConnections[createdConnectionIndex].Dest == &DestNode)
			{
				connectionPtr = &CreatedConnections[createdConnectionIndex];
				break;
			}
			else if (availableSpot == ~0
				&& CreatedConnections[createdConnectionIndex].Src == nullptr)
			{
				// Cache the available spot in case the connection needs to be created.
				availableSpot = createdConnectionIndex;
			}
		}

		// If it couldn't be found in the Created Connections, then... create it !
		if (connectionPtr == nullptr)
		{
			if (availableSpot == ~0)
			{
				// ASSERT No spots are left. Fail the operation.
				return false;
			}

			// Initialize basic connection properties.
			connectionPtr = &CreatedConnections[availableSpot];
			connectionPtr->Src = &SourceNode;
			connectionPtr->Dest = &DestNode;
		}
	}

	GraphEditConnection& connection = *connectionPtr;
	connection.Def = ConnectionDef; 

	connection.Def.nodeID_Src = SourceNode.ID;
	connection.Def.nodeID_Dest = DestNode.ID;

	GraphEditOp_ConnectionNewEdit* newConnectionNewEditOP = TransactionOperationsMemory.Allocate<GraphEditOp_ConnectionNewEdit>();
	newConnectionNewEditOP->type = EditOperationType::CONNECTION_CREATE_EDIT;
	newConnectionNewEditOP->editedOrCreatedNode = &connection;

	return true;
}

bool ClientGraphEditTransaction::DeleteConnection(GraphEditNode& SourceNode, GraphEditNode& DestNode)
{
	if (TargetGraph == nullptr)
	{
		// ASSERT TargetGraph was not assigned.
		return false;
	}

	if (SourceNode.Parent == &DestNode
	|| DestNode.Parent == &SourceNode)
	{
		// ASSERT Cannot delete parent-child connection. Use Node_Edit to change a node's parent.
		return false;
	}

	for(GraphEditConnection& connection : FetchedConnections)
	{
		if (connection.Src == &SourceNode
		&& connection.Dest == &DestNode)
		{
			return _DeleteConnection(connection);
		}
	}

	for(GraphEditConnection& connection : CreatedConnections)
	{
		if (connection.Src == &SourceNode
		&& connection.Dest == &DestNode)
		{
			return _DeleteConnection(connection);
		}
	}

	// ASSERT The nodes are not connected, or only possess implicit parent-child connections which can not be deleted !
	return false;
}

bool ClientGraphEditTransaction::_DeleteConnection(GraphEditConnection& Connection)
{
	if (TargetGraph == nullptr)
	{
		// ASSERT TargetGraph was not assigned.
		return false;
	}

	if (Connection.bDeleted)
	{
		// ASSERT Connection is already deleted.
		return false;
	}

	/*
		Set the deleted flag of the connection.
		Add a corresponding CONNECTION_DELETE operation in the operation collection.
	*/

	Connection.bDeleted = true;

	GraphEditOp_ConnectionDelete* newConnectionDeleteOp = TransactionOperationsMemory.Allocate<GraphEditOp_ConnectionDelete>();
	newConnectionDeleteOp->type = EditOperationType::CONNECTION_DELETE;
	newConnectionDeleteOp->deletedConnection = &Connection;

	_AddOperation(*newConnectionDeleteOp);

	return true;
}

void ClientGraphEditTransaction::_AddOperation(GraphEditOp_Base& NewOp)
{
	if (OperationsListBegin == nullptr)
	{
		OperationsListBegin = &NewOp;
		OperationsListEnd = &NewOp;
	}
	else
	{
		GraphEditOp_Base* previousEnd = OperationsListEnd;
		previousEnd->next = &NewOp;
		NewOp.previous = previousEnd;
		OperationsListEnd = &NewOp;
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

size_t ClientGraph::GetNodeConnections_Bidirectional(SNodeGUID NodeID, SNodeConnectionDef* NodeConnectionsBuffer, size_t NodeConnectionsBufferSize)
{
	// Go through access level matrix, counting connections. If buffer pointer is given, also fill it in with appropriate structured data
	// until it is full. Keep counting so that the function's user can notice that the given buffer wasn't large enough to hold them all.

	bool bBufferPassed = NodeConnectionsBuffer != nullptr && NodeConnectionsBufferSize > 0;
	size_t connectionsCount = 0;

	// Matrix stores the access level from one node to all others line-wise.
	// Retrieve all incoming and outgoing connections.
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
			connectionsCount += 2;
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