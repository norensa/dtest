#pragma once

#include <unit_test.h>

namespace dtest {

class DistributedUnitTest : public UnitTest {

protected:

    std::function<void()> _workerBody;

    uint64_t _workerBodyTime = 0;

    bool _distributed() const override {
        return true;
    }

    void _workerRun() override;

public:

    inline DistributedUnitTest(
        const std::string &name
    ): UnitTest(name)
    { }

    inline DistributedUnitTest(
        const std::string &module,
        const std::string &name
    ): UnitTest(module, name)
    { }

    DistributedUnitTest * copy() const override {
        return new DistributedUnitTest(*this);
    }

    ~DistributedUnitTest() = default;

    inline DistributedUnitTest & dependsOn(const std::string &dependency) {
        UnitTest::dependsOn(dependency);
        return *this;
    }

    inline DistributedUnitTest & dependsOn(const std::initializer_list<std::string> &dependencies) {
        UnitTest::dependsOn(dependencies);
        return *this;
    }

    inline DistributedUnitTest & onInit(const std::function<void()> &onInit) {
        UnitTest::onInit(onInit);
        return *this;
    }

    UnitTest & body(const std::function<void()> &) = delete;

    inline DistributedUnitTest & driver(const std::function<void()> &body) {
        UnitTest::body(body);
        return *this;
    }

    inline DistributedUnitTest & worker(const std::function<void()> &body) {
        _workerBody = body;
        return *this;
    }

    inline DistributedUnitTest & onComplete(const std::function<void()> &onComplete) {
        UnitTest::onComplete(onComplete);
        return *this;
    }

    inline DistributedUnitTest & timeoutNanos(uint64_t nanos) {
        UnitTest::timeoutNanos(nanos);
        return *this;
    }

    inline DistributedUnitTest & timeoutMicros(uint64_t micros) {
        UnitTest::timeoutMicros(micros);
        return *this;
    }

    inline DistributedUnitTest & timeoutMillis(uint64_t millis) {
        UnitTest::timeoutMillis(millis);
        return *this;
    }

    inline DistributedUnitTest & timeout(uint64_t seconds) {
        UnitTest::timeout(seconds);
        return *this;
    }

    inline DistributedUnitTest & expect(Status status) {
        UnitTest::expect(status);
        return *this;
    }

    inline DistributedUnitTest & ignoreMemoryLeak(bool val = true) {
        UnitTest::ignoreMemoryLeak(val);
        return *this;
    }

    inline DistributedUnitTest & workers(uint16_t numWorkers) {
        _numWorkers = numWorkers;
        return *this;
    }
};

}  // end namespace dtest

