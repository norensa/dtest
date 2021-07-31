#pragma once

#include <unordered_map>
#include <mutex>

namespace dtest {

class MemoryWatch {
private:

    static const int nMaxFrames = 16;

    static size_t _totalAllocated;
    static size_t _totalFreed;
    static size_t _allocateCount;
    static size_t _freeCount;

    struct Callstack {
        int len;
        void **stack;
    };

    static std::unordered_map<void *, Callstack> _allocatedBlocks;
    static std::mutex _mtx;
    static bool _trackAllocations;

public:

    static char calloc_tmp[1024];      // for dlsym, while we find calloc

    static void * (*libc_malloc)(size_t);
    static void * (*libc_calloc)(size_t, size_t);
    static void * (*libc_memalign)(size_t, size_t);
    static int    (*libc_posix_memalign)(void **, size_t, size_t);
    static void * (*libc_valloc)(size_t);
    static void * (*libc_pvalloc)(size_t);
    static void * (*libc_aligned_alloc)(size_t, size_t);
    static void * (*libc_realloc)(void *, size_t);
    static void * (*libc_reallocarray)(void *, size_t, size_t);
    static void   (*libc_free)(void *);

    static void _init();

    static void _trackAllocation(void *ptr);

    static void _retrackAllocation(void *oldPtr, size_t oldSize, void *newPtr);

    static void _removeTrackedAllocation(void *ptr);

public:

    static void clear();

    static std::string report();

    static inline size_t totalAllocated() {
        return _totalAllocated;
    }

    static inline size_t totalFreed() {
        return _totalFreed;
    }

    static inline size_t allocatedBlocks() {
        return _allocateCount;
    }

    static inline size_t freedBlocks() {
        return _freeCount;
    }

    static inline size_t totalUsedMemory() {
        return _totalAllocated - _totalFreed;
    }

    static inline void track(bool val) {
        _trackAllocations = val;
    }

    static inline bool isTracking() {
        return _trackAllocations;
    }
};

}  // end namespace dtest
