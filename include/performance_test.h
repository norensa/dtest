#pragma once

#include <unit_test.h>

namespace dtest {

class PerformanceTest : public UnitTest {

protected:

    std::function<void()> _baseline;

    uint64_t _baselineTime = 0;

    uint64_t _performanceMargin = 1e6;      // 1 ms

    void _checkPerformance();

    void _driverRun() override;

    void _report(bool driver, std::stringstream &s) override;

public:

    inline PerformanceTest(
        const std::string &name
    ): UnitTest(name)
    { }

    inline PerformanceTest(
        const std::string &module,
        const std::string &name
    ): UnitTest(module, name)
    { }

    PerformanceTest * copy() const override {
        return new PerformanceTest(*this);
    }

    ~PerformanceTest() = default;

    inline PerformanceTest & dependsOn(const std::string &dependency) {
        UnitTest::dependsOn(dependency);
        return *this;
    }

    inline PerformanceTest & dependsOn(const std::initializer_list<std::string> &dependencies) {
        UnitTest::dependsOn(dependencies);
        return *this;
    }

    inline PerformanceTest & onInit(const std::function<void()> &onInit) {
        UnitTest::onInit(onInit);
        return *this;
    }

    inline PerformanceTest & body(const std::function<void()> &body) {
        UnitTest::body(body);
        return *this;
    }

    inline PerformanceTest & baseline(const std::function<void()> &baseline) {
        _baseline = baseline;
        return *this;
    }

    inline PerformanceTest & onComplete(const std::function<void()> &onComplete) {
        UnitTest::onComplete(onComplete);
        return *this;
    }

    inline PerformanceTest & timeoutNanos(uint64_t nanos) {
        UnitTest::timeoutNanos(nanos);
        return *this;
    }

    inline PerformanceTest & timeoutMicros(uint64_t micros) {
        UnitTest::timeoutMicros(micros);
        return *this;
    }

    inline PerformanceTest & timeoutMillis(uint64_t millis) {
        UnitTest::timeoutMillis(millis);
        return *this;
    }

    inline PerformanceTest & timeout(uint64_t seconds) {
        UnitTest::timeout(seconds);
        return *this;
    }

    inline PerformanceTest & performanceMarginNanos(uint64_t nanos) {
        _performanceMargin = nanos;
        return *this;
    }

    inline PerformanceTest & performanceMarginMicros(uint64_t micros) {
        return performanceMarginNanos(micros * 1000lu);
    }

    inline PerformanceTest & performanceMarginMillis(uint64_t millis) {
        return performanceMarginNanos(millis * 1000000lu);
    }

    inline PerformanceTest & performanceMargin(uint64_t seconds) {
        return performanceMarginNanos(seconds * 1000000000lu);
    }

    inline PerformanceTest & expect(Status status) {
        UnitTest::expect(status);
        return *this;
    }

    inline PerformanceTest & ignoreMemoryLeak(bool val = true) {
        UnitTest::ignoreMemoryLeak(val);
        return *this;
    }
};

}  // end namespace dtest
