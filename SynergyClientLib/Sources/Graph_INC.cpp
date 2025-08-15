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

	// Default values for parent access levels. Useful for root too as it indicates the Fetched node actually exists.
	newFetchedNode.AccessLevelFromParent = SNodeConnectionAccessLevel::TO_CHILD_MINIMUM;
	newFetchedNode.AccessLevelToParent = SNodeConnectionAccessLevel::TO_PARENT_MINIMUM;

	// Fetch this node's connections from data store and set them up in the transaction data, 
	// updating existing nodes in the transaction as well if any of them define a parent - child relationship.
	
	SNodeConnectionDef connectionsBuffer[32]; // For simplicity we just assume no node has more than a total of 32 outgoing AND incoming connections.

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
				DeleteConnection(connection);
			}
			// Outgoing connection.
			else if (connection.Src == &ToBeDeleted)
			{
				DeleteConnection(connection);
			}
		}
	}
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
				&& CreatedConnections[createdConnectionIndex].Src == nullptr
				&& CreatedConnections[createdConnectionIndex].Dest == nullptr)
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

	// Apply changes to connection, whether it is new or edited.
	GraphEditConnection& connection = *connectionPtr;
	connection.Def = ConnectionDef; 

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
		// ASSERT Cannot delete parent-child connection. EDIT a node to change a its parent.
		return false;
	}

	for(GraphEditConnection& connection : FetchedConnections)
	{
		if (connection.Src == &SourceNode
		&& connection.Dest == &DestNode)
		{
			return DeleteConnection(connection);
		}
	}

	for(GraphEditConnection& connection : CreatedConnections)
	{
		if (connection.Src == &SourceNode
		&& connection.Dest == &DestNode)
		{
			return DeleteConnection(connection);
		}
	}

	// ASSERT The nodes are not connected, or only possess implicit parent-child connections which can not be deleted !
	return false;
}

bool ClientGraphEditTransaction::DeleteConnection(GraphEditConnection& Connection)
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
	*/

	Connection.bDeleted = true;
	return true;
}

template<size_t MaxNodeCount>
SNodeGUID StaticClientGraphDataStore<MaxNodeCount>::FindAvailableID() const
{
	for(SNodeGUID nodeID = 0; nodeID < MaxNodeCount; nodeID++)
	{
		if (_StaticNodeCoreDataStore[nodeID].name[0] == '\0')
		{
			return nodeID;
		}
	}

	return SNODE_INVALID_ID;
}

SNodeDef ClientGraph::GetNodeDef(SNodeGUID NodeID, SNodeGUID StartNodeID)
{
	// If node of this ID doesn't exist, return "hollow" definition.
	if (NodeID > DataStore.GetMaxNodeCount() || DataStore._StaticNodeCoreDataStore[NodeID].name[0] == '\0')
	{
		return {};
	}
	NodeCoreData& coreData = DataStore._StaticNodeCoreDataStore[NodeID];

	SNodeDef def = {
		NodeID,
		coreData.parentNodeID,
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

	/*
		Go through Fetched nodes and delete those marked for deletion.
		Create nodes created by the transaction.
		Resolve new parentage of all involved nodes.
		Update connection matrix.
	*/

	// TODO Check that all fetched nodes actually exist.

	// Delete fetched nodes marked for deletion.
	for (GraphEditNode& fetchedNode : TransactionToApply.FetchedNodes)
	{
		if (fetchedNode.AccessLevelFromParent == SNodeConnectionAccessLevel::NONE) break; // End of Fetched Nodes array.

		if (fetchedNode.bDeleted)
		{
			// Find node in datastore and reset all data.
			memset(DataStore._StaticNodeCoreDataStore + fetchedNode.ID, 0, sizeof(NodeCoreData));

			for(size_t matrixIndex = 0; matrixIndex < DataStore.GetMaxNodeCount(); matrixIndex++)
			{
				DataStore._StaticAccessLevelMatrix[fetchedNode.ID * DataStore.GetMaxNodeCount() + matrixIndex] = SNodeConnectionAccessLevel::NONE;
				DataStore._StaticAccessLevelMatrix[matrixIndex * DataStore.GetMaxNodeCount() + fetchedNode.ID] = SNodeConnectionAccessLevel::NONE;
			}
		}
	}

	// TODO Check that the datastores are able to store all the newly created nodes.

	// Create created nodes that aren't marked for deletion.
	for(GraphEditNode& createdNode : TransactionToApply.CreatedNodes)
	{
		if (createdNode.AccessLevelFromParent == SNodeConnectionAccessLevel::NONE) break; // End of Created Nodes array.

		if (createdNode.bDeleted)
		{
			// Skip. Should they perhaps be removed from the transaction altogether if they get deleted ?
			continue;
		}

		// Create the node !
		// Since Datastore was checked for capacity beforehand, let's assume an ID is found.
		SNodeGUID newNodeID = DataStore.FindAvailableID();
		
		// Assign ID to Graph Edit node so other related nodes and connections know what ID to reference.
		createdNode.ID = newNodeID;

		// Init properties we have knowledge of already.
		strcpy_s(DataStore._StaticNodeCoreDataStore[newNodeID].name, sizeof(NodeName), createdNode.NodeDef.name);
	}

	// Resolve parentage for new nodes.
	for(GraphEditNode& createdNode : TransactionToApply.CreatedNodes)
	{
		if (createdNode.AccessLevelFromParent == SNodeConnectionAccessLevel::NONE) break; // End of Created Nodes array.

		if (createdNode.Parent != nullptr)
		{
			DataStore._StaticNodeCoreDataStore[createdNode.ID].parentNodeID = createdNode.Parent->ID;
		}
		else
		{
			// New root node.
			// For now let's not worry about ending up with multiple root nodes. It will be checked for as a post process of the transaction.
			DataStore._StaticNodeCoreDataStore[createdNode.ID].parentNodeID = SNODE_INVALID_ID;
			RootNodeID = createdNode.ID;
		}
	}

	// Resolve parentage change for fetched nodes.
	// Remove connection data between former parent and child. It has to be done now because the information that parent changed is lost later.
	for(GraphEditNode& fetchedNode : TransactionToApply.FetchedNodes)
	{
		if (fetchedNode.AccessLevelFromParent == SNodeConnectionAccessLevel::NONE) break; // End of Created Nodes array.

		// Delete previous parent relationship.
		SNodeGUID previousParentID = DataStore._StaticNodeCoreDataStore[fetchedNode.ID].parentNodeID;
		SNodeGUID newParentID = fetchedNode.Parent != nullptr ? fetchedNode.Parent->ID : SNODE_INVALID_ID;

		if (newParentID != previousParentID)
		{
			if (previousParentID != SNODE_INVALID_ID)
			{
				// Remove connection between previous parent and child.
				DataStore._StaticAccessLevelMatrix[fetchedNode.ID * DataStore.GetMaxNodeCount() + previousParentID] = SNodeConnectionAccessLevel::NONE;
				DataStore._StaticAccessLevelMatrix[previousParentID * DataStore.GetMaxNodeCount() + fetchedNode.ID] = SNodeConnectionAccessLevel::NONE;
			}
		}

		// Update parent ID in core values.
		DataStore._StaticNodeCoreDataStore[fetchedNode.ID].parentNodeID = newParentID;	

		// New parent - Add connection between new parent and child.
		if (newParentID != SNODE_INVALID_ID)
		{
			DataStore._StaticAccessLevelMatrix[fetchedNode.ID * DataStore.GetMaxNodeCount() + newParentID] = fetchedNode.AccessLevelToParent;
			DataStore._StaticAccessLevelMatrix[newParentID * DataStore.GetMaxNodeCount() + fetchedNode.ID] = fetchedNode.AccessLevelFromParent;
		}
		// New root node. Update Graph Root Node ID.
		// For now let's not worry about ending up with multiple root nodes. It will be checked for as a post process of the transaction.
		else
		{
			RootNodeID = fetchedNode.ID;
		}
	}

	// Resolve created connections.
	for (GraphEditConnection& createdConnection : TransactionToApply.CreatedConnections)
	{
		if (createdConnection.Src == nullptr) break; // End of array reached.

		if (createdConnection.bDeleted)
		{
			continue;
		}

		// Update access level from source to destination.
		DataStore._StaticAccessLevelMatrix[createdConnection.Src->ID * DataStore.GetMaxNodeCount() + createdConnection.Dest->ID]
		 = createdConnection.Def.accessLevel;
	}

	// Resolve fetched connections.
	for (GraphEditConnection& fetchedConnection : TransactionToApply.FetchedConnections)
	{
		if (fetchedConnection.Def.accessLevel == SNodeConnectionAccessLevel::NONE) break; // End of array reached.

		// If connection was deleted, simply reset access level between the nodes.
		if (fetchedConnection.bDeleted)
		{
			DataStore._StaticAccessLevelMatrix[fetchedConnection.Src->ID * DataStore.GetMaxNodeCount() + fetchedConnection.Dest->ID]
			= SNodeConnectionAccessLevel::NONE;
		}

		// Update access level from source to destination.
		DataStore._StaticAccessLevelMatrix[fetchedConnection.Src->ID * DataStore.GetMaxNodeCount() + fetchedConnection.Dest->ID]
		 = fetchedConnection.Def.accessLevel;
	}

	return true;
}