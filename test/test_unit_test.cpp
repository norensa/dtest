#include <dtest.h>

unit("unit-test", "pass")
.body([] {
    assert(true);
});

unit("unit-test", "fail")
.body([] {
    assert(false);
})
.expectFailure();

unit("unit-test", "timeout")
.body([] {
    assert(true);
})
.timeoutNanos(0)
.expectTimeout();
