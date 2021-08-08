#include <call_stack.h>
#include <sandbox.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>    // for __cxa_demangle
#include <sstream>
#include <cstring>

using namespace dtest;

void CallStack::_dispose() {
    if (_stack != nullptr) libc().free(_stack);
}

void CallStack::_copy(const CallStack &rhs) {
    _len = rhs._len;
    _stack = (void **) libc().malloc(_MAX_STACK_FRAMES * sizeof(void *));
    memcpy(_stack, rhs._stack, _MAX_STACK_FRAMES * sizeof(void *));
    _skip = rhs._skip;
}

std::string CallStack::toString() const noexcept {
    std::stringstream s;
    char buf[1024];

    char **symbols = backtrace_symbols(_stack, _len);

    for (int i = _skip; i < _len; i++) {
        Dl_info info;
        if (dladdr(_stack[i], &info)) {
            char *demangled = NULL;
            int status;
            demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
            snprintf(
                buf, sizeof(buf), "%-3d  %p  %s + %p%s",
                _len - i - 1, _stack[i],
                status == 0 ? demangled : info.dli_sname,
                (void *) ((char *)_stack[i] - (char *)info.dli_saddr),
                (i == _len - 1) ? "" : "\n"
            );
            free(demangled);
        }
        else {
            snprintf(
                buf, sizeof(buf), "%-3d  %p  %s%s",
                _len - i - 1, _stack[i],
                symbols[i],
                (i == _len - 1) ? "" : "\n"
            );
        }
        s << buf;
    }
    free(symbols);
    if (_len == CallStack::_MAX_STACK_FRAMES + _skip) s << "\n[truncated]";

    return s.str();
}

CallStack CallStack::trace(int skip) {
    ++skip;
    void **stack = (void **) libc().malloc((_MAX_STACK_FRAMES + skip) * sizeof(void *));
    int nFrames = backtrace(stack, _MAX_STACK_FRAMES + skip);

    return { nFrames, stack, skip };
}
