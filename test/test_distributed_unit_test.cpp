#include <dtest.h>

dunit("distributed-unit-test", "pass")
.dependsOn("unit-test")
.driver([] {
    assert(true)
})
.worker([] {
    assert(true)
});

dunit("distributed-unit-test", "fail")
.dependsOn("unit-test")
.driver([] {
    assert(false)
})
.worker([] {
    assert(false)
})
.expect(Status::FAIL);

dunit("distributed-unit-test", "driver-fail")
.dependsOn("unit-test")
.driver([] {
    assert(false)
})
.worker([] {
    assert(true)
})
.expect(Status::FAIL);

dunit("distributed-unit-test", "worker-fail")
.dependsOn("unit-test")
.driver([] {
    assert(true)
})
.worker([] {
    assert(false)
})
.expect(Status::FAIL);

dunit("distributed-unit-test", "mem-leak")
.dependsOn("unit-test")
.driver([] {
    malloc(1);
    assert(true);
})
.worker([] {
    malloc(1);
    assert(true);
})
.expect(Status::PASS_WITH_MEMORY_LEAK);

dunit("distributed-unit-test", "driver-mem-leak")
.dependsOn("unit-test")
.driver([] {
    malloc(1);
    assert(true);
})
.worker([] {
    assert(true);
})
.expect(Status::PASS_WITH_MEMORY_LEAK);

dunit("distributed-unit-test", "worker-mem-leak")
.dependsOn("unit-test")
.driver([] {
    assert(true);
})
.worker([] {
    malloc(1);
    assert(true);
})
.expect(Status::PASS_WITH_MEMORY_LEAK);

dunit("distributed-unit-test", "wait-notify")
.dependsOn("unit-test")
.workers(4)
.driver([] {
    notify();
    wait(4);
})
.worker([] {
    wait();
    notify();
});
