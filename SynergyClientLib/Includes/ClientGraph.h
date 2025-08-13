// Contains symbols that outline how the Client models its own view of the Graph they are currently working on while connected to a server.

#include "Graph/SynergyGraph.h"

// The Client essentially needs to try and predict where the user will attempt to travel to next on the graph and keep that data quickly
// accessible, while also providing a potentially very large storage capacity for large graphs, with our without the help of a server.

// On top of this it must provide utilities for building graph editing transactions.

// To this end, the client maintains an interface to the rest of the app to server common commands and being able to switch into Edit Mode
// to conveniently build change operations and form a transaction that can be applied or sent to the server for approval.

/*
	Contains the entire local state of the graph and provides interface functions to process common commands in the Synergy system for
	finding nodes, interacting with them, and creating change requests.
*/
struct ClientGraph
{

};