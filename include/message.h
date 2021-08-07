#pragma once

#include <stdint.h>
#include <cstring>
#include <cstdlib>
#include <socket.h>

#include <string>
#include <vector>

namespace dtest {

class Message {
private:

    static const size_t _DEFAULT_BUFFER_SIZE = 1024;

    void *_allocBuf = nullptr;
    size_t _allocLen = 0;
    uint8_t *_buf = nullptr;
    bool _hasData = false;

    void _enter();

    void _exit();

    inline void _fit(size_t sz) {
        size_t len = _buf - (uint8_t *) _allocBuf;
        size_t rem = _allocLen - len;
        if (rem < sz) {
            _allocLen += (sz < _DEFAULT_BUFFER_SIZE) ? _DEFAULT_BUFFER_SIZE : sz;
            _allocBuf = realloc(_allocBuf, _allocLen);
            _buf = (uint8_t *) _allocBuf + len;
        }
    }

    inline void _dispose() {
        if (_allocBuf != nullptr) free(_allocBuf);
    }

    inline void _invalidate() {
        _allocBuf = nullptr;
        _allocLen = 0;
        _buf = nullptr;
        _hasData = false;
    }

    inline void _move(Message &rhs) {
        _allocBuf = rhs._allocBuf;
        _allocLen = rhs._allocLen;
        _buf = rhs._buf;
        _hasData = rhs._hasData;
    }

    inline void _copy(const Message &rhs) {
        _allocBuf = malloc(rhs._allocLen);
        memcpy(_allocBuf, rhs._allocBuf, rhs._allocLen);
        _allocLen = rhs._allocLen;
        _buf = (uint8_t *) _allocBuf + (rhs._buf - (uint8_t *) rhs._allocBuf);
        _hasData = rhs._hasData;
    }

public:

    inline Message(size_t len = _DEFAULT_BUFFER_SIZE) {
        _enter();

        _allocBuf = malloc(len);
        _allocLen = len;
        _buf = (uint8_t *) _allocBuf + sizeof(size_t);

        _exit();
    }

    inline Message(const Message &rhs) {
        _enter();

        _copy(rhs);

        _exit();
    }

    inline Message(Message &&rhs) {
        _enter();

        _move(rhs);
        rhs._invalidate();

        _exit();
    }

    inline ~Message() {
        _enter();

        _dispose();
        _invalidate();

        _exit();
    }

    inline Message & operator=(const Message &rhs) {
        _enter();

        if (this != &rhs) {
            _dispose();
            _copy(rhs);
        }

        _exit();
        return *this;
    }

    inline Message & operator=(Message &&rhs) {
        _enter();

        if (this != &rhs) {
            _dispose();
            _move(rhs);
            rhs._invalidate();
        }

        _exit();
        return *this;
    }

    void send(Socket &socket);

    Message & recv(Socket &socket);

    inline bool hasData() const {
        return _hasData;
    }

    inline Message & put(const void *data, size_t len) {
        _enter();

        _fit(len);
        memcpy(_buf, data, len);
        _buf += len;

        _exit();
        return *this;
    }

    template <typename T>
    inline Message & operator<<(const T &x) {
        _enter();

        _fit(sizeof(T));
        *((T *) _buf) = x;
        _buf += sizeof(T);

        _exit();
        return *this;
    }

    inline void * get(void *data, size_t len) {
        _enter();

        memcpy(data, _buf, len);
        _buf += len;

        _exit();
        return data;
    }

    template <typename T>
    inline Message & operator>>(T &x) {
        _enter();

        x = *((T *) _buf);
        _buf += sizeof(T);

        _exit();
        return *this;
    }

    // specialized overloads

    template <typename T>
    inline Message & operator<<(const std::vector<T> &x) {
        operator<<<size_t>(x.size());
        for (const auto & xx : x) operator<<<T>(xx);
        return *this;
    }

    template <typename T>
    inline Message & operator>>(std::vector<T> &x) {
        size_t sz; operator>><size_t>(sz);
        x = std::vector<T>(sz);
        for (size_t i = 0; i < sz; ++i) {
            operator>><T>(x[i]);
        }
        return *this;
    }
};

// template specializations

template <>
inline Message & Message::operator<<<std::string>(const std::string &x) {
    put(x.c_str(), x.size() + 1);
    return *this;
}

template <>
inline Message & Message::operator>><std::string>(std::string &x) {
    x = (const char *) _buf;
    _buf += x.size() + 1;
    return *this;
}

}  // end namespace dtest
