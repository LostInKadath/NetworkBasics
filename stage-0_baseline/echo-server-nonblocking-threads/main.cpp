#include <iostream>
#include <cstring>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#include <Descriptor.hpp>

using NetworkBasics::common::Descriptor;

namespace consts
{
	constexpr const uint16_t serverPort = 23999;
	constexpr const int incomingConnections = 3;	// May be adjusted by the kernel
}

int main(int argc, char* argv[])
{
	// Create a socket in kernel space.

	const auto serverSocket = Descriptor{ socket(
		AF_INET,		// IPv4
		SOCK_STREAM,	// TCP
		0				// number from /etc/protocols
	) };

	if (*serverSocket == -1)
	{
		std::cerr << "Failed to create socket: " << std::strerror(errno) << std::endl;
		return 1;
	}

	// IPv4 Internet domain socket address
	struct sockaddr_in serverAddress{
		.sin_family = AF_INET,					// IPv4
		.sin_port = htons(consts::serverPort),	// Port number in network byte order
		.sin_addr = { INADDR_ANY }				// Accept connections from any IP address
	};

	std::cout << "Binding server address:" << consts::serverPort << " to a socket " << *serverSocket << "...\n";

	// Typically one needs to set SO_REUSEADDR, because Linux doesn't release the socket instantly.
	if (const int res = bind(*serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)); res != 0)
	{
		std::cerr << "Server couldn't bind address to a socket: " << std::strerror(errno) << std::endl;
		return -1;
	}

	std::cout << "Marking the socket " << *serverSocket << " as ready to accept incoming connections on port " << consts::serverPort << "...\n";

	if (const int res = listen(*serverSocket, consts::incomingConnections); res != 0)
	{
		std::cerr << "Server couldn't start accepting incoming connections: " << std::strerror(errno) << std::endl;
		return -1;
	}

	// From now the server is ready to accept incoming connections.
	// When a client wants to connect, a three-way handshake is done between them and this server.
	// It's done on the Transport Layer, by a kernel -- no need in accept() in user-space.
	// After the connection is established, it goes into a queue of pending connections.
	// The client can send application data and even receive ACKs immediately after the handshake completes.
	// Every connection has its own buffers -- a queue of incoming packets. The kernel queues received bytes until the application calls accept() and later recv().

	std::vector<std::thread> clientThreads;
	
	while (true) try
	{
		// accept() extracts the first connection from the queue of pending connections.
		// A new socket is created with the same socket type protocol and address family.
		// A new file descriptor is allocated for that socket and returned to the client.
		// If there are no pending connections, accept() will block (common case) or fail (if the socket is created with O_NONBLOCK).
		auto clientSocket = Descriptor{ accept(
			*serverSocket,	// the socket from socket()+bind()+listen()
			nullptr,		// sockaddr, where the address of the connecting socket shall be returned
			nullptr			// socklen_t, the length of the stored address
		)};
		if (*clientSocket < 0)
		{
			std::cerr << "Server couldn't accept the incoming connection: " << std::strerror(errno) << std::endl;
			throw std::runtime_error("Failed to accept incoming connection!");
		}

		std::cout << "Client connected! Its socket descriptor = " << *clientSocket << "\n";
		clientThreads.emplace_back([clientSocket = std::move(clientSocket)]
		{
			char buffer[32] = {0};

			// If there are no incoming messages, recv() will block (common case) or fail (if the socket is created with O_NONBLOCK).
			while (true)
			{
				const auto receivedBytes = recv(
					*clientSocket,		// the new client socket
					buffer,				// buffer
					sizeof(buffer),		// buffer size
					0);					// flags

				if (receivedBytes < 0)
				{
					std::cerr << "Error while reading message from the client " << *clientSocket << ": " << std::strerror(errno) << '\n';
					break;
				}
				
				if (receivedBytes == 0)
				{
					std::cout << "Client " << *clientSocket << " has disconnected.\n";
					break;
				}
				
				std::cout << "Message from client " << *clientSocket << ":\n";
				std::cout.write(buffer, receivedBytes);
				std::cout << std::endl;
			}
		});
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	std::cout << "Server is shutting down...\n";

	for (auto& thread : clientThreads)
	{
		if (thread.joinable())
			thread.join();
	}
	clientThreads.clear();

	std::cout << "Server is shut down\n";
	return 0;
}
