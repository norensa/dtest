#include <unittest.h>

#include <sstream>

static std::string formatDuration(double nanos) {
    std::stringstream s;
    s.setf(std::ios::fixed);
    s.precision(3);
    
    if (nanos < 1e3) s << nanos << " ns";
    else if (nanos < 1e6) s << nanos / 1e3 << " us";
    else if (nanos < 1e9) s << nanos / 1e6 << " ms";
    else s << nanos /1e9 << " s";

    return s.str();
}

static std::string formatSize(size_t size) {
    static const size_t KB = 1024;
    static const size_t MB = 1024 * KB;
    static const size_t GB = 1024 * MB;
    static const size_t TB = 1024 * GB;

    std::stringstream s;
    s.setf(std::ios::fixed);
    s.precision(2);
    
    if (size < KB) s << size << " B";
    else if (size < MB) s << (double) size / KB << " KB";
    else if (size < GB) s << (double) size / MB << " MB";
    else if (size < TB) s << (double) size / GB << " GB";
    else s << (double) size / TB << " TB";

    return s.str();    
}

///////////////////////////////////////////////////////////////////////////////

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

Test::Status UnitTest::run() {
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

    rep << _collectErrorMessages() << "\n";

    rep << "Initialization     " << formatDuration(_initTime) << "\n";

    rep << "Run                " << formatDuration(_bodyTime) << "\n";

    rep << "Cleanup            " << formatDuration(_completeTime) << "\n";

    rep << "Allocated memory   " << formatSize(_memoryAllocated)
        << " (" << _blocksAllocated << " block(s))\n" ;

    rep << "Freed memory       " << formatSize(_memoryFreed)
        << " (" << _blocksDeallocated << " block(s))\n";

    _detailedReport = rep.str();

    return _status;
}
