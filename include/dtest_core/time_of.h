/*
 * Copyright (c) 2021 Noah Orensa.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
*/

#pragma once

#include <functional>
#include <stdint.h>

namespace dtest
{
    uint64_t timeOf(const std::function<void()> &func);
} // namespace dtest
