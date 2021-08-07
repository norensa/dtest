#include <sandbox.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/wait.h>

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

void Sandbox::run(
    const std::function<void()> &func,
    const std::function<void(Message &)> &onComplete,
    const std::function<void(Message &)> &onSuccess,
    const std::function<void(const std::string &)> &onError
) {

    enum class MessageCode : uint8_t {
        COMPLETE,
        ERROR,
        OK,
    };

    Socket serverSocket = Socket(0, 128);
    pid_t pid = fork();

    if (pid == 0) {
        Socket clientSocket = Socket(serverSocket.address());

        try {
            enter();
            func();
            exit();

            Message m;
            m << MessageCode::COMPLETE;
            onComplete(m);
            m.send(clientSocket);
        }
        catch (const SandboxException &e) {
            // exit() is invoked internally
            Message m;
            m << MessageCode::ERROR << std::string(e.what());
            m.send(clientSocket);
        }
        catch (const std::exception &e) {
            exit();
            Message m;
            m << MessageCode::ERROR << std::string("Detected uncaught exception: ") + e.what();
            m.send(clientSocket);
        }
        catch (...) {
            exit();
            Message m;
            m << MessageCode::ERROR << std::string("Unknown exception thrown");
            m.send(clientSocket);
        }

        Message reply;
        reply.recv(clientSocket);
        MessageCode code;
        reply >> code;

        clientSocket.close();

        // clear any leaked memory blocks
        clearMemoryBlocks();

        ::exit((code == MessageCode::OK) ? 0 : 1);
    }
    else {
        bool done = false;
        while (! done) {
            auto conn = serverSocket.pollOrAcceptOrTimeout();
            if (conn == nullptr) {
                int exitStatus;
                if (waitpid(pid, &exitStatus, WNOHANG) == 0) {
                    onError("Terminated unexpectedly with exit code " + std::to_string(exitStatus));
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
                serverSocket.dispose(*conn);
                continue;
            }

            Message reply;
            reply << MessageCode::OK;
            reply.send(*conn);

            MessageCode code;
            m >> code;

            switch (code) {
            case MessageCode::COMPLETE: {
                onSuccess(m);
                done = true;
            }
            break;

            case MessageCode::ERROR: {
                std::string reason;
                m >> reason;
                onError(reason);
                done = true;
            }
            break;

            default: break;
            }

            waitpid(pid, NULL, 0);
        }
    }

    serverSocket.close();
}

void Sandbox::resourceSnapshot(ResourceSnapshot &snapshot) {
    snapshot.memory.allocate.size = _memory._allocateSize - snapshot.memory.allocate.size;
    snapshot.memory.allocate.count = _memory._allocateCount - snapshot.memory.allocate.count;

    snapshot.memory.deallocate.size = _memory._freeSize - snapshot.memory.deallocate.size;
    snapshot.memory.deallocate.count = _memory._freeCount - snapshot.memory.deallocate.count;

    snapshot.network.send.size = _network._sendSize - snapshot.network.send.size;
    snapshot.network.send.count = _network._sendCount - snapshot.network.send.count;

    snapshot.network.receive.size = _network._recvSize - snapshot.network.receive.size;
    snapshot.network.receive.count = _network._recvCount - snapshot.network.receive.count;
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
