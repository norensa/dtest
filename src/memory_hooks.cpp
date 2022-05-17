/*
 * Copyright (c) 2021 Noah Orensa.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
*/

#include <dtest_core/memory.h>
#include <dtest_core/sandbox.h>
#include <malloc.h>
#include <sys/mman.h>
#include <stdarg.h>

using namespace dtest;

namespace dtest {
    extern Memory *_mmgr_instance;
}

// malloc & friends

static char _calloc_tmp[2048];      // for dlsym, while we find calloc

void * malloc(size_t __size) {
    void *ptr;

    ptr = libc().malloc(__size);
    if (ptr && _mmgr_instance) _mmgr_instance->track(ptr, __size);
    return ptr;
}

void * calloc(size_t __nmemb, size_t __size) {
    void *ptr;

    if (libc().calloc == nullptr) return _calloc_tmp;

    ptr = libc().calloc(__nmemb, __size);
    if (ptr && _mmgr_instance) _mmgr_instance->track(ptr, __nmemb * __size);
    return ptr;
}

void * memalign(size_t __alignment, size_t __size) {
    void *ptr;

    ptr = libc().memalign(__alignment, __size);
    if (ptr && _mmgr_instance) _mmgr_instance->track(ptr, __size);
    return ptr;
}

int posix_memalign(void **__memptr, size_t __alignment, size_t __size) {
    int retval;

    retval = libc().posix_memalign(__memptr, __alignment, __size);
    if (*__memptr && _mmgr_instance) _mmgr_instance->track(*__memptr, __size);
    return retval;
}

void * valloc(size_t __size) {
    void *ptr;

    ptr = libc().valloc(__size);
    if (ptr && _mmgr_instance) _mmgr_instance->track(ptr, __size);
    return ptr;
}

void * pvalloc(size_t __size) {
    void *ptr;

    ptr = libc().pvalloc(__size);
    if (ptr && _mmgr_instance) _mmgr_instance->track(ptr, __size);
    return ptr;
}

void * aligned_alloc(size_t __alignment, size_t __size) {
    void *ptr;

    ptr = libc().aligned_alloc(__alignment, __size);
    if (ptr && _mmgr_instance) _mmgr_instance->track(ptr, __size);
    return ptr;
}

void * realloc(void *__ptr, size_t __size) {
    void *ptr;

    ptr = libc().realloc(__ptr, __size);
    if (__ptr) {
        if (ptr && _mmgr_instance) _mmgr_instance->retrack(__ptr, ptr, __size);
    }
    else {
        if (ptr && _mmgr_instance) _mmgr_instance->track(ptr, __size);
    }
    return ptr;
}

void * reallocarray(void *__ptr, size_t __nmemb, size_t __size) throw() {
    void *ptr;

    ptr = libc().reallocarray(__ptr, __nmemb, __size);
    if (__ptr) {
        if (ptr && _mmgr_instance) _mmgr_instance->retrack(__ptr, ptr, __nmemb * __size);
    }
    else {
        if (ptr && _mmgr_instance) _mmgr_instance->track(ptr, __nmemb * __size);
    }
    return ptr;
}

void free(void *__ptr) {

    if (__ptr == _calloc_tmp) return;

    if (__ptr && _mmgr_instance) _mmgr_instance->remove(__ptr);
    libc().free(__ptr);
}

// mmap & friends

void * mmap(void *__addr, size_t __len, int __prot, int __flags, int __fd, __off_t __offset) {
    void *ptr;

    ptr = libc().mmap(__addr, __len, __prot, __flags, __fd, __offset);
    if (ptr != MAP_FAILED && _mmgr_instance) {
        _mmgr_instance->track_mapped((char *) ptr, __len);
    }

    return ptr;
}

void * mremap(void *__addr, size_t __old_len, size_t __new_len, int __flags, ...) {
    void *ptr;
    va_list args;
    va_start(args, __flags);

    if ((__flags & MREMAP_FIXED) != 0) {
        ptr = libc().mremap(__addr, __old_len, __new_len, __flags, va_arg(args, void *));
    }
    else {
        ptr = libc().mremap(__addr, __old_len, __new_len, __flags);
    }

    va_end(args);

    if (ptr != MAP_FAILED && _mmgr_instance) {
        _mmgr_instance->retrack_mapped((char *) __addr, __old_len, (char *) ptr, __new_len);
    }

    return ptr;
}

int munmap(void *__addr, size_t __len) {
    if (__addr && _mmgr_instance) _mmgr_instance->remove_mapped((char *) __addr, __len);
    return libc().munmap(__addr, __len);
}

// operator new overrides /////////////////////////////////////////////////////

void * operator new(size_t count) {
    return malloc(count);
}

void * operator new[](size_t count) {
    return operator new(count);
}

#if (__cplusplus >= 201703L)
void* operator new(std::size_t count, std::align_val_t al) {
    return memalign((size_t) al, count);
}
#endif

#if (__cplusplus >= 201703L)
void* operator new[](std::size_t count, std::align_val_t al) {
    return operator new(count, al);
}
#endif

void* operator new(std::size_t count, const std::nothrow_t &) noexcept {
    return operator new(count);
}

void* operator new[](std::size_t count, const std::nothrow_t &) noexcept {
    return operator new(count);
}

#if (__cplusplus >= 201703L)
void* operator new(std::size_t count, std::align_val_t al, const std::nothrow_t &) {
    return operator new(count, al);
}
#endif

#if (__cplusplus >= 201703L)
void* operator new[](std::size_t count, std::align_val_t al, const std::nothrow_t &) {
    return operator new(count, al);
}
#endif

// operator delete overrides //////////////////////////////////////////////////

void operator delete(void *ptr) noexcept {
    free(ptr);
}

void operator delete[](void *ptr) noexcept {
    operator delete(ptr);
}

#if (__cplusplus >= 201703L)
void operator delete(void *ptr, std::align_val_t al) noexcept {
    operator delete(ptr);
}
#endif

#if (__cplusplus >= 201703L)
void operator delete[](void *ptr, std::align_val_t al) noexcept {
    operator delete(ptr);
}
#endif

void operator delete(void *ptr, size_t size) noexcept {
    operator delete(ptr);
}

void operator delete[](void *ptr, size_t size) noexcept {
    operator delete(ptr);
}

#if (__cplusplus >= 201703L)
void operator delete(void *ptr, std::size_t sz, std::align_val_t al) noexcept {
    operator delete(ptr);
}
#endif

#if (__cplusplus >= 201703L)
void operator delete[](void *ptr, std::size_t sz, std::align_val_t al) noexcept {
    operator delete(ptr);
}
#endif

void operator delete(void *ptr, const std::nothrow_t &) noexcept {
    operator delete(ptr);
}

void operator delete[](void *ptr, const std::nothrow_t &) noexcept {
    operator delete(ptr);
}

#if (__cplusplus >= 201703L)
void operator delete(void *ptr, std::align_val_t al, const std::nothrow_t &) noexcept {
    operator delete(ptr);
}
#endif

#if (__cplusplus >= 201703L)
void operator delete[](void *ptr, std::align_val_t al, const std::nothrow_t &) noexcept {
    operator delete(ptr);
}
#endif
