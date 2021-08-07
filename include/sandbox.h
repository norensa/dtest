#pragma once

#include <mutex>
#include <unordered_map>
#include <exception>
#include <memory.h>
#include <sys/socket.h>
#include <network.h>

namespace dtest {

class Sandbox {

    friend class CallStack;
    friend class Memory;
    friend class Network;

private:

    std::mutex _mtx;
    bool _enabled = true;
    size_t _counter = 1;

    Memory _memory;
    Network _network;

public:

    inline void enable() {
        _enabled = true;
    }

    inline void disable() {
        _enabled = false;
    }

    void enter();

    void exit();

    inline size_t allocatedMemorySize() {
        return _memory._allocateSize;
    }

    inline size_t freedMemorySize() {
        return _memory._freeSize;
    }

    inline size_t allocatedMemoryBlocks() {
        return _memory._allocateCount;
    }

    inline size_t freedMemoryBlocks() {
        return _memory._freeCount;
    }

    inline size_t usedMemory() {
        return _memory._allocateSize - _memory._freeSize;
    }

    inline std::string memoryReport() {
        return _memory.report();
    }

    inline void clearMemoryBlocks() {
        _memory.clear();
    }

    inline size_t networkSendSize() {
        return _network._sendSize;
    }

    inline size_t networkSendCount() {
        return _network._sendCount;
    }

    inline size_t networkReceiveSize() {
        return _network._recvSize;
    }

    inline size_t networkReceiveCount() {
        return _network._recvCount;
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
private:
    FatalError _code;
    CallStack _callstack;

public:
    inline SandboxFatalException(FatalError code, const std::string &msg, int stackSkip)
    : SandboxException(msg),
      _code(code),
      _callstack(CallStack::trace(1 + stackSkip))
    { }

    inline FatalError code() const noexcept {
        return _code;
    }

    inline const CallStack & callstack() const noexcept {
        return _callstack;
    }
};

}  // end namespace dtest
