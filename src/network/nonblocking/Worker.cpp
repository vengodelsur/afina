#include "Worker.h"

#include <iostream>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "Utils.h"
#include <protocol/Parser.h>
#include <afina/execute/Command.h>
#include <cstring>

namespace Afina {
namespace Network {
namespace NonBlocking {

// See Worker.h
Worker::Worker(std::shared_ptr<Afina::Storage> ps) : _storage_ptr(ps){
    // TODO: implementation here
}

// See Worker.h
Worker::~Worker() {
    // TODO: implementation here
    Stop();
    Join();
}

// See Worker.h
void Worker::Start(int server_socket) {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;

    _running.store(true);
    _server_socket = server_socket;
    pthread_t thread;  // docs
    // the same way as RunAcceptor in ServerImpl for blocking server
    if (pthread_create(&thread, nullptr, Worker::RunWorkerProxy,
                       new WorkerInfo(this, _server_socket)) < 0) {
        throw std::runtime_error("Can't create server thread");
    }  // If attr is NULL, then the thread is created with default attributes.
}

// See Worker.h
void Worker::Stop() {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    _running.store(false);
}

// See Worker.h
void Worker::Join() {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    shutdown(
        _server_socket,
        SHUT_RDWR);  // further receptions and transmissions will be disallowed
    pthread_join(_thread, nullptr);
    // If retval is not NULL, then pthread_join() copies the exit status of the
    // target thread (i.e., the value that the target thread supplied to
    // pthread_exit(3)) into the location pointed to by retval.
}
void *Worker::RunWorkerProxy(void *p) {

    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    WorkerInfo* parameters = reinterpret_cast<WorkerInfo*>(p);
    Worker* worker = parameters->worker;
    int _server_socket = parameters->server_socket;
    worker->OnRun(_server_socket);
    return 0;
    // add try-catch
}

// See Worker.h
void Worker::OnRun(int server_socket) {  // read, write
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;

    // TODO: implementation here
    // 1. Create epoll_context here

    _server_socket = server_socket;
    _epoll_fd = epoll_create(_max_events);
    if (_epoll_fd < 0) {
        throw std::runtime_error("Can't create epoll context");
    }

    // 2. Add server_socket to context

    Connection* server_connection = new Connection(_server_socket, _running, _storage_ptr); 

    struct epoll_event event;
    event.events = EPOLLEXCLUSIVE | EPOLLHUP | EPOLLERR |
                   EPOLLIN;  // epoll_wait always waits for EPOLERR and
                             // EPOLLHUP, so it's not obligatory to set them in
                             // events
    event.data.ptr = server_connection; // Philipp doesn't approve using map
    // for storing connections by socket, using event field for user data is
    // recommended
    struct epoll_event events_chunk[_max_events];

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server_socket, &event) == -1) {
        throw std::runtime_error("Can't add server socket to context");
    }

    // 3. Accept new connections, don't forget to call make_socket_nonblocking on the client socket descriptor

    while (_running.load()) {
        // timeout -1 makes epoll_wait wait indefinitely
        int events_number = epoll_wait(_epoll_fd, events_chunk, _max_events, -1);
        // returns the number of fds ready for I/O, 0 if no fd became ready
        // during the timeout, -1 for error (errno set).
        /*events is memory where we store elements of type
         struct epoll_event {
               uint32_t     events;    // Epoll events
               epoll_data_t data;      // User data variable
           };
        */
        if (events_number == -1) {
            throw std::runtime_error("Error in epoll_wait");
            // for example, epfd is invalid or we don't have permission to write
            // to events memory
        }

        for (int i = 0; i < events_number; i++) {
            Connection *connection =
                reinterpret_cast<Connection *>(events_chunk[i].data.ptr);
            if (connection->socket == _server_socket) {
                int client_socket = accept(_server_socket, NULL, NULL);
                if (client_socket == -1) {
                    if ((errno != EWOULDBLOCK) && (errno != EAGAIN)) {
                        // "The socket is marked nonblocking and no connections
                        // are present to be accepted.  POSIX.1-2001 and
                        // POSIX.1-2008 allow either error to be returned for
                        // this case, and do not require these constants to have
                        // the same value, so a portable application should
                        // check for both possibilities".
                        close(_server_socket);
                        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, _server_socket,
                                  NULL);
                        // should we close epoll_fd here?
                        if (_running.load()) {
                            throw std::runtime_error("Can't accept");
                        }
                    }
                } else {
    // 4. Add connections to the local context
                    make_socket_non_blocking(client_socket); // uses fcntl function for managing file descriptor (sets O_NONBLOCK flag)
                    _connections.emplace_back(std::move(new Connection(client_socket, _running, _storage_ptr)));

                    event.data.ptr = _connections.back().get(); // get is unique_ptr member function returning pointer
                    event.events = EPOLLHUP | EPOLLERR | EPOLLIN | EPOLLOUT ;
                    
                    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
                        throw std::runtime_error("Can't add connection to epoll context");
                    }
                }
            } else {
    // 5. Process connection events
                int client_socket = connection->socket;
                if (events_chunk[i].events & EPOLLHUP ||
                    events_chunk[i].events & EPOLLERR) {
                    // EPOLLHUP hangup (for some channles it means unexpected
                    // close of the socket), EPOLERR stands for error condition
                    FinishWorkWithClient(client_socket);
                } else if (events_chunk[i].events & (EPOLLIN | EPOLLOUT)) { // file is avaliable for read/write operations
                    if (!connection->Process() or !connection->no_errors) {
                        FinishWorkWithClient(client_socket);
                    }
                } else {
                    FinishWorkWithClient(client_socket);
                    throw std::runtime_error("Event type doesn't fit any of expected ones");
                }
            }





        }
    }

    
    close(_epoll_fd);
}

void Worker::FinishWorkWithClient(int client_socket) {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    // should we close client socket here?
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_socket, NULL);
    for (auto iter = _connections.begin(); iter != _connections.end(); iter++) {
        if ((*iter)->socket == client_socket) {
            _connections.erase(iter);
            break;
        }
    }
}



}  // namespace NonBlocking
}  // namespace Network
}  // namespace Afina
