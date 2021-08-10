#include <performance_test.h>
#include <util.h>
#include <time_of.h>

using namespace dtest;

void PerformanceTest::_checkPerformance() {
    if (_bodyTime + _performanceMargin >= _baselineTime) {
        _status = Status::TOO_SLOW;
        err("Failed to meet performance requirements with margin of " + formatDuration(_performanceMargin));
    }
}

void PerformanceTest::_driverRun() {
    auto finish = sandbox().run(
        _timeout,
        [this] {
            _configure();

            sandbox().resourceSnapshot(_usedResources);
            _status = Status::FAIL;

            _initTime = timeOf(_onInit);
            _bodyTime = timeOf(_body);
            _baselineTime = timeOf(_baseline);
            _completeTime = timeOf(_onComplete);

            _status = Status::PASS;
            sandbox().resourceSnapshot(_usedResources);
        },
        [this] (Message &m) {
            _checkMemoryLeak();
            _checkTimeout(_bodyTime);
            _checkPerformance();

            m << _status
                << _usedResources
                << _errors
                << _initTime
                << _bodyTime
                << _completeTime
                << _baselineTime;
        },
        [this] (Message &m) {
            m >> _status
                >> _usedResources
                >> _errors
                >> _initTime
                >> _bodyTime
                >> _completeTime
                >> _baselineTime;
        },
        [this] (const std::string &error) {
            _status = Status::FAIL;
            _errors.push_back(error);
        }
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
