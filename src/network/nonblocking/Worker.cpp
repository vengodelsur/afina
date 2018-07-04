#include "Worker.h"

#include <iostream>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "Utils.h"

namespace Afina {
namespace Network {
namespace NonBlocking {

// See Worker.h
Worker::Worker(std::shared_ptr<Afina::Storage> ps) {
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
    pthread_t thread; //docs
    //the same way as RunAcceptor in ServerImpl for blocking server
    if (pthread_create(&thread, nullptr, Worker::RunWorkerProxy, new WorkerInfo(this, _server_socket)) < 0) {
        throw std::runtime_error("Can't create server thread");
    } // If attr is NULL, then the thread is created with default attributes.

}

// See Worker.h
void Worker::Stop() {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    _running.store(false);
}

// See Worker.h
void Worker::Join() {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    shutdown(_server_socket, SHUT_RDWR); //further receptions and transmissions will be disallowed
    pthread_join(_thread, nullptr);
    //If retval is not NULL, then pthread_join() copies the exit status of the target thread (i.e., the value that the target thread supplied to pthread_exit(3)) into the location pointed to by retval. 
}
void *Worker::RunWorkerProxy(void *p) {
    Worker *wrkr = reinterpret_cast<Worker *>(p);
    try {
        wrkr->OnRun(wrkr->_server_socket);
    } catch (std::runtime_error &ex) {
        std::cerr << "Worker fails: " << ex.what() << std::endl;
    }
    return 0;

}


// See Worker.h
void Worker::OnRun(int server_socket) { //read, write
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;

    // TODO: implementation here
    // 1. Create epoll_context here
    
    // 2. Add server_socket to context
    // 3. Accept new connections, don't forget to call make_socket_nonblocking on
    //    the client socket descriptor
    // 4. Add connections to the local context
    // 5. Process connection events
    //
    // Do not forget to use EPOLLEXCLUSIVE flag when register socket
    // for events to avoid thundering herd type behavior.
}

} // namespace NonBlocking
} // namespace Network
} // namespace Afina
