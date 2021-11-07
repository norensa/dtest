#pragma once

#include <string>
#include <vector>

std::string formatDuration(double nanos);
std::string formatDurationJSON(double nanos);

std::string formatSize(size_t size);

std::string indent(const std::string &str, int spaces);

std::string jsonify(const std::string &str);
std::string jsonify(size_t count, char const * const *str, int indent = 0);
std::string jsonify(const std::vector<std::string> &str, int indent = 0);
