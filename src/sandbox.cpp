#include <dtest_core/sandbox.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <dtest_core/util.h>
#include <thread>
#include <fcntl.h>

using namespace dtest;

namespace dtest {

enum class MessageCode : uint8_t {
    COMPLETE,
    ERROR,
};

}

static Sandbox instance;

void Sandbox::__signalHandler(int sig){
    switch (sig) {
    case SIGSEGV: {
        instance.exitAll();

        Message m;
        m << MessageCode::ERROR
            << std::string("Detected segmentation fault. Caused by:\n")
            + CallStack::trace(1).toString();
        m.send(instance._clientSocket);

        instance._clientSocket.close();

        ::exit(1);
    }

    case SIGABRT: {
        instance.exitAll();

        Message m;
        m << MessageCode::ERROR
            << std::string("Caught abort signal. Caused by:\n")
            + CallStack::trace(1).toString();
        m.send(instance._clientSocket);

        instance._clientSocket.close();

        ::exit(1);
    }

    case SIGPIPE:
    case SIGKILL: {
        instance.exitAll();
        instance._clientSocket.close();

        ::exit(2);
    }

    default: break;
    }
}

static void readFully(int fd, Buffer &buf) {
    size_t pos = 0;
    ssize_t bytes;

    do {
        if (buf.size() < pos + 1024) buf.resize(pos + 1024);
        bytes = read(fd, (uint8_t *) buf.data() + pos, 1024);
        if (bytes > 0) pos += bytes;
    } while (bytes > 0);

    buf.resize(pos);
}

static void writeFully(int fd, const Buffer &buf) {
    size_t pos = 0;
    ssize_t bytes;

    while (pos < buf.size()) {
        bytes = write(fd, (uint8_t *) buf.data() + pos, buf.size() - pos);
        if (bytes > 0) pos += bytes;
    }
}

void Sandbox::_sandbox_stdio(const Buffer &in) {
    int pipefd[2];

    // stdin
    _saved_stdio[0] = dup(0);
    pipe2(pipefd, O_NONBLOCK);
    dup2(pipefd[0], 0);
    _sandboxed_stdio[0] = pipefd[1];
    writeFully(_sandboxed_stdio[0], in);

    // stdout
    _saved_stdio[1] = dup(1);
    pipe2(pipefd, O_NONBLOCK);
    dup2(pipefd[1], 1);
    _sandboxed_stdio[1] = pipefd[0];

    // stderr
    _saved_stdio[2] = dup(2);
    pipe2(pipefd, O_NONBLOCK);
    dup2(pipefd[1], 2);
    _sandboxed_stdio[2] = pipefd[0];
}

void Sandbox::_unsandbox_stdio(Buffer &out, Buffer &err) {
    // stdin
    close(0);
    close(_sandboxed_stdio[0]);
    dup2(_saved_stdio[0], 0);
    close(_saved_stdio[0]);

    // stdout
    close(1);
    readFully(_sandboxed_stdio[1], out);
    close(_sandboxed_stdio[1]);
    dup2(_saved_stdio[1], 1);
    close(_saved_stdio[1]);

    // stderr
    close(2);
    readFully(_sandboxed_stdio[2], err);
    close(_sandboxed_stdio[2]);
    dup2(_saved_stdio[2], 2);
    close(_saved_stdio[2]);
}

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

void Sandbox::exitAll() {
    _mtx.lock();
    _memory.trackActivity(false);
    _network.trackActivity(false);
    _counter = 1;
    _mtx.unlock();
}

void Sandbox::lock() {
    _memory.lock();
    _network.lock();
}

void Sandbox::unlock() {
    _memory.unlock();
    _network.unlock();
}

bool Sandbox::run(
    uint64_t timeoutNanos,
    const std::function<void()> &func,
    const std::function<void(Message &)> &onComplete,
    const std::function<void(Message &)> &onSuccess,
    const std::function<void(const std::string &)> &onError,
    Sandbox::Options &options
) {

    bool finished = false;

    _sandbox_stdio(options._in);

    _serverSocket = Socket(0, 128);

    pid_t pid = options._fork ? fork() : 0;

    if (pid == 0) {
        if (options._fork) {
            _serverSocket.close();

            signal(SIGSEGV, __signalHandler);
            signal(SIGABRT, __signalHandler);
            signal(SIGPIPE, __signalHandler);
            signal(SIGKILL, __signalHandler);
        }

        _clientSocket = Socket(_serverSocket.address());

        auto t = std::thread([this, &func, &onComplete] {
            try {
                enter();
                func();
                exit();

                Message m;
                m << MessageCode::COMPLETE;
                onComplete(m);
                m.send(_clientSocket);
            }
            catch (const SandboxException &e) {
                exitAll();
                Message m;
                m << MessageCode::ERROR
                    << std::string(e.what());
                m.send(_clientSocket);
            }
            catch (const std::exception &e) {
                exitAll();
                Message m;
                m << MessageCode::ERROR
                    << std::string("Detected uncaught exception: ") + e.what();
                m.send(_clientSocket);
            }
            catch (...) {
                exitAll();
                Message m;
                m << MessageCode::ERROR
                    << std::string("Unknown exception thrown");
                m.send(_clientSocket);
            }
        });

        t.join();

        _clientSocket.close();

        if (options._fork) ::exit(0);
    }

    bool done = false;
    auto start = std::chrono::high_resolution_clock::now();
    while (! done) {
        auto conn = _serverSocket.pollOrAcceptOrTimeout();
        if (conn == nullptr) {
            auto end = std::chrono::high_resolution_clock::now();

            if ((uint64_t) (end - start).count() > timeoutNanos) {
                if (options._fork) {
                    kill(pid, SIGKILL);
                    waitpid(pid, NULL, 0);
                }
                onError("Exceeded timeout of " + formatDuration(timeoutNanos));
                done = true;
            }
            continue;
        }

        Message m;
        try {
            m.recv(*conn);
            if (! m.hasData()) continue;
        }
        catch (...) {
            _serverSocket.dispose(*conn);
            continue;
        }

        MessageCode code;
        m >> code;

        switch (code) {
        case MessageCode::COMPLETE: {
            onSuccess(m);
            finished = true;
        }
        break;

        case MessageCode::ERROR: {
            std::string reason;
            m >> reason;
            onError(reason);
            finished = true;
        }
        break;

        default: {
            if (options._fork) kill(pid, SIGKILL);
            onError("An unexpected error has occurred");
        }
        break;
        }

        if (options._fork) {
            int res;
            do {
                res = waitpid(pid, NULL, WNOHANG);
                if (res == 0) {
                    auto end = std::chrono::high_resolution_clock::now();
                    if ((uint64_t) (end - start).count() > timeoutNanos) {
                        kill(pid, SIGKILL);
                        res = waitpid(pid, NULL, 0);
                        onError("Did not terminate properly after timeout of " + formatDuration(timeoutNanos));
                    }
                    else {
                        usleep(10000);
                    }
                }
            } while (res != pid);
        }
        done = true;
    }

    _serverSocket.close();

    _unsandbox_stdio(options._out, options._err);

    return finished;
}

void Sandbox::resourceSnapshot(ResourceSnapshot &snapshot) {
    // initialization
    if (! snapshot.initialized) {
        _memory.resetMaxAllocation();
        snapshot.initialized = true;
    }

    snapshot.memory.allocate.size = _memory._allocateSize - snapshot.memory.allocate.size;
    snapshot.memory.allocate.count = _memory._allocateCount - snapshot.memory.allocate.count;

    snapshot.memory.deallocate.size = _memory._freeSize - snapshot.memory.deallocate.size;
    snapshot.memory.deallocate.count = _memory._freeCount - snapshot.memory.deallocate.count;

    snapshot.memory.max.size = _memory._maxAllocate;
    snapshot.memory.max.count = _memory._maxAllocateCount;

    snapshot.network.send.size = _network._sendSize - snapshot.network.send.size;
    snapshot.network.send.count = _network._sendCount - snapshot.network.send.count;

    snapshot.network.receive.size = _network._recvSize - snapshot.network.receive.size;
    snapshot.network.receive.count = _network._recvCount - snapshot.network.receive.count;
}

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

    mmap = (void *(*)(void *, size_t, int, int, int, __off_t)) dlsym(RTLD_NEXT, "mmap");

    mremap = (void *(*)(void *, size_t, size_t, int, ...)) dlsym(RTLD_NEXT, "mremap");

    munmap = (int (*)(void *, size_t)) dlsym(RTLD_NEXT, "munmap");

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
