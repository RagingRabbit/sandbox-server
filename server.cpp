#include "server.h"

#include <iostream>
#include <vector>

#include <SDL2/SDL_net.h>

namespace Server
{
	struct Connection
	{
		TCPsocket socket;
		int timeout;
		int id;

		Connection(TCPsocket socket, int timeout, int id)
			: socket(socket), timeout(timeout), id(id) {}
	};

	struct Packet
	{
		void* data;
		int length;

		Packet(void* data, int length)
			: data(data), length(length) {}
	};

	constexpr int PORT = 22222;
	constexpr int PACKET_SIZE = 2048;
	constexpr int MAX_PLAYERS = 25;
	constexpr int TIMEOUT_LENGTH = 10000;

	int connID = 0;
	int numPlayers = 0;
	IPaddress hostAddress;
	TCPsocket server;

	std::vector<Connection> connections;
	SDLNet_SocketSet sockets;
	char packetData[PACKET_SIZE];

	bool running;

	void init()
	{
		SDL_Init(SDL_INIT_EVERYTHING);
		SDLNet_Init();

		SDLNet_ResolveHost(&hostAddress, nullptr, PORT);
		sockets = SDLNet_AllocSocketSet(MAX_PLAYERS);
		server = SDLNet_TCP_Open(&hostAddress);

		running = true;
	}

	void sendPacket(Packet packet, Connection& connection)
	{
		SDLNet_TCP_Send(connection.socket, packet.data, packet.length);
	}

	void disconnectClient(int index)
	{
		// TODO Inform other clients
		TCPsocket socket = connections[index].socket;
		SDLNet_TCP_DelSocket(sockets, socket);
		SDLNet_TCP_Close(socket);
		connections.erase(connections.begin() + index);
		numPlayers--;
	}

	void onClientConnect(Connection& connection)
	{
		std::cout << "Player " << connection.id << " has connected" << std::endl;

		sprintf_s(packetData, "hello\n", connection.id);
		sendPacket(Packet(packetData, 7), connection);
	}

	void onPacketReceived(Packet packet, Connection& connection)
	{
		std::cout << "Packet received: " << (char*)packet.data << std::endl;
		if (strcmp((char*)packet.data, "ping"))
		{
			sendPacket(Packet((void*)"pong", 5), connection);
		}
	}

	void update()
	{
		// Check incoming connections
		TCPsocket connectingClient = SDLNet_TCP_Accept(server);
		if (connectingClient)
		{
			if (numPlayers < MAX_PLAYERS)
			{
				SDLNet_TCP_AddSocket(sockets, connectingClient);
				int connectionID = connID++;
				connections.push_back(Connection(connectingClient, SDL_GetTicks(), connectionID));
				numPlayers++;

				onClientConnect(connections.back());
			}
		}

		// Check incoming data
		while (SDLNet_CheckSockets(sockets, 0) > 0)
		{
			for (int i = 0; i < connections.size(); i++)
			{
				if (SDLNet_SocketReady(connections[i].socket))
				{
					connections[i].timeout = SDL_GetTicks();
					int length = SDLNet_TCP_Recv(connections[i].socket, packetData, PACKET_SIZE);

					onPacketReceived(Packet(packetData, length), connections[i]);
				}
			}
		}

		// Disconnect timeouted connections
		for (int i = 0; i < connections.size(); i++)
		{
			if (SDL_GetTicks() - connections[i].timeout > TIMEOUT_LENGTH)
			{
				disconnectClient(i);
				std::cout << "Connection " << connections[i].id << " timeouted after " << TIMEOUT_LENGTH << "milliseconds" << std::endl;
			}
		}

		SDL_Delay(1);
	}

	void terminate()
	{
		while (!connections.empty())
		{
			disconnectClient(0);
		}

		SDLNet_FreeSocketSet(sockets);
		SDLNet_TCP_Close(server);
		SDLNet_Quit();
		SDL_Quit();
	}

	bool isRunning()
	{
		return true;
	}
}