#pragma once

#include <vector>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <string>
#include <stdint.h>

#include <memory_watch.h>

class Test {

public:

    enum class Status {
        PENDING,
        PASS,
        PASS_WITH_MEMORY_LEAK,
        FAIL,
        TIMEOUT,
    };

protected:

    std::string _module;
    std::string _name;

    std::unordered_set<std::string> _dependencies;

    Status _status = Status::PENDING;
    std::string _detailedReport = "";

    virtual Status run() = 0;

public:

    inline Test(
        const std::string &name
    ): _module("root"),
       _name(name)
    {
        __tests[_module].push_back(this);
    }

    inline Test(
        const std::string &module,
        const std::string &name
    ): _module(module),
       _name(name)
    {
        __tests[_module].push_back(this);
    }

    virtual Test * copy() const = 0;

    virtual ~Test() = default;

    inline Test & dependsOn(const std::string &dependency) {
        _dependencies.insert(dependency);
        return *this;
    }

    inline Test & dependsOn(const std::initializer_list<std::string> &dependencies) {
        _dependencies.insert(dependencies.begin(), dependencies.end());
        return *this;
    }

    inline const std::string & name() const {
        return _name;
    }

    inline const std::string & module() const {
        return _module;
    }

private:

    static std::string __statusString[];

    static std::unordered_map<std::string, std::list<Test *>> __tests;

    static std::list<std::string> __err;

    static bool _logStatsToStderr;

protected:

    static std::string _collectErrorMessages();

public:

    static inline const std::string & statusString(const Status &status) {
        return __statusString[(uint32_t) status];
    }

    static inline void logError(const std::string &message) {
        bool tmp = MemoryWatch::isTracking();
        MemoryWatch::track(false);
        __err.push_back(message);
        MemoryWatch::track(tmp);
    }

    static inline void logStatsToStderr(bool val) {
        _logStatsToStderr = val;
    }

    static bool runAll(std::ostream &out = std::cout);
};

struct AssertionException {
    std::string msg;

    AssertionException(const std::string &msg)
    : msg(msg)
    { }
};

void __assertionFailure(const char *expr, const char *file, int line, const char *caller);

#define assert(expr) \
    if (! static_cast<bool>(expr)) { \
        __assertionFailure(#expr, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
    }

#define err(msg) Test::logError(msg)
