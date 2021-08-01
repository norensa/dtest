#include <memory_watch.h>

#include <stdlib.h>
#include <malloc.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>    // for __cxa_demangle

#include <chrono>
#include <locale>
#include <sstream>

using namespace dtest;

static thread_local bool __mem_track_locked = false;

void MemoryWatch::_init() {
    libc_malloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc");

    libc_memalign = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "memalign");

    libc_posix_memalign =
        (int(*)(void **, size_t, size_t))dlsym(RTLD_NEXT, "posix_memalign");

    libc_valloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "valloc");

    libc_pvalloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "pvalloc");

    libc_aligned_alloc =
        (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "aligned_alloc");

    libc_realloc = (void *(*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");

    libc_reallocarray =
        (void *(*)(void *, size_t, size_t))dlsym(RTLD_NEXT, "reallocarray");

    libc_free = (void (*)(void *))dlsym(RTLD_NEXT, "free");
}

void MemoryWatch::_trackAllocation(void *ptr) {
    if (! _trackAllocations) return;

    if (__mem_track_locked) return;
    __mem_track_locked = true;

    if (libc_malloc == nullptr) {
        libc_malloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc");
    }
    if (libc_free == nullptr) {
        libc_free = (void (*)(void *))dlsym(RTLD_NEXT, "free");
    }


    void **callstack = (void **) libc_malloc(nMaxFrames * sizeof(void *));
    int nFrames = backtrace(callstack, nMaxFrames);

    _mtx.lock();
    _allocatedBlocks.insert({ ptr, { nFrames, callstack } });
    _totalAllocated += malloc_usable_size(ptr);
    ++_allocateCount;
    _mtx.unlock();

    __mem_track_locked = false;
}

void MemoryWatch::_retrackAllocation(void *oldPtr, size_t oldSize, void *newPtr) {
    if (! _trackAllocations) return;

    if (__mem_track_locked) return;
    __mem_track_locked = true;

    _mtx.lock();
    auto it = _allocatedBlocks.find(oldPtr);
    if (it != _allocatedBlocks.end()) {
        auto stacktrace = it->second;
        _allocatedBlocks.erase(it);
        _allocatedBlocks.insert({ newPtr, stacktrace });
        _totalAllocated -= oldSize;
        _totalAllocated += malloc_usable_size(newPtr);
    }
    _mtx.unlock();

    __mem_track_locked = false;
}

void MemoryWatch::_removeTrackedAllocation(void *ptr) {
    if (! _trackAllocations) return;

    if (__mem_track_locked) return;
    __mem_track_locked = true;

    if (libc_free == nullptr) {
        libc_free = (void (*)(void *))dlsym(RTLD_NEXT, "free");
    }

    _mtx.lock();
    auto it = _allocatedBlocks.find(ptr);
    if (it != _allocatedBlocks.end()) {
        libc_free(it->second.stack);
        _allocatedBlocks.erase(it);
        _totalFreed += malloc_usable_size(ptr);
        ++_freeCount;
    }
    _mtx.unlock();

    __mem_track_locked = false;
}

void MemoryWatch::clear() {
    __mem_track_locked = true;

    if (libc_free == nullptr) {
        libc_free = (void (*)(void *))dlsym(RTLD_NEXT, "free");
    }

    _mtx.lock();
    for (const auto &block : _allocatedBlocks) {
        _totalFreed += malloc_usable_size(block.first);
        ++_freeCount;
        libc_free(block.first);
        libc_free(block.second.stack);
    }
    _allocatedBlocks.clear();
    _mtx.unlock();

    __mem_track_locked = false;
}

std::string MemoryWatch::report() {
    std::stringstream s;
    char buf[1024];

    for (const auto & block : _allocatedBlocks) {
        s << "\nBlock @ " << block.first << " allocated from:\n";

        char **symbols = backtrace_symbols(block.second.stack, block.second.len);

        for (int i = 1; i < block.second.len; i++) {
            Dl_info info;
            if (dladdr(block.second.stack[i], &info)) {
                char *demangled = NULL;
                int status;
                demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
                snprintf(
                    buf, sizeof(buf), "%-3d  %p  %s + %p\n",
                    block.second.len - i - 1, block.second.stack[i],
                    status == 0 ? demangled : info.dli_sname,
                    (void *) ((char *)block.second.stack[i] - (char *)info.dli_saddr)
                );
                free(demangled);
            }
            else {
                snprintf(
                    buf, sizeof(buf), "%-3d  %p  %s\n",
                    block.second.len - i - 1, block.second.stack[i],
                    symbols[i]
                );
            }
            s << buf;
        }
        free(symbols);
        if (block.second.len == nMaxFrames) s << "[truncated]\n";
    }

    return s.str();
}

char MemoryWatch::calloc_tmp[1024];

void * (*MemoryWatch::libc_malloc)(size_t) = nullptr;
void * (*MemoryWatch::libc_calloc)(size_t, size_t) = nullptr;
void * (*MemoryWatch::libc_memalign)(size_t, size_t) = nullptr;
int    (*MemoryWatch::libc_posix_memalign)(void **, size_t, size_t) = nullptr;
void * (*MemoryWatch::libc_valloc)(size_t) = nullptr;
void * (*MemoryWatch::libc_pvalloc)(size_t) = nullptr;
void * (*MemoryWatch::libc_aligned_alloc)(size_t, size_t) = nullptr;
void * (*MemoryWatch::libc_realloc)(void *, size_t) = nullptr;
void * (*MemoryWatch::libc_reallocarray)(void *, size_t, size_t) = nullptr;
void   (*MemoryWatch::libc_free)(void *) = nullptr;

size_t MemoryWatch::_totalAllocated = 0;
size_t MemoryWatch::_totalFreed = 0;
size_t MemoryWatch::_allocateCount = 0;
size_t MemoryWatch::_freeCount = 0;

std::unordered_map<void *, MemoryWatch::Callstack> MemoryWatch::_allocatedBlocks;
std::mutex MemoryWatch::_mtx;
bool MemoryWatch::_trackAllocations = false;

// malloc & friends

void * malloc(size_t __size) {
    void *ptr;

    if (MemoryWatch::libc_malloc == nullptr) MemoryWatch::_init();

    ptr = MemoryWatch::libc_malloc(__size);
    MemoryWatch::_trackAllocation(ptr);
    return ptr;
}

void * calloc(size_t __nmemb, size_t __size) {
    static bool initialized = false;
    void *ptr;

    if (! initialized) {
        initialized = true;
        MemoryWatch::libc_calloc = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
    }
    if (MemoryWatch::libc_calloc == nullptr) {
        return MemoryWatch::calloc_tmp;
    }

    ptr = MemoryWatch::libc_calloc(__nmemb, __size);
    MemoryWatch::_trackAllocation(ptr);
    return ptr;
}

void * memalign(size_t __alignment, size_t __size) {
    void *ptr;

    if (MemoryWatch::libc_memalign == nullptr) MemoryWatch::_init();

    ptr = MemoryWatch::libc_memalign(__alignment, __size);
    MemoryWatch::_trackAllocation(ptr);
    return ptr;
}

int posix_memalign(void **__memptr, size_t __alignment, size_t __size) {
    int retval;

    if (MemoryWatch::libc_posix_memalign == nullptr) MemoryWatch::_init();

    retval = MemoryWatch::libc_posix_memalign(__memptr, __alignment, __size);
    MemoryWatch::_trackAllocation(*__memptr);
    return retval;
}

void * valloc(size_t __size) {
    void *ptr;

    if (MemoryWatch::libc_valloc == nullptr) MemoryWatch::_init();

    ptr = MemoryWatch::libc_valloc(__size);
    MemoryWatch::_trackAllocation(ptr);
    return ptr;
}

void * pvalloc(size_t __size) {
    void *ptr;

    if (MemoryWatch::libc_pvalloc == nullptr) MemoryWatch::_init();

    ptr = MemoryWatch::libc_pvalloc(__size);
    MemoryWatch::_trackAllocation(ptr);
    return ptr;
}

void * aligned_alloc(size_t __alignment, size_t __size) {
    void *ptr;

    if (MemoryWatch::libc_aligned_alloc == nullptr) MemoryWatch::_init();

    ptr = MemoryWatch::libc_aligned_alloc(__alignment, __size);
    MemoryWatch::_trackAllocation(ptr);
    return ptr;
}

void * realloc(void *__ptr, size_t __size) {
    size_t oldSize = malloc_usable_size(__ptr);
    void *ptr;

    if (MemoryWatch::libc_realloc == nullptr) MemoryWatch::_init();

    ptr = MemoryWatch::libc_realloc(__ptr, __size);
    MemoryWatch::_retrackAllocation(__ptr, oldSize, ptr);
    return ptr;
}

void * reallocarray(void *__ptr, size_t __nmemb, size_t __size) throw() {
    size_t oldSize = malloc_usable_size(__ptr);
    void *ptr;

    if (MemoryWatch::libc_reallocarray == nullptr) MemoryWatch::_init();

    ptr = MemoryWatch::libc_reallocarray(__ptr, __nmemb, __size);
    MemoryWatch::_retrackAllocation(__ptr, oldSize, ptr);
    return ptr;
}

void free(void *__ptr) {

    if (__ptr == MemoryWatch::calloc_tmp) return;

    if (MemoryWatch::libc_free == nullptr) MemoryWatch::_init();

    if (__ptr != nullptr) MemoryWatch::_removeTrackedAllocation(__ptr);
    MemoryWatch::libc_free(__ptr);
}

// operator new overrides /////////////////////////////////////////////////////

void * operator new(size_t count) {
    return malloc(count);
}

void * operator new[](size_t count) {
    return operator new(count);
}

#if (__cplusplus >= 201703L)
void* operator new(std::size_t count, std::align_val_t al) {
    return memalign((size_t) al, count);
}
#endif

#if (__cplusplus >= 201703L)
void* operator new[](std::size_t count, std::align_val_t al) {
    return operator new(count, al);
}
#endif

void* operator new(std::size_t count, const std::nothrow_t &) noexcept {
    return operator new(count);
}

void* operator new[](std::size_t count, const std::nothrow_t &) noexcept {
    return operator new(count);
}

#if (__cplusplus >= 201703L)
void* operator new(std::size_t count, std::align_val_t al, const std::nothrow_t &) {
    return operator new(count, al);
}
#endif

#if (__cplusplus >= 201703L)
void* operator new[](std::size_t count, std::align_val_t al, const std::nothrow_t &) {
    return operator new(count, al);
}
#endif

// operator delete overrides //////////////////////////////////////////////////

void operator delete(void *ptr) noexcept {
    free(ptr);
}

void operator delete[](void *ptr) noexcept {
    operator delete(ptr);
}

#if (__cplusplus >= 201703L)
void operator delete(void *ptr, std::align_val_t al) noexcept {
    operator delete(ptr);
}
#endif

#if (__cplusplus >= 201703L)
void operator delete[](void *ptr, std::align_val_t al) noexcept {
    operator delete(ptr);
}
#endif

void operator delete(void *ptr, size_t size) noexcept {
    operator delete(ptr);
}

void operator delete[](void *ptr, size_t size) noexcept {
    operator delete(ptr);
}

#if (__cplusplus >= 201703L)
void operator delete(void *ptr, std::size_t sz, std::align_val_t al) noexcept {
    operator delete(ptr);
}
#endif

#if (__cplusplus >= 201703L)
void operator delete[](void *ptr, std::size_t sz, std::align_val_t al) noexcept {
    operator delete(ptr);
}
#endif

void operator delete(void *ptr, const std::nothrow_t &) noexcept {
    operator delete(ptr);
}

void operator delete[](void *ptr, const std::nothrow_t &) noexcept {
    operator delete(ptr);
}

#if (__cplusplus >= 201703L)
void operator delete(void *ptr, std::align_val_t al, const std::nothrow_t &) noexcept {
    operator delete(ptr);
}
#endif

#if (__cplusplus >= 201703L)
void operator delete[](void *ptr, std::align_val_t al, const std::nothrow_t &) noexcept {
    operator delete(ptr);
}
#endif
