/*
 * Copyright (c) 2021 Noah Orensa.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
*/

#pragma once

#include <dtest_core/test.h>

namespace dtest {

class UnitTest : public Test {

protected:

    // configuration
    uint64_t _timeout = 10 * 1e9;       // 10 seconds
    bool _ignoreMemoryLeak = false;
    size_t _memoryBytesLimit = (size_t) -1;
    size_t _memoryBlocksLimit = (size_t) -1;
    Buffer _input;
    Buffer _out;
    Buffer _err;

    // test functions
    std::function<void()> _body;
    std::function<void()> _onInit;
    std::function<void()> _onComplete;

    // time
    uint64_t _initTime = 0;
    uint64_t _bodyTime = 0;
    uint64_t _completeTime = 0;

    bool _inProcessSandbox = false;
    bool _resourceSnapshotBodyOnly = false;

    virtual void _configure();

    void _checkMemoryLeak();

    void _checkTimeout(uint64_t time);

    void _driverRun() override;

    bool _hasMemoryReport();

    std::string _memoryReport();

    void _report(bool driver, std::stringstream &s) override;

public:

    inline UnitTest(
        const std::string &name
    ): Test(name)
    { }

    inline UnitTest(
        const std::string &module,
        const std::string &name
    ): Test(module, name)
    { }

    UnitTest * copy() const override {
        return new UnitTest(*this);
    }

    ~UnitTest() = default;

    inline UnitTest & dependsOn(const std::string &dependency) {
        Test::dependsOn(dependency);
        return *this;
    }

    inline UnitTest & dependsOn(const std::initializer_list<std::string> &dependencies) {
        Test::dependsOn(dependencies);
        return *this;
    }

    inline UnitTest & onInit(const std::function<void()> &onInit) {
        _onInit = onInit;
        return *this;
    }

    inline UnitTest & body(const std::function<void()> &body) {
        _body = body;
        return *this;
    }

    inline UnitTest & onComplete(const std::function<void()> &onComplete) {
        _onComplete = onComplete;
        return *this;
    }

    inline UnitTest & timeoutNanos(uint64_t nanos) {
        _timeout = nanos;
        return *this;
    }

    inline UnitTest & timeoutMicros(uint64_t micros) {
        return timeoutNanos(micros * 1000lu);
    }

    inline UnitTest & timeoutMillis(uint64_t millis) {
        return timeoutNanos(millis * 1000000lu);
    }

    inline UnitTest & timeout(uint64_t seconds) {
        return timeoutNanos(seconds * 1000000000lu);
    }

    inline UnitTest & expect(Status status) {
        Test::expect(status);
        return *this;
    }

    inline UnitTest & memoryBytesLimit(size_t bytes) {
        _memoryBytesLimit = bytes;
        return *this;
    }

    inline UnitTest & memoryBlocksLimit(size_t blocks) {
        _memoryBlocksLimit = blocks;
        return *this;
    }

    inline UnitTest & disable() {
        Test::disable();
        return *this;
    }

    inline UnitTest & enable() {
        Test::enable();
        return *this;
    }

    inline UnitTest & ignoreMemoryLeak(bool val = true) {
        _ignoreMemoryLeak = val;
        return *this;
    }

    inline UnitTest & inProcess(bool val = true) {
        _inProcessSandbox = val;
        return *this;
    }

    inline UnitTest & input(const std::string &input) {
        _input = { input.data(), input.size() };
        return *this;
    }

    inline UnitTest & resourceSnapshotBodyOnly(bool val = true) {
        _resourceSnapshotBodyOnly = val;
        return *this;
    }
};

}  // end namespace dtest
