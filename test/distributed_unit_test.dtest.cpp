#include <dtest.h>
#include <thread>
#include <dtest_core/socket.h>

module("distributed-unit-test")
.dependsOn({
    "unit-test"
});

dunit("distributed-unit-test", "pass")
.driver([] {
    assert(true)
})
.worker([] {
    assert(true)
});

dunit("distributed-unit-test", "fail")
.expect(Status::FAIL)
.driver([] {
    fail("test_failed");
})
.worker([] {
    fail("test_failed");
});

dunit("distributed-unit-test", "driver-fail")
.expect(Status::FAIL)
.driver([] {
    fail("test_failed");
})
.worker([] {
    assert(true)
});

dunit("distributed-unit-test", "worker-fail")
.expect(Status::FAIL)
.driver([] {
    assert(true)
})
.worker([] {
    fail("test_failed");
});

dunit("distributed-unit-test", "mem-leak")
.expect(Status::PASS_WITH_MEMORY_LEAK)
.driver([] {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-result"
    malloc(1);
    #pragma GCC diagnostic pop
    assert(true);
})
.worker([] {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-result"
    malloc(1);
    #pragma GCC diagnostic pop
    assert(true);
});

dunit("distributed-unit-test", "driver-mem-leak")
.expect(Status::PASS_WITH_MEMORY_LEAK)
.driver([] {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-result"
    malloc(1);
    #pragma GCC diagnostic pop
    assert(true);
})
.worker([] {
    assert(true);
});

dunit("distributed-unit-test", "worker-mem-leak")
.expect(Status::PASS_WITH_MEMORY_LEAK)
.driver([] {
    assert(true);
})
.worker([] {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-result"
    malloc(1);
    #pragma GCC diagnostic pop
    assert(true);
});

dunit("unit-test", "worker-id")
.workers(4)
.driver([] {
    uint32_t id;
    for (auto i = 0; i < 4; ++i) {
        dtest_recv_msg(id);
        std::cout << id << '\n';
    }

    assert(dtest_worker_id() == 0);
    assert(dtest_num_workers() == 4);
})
.worker([] {
    uint32_t id = dtest_worker_id();
    dtest_send_msg(id);
    assert(id <= 4);
    assert(dtest_num_workers() == 4);
});

dunit("unit-test", "is-driver")
.workers(1)
.driver([] {
    assert(dtest_is_driver());
})
.worker([] {
    assert(! dtest_is_driver());
});

dunit("distributed-unit-test", "wait-notify")
.workers(4)
.driver([] {
    dtest_notify();
    dtest_wait(4);
})
.worker([] {
    dtest_wait();
    dtest_notify();
});

dunit("distributed-unit-test", "user-message")
.workers(4)
.driver([] {
    int x;
    x = 1;
    dtest_send_msg(x);
    for (auto i = 0; i < 4; ++i) {
        dtest_recv_msg(x);
        assert (x == 2);
    }
})
.worker([] {
    int x;
    dtest_recv_msg(x);
    assert (x == 1);
    x = 2;
    dtest_send_msg(x);
});

static int tcp_server_sock() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd != -1);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0);
    assert(bind(fd, (sockaddr *) &addr, sizeof(addr)) != -1);

    assert(listen(fd, 128) != -1);

    return fd;
}
static int tcp_connect(const sockaddr &addr) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd != -1);

    assert(connect(fd, &addr, sizeof(sockaddr)) != -1);

    return fd;
}
static int udp_server_sock() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(fd != -1);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0);
    assert(bind(fd, (sockaddr *) &addr, sizeof(addr)) != -1);

    return fd;
}
static int udp_sock() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(fd != -1);

    return fd;
}
static sockaddr get_sock_addr(int fd) {
    sockaddr addr;
    socklen_t len = sizeof(addr);
    assert(getsockname(fd, &addr, &len) != -1);
    return dtest::Socket::self_address_ipv4(dtest::Socket::get_port(addr));
}

dunit("distributed-unit-test", "tcp")
.workers(1)
.driver([] {
    auto fd = tcp_server_sock();
    auto addr = get_sock_addr(fd);
    dtest_send_msg(addr);

    int conn = accept(fd, NULL, NULL);
    int x;
    recv(conn, &x, sizeof(x), 0);
    close(conn);
    close(fd);

    assert(x == 1234);
})
.worker([] {
    sockaddr addr;
    dtest_recv_msg(addr);

    auto fd = tcp_connect(addr);
    int x = 1234;
    send(fd, &x, sizeof(x), 0);
    close(fd);
});

dunit("distributed-unit-test", "tcp-faulty")
.workers(1)
.faultyNetwork(0)
.driver([] {
    auto fd = tcp_server_sock();
    auto addr = get_sock_addr(fd);
    dtest_send_msg(addr);

    int conn = accept(fd, NULL, NULL);
    int x;
    for (int i = 0; i < 1000; ++i) {
        recv(conn, &x, sizeof(x), 0);
        assert(x == i);
    }
    close(conn);
    close(fd);
})
.worker([] {
    sockaddr addr;
    dtest_recv_msg(addr);

    auto fd = tcp_connect(addr);
    for (int i = 0; i < 1000; ++i) {
        send(fd, &i, sizeof(i), 0);
    }
    close(fd);
});

dunit("distributed-unit-test", "udp")
.workers(1)
.driver([] {
    auto fd = udp_server_sock();
    auto addr = get_sock_addr(fd);
    dtest_send_msg(addr);

    int x;
    recvfrom(fd, &x, sizeof(x), 0, NULL, NULL);
    close(fd);

    assert(x == 1234);
})
.worker([] {
    sockaddr addr;
    dtest_recv_msg(addr);

    auto fd = udp_sock();
    int x = 1234;
    sendto(fd, &x, sizeof(x), 0, &addr, sizeof(addr));
    close(fd);
});

dunit("distributed-unit-test", "udp-faulty")
.workers(1)
.faultyNetwork(0.3, 0)
.driver([] {
    auto fd = udp_server_sock();
    auto addr = get_sock_addr(fd);
    dtest_send_msg(addr);

    int x;
    for (int i = 0; i < 1000; ++i) {
        auto recvd = recvfrom(fd, &x, sizeof(x), 0, NULL, NULL);
        if (recvd == -1 || x != i) {
            close(fd);
            return;
        }
    }
    close(fd);
    fail("test_failed");;      // shouldn't get here
})
.worker([] {
    sockaddr addr;
    dtest_recv_msg(addr);

    auto fd = udp_sock();
    for (int i = 0; i < 1000; ++i) {
        sendto(fd, &i, sizeof(i), 0, &addr, sizeof(addr));
    }
    close(fd);
});

dunit("distributed-unit-test", "runaway-allocations-during-message")
.workers(1)
.driver([] {
    volatile bool run = true;
    auto t = std::thread([&run] {
        while (run) {
            void *ptr = malloc(1);
            free(ptr);
        }
    });

    int x = 5;
    for (auto i = 0; i < 1000; ++i) {
        dtest_send_msg(x);
    }

    run = false;
    t.join();
})
.worker([] {
    volatile bool run = true;
    auto t = std::thread([&run] {
        while (run) {
            void *ptr = malloc(1);
            free(ptr);
        }
    });

    int x;
    for (auto i = 0; i < 1000; ++i) {
        dtest_recv_msg(x);
    }

    run = false;
    t.join();
});
