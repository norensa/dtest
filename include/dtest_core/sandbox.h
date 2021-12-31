#pragma once

#include <mutex>
#include <unordered_map>
#include <exception>
#include <dtest_core/memory.h>
#include <sys/socket.h>
#include <dtest_core/network.h>
#include <functional>
#include <dtest_core/message.h>
#include <dtest_core/buffer.h>

namespace dtest {

struct ResourceSnapshot {
    struct Quantity {
        size_t size = 0;
        size_t count = 0;
    };

    struct {
        Quantity allocate;
        Quantity deallocate;
    } memory;

    struct {
        Quantity send;
        Quantity receive;
    } network;
};

class Sandbox {

    friend class CallStack;
    friend class Memory;
    friend class Network;

private:

    static void __signalHandler(int sig);

    std::mutex _mtx;
    bool _enabled = true;
    size_t _counter = 1;

    Socket _serverSocket;
    Socket _clientSocket;

    Memory _memory;
    Network _network;

    int _saved_stdio[3];
    int _sandboxed_stdio[3];

    void _sandbox_stdio(const Buffer &in);

    void _unsandbox_stdio(Buffer &out, Buffer &err);

public:

    class Options {
        friend class Sandbox;

    private:
        bool _fork = true;
        Buffer _in;
        Buffer _out;
        Buffer _err;

    public:

        Options & fork(bool val) {
            _fork = val;
            return *this;
        }

        Options & input(const Buffer &in) {
            _in = in;
            return *this;
        }
        Options & input(Buffer &&in) {
            _in = std::move(in);
            return *this;
        }

        Buffer & output() {
            return _out;
        }

        Buffer & error() {
            return _err;
        }
    };

    inline void enable() {
        _enabled = true;
    }

    inline void disable() {
        _enabled = false;
    }

    void enter();

    void exit();

    void exitAll();

    void lock();

    void unlock();

    bool run(
        uint64_t timeoutNanos,
        const std::function<void()> &func,
        const std::function<void(Message &)> &onComplete,
        const std::function<void(Message &)> &onSuccess,
        const std::function<void(const std::string &)> &onError,
        Options &options
    );

    void resourceSnapshot(ResourceSnapshot &snapshot);

    inline std::string memoryReport() {
        return _memory.report();
    }

    inline void clearMemoryBlocks() {
        _memory.clear();
    }

    inline void enableFaultyNetwork(double chance, uint64_t duration) {
        _network.dropSendRequests(chance, duration);
    }

    inline void disableFaultyNetwork() {
        _network.dontDropSendRequests();
    }
};

Sandbox & sandbox();

// LibC ////////////////////////////////////////////////////////////////////////

struct LibC {
    bool _initialized = false;
    void _init();

    void * (*malloc)(size_t) = nullptr;
    void * (*calloc)(size_t, size_t) = nullptr;
    void * (*memalign)(size_t, size_t) = nullptr;
    int (*posix_memalign)(void **, size_t, size_t) = nullptr;
    void * (*valloc)(size_t) = nullptr;
    void * (*pvalloc)(size_t) = nullptr;
    void * (*aligned_alloc)(size_t, size_t) = nullptr;
    void * (*realloc)(void *, size_t) = nullptr;
    void * (*reallocarray)(void *, size_t, size_t) = nullptr;
    void (*free)(void *) = nullptr;

    void * (*mmap)(void *, size_t, int, int, int, __off_t) = nullptr;
    void * (*mremap)(void *, size_t, size_t, int, ...) = nullptr;
    int (*munmap)(void *, size_t) = nullptr;

    ssize_t (*send)(int, const void *, size_t, int) = nullptr;
    ssize_t (*sendto)(int, const void *, size_t, int, const struct sockaddr *, socklen_t) = nullptr;
    ssize_t (*recv)(int, void *, size_t, int) = nullptr;
    ssize_t (*recvfrom)(int, void * __restrict, size_t, int, struct sockaddr * __restrict, socklen_t * __restrict) = nullptr;
};

LibC & libc();

// Exceptions //////////////////////////////////////////////////////////////////

class SandboxException : public std::exception {
private:
    std::string _msg;

public:
    inline SandboxException(const std::string &msg)
    : _msg(msg)
    { }

    inline const char * what() const noexcept override { 
        return _msg.c_str();
    }
};

enum class FatalError : uint16_t {
    MEMORY_BLOCK_DOES_NOT_EXIST
};

class SandboxFatalException : public SandboxException {
public:
    inline SandboxFatalException(FatalError code, const std::string &msg, int stackSkip)
    : SandboxException(
        std::string("Detected fatal error: ") + msg
        + ". Caused by:\n" + CallStack::trace(1 + stackSkip).toString()
      )
    { }
};

}  // end namespace dtest
