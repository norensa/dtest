#include <distributed_unit_test.h>
#include <util.h>

using namespace dtest;

void DistributedUnitTest::_configure() {
    UnitTest::_configure();

    if (_faultyNetwork) {
        sandbox().enableFaultyNetwork(
            _faultyNetworkChance,
            _faultyNetworkHoleDuration
        );
    }
}

void DistributedUnitTest::_resourceSnapshot() {
    UnitTest::_resourceSnapshot();

    _sendSize = sandbox().networkSendSize() - _sendSize;
    _sendCount = sandbox().networkSendCount() - _sendCount;
    _recvSize = sandbox().networkReceiveSize() - _recvSize;
    _recvCount = sandbox().networkReceiveCount() - _recvCount;
}

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
}

bool DistributedUnitTest::_hasNetworkReport() {
    return _sendCount > 0 || _recvCount > 0;
}

std::string DistributedUnitTest::_networkReport() {
    std::stringstream s;

    if (_sendCount > 0) {
        s << "\"send\": {";
        s << "\n  \"size\": " << _sendSize;
        s << ",\n  \"count\": " << _sendCount;
        s << "\n}";
        if (_recvCount > 0) s << ",\n";
    }

    if (_recvCount > 0) {
        s << "\"receive\": {";
        s << "\n  \"size\": " << _recvSize;
        s << ",\n  \"count\": " << _recvCount;
        s << "\n}";
    }

    return s.str();
}

void DistributedUnitTest::_report(bool driver, std::stringstream &s) {
    if (driver) {
        UnitTest::_report(driver, s);
    }
    else {
        if (_hasErrors()) {
           s << _collectErrorMessages() << ",\n";
        }

        s << "\"time\": {";
        s << "\n  \"body\": " << formatDurationJSON(_workerBodyTime);
        s << "\n}";

        if (_hasMemoryReport()) {
            s << ",\n\"memory\": {\n" << indent(_memoryReport(), 2) << "\n}";
        }
    }

    if (_hasNetworkReport()) {
        s << ",\n\"network\": {\n" << indent(_networkReport(), 2) << "\n}";
    }
}
