#include <distributed_unit_test.h>
#include <util.h>
#include <sstream>

using namespace dtest;

void DistributedUnitTest::_workerRun() {
    _memoryLeak = sandbox().usedMemory();
    _memoryAllocated = sandbox().allocatedMemorySize();
    _memoryFreed = sandbox().freedMemorySize();
    _blocksAllocated = sandbox().allocatedMemoryBlocks();
    _blocksDeallocated = sandbox().freedMemoryBlocks();

    _workerBodyTime = _timedRun(_workerBody);

    if (_status == Status::PASS && _workerBodyTime > _timeout) {
        err("Exceeded timeout of " + formatDuration(_timeout));
        _status = Status::TIMEOUT;
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
    rep << "\n  \"body\": " << formatDurationJSON(_workerBodyTime);
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
