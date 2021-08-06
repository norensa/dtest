#pragma once

#include <stdint.h>
#include <cstring>
#include <cstdlib>
#include <socket.h>
#include <sandbox.h>

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
        sandbox().exit();

        _allocBuf = malloc(len);
        _allocLen = len;
        _buf = (uint8_t *) _allocBuf + sizeof(size_t);

        sandbox().enter();
    }

    inline Message(const Message &rhs) {
        sandbox().exit();

        _copy(rhs);

        sandbox().enter();
    }

    inline Message(Message &&rhs) {
        sandbox().exit();

        _move(rhs);
        rhs._invalidate();

        sandbox().enter();
    }

    inline ~Message() {
        sandbox().exit();

        _dispose();
        _invalidate();

        sandbox().enter();
    }

    inline Message & operator=(const Message &rhs) {
        sandbox().exit();

        if (this != &rhs) {
            _dispose();
            _copy(rhs);
        }

        sandbox().enter();
        return *this;
    }

    inline Message & operator=(Message &&rhs) {
        sandbox().exit();

        if (this != &rhs) {
            _dispose();
            _move(rhs);
            rhs._invalidate();
        }

        sandbox().enter();
        return *this;
    }

    void send(Socket &socket);

    Message & recv(Socket &socket);

    inline bool hasData() const {
        return _hasData;
    }

    inline Message & put(const void *data, size_t len) {
        sandbox().exit();

        _fit(len);
        memcpy(_buf, data, len);
        _buf += len;

        sandbox().enter();
        return *this;
    }

    template <typename T>
    inline Message & operator<<(const T &x) {
        sandbox().exit();

        _fit(sizeof(T));
        *((T *) _buf) = x;
        _buf += sizeof(T);

        sandbox().enter();
        return *this;
    }

    inline void * get(void *data, size_t len) {
        sandbox().exit();

        memcpy(data, _buf, len);
        _buf += len;

        sandbox().enter();
        return data;
    }

    template <typename T>
    inline Message & operator>>(T &x) {
        sandbox().exit();

        x = *((T *) _buf);
        _buf += sizeof(T);

        sandbox().enter();
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
