#pragma once

#include <vector>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <string>
#include <functional>
#include <stdint.h>
#include <socket.h>
#include <unistd.h>
#include <memory_watch.h>
#include <lazy.h>

namespace dtest {

class Test {

    friend class DriverContext;
    friend class WorkerContext;

public:

    enum class Status {
        PENDING,
        PASS,
        PASS_WITH_MEMORY_LEAK,
        FAIL,
        TIMEOUT,
    };

protected:

    std::string _module;
    std::string _name;

    std::unordered_set<std::string> _dependencies;

    Status _status = Status::PENDING;

    std::string _detailedReport = "";

    uint16_t _numWorkers = 0;

    virtual bool _distributed() const {
        return false;
    }

    virtual void _driverRun() = 0;

    virtual void _workerRun() {
        // do nothing
    }

private:

    Status _run();

public:

    inline Test(
        const std::string &name
    ): _module("root"),
       _name(name)
    {
        __tests[_module].push_back(this);
    }

    inline Test(
        const std::string &module,
        const std::string &name
    ): _module(module),
       _name(name)
    {
        __tests[_module].push_back(this);
    }

    virtual Test * copy() const = 0;

    virtual ~Test() = default;

    inline Test & dependsOn(const std::string &dependency) {
        _dependencies.insert(dependency);
        return *this;
    }

    inline Test & dependsOn(const std::initializer_list<std::string> &dependencies) {
        _dependencies.insert(dependencies.begin(), dependencies.end());
        return *this;
    }

    inline const std::string & name() const {
        return _name;
    }

    inline const std::string & module() const {
        return _module;
    }

private:

    static std::string __statusString[];

    static std::unordered_map<std::string, std::list<Test *>> __tests;

    static std::list<std::string> __err;

    static bool _logStatsToStderr;

    static bool _isDriver;

    static uint16_t _defaultNumWorkers;

protected:

    static std::string _collectErrorMessages();

public:

    static inline const std::string & statusString(const Status &status) {
        return __statusString[(uint32_t) status];
    }

    static inline void logError(const std::string &message) {
        bool tmp = MemoryWatch::isTracking();
        MemoryWatch::track(false);
        __err.push_back(message);
        MemoryWatch::track(tmp);
    }

    static inline void logStatsToStderr(bool val) {
        _logStatsToStderr = val;
    }

    static bool runAll(std::ostream &out = std::cout);

    static void runWorker(uint32_t id);
};

class Context {
    friend class Test;

private:
    static Context *_currentCtx;

public:
    virtual ~Context() = default;

    virtual void notify() = 0;

    virtual void wait(uint32_t n = -1u) = 0;

    static inline Context * instance() {
        return _currentCtx;
    }
};

class DriverContext : public Context {

    friend class Test;
    friend class WorkerContext;

private:

    enum class OpCode : uint16_t {
        NOP,
        WORKER_STARTED,
        RUN_TEST,
        FINISHED_TEST,
        NOTIFY,
        TERMINATE,
    };

    struct WorkerHandle {
        uint32_t _id = -1u;
        sockaddr _addr;
        pid_t _pid = 0;
        bool _running = false;
        Lazy<Socket> _socket;
        uint32_t _notifyCount = 0;
        bool _done = false;

        inline WorkerHandle()
        : _socket([] { return nullptr; })
        { }

        inline WorkerHandle(uint32_t id, pid_t pid = 0)
        : _id(id),
          _pid(pid),
          _socket([this] { return new Socket(_addr); })
        { }

        inline WorkerHandle(const WorkerHandle &rhs)
        : _id(rhs._id),
          _addr(rhs._addr),
          _pid(rhs._pid),
          _running(rhs._running),
          _socket([this] { return new Socket(_addr); }),
          _notifyCount(rhs._notifyCount),
          _done(rhs._done)
        { }

        inline WorkerHandle(WorkerHandle &&rhs)
        : WorkerHandle(rhs)
        { }

        inline WorkerHandle & operator=(const WorkerHandle &rhs) {
            _id = rhs._id;
            _addr = rhs._addr;
            _pid = rhs._pid;
            _running = rhs._running;
            _socket = Lazy<Socket>([this] { return new Socket(_addr); });
            _notifyCount = rhs._notifyCount;
            _done = rhs._done;

            return *this;
        }

        inline WorkerHandle & operator=(WorkerHandle &&rhs) {
            return operator=(rhs);
        }

        void run(const Test *test);

        void notify();

        void terminate();
    };

    std::unordered_map<uint32_t, WorkerHandle> _workers;
    std::unordered_map<uint32_t, WorkerHandle> _allocatedWorkers;

    Socket _socket;

    DriverContext() = default;

    WorkerHandle _spawnWorker();

    void _start();

    uint32_t _waitForEvent();

    std::list<uint32_t> _allocateWorkers(uint16_t n);

    void _deallocateWorkers(std::list<uint32_t> &spawnedWorkers);

    void _run(const Test *test);

    void _join();

public:

    void notify() override;

    void wait(uint32_t n = -1u) override;

    static Lazy<DriverContext> instance;
};

class WorkerContext : public Context {

    friend class Test;
    friend class DriverContext;

    using OpCode = DriverContext::OpCode;

private:

    uint32_t _id = -1;

    uint32_t _notifyCount = 0;

    Socket _socket;

    Socket _driverSocket;

    std::unordered_map<std::string, Test *> _tests;

    bool _inTest = false;

    void _start(uint32_t id);

    void _waitForEvent();

public:

    void notify() override;

    void wait(uint32_t n = -1u) override;

    static Lazy<WorkerContext> instance;
};

struct AssertionException {
    std::string msg;

    inline AssertionException(const std::string &msg)
    : msg(msg)
    { }
};

inline void __assertionFailure(const char *expr, const char *file, int line, const char *caller) {
    throw AssertionException(
        std::string("Assertion failed for expression '") + expr + "' "
        + "in file " + file + ":" + std::to_string(line) + " " + caller
    );
}

}  // end namespace dtest

#define assert(expr) \
    if (! static_cast<bool>(expr)) { \
        dtest::__assertionFailure(#expr, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
    }

#define err(msg) dtest::Test::logError(msg)
