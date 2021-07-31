#include <dtest.h>

dunit("distributed-unit-test", "pass")
.dependsOn("unit-test")
.driver([] {
    wait(4);
})
.worker([] {
    notify();
})
.workers(4);
