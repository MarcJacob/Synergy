// Main include for working with Synergy Node Graphs.

#include <stdint>

// Unique identifier for a node across its entire Tree.
typedef uint64_t SNodeGUID
// Invalid Node ID used to represent non-existent nodes or invalid operation results.
constexpr SNodeGUID SNODE_INVALID_ID = ~0;

/*
	Structured representation of a node.
	This *represents* a node in a readable, structured format, but it is probably not the format the
	node's data is stored with in memory with.
*/ 
struct SNodeDef
{
	// Unique identifier for this node within its Tree.
	SNodeGUID id = SNODE_INVALID_ID;

	// ID of the parent node if any.
	SNodeGUID parentID = SNODE_INVALID_ID;

	// Name of the node, not necessarily unique. Only supports ASCII characters.
	char name[64];
};

// Defines the access level of a node connection.
enum class SNodeConnectionAccessLevel
{
	OPEN, // OPEN connections grant access rights to the destination node if the user has access rights to the source node and are PUBLIC otherwise.
	PUBLIC, // PUBLIC connections grant visibility to the destination node if the user has visibility of the source node.
	INTERNAL, // INTERNAL connections are visible only if the user has access rights to the source node, and only grant visibility of the destination node.
	PRIVATE, // PRIVATE connections are visible only if the user has access rights to both the source and destination node.
};

/*
	Structured representation of a node connection.
	Connections are either child - parent or arbitrary bidirectionnal connections between two nodes driving how visibility and access
	works between those nodes according to whatever rights the user possesses relative to one of or both partners.
	This *represents* a connection in a readable, structured format but it is probably 
	not the format the connection's data is stored in memory with.
*/
struct SNodeConnectionDef
{
	// Access level granted by this connection.
	SNodeConnectionAccessLevel accessLevel = SNodeConnectionAccessLevel::OPEN;
	
	// Partner nodes of this connection.
	SNodeGUID nodeID_A, nodeID_B;

	// Whether this connection is a Parent-Child connection.
	bool bIsParentChildConnection = false;
};