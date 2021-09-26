#pragma once

#include <functional>

namespace dtest {

template <typename T>
class Lazy {
private:
    std::function<T *()> _constructor = {};
    std::function<void(T *)> _destructor = {};
    T *_ptr = nullptr;

    inline T * _lazyPtr() const {
        if (_ptr == nullptr) ((Lazy<T> *) this)->_ptr = _constructor();
        return _ptr;
    }

public:
    inline Lazy(T *object)
    : _ptr(object) { }

    inline Lazy(
        const std::function<T *()> &constructor,
        const std::function<void(T *)> &destructor = nullptr
    ): _constructor(constructor), 
       _destructor(destructor)
    { }

    inline Lazy(const Lazy &rhs)
    : _constructor(rhs._constructor), 
      _destructor(rhs.destructor)
    { }

    inline Lazy(Lazy &&rhs)
    : _constructor(rhs._constructor),
      _destructor(rhs._destructor),
      _ptr(rhs._ptr)
    {
        rhs._ptr = nullptr;
    }

    inline ~Lazy() {
        if (_ptr != nullptr) {
            if (_destructor) _destructor(_ptr);
            else delete _ptr;
        }
    }

    inline Lazy & operator=(const Lazy &rhs) {
        if (_ptr != nullptr) {
            if (_destructor) _destructor(_ptr);
            delete _ptr;
        }
        _constructor = rhs._constructor;
        _destructor = rhs._destructor;
        return *this;
    }

    inline Lazy & operator=(Lazy &&rhs) {
        if (_ptr != nullptr) {
            if (_destructor) _destructor(_ptr);
            delete _ptr;
        }
        _constructor = rhs._constructor;
        _destructor = rhs._destructor;
        _ptr = rhs._ptr; rhs._ptr = nullptr;
        return *this;
    }

    inline T * ptr() {
        return _lazyPtr();
    }

    inline const T * ptr() const {
        return _lazyPtr();
    }

    inline T * operator->() {
        return _lazyPtr();
    }

    inline const T * operator->() const {
        return _lazyPtr();
    }

    inline T & operator*() {
        return *_lazyPtr();
    }

    inline const T & operator*() const {
        return *_lazyPtr();
    }

    inline operator T & () {
        return *_lazyPtr();
    }

    inline operator const T & () const {
        return *_lazyPtr();
    }

    inline bool initialized() const {
        return _ptr != nullptr;
    }
};

}  // endnamespace dtest

#define lazy(type, name, initialValue, lazyValue) \
    type __ ## name ## __ ## lazy = initialValue; \
    inline type name() { \
        if (__ ## name ## __ ## lazy == initialValue) { \
            __ ## name ## __ ## lazy = lazyValue; \
        } \
        return __ ## name ## __ ## lazy; \
    }
