#include <sandbox.h>
#include <dlfcn.h>

using namespace dtest;

void Sandbox::enter() {
    _mtx.lock();
    --_counter;
    if (_counter == 0) {
        _memory.trackAllocations(_enabled);
    }
    _mtx.unlock();
}

void Sandbox::exit() {
    _mtx.lock();
    if (_counter == 0) {
        _memory.trackAllocations(false);
    }
    ++_counter;
    _mtx.unlock();
}

static Sandbox instance;
Sandbox & dtest::sandbox() {
    return instance;
}
// LibC ////////////////////////////////////////////////////////////////////////

void LibC::_init() {
    _initialized = true;

    malloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc");

    calloc = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");

    memalign = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "memalign");

    posix_memalign = (int(*)(void **, size_t, size_t))dlsym(RTLD_NEXT, "posix_memalign");

    valloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "valloc");

    pvalloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "pvalloc");

    aligned_alloc = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "aligned_alloc");

    realloc = (void *(*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");

    reallocarray = (void *(*)(void *, size_t, size_t))dlsym(RTLD_NEXT, "reallocarray");

    free = (void (*)(void *))dlsym(RTLD_NEXT, "free");
}

static LibC libc_instance;
LibC & dtest::libc() {
    if (! libc_instance._initialized) libc_instance._init();
    return libc_instance;
}
