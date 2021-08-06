#include <unit_test.h>
#include <util.h>
#include <sstream>

using namespace dtest;

uint64_t UnitTest::_timedRun(const std::function<void()> &func) {

    auto start = std::chrono::high_resolution_clock::now();

    if (func) {
        sandbox().enter();

        try {
            _status = Status::FAIL;

            func();

            _status = Status::PASS;
        }
        catch (const AssertionException &e) {
            err(e.msg);
        }
        catch (const SandboxFatalException &e) {
            err(
                std::string("Detected sandbox() fatal error: ") + e.what()
                + ". Caused by:\n" + e.callstack().toString()
            );
        }
        catch (const std::exception &e) {
            err(
                std::string("Detected uncaught exception: ") + e.what()
            );
        }
        catch (...) {
            err("Unknown exception thrown");
        }

        sandbox().exit();
    }

    auto end = std::chrono::high_resolution_clock::now();
    if (! func) end = start;

    return (end - start).count();
}

void UnitTest::_driverRun() {
    _memoryLeak = sandbox().usedMemory();
    _memoryAllocated = sandbox().allocatedMemorySize();
    _memoryFreed = sandbox().freedMemorySize();
    _blocksAllocated = sandbox().allocatedMemoryBlocks();
    _blocksDeallocated = sandbox().freedMemoryBlocks();

    _initTime = _timedRun(_onInit);
    _bodyTime = _timedRun(_body);
    _completeTime = _timedRun(_onComplete);

    if (_status == Status::PASS && _bodyTime > _timeout) {
        _status = Status::TIMEOUT;
        err("Exceeded timeout of " + formatDuration(_timeout));
    }

    std::stringstream rep;

    _memoryLeak = sandbox().usedMemory() - _memoryLeak;
    _memoryAllocated = sandbox().allocatedMemorySize() - _memoryAllocated;
    _memoryFreed = sandbox().freedMemorySize() - _memoryFreed;
    _blocksAllocated = sandbox().allocatedMemoryBlocks() - _blocksAllocated;
    _blocksDeallocated = sandbox().freedMemoryBlocks() - _blocksDeallocated;

    if (_status == Status::PASS && _memoryLeak > 0 && ! _ignoreMemoryLeak) {
        _status = Status::PASS_WITH_MEMORY_LEAK;
        err(
            "WARNING: Possible memory leak detected. "
            + formatSize(_memoryLeak) + " ("
            + std::to_string(_blocksAllocated - _blocksDeallocated)
            + " block(s)) difference." + sandbox().memoryReport()
        );
    }

    sandbox().clearMemoryBlocks();

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
