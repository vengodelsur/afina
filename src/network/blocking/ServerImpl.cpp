#include "ServerImpl.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <pthread.h>
#include <signal.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <afina/Storage.h>

#include <afina/execute/Command.h>
#include "../../protocol/Parser.h"

#include <algorithm>
#include <sstream>

namespace Afina {
namespace Network {
namespace Blocking {

void *ServerImpl::RunAcceptorProxy(void *p) {
    ServerImpl *srv = reinterpret_cast<ServerImpl *>(p);
    try {
        srv->RunAcceptor();
    } catch (std::runtime_error &ex) {
        std::cerr << "Server acception fails: " << ex.what() << std::endl;
    }
    return 0;
}

void *ServerImpl::RunConnectionProxy(void *p) {
    ServerImpl *srv;
    int client_socket;

    ConnectionThreadInfo *parameters =
        reinterpret_cast<ConnectionThreadInfo *>(p);

    srv = parameters->server;
    client_socket = parameters->client_socket;

    try {
        srv->RunConnection(client_socket);
    } catch (std::runtime_error &ex) {
        std::cerr << client_socket << "Connection fails: " << ex.what()
                  << std::endl;
    }

    close(client_socket);

    delete parameters;
    std::lock_guard<std::mutex> lock(srv->connections_mutex);
    srv->connections.erase(pthread_self());

    return 0;
}

// See Server.h
ServerImpl::ServerImpl(std::shared_ptr<Afina::Storage> ps) : Server(ps) {}

// See Server.h
ServerImpl::~ServerImpl() {}

// See Server.h
void ServerImpl::Start(uint32_t port, uint16_t n_workers) {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;

    // If a client closes a connection, this will generally produce a SIGPIPE
    // signal that will kill the process. We want to ignore this signal, so
    // send()
    // just returns -1 when this happens.
    sigset_t sig_mask;
    sigemptyset(&sig_mask);
    sigaddset(&sig_mask, SIGPIPE);
    if (pthread_sigmask(SIG_BLOCK, &sig_mask, NULL) != 0) {
        throw std::runtime_error("Unable to mask SIGPIPE");
    }

    // Setup server parameters BEFORE thread created, that will guarantee
    // variable value visibility
    max_workers = n_workers;
    listen_port = port;

    // The pthread_create function creates a new thread.
    //
    // The first parameter is a pointer to a pthread_t variable, which we can
    // use
    // in the remainder of the program to manage this thread.
    //
    // The second parameter is used to specify the attributes of this new thread
    // (e.g., its stack size). We can leave it NULL here.
    //
    // The third parameter is the function this thread will run. This function
    // *must*
    // have the following prototype:
    //    void *f(void *args);
    //
    // Note how the function expects a single parameter of type void*. We are
    // using it to
    // pass this pointer in order to proxy call to the class member function.
    // The fourth
    // parameter to pthread_create is used to specify this parameter value.
    //
    // The thread we are creating here is the "server thread", which will be
    // responsible for listening on port 23300 for incoming connections. This
    // thread,
    // in turn, will spawn threads to service each incoming connection, allowing
    // multiple clients to connect simultaneously.
    // Note that, in this particular example, creating a "server thread" is
    // redundant,
    // since there will only be one server thread, and the program's main thread
    // (the
    // one running main()) could fulfill this purpose.
    running.store(true);
    if (pthread_create(&accept_thread, NULL, ServerImpl::RunAcceptorProxy,
                       this) < 0) {
        throw std::runtime_error("Could not create server thread");
    }
}

// See Server.h
//all workers are signalled to stop
void ServerImpl::Stop() {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    running.store(false);
    Join();
}

// See Server.h
//block thread until all threads stop
void ServerImpl::Join() {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    pthread_join(accept_thread, 0);
    std::unique_lock<std::mutex> lock(connections_mutex);
    while (!connections.empty()) {
        connections_cv.wait(lock);
    }
}

// See Server.h
void ServerImpl::RunAcceptor() {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;

    // For IPv4 we use struct sockaddr_in:
    // struct sockaddr_in {
    //     short int          sin_family;  // Address family, AF_INET
    //     unsigned short int sin_port;    // Port number
    //     struct in_addr     sin_addr;    // Internet address
    //     unsigned char      sin_zero[8]; // Same size as struct sockaddr
    // };
    //
    // Note we need to convert the port to network order

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;           // IPv4
    server_addr.sin_port = htons(listen_port);  // TCP port number
    server_addr.sin_addr.s_addr = INADDR_ANY;   // Bind to any address

    // Arguments are:
    // - Family: IPv4
    // - Type: Full-duplex stream (reliable)
    // - Protocol: TCP
    int server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == -1) {
        throw std::runtime_error("Failed to open socket");
    }

    // when the server closes the socket, the connection must stay in the
    // TIME_WAIT state to
    // make sure the client received the acknowledgement that the connection has
    // been terminated.
    // During this time, this port is unavailable to other processes, unless we
    // specify this option
    //
    // This option lets kernel know that we are OK that multiple
    // threads/processes are listen on the
    // same port. In a such case kernel will balance input traffic between all
    // listeners (except those who
    // are closed already)
    int opts = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opts,
                   sizeof(opts)) == -1) {
        close(server_socket);
        throw std::runtime_error("Socket setsockopt() failed");
    }

    // Bind the socket to the address. In other words let kernel know data for
    // what address we'd
    // like to see in the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) == -1) {
        close(server_socket);
        throw std::runtime_error("Socket bind() failed");
    }

    // Start listening. The second parameter is the "backlog", or the maximum
    // number of
    // connections that we'll allow to queue up. Note that listen() doesn't
    // block until
    // incoming connections arrive. It just makes the OS aware that this process
    // is willing
    // to accept connections on this socket (which is bound to a specific IP and
    // port)
    if (listen(server_socket, 5) == -1) {
        close(server_socket);
        throw std::runtime_error("Socket listen() failed");
    }

    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t sinSize = sizeof(struct sockaddr_in);
    while (running.load()) {
        std::cout << "network debug: waiting for connection..." << std::endl;

        // When an incoming connection arrives, accept it. The call to accept()
        // blocks until
        // the incoming connection arrives
        if ((client_socket = accept(
                 server_socket, (struct sockaddr *)&client_addr, &sinSize)) ==
            -1) {
            close(client_socket);
            throw std::runtime_error("Socket accept() failed");
        }

        // TODO: Start new thread and process data from/to connection
        {
            std::lock_guard<std::mutex> lock(connections_mutex);
            if (connections.size() + 1 > max_workers) {
                std::string msg = "Too many connections";
                if (send(client_socket, msg.data(), msg.size(), 0) <= 0) {
                    close(client_socket);
                    throw std::runtime_error("Socket send() failed");
                }
                close(client_socket);
            } else {
                pthread_t new_connection_thread;

                if (pthread_create(
                        &new_connection_thread, NULL,
                        ServerImpl::RunConnectionProxy,
                        new ConnectionThreadInfo(this, client_socket)) < 0) {
                    throw std::runtime_error("Can't create connection thread");
                }
                pthread_detach(new_connection_thread);
                connections.insert(new_connection_thread);
            }
        }
    }

    {
        std::unique_lock<std::mutex> lock(connections_mutex);
        while (!connections.empty()) {
            connections_cv.wait(lock);
        }
    }

    // Cleanup on exit...
    close(server_socket);
}

// See Server.h
void ServerImpl::RunConnection(int client_socket) {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    // TODO: All connection work is here
    char chunk[CHUNK_SIZE] = "";
    ssize_t read_length = 0;
    ssize_t read_counter = 0;
    ssize_t sent_length = 0;
    ssize_t sent_counter = 0;
    size_t parsed_length = 0;
    ssize_t answer_size = 0;
    bool command_is_parsed = false;
    Protocol::Parser parser;
    uint32_t command_body_size;
    std::unique_ptr<Execute::Command> resulting_command;
    std::string arguments;
    std::string answer = "";
    bool no_errors = true;

    while (running.load() && no_errors) {
        try {
            command_is_parsed = false;
            answer = std::string(
                "Problem when parsing command ");  // it will stay if next step
                                                   // throws an exception
            do {
                if (!ReadCommand(client_socket, chunk, read_counter,
                                 read_length, parsed_length, command_is_parsed,
                                 parser)) {
                    Close(client_socket);
                    return;
                }

            } while (!command_is_parsed);

            resulting_command = parser.Build(command_body_size);
            parser.Reset();

            answer = std::string("Problem when extracting arguments ");

            if (!ExtractArguments(client_socket, chunk, read_counter,
                                  command_body_size, arguments)) {
                Close(client_socket);
                return;
            }
            answer = std::string("Problem when executing command ");

            resulting_command->Execute(*pStorage, arguments, answer);
        } catch (std::runtime_error &ex) {
            answer = std::string("ERROR ") + answer + ex.what() +
                     std::string("\r\n");
            no_errors = false;
        }

        answer += std::string("\r\n");

        // send answer
        answer_size = answer.size();
        if (answer.size() > 0) {
            answer_size = answer.size();
            sent_counter = 0;
            while (answer_size > 0) {
                sent_length = send(client_socket, answer.data() + sent_counter,
                                   answer.size() - sent_counter, 0);
                if (sent_length <= 0) {
                    Close(client_socket);
                    return;
                }
                answer_size -= sent_length;
                sent_counter += sent_length;
            }
        }
    }

    Close(client_socket);
}

bool ServerImpl::ReadCommand(int client_socket, char *chunk,
                             ssize_t &read_counter, ssize_t &read_length,
                             size_t &parsed_length, bool &command_is_parsed,
                             Protocol::Parser &parser) {
    read_length =
        recv(client_socket, chunk + read_counter, CHUNK_SIZE - read_counter, 0);
    read_counter += read_length;

    if (read_length < 0) {
        Close(client_socket);
        throw std::runtime_error("Socket recv() failed");

    }
    if (read_counter == 0) {
        // ok but message is empty
        Close(client_socket);
        return false;
    }

    /* from parser documentation:
    Parse(const std::string &input, size_t &parsed)
    * @param input string to be added to the parsed input
    * @param size number of bytes in the input buffer that could be read
    * @param parsed output parameter tells how many bytes was consumed
    from the
    * string
   * @return true if command has been parsed out
    The resulting command is stored inside Parser instance*/
    command_is_parsed = parser.Parse(chunk, read_counter, parsed_length);

    // in case only part of the chunk has been parsed:
    std::memmove(chunk, chunk + parsed_length,
                 CHUNK_SIZE - parsed_length);  // destination, source, number
    read_counter -= parsed_length;
    return true;
}

bool ServerImpl::ExtractArguments(int client_socket, char *chunk,
                                  ssize_t &read_counter,
                                  uint32_t &command_body_size,
                                  std::string &arguments) {
    if (command_body_size) {
        command_body_size += 2;  // \r\n
    }
    while (command_body_size > read_counter) {
        arguments.append(chunk, read_counter);
        command_body_size -= read_counter;
        read_counter = recv(client_socket, chunk, CHUNK_SIZE * sizeof(char), 0);
        if (read_counter < 0) {
            Close(client_socket);
            throw std::runtime_error("Socket recv() failed");

        }
        if (read_counter == 0) {
            // ok but message is empty
            return false;
        }
    }

    arguments.append(chunk, command_body_size);
    std::memmove(chunk, chunk + command_body_size,
                 read_counter - command_body_size);
    read_counter -= command_body_size;
    if (command_body_size > 2) {
        arguments = arguments.substr(0, command_body_size - 2);  // \r\n
    }

    return true;
}

void ServerImpl::Close(int client_socket) {
    std::unique_lock<std::mutex> lock(connections_mutex);
    connections.erase(pthread_self());
    connections_cv.notify_one();
}

}  // namespace Blocking
}  // namespace Network
}  // namespace Afina
