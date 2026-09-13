#include <iostream>
#include <cstring>
#include <set>
#include <vector>

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

#include <Descriptor.hpp>

using NetworkBasics::common::Descriptor;
using NetworkBasics::common::DescriptorSet;

namespace consts
{
	constexpr const uint16_t serverPort = 23999;
	constexpr const int incomingConnections = 3;	// May be adjusted by the kernel
	constexpr const int maxEvents = 32;			// Maximum number of events to be returned by epoll_wait()
}

void setNonBlocking(int socket)
{
	const int flags = fcntl(socket, F_GETFL, 0);
	if (flags < 0)
	{
		std::cerr << "Failed to get flags for the socket " << socket << ": " << std::strerror(errno) << std::endl;
		throw std::runtime_error("Failed to get flags for the socket!");
	}
	if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		std::cerr << "Failed to set non-blocking mode for the socket " << socket << ": " << std::strerror(errno) << std::endl;
		throw std::runtime_error("Failed to set non-blocking mode for the socket!");
	}
}

void addToEpoll(int epollDescriptor, int socket)
{
	// We are interested in read events (EPOLLIN) and errors (EPOLLERR).
	// We are interested in EPOLLRDHUP to detect when the peer has closed its end of the connection.
	// EPOLLHUP is being detected automatically for the non-blocking socket, but we request it explicitly.
	// We switch on edge-triggered mode (EPOLLET) to avoid repeated notifications for the same event.
	struct epoll_event event{
		.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP,
		.data = { .fd = socket }
	};
	if (-1 == epoll_ctl(epollDescriptor, EPOLL_CTL_ADD, socket, &event))
	{
		std::cerr << "Failed to add socket " << socket << " to epoll: " << std::strerror(errno) << std::endl;
		throw std::runtime_error("Failed to add socket to epoll!");
	}
}

void drainPendingConnections(int serverSocket, int epollDescriptor, DescriptorSet& connectedClients)
{
	while (true)
	{
		std::cout << "Checking incoming connection request on socket " << serverSocket << "...\n";

		// accept() extracts the first connection from the queue of pending connections.
		// A new socket is created with the same socket type protocol and address family.
		// A new file descriptor is allocated for that socket and returned to the client.
		// If there are no pending connections, accept() will block (common case) or fail (if the socket is created with O_NONBLOCK).
		auto clientSocket = Descriptor{ accept(
			serverSocket,	// the socket from socket()+bind()+listen()
			nullptr,		// sockaddr, where the address of the connecting socket shall be returned
			nullptr			// socklen_t, the length of the stored address
		)};
		if (*clientSocket < 0)
		{
			const auto lastError = errno;
			if (lastError == EAGAIN || lastError == EWOULDBLOCK) // Actually EWOULDBLOCK==EAGAIN for non-blocking sockets
				// No more incoming connections in the queue.
				return;
			
			std::cerr << "Server couldn't accept the incoming connection: " << std::strerror(lastError) << std::endl;
			throw std::runtime_error("Failed to accept incoming connection!");
		}

		std::cout << "Client connected! Its socket descriptor = " << *clientSocket << "\n";

		// Set non-blocking mode for the new client socket, because we are using epoll in edge-triggered mode.
		setNonBlocking(*clientSocket);
		addToEpoll(epollDescriptor, *clientSocket);

		connectedClients.insert(std::move(clientSocket));
	}
}

void readFromSocket(int socket)
{
	char buffer[32] = {0};

	// If there are no incoming messages, recv() will fail for non-blocking socket.
	while (true)
	{
		// The last argument is flags=0
		const auto receivedBytes = recv(socket, buffer, sizeof(buffer), 0);

		if (receivedBytes < 0)
		{
			const auto lastError = errno;
			if (lastError == EAGAIN || lastError == EWOULDBLOCK)
				std::cout << "No more incoming messages in the queue for socket " << socket << '\n';
			else
				std::cerr << "Error while reading message from the client " << socket << ": " << std::strerror(errno) << '\n';
			return;
		}

		if (receivedBytes == 0)
		{
			std::cout << "Client " << socket << " has been disconnected.\n";
			return;
		}
		
		std::cout << "Message from client " << socket << ":\n";
		std::cout.write(buffer, receivedBytes);
		std::cout << std::endl;

		const auto sentBytes = send(socket, buffer, receivedBytes, 0);
		if (sentBytes <= 0)
		{
			std::cerr << "Error while sending message to the client " << socket << ": " << std::strerror(errno) << '\n';
			return;
		}
	}
}

int main(int argc, char* argv[])
{
	// Create a socket in kernel space.

	const auto serverSocket = Descriptor{ socket(
		AF_INET,						// IPv4
		SOCK_STREAM | SOCK_NONBLOCK,	// TCP, non-blocking for epoll
		0								// number from /etc/protocols
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

	// epoll instance -- I/O event notification facility
	const auto epollDescriptor = Descriptor{ epoll_create1(0) };	// flags = 0, optional
	if (-1 == *epollDescriptor)
	{
		std::cerr << "Failed to create epoll instance: " << std::strerror(errno) << std::endl;
		return -1;
	}

	addToEpoll(*epollDescriptor, *serverSocket);

	std::cout << "Marking the socket " << *serverSocket << " as ready to accept incoming connections on port " << consts::serverPort << "...\n";

	// listen should be called after adding the socket to epoll, because listen() may trigger an event if there are already pending connections in the queue.
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

	DescriptorSet connectedClientSockets;
	std::vector<epoll_event> events(consts::maxEvents);

	while (true) try
	{
		const int numEvents = epoll_wait(*epollDescriptor, events.data(), consts::maxEvents, -1);	// timeout = -1, wait indefinitely
		if (numEvents < 0)
		{
			std::cerr << "Error while waiting for events: " << std::strerror(errno) << std::endl;
			throw std::runtime_error("Failed to wait for events!");
		}

		for (int i = 0; i < numEvents; ++i)
		{
			if (events[i].data.fd == *serverSocket)
			{
				drainPendingConnections(*serverSocket, *epollDescriptor, connectedClientSockets);
				continue;
			}
			
			const auto clientSocketIt = connectedClientSockets.find(events[i].data.fd);
			if (clientSocketIt == connectedClientSockets.end())
			{
				std::cerr << "Unexpected event on socket " << events[i].data.fd << std::endl;
				throw std::runtime_error("Unexpected event!");
			}

			if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
			{
				std::cerr << "Error or hang-up on socket " << **clientSocketIt << std::endl;
				connectedClientSockets.erase(clientSocketIt);
				continue;
			}

			if (events[i].events & EPOLLIN)
			{
				std::cout << "Incoming message on socket " << **clientSocketIt << "...\n";
				readFromSocket(**clientSocketIt);
				continue;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	std::cout << "Server is shutting down...\n";
	return 0;
}
