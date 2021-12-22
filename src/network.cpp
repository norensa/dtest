#include <dtest_core/network.h>
#include <dtest_core/sandbox.h>
#include <dtest_core/random.h>

using namespace dtest;

thread_local size_t Network::_locked = false;

namespace dtest {
    Network *_netmgr_instance = nullptr;
}

Network::Network()
: _sendSize(0),
  _sendCount(0),
  _recvSize(0),
  _recvCount(0)
{
    _netmgr_instance = this;
}

bool Network::canSend(int fd) {
    static thread_local auto _holeEndTime = std::chrono::high_resolution_clock::now();

    int type;
    socklen_t optlen = sizeof(type);
    if (
        getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &optlen) == -1
        || type == SOCK_STREAM || type == SOCK_SEQPACKET || type == SOCK_RDM
    ) return true;

    if (! _probabilistic || ! _enter()) return true;

    auto now = std::chrono::high_resolution_clock::now();
    bool ok = now > _holeEndTime;
    if (ok) {
        ok = frand() < _chance;
        if (! ok) {
            _holeEndTime = now + std::chrono::nanoseconds(
                (uint64_t) (_holeDuration * frand())
            );
        }
    }

    _exit();
    return ok;
}

void Network::trackSend(size_t size) {
    if (! _enter()) return;

    _sendSize += size;
    ++_sendCount;

    _exit();
}

void Network::trackRecv(size_t size) {
    if (! _enter()) return;

    _recvSize += size;
    ++_recvCount;

    _exit();
}
