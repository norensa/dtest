#include <dtest.h>

module("performance-test")
.dependsOn({
    "unit-test"
});

perf("performance-test", "pass")
.body([] {
    for (int i = 0; i < 1000000; ++i);
})
.baseline([] {
    for (int i = 0; i < 8000000; ++i);
});

perf("performance-test", "too-slow")
.expect(Status::TOO_SLOW)
.body([] {
    for (int i = 0; i < 8000000; ++i);
})
.baseline([] {
    for (int i = 0; i < 1000000; ++i);
});
