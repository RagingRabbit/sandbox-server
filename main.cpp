#include <iostream>

#include "server.h"

int main()
{
	Server::init();
	std::cout << "Server initialized" << std::endl;

	while (Server::isRunning())
	{
		Server::update();
	}

	Server::terminate();
	std::cout << "Server terminated" << std::endl;

	return 0;
}