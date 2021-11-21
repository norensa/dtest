#pragma once

#include <string>
#include <sstream>

std::string formatDuration(double nanos);
std::string formatDurationJSON(double nanos);

std::string formatSize(size_t size);

std::string indent(const std::string &str, int spaces);

std::string jsonify(const std::string &str);
std::string jsonify(size_t count, char const * const *str, int indent = 0);

template <typename Container>
std::string jsonify(const Container &str, int indent = 0) {
    std::stringstream s;

    std::string in = "";
    in.resize(indent, ' ');

    size_t count = str.size();

    s << '[';
    if (count > 0) s << '\n' << in;
    else s << ' ';
    auto it = str.begin();
    for (size_t i = 0; it != str.end(); ++it, ++i) {
        s << "  \"";
        for (auto c : *it) {
            switch (c) {
            case '\n':
                s << "\",\n  \"";
                break;
            case '"':
                s << '\\';
            default:
                s << c;
            }
        }
        if (i < count - 1) s << "\",\n" << in;
        else s << "\"\n" << in;
    }
    s << ']';

    return s.str();
}

