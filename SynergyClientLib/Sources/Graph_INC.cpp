SOURCE_INC_FILE()

// Implementation source include file for symbols related to Graph management on the Client.

#include "SynergyCore.h"
#include "ClientGraph.h"

ClientGraphEditTransaction::Node* ClientGraphEditTransaction::FetchGraphNode(SNodeGUID NodeID)
{
	/*
		Find the Node with the given ID in the graph and "deploy" it in the FetchedNodes collection.
		Along with the node itself, find all of its outgoing connections and place them in the Connections collection.
		Add a corresponding FETCH operation in the operation collection.
	*/

	return nullptr;
}

ClientGraphEditTransaction::Node* ClientGraphEditTransaction::CreateNode(SNodeDef NewNodeDef, ClientGraphEditTransaction::Node* Parent)
{
	/*
		Allocate a new node in the CreatedNode collection and build it using the passed parameters and return its address.
		Add a corresponding NODE_CREATE operation in the operation collection.
	*/

	return nullptr;
}

void ClientGraphEditTransaction::EditNode(SNodeDef NewNodeDef, ClientGraphEditTransaction::Node* TargetNode, ClientGraphEditTransaction::Node* NewParent)
{
	/*
		Change the target node's data with the passed Def parameter and Parent (if non null).
		Add a corresponding NODE_EDIT operation in the operation collection.
	*/
}

void ClientGraphEditTransaction::DeleteNode(ClientGraphEditTransaction::Node* ToBeDeleted)
{
	/*
		Set the bIsDeleted flag of the target node to true.
		Add a corresponding NODE_DELETE operation in the operation collection.
	*/
}

void ClientGraphEditTransaction::AddOrEditConnection(ClientGraphEditTransaction::Node* SourceNode,
							ClientGraphEditTransaction::Node* TargetNode, SNodeConnectionDef ConnectionDef)
{
	/*
		Create a new connection or edit an existing one from a source to a target node based on the passed Connection Def.
		Add a corresponding CONNECTION_CREATE_EDIT operation in the operation collection.
	*/
}

void ClientGraphEditTransaction::DeleteConnection(ClientGraphEditTransaction::Node* SourceNode, ClientGraphEditTransaction::Node* TargetNode)
{
	/*
		Delete the connection from a source to a target node.
		Add a corresponding CONNECTION_DELETE operation in the operation collection.
	*/
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