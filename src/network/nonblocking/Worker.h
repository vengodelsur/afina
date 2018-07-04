#ifndef AFINA_NETWORK_NONBLOCKING_WORKER_H
#define AFINA_NETWORK_NONBLOCKING_WORKER_H

#include <memory>
#include <pthread.h>
#include <atomic>
namespace Afina {

// Forward declaration, see afina/Storage.h
class Storage;

namespace Network {
namespace NonBlocking {

/**
 * # Thread running epoll
 * On Start spaws background thread that is doing epoll on the given server
 * socket and process incoming connections and its data
 */
class Worker {
public:
    Worker(std::shared_ptr<Afina::Storage> ps);
    ~Worker();

    /**
     * Spaws new background thread that is doing epoll on the given server
     * socket. Once connection accepted it must be registered and being processed
     * on this thread
     */
    void Start(int server_socket);

    /**
     * Signal background thread to stop. After that signal thread must stop to
     * accept new connections and must stop read new commands from existing. Once
     * all readed commands are executed and results are send back to client, thread
     * must stop
     */
    void Stop();

    /**
     * Blocks calling thread until background one for this worker is actually
     * been destoryed
     */
    void Join();

protected:
    
    /**
     * Method executing by background thread
     */
    void OnRun(int server_socket);

private:

    static void *RunWorkerProxy(void *p);
    //Each worker is run in its own thread
    pthread_t _thread;

    int _server_socket;
    size_t _max_connections = 64;

    std::atomic<bool> _running; //enum class to tell between stopping and stopped worker?
   
    
};

} // namespace NonBlocking
} // namespace Network
} // namespace Afina
#endif // AFINA_NETWORK_NONBLOCKING_WORKER_H

