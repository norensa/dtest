# dtest

A C++ testing framework for performing unit tests, distributed tests, and
performance tests.

## Usage

1. Add **Makefile.template** in your project's test directory.
2. Write your test modules (must have extension **.dtest.cpp** and **#include <dtest.h>**).
3. Build and run tests.
4. Check the test results in the output file **dtest.log.json**.

## Syntax

### 1. Test Modules

Tests for a given project are divided into modules. Each module can contain one
or more tests. Modules can have dependencies (i.e. an entire test module cannot
run unless its module dependencies are run without errors).

    module("module-name")
    .dependsOn({
        "module-a",
        "module-b"
    });

### 2. Test Status

Each test run has a status. The status describes the general category of the
result of the test. The output file **dtest.log.json** will have more
detailed information on every test.

| Status                         | Description |
| ------------------------------ | ----------- |
| Status:: PASS                  | Test passed with no errors detected. |
| Status:: PASS_WITH_MEMORY_LEAK | Test passed but has a memory leak. |
| Status:: MEMORY_LIMIT_EXCEEDED | Test passed but a set memory limit was exceeded. |
| Status:: TOO_SLOW              | Test did not meet performance requirements. |
| Status:: TIMEOUT               | Test duration exceeded the specified timeout value. |
| Status:: FAIL                  | Test has failed (assertion error, uncaught exception, segfault, etc.).

### 3. Unit Tests

Unit tests provide an easy and fast way to verify that your components behave as
expected. You can write unit tests as follows:

    unit("module-name", "test-name")
    .option()
    .body([] {
        // test code here
        assert(1 == 1);
    });

The macro **assert()** can accept any C++ expression that can be evaluated as a
boolean. If the expression evaluates to false, an assertion error is generated.

In addition to assertions, unit tests check for memory leaks, timeouts,
segfaults, etc. The framework runs the tests inside a sandbox that measures cpu
time, memory allocations, and network activity.

Each test can have any number of options set to control its behavior. The
available options are as follows:

| Option             | Description |
| ------------------ | ----------- |
| .body              | The main body of the test is defined using this option. This accepts either a (void)->void lambda or a function of the same signature. |
| .onInit            | Adds a code block to run before the beginning of the main test body. This accepts either a (void)->void lambda or a function of the same signature. |
| .onComplete        | Adds a code block to run after the end of the main test body. This accepts either a (void)->void lambda or a function of the same signature. |
| .timeout           | Specifies a timeout duration in seconds. If the test takes longer than this duration, it is terminated and considered to fail. (default = 10 seconds)
| .timeoutMillis     | Specifies a timeout duration in milliseconds. If the test takes longer than this duration, it is terminated and considered to fail. (default = 10 seconds)
| .timeoutMicros     | Specifies a timeout duration in microseconds. If the test takes longer than this duration, it is terminated and considered to fail. (default = 10 seconds)
| .timeoutNanos      | Specifies a timeout duration in nanoseconds. If the test takes longer than this duration, it is terminated and considered to fail. (default = 10 seconds)
| .memoryBytesLimit  | Sets a limit on the maximum amount of memory (in bytes) allocated. |
| .memoryBlocksLimit | Sets a limit on the maximum number of memory blocks allocated. |
| .expect            | Sets the expected test status. If the test status is different from the expected, it is considered as a failed test. (default = Status::PASS)
| .disable           | Disables the test. |
| .enable            | Enables the test. |
| .dependsOn         | Adds extra dependencies for this test (in addition to the module-wide dependencies). |
| .ignoreMemoryLeak  | Does not perform a memory leak check at the end of the test. |
| .inProcess         | Runs the test in a local sandbox for debugging. The default behavior is to run the test in a separate process to ensure the best possible isolation between tests. |
| .input             | Sets an input string to be fed to the test through stdin. |

### 4. Distributed Unit Tests

Distributed unit tests run tests to validate components that typically depend on
the interaction between multiple processes or machines. Instead of a single test
body, the test is divided into two blocks of code: driver and worker. A single
instance of the driver test code is spawn, and there can be one or more
instances of the worker test code running.

    dunit("module-name", "test-name")
    .option()
    .driver([] {
        // test code here
        assert(1 == 1);
    })
    .worker([] {
        // test code here
        assert(1 == 1);
    });

In addition to the options available for unit tests, distributed unit tests have
the following options:

| Option                           | Description |
| -------------------------------- | ----------- |
| .driver                          | The driver code is defined using this option. This accepts either a (void)->void lambda or a function of the same signature. |
| .worker                          | The worker code is defined using this option. This accepts either a (void)->void lambda or a function of the same signature. |
| .workers                         | Sets the number of worker instances. (default = 4) |
| .faultyNetwork(chance, duration) | Simulates a faulty network by introducing holes during which send operations are ignored. |


### 5. Performance Tests

Performance tests provide a means to measure the runtime of a piece of code in
comparison to some baseline. The test requires that the runtime of the main body
is less than the baseline. An optional margin can be set to specify the minimum
reduction in runtime allowed (as an absolute value or ratio) to consider this
test successful.

    perf("module-name", "test-name")
    .option()
    .body([] {
        // test code here
    })
    .baseline([] {
        // test code here
    });

In addition to the options available for unit tests, performance tests have the
following options:

| Option                            | Description |
| --------------------------------- | ----------- |
| .baseline                         | Provides the body of some baseline to be used in comparison to the code block in .body(). This accepts either a (void)->void lambda or a function of the same signature. |
| .performanceMargin                | Specifies the absolute time difference in seconds required between the main body and baseline to consider this test as an improvement over the baseline. (default = 1ms) |
| .performanceMarginMillis          | Specifies the absolute time difference in milliseconds required between the main body and baseline to consider this test as an improvement over the baseline. (default = 1ms) |
| .performanceMarginMicros          | Specifies the absolute time difference in microseconds required between the main body and baseline to consider this test as an improvement over the baseline. (default = 1ms) |
| .performanceMarginNanos           | Specifies the absolute time difference in nanoseconds required between the main body and baseline to consider this test as an improvement over the baseline. (default = 1ms) |
| .performanceMarginAsBaselineRatio | Sets the required ratio of body/baseline runtime. If set, this ignores the absolute performance margin. |

### 6. Utilities

The framework provides a number of utilities that help facilitate a number of
frequently used operations. These utilities are provided as macros and functions
that can be used inside the test body.

| Function/Macro      | Description |
| ------------------- | ----------- |
| dtest_random()      | Returns a random number between 0.0 and 1.0 from a uniform distribution. This can be substantially faster than standard C/C++ methods in a multithreaded context. |
| dtest_num_workers() | Returns the set number of workers for a distributed unit test. |
| dtest_worker_id()   | Returns the unique id of the worker. The driver is always guaranteed to have id = 0. |
| dtest_is_driver()   | Returns true for the driver, false for any worker. |
| dtest_notify()      | Sends a notification to the driver or workers. |
| dtest_wait([n])     | Waits for a notification from the driver or worker(s). An optional parameter n can be set to specify the number of notify messages required. The default value is 1 on workers. On the driver, the default is the number of workers set for the test. |
| dtest_send_msg(msg) | Sends a message to the driver/worker. The parameter msg can be any series of variables separated by "<<" (e.g. var1 << var2 << ...) |
| dtest_recv_msg(msg) | Receives a message from the driver/worker. The parameter msg can be any series of variables separated by ">>" (e.g. var1 >> var2 >> ...) |
