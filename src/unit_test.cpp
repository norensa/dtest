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
                std::string("Detected fatal error: ") + e.what()
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

void UnitTest::_resourceSnapshot() {
    _memoryLeak = sandbox().usedMemory() - _memoryLeak;
    _memoryAllocated = sandbox().allocatedMemorySize() - _memoryAllocated;
    _memoryFreed = sandbox().freedMemorySize() - _memoryFreed;
    _blocksAllocated = sandbox().allocatedMemoryBlocks() - _blocksAllocated;
    _blocksDeallocated = sandbox().freedMemoryBlocks() - _blocksDeallocated;
}

void UnitTest::_checkMemoryLeak() {
    if (_status == Status::PASS && _memoryLeak > 0 && ! _ignoreMemoryLeak) {
        _status = Status::PASS_WITH_MEMORY_LEAK;
        err(
            "WARNING: Possible memory leak detected. "
            + formatSize(_memoryLeak) + " ("
            + std::to_string(_blocksAllocated - _blocksDeallocated)
            + " block(s)) difference." + sandbox().memoryReport()
        );
    }
}

void UnitTest::_checkTimeout(uint64_t time) {
    if (_status == Status::PASS && time > _timeout) {
        _status = Status::TIMEOUT;
        err("Exceeded timeout of " + formatDuration(_timeout));
    }
}

std::string UnitTest::_memoryReport() {
    std::stringstream s;

    s << "\n\"allocated\": {";
    s << "\n  \"size\": " << _memoryAllocated;
    s << ",\n  \"blocks\": " << _blocksAllocated;
    s << "\n},";
    s << "\n\"freed\": {";
    s << "\n  \"size\": " << _memoryFreed;
    s << ",\n  \"blocks\": " << _blocksDeallocated;
    s << "\n}";

    return s.str();
}

void UnitTest::_driverRun() {
    // take a snapshot of current resource usage
    _resourceSnapshot();

    // run
    _initTime = _timedRun(_onInit);
    _bodyTime = _timedRun(_body);
    _completeTime = _timedRun(_onComplete);

    // take another snapshot to find the difference
    _resourceSnapshot();

    // post-run checks
    _checkMemoryLeak();
    _checkTimeout(_bodyTime);

    // clear any leaked memory blocks
    sandbox().clearMemoryBlocks();

    // generate report
    std::stringstream s;
    if (_hasErrors()) s << _collectErrorMessages() << ",\n";
    s << "\"time\": {";
    s << "\n  \"initialization\": " << formatDurationJSON(_initTime);
    s << ",\n  \"body\": " << formatDurationJSON(_bodyTime);
    s << ",\n  \"cleanup\": " << formatDurationJSON(_completeTime);
    s << "\n},";
    s << "\n\"memory\": {\n" << indent(_memoryReport(), 2) << "\n}";
    _detailedReport = s.str();
}
