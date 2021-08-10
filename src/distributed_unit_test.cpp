#include <distributed_unit_test.h>
#include <util.h>
#include <time_of.h>

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

void DistributedUnitTest::_workerRun() {
    auto finish = sandbox().run(
        _timeout,
        [this] {
            _configure();

            sandbox().resourceSnapshot(_usedResources);
            _status = Status::FAIL;

            _workerBodyTime = timeOf(_workerBody);

            _status = Status::PASS;
            sandbox().resourceSnapshot(_usedResources);
        },
        [this] (Message &m) {
            _checkMemoryLeak();
            _checkTimeout(_bodyTime);

            m << _status
                << _usedResources
                << _errors
                << _workerBodyTime;
        },
        [this] (Message &m) {
            m >> _status
                >> _usedResources
                >> _errors
                >> _workerBodyTime;
        },
        [this] (const std::string &error) {
            _status = Status::FAIL;
            _errors.push_back(error);
        }
    );

    if (! finish) _status = Status::TIMEOUT;
}

bool DistributedUnitTest::_hasNetworkReport() {
    return _usedResources.network.send.count > 0
    || _usedResources.network.receive.count > 0;
}

std::string DistributedUnitTest::_networkReport() {
    std::stringstream s;

    if (_usedResources.network.send.count > 0) {
        s << "\"send\": {";
        s << "\n  \"size\": " << _usedResources.network.send.size;
        s << ",\n  \"count\": " << _usedResources.network.send.count;
        s << "\n}";
        if (_usedResources.network.receive.count > 0) s << ",\n";
    }

    if (_usedResources.network.receive.count > 0) {
        s << "\"receive\": {";
        s << "\n  \"size\": " << _usedResources.network.receive.size;
        s << ",\n  \"count\": " << _usedResources.network.receive.count;
        s << "\n}";
    }

    return s.str();
}

void DistributedUnitTest::_report(bool driver, std::stringstream &s) {
    if (driver) {
        UnitTest::_report(driver, s);
    }
    else {
        if (! _errors.empty()) {
           s << _errorReport() << ",\n";
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
