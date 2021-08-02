#pragma once

#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace dtest {

class Socket {

private:

    static const size_t _INITIAL_SYSCALL_SIZE = 64 * 1024;
    static const size_t _MAX_MTU = 8192;

    static size_t _MTU;

    static void _resizeMTU(size_t oldMTU);

    int _fd;
    sockaddr _addr;
    std::unordered_map<int, Socket *> _openConnections;

    void _fillAddr();

    inline Socket(int fd, const sockaddr &addr)
    : _fd(fd), _addr(addr)
    { }

    Socket(int fd);

public:

    inline Socket()
    : Socket(-1)
    { }

    Socket(const sockaddr &addr);

    inline Socket(const std::string &ip, uint16_t port)
    : Socket(str_to_ipv4(ip, port))
    { }

    inline Socket(const std::string &addr)
    : Socket(str_to_ipv4(addr))
    { }

    Socket(uint16_t port, int maxWaitingQueueLength);

    inline Socket(const Socket &) = delete;

    inline Socket(Socket &&rhs)
    : _fd(rhs._fd),
      _addr(std::move(rhs._addr)),
      _openConnections(std::move(rhs._openConnections))
    {
        rhs._fd = -1;
        rhs._openConnections.clear();
    }

    ~Socket() {
        close();
    }

    inline Socket & operator=(const Socket &) = delete;

    inline Socket & operator=(Socket &&rhs) {
        close();

        _fd = rhs._fd; rhs._fd = -1;
        _addr = rhs._addr;
        _openConnections = std::move(rhs._openConnections); rhs._openConnections.clear();

        return *this;
    }

    inline void invalidate() {
        _fd = -1;
    }

    inline bool operator==(const Socket &rhs) const {
        return _fd == rhs._fd;
    }

    const sockaddr & address() const {
        return _addr;
    }

    void send(void *data, size_t len);

    size_t recv(void *data, size_t len, bool returnOnBlock = false);

    void close();

    Socket accept();

    Socket & pollOrAccept();

    void dispose(Socket &sock);

    static sockaddr self_address_ipv4(uint16_t port);

    static std::string ipv4_to_str(const sockaddr &addr);

    static sockaddr str_to_ipv4(const std::string &ip, uint16_t port);
    static sockaddr str_to_ipv4(const std::string &str);
};

struct sockaddr_in_hash {
    inline size_t operator()(const sockaddr &addr) const {
        auto a = (const sockaddr_in *) &addr;
        return ((size_t) a->sin_port) << 32 | (size_t) a->sin_addr.s_addr;
    }
};

struct sockaddr_in_equal_to {
    inline bool operator()(const sockaddr &addr1, const sockaddr &addr2) const {
        auto a = (const sockaddr_in *) &addr1;
        auto b = (const sockaddr_in *) &addr2;

        return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
    }
};

}  // end namesspace dtest
