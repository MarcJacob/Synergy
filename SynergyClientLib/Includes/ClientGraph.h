// Contains symbols that outline how the Client models its own view of the Graph they are currently working on while connected to a server.

#include "Graph/SynergyGraph.h"

// The Client essentially needs to try and predict where the user will attempt to travel to next on the graph and keep that data quickly
// accessible, while also providing a potentially very large storage capacity for large graphs, with our without the help of a server.

// On top of this it must provide utilities for building graph editing transactions.

// To this end, the client maintains an interface to the rest of the app to server common commands and being able to switch into Edit Mode
// to conveniently build change operations and form a transaction that can be applied or sent to the server for approval.

// Core Data making up a Node.
struct NodeCoreData
{
	NodeName name;
	Vector2i coordinates;

	SNodeGUID parentNodeID;
};

/*
	Contains the entire local state of the graph and provides interface functions to process common commands in the Synergy system for
	finding nodes, interacting with them, and creating change requests.
*/
struct ClientGraph
{
	/*
		Returns a structured representation of a node's core definition data from its ID.
		StartNodeID, if set, gives the system a hint on where to start the search.
	*/ 
	SNodeDef GetNodeDef(SNodeGUID NodeID, SNodeGUID StartNodeID = SNODE_INVALID_ID);

	/*
		Returns a structured representation of a node's core definition data from its Name.
		StartNodeID, if set, gives the system a hint on where to start the search.
		If the graph contains multiple nodes with the same name, the first one found is returned.
	*/ 
	SNodeDef GetNodeDef(NodeName Name, SNodeGUID StartNodeID = SNODE_INVALID_ID);

	// Returns the connections from the passed Node ID. Returns the total amount of connections.
	size_t GetNodeConnections(SNodeGUID NodeID, SNodeConnectionDef* NodeConnectionsBuffer, size_t NodeIDBufferSize);

	// Creates a new node or updates an existing one to match the passed info.
	// #TODO To be replaced with an appropriate transaction-based system.
	void CreateOrUpdateNode(SNodeDef Def, SNodeConnectionDef* Connections, size_t ConnectionCount);

	MemoryAllocator GraphMemory;

	// Core Data datastore for stored nodes. Supports up to a certain amount of nodes at once. Empty slots are marked starting with a 0.
	// Slot in the store corresponds to the Node's GUID.
	NodeCoreData NodeCoreDataStore[16];

	// Access Level matrix between all stored nodes.
	SNodeConnectionAccessLevel AccessLevelMatrix[16 * 16];
};