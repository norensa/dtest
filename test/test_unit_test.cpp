#include <dtest.h>

unit(unittest, pass)
.body([] {
    assert(true);
});

unit(unittest, fail)
.body([] {
    assert(false);
})
.expectFailure();

unit(unittest, timeout)
.body([] {
    assert(true);
})
.timeoutNanos(0)
.expectTimeout();
