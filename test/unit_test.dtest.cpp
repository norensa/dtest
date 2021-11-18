#include <dtest.h>
#include <iostream>

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
    assert(false);
});

unit("unit-test", "timeout")
.timeoutNanos(0)
.expect(Status::TIMEOUT)
.body([] {
});

unit("unit-test", "mem-leak")
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

    assert(false);
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
    assert(false);
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
