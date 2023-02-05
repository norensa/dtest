/*
 * Copyright (c) 2021 Noah Orensa.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
*/

#pragma once

#include <dtest_core/unit_test.h>

namespace dtest {

class DistributedUnitTest : public UnitTest {

protected:

    std::function<void()> _workerBody;

    uint64_t _workerBodyTime = 0;

    bool _faultyNetwork = false;
    double _faultyNetworkChance = 1;
    uint64_t _faultyNetworkHoleDuration = 0;

    bool _distributed() const override {
        return true;
    }

    void _configure() override;

    void _workerRun() override;

    bool _hasNetworkReport();

    std::string _networkReport();

    void _report(bool driver, std::stringstream &s) override;

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

    inline DistributedUnitTest & memoryBytesLimit(size_t bytes) {
        UnitTest::memoryBytesLimit(bytes);
        return *this;
    }

    inline DistributedUnitTest & memoryBlocksLimit(size_t blocks) {
        UnitTest::memoryBlocksLimit(blocks);
        return *this;
    }

    inline DistributedUnitTest & disable() {
        UnitTest::disable();
        return *this;
    }

    inline DistributedUnitTest & enable() {
        UnitTest::enable();
        return *this;
    }

    inline DistributedUnitTest & ignoreMemoryLeak(bool val = true) {
        UnitTest::ignoreMemoryLeak(val);
        return *this;
    }

    inline DistributedUnitTest & inProcess(bool val = true) {
        UnitTest::inProcess(val);
        return *this;
    }

    inline DistributedUnitTest & input(const std::string &input) {
        UnitTest::input(input);
        return *this;
    }

    inline DistributedUnitTest & resourceSnapshotBodyOnly(bool val = true) {
        UnitTest::resourceSnapshotBodyOnly(val);
        return *this;
    }

    inline DistributedUnitTest & workers(uint16_t numWorkers) {
        _numWorkers = numWorkers;
        return *this;
    }

    inline DistributedUnitTest & faultyNetwork(double chance = 0.9, uint64_t holeDurationMillis = 10) {
        _faultyNetwork = true;
        _faultyNetworkChance = chance;
        _faultyNetworkHoleDuration = holeDurationMillis * 1e6;
        return *this;
    }
};

}  // end namespace dtest

