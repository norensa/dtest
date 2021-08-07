#include <unit_test.h>
#include <util.h>

using namespace dtest;

void UnitTest::_configure() {
    sandbox().disableFaultyNetwork();
}

uint64_t UnitTest::_timedRun(const std::function<void()> &func) {

    _configure();

    auto start = std::chrono::high_resolution_clock::now();

    if (func) {
        try {
            _status = Status::FAIL;

            sandbox().enter();
            func();
            sandbox().exit();

            _status = Status::PASS;
        }
        catch (const SandboxFatalException &e) {
            // sandbox().exit() is invoked internally
            err(
                std::string("Detected fatal error: ") + e.what()
                + ". Caused by:\n" + indent(e.callstack().toString(), 2)
            );
        }
        catch (const AssertionException &e) {
            // sandbox().exit() is invoked internally
            err(e.msg);
        }
        catch (const std::exception &e) {
            sandbox().exit();
            err(
                std::string("Detected uncaught exception: ") + e.what()
            );
        }
        catch (...) {
            sandbox().exit();
            err("Unknown exception thrown");
        }
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
}

bool UnitTest::_hasMemoryReport() {
    return _blocksAllocated > 0 || _blocksDeallocated > 0;
}

std::string UnitTest::_memoryReport() {
    std::stringstream s;

    if (_blocksAllocated > 0) {
        s << "\"allocated\": {";
        s << "\n  \"size\": " << _memoryAllocated;
        s << ",\n  \"blocks\": " << _blocksAllocated;
        s << "\n}";
        if (_blocksDeallocated > 0) s << ",\n";
    }

    if (_blocksDeallocated > 0) {
        s << "\"freed\": {";
        s << "\n  \"size\": " << _memoryFreed;
        s << ",\n  \"blocks\": " << _blocksDeallocated;
        s << "\n}";
    }

    return s.str();
}

void UnitTest::_report(bool driver, std::stringstream &s) {
    if (_hasErrors()) {
        s << _collectErrorMessages() << ",\n";
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
