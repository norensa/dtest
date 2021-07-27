#pragma once

#include <unittest.h>

#define __test(name, t) static auto name = (*(t))

#define __unit_1(name) __test(__unittest__ ## name, new UnitTest(#name))
#define __unit_2(module,name) __test(__unittest__ ## module ## _ ## name, new UnitTest(#module, #name))

#define __unit(_1, _2, NAME, ...) NAME
#define unit(...) __unit(__VA_ARGS__, __unit_2, __unit_1)(__VA_ARGS__)
