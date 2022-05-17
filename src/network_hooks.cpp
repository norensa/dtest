/*
 * Copyright (c) 2021 Noah Orensa.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
*/

#include <dtest_core/network.h>
#include <dtest_core/sandbox.h>

using namespace dtest;

namespace dtest {
    extern Network *_netmgr_instance;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    ssize_t res;

    if (_netmgr_instance->canSend(sockfd)) {
        res = libc().send(sockfd, buf, len, flags);
    }
    else {
        res = len;
    }

    _netmgr_instance->trackSend((res == -1) ? 0 : res);

    return res;
}

ssize_t sendto(
    int sockfd,
    const void *buf,
    size_t len,
    int flags,
    const struct sockaddr *dest_addr,
    socklen_t addrlen
) {
    ssize_t res;

    if (_netmgr_instance->canSend(sockfd)) {
        res = libc().sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    }
    else {
        res = len;
    }

    _netmgr_instance->trackSend((res == -1) ? 0 : res);

    return res;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    ssize_t res = libc().recv(sockfd, buf, len, flags);

    _netmgr_instance->trackRecv((res == -1) ? 0 : res);

    return res;
}

ssize_t recvfrom(
    int sockfd,
    void * __restrict buf,
    size_t len,
    int flags,
    struct sockaddr * __restrict src_addr,
    socklen_t * __restrict addrlen
) {
    ssize_t res = libc().recvfrom(sockfd, buf, len, flags, src_addr, addrlen);

    _netmgr_instance->trackRecv((res == -1) ? 0 : res);

    return res;
}
