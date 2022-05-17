/*
 * Copyright (c) 2021 Noah Orensa.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
*/

#include <dtest_core/time_of.h>

#include <chrono>

uint64_t dtest::timeOf(const std::function<void()> &func) {
    auto start = std::chrono::high_resolution_clock::now();

    if (func) func();

    auto end = std::chrono::high_resolution_clock::now();
    if (! func) end = start;

    return (end - start).count();
}
