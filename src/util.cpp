#include <util.h>
#include <sstream>

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