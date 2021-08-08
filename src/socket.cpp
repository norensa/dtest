#include <socket.h>

#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <unistd.h>
#include <poll.h>
#include <unordered_map>

using namespace dtest;

#undef socket
#undef connect
#undef bind
#undef listen
#undef setsockopt
#undef close
#undef inet_ntop
#undef inet_pton
#undef htons
#undef htonl
#undef ntohs
#undef send
#undef sendto
#undef recv
#undef recvfrom
#undef shutdown
#undef close
#undef accept
#undef getsockname
#undef poll
namespace sys {
    using ::socket;
    using ::connect;
    using ::bind;
    using ::listen;
    using ::setsockopt;
    using ::close;
    using ::inet_ntop;
    using ::inet_pton;
    using ::htons;
    using ::htonl;
    using ::ntohs;
    using ::send;
    using ::sendto;
    using ::recv;
    using ::recvfrom;
    using ::shutdown;
    using ::close;
    using ::accept;
    using ::getsockname;
    using ::poll;
}

size_t Socket::_MTU = _MAX_MTU;

void Socket::_resizeMTU(size_t oldMTU) {

    if (oldMTU > _MTU) return;

    size_t newMTU = _MTU;
    if (
        (newMTU <= _MAX_MTU && newMTU > 8000)
        || (newMTU <= 512)
    ) {
        --newMTU;
    }
    else {
        newMTU = 512;
    }
    _MTU = newMTU;
}

Socket::Socket(int fd)
: _fd(fd)
{
    if (_fd == -1) return;

    socklen_t len = sizeof(_addr);
    if (sys::getsockname(_fd, &_addr, &len) == -1) {
        throw std::runtime_error(
            std::string("Error getting socket address. ") + strerror(errno)
        );
    }
}

Socket::Socket(const sockaddr &addr) {
    _fd = sys::socket(AF_INET, SOCK_STREAM, 0);
    _addr = addr;

    if (_fd == -1) {
        throw std::runtime_error(
            std::string("Failed to create socket. ") + strerror(errno)
        );
    }

    if (sys::connect(_fd, &addr, sizeof(sockaddr)) == -1) {
        sys::close(_fd);

        throw std::runtime_error(
            std::string("Failed to connect. ") + strerror(errno)
        );
    }
}

Socket::Socket(uint16_t port, int maxWaitingQueueLength) {

    _fd = sys::socket(AF_INET, SOCK_STREAM, 0);

    if (_fd == -1) {
        throw std::runtime_error(
            std::string("Failed to create socket. ") + strerror(errno)
        );
    }

    int opt = 1;
    if (sys::setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) { 
        sys::close(_fd);
        throw std::runtime_error(
            std::string("Failed to set socket options. ") + strerror(errno)
        );
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = sys::htonl(INADDR_ANY);
    addr.sin_port = sys::htons(port);

    if (sys::bind(_fd, (sockaddr *) &addr, sizeof(addr)) == -1) {
        sys::close(_fd);
        throw std::runtime_error(
            "Failed to bind socket to port " + std::to_string(port) +
            ". " + strerror(errno)
        );
    }

    if (sys::listen(_fd, maxWaitingQueueLength) == -1) {
        throw std::runtime_error(
            "Error attempting to listen to port " + std::to_string(port) +
            ". " + strerror(errno)
        );
    }

    socklen_t len = sizeof(_addr);
    if (sys::getsockname(_fd, &_addr, &len) == -1) {
        throw std::runtime_error(
            std::string("Error getting socket address. ") + strerror(errno)
        );
    }
}

void Socket::send(void *data, size_t len) {

    size_t maxLen = _INITIAL_SYSCALL_SIZE;

    while (len > 0) {
        ssize_t sent;
        
        if (len <= maxLen) {
            sent = sys::send(_fd, data, len, 0);
        }
        else {
            sent = sys::send(_fd, data, maxLen, MSG_MORE);
        }

        if (sent != -1) {
            len -= sent;
            data = (uint8_t *) data + sent;
        }
        else {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            else if (errno == EMSGSIZE) {
                _resizeMTU(maxLen);
                maxLen = _MTU;
            }
            else {
                throw std::runtime_error(
                    std::string("Failed to send. ") + strerror(errno)
                );
            }
        }
    }
}

size_t Socket::recv(void *data, size_t len, bool returnOnBlock) {
    size_t maxLen = _INITIAL_SYSCALL_SIZE;

    while (len > 0) {
        ssize_t recvd = sys::recv(_fd, data, len < maxLen ? len : maxLen, 0);
        if (recvd == 0) {
            return -1lu;
        }
        else if (recvd != -1) {
            len -= recvd;
            data = (uint8_t *) data + recvd;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            if (returnOnBlock) return len;
            else continue;
        }
        else if (errno != EINTR) {
            throw std::runtime_error(
                std::string("Failed to receive. ") + strerror(errno)
            );
        }
    }

    return len;
}

void Socket::close() {
    if (_fd != -1) {
        sys::close(_fd);
        _fd = -1;
    }

    for (auto &conn : _openConnections) {
        conn.second->close();
        delete conn.second;
    }
    _openConnections.clear();
}

Socket Socket::accept() {

    sockaddr addr;
    socklen_t len = sizeof(addr);
    int incoming = sys::accept(_fd, &addr, &len);

    return { incoming, addr };
}

Socket * Socket::pollOrAcceptOrTimeout() {

    std::vector<pollfd> pollSocks;

    int incoming = -1;

    pollSocks.push_back({ _fd, POLLIN, 0 });
    for (const auto &s : _openConnections) {
        if (s.second->_fd != -1) {
            pollSocks.push_back(pollfd{ s.second->_fd, POLLIN, 0 });
        }
    }

    int count = sys::poll(pollSocks.data(), pollSocks.size(), 10);

    for (const auto &p : pollSocks) {
        if (count == 0) break;

        if (p.revents == 0) continue;

        --count;

        if (p.fd == _fd) {
            sockaddr addr;
            socklen_t len = sizeof(addr);

            if (
                (p.revents & POLLIN) &&
                (incoming = accept4(_fd, &addr, &len, SOCK_NONBLOCK)) != -1
            ) {
                _openConnections[incoming] = new Socket(incoming, addr);
            }
        }
        else if (
            (p.revents & POLLRDHUP)
            || (p.revents & POLLERR)
            || (p.revents & POLLHUP)
            || (p.revents & POLLNVAL)
        ) {
            auto c = _openConnections[p.fd];
            c->close();
            delete c;
            _openConnections.erase(p.fd);
        }
        else if ((p.revents & POLLIN)) {
            incoming = p.fd;
        }
    }

    return (incoming == -1) ? nullptr : _openConnections[incoming];
}

Socket & Socket::pollOrAccept() {
    Socket *ptr = nullptr;
    while (ptr == nullptr) ptr = pollOrAcceptOrTimeout();
    return *ptr;
}

void Socket::dispose(Socket &sock) {
    int fd = sock._fd;
    auto c = _openConnections[fd];
    c->close();
    delete c;
    _openConnections.erase(fd);
}

sockaddr Socket::self_address_ipv4(uint16_t port) {

    ifaddrs *ifaddr;

    if (getifaddrs(&ifaddr)) {
        throw std::runtime_error(
            std::string("Unable to get network interface info. ") + strerror(errno)
        );
    }

    sockaddr self;
    bool found = false;
    for (auto ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (
            (ifa->ifa_flags & IFF_LOOPBACK) == 0 &&       // not loopback
            ifa->ifa_addr->sa_family == AF_INET           // and ipv4
        ) {
            self = *(ifa->ifa_addr);
            found = true;
            break;
        }
    }

    freeifaddrs(ifaddr);

    if (found) {
        ((sockaddr_in *) &self)->sin_port = htons(port);
        return self;
    }
    else {
        throw std::runtime_error("Failed to find own interface");
    }
}

std::string Socket::ipv4_to_str(const sockaddr &addr) {
    char ipstr[INET_ADDRSTRLEN];

    return 
        sys::inet_ntop(AF_INET, &(((sockaddr_in *) &addr)->sin_addr), ipstr, sizeof(addr))
        + std::string(":") + std::to_string(sys::ntohs(((sockaddr_in *) &addr)->sin_port));
}

sockaddr Socket::str_to_ipv4(const std::string &ip, uint16_t port) {
    sockaddr addr;
    sockaddr_in *addr_in = (sockaddr_in *) &addr;

    addr_in->sin_family = AF_INET;
    addr_in->sin_port = htons(port);
    if (sys::inet_pton(AF_INET, ip.c_str(), &addr_in->sin_addr) == -1) {
        throw std::invalid_argument("Error parsing IP address '" + ip + "'. " + strerror(errno));
    }

    return addr;
}

sockaddr Socket::str_to_ipv4(const std::string &str) {

    char ip[INET_ADDRSTRLEN + 6];
    strncpy(ip, str.c_str(), INET_ADDRSTRLEN + 6);
    char *colon = std::strchr(ip, ':');

    if (colon == NULL) {
        throw std::invalid_argument(
            "Error parsing IP address '" + std::string(ip) + "'. "
        );
    }

    *colon = '\0';
    uint16_t port = std::atoi(colon + 1);

    return str_to_ipv4(ip, port);
}
