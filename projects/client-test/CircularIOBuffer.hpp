#ifndef _CIRCULAR_IO_BUFFER_H_
#define _CIRCULAR_IO_BUFFER_H_

#include <string.h>

namespace jw {

    template <int _N> class CircularIOBuffer {

    protected:
        char _buf[_N];
        int _head;
        int _tail;

    public:
        static const int capacity = _N - 1;

        CircularIOBuffer<_N>() : _head(0), _tail(0) {
            static_assert(_N > 1, "_N must greater than 1");
        }

        inline bool isFull() const {
            return (_tail + 1 == _head || _tail + 1 - _N == _head);
        }

        inline bool isEmpty() const {
            return (_head == _tail);
        }

        inline int size() {
            if (_head <= _tail) {
                return _tail - _head;
            } else {
                return _N - _head + _tail;
            }
        }

        inline int freeSize() {
            if (_head <= _tail) {
                return _N - 1 - _tail + _head;
            } else {
                return _head - _tail - 1;
            }
        }

        int read(char *data, int len) {
            if (isEmpty()) {
                return 0;
            }

            if (_tail > _head) {
                int s = _tail - _head;
                if (s >= len) {
                    memcpy(data, _buf + _head, len);
                    _head += len;
                    return len;
                } else {
                    memcpy(data, _buf + _head, s);
                    _head += s;
                    return s;
                }
            } else {
                int s1 = _N - _head;
                if (s1 >= len) {
                    memcpy(data, _buf + _head, len);
                    _head += len;
                    if (_head == _N) {
                        _head = 0;
                    }
                    return len;
                } else {
                    memcpy(data, _buf + _head, s1);
                    _head += s1;
                    return s1 + read(data + s1, len - s1);
                }
            }
        }

        int write(const char *data, int len) {
            if (isFull()) {
                return 0;
            }

            if (_tail >= _head) {
                int s = _N - _tail - 1;
                if (s >= len) {
                    memcpy(_buf + _tail, data, len);
                    _tail += len;
                    return len;
                } else if (s > 0) {
                    memcpy(_buf + _tail, data, s);
                    _tail += s;
                    return s + write(data + s, len - s);
                } else {
                    _buf[_tail] = *data;
                    _tail = 0;
                    return 1 + write(data + 1, len - 1);
                }
            } else {
                int s = _head - _tail - 1;
                if (s >= len) {
                    s = len;
                }
                memcpy(_buf + _tail, data, s);
                _tail += s;
                return s;
            }
        }

        template <size_t _C> int write(const char (&data)[_C]) {
            return write(data, _C);
        }

        template <size_t _C> int read(char (&data)[_C]) {
            return read(data, _C);
        }
    };
}

#endif
