#pragma once

#include <vector>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <string>
#include <functional>
#include <stdint.h>
#include <dtest_core/socket.h>
#include <unistd.h>
#include <dtest_core/sandbox.h>
#include <dtest_core/lazy.h>
#include <dtest_core/message.h>
#include <sstream>

namespace dtest {

class Test {

    friend class Context;
    friend class DriverContext;
    friend class WorkerContext;

public:

    enum class Status {
        PASS,
        SKIP,
        PASS_WITH_MEMORY_LEAK,
        TOO_SLOW,
        TIMEOUT,
        FAIL,
        PENDING,
    };

protected:

    std::string _module;
    std::string _name;

    std::unordered_set<std::string> _dependencies;
    std::unordered_set<std::string> _remainingDependencies;

    bool _enabled = true;
    bool _success = false;

    Status _status = Status::PENDING;
    Status _expectedStatus = Status::PASS;
    std::vector<Status> _childStatus;

    std::string _detailedReport = "";
    std::vector<std::string> _childDetailedReport;

    ResourceSnapshot _usedResources;
    std::list<std::string> _errors;

    uint16_t _numWorkers = 0;

    std::string _errorReport();

    virtual bool _distributed() const {
        return false;
    }

    virtual void _driverRun() = 0;

    virtual void _workerRun() {
        // do nothing
    }

    virtual void _report(bool driver, std::stringstream &s) = 0;

private:

    void _skip();

    void _run();

public:

    inline Test(
        const std::string &module,
        const std::string &name
    ): _module(module),
       _name(name)
    {
        __tests[_module].push_back(this);

        auto it = __globalDependencies.find(module);
        if (it != __globalDependencies.end()) {
            _dependencies = it->second;
        }
    }

    inline Test(
        const std::string &name
    ): Test("root", name)
    { }

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

    inline Test & expect(Status status) {
        _expectedStatus = status;
        return *this;
    }

    inline Test & disable() {
        _enabled = false;
        return *this;
    }

    inline Test & enable() {
        _enabled = true;
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

    static std::unordered_map<std::string, std::unordered_set<std::string>> __globalDependencies;

    static bool _logStatsToStderr;

    static bool _isDriver;

    static uint16_t _defaultNumWorkers;

public:

    static void setGlobalModuleDependencies(
        const std::string &module,
        const std::initializer_list<std::string> &dependencies
    );

    static inline const std::string & statusString(const Status &status) {
        return __statusString[(uint32_t) status];
    }

    static inline void logStatsToStderr(bool val) {
        _logStatsToStderr = val;
    }

    static bool runAll(
        const std::vector<std::pair<std::string, std::string>> &config = {},
        const std::unordered_set<std::string> &modules = {},
        std::ostream &out = std::cout
    );

    static void runWorker(uint32_t id);
};


class ModuleController {
private:

    std::string _module;

public:

    ModuleController(const std::string &module)
    :   _module(module)
    { }

    ModuleController & dependsOn(
        const std::initializer_list<std::string> &dependencies
    ) {
        Test::setGlobalModuleDependencies(_module, dependencies);
        return *this;
    }
};

class Context {
    friend class Test;

private:
    static Context *_currentCtx;

protected: 

    Test *_currentTest;

public:
    virtual ~Context() = default;

    inline void logError(const std::string &message) {
        sandbox().lock();
        _currentTest->_errors.push_back(message);
        sandbox().unlock();
    }

    virtual Message createUserMessage() = 0;

    virtual void sendUserMessage(Message &message) = 0;

    virtual Message getUserMessage() = 0;

    virtual void notify() = 0;

    virtual void wait(uint32_t n = -1u) = 0;

    static inline Context * instance() {
        return _currentCtx;
    }

    virtual uint32_t workerId() const = 0;
};

class DriverContext : public Context {

    friend class Test;
    friend class WorkerContext;
    friend class UserMessage;

private:

    enum class OpCode : uint16_t {
        NOP,
        WORKER_STARTED,
        RUN_TEST,
        FINISHED_TEST,
        NOTIFY,
        TERMINATE,
        USER_MESSAGE,
    };

    struct WorkerHandle {
        uint32_t _id = -1u;
        sockaddr _addr;
        pid_t _pid = 0;
        bool _running = false;
        Lazy<Socket> _socket;
        uint32_t _notifyCount = 0;
        bool _done = false;
        Test::Status _status = Test::Status::PENDING;
        std::string _detailedReport = "";

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
    std::list<Message> _userMessages;
    Socket _socket;
    Socket _superSocket;

    uint16_t _port = 0;

    sockaddr _address;
    sockaddr _superAddress;

    DriverContext() = default;

    WorkerHandle _spawnWorker();

    void _start();

    uint32_t _waitForSuperEvent();

    uint32_t _waitForEvent();

    std::list<uint32_t> _allocateWorkers(uint16_t n);

    void _deallocateWorkers(std::list<uint32_t> &spawnedWorkers);

    void _run(const Test *test);

    void _join(Test *test);

public:
    void setPort(uint16_t port) {
        _port = port;
    }

    void setAddress(const char *address);

    void addWorker(uint32_t id) {
        _workers[id] = WorkerHandle(id);
    }

    Message createUserMessage() override;

    void sendUserMessage(Message &message) override;

    Message getUserMessage() override;

    void notify() override;

    void wait(uint32_t n = -1u) override;

    uint32_t workerId() const {
        return 0;
    }

    static Lazy<DriverContext> instance;
};

class WorkerContext : public Context {

    friend class Test;
    friend class DriverContext;

    using OpCode = DriverContext::OpCode;

private:

    uint32_t _id = -1;
    uint32_t _notifyCount = 0;
    std::list<Message> _userMessages;
    Socket _socket;
    std::unordered_map<std::string, Test *> _tests;
    bool _inTest = false;

    void _start(uint32_t id);

    void _waitForEvent();

public:

    Message createUserMessage() override;

    void sendUserMessage(Message &message) override;

    Message getUserMessage() override;

    void notify() override;

    void wait(uint32_t n = -1u) override;

    uint32_t workerId() const override {
        return _id;
    }

    static Lazy<WorkerContext> instance;
};

class AssertionException : public SandboxException {
public:
    inline AssertionException(const char *expr, const char *file, int line, const char *caller)
    : SandboxException(
        std::string("Assertion failed for expression '") + expr +
        "' in file " + file + ":" + std::to_string(line) + " " + caller
      )
    {
        sandbox().unlock();
    }
};

class TestFailureException : public SandboxException {
public:
    inline TestFailureException(const char *msg)
    : SandboxException(msg)
    {
        sandbox().unlock();
    }
};

}  // end namespace dtest

#define assert(expr) \
    if (! static_cast<bool>(expr)) { \
        dtest::sandbox().lock(); \
        throw dtest::AssertionException(#expr, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
    }

#define fail(msg) \
    { \
        dtest::sandbox().lock(); \
        throw dtest::TestFailureException(msg); \
    }

#define err(msg) dtest::Context::instance()->logError(msg)
