#pragma once

#include <test.h>

using Status = dtest::Test::Status;

#define __dtest_concat(a,b) __dtest_concat2(a,b)    // force expand
#define __dtest_concat2(a,b) a ## b                 // actually concatenate

#define __dtest_instance(t) static auto __dtest_concat(__unit_test__uid_, __COUNTER__) = (*(t))

#define __test_1__(testType, name) __dtest_instance(new testType(name))
#define __test_2__(testType, module,name) __dtest_instance(new testType(module, name))
#define __test___(_1, _2, NAME, ...) NAME
#define __test__(testType, ...) __test___(__VA_ARGS__, __test_2__, __test_1__, UNUSED)(testType, __VA_ARGS__)

////

#include <unit_test.h>

#define unit(...) __test__(dtest::UnitTest, __VA_ARGS__)

////

#include <performance_test.h>

#define perf(...) __test__(dtest::PerformanceTest, __VA_ARGS__)

////

#include <distributed_unit_test.h>

#define dunit(...) __test__(dtest::DistributedUnitTest, __VA_ARGS__)

#define dtest_notify() dtest::Context::instance()->notify()
#define dtest_wait(n) dtest::Context::instance()->wait(n)

#define dtest_sendMsg(m) dtest::Context::instance()->sendUserMessage(dtest::Context::instance()->createUserMessage() << m)
#define dtest_recvMsg() dtest::Context::instance()->getUserMessage()

////

#include <random.h>

#define dtest_random() dtest::frand()

////

#include <time_of.h>

#define dtest_timeOf(code) dtest::timeOf(code)
