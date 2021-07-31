#include <distributed_unit_test.h>
#include <util.h>
#include <sstream>

using namespace dtest;

void DistributedUnitTest::_workerRun() {
    _memoryLeak = MemoryWatch::totalUsedMemory();
    _memoryAllocated = MemoryWatch::totalAllocated();
    _memoryFreed = MemoryWatch::totalFreed();
    _blocksAllocated = MemoryWatch::allocatedBlocks();
    _blocksDeallocated = MemoryWatch::freedBlocks();

    _workerBodyTime = _timedRun(_workerBody);

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

    rep << "Run                " << formatDuration(_workerBodyTime) << "\n";

    rep << "Allocated memory   " << formatSize(_memoryAllocated)
        << " (" << _blocksAllocated << " block(s))\n" ;

    rep << "Freed memory       " << formatSize(_memoryFreed)
        << " (" << _blocksDeallocated << " block(s))\n";

    _detailedReport = rep.str();
}
