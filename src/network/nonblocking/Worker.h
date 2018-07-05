#ifndef AFINA_NETWORK_NONBLOCKING_WORKER_H
#define AFINA_NETWORK_NONBLOCKING_WORKER_H

#include <afina/execute/Command.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "../../protocol/Parser.h"

namespace Afina {

class Storage;

namespace Network {
namespace NonBlocking {
const uint32_t EPOLLEXCLUSIVE =
    (1 << 28);  // why do we have to set it explicitly?

enum class State { ReadCommand, ExtractArguments, SendAnswer };

class Connection {
   public:
    Connection(int fd, std::atomic<bool>& running,
               std::shared_ptr<Afina::Storage> ps)
        : socket(fd),
          state(State::ReadCommand),
          storage_ptr(ps),
          running(running) {}
    ~Connection() { close(socket); }

    int socket;
    std::shared_ptr<Afina::Storage> storage_ptr;
    std::atomic<bool>& running;

    uint32_t command_body_size;
    std::unique_ptr<Execute::Command> resulting_command;

    bool done = false;
    bool result;

    std::string command_body;
    std::string answer;

    Protocol::Parser parser;

    State state;

    static const size_t CHUNK_SIZE = 2048;
    char chunk[CHUNK_SIZE];
    size_t read_counter;
    size_t sent_counter = 0;
    size_t parsed_length;
    ssize_t read_length;
    ssize_t sent_length;

    bool ReadStep(char* start, size_t size);
    bool ReadCommandStep();
    bool ExtractArgumentsStep();
    bool SendAnswerStep();
    bool Process(uint32_t events);
};
class Worker {
   public:
    Worker(std::shared_ptr<Afina::Storage> ps);
    ~Worker();
    Worker(const Worker& w) : _storage_ptr(w._storage_ptr){};

    /**
     * Spaws new background thread that is doing epoll on the given server
     * socket. Once connection accepted it must be registered and being
     * processed
     * on this thread
     */
    void Start(int server_socket);

    /**
     * Signal background thread to stop. After that signal thread must stop to
     * accept new connections and must stop read new commands from existing.
     * Once
     * all readed commands are executed and results are send back to client,
     * thread
     * must stop
     */
    void Stop();

    /**
     * Blocks calling thread until background one for this worker is actually
     * been destoryed
     */
    void Join();

    pthread_t thread;

   protected:
    /**
     * Method executing by background thread
     */
    void OnRun(int server_socket);

   private:
    struct WorkerInfo {
        Worker* worker;
        int server_socket;

        WorkerInfo(Worker* p, int number) : worker(p), server_socket(number) {}
    };

    static void* RunWorkerProxy(void* p);
    void AddConnection(int client_socket, epoll_event& event);
    void FinishWorkWithClient(int client_socket);
    void CleanUp();

    pthread_t _thread;
    int _server_socket;
    size_t _max_events = 32;

    std::atomic<bool> _running;
    std::vector<std::unique_ptr<Connection>> _connections;

    int _epoll_fd;
    std::shared_ptr<Afina::Storage> _storage_ptr;
};

}  // namespace NonBlocking
}  // namespace Network
}  // namespace Afina
#endif  // AFINA_NETWORK_NONBLOCKING_WORKER_H
