#include <dtest_core/memory.h>
#include <dtest_core/sandbox.h>
#include <sstream>
#include <elf.h>
#include <link.h>

using namespace dtest;

thread_local bool Memory::_locked = false;

namespace dtest {
    Memory *_mmgr_instance = nullptr;
}

struct TrackingException {
    size_t stackPos;
    const char *name;
    size_t offset;
    void *addressLow = nullptr;
    void *addressHigh = nullptr;

    TrackingException(size_t stackPos, const char *name, size_t offset)
    :   stackPos(stackPos),
        name(name),
        offset(offset)
    { }
};

static TrackingException allocEx[] = {
    { 0, "_dl_allocate_tls", (size_t) -1 },
    { 2, "GOMP_parallel", 0x26 },
    { 2, "GOMP_parallel", 0x2a },
    { 2, "GOMP_parallel", 0x3a },
    { 2, "GOMP_parallel", 0x41 },
    { 1, "__tls_get_addr", 0x38 },
    { 1, "__tls_get_addr", 0x3c },
    { 0, "_IO_file_doallocate", (size_t) -1 },

};
const size_t nAllocEx = sizeof(allocEx) / sizeof(TrackingException);

static TrackingException deallocEx[] = {
    { 0, "_dl_deallocate_tls", (size_t) -1 },
    { 0, "_dl_deallocate_tls", (size_t) -1 },
    { 0, "pthread_create", (size_t) -1 },
};
const size_t nDeallocEx = sizeof(deallocEx) / sizeof(TrackingException);

bool Memory::_canTrackAlloc(const CallStack &callstack) {
    auto s = callstack.stack();

    for (size_t i = 0; i < nAllocEx; ++i) {
        if (
            s[allocEx[i].stackPos] >= allocEx[i].addressLow
            && s[allocEx[i].stackPos] < allocEx[i].addressHigh
        ) return false;
    }

    return true;
}

bool Memory::_canTrackDealloc(const CallStack &callstack) {
    auto s = callstack.stack();

    for (size_t i = 0; i < nDeallocEx; ++i) {
        if (
            s[deallocEx[i].stackPos] >= deallocEx[i].addressLow
            && s[deallocEx[i].stackPos] < deallocEx[i].addressHigh
        ) return false;
    }

    return true;
}

void Memory::reinitialize(void *handle) {
    for (size_t i = 0; i < nAllocEx; ++i) {
        if (allocEx[i].addressLow != nullptr) continue;

        allocEx[i].addressLow = dlsym(handle, allocEx[i].name);

        if (allocEx[i].addressLow == nullptr) continue;

        if (allocEx[i].offset == (size_t) -1) {
            Dl_info dli;
            ElfW(Sym) *sym;
            dladdr1(allocEx[i].addressLow, &dli, (void **) &sym, RTLD_DL_SYMENT);
            allocEx[i].addressHigh = (char *) allocEx[i].addressLow + sym->st_size;
        }
        else {
            allocEx[i].addressLow = (char *) allocEx[i].addressLow + allocEx[i].offset;
            allocEx[i].addressHigh = (char *) allocEx[i].addressLow + 1;
        }
    }

    for (size_t i = 0; i < nDeallocEx; ++i) {
        if (deallocEx[i].addressLow != nullptr) continue;

        deallocEx[i].addressLow = dlsym(handle, deallocEx[i].name);

        if (deallocEx[i].addressLow == nullptr) continue;

        if (deallocEx[i].offset == (size_t) -1) {
            Dl_info dli;
            ElfW(Sym) *sym;
            dladdr1(deallocEx[i].addressLow, &dli, (void **) &sym, RTLD_DL_SYMENT);
            deallocEx[i].addressHigh = (char *) deallocEx[i].addressLow + sym->st_size;
        }
        else {
            deallocEx[i].addressLow = (char *) deallocEx[i].addressLow + deallocEx[i].offset;
            deallocEx[i].addressHigh = (char *) deallocEx[i].addressLow + 1;
        }
    }
}

Memory::Memory() {
    _mmgr_instance = this;
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
