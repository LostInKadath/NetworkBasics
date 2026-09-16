This is non-blocking echo server, based on `epoll` mechanism.

# Common pipeline

## Prerequisities

Create a server socket with `socket()`. This is a listening socket, it's created in kernel space.

To have non-blocking socket, we need either specify it explicitly, via `SOCK_NONBLOCK` flag, or later, by setting `O_NONBLOCK` flag via `fcntl()`:
```c++
const int flags = fcntl(socket, F_GETFL, 0);
fcntl(socket, F_SETFL, flags | O_NONBLOCK);
```

Bind the listening socket to a specific listening port by creating `struct sockaddr_in` and calling `bind()`.

Create an `epoll` instance with `epoll_create1()`. Now we have a descriptor to that instance.

Add the listening socket to `epoll`. For that we need to create `struct epoll_event`, specify events to monitor and call `epoll_ctl()`. \
This should be done before any other operations with listening socket. Otherwise some events could be lost.

While adding socket to `epoll` we can specify one of two modes -- level-based and edge-triggered:
- Level-based mode allows the app to get notifications, as long as the condition holds. It's the default mode.
- Edge-Triggered mode allows the app to get notification only once, as the condition changes its state. `EPOLLET` flag should be set in `epoll_event::events`.

Start listening the specified port by calling `listen()`. That marks the socket as ready to receive and process incoming connections.

## Monitoring events

Pre-create two containers -- for events and for clients connections.

After that organize a loop. Inside it call `epoll_wait()` (blocking or with timeout) and get a number of events.

Iterate over these events.

### Accepting incoming connections

If an event happened for the listening socket -- there are incoming connections.

When a client wants to connect, a three-way handshake is performed between the client and the server. This is done by kernel TCP stack, in a Transport layer, not by the application. \
While the 3WHS is not finished, the connection is in pending state (usually they say about a specific queue for pending connections located in Linux kernel, but [that's not true](https://arthurchiao.art/blog/tcp-listen-a-tale-of-two-queues)). \
After the 3WHS is finished, the connection is moved to the accept queue in Linux kernel. \
The client can send application data and even receive ACKs immediately after the handshake completes. Every connection has its own buffers -- a queue of incoming packets. The kernel puts received bytes into these buffers, until the application calls `accept()` and later `recv()`.

`accept()` pops the first connection from the accept queue and creates a new socket for it -- with the same socket type protocol and address family. A new descriptor is allocated for that socket and returned to the application. This new socket is dedicated to a connection for that specific incoming connection, while the original server socket still keeps listening for new connections.

This new client socket also needs to be non-blocking -- so `O_NONBLOCK` flag is set for it via `fcntl()`. After that the client socket is added to `epoll` via `epoll_ctl()` to monitor events on it. Also the client socket could be stored in a container for future work.

In edge-triggered mode one event is generated for all incoming connections -- so we need to call `accept()` for all pending connections, until we have `EAGAIN` or `EWOULDBLOCK` error. That means there are no pending connections for that moment.

### Processing events from the clients

If an event happened for any client socket -- we need to get the event type and process it in a proper way:
* `EPOLLHUP` -- hang up is detected for that descriptor;
* `EPOLLRDHUP` -- the peer has closed its end of the connection;
* `EPOLLIN` -- the associated descriptor is available for `read()` operations;
* `EPOLLOUT` -- the associated descriptor is available for `write()` operations;
* ...

### Reading from the socket

Allocate a buffer and call `recv()` repeatedly, reading data in portions. For non-blocking socket do that, until `EAGAIN` or `EWOULDBLOCK` is returned. If needed, data can also be sent via `send()`.


