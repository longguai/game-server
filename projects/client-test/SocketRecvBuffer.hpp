#ifndef _CW_SOCKET_RECV_BUFFER_H_
#define _CW_SOCKET_RECV_BUFFER_H_

#include "CircularIOBuffer.hpp"

namespace jw {

    template <int _N> class SocketRecvBuffer final : protected CircularIOBuffer<_N> {

    protected:
        // xcode needs these
        using CircularIOBuffer<_N>::_buf;
        using CircularIOBuffer<_N>::_head;
        using CircularIOBuffer<_N>::_tail;

    public:
        using CircularIOBuffer<_N>::capacity;
        using CircularIOBuffer<_N>::isFull;
        using CircularIOBuffer<_N>::isEmpty;
        using CircularIOBuffer<_N>::size;
        using CircularIOBuffer<_N>::freeSize;
        using CircularIOBuffer<_N>::read;

        template <class _RECV> int doRecv(_RECV &&func) {
            if (isFull()) {
                return 0;
            }

            if (_tail >= _head) {
                if (_head == 0) {
                    int s = _N - _tail - 1;
                    int ret = func(_buf + _tail, s);
                    if (ret > 0) {
                        _tail += ret;
                    }
                    return ret;
                } else {
                    int s = _N - _tail;
                    int ret = func(_buf + _tail, s);
                    if (ret > 0) {
                        _tail += ret;
                        if (_tail >= _N) {
                            _tail -= _N;
                        }
                    }
                    return ret;
                }
            } else {
                int s = _head - _tail - 1;
                int ret = func(_buf + _tail, s);
                if (ret > 0) {
                    _tail += ret;
                }
                return ret;
            }
        }

        bool skip(int len) {
            assert(len != 0);

            if (_head <= _tail) {
                if (_tail - _head < len) {
                    return false;
                }
                _head += len;
                return true;
            } else {
                int s = _N - _head;
                if (s >= len) {
                    _head += len;
                    if (_head == _N) {
                        _head = 0;
                    }
                    return true;
                } else {
                    if (s + _tail >= len) {
                        _head = len - s;
                        return true;
                    }
                    return false;
                }
            }
        }

        bool peek(char *buf, int len, bool skip) {
            assert(len != 0);

            if (_head <= _tail) {
                if (_tail - _head < len) {
                    return false;
                }
                memcpy(buf, _buf + _head, len);
                if (skip) {
                    _head += len;
                }
                return true;
            } else {
                int s = _N - _head;
                if (s >= len) {
                    memcpy(buf, _buf + _head, len);
                    if (skip) {
                        _head += len;
                        if (_head == _N) {
                            _head = 0;
                        }
                    }
                    return true;
                } else {
                    if (s + _tail >= len) {
                        memcpy(buf, _buf + _head, s);
                        memcpy(buf + s, _buf, len - s);
                        if (skip) {
                            _head = len - s;
                        }
                        return true;
                    }
                    return false;
                }
            }
        }
    };

}

#endif
