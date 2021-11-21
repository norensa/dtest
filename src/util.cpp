#include <dtest_core/util.h>

std::string formatDuration(double nanos) {
    std::stringstream s;
    s.setf(std::ios::fixed);
    s.precision(3);
    
    if (nanos < 1e3) s << nanos << " ns";
    else if (nanos < 1e6) s << nanos / 1e3 << " us";
    else if (nanos < 1e9) s << nanos / 1e6 << " ms";
    else s << nanos /1e9 << " s";

    return s.str();
}

std::string formatDurationJSON(double nanos) {
    std::stringstream s;
    s.setf(std::ios::fixed);
    s.precision(3);
    
    if (nanos < 1e3) s << "{ \"value\": " << nanos << ", \"unit\": \"ns\" }";
    else if (nanos < 1e6) s << "{ \"value\": " << nanos / 1e3 << ", \"unit\": \"us\" }";
    else if (nanos < 1e9) s << "{ \"value\": " << nanos / 1e6 << ", \"unit\": \"ms\" }";
    else s << "{ \"value\": " << nanos / 1e9 << ", \"unit\": \"s\" }";

    return s.str();
}

std::string formatSize(size_t size) {
    static const size_t KB = 1024;
    static const size_t MB = 1024 * KB;
    static const size_t GB = 1024 * MB;
    static const size_t TB = 1024 * GB;

    std::stringstream s;
    s.setf(std::ios::fixed);
    s.precision(2);
    
    if (size < KB) s << size << " B";
    else if (size < MB) s << (double) size / KB << " KB";
    else if (size < GB) s << (double) size / MB << " MB";
    else if (size < TB) s << (double) size / GB << " GB";
    else s << (double) size / TB << " TB";

    return s.str();    
}

std::string indent(const std::string &str, int spaces) {
    std::string in = "";
    in.resize(spaces, ' ');

    std::stringstream s;
    s << in;

    const char *p = str.c_str();
    while (*p != '\0') {
        s << *p;

        if (*p == '\n' && *(p + 1) != '\0') {
            s << in;
        }

        ++p;
    }

    return s.str();
}

std::string jsonify(const std::string &str) {
    std::stringstream s;

    if (str.find('\n') == std::string::npos) {
        s << "\"";
        for (auto c : str) {
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
        s << "\"";
    }
    else {
        s << "[\n  \"";
        const char *p = str.c_str();
        while (*p != '\0') {
            switch (*p) {
            case '\n':
                s << "\",\n  \"";
                break;
            case '"':
                s << '\\';
            default:
                s << *p;
            }
            ++p;
        }
        s << "\"\n]";

    }
    return s.str();
}

std::string jsonify(size_t count, char const * const *str, int indent) {
    std::stringstream s;

    std::string in = "";
    in.resize(indent, ' ');

    s << '[';
    if (count > 0) s << '\n' << in;
    else s << ' ';
    for (size_t i = 0; i < count; ++i) {
        s << "  \"";
        const char *p = str[i];
        while (*p != '\0') {
            switch (*p) {
            case '\n':
                s << "\",\n  \"";
                break;
            case '"':
                s << '\\';
            default:
                s << *p;
            }
            ++p;
        }
        if (i < count - 1) s << "\",\n" << in;
        else s << "\"\n" << in;
    }
    s << ']';

    return s.str();
}
