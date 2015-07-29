#ifndef _CW_SOCKET_SEND_BUFFER_H_
#define _CW_SOCKET_SEND_BUFFER_H_

#include "CircularIOBuffer.hpp"

namespace jw {

    template <int _N> class SocketSendBuffer final : protected CircularIOBuffer<_N> {

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
        using CircularIOBuffer<_N>::write;

        template <class _SEND> int doSend(_SEND &&func) {
            if (_head == _tail) {
                return 0;
            }

            if (_head <= _tail) {
                int s = _tail - _head;
                int ret = func(_buf + _head, s);
                if (ret > 0) {
                    _head += ret;
                }
                return ret;
            } else {
                int s = _N - _head;
                int ret = func(_buf + _head, s);
                if (ret > 0) {
                    _head += ret;
                    if (_head >= _N) {
                        _head -= _N;
                    }
                }
                return ret;
            }
        }
    };
}

#endif
