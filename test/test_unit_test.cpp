#include <dtest.h>

unit("root-test")
.body([] {
    assert(true);
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
    assert(true);
})
.timeoutNanos(0)
.expect(Status::TIMEOUT);

unit("unit-test", "mem-leak")
.body([] {
    malloc(1);
    assert(true);
})
.expect(Status::PASS_WITH_MEMORY_LEAK);
