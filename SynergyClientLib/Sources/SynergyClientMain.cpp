#include "SynergyClient.h"
#include "SynergyCore.h"

#include <iostream> // TODO instead of using iostream the user of the library should provide
// contextual data so it can use any logging solution it wants.

DLL_EXPORT void Hello()
{
	std::cout << "Hello World from Synergy Client Lib !" << "\n";
	std::cout << "VERSION 0.1 - IN DEV\n";
}