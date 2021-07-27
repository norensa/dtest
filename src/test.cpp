#include <test.h>

#include <sstream>

static std::string indent(const std::string &str, int spaces) {
    std::string in = "";
    in.resize(spaces, ' ');

    std::stringstream s;
    s << in;

    const char *p = str.c_str();
    while (*p != '\0') {
        s << *p;

        if (*p == '\n' && *(p + 1) != '\0') {
            s << in;
        }

        ++p;
    }

    return s.str();
}

///////////////////////////////////////////////////////////////////////////////

std::string Test::__statusString[] = {
    "PENDING",
    "PASS",
    "PASS (with memory leak)",
    "FAIL",
    "TIMEOUT",
};

static std::string __statusStringPastTense[] = {
    "",
    "PASSED",
    "PASSED (with memory leak)",
    "FAILED",
    "TIMED OUT",
};

std::unordered_map<std::string, std::list<Test *>> Test::__tests;

std::list<std::string> Test::__err;

bool Test::_logStatsToStderr = false;

std::string Test::_collectErrorMessages() {
    std::stringstream s;

    if (__err.size() > 0) {
        for (const auto &e : __err) s << e << std::endl;

        __err.clear();
    }

    return s.str();
}

bool Test::runAll(std::ostream &out) {

    bool success = true;

    std::unordered_map<Status, uint32_t> result;

    std::list<Test *> ready;
    std::unordered_map<std::string, std::list<Test *>> blocked;

    std::unordered_map<std::string, std::unordered_set<Test *>> remaining;

    for (const auto &moduleTests : __tests) {
        for (auto t : moduleTests.second) {
            auto tt = t->copy();

            if (t->_dependencies.empty()) {
                ready.push_back(tt);
            }
            else {
                for (const auto &dep: t->_dependencies) {
                    blocked[dep].push_back(tt);
                }
            }

            remaining[t->_module].insert(tt);
        }
    }

    if (_logStatsToStderr) std::cerr << std::endl;

    uint32_t count = 0;
    while (! ready.empty()) {

        auto test = ready.front();
        ready.pop_front();

        auto testnum = std::to_string(count + 1);
        testnum.resize(5, ' ');

        auto testname = test->_module + "::" + test->_name;
        auto shortTestName = testname;
        if (testname.size() > 33) {
            shortTestName = testname.substr(0, 14) + " ... " + testname.substr(testname.size() - 14);
        }
        else {
            shortTestName.resize(33, ' ');
        }

        out << "RUNNING TEST #" << testnum << "  " << testname << "   ";
        out.flush();
        if (_logStatsToStderr) std::cerr << "RUNNING TEST #" << testnum << "  " << shortTestName  << "   ";

        auto statusStr = statusString(test->run());

        out << statusStr << "\n" << indent(test->_detailedReport, 2) << "\n";
        out.flush();
        if (_logStatsToStderr) std::cerr << statusStr << "\n";

        if (test->_status == Status::PASS) {
            auto it = remaining.find(test->_module);
            it->second.erase(test);     // remove from it module's remaining list of tests
            if (it->second.empty()) {   // if entire module test completed
                // find tests blocked on the (now) completed module
                auto it = blocked.find(test->_module);
                if (it != blocked.end()) {
                    for (auto tt : it->second) {
                        // and remove that module from their set of dependencies
                        tt->_dependencies.erase(test->_module);
                        // if no more dependencies are needed, then push to ready queue
                        if (tt->_dependencies.empty()) ready.push_back(tt);
                    }
                }
            }
        }
        else {
            success = false;
        }

        ++result[test->_status];
        ++count;
        delete test;
    }

    // print a dashed line
    std::string line = "";
    line.resize(80, '-');
    out << line << "\n";
    if (_logStatsToStderr) std::cerr << "\n" << line << "\n";

    // print summary
    std::stringstream resultSummary;
    for (const auto &r : result) {
        resultSummary
            << r.second << "/" << count
            << " TESTS " << __statusStringPastTense[(uint32_t) r.first]
            << "\n";
    }
    out << resultSummary.str();
    if (_logStatsToStderr) std::cerr << resultSummary.str();

    return success;
}

void __assertionFailure(const char *expr, const char *file, int line, const char *caller) {
    throw AssertionException(
        std::string("Assertion failed for expression '") + expr + "' "
        + "in file " + file + ":" + std::to_string(line) + " " + caller
    );
}
