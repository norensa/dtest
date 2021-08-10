#pragma once

#include <test.h>

using Status = dtest::Test::Status;

#define __dtest_concat(a,b) __dtest_concat2(a,b)    // force expand
#define __dtest_concat2(a,b) a ## b                 // actually concatenate

#define __dtest_instance(t) static auto __dtest_concat(__unit_test__uid_, __COUNTER__) = (*(t))

#define __test_1(testType, name) __dtest_instance(new testType(name))
#define __test_2(testType, module,name) __dtest_instance(new testType(module, name))
#define __test_(_1, _2, NAME, ...) NAME
#define __test(testType, ...) __test_(__VA_ARGS__, __test_2, __test_1, UNUSED)(testType, __VA_ARGS__)

////

#include <unit_test.h>

#define unit(...) __test(dtest::UnitTest, __VA_ARGS__)

////

#include <performance_test.h>

#define perf(...) __test(dtest::PerformanceTest, __VA_ARGS__)

////

#include <distributed_unit_test.h>

#define dunit(...) __test(dtest::DistributedUnitTest, __VA_ARGS__)

#define notify() dtest::Context::instance()->notify()

#define __wait_0() dtest::Context::instance()->wait()
#define __wait_1(n) dtest::Context::instance()->wait(n)

#define __wait(_1, NAME, ...) NAME
#define wait(...)  __wait(__VA_ARGS__, __wait_1, __wait_0, UNUSED)(__VA_ARGS__)

#define sendMsg(m) dtest::Context::instance()->sendUserMessage(dtest::Context::instance()->createUserMessage() << m)
#define recvMsg() dtest::Context::instance()->getUserMessage()

////

#include <random.h>

#define random() dtest::frand()

////

#include <time_of.h>

#define timeOf(code) dtest::timeOf(code)
