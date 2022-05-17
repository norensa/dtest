/*
 * Copyright (c) 2021 Noah Orensa.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
*/

#pragma once

#include <string>

namespace dtest {

class CallStack {
private:
    static const int _MAX_STACK_FRAMES = 32;

    int _len = 0;
    void **_stack = nullptr;
    int _skip = 0;

    void _dispose();

    inline void _invalidate() {
        _len = 0;
        _stack = nullptr;
        _skip = 0;
    }

    inline void _move(CallStack &rhs) {
        _len = rhs._len;
        _stack = rhs._stack;
        _skip = rhs._skip;
    }

    void _copy(const CallStack &rhs);

    inline CallStack(int len, void **stack, int skip)
    : _len(len),
      _stack(stack),
      _skip(skip)
    { }

public:

    inline CallStack(const CallStack &rhs) {
        _copy(rhs);
    }

    inline CallStack(CallStack &&rhs) {
        _move(rhs);
        rhs._invalidate();
    }

    static CallStack trace(int skip = 0);

    inline ~CallStack() {
        _dispose();
        _invalidate();
    }

    inline CallStack & operator=(const CallStack &rhs) {
        if (this != &rhs) {
            _dispose();
            _copy(rhs);
        }
        return *this;
    }

    inline CallStack & operator=(CallStack &&rhs) {
        if (this != &rhs) {
            _dispose();
            _move(rhs);
            rhs._invalidate();
        }
        return *this;
    }

    std::string toString() const noexcept;

    void * const * stack() const {
        return _stack + _skip;
    }
};

}  // end namespace dtest;