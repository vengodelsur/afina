#ifndef AFINA_NETWORK_NONBLOCKING_WORKER_H
#define AFINA_NETWORK_NONBLOCKING_WORKER_H

#include <memory>
#include <atomic>
#include <pthread.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <afina/execute/Command.h>
#include "../../protocol/Parser.h"
#include <cstring>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>

namespace Afina {

// Forward declaration, see afina/Storage.h
class Storage;

namespace Network {
namespace NonBlocking {

const uint32_t EPOLLEXCLUSIVE = (1 << 28); // why do we have to set it explicitly?
enum class State {
    ReadCommand,
    ExtractArguments,
    SendAnswer
};

class Connection {
public:
    Connection(int _socket, std::atomic<bool>& running, std::shared_ptr<Afina::Storage> ps) : socket(_socket), state(State::ReadCommand), storage_ptr(ps), running(running) {
    }
    ~Connection(void) {
        close(socket);
    }

    
    int socket;    
    std::shared_ptr<Afina::Storage> storage_ptr;
    std::atomic<bool>& running;

    uint32_t command_body_size;
    std::unique_ptr<Execute::Command> resulting_command;
    
   
    bool no_errors = true;
    bool done = false;
    bool result;

    std::string body;
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

    void ReadCommand() {
        parsed_length = 0;
                   while (!parser.Parse(chunk, read_counter, parsed_length)) {
                       std::memmove(chunk, chunk + parsed_length, read_counter - parsed_length);
                       read_counter -= parsed_length;

                       read_length = recv(socket, chunk + read_counter, CHUNK_SIZE - read_counter, 0);
                       if (read_length <= 0) {
                           if ((errno == EWOULDBLOCK || errno == EAGAIN) && read_length < 0 && running.load()) {
                               done = true; result = true; continue;
                           } else {
                              
                               done = true; result = false; continue;
                           }
                       }

                       read_counter += read_length;
                   }
    }
    void ExtractArguments() {
        while (command_body_size > read_counter) {
                           body.append(chunk, read_counter);
                           command_body_size -= read_counter;
                           read_counter = 0;


                           ssize_t read_length = recv(socket, chunk, CHUNK_SIZE, 0);
                           if (read_length <= 0) {
                               if ((errno == EWOULDBLOCK || errno == EAGAIN) && read_length < 0 && running.load()) {
                                   done = true; result = true; continue;
                               } else {
                                   done = true; result = false; continue;
                               }
                           }

                           read_counter = read_length;
                       }
    }
    void SendAnswer() {
        while (sent_counter < answer.size()) {

                       sent_length = send(socket, answer.data() + sent_counter, answer.size() - sent_counter, 0);
                       if (sent_length < 0) {
                            if ((errno == EWOULDBLOCK || errno == EAGAIN) && running.load()) {
                                done = true; result = true; continue;
                            } else {
                                done = true; result = false; continue;
                            }
                        }

                       sent_counter += sent_length;
                   }
    }
    bool Process() {
        no_errors = true;
        done = false;
        while (no_errors & running.load()) {
            try {
               
               if (state == State::ReadCommand) {
                   ReadCommand();
                   if (done) return result;
                   std::memmove(chunk, chunk + parsed_length, read_counter - parsed_length);
                   read_counter -= parsed_length;

                   resulting_command = parser.Build(command_body_size);
                   command_body_size += 2;
                   parser.Reset();

                   body.clear();
                   state = State::ExtractArguments;
               }
               
               if (state == State::ExtractArguments) {
                   if (command_body_size > 2) {
                       ExtractArguments();
                       if (done) return result;
                       body.append(chunk, command_body_size);
                       std::memmove(chunk, chunk + command_body_size, read_counter - command_body_size);
                       read_counter -= command_body_size;

                       body = body.substr(0, body.length() - 2);
                   }

                   resulting_command->Execute(*storage_ptr, body, answer);
                   answer.append("\r\n");
                   state = State::SendAnswer;
               }
           } catch (std::runtime_error &e) {
               answer = std::string("SERVER_ERROR ") + e.what() + std::string("\r\n");
               std::cout << answer << std::endl;
               no_errors = false;
               state = State::SendAnswer;
           }
           if (state == State::SendAnswer) {
               if (answer.size() > 2) {
                   SendAnswer();
                   if (done) return result;
               }
               sent_counter = 0;
               state = State::ReadCommand;
           }
        }
        return false;
    }
};
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
    struct WorkerInfo {
        Worker* worker;
        int server_socket;

        WorkerInfo(Worker *p, int number)
            : worker(p), server_socket(number) {}
    };    

    static void *RunWorkerProxy(void *p);
    void FinishWorkWithClient(int client_socket);
    bool Process(Connection* connection);
    
    pthread_t _thread;
    int _server_socket;
    size_t _max_events = 32;

    std::atomic<bool> _running; //enum class to tell between stopping and stopped worker?
    std::vector<std::unique_ptr<Connection>> _connections;
   
    int _epoll_fd;
    std::shared_ptr<Afina::Storage> _storage_ptr;
    
};

} // namespace NonBlocking
} // namespace Network
} // namespace Afina
#endif // AFINA_NETWORK_NONBLOCKING_WORKER_H

