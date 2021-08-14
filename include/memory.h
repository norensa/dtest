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

    volatile bool _track = false;
    std::unordered_map<void *, Allocation> _blocks;

    size_t _allocateSize = 0;
    size_t _freeSize = 0;
    size_t _allocateCount = 0;
    size_t _freeCount = 0;

    static thread_local bool _locked;
    inline bool _enter() {
        if (! _track || _locked) return false;
        _locked = true;
        return true;
    }

    inline void _exit() {
        _locked = false;
    }

    bool _canTrackAlloc(const CallStack &callstack);

    bool _canTrackDealloc(const CallStack &callstack);

public:

    static void reinitialize();

    Memory();

    inline void trackActivity(bool val) {
        _track = val;
    }

    void track(void *ptr, size_t size);

    void retrack(void *oldPtr, void *newPtr, size_t newSize);

    void remove(void *ptr);

    void clear();

    std::string report();
};

}  // end namespace dtest
