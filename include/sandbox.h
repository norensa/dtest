#pragma once

#include <mutex>
#include <unordered_map>
#include <exception>
#include <memory.h>
#include <sys/socket.h>
#include <network.h>
#include <functional>
#include <message.h>

namespace dtest {

struct ResourceSnapshot {
    struct Quantity {
        size_t size;
        size_t count;
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

    std::mutex _mtx;
    bool _enabled = true;
    size_t _counter = 1;

    Socket _serverSocket;
    Socket _clientSocket;

    Memory _memory;
    Network _network;

    static void __signalHandler(int sig);

public:

    inline void enable() {
        _enabled = true;
    }

    inline void disable() {
        _enabled = false;
    }

    void enter();

    void exit();

    void exitAll();

    bool run(
        uint64_t timeoutNanos,
        const std::function<void()> &func,
        const std::function<void(Message &)> &onComplete,
        const std::function<void(Message &)> &onSuccess,
        const std::function<void(const std::string &)> &onError
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
