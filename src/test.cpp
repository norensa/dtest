/*
 * Copyright (c) 2021 Noah Orensa.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
*/

#include <dtest_core/test.h>

#include <algorithm>
#include <sys/wait.h>
#include <dtest_core/util.h>

using namespace dtest;

///////////////////////////////////////////////////////////////////////////////

void Test::_skip() {
    _status = Status::SKIP;
    _success = true;
}

void Test::_run() {

    if (_enabled) {
        Context::instance()->_currentTest = this;

        if (_isDriver) {
            if (_distributed()) {
                if (_numWorkers == 0) _numWorkers = _defaultNumWorkers;

                auto extraWorkers = DriverContext::instance->_allocateWorkers(_numWorkers);

                DriverContext::instance->_run(this);
                _driverRun();

                DriverContext::instance->_join(this);
                DriverContext::instance->_deallocateWorkers(extraWorkers);
            }
            else {
                _driverRun();
            }
        }
        else {
            _workerRun();
        }

        _success = _status == _expectedStatus;

        std::stringstream s;
        _report(_isDriver, s);
        _detailedReport = s.str();
    }
    else {
        _skip();
    }
}

std::string Test::__statusString[] = {
    "PASS",
    "SKIP",
    "PASS (memory leak)",
    "PASS (memory limit exceeded)",
    "TOO SLOW",
    "TIMEOUT",
    "FAIL",
    "PENDING",
};

static std::string __statusStringPastTense[] = {
    "PASSED",
    "SKIPPED",
    "PASSED (memory leak)",
    "PASSED (memory limit exceeded)",
    "TOO SLOW",
    "TIMED OUT",
    "FAILED",
    "PENDING",
};

std::unordered_map<std::string, std::list<Test *>> Test::__tests;

std::unordered_map<std::string, std::unordered_set<std::string>> Test::__globalDependencies;

bool Test::_logStatsToStderr = false;

bool Test::_isDriver = false;

uint16_t Test::_defaultNumWorkers = 4;

std::string Test::_errorReport() {
    std::stringstream s;

    if (! _errors.empty()) {
        s << "\"errors\": [";

        size_t i = 0;
        for (const auto &e : _errors) {
            if (i++ > 0) s << ",";
            s << "\n"<< indent(jsonify(e), 2);
        }
        s << "\n]";
    }

    return s.str();
}

void Test::setGlobalModuleDependencies(
    const std::string &module,
    const std::initializer_list<std::string> &dependencies
) {
    __globalDependencies[module] = dependencies;

    for (auto t : __tests[module]) {
        for (const auto &dep : dependencies) {
            if (t->_dependencies.count(dep) == 0) {
                t->_dependencies.insert(dep);
            }
        }
    }
}

bool Test::runAll(
    const std::vector<std::pair<std::string, std::string>> &config,
    const std::unordered_set<std::string> &modules,
    std::ostream &out
) {

    _isDriver = true;
    Context::_currentCtx = DriverContext::instance.ptr();
    DriverContext::instance->_start();

    bool success = true;

    std::list<Test *> ready;
    std::unordered_map<std::string, std::list<Test *>> blocked;
    std::unordered_map<std::string, std::unordered_set<Test *>> remaining;
    size_t totalTestCount = 0;

    for (const auto &moduleTests : __tests) {
        totalTestCount += moduleTests.second.size();

        for (auto t : moduleTests.second) {
            auto tt = t->copy();

            if (t->_dependencies.empty()) {
                ready.push_back(tt);
            }
            else {
                tt->_remainingDependencies = t->_dependencies;
                for (const auto &dep: t->_dependencies) {
                    blocked[dep].push_back(tt);
                }
            }

            remaining[t->_module].insert(tt);
        }
    }

    if (_logStatsToStderr) std::cerr << std::endl;

    out << std::boolalpha;
    out << "{\n";

    for (const auto &conf : config) {
        out << "  \"" << conf.first << "\": ";

        if (conf.second.front() != '[' && conf.second.front() != '{') {
            out << '"' << conf.second << '"';
        }
        else {
            out << conf.second;
        }

        out << ",\n";
    }

    out << "  \"tests\": [";

    auto start = std::chrono::high_resolution_clock::now();

    std::unordered_map<Status, uint32_t> expectedStatusSummary;
    std::unordered_map<Status, uint32_t> unExpectedStatusSummary;
    size_t runCount = 0;
    size_t skipCount = 0;
    size_t successCount = 0;
    bool firstLog = true;

    while (! ready.empty()) {

        auto test = ready.front();
        ready.pop_front();

        auto testnum = std::to_string(runCount + 1);
        testnum.resize(5, ' ');

        auto testname = test->_module + "::" + test->_name;
        auto shortTestName = testname;
        if (testname.size() > 52) {
            shortTestName = 
                testname.substr(0, 20)
                + " ... "
                + testname.substr(testname.size() - 27);
        }
        else {
            shortTestName.resize(52, ' ');
        }

        if (_logStatsToStderr) {
            std::cerr << "RUNNING TEST #" << testnum << "  " << shortTestName  << "   ";
        }

        if (modules.empty() || modules.count(test->_module) != 0) test->_run();
        else test->_skip();

        if (test->_status == Status::SKIP) {
            if (_logStatsToStderr) {
                std::cerr << "\r";
                std::cerr << std::string(80, ' ');
                std::cerr << "\r";
            }
            ++skipCount;
        }
        else {
            if (firstLog) {
                firstLog = false;
            }
            else {
                out << ",";
            }

            out << "\n    {";
            out << "\n      \"name\": \"" << testname << "\",";
            if (! test->_dependencies.empty()) {
                out << "\n      \"dependencies\": " << jsonify(test->_dependencies, 6) << ",";
            }
            out << "\n      \"i\": " << runCount << ",";
            out << "\n      \"success\": " << test->_success << ",";
            out << "\n      \"status\": \"" << statusString(test->_status) << "\",";
            out << "\n      \"report\": {\n" << indent(test->_detailedReport, 8);
            out << "\n      }";
            if (! test->_childStatus.empty()) {
                out << ",\n      \"workers\": [";
                for (size_t i = 0; i < test->_childStatus.size(); ++i) {
                    if (i > 0) out << ",";

                    out << "\n        {";
                    out << "\n          \"status\": \"" << statusString(test->_childStatus[i]) << "\",";
                    out << "\n          \"report\": {\n" << indent(test->_childDetailedReport[i], 12);
                    out << "\n          }";
                    out << "\n        }";
                }
                out << "\n      ]";
            }
            out << "\n    }";
            out.flush();
            if (_logStatsToStderr) std::cerr << (test->_success ? "PASS" : "FAIL") << "\n";
        }

        if (test->_success) {
            auto it = remaining.find(test->_module);
            it->second.erase(test);     // remove from it module's remaining list of tests
            if (it->second.empty()) {   // if entire module test completed
                // find tests blocked on the (now) completed module
                auto it = blocked.find(test->_module);
                if (it != blocked.end()) {
                    auto insertPos = ready.begin();
                    for (auto tt : it->second) {
                        // and remove that module from their set of dependencies
                        tt->_remainingDependencies.erase(test->_module);
                        // if no more dependencies are needed, then push to ready queue
                        if (tt->_remainingDependencies.empty()) ready.insert(insertPos, tt);
                    }
                }
            }
            ++successCount;
            ++expectedStatusSummary[test->_status];
        }
        else {
            success = false;
            ++unExpectedStatusSummary[test->_status];
        }

        ++runCount;
        delete test;
    }

    auto end = std::chrono::high_resolution_clock::now();

    out << "\n  ]";

    // summary
    size_t failedCount = runCount - successCount;
    size_t blockedCount = totalTestCount - runCount;

    out << ",\n  \"summary\": {";

    if (successCount > 0) {
        out << "\n    \"nominal\": {";
        size_t i = 0;
        for (const auto &r : expectedStatusSummary) {
            if (i > 0) out << ",";
            out << "\n      \"" << __statusStringPastTense[(uint32_t) r.first] << "\": "
                << r.second;

            ++i;
        }
        out << "\n    }";

        if (failedCount > 0) out << ',';
    }

    if (failedCount > 0) {
        out << "\n    \"unexpected\": {";
        size_t i = 0;
        for (const auto &r : unExpectedStatusSummary) {
            if (i > 0) out << ",";
            out << "\n      \"" << __statusStringPastTense[(uint32_t) r.first] << "\": "
                << r.second;

            ++i;
        }
        out << "\n    }";
    }

    out << "\n  }";

    // finish log output
    out << "\n}\n";
    out.flush();

    if (_logStatsToStderr) {
        successCount -= skipCount;
        totalTestCount -= skipCount;

        std::string line = "";
        line.resize(80, '-');
        std::cerr << "\n" << line << "\n";

        std::cerr << "Finished in " << formatDuration((end - start).count()) << "\n";

        std::cerr << successCount << '/' << totalTestCount << " TESTS PASSED\n";

        if (failedCount > 0) {
            std::cerr << failedCount << '/' << totalTestCount << " TESTS FAILED\n";
        }

        if (blockedCount > 0) {
            std::cerr << blockedCount << '/' << totalTestCount << " TESTS NOT RUN\n";
        }

        if (skipCount > 0) {
            std::cerr << skipCount << " TESTS SKIPPED\n";
        }
    }

    return success;
}

void Test::runWorker(uint32_t id) {

    _isDriver = false;
    Context::_currentCtx = WorkerContext::instance.ptr();
    WorkerContext::instance->_start(id);

    while (true) {
        WorkerContext::instance->_waitForEvent();
    }
}

// Context /////////////////////////////////////////////////////////////////////

Context * Context::_currentCtx = nullptr;

// DriverContext::WorkerHandle /////////////////////////////////////////////////

void DriverContext::WorkerHandle::run(const Test *test) {
    Message m;
    m << OpCode::RUN_TEST << test->_module << test->_name;
    m.send(_socket);
}

void DriverContext::WorkerHandle::notify() {
    Message m;
    m << OpCode::NOTIFY;
    m.send(_socket);
}

void DriverContext::WorkerHandle::terminate() {
    Message m;
    m << OpCode::TERMINATE;
    m.send(_socket);

    if (_pid != 0) {
        waitpid(_pid, NULL, 0);
    }
}

// DriverContext ///////////////////////////////////////////////////////////////

DriverContext::WorkerHandle DriverContext::_spawnWorker() {
    uint32_t id = _workers.size() + 1;
    pid_t pid = fork();

    if (pid == 0) {
        // close all open descriptors
        for (int fd = getdtablesize(); fd > 2; --fd) close(fd);

        try {
            Test::runWorker(id);
            exit(0);
        }
        catch (...) {
            exit(1);
        }
    }
    else {
        WorkerHandle w(id, pid);
        _workers[id] = w;
        return w;
    }
}

void DriverContext::_start() {
    _socket = Socket(_port, 128);
    _address = _socket.address();

    _superSocket = Socket(_port == 0 ? 0 : _port + 1, 128);
    _superAddress = _superSocket.address();
}

uint32_t DriverContext::_waitForSuperEvent() {
    while (true) {
        auto &conn = _superSocket.pollOrAccept();

        Message m;
        try {
            m.recv(conn);
            if (! m.hasData()) continue;
        }
        catch (...) {
            _superSocket.dispose(conn);
            continue;
        }

        OpCode op;
        uint32_t id;
        m >> op >> id;

        switch (op) {
        case OpCode::WORKER_STARTED: {
            auto it = _workers.find(id);
            if (it == _workers.end()) return -1;
            m >> it->second._addr;
            it->second._running = true;
        }
        break;

        case OpCode::FINISHED_TEST: {
            auto it = _allocatedWorkers.find(id);
            if (it == _allocatedWorkers.end()) return -1;
            it->second._done = true;
            m >> it->second._status >> it->second._detailedReport;
        }
        break;

        default: break;
        }

        return id;
    }
}

uint32_t DriverContext::_waitForEvent() {
    while (true) {
        auto &conn = _socket.pollOrAccept();

        Message m;
        try {
            m.recv(conn);
            if (! m.hasData()) continue;
        }
        catch (...) {
            _socket.dispose(conn);
            continue;
        }

        OpCode op;
        uint32_t id;
        m >> op >> id;

        switch (op) {
        case OpCode::NOTIFY: {
            auto it = _allocatedWorkers.find(id);
            if (it == _allocatedWorkers.end()) return -1;
            ++it->second._notifyCount;
        }
        break;

        case OpCode::USER_MESSAGE: {
            _userMessages.push_back(std::move(m));
        }
        break;

        default: break;
        }

        return id;
    }
}

std::list<uint32_t> DriverContext::_allocateWorkers(uint16_t n) {
    std::list<uint32_t> spawned;

    while (_workers.size() < n) {
        auto w = _spawnWorker();
        spawned.push_back(w._id);
        _workers[w._id] = w;
    }

    for (auto &w : _workers) {
        if (n-- == 0) break;

        while (! w.second._running) {
            _waitForSuperEvent();
        }

        w.second._notifyCount = 0;
        w.second._done = false;

        _allocatedWorkers[w.second._id] = w.second;
    }

    return spawned;
}

void DriverContext::_deallocateWorkers(std::list<uint32_t> &spawnedWorkers) {
    for (auto &w : spawnedWorkers) {
        _allocatedWorkers[w].terminate();
        _workers.erase(w);
    }

    _allocatedWorkers.clear();
}

void DriverContext::_run(const Test *test) {
    for (auto &w : _allocatedWorkers) {
        w.second.run(test);
    }
}

void DriverContext::_join(Test *test) {
    for (auto &w : _allocatedWorkers) {
        while (! w.second._done) {
            _waitForSuperEvent();
        }

        if (w.second._status > test->_status) {
            test->_status = w.second._status;
        }

        test->_childStatus.push_back(w.second._status);
        test->_childDetailedReport.push_back(w.second._detailedReport);
    }
}

void DriverContext::setAddress(const char *address) {
    _address = Socket::str_to_ipv4(address);

    _port = Socket::get_port(_address);
    _superAddress = Socket::set_port(_address, _port == 0 ? 0 : _port + 1);
}

Message DriverContext::createUserMessage() {
    sandbox().lock();

    Message m;
    m << OpCode::USER_MESSAGE;

    sandbox().unlock();

    return m;
}

void DriverContext::sendUserMessage(Message &message) {
    sandbox().lock();

    for (auto &w : _allocatedWorkers) {
        message.send(w.second._socket);
    }

    sandbox().unlock();
}

Message DriverContext::getUserMessage() {
    sandbox().lock();

    while (_userMessages.empty()) {
        _waitForEvent();
    }
    Message m = std::move(_userMessages.front());
    _userMessages.pop_front();

    sandbox().unlock();

    return m;
}

void DriverContext::notify() {
    sandbox().lock();

    for (auto &w : _allocatedWorkers) {
        w.second.notify();
    }

    sandbox().unlock();
}

void DriverContext::wait(uint32_t n) {
    sandbox().lock();

    if (n == -1u) n = _allocatedWorkers.size();

    auto &pulled = *new std::unordered_set<uint32_t>();

    for (auto &w : _allocatedWorkers) {
        if (pulled.size() == n) break;

        if (w.second._notifyCount > 0) {
            --w.second._notifyCount;
            pulled.insert(w.second._id);
        }
    }

    while (pulled.size() != n) {
        auto id = _waitForEvent();
        if (id == -1u) continue;

        auto it = _allocatedWorkers.find(id);
        if (
            it->second._notifyCount > 0
            && pulled.count(it->second._id) == 0
        ) {
            --it->second._notifyCount;
            pulled.insert(it->second._id);
        }
    }

    delete &pulled;
    sandbox().unlock();
}

Lazy<DriverContext> DriverContext::instance([] { return new DriverContext(); });

// WorkerContext ///////////////////////////////////////////////////////////////

void WorkerContext::_start(uint32_t id) {
    _id = id;

    _socket = Socket(0, 128);
    auto superDriverSocket = Socket(DriverContext::instance->_superAddress);

    Message m;
    m << OpCode::WORKER_STARTED << _id << _socket.address();
    m.send(superDriverSocket);
}

void WorkerContext::_waitForEvent() {
    while (true) {
        auto &conn = _socket.pollOrAccept();

        Message m;
        try {
            m.recv(conn);
            if (! m.hasData()) continue;
        }
        catch (...) {
            _socket.dispose(conn);
            continue;
        }

        OpCode op;
        m >> op;

        switch (op) {
        case OpCode::RUN_TEST: {
            if (_inTest) break;
            _inTest = true;

            std::string module;
            std::string name;

            m >> module >> name;

            auto it = std::find_if(
                Test::__tests[module].begin(),
                Test::__tests[module].end(),
                [&name] (Test *t) { return t->name() == name; }
            );

            if (it != Test::__tests[module].end()) {
                Test *t = (*it)->copy();
                t->_run();

                auto superDriverSocket = Socket(DriverContext::instance->_superAddress);
                Message m;
                m << OpCode::FINISHED_TEST << _id << t->_status << t->_detailedReport;
                m.send(superDriverSocket);
            }

            _inTest = false;
        }
        break;

        case OpCode::NOTIFY: {
            ++_notifyCount;
        }
        break;

        case OpCode::TERMINATE: {
            exit(0);
        }
        break;

        case OpCode::USER_MESSAGE: {
            _userMessages.push_back(std::move(m));
        }
        break;

        default: break;
        }

        return;
    }
}

Message WorkerContext::createUserMessage() {
    sandbox().lock();

    Message m;
    m << OpCode::USER_MESSAGE << _id;

    sandbox().unlock();

    return m;
}

void WorkerContext::sendUserMessage(Message &message) {
    sandbox().lock();

    {
        auto driverSocket = Socket(DriverContext::instance->_address);
        message.send(driverSocket);
    }

    sandbox().unlock();
}

Message WorkerContext::getUserMessage() {
    sandbox().lock();

    while (_userMessages.empty()) {
        _waitForEvent();
    }
    Message m = _userMessages.front();
    _userMessages.pop_front();

    sandbox().unlock();

    return m;
}

void WorkerContext::notify() {
    sandbox().lock();

    {
        auto driverSocket = Socket(DriverContext::instance->_address);
        Message m;
        m << OpCode::NOTIFY << _id;
        m.send(driverSocket);
    }

    sandbox().unlock();
}

void WorkerContext::wait(uint32_t n) {
    sandbox().lock();

    if (n == -1u) n = 1;

    while (_notifyCount < n) {
        _waitForEvent();
    }

    _notifyCount -= n;

    sandbox().unlock();
}

Lazy<WorkerContext> WorkerContext::instance([] { return new WorkerContext(); });
