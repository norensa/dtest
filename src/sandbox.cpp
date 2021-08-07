#include <sandbox.h>
#include <dlfcn.h>

using namespace dtest;

void Sandbox::enter() {
    _mtx.lock();
    --_counter;
    if (_counter == 0) {
        _memory.trackActivity(_enabled);
        _network.trackActivity(_enabled);
    }
    _mtx.unlock();
}

void Sandbox::exit() {
    _mtx.lock();
    if (_counter == 0) {
        _memory.trackActivity(false);
        _network.trackActivity(false);
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

    malloc = (void *(*)(size_t)) dlsym(RTLD_NEXT, "malloc");

    calloc = (void *(*)(size_t, size_t)) dlsym(RTLD_NEXT, "calloc");

    memalign = (void *(*)(size_t, size_t)) dlsym(RTLD_NEXT, "memalign");

    posix_memalign = (int(*)(void **, size_t, size_t)) dlsym(RTLD_NEXT, "posix_memalign");

    valloc = (void *(*)(size_t)) dlsym(RTLD_NEXT, "valloc");

    pvalloc = (void *(*)(size_t)) dlsym(RTLD_NEXT, "pvalloc");

    aligned_alloc = (void *(*)(size_t, size_t)) dlsym(RTLD_NEXT, "aligned_alloc");

    realloc = (void *(*)(void *, size_t)) dlsym(RTLD_NEXT, "realloc");

    reallocarray = (void *(*)(void *, size_t, size_t)) dlsym(RTLD_NEXT, "reallocarray");

    free = (void (*)(void *)) dlsym(RTLD_NEXT, "free");

    send = (ssize_t (*)(int, const void *, size_t, int)) dlsym(RTLD_NEXT, "send");

    sendto = (ssize_t (*)(int, const void *, size_t, int, const struct sockaddr *, socklen_t)) dlsym(RTLD_NEXT, "sendto");

    recv = (ssize_t (*)(int, void *, size_t, int)) dlsym(RTLD_NEXT, "recv");

    recvfrom = (ssize_t (*)(int, void * __restrict, size_t, int, struct sockaddr * __restrict, socklen_t * __restrict)) dlsym(RTLD_NEXT, "recvfrom");
}

static LibC libc_instance;
LibC & dtest::libc() {
    if (! libc_instance._initialized) libc_instance._init();
    return libc_instance;
}
