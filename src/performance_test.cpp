/*
 * Copyright (c) 2021 Noah Orensa.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
*/

#include <dtest_core/performance_test.h>
#include <dtest_core/util.h>
#include <dtest_core/time_of.h>

using namespace dtest;

void PerformanceTest::_checkPerformance() {
    if (_performanceMarginRatio == 0) {
        if (_bodyTime + _performanceMargin >= _baselineTime) {
            _status = Status::TOO_SLOW;
            err(
                "Failed to meet performance requirements with a margin of "
                + formatDuration(_performanceMargin)
            );
        }
    }
    else {
        if (_bodyTime > _baselineTime * _performanceMarginRatio) {
            _status = Status::TOO_SLOW;
            err(
                "Failed to meet performance requirements of "
                + std::to_string(_performanceMarginRatio) + " of the baseline time"
            );
        }
    }
}

void PerformanceTest::_driverRun() {
    UnitTest::_driverRun();

    auto opt = Sandbox::Options();
    opt.fork(! _inProcessSandbox);

    auto finish = sandbox().run(
        _timeout < 2000000000lu ? 2000000000lu : _timeout,
        [this] {
            _configure();

            timeOf(_onInit);
            _baselineTime = timeOf(_baseline);
            timeOf(_onComplete);
        },
        [this] (Message &m) {
            _checkMemoryLeak();
            _checkPerformance();

            m << _status
                << _errors
                << _baselineTime;
        },
        [this] (Message &m) {
            m >> _status
                >> _errors
                >> _baselineTime;
        },
        [this] (const std::string &error) {
            _status = Status::FAIL;
            _errors.push_back(error);
        },
        opt
    );

    if (! finish) _status = Status::TIMEOUT;
}

void PerformanceTest::_report(bool driver, std::stringstream &s) {
    if (! _errors.empty()) {
        s << _errorReport() << ",\n";
    }

    s << "\"time\": {";
    if (_initTime > 0) {
        s << "\n  \"initialization\": " << formatDurationJSON(_initTime) << ',';
    }
    s << "\n  \"body\": " << formatDurationJSON(_bodyTime);
    s << ",\n  \"baseline\": " << formatDurationJSON(_baselineTime);
    if (_completeTime) {
        s << ",\n  \"cleanup\": " << formatDurationJSON(_completeTime);
    }
    s << "\n}";

    if (_hasMemoryReport()) {
        s << ",\n\"memory\": {\n" << indent(_memoryReport(), 2) << "\n}";
    }
}
