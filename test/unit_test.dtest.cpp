/*
 * Copyright (c) 2021 Noah Orensa.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
*/

#include <dtest.h>
#include <iostream>
#include <thread>
#include <sys/mman.h>
#include <unistd.h>

unit("root-test")
.body([] {
});

unit("unit-test", "pass")
.body([] {
    assert(true);
});

unit("unit-test", "fail")
.expect(Status::FAIL)
.body([] {
    fail("test failed");
});

unit("unit-test", "timeout")
.timeoutNanos(0)
.expect(Status::TIMEOUT)
.body([] {
});

unit("unit-test", "malloc-mem-leak")
.expect(Status::PASS_WITH_MEMORY_LEAK)
.body([] {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-result"
    malloc(1);
    #pragma GCC diagnostic pop
});

unit("unit-test", "invalid-free")
.expect(Status::FAIL)
.body([] {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfree-nonheap-object"
    free((void *) 0xdead);
    #pragma GCC diagnostic pop
});

unit("unit-test", "mmap")
.body([] {
    size_t sz = getpagesize();

    void *ptr = mmap(nullptr, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(ptr != MAP_FAILED);
    assert(munmap(ptr, sz) == 0);
});

unit("unit-test", "mmap-munmap-multiple")
.body([] {
    size_t sz = getpagesize();

    void *ptr = mmap(nullptr, 2 * sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(ptr != MAP_FAILED);
    assert(munmap(ptr, sz) == 0);
    assert(munmap((char *) ptr + sz, sz) == 0);
});

unit("unit-test", "mmap-mem-leak")
.expect(Status::PASS_WITH_MEMORY_LEAK)
.body([] {
    size_t sz = getpagesize();

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-result"
    mmap(nullptr, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    #pragma GCC diagnostic pop
});

unit("unit-test", "invalid-munmap")
.expect(Status::FAIL)
.body([] {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfree-nonheap-object"
    munmap((void *) 0xdead, 1);
    #pragma GCC diagnostic pop
});

unit("unit-test", "mmap-munmap-partial")
.expect(Status::PASS_WITH_MEMORY_LEAK)
.body([] {
    size_t sz = getpagesize();

    void *ptr = mmap(nullptr, 3 * sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(ptr != MAP_FAILED);
    assert(munmap((char *) ptr + sz, sz) == 0);
});

unit("unit-test", "mmap-mremap")
.expect(Status::PASS_WITH_MEMORY_LEAK)
.body([] {
    size_t sz = getpagesize();

    void *ptr = mmap(nullptr, 3 * sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(ptr != MAP_FAILED);
    mremap((char *) ptr + sz, sz, 2 * sz, MREMAP_MAYMOVE);

    mremap(ptr, sz, sz * 2, MREMAP_MAYMOVE);
});

unit("unit-test", "error-message")
.body([] {
    err("error");
});

unit("unit-test", "random")
.body([] {
    double sum = 0;
    for (auto i = 0; i < 100; ++i) {
        sum += dtest_random();
    }
    assert(sum < 100);
});

unit("unit-test", "seg-fault")
.expect(Status::FAIL)
.body([] {
    int *ptr = (int *) 0xdead;
    *ptr = 0;
});

unit("unit-test", "seg-fault-again")
.expect(Status::FAIL)
.body([] {
    int *ptr = (int *) 0xdead;
    *ptr = 0;
});

unit("unit-test", "large-report")
.expect(Status::PASS_WITH_MEMORY_LEAK)
.body([] {
    for (auto i = 0; i < 50; ++i) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wunused-result"
        malloc(1);
        #pragma GCC diagnostic pop
    }
});

unit("unit-test", "fail-before-dynamic-free")
.expect(Status::FAIL)
.body([] {
    struct b {
        void *buf = malloc(1);
        ~b() {
            free(buf);
        }
    } b;

    fail("test failed");
});

unit("unit-test", "uncaught-exception")
.expect(Status::FAIL)
.body([] {
    throw std::string("error");
});

unit("unit-test", "local-test-pass")
.inProcess()
.body([] {
    assert(true);
});

unit("unit-test", "local-test-fail")
.inProcess()
.expect(Status::FAIL)
.body([] {
    fail("test failed");
});

unit("unit-test", "openmp")
.body([] {
    #pragma omp parallel
    {
    }
});

unit("unit-test", "openmp-memleak")
.expect(Status::PASS_WITH_MEMORY_LEAK)
.body([] {
    #pragma omp parallel
    {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wunused-result"
        malloc(1);
        #pragma GCC diagnostic pop
    }
});

unit("unit-test", "local-openmp")
.inProcess()
.body([] {
    #pragma omp parallel
    {
    }
});

unit("unit-test", "openmp-after-local-openmp")
.body([] {
    #pragma omp parallel
    {
    }
});

unit("unit-test", "parallel-alloc-dealloc")
.body([] {
    #pragma omp parallel for
    for (auto i = 0; i < 2000; ++i) {
        auto ptr = malloc(1);
        uint32_t j = dtest_random() * 10;
        while (j--);
        free(ptr);
    }
});

unit("unit-test", "false-tls-mem-leak")
.body([] {
    static thread_local int var;

    var = 5;
    assert(var == 5);
});

unit("unit-test", "sandboxed-stdio")
.input("x")
.body([] {
    char c;
    std::cin >> c;
    assert(c == 'x');
    std::cout << "This is a stdout test";
    std::cerr << "This is a stderr test";
});

unit("unit-test", "sandboxed-stdio-local")
.inProcess()
.input("x")
.body([] {
    char c;
    std::cin >> c;
    assert(c == 'x');
    std::cout << "This is a stdout test";
    std::cerr << "This is a stderr test";
});

unit("unit-test", "special-characters-in-report")
.body([] {
    std::cout << "This is a stdout test of \"special characters\"\nAnother line...";
    err("This is an error message with \"special characters\"");
    err("This is an error message with \"special characters\"\nAnother line...");
});

unit("unit-test", "runaway-allocations-during-error-log")
.body([] {
    volatile bool run = true;
    volatile bool running = false;
    auto t = std::thread([&run, &running] {
        while (run) {
            void *ptr = malloc(1);
            free(ptr);
            running = true;
        }
    });

    while (! running);

    for (auto i = 0; i < 100; ++i) {
        err("test error log");
    }

    run = false;
    t.join();
});

unit("unit-test", "memory-bytes-limit")
.memoryBytesLimit(1)
.body([] {
    auto p1 = malloc(1);
    free(p1);
    auto p2 = malloc(1);
    free(p2);
});

unit("unit-test", "memory-bytes-limit-fail")
.memoryBytesLimit(1)
.expect(Status::MEMORY_LIMIT_EXCEEDED)
.body([] {
    auto p1 = malloc(1);
    auto p2 = malloc(1);
    free(p1);
    free(p2);
});

unit("unit-test", "memory-blocks-limit")
.memoryBlocksLimit(1)
.body([] {
    auto p1 = malloc(1024);
    free(p1);
    auto p2 = malloc(1024);
    free(p2);
});

unit("unit-test", "memory-blocks-limit-fail")
.memoryBlocksLimit(1)
.expect(Status::MEMORY_LIMIT_EXCEEDED)
.body([] {
    auto p1 = malloc(1024);
    auto p2 = malloc(1024);
    free(p1);
    free(p2);
});
