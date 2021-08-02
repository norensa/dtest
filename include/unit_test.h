#pragma once

#include <test.h>

#include <chrono>

namespace dtest {

class UnitTest : public Test {

protected:

    // configuration
    uint64_t _timeout = -1lu;
    bool _ignoreMemoryLeak = false;

    // test functions
    std::function<void()> _body;
    std::function<void()> _onInit;
    std::function<void()> _onComplete;

    // time
    uint64_t _initTime = 0;
    uint64_t _bodyTime = 0;
    uint64_t _completeTime = 0;

    // memory
    size_t _memoryLeak = 0;
    size_t _memoryAllocated = 0;
    size_t _memoryFreed = 0;
    size_t _blocksAllocated = 0;
    size_t _blocksDeallocated = 0;

    uint64_t _timedRun(const std::function<void()> &func);

    void _driverRun() override;

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

    inline UnitTest & ignoreMemoryLeak(bool val = true) {
        _ignoreMemoryLeak = val;
        return *this;
    }
};

}  // end namespace dtest
