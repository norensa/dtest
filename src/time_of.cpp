#include <dtest_core/time_of.h>

#include <chrono>

uint64_t dtest::timeOf(const std::function<void()> &func) {
    auto start = std::chrono::high_resolution_clock::now();

    if (func) func();

    auto end = std::chrono::high_resolution_clock::now();
    if (! func) end = start;

    return (end - start).count();
}
