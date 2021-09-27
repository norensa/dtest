#include <dtest_core/unit_test.h>
#include <dtest_core/util.h>
#include <dtest_core/time_of.h>

using namespace dtest;

void UnitTest::_configure() {
    sandbox().disableFaultyNetwork();
}

void UnitTest::_checkMemoryLeak() {
    if (
        ! _ignoreMemoryLeak
        && _usedResources.memory.allocate.size > _usedResources.memory.deallocate.size
    ) {
        _status = Status::PASS_WITH_MEMORY_LEAK;
        err(
            "WARNING: Possible memory leak detected. "
            + formatSize(_usedResources.memory.allocate.size - _usedResources.memory.deallocate.size) + " ("
            + std::to_string(_usedResources.memory.allocate.count - _usedResources.memory.deallocate.count)
            + " block(s)) difference." + sandbox().memoryReport()
        );
    }
}

void UnitTest::_checkTimeout(uint64_t time) {
    if (time > _timeout) {
        _status = Status::TIMEOUT;
        err("Exceeded timeout of " + formatDuration(_timeout));
    }
}

void UnitTest::_driverRun() {
    auto opt = Sandbox::Options();
    opt.fork(! _inProcessSandbox);
    opt.input(_input);

    auto finish = sandbox().run(
        _timeout < 2000000000lu ? 2000000000lu : _timeout,
        [this] {
            _configure();

            sandbox().resourceSnapshot(_usedResources);
            _status = Status::FAIL;

            _initTime = timeOf(_onInit);
            _bodyTime = timeOf(_body);
            _completeTime = timeOf(_onComplete);

            _status = Status::PASS;
            sandbox().resourceSnapshot(_usedResources);
        },
        [this] (Message &m) {
            _checkMemoryLeak();
            _checkTimeout(_bodyTime);

            m << _status
                << _usedResources
                << _errors
                << _initTime
                << _bodyTime
                << _completeTime;
        },
        [this] (Message &m) {
            m >> _status
                >> _usedResources
                >> _errors
                >> _initTime
                >> _bodyTime
                >> _completeTime;
        },
        [this] (const std::string &error) {
            _status = Status::FAIL;
            _errors.push_back(error);
        },
        opt
    );

    _out = std::move(opt.output());
    _err = std::move(opt.error());

    if (! finish) _status = Status::TIMEOUT;
}

bool UnitTest::_hasMemoryReport() {
    return _usedResources.memory.allocate.count > 0
    || _usedResources.memory.deallocate.count > 0;
}

std::string UnitTest::_memoryReport() {
    std::stringstream s;

    if (_usedResources.memory.allocate.count > 0) {
        s << "\"allocated\": {";
        s << "\n  \"size\": " << _usedResources.memory.allocate.size;
        s << ",\n  \"blocks\": " << _usedResources.memory.allocate.count;
        s << "\n}";
        if (_usedResources.memory.deallocate.count > 0) s << ",\n";
    }

    if (_usedResources.memory.deallocate.count > 0) {
        s << "\"freed\": {";
        s << "\n  \"size\": " << _usedResources.memory.deallocate.size;
        s << ",\n  \"blocks\": " << _usedResources.memory.deallocate.count;
        s << "\n}";
    }

    return s.str();
}

void UnitTest::_report(bool driver, std::stringstream &s) {
    if (! _errors.empty()) {
        s << _errorReport() << ",\n";
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

    if (_out.size() > 0) {
        s << ",\n\"stdout\": \n" << indent(jsonify(std::string((const char *) _out.data(), _out.size())), 2);
    }

    if (_err.size() > 0) {
        s << ",\n\"stderr\": \n" << indent(jsonify(std::string((const char *) _err.data(), _err.size())), 2);
    }
}
