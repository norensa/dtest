#include <dtest.h>

unit("root-test")
.body([] {
});

unit("unit-test", "pass")
.body([] {
    assert(true);
});

unit("unit-test", "fail")
.body([] {
    assert(false);
})
.expect(Status::FAIL);

unit("unit-test", "timeout")
.body([] {
})
.timeoutNanos(0)
.expect(Status::TIMEOUT);

unit("unit-test", "mem-leak")
.body([] {
    malloc(1);
})
.expect(Status::PASS_WITH_MEMORY_LEAK);

unit("unit-test", "invalid-free")
.body([] {
    free((void *) 0xdead);
})
.expect(Status::FAIL);
