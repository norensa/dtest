#pragma once

#define __dtest_concat(a,b) __dtest_concat2(a,b)    // force expand
#define __dtest_concat2(a,b) a ## b                 // actually concatenate

#define __dtest_instance(t) static auto __dtest_concat(__unit_test__uid_, __COUNTER__) = (*(t))

////

#include <unit_test.h>

#define __unit_1(name) __dtest_instance(new dtest::UnitTest(name))
#define __unit_2(module,name) __dtest_instance(new dtest::UnitTest(module, name))

#define __unit(_1, _2, NAME, ...) NAME
#define unit(...) __unit(__VA_ARGS__, __unit_2, __unit_1)(__VA_ARGS__)

////

#include <distributed_unit_test.h>

#define __dunit_1(name) __dtest_instance(new dtest::DistributedUnitTest(name))
#define __dunit_2(module,name) __dtest_instance(new dtest::DistributedUnitTest(module, name))

#define __dunit(_1, _2, NAME, ...) NAME
#define dunit(...) __dunit(__VA_ARGS__, __dunit_2, __dunit_1)(__VA_ARGS__)

#define notify() dtest::Context::instance()->notify()

#define __wait_0() dtest::Context::instance()->wait()
#define __wait_1(n) dtest::Context::instance()->wait(n)

#define __wait(_1, NAME, ...) NAME
#define wait(...)  __wait(__VA_ARGS__, __wait_1, __wait_0)(__VA_ARGS__)
