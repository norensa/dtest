/*
 * Copyright (c) 2021 Noah Orensa.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
*/

#pragma once

#include <cstdlib>
#include <cstring>

namespace dtest {

class Buffer {
private:

    size_t _size = 0;
    void *_data = nullptr;

    void _invalidate() {
        _size = 0;
        _data = nullptr;
    }

    void _free() {
        if (_data != nullptr) free(_data);
    }

    void _copy(const Buffer &rhs) {
        _size = rhs._size;
        _data = malloc(_size);
        memcpy(_data, rhs._data, _size);
    }

    void _move(Buffer &rhs) {
        _size = rhs._size;
        _data = rhs._data;
    }

public:

    Buffer()
    :   _size(0),
        _data(nullptr)
    { }

    Buffer(size_t len)
    :   _size(len),
        _data(malloc(len))
    { }

    Buffer(void *data, size_t len)
    :   _size(len),
        _data(data)
    { }

    Buffer(const void *data, size_t len)
    :   _size(len),
        _data(malloc(len))
    {
        memcpy(_data, data, len);
    }

    Buffer(const Buffer &rhs) {
        _copy(rhs);
    }

    Buffer(Buffer &&rhs) {
        _move(rhs);
        rhs._invalidate();
    }

    ~Buffer() {
        _free();
    }

    Buffer & operator=(const Buffer &rhs) {
        if (this != &rhs) {
            _free();
            _copy(rhs);
        }
        return *this;
    }

    Buffer & operator=(Buffer &&rhs) {
        if (this != &rhs) {
            _free();
            _move(rhs);
            rhs._invalidate();
        }
        return *this;
    }

    size_t size() const {
        return _size;
    }

    void * data() {
        return _data;
    }

    const void * data() const {
        return _data;
    }

    operator void * () {
        return _data;
    }

    operator const void * () const {
        return _data;
    }

    bool resize(size_t size) {
        void *ptr = realloc(_data, size);
        if (size == 0 || ptr != nullptr) {
            _size = size;
            _data = ptr;
            return true;
        }
        else {
            return false;
        }
    }
};

}
