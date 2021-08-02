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

    void *_allocBuf;
    size_t _allocLen;
    uint8_t *_buf;
    bool _hasData = false;

    void _fit(size_t sz) {
        size_t len = _buf - (uint8_t *) _allocBuf;
        size_t rem = _allocLen - len;
        if (rem < sz) {
            _allocLen += (sz < _DEFAULT_BUFFER_SIZE) ? _DEFAULT_BUFFER_SIZE : sz;
            _allocBuf = realloc(_allocBuf, _allocLen);
            _buf = ((uint8_t *) _allocBuf) + len;
        }
    }

public:

    inline Message(size_t len = _DEFAULT_BUFFER_SIZE) {
        _allocBuf = malloc(len);
        _allocLen = len;
        _buf = (uint8_t *) _allocBuf + sizeof(size_t);
    }

    Message(const Message &) = delete;

    Message(Message &&) = delete;

    inline ~Message() {
        free(_allocBuf);
    }

    Message & operator=(const Message &) = delete;

    Message & operator=(Message &&) = delete;

    void send(Socket &socket);

    Message & recv(Socket &socket);

    inline bool hasData() const {
        return _hasData;
    }

    inline Message & put(const void *data, size_t len) {
        _fit(len);
        memcpy(_buf, data, len);
        _buf += len;
        return *this;
    }

    template <typename T>
    inline Message & operator<<(const T &x) {
        _fit(sizeof(T));
        *((T *) _buf) = x;
        _buf += sizeof(T);
        return *this;
    }

    inline void * get(void *data, size_t len) {
        memcpy(data, _buf, len);
        _buf += len;
        return data;
    }

    template <typename T>
    inline Message & operator>>(T &x) {
        x = *((T *) _buf);
        _buf += sizeof(T);
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
        return x;
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
