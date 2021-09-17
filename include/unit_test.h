#pragma once

#include <test.h>

namespace dtest {

class UnitTest : public Test {

protected:

    // configuration
    uint64_t _timeout = 10 * 1e9;       // 10 seconds
    bool _ignoreMemoryLeak = false;

    // test functions
    std::function<void()> _body;
    std::function<void()> _onInit;
    std::function<void()> _onComplete;

    // time
    uint64_t _initTime = 0;
    uint64_t _bodyTime = 0;
    uint64_t _completeTime = 0;

    bool _inProcessSandbox = false;

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

    inline UnitTest & disable() {
        Test::disable();
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
};

}  // end namespace dtest
