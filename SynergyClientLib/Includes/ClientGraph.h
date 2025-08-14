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
};

struct ClientGraph;

enum class EditOperationType
{
	INVALID,
	FETCH,
	NODE_NEW,
	NODE_EDIT,
	NODE_DELETE,
	CONNECTION_CREATE_EDIT,
	CONNECTION_DELETE
};

// Abtract data structure for an operation in a Client Graph Edit Transaction. Links to next and previous operation if any.
struct GraphEditOp_Base
{
	EditOperationType type;
	GraphEditOp_Base* previous;
	GraphEditOp_Base* next;
};

struct GraphEditOp_Fetch : public GraphEditOp_Base
{
	SNodeGUID fetchedNodeID;
};

// Data structure specifying the new state of a created or existing node.
struct GraphEditOp_NodeNewEdit : public GraphEditOp_Base
{
	// Definition data for the node after creation or edition.
	SNodeDef def;

	// Only relevant for new nodes. Their ID within the Created Nodes collection of the Transaction.
	SNodeGUID createdNodeIndex;
};

/*
	Contains a set of operations to perform and the necessary data to build connections involving nodes created during the transaction.
	TODO The system can be made simpler once a arbitrary memory allocator is implemented.
*/
struct ClientGraphEditTransaction
{
	ClientGraph* TargetGraph = nullptr;

	struct Node
	{
		SNodeGUID ID = SNODE_INVALID_ID; // Invalid == New node.
		bool bDeleted = false;
		bool bFetched = false;

		Node* Parent = nullptr;
		SNodeConnectionAccessLevel AccessLevelToParent;
		SNodeConnectionAccessLevel AccessLevelFromParent;

		// Hints for node data to store when transaction is applied. Not all fields may be relevant.
		SNodeDef NodeDef;
	};

	/*
		"Deployed" wrapper around a connection definition, giving direct pointer access to involved Nodes in the transaction if any.
		By the end of the transaction construction Source HAS to be defined, and Destination unless connection was fetched and contains
		a valid destination ID.
	*/
	struct Connection
	{
		Node* Src = nullptr;
		Node* Dest = nullptr;

		SNodeConnectionAccessLevel AccessLevel;

		SNodeConnectionDef FetchedDef;

		bool bDeleted = false;
	};

	// Memory the allocator may use for its common o
	MemoryAllocator TransactionOperationsMemory;

	// Linked list of generated operations for this transaction.
	// Every new operation should be allocated from the Operations Memory.
	GraphEditOp_Base* OperationsListBegin = nullptr;
	GraphEditOp_Base* OperationsListEnd = nullptr;

	// Total number of generated operations. Used to double-check the validity of the transaction when applying it.
	size_t OperationsCount = 0;

	Node FetchedNodes[32];
	Node CreatedNodes[32];

	Connection FetchedConnections[32 * 32];
	Connection CreatedConnections[32 * 32];

	/*
		Loads an existing node from the parent graph so it may be involved in the transaction.
		Returns the fetched node within the transaction, usable to create other nodes or edit it conditionally.
	*/ 
	Node* FetchGraphNode(SNodeGUID NodeID);

	/*
		Creates a new node with the passed definition.
		Def Parent ID not taken into account. Node ID should be defined if editing an existing node.
		Returns the newly created node within the transaction, usable to create further nodes or edit it conditionally.
	*/
	Node* CreateNode(SNodeDef NewNodeDef, Node* Parent);

	/*
		Edits a node that was fetched or created earlier in the transaction.
		Returns whether the operation was successfully added.
	*/
	bool EditNode(SNodeDef NewNodeDef, Node* TargetNode, Node* NewParent = nullptr);

	/*
		Deletes a node from the transaction.
		If the node was present on the graph before the transaction, will record the node's ID not existing as a post-requisite of the transaction. 
		Returns whether the operation was successfully added.
	*/
	bool DeleteNode(Node* ToBeDeleted);

	/*
		Adds or edits a connection from a source node to a target node based on the passed connection def.
		Returns whether the operation was successfully added.
	*/
	bool AddOrEditConnection(Node* SourceNode, Node* TargetNode, SNodeConnectionDef ConnectionDef);

	/*
		Deletes the connection from the Source node to the Target node.
		Returns whether the operation was successfully added.
	*/
	bool DeleteConnection(Node* SourceNode, Node* TargetNode);

	/*
		Direct addition of an operation to the operation collection.
		It is not recommended to do so but rather make use of the other functions available which maintain an internal
		"deployed" graph that guarantees operational integrity of the transaction.
	*/
	void AddOperation(GraphEditOp_Base* NewOp);
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

	// Returns the connections from the passed Node ID. Returns the total amount of connections.
	size_t GetNodeConnections(SNodeGUID NodeID, SNodeConnectionDef* NodeConnectionsBuffer, size_t NodeIDBufferSize);

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