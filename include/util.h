#pragma once

#include <string>

std::string formatDuration(double nanos);

std::string formatSize(size_t size);

std::string indent(const std::string &str, int spaces);
