#pragma once

#include <functional>
#include <stdint.h>

namespace dtest
{
    uint64_t timeOf(const std::function<void()> &func);
} // namespace dtest
