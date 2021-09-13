#include <memory.h>
#include <sandbox.h>
#include <sstream>
#include <malloc.h>
#include <dlfcn.h>
#include<iostream>
using namespace dtest;

thread_local bool Memory::_locked = false;

static Memory *instance = nullptr;

struct TrackingException {
    size_t stackPos;
    const char *name;
    size_t offset;
    void *address = nullptr;

    TrackingException(size_t stackPos, const char *name, size_t offset)
    :   stackPos(stackPos),
        name(name),
        offset(offset)
    { }
};

static TrackingException allocEx[] = {
    { 0, "_dl_allocate_tls", 0x2b },
    { 2, "GOMP_parallel", 0x2a },
    { 2, "GOMP_parallel", 0x41 },
    { 1, "__tls_get_addr", 0x3c },
};
const size_t nAllocEx = sizeof(allocEx) / sizeof(TrackingException);

static TrackingException deallocEx[] = {
    { 0, "", 0x0 },
};
const size_t nDeallocEx = sizeof(deallocEx) / sizeof(TrackingException);

bool Memory::_canTrackAlloc(const CallStack &callstack) {
    auto s = callstack.stack();

    for (size_t i = 0; i < nAllocEx; ++i) {
        if (s[allocEx[i].stackPos] == allocEx[i].address) return false;
    }

    return true;
}

bool Memory::_canTrackDealloc(const CallStack &callstack) {
    auto s = callstack.stack();

    for (size_t i = 0; i < nDeallocEx; ++i) {
        if (s[deallocEx[i].stackPos] == deallocEx[i].address) return false;
    }

    return true;
}

void Memory::reinitialize() {
    for (size_t i = 0; i < nAllocEx; ++i) {
        allocEx[i].address = dlsym(RTLD_NEXT, allocEx[i].name);
        if (allocEx[i].address != nullptr) {
            allocEx[i].address = (char *) allocEx[i].address + allocEx[i].offset;
        }
    }

    for (size_t i = 0; i < nDeallocEx; ++i) {
        deallocEx[i].address = dlsym(RTLD_NEXT, deallocEx[i].name);
        if (deallocEx[i].address != nullptr) {
            deallocEx[i].address = (char *) deallocEx[i].address + deallocEx[i].offset;
        }
    }
}

Memory::Memory() {
    instance = this;
    reinitialize();
}

void Memory::track(void *ptr, size_t size) {
    if (! _enter()) return;

    auto callstack = CallStack::trace(2);
    if (_canTrackAlloc(callstack)) {
        _mtx.lock();
        _blocks.insert({ ptr, { size, std::move(callstack) } });
        _allocateSize += size;
        ++_allocateCount;
        _mtx.unlock();
    }

    _exit();
}

void Memory::retrack(void *oldPtr, void *newPtr, size_t newSize) {
    if (! _enter()) return;
    _mtx.lock();

    auto it = _blocks.find(oldPtr);
    if (it == _blocks.end()) {
        _mtx.unlock();
        bool error = _canTrackDealloc(CallStack::trace(2));
        _exit();

        if (error) {
            sandbox().exitAll();

            char buf[64];
            snprintf(buf, sizeof(buf), "no valid memory block at %p", oldPtr);

            throw SandboxFatalException(
                FatalError::MEMORY_BLOCK_DOES_NOT_EXIST,
                buf,
                2
            );
        }

        return;
    }

    auto alloc = std::move(it->second);
    size_t oldSize = alloc.size;
    alloc.size = newSize;
    _blocks.erase(it);
    _blocks.insert({ newPtr, std::move(alloc) });
    _allocateSize += newSize - oldSize;

    _mtx.unlock();
    _exit();
}

void Memory::remove(void *ptr) {
    if (! _enter()) return;
    _mtx.lock();

    auto it = _blocks.find(ptr);
    if (it == _blocks.end()) {
        _mtx.unlock();
        bool error = _canTrackDealloc(CallStack::trace(2));
        _exit();

        if (error) {
            sandbox().exitAll();

            char buf[64];
            snprintf(buf, sizeof(buf), "no valid memory block at %p", ptr);

            throw SandboxFatalException(
                FatalError::MEMORY_BLOCK_DOES_NOT_EXIST,
                buf,
                2
            );
        }

        return;
    }

    _freeSize += it->second.size;
    ++_freeCount;
    _blocks.erase(it);

    _mtx.unlock();
    _exit();
}

void Memory::clear() {
    _enter();
    _mtx.lock();

    for (const auto &block : _blocks) {
        _freeSize += block.second.size;
        ++_freeCount;
        libc().free(block.first);
    }
    _blocks.clear();

    _mtx.unlock();
    _exit();
}

std::string Memory::report() {
    _enter();
    _mtx.lock();

    std::stringstream s;

    for (const auto & block : _blocks) {
        s << "\nBlock @ " << block.first << " allocated from:\n" << block.second.callstack.toString();;
    }

    _mtx.unlock();
    _exit();
    return s.str();
}

// malloc & friends

static char _calloc_tmp[2048];      // for dlsym, while we find calloc

void * malloc(size_t __size) {
    void *ptr;

    ptr = libc().malloc(__size);
    if (ptr && instance) instance->track(ptr, __size);
    return ptr;
}

void * calloc(size_t __nmemb, size_t __size) {
    void *ptr;

    if (libc().calloc == nullptr) return _calloc_tmp;

    ptr = libc().calloc(__nmemb, __size);
    if (ptr && instance) instance->track(ptr, __nmemb * __size);
    return ptr;
}

void * memalign(size_t __alignment, size_t __size) {
    void *ptr;

    ptr = libc().memalign(__alignment, __size);
    if (ptr && instance) instance->track(ptr, __size);
    return ptr;
}

int posix_memalign(void **__memptr, size_t __alignment, size_t __size) {
    int retval;

    retval = libc().posix_memalign(__memptr, __alignment, __size);
    if (*__memptr && instance) instance->track(*__memptr, __size);
    return retval;
}

void * valloc(size_t __size) {
    void *ptr;

    ptr = libc().valloc(__size);
    if (ptr && instance) instance->track(ptr, __size);
    return ptr;
}

void * pvalloc(size_t __size) {
    void *ptr;

    ptr = libc().pvalloc(__size);
    if (ptr && instance) instance->track(ptr, __size);
    return ptr;
}

void * aligned_alloc(size_t __alignment, size_t __size) {
    void *ptr;

    ptr = libc().aligned_alloc(__alignment, __size);
    if (ptr && instance) instance->track(ptr, __size);
    return ptr;
}

void * realloc(void *__ptr, size_t __size) {
    void *ptr;

    ptr = libc().realloc(__ptr, __size);
    if (__ptr) {
        if (ptr && instance) instance->retrack(__ptr, ptr, __size);
    }
    else {
        if (ptr && instance) instance->track(ptr, __size);
    }
    return ptr;
}

void * reallocarray(void *__ptr, size_t __nmemb, size_t __size) throw() {
    void *ptr;

    ptr = libc().reallocarray(__ptr, __nmemb, __size);
    if (__ptr) {
        if (ptr && instance) instance->retrack(__ptr, ptr, __nmemb * __size);
    }
    else {
        if (ptr && instance) instance->track(ptr, __nmemb * __size);
    }
    return ptr;
}

void free(void *__ptr) {

    if (__ptr == _calloc_tmp) return;

    if (__ptr && instance) instance->remove(__ptr);
    libc().free(__ptr);
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
