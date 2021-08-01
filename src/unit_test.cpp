#include <unit_test.h>
#include <util.h>
#include <sstream>

using namespace dtest;

uint64_t UnitTest::_timedRun(const std::function<void()> &func) {

    auto start = std::chrono::high_resolution_clock::now();

    if (func) {
        MemoryWatch::track(true);

        try {
            func();

            if (_expectFailure) {
                err("Expected failure");
                _status = Status::FAIL;
            }
            else {
                _status = Status::PASS;
            }
        }
        catch (const AssertionException &e) {
            err(e.msg);

            if (_expectFailure) {
                MemoryWatch::clear();
                _status = Status::PASS;
            }
            else {
                _status = Status::FAIL;
            }
        }
        catch (const std::exception &e) {
            err(e.what());

            if (_expectFailure) {
                MemoryWatch::clear();
                _status = Status::PASS;
            }
            else {
                _status = Status::FAIL;
            }
        }
        catch (...) {
            if (_expectFailure) {
                MemoryWatch::clear();
                _status = Status::PASS;
            }
            else {
                err("Unknown exception thrown");
                _status = Status::FAIL;
            }
        }

        MemoryWatch::track(false);
    }

    auto end = std::chrono::high_resolution_clock::now();
    if (! func) end = start;

    return (end - start).count();
}

void UnitTest::_driverRun() {
    _memoryLeak = MemoryWatch::totalUsedMemory();
    _memoryAllocated = MemoryWatch::totalAllocated();
    _memoryFreed = MemoryWatch::totalFreed();
    _blocksAllocated = MemoryWatch::allocatedBlocks();
    _blocksDeallocated = MemoryWatch::freedBlocks();

    _initTime = _timedRun(_onInit);
    _bodyTime = _timedRun(_body);
    _completeTime = _timedRun(_onComplete);

    if (_status == Status::PASS) {
        if (_bodyTime > _timeout) {
            err("Exceeded timeout of " + formatDuration(_timeout));
            if (! _expectTimeout) _status = Status::TIMEOUT;
        }
        else if (_expectTimeout) {
            err("Expected timeout after " + formatDuration(_timeout));
            _status = Status::FAIL;
        }
    }

    std::stringstream rep;

    _memoryLeak = MemoryWatch::totalUsedMemory() - _memoryLeak;
    _memoryAllocated = MemoryWatch::totalAllocated() - _memoryAllocated;
    _memoryFreed = MemoryWatch::totalFreed() - _memoryFreed;
    _blocksAllocated = MemoryWatch::allocatedBlocks() - _blocksAllocated;
    _blocksDeallocated = MemoryWatch::freedBlocks() - _blocksDeallocated;

    if (_status == Status::PASS && _memoryLeak > 0 && ! _ignoreMemoryLeak) {
        _status = Status::PASS_WITH_MEMORY_LEAK;

        err(
            "WARNING: Possible memory leak detected. "
            + formatSize(_memoryLeak) + " ("
            + std::to_string(_blocksAllocated - _blocksDeallocated)
            + " block(s)) difference." + MemoryWatch::report()
        );
    }

    MemoryWatch::clear();

    if (_hasErrors()) {
        rep << _collectErrorMessages() << ",\n";
    }

    rep << "\"time\": {";
    rep << "\n  \"initialization\": " << formatDurationJSON(_initTime);
    rep << ",\n  \"body\": " << formatDurationJSON(_bodyTime);
    rep << ",\n  \"cleanup\": " << formatDurationJSON(_completeTime);
    rep << "\n},";

    rep << "\n\"memory\": {";
    rep << "\n  \"allocated\": {";
    rep << "\n    \"size\": " << _memoryAllocated;
    rep << ",\n    \"blocks\": " << _blocksAllocated;
    rep << "\n  },";
    rep << "\n  \"freed\": {";
    rep << "\n    \"size\": " << _memoryFreed;
    rep << ",\n    \"blocks\": " << _blocksDeallocated;
    rep << "\n  }";
    rep << "\n}";

    _detailedReport = rep.str();
}
