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
.expect(Status::FAIL)
.driver([] {
    assert(false)
})
.worker([] {
    assert(false)
});

dunit("distributed-unit-test", "driver-fail")
.dependsOn("unit-test")
.expect(Status::FAIL)
.driver([] {
    assert(false)
})
.worker([] {
    assert(true)
});

dunit("distributed-unit-test", "worker-fail")
.dependsOn("unit-test")
.expect(Status::FAIL)
.driver([] {
    assert(true)
})
.worker([] {
    assert(false)
});

dunit("distributed-unit-test", "mem-leak")
.dependsOn("unit-test")
.expect(Status::PASS_WITH_MEMORY_LEAK)
.driver([] {
    malloc(1);
    assert(true);
})
.worker([] {
    malloc(1);
    assert(true);
});

dunit("distributed-unit-test", "driver-mem-leak")
.dependsOn("unit-test")
.expect(Status::PASS_WITH_MEMORY_LEAK)
.driver([] {
    malloc(1);
    assert(true);
})
.worker([] {
    assert(true);
});

dunit("distributed-unit-test", "worker-mem-leak")
.dependsOn("unit-test")
.expect(Status::PASS_WITH_MEMORY_LEAK)
.driver([] {
    assert(true);
})
.worker([] {
    malloc(1);
    assert(true);
});

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

dunit("distributed-unit-test", "user-message")
.dependsOn("unit-test")
.workers(4)
.driver([] {
    int x;
    x = 1;
    sendMsg(x);
    for (auto i = 0; i < 4; ++i) {
        recvMsg() >> x;
        assert (x == 2);
    }
})
.worker([] {
    int x;
    recvMsg() >> x;
    assert (x == 1);
    x = 2;
    sendMsg(x);
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
    return addr;
}

dunit("distributed-unit-test", "tcp")
.dependsOn("unit-test")
.workers(1)
.driver([] {
    auto fd = tcp_server_sock();
    auto addr = get_sock_addr(fd);
    sendMsg(addr);

    int conn = accept(fd, NULL, NULL);
    int x;
    recv(conn, &x, sizeof(x), 0);
    close(conn);
    close(fd);

    assert(x == 1234);
})
.worker([] {
    sockaddr addr;
    recvMsg() >> addr;

    auto fd = tcp_connect(addr);
    int x = 1234;
    send(fd, &x, sizeof(x), 0);
    close(fd);
});

dunit("distributed-unit-test", "tcp-faulty")
.dependsOn("unit-test")
.workers(1)
.faultyNetwork(0)
.driver([] {
    auto fd = tcp_server_sock();
    auto addr = get_sock_addr(fd);
    sendMsg(addr);

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
    recvMsg() >> addr;

    auto fd = tcp_connect(addr);
    for (int i = 0; i < 1000; ++i) {
        send(fd, &i, sizeof(i), 0);
    }
    close(fd);
});

dunit("distributed-unit-test", "udp")
.dependsOn("unit-test")
.workers(1)
.driver([] {
    auto fd = udp_server_sock();
    auto addr = get_sock_addr(fd);
    sendMsg(addr);

    int x;
    recvfrom(fd, &x, sizeof(x), 0, NULL, NULL);
    close(fd);

    assert(x == 1234);
})
.worker([] {
    sockaddr addr;
    recvMsg() >> addr;

    auto fd = udp_sock();
    int x = 1234;
    sendto(fd, &x, sizeof(x), 0, &addr, sizeof(addr));
    close(fd);
});

dunit("distributed-unit-test", "udp-faulty")
.dependsOn("unit-test")
.workers(1)
.faultyNetwork(0.3, 0)
.driver([] {
    auto fd = udp_server_sock();
    auto addr = get_sock_addr(fd);
    sendMsg(addr);

    int x;
    for (int i = 0; i < 1000; ++i) {
        auto recvd = recvfrom(fd, &x, sizeof(x), 0, NULL, NULL);
        if (recvd == -1 || x != i) {
            close(fd);
            return;
        }
    }
    close(fd);
    assert(false);      // shouldn't get here
})
.worker([] {
    sockaddr addr;
    recvMsg() >> addr;

    auto fd = udp_sock();
    for (int i = 0; i < 1000; ++i) {
        sendto(fd, &i, sizeof(i), 0, &addr, sizeof(addr));
    }
    close(fd);
});
