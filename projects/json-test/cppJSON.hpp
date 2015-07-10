﻿/*
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

#ifndef __cppJSON__h__
#define __cppJSON__h__

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
#include <assert.h>

namespace jw {

    template <class _Traits, class _Alloc>
    class BasicJSON;

    namespace __cpp_basic_json_impl {

        template <class _JsonType, class _SourceType> struct AssignImpl {
            typedef _SourceType SourceType;
            static void invoke(_JsonType &c, SourceType arg);
        };

        template <class _JsonType, class _Integer> struct AssignFromIntegerImpl;
        template <class _JsonType, class _Float> struct AssignFromFloatImpl;
        template <class _JsonType, class _String> struct AssignFromStringImpl;
        template <class _JsonType, class _Array> struct AssignFromArrayImpl;
        template <class _JsonType, class _Map> struct AssignFromMapImpl;

        template <class _JsonType, class _TargetType> struct AsImpl {
            typedef _TargetType TargetType;
            static TargetType invoke(const _JsonType &c);
        };

        template <class _JsonType, class _Integer> struct AsIntegerImpl;
        template <class _JsonType, class _Float> struct AsFloatImpl;
        template <class _JsonType, class _String> struct AsStringImpl;
        template <class _JsonType, class _Array> struct AsArrayImpl;
        template <class _JsonType, class _Map> struct AsMapImpl;
    }

    template <class _Traits, class _Alloc>
    class BasicJSON {
    public:
        friend class iterator;

        enum class ValueType {
            Null, False, True, Integer, Float, String, Array, Object
        };

        // 开始作死 →_→
        typedef std::basic_string<char, _Traits, typename _Alloc::template rebind<char>::other> StringType;

    private:
        ValueType _type;  // The type of the item, as above.
        int64_t _valueInt64;  // The item's number, if type==Integer
        double _valueDouble;  // The item's number, if type==Float
        StringType _valueString;  // The item's string, if type==String

        StringType _key;  // The item's name string, if this item is the child of, or is in the list of subitems of an object.
        BasicJSON<_Traits, _Alloc> *_child;  // An array or object item will have a child pointer pointing to a chain of the items in the array/object.
        BasicJSON<_Traits, _Alloc> *_next;  // next/prev allow you to walk array/object chains.
        BasicJSON<_Traits, _Alloc> *_prev;

        // 原本cJSON的实现是用的双向非循环键表，
        // 这里为了实现迭代器，增加一个头结点，用_child指向它，将头结点的_valueInt64用来表示链表结点数，
        // 改成了循环键表

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
        BasicJSON<_Traits, _Alloc>() {
            reset();
        }

        ~BasicJSON<_Traits, _Alloc>() {
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
                for (BasicJSON<_Traits, _Alloc> *p = _child->_next; p != _child; ) {
                    BasicJSON<_Traits, _Alloc> *q = p->_next;
                    Delete(p);
                    p = q;
                }
                Delete(_child);
                _child = nullptr;
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
        explicit BasicJSON<_Traits, _Alloc>(_T &&val) {
            reset();
            __cpp_basic_json_impl::AssignImpl<BasicJSON<_Traits, _Alloc>,
                typename std::remove_cv<typename std::remove_reference<_T>::type>::type>::invoke(*this, std::forward<_T>(val));
        }

        template <class _T>
        explicit BasicJSON<_Traits, _Alloc>(const std::initializer_list<_T> &il) {
            reset();
            __cpp_basic_json_impl::AssignImpl<_Traits, _Alloc, std::initializer_list<_T> >::invoke(*this, il);
        }

        template <class _T>
        explicit BasicJSON<_Traits, _Alloc>(std::initializer_list<_T> &&il) {
            reset();
            __cpp_basic_json_impl::AssignImpl<BasicJSON<_Traits, _Alloc>, std::initializer_list<_T> >::invoke(*this, il);
        }

        // 复制构造
        BasicJSON<_Traits, _Alloc>(const BasicJSON<_Traits, _Alloc> &other) {
            reset();
            Duplicate(*this, other, true);
        }

        // 移动构造
        BasicJSON<_Traits, _Alloc>(BasicJSON<_Traits, _Alloc> &&other) {
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
        BasicJSON<_Traits, _Alloc> &operator=(const BasicJSON<_Traits, _Alloc> &other) {
            clear();
            Duplicate(*this, other, true);
            return *this;
        }

        // 移动赋值
        BasicJSON<_Traits, _Alloc> &operator=(BasicJSON<_Traits, _Alloc> &&other) {
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
        BasicJSON<_Traits, _Alloc> &operator=(std::nullptr_t) {
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
            return __cpp_basic_json_impl::AsImpl<BasicJSON<_Traits, _Alloc>, _T>::invoke(*this);
        }

        bool empty() const {
            assert(_type == ValueType::Array || _type == ValueType::Object);
            return (_child->_next == _child);
        }

    public:
        // 迭代器相关
        class iterator {
            friend class BasicJSON<_Traits, _Alloc>;
            friend class const_iterator;
            BasicJSON<_Traits, _Alloc> *_ptr;

            iterator(BasicJSON<_Traits, _Alloc> *ptr) throw() : _ptr(ptr) { }

        public:
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef BasicJSON<_Traits, _Alloc> value_type;
            typedef ptrdiff_t difference_type;
            typedef difference_type distance_type;
            typedef value_type *pointer;
            typedef value_type &reference;

            iterator() throw() : _ptr(nullptr) { }
            iterator(const iterator &other) throw() : _ptr(other._ptr) { }

            iterator &operator=(const iterator &other) throw() {
                _ptr = other._ptr;
                return *this;
            }

            inline reference operator*() throw() { return *_ptr; }
            inline const reference operator*() const throw() { return *_ptr; }
            inline pointer operator->() throw() { return _ptr; }
            inline const pointer operator->() const throw() { return _ptr; }

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

            inline bool operator==(const iterator &other) const throw() { return _ptr == other._ptr; }
            inline bool operator!=(const iterator &other) const throw() { return _ptr != other._ptr; }
        };

        typedef std::reverse_iterator<iterator> reverse_iterator;

        class const_iterator {
            friend class BasicJSON<_Traits, _Alloc>;
            BasicJSON<_Traits, _Alloc> *_ptr;

            const_iterator(BasicJSON<_Traits, _Alloc> *ptr) throw() : _ptr(ptr) {
            }

        public:
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef const BasicJSON<_Traits, _Alloc> value_type;
            typedef ptrdiff_t difference_type;
            typedef difference_type distance_type;
            typedef value_type *pointer;
            typedef value_type &reference;

            const_iterator() throw() : _ptr(nullptr) { }
            const_iterator(const const_iterator &other) throw() : _ptr(other._ptr) { }
            const_iterator(const iterator &other) throw() : _ptr(other._ptr) { }

            const_iterator &operator=(const const_iterator &other) throw() {
                _ptr = other._ptr;
                return *this;
            }

            inline reference &operator*() const throw() { return *_ptr; }
            inline pointer *operator->() const throw() { return _ptr; }

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

            inline bool operator==(const const_iterator &other) const throw() { return _ptr == other._ptr; }
            inline bool operator!=(const const_iterator &other) const throw() { return _ptr != other._ptr; }
        };

        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        iterator begin() {
            return iterator(_child->_next);
        }

        const_iterator begin() const {
            return const_iterator(_child->_next);
        }

        const_iterator cbegin() const {
            return const_iterator(_child->_next);
        }

        iterator end() {
            return iterator(_child);
        }

        const_iterator end() const {
            return const_iterator(_child);
        }

        const_iterator cend() const {
            return const_iterator(_child);
        }

        reverse_iterator rbegin() {
            return reverse_iterator(_child);
        }

        const_reverse_iterator rbegin() const {
            return const_reverse_iterator(_child);
        }

        const_reverse_iterator crbegin() const {
            return const_reverse_iterator(_child);
        }

        reverse_iterator rend() {
            return reverse_iterator(_child->_next);
        }

        const_reverse_iterator rend() const {
            return const_reverse_iterator(_child->_next);
        }

        const_reverse_iterator crend() const {
            return const_reverse_iterator(_child->_next);
        }

        template <class, class> friend struct __cpp_basic_json_impl::AssignImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AssignFromIntegerImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AssignFromFloatImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AssignFromStringImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AssignFromArrayImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AssignFromMapImpl;

        template <class, class> friend struct __cpp_basic_json_impl::AsImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AsIntegerImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AsFloatImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AsStringImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AsArrayImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AsMapImpl;

    private:
        const char *ep = nullptr;
        //typename _Alloc::template rebind<BasicJSON<_Traits, _Alloc>>::other _allocator;

        static inline BasicJSON<_Traits, _Alloc> *New() {
            typedef typename _Alloc::template rebind<BasicJSON<_Traits, _Alloc>>::other AllocatorType;
            AllocatorType allocator;
            typename AllocatorType::pointer p = allocator.allocate(sizeof(BasicJSON<_Traits, _Alloc>));
            allocator.construct(p);
            return (BasicJSON<_Traits, _Alloc> *)p;
        }

        static inline void Delete(BasicJSON<_Traits, _Alloc> *c) {
            typedef typename _Alloc::template rebind<BasicJSON<_Traits, _Alloc>>::other AllocatorType;
            AllocatorType allocator;
            allocator.destroy(c);
            allocator.deallocate(c, sizeof(BasicJSON<_Traits, _Alloc>));
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
            BasicJSON<_Traits, _Alloc> *child;
            if (*value != '[')  { ep = value; return nullptr; } // not an array!

            value = skip(value + 1);
            if (*value == ']') return value + 1;    // empty array.

            _type = ValueType::Array;
            this->_child = New();
            if (this->_child == nullptr) return nullptr;        // memory fail
            this->_child->_next = child = New();
            if (child == nullptr) return nullptr;        // memory fail
            value = skip(child->parse_value(skip(value)));  // skip any spacing, get the value.
            if (value == nullptr) return nullptr;
            child->_next = this->_child;
            child->_prev = this->_child;
            ++this->_child->_valueInt64;

            while (*value == ',') {
                BasicJSON<_Traits, _Alloc> *new_item = New();
                if (new_item == nullptr) return nullptr;     // memory fail
                child->_next = new_item; new_item->_prev = child; child = new_item;
                value = skip(child->parse_value(skip(value + 1)));
                if (value == nullptr) return nullptr; // memory fail
                new_item->_next = this->_child;
                this->_child->_prev = new_item;
                ++this->_child->_valueInt64;
            }

            if (*value == ']') return value + 1;    // end of array
            ep = value; return nullptr; // malformed.
        }

        // Build an object from the text.
        const char *parse_object(const char *value) {
            if (*value != '{')  { ep = value; return nullptr; } // not an object!

            value = skip(value + 1);
            if (*value == '}') return value + 1;    // empty array.

            _type = ValueType::Object;
            BasicJSON<_Traits, _Alloc> *child;
            this->_child = New();
            if (this->_child == nullptr) return nullptr;        // memory fail
            this->_child->_next = child = New();
            if (child == nullptr) return nullptr;        // memory fail
            value = skip(child->parse_string(skip(value)));
            child->_type = ValueType::Null;
            if (value == nullptr) return nullptr;
            child->_key = std::move(child->_valueString); child->_valueString.clear();
            if (*value != ':') { ep = value; return nullptr; }  // fail!
            value = skip(child->parse_value(skip(value + 1)));  // skip any spacing, get the value.
            if (value == nullptr) return nullptr;
            child->_next = this->_child;
            child->_prev = this->_child;
            ++this->_child->_valueInt64;

            while (*value == ',') {
                BasicJSON<_Traits, _Alloc> *new_item = New();
                if (new_item == nullptr)   return nullptr; // memory fail
                child->_next = new_item; new_item->_prev = child; child = new_item;
                value = skip(child->parse_string(skip(value + 1)));
                child->_type = ValueType::Null;
                if (value == nullptr) return nullptr;
                child->_key = std::move(child->_valueString); child->_valueString.clear();
                if (*value != ':') { ep = value; return nullptr; }  // fail!
                value = skip(child->parse_value(skip(value + 1)));  // skip any spacing, get the value.
                if (value == nullptr) return nullptr;
                new_item->_next = this->_child;
                this->_child->_prev = new_item;
                ++this->_child->_valueInt64;
            }

            if (*value == '}') return value + 1;    // end of array
            ep = value; return nullptr; // malformed.
        }

        template <class _String>
        void print_value(_String &ret, int depth, bool fmt) const {
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

        template <class _String>
        void print_integer(_String &ret) const {
            char str[21];  // 2^64+1 can be represented in 21 chars.
            sprintf(str, "%" PRId64, _valueInt64);
            ret.append(str);
        }

        template <class _String>
        void print_float(_String &ret) const {
            char str[64];  // This is a nice tradeoff.
            double d = _valueDouble;
            if (fabs(floor(d) - d) <= DBL_EPSILON && fabs(d) < 1.0e60) sprintf(str, "%.0f", d);
            else if (fabs(d) < 1.0e-6 || fabs(d) > 1.0e9)           sprintf(str, "%e", d);
            else                                                sprintf(str, "%f", d);
            ret.append(str);
        }

        template <class _String>
        static void print_string_ptr(_String &ret, const StringType &str) {
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

        template <class _String>
        void print_string(_String &ret) const {
            print_string_ptr(ret, _valueString);
        }

        template <class _String>
        void print_array(_String &ret, int depth, bool fmt) const {
            size_t numentries = static_cast<size_t>(_child->_valueInt64);

            // Explicitly handle empty object case
            if (_child->_valueInt64 == 0) {
                ret.append("[]");
                return;
            }

            // Retrieve all the results:
            BasicJSON<_Traits, _Alloc> *child = _child;
            size_t i = 0;
            ret.append("[");
            for (child = _child->_next; child != _child; child = child->_next, ++i) {
                child->print_value(ret, depth + 1, fmt);
                if (i != numentries - 1) { ret.append(1, ','); if (fmt) ret.append(1, ' '); }
            }
            ret.append("]");
        }

        template <class _String>
        void print_object(_String &ret, int depth, bool fmt) const {
            size_t numentries = static_cast<size_t>(_child->_valueInt64);

            // Explicitly handle empty object case
            if (numentries == 0) {
                ret.append(1, '{');
                if (fmt) { ret.append(1, '\n'); if (depth > 0) ret.append(depth - 1, '\t'); }
                ret.append(1, '}');
                return;
            }

            // Compose the output:
            BasicJSON<_Traits, _Alloc> *child = _child->_next;
            ++depth;
            ret.append(1, '{'); if (fmt) ret.append(1, '\n');
            for (size_t i = 0; i < numentries; ++i) {
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

        static bool Duplicate(BasicJSON<_Traits, _Alloc> &newitem, const BasicJSON<_Traits, _Alloc> &item, bool recurse) {
            newitem.clear();
            const BasicJSON<_Traits, _Alloc> *cptr;
            BasicJSON<_Traits, _Alloc> *nptr = nullptr, *newchild;
            // Copy over all vars
            newitem._type = item._type, newitem._valueInt64 = item._valueInt64, newitem._valueDouble = item._valueDouble;
            newitem._valueString = item._valueString;
            newitem._key = item._key;
            // If non-recursive, then we're done!
            if (!recurse) return true;
            // Walk the ->next chain for the child.
            if (item._child != nullptr) {
                newitem._child = New();
                nptr = newitem._child;
                cptr = item._child->_next;
                while (cptr != item._child) {
                    newchild = New();
                    if (newchild == nullptr) return false;
                    if (!Duplicate(*newchild, *cptr, true)) { Delete(newchild); return false; }     // Duplicate (with recurse) each item in the ->next chain
                    nptr->_next = newchild, newchild->_prev = nptr; nptr = newchild;    // crosswire ->prev and ->next and move on
                    cptr = cptr->_next;
                }
                newitem._child->_prev = nptr;
                nptr->_next = newitem._child;
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
    template <class _OS, class _Traits, class _Alloc>
    static inline _OS &operator<<(_OS &os, const BasicJSON<_Traits, _Alloc> &c) {
        os << c.Print();
        return os;
    }

    // 重载与nullptr的比较
    template <class _Traits, class _Alloc>
    static inline bool operator==(std::nullptr_t, const BasicJSON<_Traits, _Alloc> &c) throw() {
        return c.operator==(nullptr);
    }

    template <class _Traits, class _Alloc>
    static inline bool operator!=(std::nullptr_t, const BasicJSON<_Traits, _Alloc> &c) {
        return c.operator!=(nullptr);
    }

    namespace __cpp_basic_json_impl {
        template <class _JsonType, class _SourceType>
        void AssignImpl<_JsonType, _SourceType>::invoke(_JsonType &c, _SourceType arg) {
            // TODO:
            //static_assert(0, "unimplemented type");
        }

        template <class _JsonType> struct AssignImpl<_JsonType, std::nullptr_t> {
            typedef std::nullptr_t SourceType;
            static inline void invoke(_JsonType &c, SourceType) {
                c._type = _JsonType::ValueType::Null;
            }
        };

        template <class _JsonType> struct AssignImpl<_JsonType, bool> {
            typedef bool SourceType;
            static inline void invoke(_JsonType &c, SourceType src) {
                c._type = src ? _JsonType::ValueType::True : _JsonType::ValueType::False;
            }
        };

        template <class _JsonType, class _INTEGER> struct AssignFromIntegerImpl {
            typedef _INTEGER SourceType;
            static inline void invoke(_JsonType &c, SourceType arg) {
                c._type = _JsonType::ValueType::Integer;
                c._valueInt64 = arg;
            }
        };

        template <class _JsonType> struct AssignImpl<_JsonType, char>
            : AssignFromIntegerImpl<_JsonType, char> { };
        template <class _JsonType> struct AssignImpl<_JsonType, signed char>
            : AssignFromIntegerImpl<_JsonType, signed char> { };
        template <class _JsonType> struct AssignImpl<_JsonType, unsigned char>
            : AssignFromIntegerImpl<_JsonType, unsigned char> { };
        template <class _JsonType> struct AssignImpl<_JsonType, short>
            : AssignFromIntegerImpl<_JsonType, short> { };
        template <class _JsonType> struct AssignImpl<_JsonType, unsigned short>
            : AssignFromIntegerImpl<_JsonType, unsigned short> { };
        template <class _JsonType> struct AssignImpl<_JsonType, int>
            : AssignFromIntegerImpl<_JsonType, int> { };
        template <class _JsonType> struct AssignImpl<_JsonType, unsigned>
            : AssignFromIntegerImpl<_JsonType, unsigned> { };
        template <class _JsonType> struct AssignImpl<_JsonType, long>
            : AssignFromIntegerImpl<_JsonType, long> { };
        template <class _JsonType> struct AssignImpl<_JsonType, unsigned long>
            : AssignFromIntegerImpl<_JsonType, unsigned long> { };
        template <class _JsonType> struct AssignImpl<_JsonType, int64_t>
            : AssignFromIntegerImpl<_JsonType, int64_t> { };
        template <class _JsonType> struct AssignImpl<_JsonType, uint64_t>
            : AssignFromIntegerImpl<_JsonType, uint64_t> { };

        template <class _JsonType, class _Float> struct AssignFromFloatImpl {
            typedef _Float SourceType;
            static inline void invoke(_JsonType &c, SourceType arg) {
                c._type = _JsonType::ValueType::Float;
                c._valueDouble = arg;
            }
        };

        template <class _JsonType> struct AssignImpl<_JsonType, float>
            : AssignFromFloatImpl<_JsonType, float> { };
        template <class _JsonType> struct AssignImpl<_JsonType, double>
            : AssignFromFloatImpl<_JsonType, double> { };

        static inline const char *_ConvertString(const char *str) {
            return str;
        }

        template <class _Traits, class _Alloc>
        static inline const char *_ConvertString(const std::basic_string<char, _Traits, _Alloc> &str) {
            return str.c_str();
        }

        template <class _JsonType, class _String> struct AssignFromStringImpl {
            typedef _String SourceType;
            static inline void invoke(_JsonType &c, const SourceType &arg) {
                c._type = _JsonType::ValueType::String;
                c._valueString = _ConvertString(arg);
            }
        };

        template <class _JsonType, size_t _N> struct AssignImpl<_JsonType, char [_N]>
            : AssignFromStringImpl<_JsonType, char [_N]> { };

        template <class _JsonType> struct AssignImpl<_JsonType, char *>
            : AssignFromStringImpl<_JsonType, char *> { };

        template <class _JsonType> struct AssignImpl<_JsonType, const char *>
            : AssignFromStringImpl<_JsonType, const char *> { };

        template <class _JsonType, class _Traits, class _Alloc>
        struct AssignImpl<_JsonType, std::basic_string<char, _Traits, _Alloc> >
            : AssignFromStringImpl<_JsonType, std::basic_string<char, _Traits, _Alloc> > { };

        //template <class _JsonType>
        //struct AssignImpl<_JsonType, typename _JsonType::StringType> {
        //    typedef typename _JsonType::StringType SourceType;
        //    static inline void invoke(_JsonType &c, const SourceType &arg) {
        //        c._type = _JsonType::ValueType::String;
        //        c._valueString = arg;
        //    }
        //    static inline void invoke(_JsonType &c, SourceType &&arg) {
        //        c._type = _JsonType::ValueType::String;
        //        c._valueString = std::move(arg);
        //    }
        //};

        template <class _JsonType, class _Elem, size_t _N>
        struct AssignImpl<_JsonType, _Elem [_N]> {
            typedef _Elem SourceType[_N];
            static void invoke(_JsonType &c, const SourceType &arg) {
                c._type = _JsonType::ValueType::Array;
                c._child = _JsonType::New();
                _JsonType *prev = c._child;
                prev->_next = prev->_prev = prev;
                for (size_t i = 0; i < _N; ++i) {
                    _JsonType *item = _JsonType::New();
                    AssignImpl<_JsonType, _Elem>::invoke(*item, arg[i]);
                    prev->_next = item;
                    item->_prev = prev;
                    item->_next = c._child;
                    c._child->_prev = item;
                    ++c._child->_valueInt64;
                    prev = item;
                }
            }
            static void invoke(_JsonType &c, SourceType &&arg) {
                c._type = _JsonType::ValueType::Array;
                c._child = _JsonType::New();
                _JsonType *prev = c._child;
                prev->_next = prev->_prev = prev;
                for (size_t i = 0; i < _N; ++i) {
                    _JsonType *item = _JsonType::New();
                    AssignImpl<_JsonType, _Elem>::invoke(*item, std::move(arg[i]));
                    prev->_next = item;
                    item->_prev = prev;
                    item->_next = c._child;
                    c._child->_prev = item;
                    ++c._child->_valueInt64;
                    prev = item;
                }
            }
        };

        template <class _JsonType, class _Array>
        struct AssignFromArrayImpl {
            typedef _Array SourceType;
            static void invoke(_JsonType &c, const SourceType &arg) {
                c._type = _JsonType::ValueType::Array;
                c._child = _JsonType::New();
                _JsonType *prev = c._child;
                prev->_next = prev->_prev = prev;
                for (typename SourceType::const_iterator it = arg.begin(); it != arg.end(); ++it) {
                    _JsonType *item = _JsonType::New();
                    AssignImpl<_JsonType, typename SourceType::value_type>::invoke(*item, *it);
                    prev->_next = item;
                    item->_prev = prev;
                    item->_next = c._child;
                    c._child->_prev = item;
                    ++c._child->_valueInt64;
                    prev = item;
                }
            }
            static void invoke(_JsonType &c, SourceType &&arg) {
                c._type = _JsonType::ValueType::Array;
                c._child = _JsonType::New();
                _JsonType *prev = c._child;
                prev->_next = prev->_prev = prev;
                for (typename SourceType::iterator it = arg.begin(); it != arg.end(); ++it) {
                    _JsonType *item = _JsonType::New();
                    AssignImpl<_JsonType, typename SourceType::value_type>::invoke(*item, std::move(*it));
                    prev->_next = item;
                    item->_prev = prev;
                    item->_next = c._child;
                    c._child->_prev = item;
                    ++c._child->_valueInt64;
                    prev = item;
                }
            }
        };

        template <class _JsonType, class _T, class _Alloc>
        struct AssignImpl<_JsonType, std::vector<_T, _Alloc> >
            : AssignFromArrayImpl<_JsonType, std::vector<_T, _Alloc> > {
        };

        template <class _JsonType, class _T, class _Alloc>
        struct AssignImpl<_JsonType, std::list<_T, _Alloc> >
            : AssignFromArrayImpl<_JsonType, std::list<_T, _Alloc> > {
        };

        template <class _JsonType, class _T, class _Compare, class _Alloc>
        struct AssignImpl<_JsonType, std::set<_T, _Compare, _Alloc> >
            : AssignFromArrayImpl<_JsonType, std::set<_T, _Compare, _Alloc> > {
        };

        template <class _JsonType, class _T, class _Compare, class _Alloc>
        struct AssignImpl<_JsonType, std::multiset<_T, _Compare, _Alloc> >
            : AssignFromArrayImpl<_JsonType, std::multiset<_T, _Compare, _Alloc> > {
        };

        template <class _JsonType, class _T, class _Hash, class _Pred, class _Alloc>
        struct AssignImpl<_JsonType, std::unordered_set<_T, _Hash, _Pred, _Alloc> >
            : AssignFromArrayImpl<_JsonType, std::unordered_set<_T, _Hash, _Pred, _Alloc> > {
        };

        template <class _JsonType, class _T, class _Hash, class _Pred, class _Alloc>
        struct AssignImpl<_JsonType, std::unordered_multiset<_T, _Hash, _Pred, _Alloc> >
            : AssignFromArrayImpl<_JsonType, std::unordered_multiset<_T, _Hash, _Pred, _Alloc> > {
        };

        template <class _JsonType, class _Map>
        struct AssignFromMapImpl {
            typedef _Map SourceType;
            static void invoke(_JsonType &c, const SourceType &arg) {
                static_assert(std::is_convertible<const char *, typename SourceType::key_type>::value, "key_type must be able to convert to const char *");
                c._type = _JsonType::ValueType::Object;
                c._child = _JsonType::New();
                _JsonType *prev = c._child;
                prev->_next = prev->_prev = prev;
                for (typename SourceType::const_iterator it = arg.begin(); it != arg.end(); ++it) {
                    _JsonType *item = _JsonType::New();
                    item->_key = _ConvertString(it->first);
                    AssignImpl<_JsonType, typename SourceType::mapped_type>::invoke(*item, it->second);
                    prev->_next = item;
                    item->_prev = prev;
                    item->_next = c._child;
                    c._child->_prev = item;
                    ++c._child->_valueInt64;
                    prev = item;
                }
            }

            static void invoke(_JsonType &c, SourceType &&arg) {
                static_assert(std::is_convertible<const char *, typename SourceType::key_type>::value, "key_type must be able to convert to const char *");
                c._type = _JsonType::ValueType::Object;
                c._child = _JsonType::New();
                _JsonType *prev = c._child;
                prev->_next = prev->_prev = prev;
                for (typename SourceType::iterator it = arg.begin(); it != arg.end(); ++it) {
                    _JsonType *item = _JsonType::New();
                    item->_key = _ConvertString(it->first);
                    AssignImpl<_JsonType, typename SourceType::mapped_type>::invoke(*item, std::move(it->second));
                    prev->_next = item;
                    item->_prev = prev;
                    item->_next = c._child;
                    c._child->_prev = item;
                    ++c._child->_valueInt64;
                    prev = item;
                }
            }
        };

        template <class _JsonType, class _Key, class _Val, class _Compare, class _Alloc>
        struct AssignImpl<_JsonType, std::map<_Key, _Val, _Compare, _Alloc> >
            : AssignFromMapImpl<_JsonType, std::map<_Key, _Val, _Compare, _Alloc> > {
        };

        template <class _JsonType, class _Key, class _Val, class _Compare, class _Alloc>
        struct AssignImpl<_JsonType, std::multimap<_Key, _Val, _Compare, _Alloc> >
            : AssignFromMapImpl<_JsonType, std::multimap<_Key, _Val, _Compare, _Alloc> > {
        };

        template <class _JsonType, class _Key, class _Val, class _Hash, class _Pred, class _Alloc>
        struct AssignImpl<_JsonType, std::unordered_map<_Key, _Val, _Hash, _Pred, _Alloc> >
            : AssignFromMapImpl<_JsonType, std::unordered_map<_Key, _Val, _Hash, _Pred, _Alloc> > {
        };

        template <class _JsonType, class _Key, class _Val, class _Hash, class _Pred, class _Alloc>
        struct AssignImpl<_JsonType, std::unordered_multimap<_Key, _Val, _Hash, _Pred, _Alloc> >
            : AssignFromMapImpl<_JsonType, std::unordered_multimap<_Key, _Val, _Hash, _Pred, _Alloc> > {
        };

        template <class _JsonType, class _T>
        struct AssignImpl<_JsonType, std::initializer_list<_T> > {
            typedef std::initializer_list<_T> SourceType;
            static void invoke(_JsonType &c, const SourceType &arg) {
                // TODO:
            }
        };

        template <class _JsonType, class _TargetType>
        _TargetType AsImpl<_JsonType, _TargetType>::invoke(const _JsonType &c) {
            std::basic_stringstream<char, typename _JsonType::StringType::value_type, typename _JsonType::StringType::allocator_type> ss;
            switch (c._type) {
            case _JsonType::ValueType::Null: break;
            case _JsonType::ValueType::False: ss << 0; break;
            case _JsonType::ValueType::True: ss << 1; break;
            case _JsonType::ValueType::Integer: ss << c._valueInt64; break;
            case _JsonType::ValueType::Float: ss << c._valueDouble; break;
            case _JsonType::ValueType::String: ss << c._valueString; break;
            case _JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to target type"); break;
            case _JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to target type"); break;
            default: throw std::out_of_range("JSON type out of range"); break;
            }
            TargetType ret = TargetType();
            // TODO：
            //ss >> ret;
            return ret;
        }

        //template <class _JsonType>
        //struct AsImpl<_JsonType, const _JsonType *> {
        //    typedef const _JsonType *TargetType;
        //    static TargetType invoke(const _JsonType &c) {
        //        return &c;
        //    }
        //};

        template <class _JsonType, class _Integer>
        struct AsIntegerImpl {
            typedef _Integer TargetType;
            static TargetType invoke(const _JsonType &c) {
                switch (c._type) {
                case _JsonType::ValueType::Null: return TargetType(0);
                case _JsonType::ValueType::False: return TargetType(0);
                case _JsonType::ValueType::True: return TargetType(1);
                case _JsonType::ValueType::Integer: return static_cast<TargetType>(c._valueInt64);
                case _JsonType::ValueType::Float: return static_cast<TargetType>(c._valueDouble);
                case _JsonType::ValueType::String: return static_cast<TargetType>(atoll(c._valueString.c_str()));
                case _JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Integer"); break;
                case _JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Integer"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, char>
            : AsIntegerImpl<_JsonType, char> {
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, signed char>
            : AsIntegerImpl<_JsonType, signed char> {
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, unsigned char>
            : AsIntegerImpl<_JsonType, unsigned char> {
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, short>
            : AsIntegerImpl<_JsonType, short> {
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, unsigned short>
            : AsIntegerImpl<_JsonType, unsigned short> {
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, int>
            : AsIntegerImpl<_JsonType, int> {
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, unsigned>
            : AsIntegerImpl<_JsonType, unsigned> {
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, long>
            : AsIntegerImpl<_JsonType, long> {
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, unsigned long>
            : AsIntegerImpl<_JsonType, unsigned long> {
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, int64_t>
            : AsIntegerImpl<_JsonType, int64_t> {
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, uint64_t>
            : AsIntegerImpl<_JsonType, uint64_t> {
        };

        template <class _JsonType, class _Float>
        struct AsFloatImpl {
            typedef _Float TargetType;
            static TargetType invoke(const _JsonType &c) {
                switch (c._type) {
                case _JsonType::ValueType::Null: return TargetType(0);
                case _JsonType::ValueType::False: return TargetType(0);
                case _JsonType::ValueType::True: return TargetType(1);
                case _JsonType::ValueType::Integer: return static_cast<TargetType>(c._valueInt64);
                case _JsonType::ValueType::Float: return static_cast<TargetType>(c._valueDouble);
                case _JsonType::ValueType::String: return static_cast<TargetType>(atof(c._valueString.c_str()));
                case _JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Float"); break;
                case _JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Float"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, float>
            : AsFloatImpl<_JsonType, float> {
        };

        template <class _JsonType>
        struct AsImpl<_JsonType, double>
            : AsFloatImpl<_JsonType, double> {
        };

        template <class _JsonType, class _String>
        struct AsStringImpl {
            typedef _String TargetType;
            static TargetType invoke(const _JsonType &c) {
                switch (c._type) {
                case _JsonType::ValueType::Null: return TargetType();
                case _JsonType::ValueType::False: return TargetType("false");
                case _JsonType::ValueType::True: return TargetType("true");
                case _JsonType::ValueType::Integer: {
                    char str[21];  // 2^64+1 can be represented in 21 chars.
                    sprintf(str, "%" PRId64, c._valueInt64);
                    return TargetType(str);
                }
                case _JsonType::ValueType::Float: {
                    std::basic_ostringstream<typename _String::value_type, typename _String::traits_type, typename _String::allocator_type> ss;
                    ss << c._valueDouble;
                    return ss.str();
                }
                case _JsonType::ValueType::String: return TargetType(c._valueString.begin(), c._valueString.end());
                case _JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to String"); break;
                case _JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to String"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        template <class _JsonType, class _Char, class _Traits, class _Alloc>
        struct AsImpl<_JsonType, std::basic_string<_Char, _Traits, _Alloc> >
            : AsStringImpl<_JsonType, std::basic_string<_Char, _Traits, _Alloc> > {
        };

        template <class _JsonType, class _Array>
        struct AsArrayImpl {
            typedef _Array TargetType;
            static TargetType invoke(const _JsonType &c) {
                switch (c._type) {
                case _JsonType::ValueType::Null: return TargetType();
                case _JsonType::ValueType::False: throw std::logic_error("Cannot convert JSON_False to Array"); break;
                case _JsonType::ValueType::True: throw std::logic_error("Cannot convert JSON_True to Array"); break;
                case _JsonType::ValueType::Integer: throw std::logic_error("Cannot convert JSON_Integer to Array"); break;
                case _JsonType::ValueType::Float: throw std::logic_error("Cannot convert JSON_Float to Array"); break;
                case _JsonType::ValueType::String: throw std::logic_error("Cannot convert JSON_String to Array"); break;
                case _JsonType::ValueType::Array: {
                    TargetType ret = TargetType();
                    std::transform(c.begin(), c.end(), std::inserter(ret, ret.begin()),
                        &AsImpl<_JsonType, typename TargetType::value_type>::invoke);
                    return ret;
                }
                case _JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Array"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        template <class _JsonType, class _T, class _Alloc>
        struct AsImpl<_JsonType, std::vector<_T, _Alloc> >
            : AsArrayImpl<_JsonType, std::vector<_T, _Alloc> > {
        };

        template <class _JsonType, class _T, class _Alloc>
        struct AsImpl<_JsonType, std::list<_T, _Alloc> >
            : AsArrayImpl<_JsonType, std::list<_T, _Alloc> > {
        };

        template <class _JsonType, class _T, class _Compare, class _Alloc>
        struct AsImpl<_JsonType, std::set<_T, _Compare, _Alloc> >
            : AsArrayImpl<_JsonType, std::set<_T, _Compare, _Alloc> > {
        };

        template <class _JsonType, class _T, class _Compare, class _Alloc>
        struct AsImpl<_JsonType, std::multiset<_T, _Compare, _Alloc> >
            : AsArrayImpl<_JsonType, std::multiset<_T, _Compare, _Alloc> > {
        };

        template <class _JsonType, class _T, class _Hash, class _Pred, class _Alloc>
        struct AsImpl<_JsonType, std::unordered_set<_T, _Hash, _Pred, _Alloc> >
            : AsArrayImpl<_JsonType, std::unordered_set<_T, _Hash, _Pred, _Alloc> > {
        };

        template <class _JsonType, class _T, class _Hash, class _Pred, class _Alloc>
        struct AsImpl<_JsonType, std::unordered_multiset<_T, _Hash, _Pred, _Alloc> >
            : AsArrayImpl<_JsonType, std::unordered_multiset<_T, _Hash, _Pred, _Alloc> > {
        };

        template <class _JsonType, class _Map>
        struct AsMapImpl {
            typedef _Map TargetType;
            static TargetType invoke(const _JsonType &c) {
                switch (c._type) {
                case _JsonType::ValueType::Null: return TargetType();
                case _JsonType::ValueType::False: throw std::logic_error("Cannot convert JSON_False to Object"); break;
                case _JsonType::ValueType::True: throw std::logic_error("Cannot convert JSON_True to Object"); break;
                case _JsonType::ValueType::Integer: throw std::logic_error("Cannot convert JSON_Integer to Object"); break;
                case _JsonType::ValueType::Float: throw std::logic_error("Cannot convert JSON_Float to Object"); break;
                case _JsonType::ValueType::String: throw std::logic_error("Cannot convert JSON_String to Object"); break;
                case _JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Object"); break;
                case _JsonType::ValueType::Object: {
                    TargetType ret = TargetType();
                    std::transform(c.begin(), c.end(), std::inserter(ret, ret.begin()), &_make_value);
                    return ret;
                }
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        private:
            static inline typename TargetType::value_type _make_value(const _JsonType &j) {
                return typename TargetType::value_type(typename TargetType::key_type(j._key.begin(), j._key.end()),
                    AsImpl<_JsonType, typename TargetType::mapped_type>::invoke(j));
            }
        };

        template <class _JsonType, class _String, class _Val, class _Compare, class _Alloc>
        struct AsImpl<_JsonType, std::map<_String, _Val, _Compare, _Alloc> >
            : AsMapImpl<_JsonType, std::map<_String, _Val, _Compare, _Alloc> > {
        };

        template <class _JsonType, class _String, class _Val, class _Compare, class _Alloc>
        struct AsImpl<_JsonType, std::multimap<_String, _Val, _Compare, _Alloc> >
            : AsMapImpl<_JsonType, std::multimap<_String, _Val, _Compare, _Alloc> > {
        };

        template <class _JsonType, class _String, class _Val, class _Hash, class _Pred, class _Alloc>
        struct AsImpl<_JsonType, std::unordered_map<_String, _Val, _Hash, _Pred, _Alloc> >
            : AsMapImpl<_JsonType, std::unordered_map<_String, _Val, _Hash, _Pred, _Alloc> > {
        };

        template <class _JsonType, class _String, class _Val, class _Hash, class _Pred, class _Alloc>
        struct AsImpl<_JsonType, std::unordered_multimap<_String, _Val, _Hash, _Pred, _Alloc> >
            : AsMapImpl<_JsonType, std::unordered_multimap<_String, _Val, _Hash, _Pred, _Alloc> > {
        };
    }

    typedef BasicJSON<std::char_traits<char>, std::allocator<char> > cppJSON;
}

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

#endif
