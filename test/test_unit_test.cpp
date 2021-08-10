#include <dtest.h>

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
    malloc(1);
});

unit("unit-test", "invalid-free")
.expect(Status::FAIL)
.body([] {
    free((void *) 0xdead);
});

unit("unit-test", "error-message")
.body([] {
    err("error");
});

unit("unit-test", "random")
.body([] {
    double sum = 0;
    for (auto i = 0; i < 100; ++i) {
        sum += random();
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
    for (auto i = 0; i < 500; ++i) malloc(1);
});
