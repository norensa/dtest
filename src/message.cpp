#include <message.h>

using namespace dtest;

void Message::send(Socket &socket) {
    size_t len = _buf - (uint8_t *) _allocBuf;
    *((size_t *) _allocBuf) = len;
    socket.send(_allocBuf, len);
}

Message & Message::recv(Socket &socket) {
    size_t len;
    size_t r = socket.recv(&len, sizeof(size_t), true);
    if (r == -1u || (r != 0 && r != sizeof(size_t))) throw std::runtime_error("Failed to receive");
    if (r != 0) return *this;
    len -= sizeof(size_t);

    _fit(len);
    r = socket.recv(_buf, len);
    if (r == -1u) throw std::runtime_error("Failed to receive");

    _hasData = true;

    return *this;
}
