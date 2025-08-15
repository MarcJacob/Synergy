// Contains symbols that outline how the Client models its own view of the Graph they are currently working on while connected to a server.

#ifndef CLIENT_GRAPH_INCLUDED
#define CLIENT_GRAPH_INCLUDED

#include "Graph/SynergyGraph.h"
#include "SynergyCore.h"

// The Client essentially needs to try and predict where the user will attempt to travel to next on the graph and keep that data quickly
// accessible, while also providing a potentially very large storage capacity for large graphs, with our without the help of a server.

// On top of this it must provide utilities for building graph editing transactions.

// To this end, the client maintains an interface to the rest of the app to server common commands and being able to switch into Edit Mode
// to conveniently build change operations and form a transaction that can be applied or sent to the server for approval.

// Core Data making up a Node.
struct NodeCoreData
{
	// Display name of the node.
	NodeName name;

	// ID of parent node.
	SNodeGUID parentNodeID;
};

struct ClientGraph;

/*
	"Deployed" version of a node, forming a structured and easy-to-parse graph for edition.
*/
struct GraphEditNode
{
	SNodeGUID ID = SNODE_INVALID_ID; // Invalid == New node.
	bool bDeleted = false;
	bool bFetched = false;

	GraphEditNode* Parent = nullptr;
	SNodeConnectionAccessLevel AccessLevelToParent = SNodeConnectionAccessLevel::NONE;
	SNodeConnectionAccessLevel AccessLevelFromParent = SNodeConnectionAccessLevel::NONE;

	// Node Def data from latest new or edit operation affecting this node.
	SNodeDef NodeDef = {};
};

/*
	"Deployed" wrapper around a connection definition, giving direct pointer access to involved Nodes in the transaction if any.
	By the end of the transaction construction Source HAS to be defined, and Destination unless connection was fetched and contains
	a valid destination ID.
*/
struct GraphEditConnection
{
	GraphEditNode* Src = nullptr;
	GraphEditNode* Dest = nullptr;

	SNodeConnectionDef Def = {};

	bool bDeleted = false;
};

/*
	Contains a set of operations to perform and the necessary data to build connections involving nodes created during the transaction.
	TODO The system can be made simpler once a arbitrary memory allocator is implemented.
*/
struct ClientGraphEditTransaction
{
	ClientGraph* TargetGraph = nullptr;

	// Total number of generated operations. Used to double-check the validity of the transaction when applying it.
	size_t OperationsCount = 0;

	// Nodes that were Fetched from existing datastores. Effectively contains which existing nodes are involved in the transaction.
	GraphEditNode FetchedNodes[32];
	// Nodes created by this transaction. They need to be assigned an ID before most other steps of applying the transaction.
	GraphEditNode CreatedNodes[32];

	GraphEditConnection FetchedConnections[32 * 32];
	GraphEditConnection CreatedConnections[32 * 32];

	/*
		Loads an existing node from the parent graph so it may be involved in the transaction.
		Returns the fetched node within the transaction, usable to create other nodes or edit it conditionally.
	*/ 
	GraphEditNode* FetchGraphNode(SNodeGUID NodeID);

	/*
		Creates a new node with the passed definition.
		Def Parent ID not taken into account. Node ID should be defined if editing an existing node.
		Returns the newly created node within the transaction, usable to create further nodes or edit it conditionally.
	*/
	GraphEditNode* CreateNode(SNodeDef NewNodeDef, GraphEditNode* Parent);

	/*
		Edits a node that was fetched or created earlier in the transaction.
		Returns whether the operation was successfully added.
	*/
	bool EditNode(GraphEditNode& TargetNode, SNodeDef NewNodeDef, GraphEditNode* NewParent = nullptr,
					SNodeConnectionAccessLevel AccessToParent = SNodeConnectionAccessLevel::TO_PARENT_MINIMUM,
					SNodeConnectionAccessLevel AccessFromParent = SNodeConnectionAccessLevel::TO_CHILD_MINIMUM);

	/*
		Deletes a node from the transaction.
		If the node was present on the graph before the transaction, will record the node's ID not existing as a post-requisite of the transaction. 
		Returns whether the operation was successfully added.
	*/
	bool DeleteNode(GraphEditNode& ToBeDeleted);

	/*
		Adds or edits a connection from a source node to a target node based on the passed connection def.
		Returns whether the operation was successfully added.
	*/
	bool AddOrEditConnection(GraphEditNode& SourceNode, GraphEditNode& DestNode, SNodeConnectionDef ConnectionDef);

	/*
		Deletes the connection from the Source node to the Target node and its symmetrical.
		Can only delete non-parent-child connections. Use Edit Node operation to change a node's parent.
		Returns whether the operation was successfully added.
	*/
	bool DeleteConnection(GraphEditNode& SourceNode, GraphEditNode& TargetNode);

	/*
		Deletes the connection. Make sure to delete its symmetrical as well !
		Can delete parent-child connections.
		Returns whether the operation was successfully added.
	*/
	bool DeleteConnection(GraphEditConnection& Connection);
};

/*
	Statically defined Data Store for a client graph's nodal data.
*/
template<size_t MaxNodeCount>
struct StaticClientGraphDataStore
{
	// Core Data datastore for stored nodes. Supports up to a certain amount of nodes at once. Empty slots are marked starting with a 0.
	// Slot in the store corresponds to the Node's GUID.
	NodeCoreData _StaticNodeCoreDataStore[MaxNodeCount];

	// Access Level matrix between all stored nodes. To be updated after every edit operation.
	SNodeConnectionAccessLevel _StaticAccessLevelMatrix[MaxNodeCount * MaxNodeCount];

	constexpr size_t GetMaxNodeCount() const { return MaxNodeCount; }

	SNodeGUID FindAvailableID() const;
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

	// Returns the connections to AND from the passed Node ID. Returns the total amount of connections.
	size_t GetNodeConnections_Bidirectional(SNodeGUID NodeID, SNodeConnectionDef* NodeConnectionsBuffer, size_t NodeIDBufferSize);

	/*
		Attempts to apply the passed transaction.
		Will mutate the transaction to facilitate its application.
	*/
	bool ApplyEditTransaction(ClientGraphEditTransaction& TransactionToApply);

	// Node ID for Root Node of the Graph, which is the only node that can have no parent.
	SNodeGUID RootNodeID = 0;

	/*
		Data Store for the Graph.In the spirit of keeping it simple for now, we just use a static data store with a predefined max size.
		Later this will probably be abstracted away with a type that simply exposes function pointers for access to various elements.
	*/
	StaticClientGraphDataStore<16> DataStore;
};

#endif