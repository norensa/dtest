#include <dtest.h>

perf("performance-test", "pass")
.dependsOn("unit-test")
.body([] {
    for (int i = 0; i < 1000000; ++i);
})
.baseline([] {
    for (int i = 0; i < 8000000; ++i);
});

perf("performance-test", "too-slow")
.dependsOn("unit-test")
.expect(Status::TOO_SLOW)
.body([] {
    for (int i = 0; i < 8000000; ++i);
})
.baseline([] {
    for (int i = 0; i < 1000000; ++i);
});
