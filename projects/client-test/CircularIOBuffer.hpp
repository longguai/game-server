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
                return _N + _head - _tail - 1;
            } else {
                return _head - _tail - 1;
            }
        }

        int read(char *data, int len) {
            if (isEmpty()) {
                return 0;
            }

            if (_tail > _head) {  // _tail在_head后面
                int s = _tail - _head;  // 有效字节数
                if (s >= len) {  // 大于接收缓冲区长度，则填满接收缓冲区为止
                    memcpy(data, _buf + _head, len);
                    _head += len;
                    return len;
                } else {  // 小于接收缓冲区长度，则全部读走
                    memcpy(data, _buf + _head, s);
                    _head += s;
                    return s;
                }
            } else {  // _tail在_head前面
                int s1 = _N - _head;  // 尾部有效字节数
                if (s1 >= len) {  // 大于接收缓冲区长度，则填满接收缓冲区为止
                    memcpy(data, _buf + _head, len);
                    _head += len;
                    if (_head == _N) {
                        _head = 0;
                    }
                    return len;
                } else {
                    if (s1 > 0) {  // 小于接收缓冲区长度，则先全部读走
                        memcpy(data, _buf + _head, s1);
                        _head += s1;
                        return s1 + read(data + s1, len - s1);  // 再拼从接环形区头部读的数据
                    } else {  // 尾部有效字节数为0，从接环形区头部读
                        _head = 0;
                        return read(data, len);
                    }
                }
            }
        }

        int write(const char *data, int len) {
            if (isFull()) {
                return 0;
            }

            if (_tail >= _head) {  // _tail在_head后面
                int s = _N - _tail - 1;  // 尾部空闲的字节数
                if (s >= len) {  // 足够存放
                    memcpy(_buf + _tail, data, len);
                    _tail += len;
                    return len;
                } else {
                    if (s > 0) {
                        memcpy(_buf + _tail, data, s);  // 写满尾部空闲的字节数
                        _tail += s;
                        return s + write(data + s, len - s);  // 再往头部写
                    } else {
                        _buf[_tail] = *data;  // 写一个字节到尾部
                        _tail = 0;
                        return 1 + write(data + 1, len - 1);  // 再往头部写
                    }
                }
            } else {  // _tail在_head前面
                // 写到不覆盖到_head即可
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
