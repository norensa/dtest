#include <unit_test.h>
#include <util.h>
#include <chrono>

using namespace dtest;

void UnitTest::_configure() {
    sandbox().disableFaultyNetwork();
}

uint64_t UnitTest::_timedRun(const std::function<void()> &func) {
    auto start = std::chrono::high_resolution_clock::now();

    if (func) func();

    auto end = std::chrono::high_resolution_clock::now();
    if (! func) end = start;

    return (end - start).count();
}

void UnitTest::_checkMemoryLeak() {
    if (
        ! _ignoreMemoryLeak
        && _usedResources.memory.allocate.size > _usedResources.memory.deallocate.size
    ) {
        _status = Status::PASS_WITH_MEMORY_LEAK;
        err(
            "WARNING: Possible memory leak detected. "
            + formatSize(_usedResources.memory.allocate.size - _usedResources.memory.deallocate.size) + " ("
            + std::to_string(_usedResources.memory.allocate.count - _usedResources.memory.deallocate.count)
            + " block(s)) difference." + sandbox().memoryReport()
        );
    }
}

void UnitTest::_checkTimeout(uint64_t time) {
    if (time > _timeout) {
        _status = Status::TIMEOUT;
        err("Exceeded timeout of " + formatDuration(_timeout));
    }
}

void UnitTest::_driverRun() {
    auto finish = sandbox().run(
        _timeout,
        [this] {
            _configure();

            sandbox().resourceSnapshot(_usedResources);
            _status = Status::FAIL;

            _initTime = _timedRun(_onInit);
            _bodyTime = _timedRun(_body);
            _completeTime = _timedRun(_onComplete);

            _status = Status::PASS;
            sandbox().resourceSnapshot(_usedResources);
        },
        [this] (Message &m) {
            _checkMemoryLeak();
            _checkTimeout(_bodyTime);

            m << _status
                << _usedResources
                << _errors
                << _initTime
                << _bodyTime
                << _completeTime;
        },
        [this] (Message &m) {
            m >> _status
                >> _usedResources
                >> _errors
                >> _initTime
                >> _bodyTime
                >> _completeTime;
        },
        [this] (const std::string &error) {
            _status = Status::FAIL;
            _errors.push_back(error);
        }
    );

    if (! finish) _status = Status::TIMEOUT;
}

bool UnitTest::_hasMemoryReport() {
    return _usedResources.memory.allocate.count > 0
    || _usedResources.memory.deallocate.count > 0;
}

std::string UnitTest::_memoryReport() {
    std::stringstream s;

    if (_usedResources.memory.allocate.count > 0) {
        s << "\"allocated\": {";
        s << "\n  \"size\": " << _usedResources.memory.allocate.size;
        s << ",\n  \"blocks\": " << _usedResources.memory.allocate.count;
        s << "\n}";
        if (_usedResources.memory.deallocate.count > 0) s << ",\n";
    }

    if (_usedResources.memory.deallocate.count > 0) {
        s << "\"freed\": {";
        s << "\n  \"size\": " << _usedResources.memory.deallocate.size;
        s << ",\n  \"blocks\": " << _usedResources.memory.deallocate.count;
        s << "\n}";
    }

    return s.str();
}

void UnitTest::_report(bool driver, std::stringstream &s) {
    if (! _errors.empty()) {
        s << _errorReport() << ",\n";
    }

    s << "\"time\": {";
    if (_initTime > 0) {
        s << "\n  \"initialization\": " << formatDurationJSON(_initTime) << ',';
    }
    s << "\n  \"body\": " << formatDurationJSON(_bodyTime);
    if (_completeTime) {
        s << ",\n  \"cleanup\": " << formatDurationJSON(_completeTime);
    }
    s << "\n}";

    if (_hasMemoryReport()) {
        s << ",\n\"memory\": {\n" << indent(_memoryReport(), 2) << "\n}";
    }
}
