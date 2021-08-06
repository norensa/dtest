#pragma once

#include <mutex>
#include <unordered_map>
#include <exception>
#include <memory.h>

namespace dtest {

class Sandbox {

    friend class CallStack;
    friend class Memory;

private:

    std::mutex _mtx;
    bool _enabled = true;
    size_t _counter = 1;

    Memory _memory;

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
};

Sandbox & sandbox();

// LibC ////////////////////////////////////////////////////////////////////////

struct LibC {
    bool _initialized = false;
    void _init();

    void * (*malloc)(size_t) = nullptr;
    void * (*calloc)(size_t, size_t) = nullptr;
    void * (*memalign)(size_t, size_t) = nullptr;
    int    (*posix_memalign)(void **, size_t, size_t) = nullptr;
    void * (*valloc)(size_t) = nullptr;
    void * (*pvalloc)(size_t) = nullptr;
    void * (*aligned_alloc)(size_t, size_t) = nullptr;
    void * (*realloc)(void *, size_t) = nullptr;
    void * (*reallocarray)(void *, size_t, size_t) = nullptr;
    void   (*free)(void *) = nullptr;
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
    inline SandboxFatalException(FatalError code, const std::string &msg)
    : SandboxException(msg),
      _code(code),
      _callstack(CallStack::trace())
    { }

    inline FatalError code() const noexcept {
        return _code;
    }

    inline const CallStack & callstack() const noexcept {
        return _callstack;
    }
};

}  // end namespace dtest
