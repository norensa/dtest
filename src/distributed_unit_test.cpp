#include <distributed_unit_test.h>
#include <util.h>
#include <sstream>

using namespace dtest;

void DistributedUnitTest::_workerRun() {
    // take a snapshot of current resource usage
    _resourceSnapshot();

    // run
    _workerBodyTime = _timedRun(_workerBody);

    // take another snapshot to find the difference
    _resourceSnapshot();

    // post-run checks
    _checkMemoryLeak();
    _checkTimeout(_workerBodyTime);

    // clear any leaked memory blocks
    sandbox().clearMemoryBlocks();

    // generate report
    std::stringstream s;
    if (_hasErrors()) s << _collectErrorMessages() << ",\n";
    s << "\"time\": {";
    s << "\n  \"body\": " << formatDurationJSON(_workerBodyTime);
    s << "\n},";
    s << "\n\"memory\": {\n" << indent(_memoryReport(), 2) << "\n}";
    _detailedReport = s.str();
}
