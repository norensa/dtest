#include <message.h>

using namespace dtest;

void Message::send(Socket &socket) {
    size_t len = _buf - (uint8_t *) _allocBuf;
    *((size_t *) _allocBuf) = len;
    socket.send(_allocBuf, len);
}

Message & Message::recv(Socket &socket) {
    size_t len;
    socket.recv(&len, sizeof(len));
    len -= sizeof(size_t);
    _fit(len);
    socket.recv(_buf, len);
    return *this;
}
