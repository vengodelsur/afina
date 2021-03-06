#ifndef AFINA_NETWORK_BLOCKING_SERVER_H
#define AFINA_NETWORK_BLOCKING_SERVER_H

#include <pthread.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_set>

#include <afina/network/Server.h>
#include "../../protocol/Parser.h"

namespace Afina {
namespace Network {
namespace Blocking {

/**
 * # Network resource manager implementation
 * Server that is spawning a separate thread for each connection
 */
class ServerImpl : public Server {
   public:
    ServerImpl(std::shared_ptr<Afina::Storage> ps);
    ~ServerImpl();

    // See Server.h
    void Start(uint32_t port, uint16_t workers) override;

    // See Server.h
    void Stop() override;

    // See Server.h
    void Join() override;

   protected:
    /**
     * Method is running in the connection acceptor thread
     */
    void RunAcceptor();

    /**
     * Method is running for each connection
     */
    void RunConnection(int client_socket);
    bool ReadCommand(int client_socket, char *chunk, ssize_t &read_counter,
                     ssize_t &read_length, size_t &parsed_length,
                     bool &command_is_parsed, Protocol::Parser &parser);
    bool ExtractArguments(int client_socket, char *chunk, ssize_t &read_counter,
                          uint32_t &command_body_size, std::string &arguments);

    void Close(int client_socket);

   private:
    static void *RunAcceptorProxy(void *p);
    static void *RunConnectionProxy(void *p);

    // Atomic flag to notify threads when it is time to stop. Note that
    // flag must be atomic in order to safely publish changes cross thread
    // bounds
    std::atomic<bool> running;

    // Thread that is accepting new connections
    pthread_t accept_thread;

    // Maximum number of client allowed to exists concurrently
    // on server, permits access only from inside of accept_thread.
    // Read-only
    uint16_t max_workers;

    // Port to listen for new connections, permits access only from
    // inside of accept_thread
    // Read-only
    uint32_t listen_port;

    // Mutex used to access connections list
    std::mutex connections_mutex;

    // Conditional variable used to notify waiters about empty
    // connections list
    std::condition_variable connections_cv;

    // Threads that are processing connection data, permits
    // access only from inside of accept_thread
    std::unordered_set<pthread_t> connections;

    const size_t CHUNK_SIZE = 2048;
    // needed for passing arguments in pthread_create like
    // http://man7.org/linux/man-pages/man3/pthread_create.3.html
    struct ConnectionThreadInfo {
        ServerImpl *server;
        int client_socket;

        ConnectionThreadInfo(ServerImpl *p, int number)
            : server(p), client_socket(number) {}
    };
};

}  // namespace Blocking
}  // namespace Network
}  // namespace Afina

#endif  // AFINA_NETWORK_BLOCKING_SERVER_H
