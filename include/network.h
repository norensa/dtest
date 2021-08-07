#pragma once

#include <mutex>
#include <cstdlib>

namespace dtest {

class Network {

    friend class Sandbox;

private:
    std::mutex _mtx;

    bool _track = false;

    size_t _sendSize = 0;
    size_t _sendCount = 0;
    size_t _recvSize = 0;
    size_t _recvCount = 0;

    bool _probabilistic = false;
    double _chance = 1;
    uint64_t _holeDuration = 0;

    inline bool _enter() {
        if (! _track) return false;
        _mtx.lock();
        return true;
    }

    inline void _exit() {
        _mtx.unlock();
    }

public:

    Network();

    inline void trackActivity(bool val) {
        _track = val;
    }

    inline void dropSendRequests(double chance, uint64_t duration) {
        _chance = chance;
        _holeDuration = duration;
        _probabilistic = true;
    }

    inline void dontDropSendRequests() {
        _probabilistic = false;
    }

    bool canSend(int fd);

    void trackSend(size_t size);

    void trackRecv(size_t size);
};

}  // end namespace dtest
