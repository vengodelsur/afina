## Memo for the newbie author

#### Why is function pretty?!

\_\_PRETTY\_FUNCTION\_\_ gives the name of the current function and its signature :)

#### What is masking sigpipe?

pthread_sigmask() function is just like sigprocmask() which is used to fetch and/or change the signal mask of the calling thread. The signal mask is the set of signals whose delivery is currently blocked for the caller. [man7.org](http://man7.org/linux/man-pages/man2/sigprocmask.2.html) SIGPIPE is the "broken pipe" signal, which is sent to a process when it attempts to write to a pipe whose read end has closed (or when it attempts to write to a socket that is no longer open for reading), but not vice versa. [Quora](https://www.quora.com/What-are-SIGPIPEs)

#### What does htons do?

The htons() function converts the unsigned short integer hostshort from host byte order to network byte order. [linux.die.net](https://linux.die.net/man/3/htons)

#### What is epoll?

[Wikipedia (in Russian)](https://ru.wikipedia.org/wiki/Epoll)

#### Erm... SO_REUSEADDR...
SO_REUSEADDR indicates that the rules used in validating addresses supplied in a bind call should allow reuse of local addresses. For AF_INET sockets this means that a socket may bind, except when there is an active listening socket bound to the address. [man7.org](http://man7.org/linux/man-pages/man7/socket.7.html)

