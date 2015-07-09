/*
  Copyright (c) 2009 Dave Gamble
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
 
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
 
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef cppJSON__h
#define cppJSON__h

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4996)
#endif

#include <stdint.h>  // for int64_t
#include <string>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>  // for PRId64
#include <string.h>  // for strncmp
#include <stdio.h>
#include <float.h>  // for DBL_EPSILON

#include <stdlib.h>
#include <stdexcept>
#include <type_traits>

#include <sstream>
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

#include <iterator>
#include <algorithm>

namespace jw {

    template <class _TRAITS, class _ALLOC>
    class BasicJSON;

    namespace __cpp_basic_json_impl {

        template <class _TRAITS, class _ALLOC, class _T>
        struct AssignImpl {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _T SourceType;
            static void invoke(JsonType &c, SourceType arg);
        };

        template <class _TRAITS, class _ALLOC, class _INTEGER>
        struct AssignFromIntegerImpl;

        template <class _TRAITS, class _ALLOC, class _FLOAT>
        struct AssignFromFloatImpl;

		template <class _TRAITS, class _ALLOC, class _STRING>
		struct AssignFromStringImpl;

		template <class _TRAITS, class _ALLOC, class _ARRAY>
        struct AssignFromArrayImpl;

        template <class _TRAITS, class _ALLOC, class _MAP>
        struct AssignFromMapImpl;

        template <class _TRAITS, class _ALLOC, class _T>
        struct AsImpl {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _T TargetType;
            static TargetType invoke(const JsonType &c);
        };

        template <class _TRAITS, class _ALLOC, class _INTEGER>
        struct AsIntegerImpl;

        template <class _TRAITS, class _ALLOC, class _FLOAT>
        struct AsFloatImpl;

        template <class _TRAITS, class _ALLOC, class _STRING>
        struct AsStringImpl;

        template <class _TRAITS, class _ALLOC, class _ARRAY>
        struct AsArrayImpl;

        template <class _TRAITS, class _ALLOC, class _MAP>
        struct AsMapImpl;
    }

    template <class _TRAITS, class _ALLOC>
    class BasicJSON {
    public:
        friend class iterator;

        enum class ValueType {
            Null, False, True, Integer, Float, String, Array, Object
        };

        typedef BasicJSON<_TRAITS, _ALLOC> ThisType;

        // 开始作死 →_→
        typedef std::basic_string<char, _TRAITS, typename _ALLOC::template rebind<char>::other> StringType;

    private:
        ValueType _type;
        int64_t _valueInt64;
        double _valueDouble;
        StringType _valueString;

        StringType _key;
        ThisType *_child;
        ThisType *_next;
        ThisType *_prev;

    private:
        inline void reset() {
            _type = ValueType::Null;
            _valueInt64 = 0LL;
            _valueDouble = 0.0;
            _valueString.clear();

            _key.clear();
            _child = nullptr;
            _next = nullptr;
            _prev = nullptr;
        }

    public:
        // 默认构造
        BasicJSON<_TRAITS, _ALLOC>() {
            reset();
        }

        ~BasicJSON<_TRAITS, _ALLOC>() {
            clear();
        }

        ValueType type() const {
            return _type;
        }

        inline bool Parse(const char *src) {
            return ParseWithOpts(src, nullptr, 0);
        }

        bool ParseWithOpts(const char *src, const char **return_parse_end, int require_null_terminated) {
            clear();
            const char *end = parse_value(skip(src));
            ep = nullptr;
            if (end == nullptr) return false;

            // if we require null-terminated JSON without appended garbage, skip and then check for a null terminator
            if (require_null_terminated) { end = skip(end); if (*end) { ep = end; return 0; } }
            if (return_parse_end) *return_parse_end = end;
            return true;
        }

        void clear() {
            if (_child != nullptr) {
                Delete(_child);
                _child = nullptr;
            }

            if (_next != nullptr) {
                Delete(_next);
                _next = nullptr;
            }
            reset();
        }

        std::string stringfiy() const {
            std::string ret;
            print_value(ret, 0, true);
            return ret;
        }

        // 带参构造
        template <class _T>
        explicit BasicJSON<_TRAITS, _ALLOC>(_T &&t) {
            reset();
            __cpp_basic_json_impl::AssignImpl<_TRAITS, _ALLOC,
                typename std::remove_cv<typename std::remove_reference<_T>::type>::type>::invoke(*this, std::forward<_T>(t));
        }

        template <class _T>
        explicit BasicJSON<_TRAITS, _ALLOC>(const std::initializer_list<_T> &il) {
            reset();
            __cpp_basic_json_impl::AssignImpl<_TRAITS, _ALLOC, std::initializer_list<_T> >::invoke(*this, il);
        }

        template <class _T>
        explicit BasicJSON<_TRAITS, _ALLOC>(std::initializer_list<_T> &&il) {
            reset();
            __cpp_basic_json_impl::AssignImpl<_TRAITS, _ALLOC, std::initializer_list<_T> >::invoke(*this, il);
        }

        // 复制构造
        BasicJSON<_TRAITS, _ALLOC>(const BasicJSON<_TRAITS, _ALLOC> &other) {
            reset();
            Duplicate(*this, other, true);
        }

        // 移动构造
        BasicJSON<_TRAITS, _ALLOC>(BasicJSON<_TRAITS, _ALLOC> &&other) {
            _type = other._type;
            _valueInt64 = other._valueInt64;
            _valueDouble = other._valueDouble;
            _valueString = std::move(other._valueString);

            _key = std::move(other._key);
            _child = other._child;
            _next = other._next;
            _prev = other._prev;

            other.reset();
        }

        // 赋值
        BasicJSON<_TRAITS, _ALLOC> &operator=(const BasicJSON<_TRAITS, _ALLOC> &other) {
            clear();
            Duplicate(*this, other, true);
            return *this;
        }

        // 移动赋值
        BasicJSON<_TRAITS, _ALLOC> &operator=(BasicJSON<_TRAITS, _ALLOC> &&other) {
            clear();

            _type = other._type;
            _valueInt64 = other._valueInt64;
            _valueDouble = other._valueDouble;
            _valueString = std::move(other._valueString);

            _key = std::move(other._key);
            _child = other._child;
            _next = other._next;
            _prev = other._prev;

            other.reset();
            return *this;
        }

        // 用nullptr赋值
        BasicJSON<_TRAITS, _ALLOC> &operator=(std::nullptr_t) {
            clear();
            return *this;
        }

        // 重载与nullptr的比较
        inline bool operator==(std::nullptr_t) const {
            return (_type == ValueType::Null);
        }

        inline bool operator!=(std::nullptr_t) const {
            return (_type != ValueType::Null);
        }

		// as
        template <class _T> _T as() const {
            return __cpp_basic_json_impl::AsImpl<_TRAITS, _ALLOC, _T>::invoke(*this);
        }

    public:
        class iterator {
            friend class BasicJSON<_TRAITS, _ALLOC>;
            friend class reverse_iterator;
            ThisType *_ptr;

            iterator(ThisType *ptr) throw() : _ptr(ptr) {
            }

        public:
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef ThisType value_type;
            typedef ptrdiff_t difference_type;
            typedef difference_type distance_type;
            typedef value_type *pointer;
            typedef value_type &reference;

            iterator() throw() : _ptr(nullptr) {
            }

            iterator(const iterator &other) throw() : _ptr(other._ptr) {
            }

            iterator &operator=(const iterator &other) throw() {
                _ptr = other._ptr;
                return *this;
            }

            inline ThisType &operator*() throw() {
                return *_ptr;
            }

            inline const ThisType &operator*() const throw() {
                return *_ptr;
            }

            inline ThisType *operator->() throw() {
                return _ptr;
            }

            inline const ThisType *operator->() const throw() {
                return _ptr;
            }

            inline iterator &operator++() throw() {
                _ptr = _ptr->_next;
                return *this;
            }

            inline iterator operator++(int) throw() {
                iterator ret(this);
                _ptr = _ptr->_next;
                return ret;
            }

            inline iterator &operator--() throw() {
                _ptr = _ptr->_prev;
                return *this;
            }

            inline iterator operator--(int) throw() {
                iterator ret(this);
                _ptr = _ptr->_prev;
                return ret;
            }

            inline bool operator==(const iterator &other) const throw() {
                return _ptr == other._ptr;
            }

            inline bool operator!=(const iterator &other) const throw() {
                return _ptr != other._ptr;
            }
        };

        class reverse_iterator {
            friend class BasicJSON<_TRAITS, _ALLOC>;
            ThisType *_ptr;

            reverse_iterator(ThisType *ptr) throw() : _ptr(ptr) {
            }

        public:
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef ThisType value_type;
            typedef ptrdiff_t difference_type;
            typedef difference_type distance_type;
            typedef value_type *pointer;
            typedef value_type &reference;

            reverse_iterator() throw() : _ptr(nullptr) {
            }

            reverse_iterator(const reverse_iterator &other) throw() : _ptr(other._ptr) {
            }

            reverse_iterator &operator=(const reverse_iterator &other) throw() {
                _ptr = other._ptr;
                return *this;
            }

            iterator base() {
                return iterator(_ptr);
            }

            inline ThisType &operator*() throw() {
                return *_ptr;
            }

            inline const ThisType &operator*() const throw() {
                return *_ptr;
            }

            inline ThisType *operator->() throw() {
                return _ptr;
            }

            inline const ThisType *operator->() const throw() {
                return _ptr;
            }

            inline reverse_iterator &operator++() throw() {
                _ptr = _ptr->_prev;
                return *this;
            }

            inline reverse_iterator operator++(int) throw() {
                reverse_iterator ret(this);
                _ptr = _ptr->_prev;
                return ret;
            }

            inline reverse_iterator &operator--() throw() {
                _ptr = _ptr->_next;
                return *this;
            }

            inline reverse_iterator operator--(int) throw() {
                reverse_iterator ret(this);
                _ptr = _ptr->_next;
                return ret;
            }

            inline bool operator==(const reverse_iterator &other) const throw() {
                return _ptr == other._ptr;
            }

            inline bool operator!=(const reverse_iterator &other) const throw() {
                return _ptr != other._ptr;
            }
        };

        class const_iterator {
            friend class const_reverse_iterator;
            friend class BasicJSON<_TRAITS, _ALLOC>;
            const ThisType *_ptr;

            const_iterator(ThisType *ptr) throw() : _ptr(ptr) {
            }

        public:
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef ThisType value_type;
            typedef ptrdiff_t difference_type;
            typedef difference_type distance_type;
            typedef value_type *pointer;
            typedef value_type &reference;

            const_iterator() throw() : _ptr(nullptr) {
            }

            const_iterator(const const_iterator &other) throw() : _ptr(other._ptr) {
            }

            const_iterator &operator=(const const_iterator &other) throw() {
                _ptr = other._ptr;
                return *this;
            }

            inline const ThisType &operator*() const throw() {
                return *_ptr;
            }

            inline const ThisType *operator->() const throw() {
                return _ptr;
            }

            inline const_iterator &operator++() throw() {
                _ptr = _ptr->_next;
                return *this;
            }

            inline const_iterator operator++(int) throw() {
                const_iterator ret(this);
                _ptr = _ptr->_next;
                return ret;
            }

            inline const_iterator &operator--() throw() {
                _ptr = _ptr->_prev;
                return *this;
            }

            inline const_iterator operator--(int) throw() {
                const_iterator ret(this);
                _ptr = _ptr->_prev;
                return ret;
            }

            inline bool operator==(const const_iterator &other) const throw() {
                return _ptr == other._ptr;
            }

            inline bool operator!=(const const_iterator &other) const throw() {
                return _ptr != other._ptr;
            }
        };

        class const_reverse_iterator {
            friend class BasicJSON<_TRAITS, _ALLOC>;
            const ThisType *_ptr;

            const_reverse_iterator(ThisType *ptr) throw() : _ptr(ptr) {
            }

        public:
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef ThisType value_type;
            typedef ptrdiff_t difference_type;
            typedef difference_type distance_type;
            typedef value_type *pointer;
            typedef value_type &reference;

            const_reverse_iterator() throw() : _ptr(nullptr) {
            }

            const_reverse_iterator(const const_reverse_iterator &other) throw() : _ptr(other._ptr) {
            }

            const_reverse_iterator &operator=(const const_reverse_iterator &other) throw() {
                _ptr = other._ptr;
                return *this;
            }

            const_iterator base() {
                return const_iterator(_ptr);
            }

            inline const ThisType &operator*() const throw() {
                return *_ptr;
            }

            inline const ThisType *operator->() const throw() {
                return _ptr;
            }

            inline const_reverse_iterator &operator++() throw() {
                _ptr = _ptr->_next;
                return *this;
            }

            inline const_reverse_iterator operator++(int) throw() {
                const_reverse_iterator ret(this);
                _ptr = _ptr->_next;
                return ret;
            }

            inline const_reverse_iterator &operator--() throw() {
                _ptr = _ptr->_prev;
                return *this;
            }

            inline const_reverse_iterator operator--(int) throw() {
                const_reverse_iterator ret(this);
                _ptr = _ptr->_prev;
                return ret;
            }

            inline bool operator==(const const_reverse_iterator &other) const throw() {
                return _ptr == other._ptr;
            }

            inline bool operator!=(const const_reverse_iterator &other) const throw() {
                return _ptr != other._ptr;
            }
        };

        iterator begin() {
            return iterator(_child);
        }

        const_iterator begin() const {
            return const_iterator(_child);
        }

        const_iterator cbegin() const {
            return const_iterator(_child);
        }

        iterator end() {
            return iterator(nullptr);
        }

        const_iterator end() const {
            return const_iterator(nullptr);
        }

        const_iterator cend() const {
            return const_iterator(nullptr);
        }

        reverse_iterator rbegin() {
            if (_child == nullptr) return reverse_iterator(nullptr);
            ThisType *next = _child->_next, *prev = nullptr;
            while (next != nullptr) {
                prev = next;
                next = next->_next;
            }
            return reverse_iterator(prev);
        }

        const_reverse_iterator rbegin() const {
            if (_child == nullptr) return const_reverse_iterator(nullptr);
            ThisType *next = _child->_next, *prev = nullptr;
            while (next != nullptr) {
                prev = next;
                next = next->_next;
            }
            return const_reverse_iterator(prev);
        }

        const_reverse_iterator crbegin() const {
            if (_child == nullptr) return const_reverse_iterator(nullptr);
            ThisType *next = _child->_next, *prev = nullptr;
            while (next != nullptr) {
                prev = next;
                next = next->_next;
            }
            return const_reverse_iterator(prev);
        }

        reverse_iterator rend() {
            return reverse_iterator(nullptr);
        }

        const_reverse_iterator rend() const {
            return const_reverse_iterator(nullptr);
        }

        const_reverse_iterator crend() const {
            return const_reverse_iterator(nullptr);
        }

        template <class, class, class> friend struct __cpp_basic_json_impl::AssignImpl;
        template <class, class, class> friend struct __cpp_basic_json_impl::AssignFromIntegerImpl;
        template <class, class, class> friend struct __cpp_basic_json_impl::AssignFromFloatImpl;
        template <class, class, class> friend struct __cpp_basic_json_impl::AssignFromStringImpl;
        template <class, class, class> friend struct __cpp_basic_json_impl::AssignFromArrayImpl;
        template <class, class, class> friend struct __cpp_basic_json_impl::AssignFromMapImpl;

        template <class, class, class> friend struct __cpp_basic_json_impl::AsImpl;
        template <class, class, class> friend struct __cpp_basic_json_impl::AsIntegerImpl;
        template <class, class, class> friend struct __cpp_basic_json_impl::AsFloatImpl;
        template <class, class, class> friend struct __cpp_basic_json_impl::AsStringImpl;
        template <class, class, class> friend struct __cpp_basic_json_impl::AsArrayImpl;
        template <class, class, class> friend struct __cpp_basic_json_impl::AsMapImpl;

    private:
        const char *ep = nullptr;
        //typename _ALLOC::template rebind<ThisType>::other _allocator;

        static inline ThisType *New() {
            typedef typename _ALLOC::template rebind<ThisType>::other AllocatorType;
            AllocatorType allocator;
            typename AllocatorType::pointer p = allocator.allocate(sizeof(ThisType));
            allocator.construct(p);
            return (ThisType *)p;
        }

        static inline void Delete(ThisType *c) {
            typedef typename _ALLOC::template rebind<ThisType>::other AllocatorType;
            AllocatorType allocator;
            allocator.destroy(c);
            allocator.deallocate(c, sizeof(ThisType));
        }

        static const char *skip(const char *in) {
            while (in != nullptr && *in != 0 && (unsigned char)*in <= 32) ++in;
            return in;
        }

        const char *parse_value(const char *value) {
            if (value == nullptr) return 0; // Fail on null.
            if (!strncmp(value, "null", 4)) { _type = ValueType::Null;  return value + 4; }
            if (!strncmp(value, "false", 5)) { _type = ValueType::False; return value + 5; }
            if (!strncmp(value, "true", 4)) { _type = ValueType::True; _valueInt64 = 1; return value + 4; }
            if (*value == '\"') { return parse_string(value); }
            if (*value == '-' || (*value >= '0' && *value <= '9')) { return parse_number(value); }
            if (*value == '[') { return parse_array(value); }
            if (*value == '{') { return parse_object(value); }

            ep = value; return nullptr; // failure.
        }

        static unsigned parse_hex4(const char *str) {
            unsigned h = 0;
            if (*str >= '0' && *str <= '9') h += (*str) - '0';
            else if (*str >= 'A' && *str <= 'F') h += 10 + (*str) - 'A';
            else if (*str >= 'a' && *str <= 'f') h += 10 + (*str) - 'a';
            else return 0;
            h = h << 4; ++str;
            if (*str >= '0' && *str <= '9') h += (*str) - '0';
            else if (*str >= 'A' && *str <= 'F') h += 10 + (*str) - 'A';
            else if (*str >= 'a' && *str <= 'f') h += 10 + (*str) - 'a';
            else return 0;
            h = h << 4; ++str;
            if (*str >= '0' && *str <= '9') h += (*str) - '0';
            else if (*str >= 'A' && *str <= 'F') h += 10 + (*str) - 'A';
            else if (*str >= 'a' && *str <= 'f') h += 10 + (*str) - 'a';
            else return 0;
            h = h << 4; ++str;
            if (*str >= '0' && *str <= '9') h += (*str) - '0';
            else if (*str >= 'A' && *str <= 'F') h += 10 + (*str) - 'A';
            else if (*str >= 'a' && *str <= 'f') h += 10 + (*str) - 'a';
            else return 0;
            return h;
        }

        const char *parse_string(const char *str) {
            static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
            const char *ptr = str + 1; int len = 0; unsigned uc, uc2;
            if (*str != '\"') { ep = str; return 0; }   // not a string!

            while (*ptr != '\"' && *ptr && ++len) if (*ptr++ == '\\') ++ptr;    // Skip escaped quotes.

            _valueString.resize(len + 1);    // This is how long we need for the string, roughly.
            typename StringType::iterator ptr2 = _valueString.begin();

            ptr = str + 1;
            while (*ptr != '\"' && *ptr) {
                if (*ptr != '\\') *ptr2++ = *ptr++;
                else {
                    ++ptr;
                    switch (*ptr) {
                    case 'b': *ptr2++ = '\b';   break;
                    case 'f': *ptr2++ = '\f';   break;
                    case 'n': *ptr2++ = '\n';   break;
                    case 'r': *ptr2++ = '\r';   break;
                    case 't': *ptr2++ = '\t';   break;
                    case 'u':    // transcode utf16 to utf8.
                        uc = parse_hex4(ptr + 1); ptr += 4; // get the unicode char.

                        if ((uc >= 0xDC00 && uc <= 0xDFFF) || uc == 0)  break;  // check for invalid.

                        if (uc >= 0xD800 && uc <= 0xDBFF) { // UTF16 surrogate pairs.
                            if (ptr[1] != '\\' || ptr[2] != 'u')    break;  // missing second-half of surrogate.
                            uc2 = parse_hex4(ptr + 3); ptr += 6;
                            if (uc2 < 0xDC00 || uc2>0xDFFF)       break;  // invalid second-half of surrogate.
                            uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
                        }

                        len = 4; if (uc < 0x80) len = 1; else if (uc < 0x800) len = 2; else if (uc < 0x10000) len = 3; ptr2 += len;

                        switch (len) {
                        case 4: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
                        case 3: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
                        case 2: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
                        case 1: *--ptr2 = (uc | firstByteMark[len]);
                        }
                        ptr2 += len;
                        break;
                    default:  *ptr2++ = *ptr; break;
                    }
                    ++ptr;
                }
            }
            *ptr2 = 0;
            if (*ptr == '\"') ++ptr;
            _type = ValueType::String;
            return ptr;
        }

        const char *parse_number(const char *num) {
            double n = 0, scale = 0;
            int sign = 1, subscale = 0, signsubscale = 1;
            int64_t ll = 0;
            bool point = false;

            if (*num == '-') sign = -1, ++num;  // Has sign?
            if (*num == '0') ++num;         // is zero
            if (*num >= '1' && *num <= '9') do n = (n*10.0) + (*num - '0'), ll = (ll * 10) + (*num++ - '0'); while (*num >= '0' && *num <= '9');    // Number?
            if (*num == '.' && num[1] >= '0' && num[1] <= '9') { point = true; ++num; do n = (n * 10.0) + (*num++ - '0'), --scale; while (*num >= '0' && *num <= '9'); }    // Fractional part?
            if (*num == 'e' || *num == 'E') {   // Exponent?
                point = true; ++num; if (*num == '+') ++num;    else if (*num == '-') signsubscale = -1, ++num;     // With sign?
                while (*num >= '0' && *num <= '9') subscale = (subscale * 10) + (*num++ - '0'); // Number?
            }

            if (point || subscale > 0) {
                n = sign * n * pow(10.0, (scale + subscale * signsubscale));    // number = +/- number.fraction * 10^+/- exponent
                _valueDouble = n;
                _type = ValueType::Float;
            }
            else {
                _valueInt64 = ll; _type = ValueType::Integer;
            }
            return num;
        }

        const char *parse_array(const char *value) {
            ThisType *child;
            if (*value != '[')  { ep = value; return nullptr; } // not an array!

            _type = ValueType::Array;
            value = skip(value + 1);
            if (*value == ']') return value + 1;    // empty array.

            this->_child = child = New();
            if (child == nullptr) return nullptr;        // memory fail
            value = skip(child->parse_value(skip(value)));  // skip any spacing, get the value.
            if (value == nullptr) return nullptr;

            while (*value == ',') {
                ThisType *new_item = New();
                if (new_item == nullptr) return nullptr;     // memory fail
                child->_next = new_item; new_item->_prev = child; child = new_item;
                value = skip(child->parse_value(skip(value + 1)));
                if (value == nullptr) return nullptr; // memory fail
            }

            if (*value == ']') return value + 1;    // end of array
            ep = value; return nullptr; // malformed.
        }

        // Build an object from the text.
        const char *parse_object(const char *value) {
            if (*value != '{')  { ep = value; return nullptr; } // not an object!
            _type = ValueType::Object;
            value = skip(value + 1);
            if (*value == '}') return value + 1;    // empty array.

            ThisType *child;
            _child = child = New();
            if (_child == nullptr) return nullptr;
            value = skip(child->parse_string(skip(value)));
            if (value == nullptr) return nullptr;
            child->_key = std::move(child->_valueString); child->_valueString.clear();
            if (*value != ':') { ep = value; return nullptr; }  // fail!
            value = skip(child->parse_value(skip(value + 1)));  // skip any spacing, get the value.
            if (value == nullptr) return nullptr;

            while (*value == ',') {
                ThisType *new_item = New();
                if (new_item == nullptr)   return nullptr; // memory fail
                child->_next = new_item; new_item->_prev = child; child = new_item;
                value = skip(child->parse_string(skip(value + 1)));
                if (value == nullptr) return nullptr;
                child->_key = std::move(child->_valueString); child->_valueString.clear();
                if (*value != ':') { ep = value; return nullptr; }  // fail!
                value = skip(child->parse_value(skip(value + 1)));  // skip any spacing, get the value.
                if (value == nullptr) return nullptr;
            }

            if (*value == '}') return value + 1;    // end of array
            ep = value; return nullptr; // malformed.
        }

        template <class _STRING>
        void print_value(_STRING &ret, int depth, bool fmt) const {
            switch (_type) {
            case ValueType::Null: ret.append("null"); break;
            case ValueType::False: ret.append("false"); break;
            case ValueType::True: ret.append("true"); break;
            case ValueType::Integer: print_integer(ret); break;
            case ValueType::Float: print_float(ret); break;
            case ValueType::String: print_string(ret); break;
            case ValueType::Array: print_array(ret, depth, fmt); break;
            case ValueType::Object: print_object(ret, depth, fmt); break;
            default: break;
            }
        }

        template <class _STRING>
        void print_integer(_STRING &ret) const {
            char str[21];  // 2^64+1 can be represented in 21 chars.
            sprintf(str, "%" PRId64, _valueInt64);
            ret.append(str);
        }

        template <class _STRING>
        void print_float(_STRING &ret) const {
            char str[64];  // This is a nice tradeoff.
            double d = _valueDouble;
            if (fabs(floor(d) - d) <= DBL_EPSILON && fabs(d) < 1.0e60) sprintf(str, "%.0f", d);
            else if (fabs(d) < 1.0e-6 || fabs(d) > 1.0e9)           sprintf(str, "%e", d);
            else                                                sprintf(str, "%f", d);
            ret.append(str);
        }

        template <class _STRING>
        static void print_string_ptr(_STRING &ret, const StringType &str) {
            if (str.empty()) return;

            const char *ptr; int len = 0; unsigned char token;
            ptr = str.c_str(); while ((token = *ptr) && ++len) { if (strchr("\"\\\b\f\n\r\t", token)) ++len; else if (token < 32) len += 5; ++ptr; }

            ret.reserve(ret.size() + len + 3);
            ptr = str.c_str();
            ret.append(1, '\"');
            while (*ptr) {
                if ((unsigned char)*ptr > 31 && *ptr != '\"' && *ptr != '\\') ret.append(1, *ptr++);
                else {
                    ret.append(1, '\\');
                    switch (token = *ptr++) {
                    case '\\':  ret.append(1, '\\'); break;
                    case '\"':  ret.append(1, '\"'); break;
                    case '\b':  ret.append(1, 'b'); break;
                    case '\f':  ret.append(1, 'f'); break;
                    case '\n':  ret.append(1, 'n'); break;
                    case '\r':  ret.append(1, 'r'); break;
                    case '\t':  ret.append(1, 't'); break;
                    default: { char ptr2[8]; sprintf(ptr2, "u%04x", token); ret.append(ptr2); } break;  // escape and print
                    }
                }
            }
            ret.append(1, '\"');
        }

        template <class _STRING>
        void print_string(_STRING &ret) const {
            print_string_ptr(ret, _valueString);
        }

        template <class _STRING>
        void print_array(_STRING &ret, int depth, bool fmt) const {
            ThisType *child = _child;
            int numentries = 0, i = 0;

            // How many entries in the array?
            while (child != nullptr) ++numentries, child = child->_next;
            // Explicitly handle numentries==0
            if (numentries == 0) {
                ret.append("[]");
                return;
            }

            // Retrieve all the results:
            ret.append("[");
            for (child = _child; child != nullptr; child = child->_next, ++i) {
                child->print_value(ret, depth + 1, fmt);
                if (i != numentries - 1) { ret.append(1, ','); if (fmt) ret.append(1, ' '); }
            }
            ret.append("]");
        }

        template <class _STRING>
        void print_object(_STRING &ret, int depth, bool fmt) const {
            int numentries = 0;
            ThisType *child = _child;
            // Count the number of entries.
            while (child != nullptr) ++numentries, child = child->_next;
            // Explicitly handle empty object case
            if (numentries == 0) {
                ret.append(1, '{');
                if (fmt) { ret.append(1, '\n'); if (depth > 0) ret.append(depth - 1, '\t'); }
                ret.append(1, '}');
                return;
            }

            // Compose the output:
            child = _child; ++depth;
            ret.append(1, '{'); if (fmt) ret.append(1, '\n');
            for (int i = 0; i < numentries; ++i) {
                if (fmt) ret.append(depth, '\t');
                print_string_ptr(ret, child->_key);
                ret.append(1, ':'); if (fmt) ret.append(1, '\t');
                child->print_value(ret, depth, fmt);
                if (i != numentries - 1) ret.append(1, ',');
                if (fmt) ret.append(1, '\n');
                child = child->_next;
            }
            if (fmt) ret.append(depth - 1, '\t');
            ret.append(1, '}');
        }

        // TODO:
        static bool Duplicate(ThisType &newitem, const ThisType &item, bool recurse) {
            newitem.clear();
            const ThisType *cptr;
            ThisType *nptr = nullptr, *newchild;
            // Copy over all vars
            newitem._type = item._type, newitem._valueInt64 = item._valueInt64, newitem._valueDouble = item._valueDouble;
            newitem._valueString = item._valueString;
            newitem._key = item._key;
            // If non-recursive, then we're done!
            if (!recurse) return true;
            // Walk the ->next chain for the child.
            cptr = item._child;
            while (cptr != nullptr) {
                newchild = New();
                if (newchild == nullptr) return false;
                if (!Duplicate(*newchild, *cptr, true)) { Delete(newchild); return false; }     // Duplicate (with recurse) each item in the ->next chain
                if (nptr != nullptr)    { nptr->_next = newchild, newchild->_prev = nptr; nptr = newchild; }    // If newitem->child already set, then crosswire ->prev and ->next and move on
                else        { newitem._child = newchild; nptr = newchild; }                 // Set newitem->child and move to it
                cptr = cptr->_next;
            }
            return true;
        }

    public:
        inline std::string Print() const {
            std::string ret;
            print_value(ret, 0, true);
            return ret;
        }

        inline std::string PrintUnformatted() const {
            std::string ret;
            print_value(ret, 0, false);
            return ret;
        }

        static void Minify(char *json) {
            char *into = json;
            while (*json) {
                if (*json == ' ') ++json;
                else if (*json == '\t') ++json; // Whitespace characters.
                else if (*json == '\r') ++json;
                else if (*json == '\n') ++json;
                else if (*json == '/' && json[1] == '/')  while (*json && *json != '\n') ++json;    // double-slash comments, to end of line.
                else if (*json == '/' && json[1] == '*') { while (*json && !(*json == '*' && json[1] == '/')) ++json; json += 2; }  // multiline comments.
                else if (*json == '\"'){ *into++ = *json++; while (*json && *json != '\"'){ if (*json == '\\') *into++ = *json++; *into++ = *json++; }*into++ = *json++; } // string literals, which are \" sensitive.
                else *into++ = *json++;         // All other characters.
            }
            *into = 0;  // and null-terminate.
        }
    };

    // 流输出
    template <class _OS, class _TRAITS, class _ALLOC>
    static inline _OS &operator<<(_OS &os, const BasicJSON<_TRAITS, _ALLOC> &c) {
        os << c.Print();
        return os;
    }

    // 重载与nullptr的比较
    template <class _TRAITS, class _ALLOC>
    static inline bool operator==(std::nullptr_t, const BasicJSON<_TRAITS, _ALLOC> &c) throw() {
        return c.operator==(nullptr);
    }

    template <class _TRAITS, class _ALLOC>
    static inline bool operator!=(std::nullptr_t, const BasicJSON<_TRAITS, _ALLOC> &c) {
        return c.operator!=(nullptr);
    }

    namespace __cpp_basic_json_impl {
        template <class _TRAITS, class _ALLOC, class _T>
        void AssignImpl<_TRAITS, _ALLOC, _T>::invoke(typename AssignImpl<_TRAITS, _ALLOC, _T>::JsonType &c,
            typename AssignImpl<_TRAITS, _ALLOC, _T>::SourceType arg) {
            // TODO:
            //static_assert(0, "unimplemented type");
        }

		template <class _TRAITS, class _ALLOC>
		struct AssignImpl<_TRAITS, _ALLOC, std::nullptr_t> {
			typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
			typedef std::nullptr_t SourceType;
			static inline void invoke(JsonType &c, SourceType) {
				c._type = JsonType::ValueType::Null;
			}
		};

		template <class _TRAITS, class _ALLOC>
		struct AssignImpl<_TRAITS, _ALLOC, bool> {
			typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
			typedef bool SourceType;
			static inline void invoke(JsonType &c, bool src) {
				c._type = src ? JsonType::ValueType::True : JsonType::ValueType::False;
			}
		};

		template <class _TRAITS, class _ALLOC, class _INTEGER>
        struct AssignFromIntegerImpl {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _INTEGER SourceType;
            static inline void invoke(JsonType &c, SourceType arg) {
                c._type = JsonType::ValueType::Integer;
                c._valueInt64 = arg;
            }
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, char>
            : AssignFromIntegerImpl<_TRAITS, _ALLOC, char> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, signed char>
            : AssignFromIntegerImpl<_TRAITS, _ALLOC, signed char> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, unsigned char>
            : AssignFromIntegerImpl<_TRAITS, _ALLOC, unsigned char> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, short>
            : AssignFromIntegerImpl<_TRAITS, _ALLOC, short> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, unsigned short>
            : AssignFromIntegerImpl<_TRAITS, _ALLOC, unsigned short> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, int>
            : AssignFromIntegerImpl<_TRAITS, _ALLOC, int> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, unsigned>
            : AssignFromIntegerImpl<_TRAITS, _ALLOC, unsigned> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, long>
            : AssignFromIntegerImpl<_TRAITS, _ALLOC, long> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, unsigned long>
            : AssignFromIntegerImpl<_TRAITS, _ALLOC, unsigned long> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, int64_t>
            : AssignFromIntegerImpl<_TRAITS, _ALLOC, int64_t> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, uint64_t>
            : AssignFromIntegerImpl<_TRAITS, _ALLOC, uint64_t> {
        };

        template <class _TRAITS, class _ALLOC, class _FLOAT>
        struct AssignFromFloatImpl {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _FLOAT SourceType;
            static inline void invoke(JsonType &c, SourceType arg) {
                c._type = JsonType::ValueType::Float;
                c._valueDouble = arg;
            }
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, float>
            : AssignFromFloatImpl<_TRAITS, _ALLOC, float> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, double>
            : AssignFromFloatImpl<_TRAITS, _ALLOC, double> {
        };

        static inline const char *_ConvertString(const char *str) {
            return str;
        }

        template <class _TR, class _AX>
        static inline const char *_ConvertString(const std::basic_string<char, _TR, _AX> &str) {
            return str.c_str();
        }

        template <class _TRAITS, class _ALLOC, class _STRING>
        struct AssignFromStringImpl {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _STRING SourceType;
            static inline void invoke(JsonType &c, const SourceType &arg) {
                c._type = JsonType::ValueType::String;
                c._valueString = _ConvertString(arg);
            }
        };

		template <class _TRAITS, class _ALLOC, size_t _N>
        struct AssignImpl<_TRAITS, _ALLOC, char [_N]>
            : AssignFromStringImpl<_TRAITS, _ALLOC, char [_N]> {
        };

		template <class _TRAITS, class _ALLOC>
		struct AssignImpl<_TRAITS, _ALLOC, char *>
			: AssignFromStringImpl<_TRAITS, _ALLOC, char *> {
		};

		template <class _TRAITS, class _ALLOC>
		struct AssignImpl<_TRAITS, _ALLOC, const char *>
			: AssignFromStringImpl<_TRAITS, _ALLOC, const char *> {
		};

		template <class _TRAITS, class _ALLOC, class _TR, class _AX>
        struct AssignImpl<_TRAITS, _ALLOC, std::basic_string<char, _TR, _AX> >
            : AssignFromStringImpl<_TRAITS, _ALLOC, std::basic_string<char, _TR, _AX> > {
        };

        template <class _TRAITS, class _ALLOC>
        struct AssignImpl<_TRAITS, _ALLOC, std::basic_string<char, _TRAITS, _ALLOC> > {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef std::basic_string<char, _TRAITS, _ALLOC> SourceType;
            static inline void invoke(JsonType &c, const SourceType &arg) {
                c._type = JsonType::ValueType::String;
                c._valueString = arg;
            }

            static inline void invoke(JsonType &c, SourceType &&arg) {
                c._type = JsonType::ValueType::String;
                c._valueString = std::move(arg);
            }
        };

        template <class _TRAITS, class _ALLOC, class _ARRAY, size_t _N>
        struct AssignImpl<_TRAITS, _ALLOC, _ARRAY [_N]> {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _ARRAY SourceType[_N];
            static void invoke(JsonType &c, const SourceType &arg) {
                c._type = JsonType::ValueType::Array;
                JsonType *prev = nullptr;
                for (size_t i = 0; i < _N; ++i) {
                    JsonType *item = JsonType::New();
                    AssignImpl<_TRAITS, _ALLOC, _ARRAY>::invoke(*item, arg[i]);
                    if (i != 0) {
                        prev->_next = item;
                        item->_prev = prev;
                    } else {
                        c._child = item;
                    }
                    prev = item;
                }
            }

            static void invoke(JsonType &c, SourceType &&arg) {
                c._type = JsonType::ValueType::Array;
                JsonType *prev = nullptr;
                for (size_t i = 0; i < _N; ++i) {
                    JsonType *item = JsonType::New();
                    AssignImpl<_TRAITS, _ALLOC, _ARRAY>::invoke(*item, std::move(arg[i]));
                    if (i != 0) {
                        prev->_next = item;
                        item->_prev = prev;
                    } else {
                        c._child = item;
                    }
                    prev = item;
                }
            }
        };

        template <class _TRAITS, class _ALLOC, class _ARRAY>
        struct AssignFromArrayImpl {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _ARRAY SourceType;
            static void invoke(JsonType &c, const SourceType &arg) {
                c._type = JsonType::ValueType::Array;
                JsonType *prev = nullptr;
                for (typename SourceType::const_iterator it = arg.begin(); it != arg.end(); ++it) {
                    JsonType *item = JsonType::New();
                    AssignImpl<_TRAITS, _ALLOC, typename _ARRAY::value_type>::invoke(*item, *it);
                    if (it != arg.begin()) {
                        prev->_next = item;
                        item->_prev = prev;
                    } else {
                        c._child = item;
                    }
                    prev = item;
                }
            }

            static void invoke(JsonType &c, SourceType &&arg) {
                c._type = JsonType::ValueType::Array;
                JsonType *prev = nullptr;
                for (typename SourceType::iterator it = arg.begin(); it != arg.end(); ++it) {
                    JsonType *item = JsonType::New();
                    AssignImpl<_TRAITS, _ALLOC, typename _ARRAY::value_type>::invoke(*item, std::move(*it));
                    if (it != arg.begin()) {
                        prev->_next = item;
                        item->_prev = prev;
                    } else {
                        c._child = item;
                    }
                    prev = item;
                }
            }
        };

        template <class _TRAITS, class _ALLOC, class _T, class _AX>
        struct AssignImpl<_TRAITS, _ALLOC, std::vector<_T, _AX> >
            : AssignFromArrayImpl<_TRAITS, _ALLOC, std::vector<_T, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _T, class _AX>
        struct AssignImpl<_TRAITS, _ALLOC, std::list<_T, _AX> >
            : AssignFromArrayImpl<_TRAITS, _ALLOC, std::list<_T, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _T, class _CMP, class _AX>
        struct AssignImpl<_TRAITS, _ALLOC, std::set<_T, _CMP, _AX> >
            : AssignFromArrayImpl<_TRAITS, _ALLOC, std::set<_T, _CMP, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _T, class _CMP, class _AX>
        struct AssignImpl<_TRAITS, _ALLOC, std::multiset<_T, _CMP, _AX> >
            : AssignFromArrayImpl<_TRAITS, _ALLOC, std::multiset<_T, _CMP, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _T, class _HASH, class _EQ, class _AX>
        struct AssignImpl<_TRAITS, _ALLOC, std::unordered_set<_T, _HASH, _EQ, _AX> >
            : AssignFromArrayImpl<_TRAITS, _ALLOC, std::unordered_set<_T, _HASH, _EQ, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _T, class _HASH, class _EQ, class _AX>
        struct AssignImpl<_TRAITS, _ALLOC, std::unordered_multiset<_T, _HASH, _EQ, _AX> >
            : AssignFromArrayImpl<_TRAITS, _ALLOC, std::unordered_multiset<_T, _HASH, _EQ, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _MAP>
        struct AssignFromMapImpl {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _MAP SourceType;
            static void invoke(JsonType &c, const SourceType &arg) {
                static_assert(std::is_convertible<const char *, typename _MAP::key_type>::value, "key_type must be able to convert to const char *");
                c._type = JsonType::ValueType::Object;
                JsonType *prev = nullptr;
                for (typename SourceType::const_iterator it = arg.begin(); it != arg.end(); ++it) {
                    JsonType *item = JsonType::New();
                    item->_key = _ConvertString(it->first);
                    AssignImpl<_TRAITS, _ALLOC, typename _MAP::mapped_type>::invoke(*item, it->second);
                    if (it != arg.begin()) {
                        prev->_next = item;
                        item->_prev = prev;
                    } else {
                        c._child = item;
                    }
                    prev = item;
                }
            }

            static void invoke(JsonType &c, SourceType &&arg) {
                static_assert(std::is_convertible<const char *, typename _MAP::key_type>::value, "key_type must be able to convert to const char *");
                c._type = JsonType::ValueType::Object;
                JsonType *prev = nullptr;
                for (typename SourceType::iterator it = arg.begin(); it != arg.end(); ++it) {
                    JsonType *item = JsonType::New();
                    item->_key = _ConvertString(it->first);
                    AssignImpl<_TRAITS, _ALLOC, typename _MAP::mapped_type>::invoke(*item, std::move(it->second));
                    if (it != arg.begin()) {
                        prev->_next = item;
                        item->_prev = prev;
                    } else {
                        c._child = item;
                    }
                    prev = item;
                }
            }
        };

        template <class _TRAITS, class _ALLOC, class _KEY, class _VAL, class _CMP, class _AX>
        struct AssignImpl<_TRAITS, _ALLOC, std::map<_KEY, _VAL, _CMP, _AX> >
            : AssignFromMapImpl<_TRAITS, _ALLOC, std::map<_KEY, _VAL, _CMP, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _KEY, class _VAL, class _CMP, class _AX>
        struct AssignImpl<_TRAITS, _ALLOC, std::multimap<_KEY, _VAL, _CMP, _AX> >
            : AssignFromMapImpl<_TRAITS, _ALLOC, std::multimap<_KEY, _VAL, _CMP, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _KEY, class _VAL, class _HASH, class _EQ, class _AX>
        struct AssignImpl<_TRAITS, _ALLOC, std::unordered_map<_KEY, _VAL, _HASH, _EQ, _AX> >
            : AssignFromMapImpl<_TRAITS, _ALLOC, std::unordered_map<_KEY, _VAL, _HASH, _EQ, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _KEY, class _VAL, class _HASH, class _EQ, class _AX>
        struct AssignImpl<_TRAITS, _ALLOC, std::unordered_multimap<_KEY, _VAL, _HASH, _EQ, _AX> >
            : AssignFromMapImpl<_TRAITS, _ALLOC, std::unordered_multimap<_KEY, _VAL, _HASH, _EQ, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _T>
        struct AssignImpl<_TRAITS, _ALLOC, std::initializer_list<_T> > {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef std::initializer_list<_T> SourceType;
            static void invoke(JsonType &c, const SourceType &arg) {
                // TODO:
            }
        };

        template <class _TRAITS, class _ALLOC, class _T>
        typename AsImpl<_TRAITS, _ALLOC, _T>::TargetType
            AsImpl<_TRAITS, _ALLOC, _T>::invoke(const typename AsImpl<_TRAITS, _ALLOC, _T>::JsonType &c) {
            std::basic_stringstream<char, _TRAITS, typename _ALLOC::template rebind<char>::other> ss;
            switch (c._type) {
            case JsonType::ValueType::Null: break;
            case JsonType::ValueType::False: ss << 0; break;
            case JsonType::ValueType::True: ss << 1; break;
            case JsonType::ValueType::Integer: ss << c._valueInt64; break;
            case JsonType::ValueType::Float: ss << c._valueDouble; break;
            case JsonType::ValueType::String: ss << c._valueString; break;
            case JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to target type"); break;
            case JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to target type"); break;
            default: throw std::out_of_range("JSON type out of range"); break;
            }
            TargetType ret = TargetType();
            // TODO：
            //ss >> ret;
            return ret;
        }

        //template <class _TRAITS, class _ALLOC>
        //struct AsImpl<_TRAITS, _ALLOC, const BasicJSON<_TRAITS, _ALLOC> *> {
        //    typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
        //    typedef const BasicJSON<_TRAITS, _ALLOC> *TargetType;
        //    static TargetType invoke(const JsonType &c) {
        //        return &c;
        //    }
        //};

        template <class _TRAITS, class _ALLOC, class _INTEGER>
        struct AsIntegerImpl {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _INTEGER TargetType;
            static TargetType invoke(const JsonType &c) {
                switch (c._type) {
                case JsonType::ValueType::Null: return TargetType(0);
                case JsonType::ValueType::False: return TargetType(0);
                case JsonType::ValueType::True: return TargetType(1);
                case JsonType::ValueType::Integer: return static_cast<TargetType>(c._valueInt64);
                case JsonType::ValueType::Float: return static_cast<TargetType>(c._valueDouble);
                case JsonType::ValueType::String: return static_cast<TargetType>(atoll(c._valueString.c_str()));
                case JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Integer"); break;
                case JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Integer"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, char>
            : AsIntegerImpl<_TRAITS, _ALLOC, char> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, signed char>
            : AsIntegerImpl<_TRAITS, _ALLOC, signed char> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, unsigned char>
            : AsIntegerImpl<_TRAITS, _ALLOC, unsigned char> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, short>
            : AsIntegerImpl<_TRAITS, _ALLOC, short> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, unsigned short>
            : AsIntegerImpl<_TRAITS, _ALLOC, unsigned short> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, int>
            : AsIntegerImpl<_TRAITS, _ALLOC, int> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, unsigned>
            : AsIntegerImpl<_TRAITS, _ALLOC, unsigned> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, long>
            : AsIntegerImpl<_TRAITS, _ALLOC, long> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, unsigned long>
            : AsIntegerImpl<_TRAITS, _ALLOC, unsigned long> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, int64_t>
            : AsIntegerImpl<_TRAITS, _ALLOC, int64_t> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, uint64_t>
            : AsIntegerImpl<_TRAITS, _ALLOC, uint64_t> {
        };

        template <class _TRAITS, class _ALLOC, class _FLOAT>
        struct AsFloatImpl {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _FLOAT TargetType;
            static TargetType invoke(const JsonType &c) {
                switch (c._type) {
                case JsonType::ValueType::Null: return TargetType(0);
                case JsonType::ValueType::False: return TargetType(0);
                case JsonType::ValueType::True: return TargetType(1);
                case JsonType::ValueType::Integer: return static_cast<TargetType>(c._valueInt64);
                case JsonType::ValueType::Float: return static_cast<TargetType>(c._valueDouble);
                case JsonType::ValueType::String: return static_cast<TargetType>(atof(c._valueString.c_str()));
                case JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Float"); break;
                case JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Float"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, float>
            : AsFloatImpl<_TRAITS, _ALLOC, float> {
        };

        template <class _TRAITS, class _ALLOC>
        struct AsImpl<_TRAITS, _ALLOC, double>
            : AsFloatImpl<_TRAITS, _ALLOC, double> {
        };

        template <class _TRAITS, class _ALLOC, class _STRING>
        struct AsStringImpl {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _STRING TargetType;
            static TargetType invoke(const JsonType &c) {
                switch (c._type) {
                case JsonType::ValueType::Null: return TargetType();
                case JsonType::ValueType::False: return TargetType("false");
                case JsonType::ValueType::True: return TargetType("true");
                case JsonType::ValueType::Integer: {
                    char str[21];  // 2^64+1 can be represented in 21 chars.
                    sprintf(str, "%" PRId64, c._valueInt64);
                    return TargetType(str);
                }
                case JsonType::ValueType::Float: {
                    std::basic_ostringstream<typename _STRING::value_type, typename _STRING::traits_type, typename _STRING::allocator_type> ss;
                    ss << c._valueDouble;
                    return ss.str();
                }
                case JsonType::ValueType::String: return TargetType(c._valueString.begin(), c._valueString.end());
                case JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to String"); break;
                case JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to String"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        template <class _TRAITS, class _ALLOC, class _CHAR, class _TR, class _AX>
        struct AsImpl<_TRAITS, _ALLOC, std::basic_string<_CHAR, _TR, _AX> >
            : AsStringImpl<_TRAITS, _ALLOC, std::basic_string<_CHAR, _TR, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _ARRAY>
        struct AsArrayImpl {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _ARRAY TargetType;
            static TargetType invoke(const JsonType &c) {
                switch (c._type) {
                case JsonType::ValueType::Null: return TargetType();
                case JsonType::ValueType::False: throw std::logic_error("Cannot convert JSON_False to Array"); break;
                case JsonType::ValueType::True: throw std::logic_error("Cannot convert JSON_True to Array"); break;
                case JsonType::ValueType::Integer: throw std::logic_error("Cannot convert JSON_Integer to Array"); break;
                case JsonType::ValueType::Float: throw std::logic_error("Cannot convert JSON_Float to Array"); break;
                case JsonType::ValueType::String: throw std::logic_error("Cannot convert JSON_String to Array"); break;
                case JsonType::ValueType::Array: {
                    TargetType ret = TargetType();
                    std::transform(c.begin(), c.end(), std::inserter(ret, ret.begin()),
                        &AsImpl<_TRAITS, _ALLOC, typename TargetType::value_type>::invoke);
                    return ret;
                }
                case JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Array"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        template <class _TRAITS, class _ALLOC, class _T, class _AX>
        struct AsImpl<_TRAITS, _ALLOC, std::vector<_T, _AX> >
            : AsArrayImpl<_TRAITS, _ALLOC, std::vector<_T, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _T, class _AX>
        struct AsImpl<_TRAITS, _ALLOC, std::list<_T, _AX> >
            : AsArrayImpl<_TRAITS, _ALLOC, std::list<_T, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _T, class _CMP, class _AX>
        struct AsImpl<_TRAITS, _ALLOC, std::set<_T, _CMP, _AX> >
            : AsArrayImpl<_TRAITS, _ALLOC, std::set<_T, _CMP, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _T, class _CMP, class _AX>
        struct AsImpl<_TRAITS, _ALLOC, std::multiset<_T, _CMP, _AX> >
            : AsArrayImpl<_TRAITS, _ALLOC, std::multiset<_T, _CMP, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _T, class _HASH, class _EQ, class _AX>
        struct AsImpl<_TRAITS, _ALLOC, std::unordered_set<_T, _HASH, _EQ, _AX> >
            : AsArrayImpl<_TRAITS, _ALLOC, std::unordered_set<_T, _HASH, _EQ, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _T, class _HASH, class _EQ, class _AX>
        struct AsImpl<_TRAITS, _ALLOC, std::unordered_multiset<_T, _HASH, _EQ, _AX> >
            : AsArrayImpl<_TRAITS, _ALLOC, std::unordered_multiset<_T, _HASH, _EQ, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _MAP>
        struct AsMapImpl {
            typedef BasicJSON<_TRAITS, _ALLOC> JsonType;
            typedef _MAP TargetType;
            static TargetType invoke(const JsonType &c) {
                switch (c._type) {
                case JsonType::ValueType::Null: return TargetType();
                case JsonType::ValueType::False: throw std::logic_error("Cannot convert JSON_False to Object"); break;
                case JsonType::ValueType::True: throw std::logic_error("Cannot convert JSON_True to Object"); break;
                case JsonType::ValueType::Integer: throw std::logic_error("Cannot convert JSON_Integer to Object"); break;
                case JsonType::ValueType::Float: throw std::logic_error("Cannot convert JSON_Float to Object"); break;
                case JsonType::ValueType::String: throw std::logic_error("Cannot convert JSON_String to Object"); break;
                case JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Object"); break;
                case JsonType::ValueType::Object: {
                    TargetType ret = TargetType();
                    std::transform(c.begin(), c.end(), std::inserter(ret, ret.begin()), &_make_value);
                    return ret;
                }
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        private:
            static inline typename TargetType::value_type _make_value(const JsonType &j) {
                return typename TargetType::value_type(typename TargetType::key_type(j._key.begin(), j._key.end()),
                    AsImpl<_TRAITS, _ALLOC, typename TargetType::mapped_type>::invoke(j));
            }
        };

        template <class _TRAITS, class _ALLOC, class _STRING, class _VAL, class _CMP, class _AX>
        struct AsImpl<_TRAITS, _ALLOC, std::map<_STRING, _VAL, _CMP, _AX> >
            : AsMapImpl<_TRAITS, _ALLOC, std::map<_STRING, _VAL, _CMP, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _STRING, class _VAL, class _CMP, class _AX>
        struct AsImpl<_TRAITS, _ALLOC, std::multimap<_STRING, _VAL, _CMP, _AX> >
            : AsMapImpl<_TRAITS, _ALLOC, std::multimap<_STRING, _VAL, _CMP, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _STRING, class _VAL, class _HASH, class _EQ, class _AX>
        struct AsImpl<_TRAITS, _ALLOC, std::unordered_map<_STRING, _VAL, _HASH, _EQ, _AX> >
            : AsMapImpl<_TRAITS, _ALLOC, std::unordered_map<_STRING, _VAL, _HASH, _EQ, _AX> > {
        };

        template <class _TRAITS, class _ALLOC, class _STRING, class _VAL, class _HASH, class _EQ, class _AX>
        struct AsImpl<_TRAITS, _ALLOC, std::unordered_multimap<_STRING, _VAL, _HASH, _EQ, _AX> >
            : AsMapImpl<_TRAITS, _ALLOC, std::unordered_multimap<_STRING, _VAL, _HASH, _EQ, _AX> > {
        };
    }

    typedef BasicJSON<std::char_traits<char>, std::allocator<char> > cppJSON;
}

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

#endif
