## Memo for the newbie author

#### What does "protected" stand for?

It's something betwwen "public" and "private". Children have access to the member. [cplusplus.com](http://www.cplusplus.com/forum/beginner/219643/).

#### What is "atomic"?

"Each instantiation and full specialization of std::atomic<> represents a type, that different threads can simultaneously operate on (their instances), without raising undefined behavior". More precisely, "Objects of atomic types are the only C++ objects that are free from data races; that is, if one thread writes to an atomic object while another thread reads from it, the behavior is well-defined. In addition, accesses to atomic objects may establish inter-thread synchronization and order non-atomic memory accesses as specified by std::memory_order." [Stackoverflow] (https://stackoverflow.com/questions/31978324/what-exactly-is-stdatomic)

#### reinterpret_cast? Sorry, what?

It is a compiler directive which instructs the compiler to treat the sequence of bits (object representation) of expression as if it had the type between brackets. [cppreference] (https://en.cppreference.com/w/cpp/language/reinterpret_cast)

#### What is masking sigpipe?

pthread_sigmask() function is just like sigprocmask() which is used to fetch and/or change the signal mask of the calling thread. The signal mask is the set of signals whose delivery is currently blocked for the caller. [man7.org] (http://man7.org/linux/man-pages/man2/sigprocmask.2.html) SIGPIPE is the "broken pipe" signal, which is sent to a process when it attempts to write to a pipe whose read end has closed (or when it attempts to write to a socket that is no longer open for reading), but not vice versa. [Quora] (https://www.quora.com/What-are-SIGPIPEs)

