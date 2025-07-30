#include "SynergyServer.h"
#include "SynergyCore.h"

#include <iostream>

int main(int argc, char** argv)
{
	std::cout << "Hello World ! This is the Server.\n";

	SynergyCore::OutputHelloWorldFromCore();

	return 0;
}