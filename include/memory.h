#pragma once

#include <call_stack.h>
#include <mutex>
#include <unordered_map>
#include <string>

namespace dtest {

class Memory {

    friend class Sandbox;

private:
    std::mutex _mtx;

    struct Allocation {
        size_t size;
        CallStack callstack;
    };

    bool _track = false;
    std::unordered_map<void *, Allocation> _blocks;

    size_t _allocateSize = 0;
    size_t _freeSize = 0;
    size_t _allocateCount = 0;
    size_t _freeCount = 0;

    static thread_local bool __locked;
    inline bool _enter() {
        if (! _track) return false;
        if (__locked) return false;
        __locked = true;
        _mtx.lock();
        return true;
    }

    inline void _exit() {
        _mtx.unlock();
        __locked = false;
    }

public:

    Memory();

    inline void trackAllocations(bool val) {
        _track = val;
    }

    void track(void *ptr, size_t size);

    void retrack(void *oldPtr, void *newPtr, size_t newSize);

    void remove(void *ptr);

    void clear();

    std::string report();
};

}  // end namespace dtest
