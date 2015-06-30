#ifndef _JSON_VALUE_H_
#define _JSON_VALUE_H_

#include "cJSON.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

namespace jw {

    struct cJSON_Exception : public std::exception {
        virtual const char *what() const throw () override {
            const char *e = cJSON_GetErrorPtr();
            return e != nullptr ? e : "";
        }
    };

    class JSON_Value;

    namespace __json_impl {

        template <class _T> struct ArgType {
            typedef typename std::remove_const<typename std::remove_reference<_T>::type>::type type;
        };

        // --JsonValueConstructorImpl

        // 默认
        template <class _T>
        struct JsonValueConstructorImpl {
            static cJSON *invoke(const _T &) {
                static_assert(0, "unimplemented type");
                return nullptr;
            }
        };

        // 基本类型

        // char是个古怪东西：
        // std::is_same<char, signed char>::value是false，
        // std::is_same<char, unsigned char>::value也是false
        template <>
        struct JsonValueConstructorImpl<char> {
            static cJSON *invoke(char num) {
                return cJSON_CreateInt(num);
            }
        };

        template <>
        struct JsonValueConstructorImpl<signed char> {
            static cJSON *invoke(signed char num) {
                return cJSON_CreateInt(num);
            }
        };

        template <>
        struct JsonValueConstructorImpl<unsigned char> {
            static cJSON *invoke(unsigned char num) {
                return cJSON_CreateInt(num);
            }
        };

        template <>
        struct JsonValueConstructorImpl<short> {
            static cJSON *invoke(short num) {
                return cJSON_CreateInt(num);
            }
        };

        template <>
        struct JsonValueConstructorImpl<unsigned short> {
            static cJSON *invoke(unsigned short num) {
                return cJSON_CreateInt(num);
            }
        };

        template <>
        struct JsonValueConstructorImpl<int> {
            static cJSON *invoke(int num) {
                return cJSON_CreateInt(num);
            }
        };

        template <>
        struct JsonValueConstructorImpl<unsigned> {
            static cJSON *invoke(unsigned num) {
                return cJSON_CreateInt(num);
            }
        };

        template <>
        struct JsonValueConstructorImpl<long long> {
            static cJSON *invoke(long long num) {
                return cJSON_CreateInt64(num);
            }
        };

        template <>
        struct JsonValueConstructorImpl<unsigned long long> {
            static cJSON *invoke(unsigned long long num) {
                return cJSON_CreateInt64(num);
            }
        };

        template <>
        struct JsonValueConstructorImpl<float> {
            static cJSON *invoke(float num) {
                return cJSON_CreateNumber(num);
            }
        };

        template <>
        struct JsonValueConstructorImpl<double> {
            static cJSON *invoke(double num) {
                return cJSON_CreateNumber(num);
            }
        };

        template <>
        struct JsonValueConstructorImpl<bool> {
            static cJSON *invoke(bool value) {
                return value ? cJSON_CreateTrue() : cJSON_CreateFalse();
            }
        };

        // 空
        template <>
        struct JsonValueConstructorImpl<std::nullptr_t> {
            static cJSON *invoke(std::nullptr_t) {
                return cJSON_CreateNull();
            }
        };

        // 字符串
        template <>
        struct JsonValueConstructorImpl<char *> {
            static cJSON *invoke(const char *str) {
                return cJSON_CreateString(str);
            }
        };

        template <>
        struct JsonValueConstructorImpl<const char *> {
            static cJSON *invoke(const char *str) {
                return cJSON_CreateString(str);
            }
        };

        template <size_t _N>
        struct JsonValueConstructorImpl<char [_N]> {
            static cJSON *invoke(const char (&str)[_N]) {
                return cJSON_CreateString(str);
            }
        };

        template <class _TRAITS, class _ALLOC>
        struct JsonValueConstructorImpl<std::basic_string<char, _TRAITS, _ALLOC> > {
            static cJSON *invoke(const std::basic_string<char, _TRAITS, _ALLOC> &str) {
                return cJSON_CreateString(str.c_str());
            }
        };

        // 数组
        template <size_t _N, class _T>
        struct JsonValueConstructorImpl<_T [_N]> {
            static cJSON *invoke(const _T (&arr)[_N]) {
                cJSON *ret = cJSON_CreateArray();
                if (_N > 0) {
                    cJSON *item = JsonValueConstructorImpl<_T>::invoke(arr[0]);
                    ret->child = item;
                    cJSON *prev = item;
                    for (size_t i = 1; i < _N; ++i) {
                        cJSON *item = JsonValueConstructorImpl<_T>::invoke(arr[i]);
                        prev->next = item;
                        item->prev = prev;
                        prev = item;
                    }
                }
                return ret;
            }
        };

        template <class _T, class _ALLOC>
        struct JsonValueConstructorImpl<std::vector<_T, _ALLOC> > {
            static cJSON *invoke(const std::vector<_T, _ALLOC> &v) {
                cJSON *ret = cJSON_CreateArray();
                if (!v.empty()) {
                    cJSON *item = JsonValueConstructorImpl<_T>::invoke(v.at(0));
                    ret->child = item;
                    cJSON *prev = item;
                    for (typename std::vector<_T, _ALLOC>::size_type i = 1, cnt = v.size(); i < cnt; ++i) {
                        cJSON *item = JsonValueConstructorImpl<_T>::invoke(v.at(i));
                        prev->next = item;
                        item->prev = prev;
                        prev = item;
                    }
                }
                return ret;
            }
        };

        template <class _T, class _ALLOC>
        struct JsonValueConstructorImpl<std::list<_T, _ALLOC> > {
            static cJSON *invoke(const std::list<_T, _ALLOC> &l) {
                cJSON *ret = cJSON_CreateArray();
                cJSON *prev = nullptr;
                for (typename std::list<_T, _ALLOC>::const_iterator it = l.begin(); it != l.end(); ++it) {
                    cJSON *item = JsonValueConstructorImpl<_T>::invoke(*it);
                    if (it != l.begin()) {
                        prev->next = item;
                        item->prev = prev;
                    } else {
                        ret->child = item;
                    }
                    prev = item;
                }
                return ret;
            }
        };

        template <class _T, class _P, class _ALLOC>
        struct JsonValueConstructorImpl<std::set<_T, _P, _ALLOC> > {
            static cJSON *invoke(const std::set<_T, _P, _ALLOC> &s) {
                cJSON *ret = cJSON_CreateArray();
                cJSON *prev = nullptr;
                for (typename std::set<_T, _P, _ALLOC>::const_iterator it = s.begin(); it != s.end(); ++it) {
                    cJSON *item = JsonValueConstructorImpl<_T>::invoke(*it);
                    if (it != s.begin()) {
                        prev->next = item;
                        item->prev = prev;
                    } else {
                        ret->child = item;
                    }
                    prev = item;
                }
                return ret;
            }
        };

        template <class _T, class _P, class _ALLOC>
        struct JsonValueConstructorImpl<std::multiset<_T, _P, _ALLOC> > {
            static cJSON *invoke(const std::multiset<_T, _P, _ALLOC> &s) {
                cJSON *ret = cJSON_CreateArray();
                cJSON *prev = nullptr;
                for (typename std::multiset<_T, _P, _ALLOC>::const_iterator it = s.begin(); it != s.end(); ++it) {
                    cJSON *item = JsonValueConstructorImpl<_T>::invoke(*it);
                    if (it != s.begin()) {
                        prev->next = item;
                        item->prev = prev;
                    } else {
                        ret->child = item;
                    }
                    prev = item;
                }
                return ret;
            }
        };

        template <class _T, class _HASHER, class _EQ, class _ALLOC>
        struct JsonValueConstructorImpl<std::unordered_set<_T, _HASHER, _EQ, _ALLOC> > {
            static cJSON *invoke(const std::unordered_set<_T, _HASHER, _EQ, _ALLOC> &s) {
                cJSON *ret = cJSON_CreateArray();
                cJSON *prev = nullptr;
                for (typename std::unordered_set<_T, _HASHER, _EQ, _ALLOC>::const_iterator it = s.begin(); it != s.end(); ++it) {
                    cJSON *item = JsonValueConstructorImpl<_T>::invoke(*it);
                    if (it != s.begin()) {
                        prev->next = item;
                        item->prev = prev;
                    } else {
                        ret->child = item;
                    }
                    prev = item;
                }
                return ret;
            }
        };

        template <class _T, class _HASHER, class _EQ, class _ALLOC>
        struct JsonValueConstructorImpl<std::unordered_multiset<_T, _HASHER, _EQ, _ALLOC> > {
            static cJSON *invoke(const std::unordered_multiset<_T, _HASHER, _EQ, _ALLOC> &s) {
                cJSON *ret = cJSON_CreateArray();
                cJSON *prev = nullptr;
                for (typename std::unordered_multiset<_T, _HASHER, _EQ, _ALLOC>::const_iterator it = s.begin(); it != s.end(); ++it) {
                    cJSON *item = JsonValueConstructorImpl<_T>::invoke(*it);
                    if (it != s.begin()) {
                        prev->next = item;
                        item->prev = prev;
                    } else {
                        ret->child = item;
                    }
                    prev = item;
                }
                return ret;
            }
        };

        template <class _T>
        struct JsonValueConstructorImpl<std::initializer_list<_T> > {
            static cJSON *invoke(const std::initializer_list<_T> &l) {
                cJSON *ret = cJSON_CreateArray();
                cJSON *prev = nullptr;
                for (typename std::initializer_list<_T>::const_iterator it = l.begin(); it != l.end(); ++it) {
                    cJSON *item = JsonValueConstructorImpl<typename ArgType<_T>::type>::invoke(*it);
                    if (it != l.begin()) {
                        prev->next = item;
                        item->prev = prev;
                    } else {
                        ret->child = item;
                    }
                    prev = item;
                }
                return ret;
            }
        };

        // 键值对
        template <class _TRAITS, class _ALLOC1, class _P, class _ALLOC2>
        struct JsonValueConstructorImpl<std::map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> >;

        template <class _TRAITS, class _ALLOC1, class _P, class _ALLOC2>
        struct JsonValueConstructorImpl<std::multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> >;

        template <class _TRAITS, class _ALLOC1, class _HASHER, class _EQ, class _ALLOC2>
        struct JsonValueConstructorImpl<std::unordered_map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> >;

        template <class _TRAITS, class _ALLOC1, class _HASHER, class _EQ, class _ALLOC2>
        struct JsonValueConstructorImpl<std::unordered_multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> >;

        template <>
        struct JsonValueConstructorImpl<std::initializer_list<std::pair<const char *, JSON_Value> > >;

        // --JsonValueAsImpl

        // 默认
        template <class _T>
        struct JsonValueAsImpl {
            static _T invoke(cJSON *root) {
                _T t = _T();
                std::stringstream ss;
                switch (root->type) {
                case cJSON_False: ss << false; break;
                case cJSON_True: ss << true; break;
                case cJSON_NULL: break;
                case cJSON_Int: ss << root->valueint; break;
                case cJSON_Int64: ss << root->valueint64; break;
                case cJSON_Number: ss << root->valuedouble; break;
                case cJSON_String: ss << root->string; break;
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to target type"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to target type"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                ss >> t;
                return t;
            }
        };

        // 基本类型
        template <>
        struct JsonValueAsImpl<char> {
            static char invoke(cJSON *root) {
                switch (root->type) {
                case cJSON_False: return 0;
                case cJSON_True: return 1;
                case cJSON_NULL: return 0;
                case cJSON_Int: return static_cast<char>(root->valueint);
                case cJSON_Int64: return static_cast<char>(root->valueint64);
                case cJSON_Number: return static_cast<char>(root->valuedouble);
                case cJSON_String: return static_cast<char>(atoi(root->valuestring));
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to 'char'"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'char'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return 0;
            }
        };

        template <>
        struct JsonValueAsImpl<signed char> {
            static signed char invoke(cJSON *root) {
                switch (root->type) {
                case cJSON_False: return 0;
                case cJSON_True: return 1;
                case cJSON_NULL: return 0;
                case cJSON_Int: return static_cast<signed char>(root->valueint);
                case cJSON_Int64: return static_cast<signed char>(root->valueint64);
                case cJSON_Number: return static_cast<signed char>(root->valuedouble);
                case cJSON_String: return static_cast<signed char>(atoi(root->valuestring));
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to 'signed char'"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'signed char'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return 0;
            }
        };

        template <>
        struct JsonValueAsImpl<unsigned char> {
            static unsigned char invoke(cJSON *root) {
                switch (root->type) {
                case cJSON_False: return 0;
                case cJSON_True: return 1;
                case cJSON_NULL: return 0;
                case cJSON_Int: return static_cast<unsigned char>(root->valueint);
                case cJSON_Int64: return static_cast<unsigned char>(root->valueint64);
                case cJSON_Number: return static_cast<unsigned char>(root->valuedouble);
                case cJSON_String: return static_cast<unsigned char>(atoi(root->valuestring));
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to 'unsigned char'"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'unsigned char'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return 0;
            }
        };

        template <>
        struct JsonValueAsImpl<short> {
            static short invoke(cJSON *root) {
                switch (root->type) {
                case cJSON_False: return 0;
                case cJSON_True: return 1;
                case cJSON_NULL: return 0;
                case cJSON_Int: return static_cast<short>(root->valueint);
                case cJSON_Int64: return static_cast<short>(root->valueint64);
                case cJSON_Number: return static_cast<short>(root->valuedouble);
                case cJSON_String: return static_cast<short>(atoi(root->valuestring));
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to 'short'"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'short'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return 0;
            }
        };

        template <>
        struct JsonValueAsImpl<unsigned short> {
            static unsigned short invoke(cJSON *root) {
                switch (root->type) {
                case cJSON_False: return 0;
                case cJSON_True: return 1;
                case cJSON_NULL: return 0;
                case cJSON_Int: return static_cast<unsigned short>(root->valueint);
                case cJSON_Int64: return static_cast<unsigned short>(root->valueint64);
                case cJSON_Number: return static_cast<unsigned short>(root->valuedouble);
                case cJSON_String: return static_cast<unsigned short>(atoi(root->valuestring));
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to 'unsigned short'"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'unsigned short'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return 0;
            }
        };

        template <>
        struct JsonValueAsImpl<int> {
            static int invoke(cJSON *root) {
                switch (root->type) {
                case cJSON_False: return 0;
                case cJSON_True: return 1;
                case cJSON_NULL: return 0;
                case cJSON_Int: return root->valueint;
                case cJSON_Int64: return static_cast<int>(root->valueint64);
                case cJSON_Number: return static_cast<int>(root->valuedouble);
                case cJSON_String: return atoi(root->valuestring);
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to 'int'"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'int'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return 0;
            }
        };

        template <>
        struct JsonValueAsImpl<unsigned> {
            static unsigned invoke(cJSON *root) {
                switch (root->type) {
                case cJSON_False: return 0U;
                case cJSON_True: return 1U;
                case cJSON_NULL: return 0U;
                case cJSON_Int: return static_cast<unsigned>(root->valueint);
                case cJSON_Int64: return static_cast<unsigned>(root->valueint64);
                case cJSON_Number: return static_cast<unsigned>(root->valuedouble);
                case cJSON_String: return static_cast<unsigned>(atoi(root->valuestring));
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to 'unsigned'"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'unsigned'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return 0U;
            }
        };

        template <>
        struct JsonValueAsImpl<int64_t> {
            static int64_t invoke(cJSON *root) {
                switch (root->type) {
                case cJSON_False: return 0LL;
                case cJSON_True: return 1LL;
                case cJSON_NULL: return 0LL;
                case cJSON_Int: return static_cast<int64_t>(root->valueint);
                case cJSON_Int64: return root->valueint64;
                case cJSON_Number: return static_cast<int64_t>(root->valuedouble);
                case cJSON_String: return atoll(root->valuestring);
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to 'int64_t'"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'int64_t'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return 0LL;
            }
        };

        template <>
        struct JsonValueAsImpl<uint64_t> {
            static uint64_t invoke(cJSON *root) {
                switch (root->type) {
                case cJSON_False: return 0ULL;
                case cJSON_True: return 1ULL;
                case cJSON_NULL: return 0ULL;
                case cJSON_Int: return static_cast<uint64_t>(root->valueint);
                case cJSON_Int64: return static_cast<uint64_t>(root->valueint64);
                case cJSON_Number: return static_cast<uint64_t>(root->valuedouble);
                case cJSON_String: return atoll(root->valuestring);
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to 'uint64_t'"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'uint64_t'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return 0ULL;
            }
        };

        template <>
        struct JsonValueAsImpl<float> {
            static float invoke(cJSON *root) {
                switch (root->type) {
                case cJSON_False: return 0.0F;
                case cJSON_True: return 1.0F;
                case cJSON_NULL: return 0.0F;
                case cJSON_Int: return static_cast<float>(root->valueint);
                case cJSON_Int64: return static_cast<float>(root->valueint64);
                case cJSON_Number: return static_cast<float>(root->valuedouble);
                case cJSON_String: return static_cast<float>(atof(root->valuestring));
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to 'float'"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'float'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return 0;
            }
        };

        template <>
        struct JsonValueAsImpl<double> {
            static double invoke(cJSON *root) {
                switch (root->type) {
                case cJSON_False: return 0.0;
                case cJSON_True: return 1.0;
                case cJSON_NULL: return 0.0;
                case cJSON_Int: return static_cast<double>(root->valueint);
                case cJSON_Int64: return static_cast<double>(root->valueint64);
                case cJSON_Number: return static_cast<double>(root->valuedouble);
                case cJSON_String: return atof(root->valuestring);
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to 'double'"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'double'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return 0;
            }
        };

        // 字符串
        template <class _TRAITS, class _ALLOC>
        struct JsonValueAsImpl<std::basic_string<char, _TRAITS, _ALLOC> > {
            static std::basic_string<char, _TRAITS, _ALLOC> invoke(cJSON *root) {
                switch (root->type) {
                case cJSON_False: return "false";
                case cJSON_True: return "true";
                case cJSON_NULL: return "null";
                case cJSON_Int: {
                    std::basic_ostringstream<char, _TRAITS, _ALLOC> ss;
                    ss << root->valueint;
                    return ss.str();
                }
                case cJSON_Int64: {
                    std::basic_ostringstream<char, _TRAITS, _ALLOC> ss;
                    ss << root->valueint64;
                    return ss.str();
                }
                case cJSON_Number: {
                    std::basic_ostringstream<char, _TRAITS, _ALLOC> ss;
                    ss << root->valuedouble;
                    return ss.str();
                }
                case cJSON_String: return root->valuestring;
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Array to 'string'"); break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'string'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return std::basic_string<char, _TRAITS, _ALLOC>();
            }
        };

        // 数组
        template <class _T, class _ALLOC>
        struct JsonValueAsImpl<std::vector<_T, _ALLOC> > {
            static std::vector<_T, _ALLOC> invoke(cJSON *root) {
                std::vector<_T, _ALLOC> ret;
                switch (root->type) {
                case cJSON_False:  throw std::logic_error("Cannot convert cJSON_False to 'array'"); break;
                case cJSON_True:  throw std::logic_error("Cannot convert cJSON_True to 'array'"); break;
                case cJSON_NULL: break;
                case cJSON_Int: throw std::logic_error("Cannot convert cJSON_Int to 'array'"); break;
                case cJSON_Int64: throw std::logic_error("Cannot convert cJSON_Int64 to 'array'"); break;
                case cJSON_Number:  throw std::logic_error("Cannot convert cJSON_Number to 'array'"); break;
                case cJSON_String: throw std::logic_error("Cannot convert cJSON_String to 'array'"); break;
                case cJSON_Array:
                    for (cJSON *c = root->child; c != nullptr; c = c->next) {
                        ret.push_back(JsonValueAsImpl<_T>::invoke(c));
                    }
                    break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'array'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return ret;
            }
        };

        template <class _T, class _ALLOC>
        struct JsonValueAsImpl<std::list<_T, _ALLOC> > {
            static std::list<_T, _ALLOC> invoke(cJSON *root) {
                std::list<_T, _ALLOC> ret;
                switch (root->type) {
                case cJSON_False:  throw std::logic_error("Cannot convert cJSON_False to 'array'"); break;
                case cJSON_True:  throw std::logic_error("Cannot convert cJSON_True to 'array'"); break;
                case cJSON_NULL: break;
                case cJSON_Int: throw std::logic_error("Cannot convert cJSON_Int to 'array'"); break;
                case cJSON_Int64: throw std::logic_error("Cannot convert cJSON_Int64 to 'array'"); break;
                case cJSON_Number:  throw std::logic_error("Cannot convert cJSON_Number to 'array'"); break;
                case cJSON_String: throw std::logic_error("Cannot convert cJSON_String to 'array'"); break;
                case cJSON_Array:
                    for (cJSON *c = root->child; c != nullptr; c = c->next) {
                        ret.push_back(JsonValueAsImpl<_T>::invoke(c));
                    }
                    break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'array'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return ret;
            }
        };

        template <class _T, class _P, class _ALLOC>
        struct JsonValueAsImpl<std::set<_T, _P, _ALLOC> > {
            static std::set<_T, _P, _ALLOC> invoke(cJSON *root) {
                std::set<_T, _P, _ALLOC> ret;
                switch (root->type) {
                case cJSON_False:  throw std::logic_error("Cannot convert cJSON_False to 'array'"); break;
                case cJSON_True:  throw std::logic_error("Cannot convert cJSON_True to 'array'"); break;
                case cJSON_NULL: break;
                case cJSON_Int: throw std::logic_error("Cannot convert cJSON_Int to 'array'"); break;
                case cJSON_Int64: throw std::logic_error("Cannot convert cJSON_Int64 to 'array'"); break;
                case cJSON_Number:  throw std::logic_error("Cannot convert cJSON_Number to 'array'"); break;
                case cJSON_String: throw std::logic_error("Cannot convert cJSON_String to 'array'"); break;
                case cJSON_Array:
                    for (cJSON *c = root->child; c != nullptr; c = c->next) {
                        ret.insert(JsonValueAsImpl<_T>::invoke(c));
                    }
                    break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'array'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return ret;
            }
        };

        template <class _T, class _P, class _ALLOC>
        struct JsonValueAsImpl<std::multiset<_T, _P, _ALLOC> > {
            static std::multiset<_T, _P, _ALLOC> invoke(cJSON *root) {
                std::multiset<_T, _P, _ALLOC> ret;
                switch (root->type) {
                case cJSON_False:  throw std::logic_error("Cannot convert cJSON_False to 'array'"); break;
                case cJSON_True:  throw std::logic_error("Cannot convert cJSON_True to 'array'"); break;
                case cJSON_NULL: break;
                case cJSON_Int: throw std::logic_error("Cannot convert cJSON_Int to 'array'"); break;
                case cJSON_Int64: throw std::logic_error("Cannot convert cJSON_Int64 to 'array'"); break;
                case cJSON_Number:  throw std::logic_error("Cannot convert cJSON_Number to 'array'"); break;
                case cJSON_String: throw std::logic_error("Cannot convert cJSON_String to 'array'"); break;
                case cJSON_Array:
                    for (cJSON *c = root->child; c != nullptr; c = c->next) {
                        ret.insert(JsonValueAsImpl<_T>::invoke(c));
                    }
                    break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'array'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return ret;
            }
        };

        template <class _T, class _HASHER, class _EQ, class _ALLOC>
        struct JsonValueAsImpl<std::unordered_set<_T, _HASHER, _EQ, _ALLOC> > {
            static std::unordered_set<_T, _HASHER, _EQ, _ALLOC> invoke(cJSON *root) {
                std::unordered_set<_T, _HASHER, _EQ, _ALLOC> ret;
                switch (root->type) {
                case cJSON_False:  throw std::logic_error("Cannot convert cJSON_False to 'array'"); break;
                case cJSON_True:  throw std::logic_error("Cannot convert cJSON_True to 'array'"); break;
                case cJSON_NULL: break;
                case cJSON_Int: throw std::logic_error("Cannot convert cJSON_Int to 'array'"); break;
                case cJSON_Int64: throw std::logic_error("Cannot convert cJSON_Int64 to 'array'"); break;
                case cJSON_Number:  throw std::logic_error("Cannot convert cJSON_Number to 'array'"); break;
                case cJSON_String: throw std::logic_error("Cannot convert cJSON_String to 'array'"); break;
                case cJSON_Array:
                    for (cJSON *c = root->child; c != nullptr; c = c->next) {
                        ret.insert(JsonValueAsImpl<_T>::invoke(c));
                    }
                    break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'array'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return ret;
            }
        };

        template <class _T, class _HASHER, class _EQ, class _ALLOC>
        struct JsonValueAsImpl<std::unordered_multiset<_T, _HASHER, _EQ, _ALLOC> > {
            static std::unordered_multiset<_T, _HASHER, _EQ, _ALLOC> invoke(cJSON *root) {
                std::unordered_multiset<_T, _HASHER, _EQ, _ALLOC> ret;
                switch (root->type) {
                case cJSON_False:  throw std::logic_error("Cannot convert cJSON_False to 'array'"); break;
                case cJSON_True:  throw std::logic_error("Cannot convert cJSON_True to 'array'"); break;
                case cJSON_NULL: break;
                case cJSON_Int: throw std::logic_error("Cannot convert cJSON_Int to 'array'"); break;
                case cJSON_Int64: throw std::logic_error("Cannot convert cJSON_Int64 to 'array'"); break;
                case cJSON_Number:  throw std::logic_error("Cannot convert cJSON_Number to 'array'"); break;
                case cJSON_String: throw std::logic_error("Cannot convert cJSON_String to 'array'"); break;
                case cJSON_Array:
                    for (cJSON *c = root->child; c != nullptr; c = c->next) {
                        ret.insert(JsonValueAsImpl<_T>::invoke(c));
                    }
                    break;
                case cJSON_Object: throw std::logic_error("Cannot convert cJSON_Object to 'array'"); break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return ret;
            }
        };

        // 键值对
        template <class _TRAITS, class _ALLOC1, class _P, class _ALLOC2>
        struct JsonValueAsImpl<std::map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> >;

        template <class _TRAITS, class _ALLOC1, class _P, class _ALLOC2>
        struct JsonValueAsImpl<std::multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> >;

        template <class _TRAITS, class _ALLOC1, class _HASHER, class _EQ, class _ALLOC2>
        struct JsonValueAsImpl<std::unordered_map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> >;

        template <class _TRAITS, class _ALLOC1, class _HASHER, class _EQ, class _ALLOC2>
        struct JsonValueAsImpl<std::unordered_multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> >;
    }

    class JSON_Value final
    {
    public:
        // 默认构造
        JSON_Value() : _root(nullptr) {
        }

        // 用cJSON指针构造，duplicate是否复制
        JSON_Value(cJSON *root, bool duplicate) : _root(nullptr) {
            if (duplicate) {
                _root = cJSON_Duplicate(root, 1);
            } else {
                _root = root;
            }
        }

        // 带参构造
        template <class _T>
        explicit JSON_Value(_T &&t) : _root(nullptr) {
            _root = __json_impl::JsonValueConstructorImpl<typename __json_impl::ArgType<_T>::type>::invoke(std::forward<_T>(t));
        }

        template <class _T>
        explicit JSON_Value(const std::initializer_list<_T> &l) : _root(nullptr) {
            _root = __json_impl::JsonValueConstructorImpl<std::initializer_list<_T> >::invoke(l);
        }

        template <class _T>
        explicit JSON_Value(std::initializer_list<_T> &&l) : _root(nullptr) {
            _root = __json_impl::JsonValueConstructorImpl<std::initializer_list<_T> >::invoke(l);
        }

        // 复制构造
        JSON_Value(const JSON_Value &other) {
            _root = cJSON_Duplicate(other._root, 1);
        }

        // 移动构造
        JSON_Value(JSON_Value &&other) {
            _root = other._root;
            other._root = nullptr;
        }

        // 赋值
        JSON_Value &operator=(const JSON_Value &other) {
            clear();
            _root = cJSON_Duplicate(other._root, 1);
            return *this;
        }

        // 移动赋值
        JSON_Value &operator=(JSON_Value &&other) {
            clear();
            _root = other._root;
            other._root = nullptr;
            return *this;
        }

        // 用nullptr赋值
        JSON_Value &operator=(std::nullptr_t) {
            clear();
            return *this;
        }

        // 析构
        ~JSON_Value() {
            clear();
        }

        // 清空
        inline void clear() {
            if (_root != nullptr) {
                cJSON_Delete(_root);
                _root = nullptr;
            }
        }

        // 从字符串解释
        bool parse(const char *str) {
            clear();
            _root = cJSON_Parse(str);
            if (_root != nullptr) {
                return true;
            }
            return false;
        }

        // 当前结点类型
        enum class TYPE {
            False = cJSON_False,
            True = cJSON_True,
            Null = cJSON_NULL,
            Int = cJSON_Int,
            Int64 = cJSON_Int64,
            Number = cJSON_Number,
            String = cJSON_String,
            Array = cJSON_Array,
            Object = cJSON_Object
        };

        inline TYPE type() const {
            return static_cast<TYPE>(_root->type);
        }

        // 字符串化
        std::string stringfiy() const {
            if (_root != nullptr) {
                char *out = cJSON_Print(_root);
                std::string ret = out;
                free(out);
                return ret;
            }
            return std::string();
        }

        // 重载与nullptr的比较
        inline bool operator==(std::nullptr_t) const {
            return (_root == nullptr);
        }

        inline bool operator!=(std::nullptr_t) const {
            return (_root != nullptr);
        }

        template <class _T> _T as() const {
            if (_root != nullptr) {
                return __json_impl::JsonValueAsImpl<_T>::invoke(_root);
            }
            return _T();
        }

        // 对数组的增加
        template <class _T> void add(const _T &value) {
            if (_root == nullptr) {
                _root = cJSON_CreateArray();
            }
            if (cJSON_Array == _root->type) {
                cJSON *item = __json_impl::JsonValueConstructorImpl<typename __json_impl::ArgType<_T>::type>::invoke(value);
                cJSON_AddItemToArray(_root, item);
            }
        }

        // 对数组的增加
        void add(const JSON_Value &value) {
            if (value == nullptr) return;
            if (_root == nullptr) {
                _root = cJSON_CreateArray();
            }
            if (cJSON_Array == _root->type) {
                cJSON_AddItemToArray(_root, cJSON_Duplicate(value._root, 1));
            }
        }

        // 对数组的增加
        void add(JSON_Value &&value) {
            if (value == nullptr) return;
            if (_root == nullptr) {
                _root = cJSON_CreateArray();
            }
            if (cJSON_Array == _root->type) {
                cJSON_AddItemToArray(_root, value._root);
                value._root = nullptr;
            }
        }

        // 对键值对的增加
        template <class _T> void insert(const char *key, const _T &value) {
            if (value == nullptr) return;
            if (_root == nullptr) {
                _root = cJSON_CreateObject();
            }
            if (cJSON_Object == _root->type) {
                cJSON *item = __json_impl::JsonValueConstructorImpl<typename __json_impl::ArgType<_T>::type>::invoke(value);
                cJSON_AddItemToObject(_root, key, item);
            }
        }

        // 对键值对的增加
        void insert(const char *key, const JSON_Value &value) {
            if (value == nullptr) return;
            if (_root == nullptr) {
                _root = cJSON_CreateObject();
            }
            if (cJSON_Object == _root->type) {
                cJSON_AddItemToObject(_root, key, cJSON_Duplicate(value._root, 1));
            }
        }

        // 对键值对的增加
        void insert(const char *key, JSON_Value &&value) {
            if (value == nullptr) return;
            if (_root == nullptr) {
                _root = cJSON_CreateObject();
            }
            if (cJSON_Object == _root->type) {
                cJSON_AddItemToObject(_root, key, value._root);
                value._root = nullptr;
            }
        }

        // 合并，仅对数组和键值对有效
        void merge(const JSON_Value &value) {
            if (value == nullptr) return;
            if (_root == nullptr) {
                switch (value._root->type) {
                case cJSON_Array: _root = cJSON_CreateArray(); break;
                case cJSON_Object: _root = cJSON_CreateObject(); break;
                default:
                    break;
                }
            }
            if (_root != nullptr) {
                if (_root->type == value._root->type) {
                    switch (_root->type) {
                    case cJSON_Array:
                        for (cJSON *c = value._root->child; c != nullptr; c = c->next) {
                            cJSON_AddItemToArray(_root, cJSON_Duplicate(c, 1));
                        }
                        break;
                    case cJSON_Object:
                        for (cJSON *c = value._root->child; c != nullptr; c = c->next) {
                            cJSON_AddItemToObject(_root, c->string, cJSON_Duplicate(c, 1));
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
        }

    private:
        cJSON *_root = nullptr;

        template <class _OS> friend _OS &operator<<(_OS &os, const JSON_Value &js);
        template <class _T> friend struct __json_impl::JsonValueConstructorImpl;
    };

    // 流输出
    template <class _OS> static _OS &operator<<(_OS &os, const JSON_Value &js) {
        if (js._root != nullptr) {
            char *out = cJSON_Print(js._root);
            if (out != nullptr) {
                os << out;
                free(out);
            }
        }
        return os;
    }

    // 重载与nullptr的比较
    static inline bool operator==(std::nullptr_t, const jw::JSON_Value &js) {
        return js.operator==(nullptr);
    }

    static inline bool operator!=(std::nullptr_t, const jw::JSON_Value &js) {
        return js.operator!=(nullptr);
    }

    namespace __json_impl {

        // --JsonValueConstructorImpl

        template <class _TRAITS, class _ALLOC1, class _P, class _ALLOC2>
        struct JsonValueConstructorImpl<std::map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> > {
            static cJSON *invoke(const std::map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> &m) {
                cJSON *ret = cJSON_CreateObject();
                for (typename std::map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2>::const_iterator it = m.begin(); it != m.end(); ++it) {
                    cJSON_AddItemToObject(ret, it->first.c_str(), cJSON_Duplicate(it->second._root, 1));
                }
                return ret;
            }
        };

        template <class _TRAITS, class _ALLOC1, class _P, class _ALLOC2>
        struct JsonValueConstructorImpl<std::multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> > {
            static cJSON *invoke(const std::multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> &m) {
                cJSON *ret = cJSON_CreateObject();
                for (typename std::multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2>::const_iterator it = m.begin(); it != m.end(); ++it) {
                    cJSON_AddItemToObject(ret, it->first.c_str(), cJSON_Duplicate(it->second._root, 1));
                }
                return ret;
            }
        };

        template <class _TRAITS, class _ALLOC1, class _HASHER, class _EQ, class _ALLOC2>
        struct JsonValueConstructorImpl<std::unordered_map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> > {
            static cJSON *invoke(const std::unordered_map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> &m) {
                cJSON *ret = cJSON_CreateObject();
                for (typename std::unordered_map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2>::const_iterator it = m.begin(); it != m.end(); ++it) {
                    cJSON_AddItemToObject(ret, it->first.c_str(), cJSON_Duplicate(it->second._root, 1));
                }
                return ret;
            }
        };

        template <class _TRAITS, class _ALLOC1, class _HASHER, class _EQ, class _ALLOC2>
        struct JsonValueConstructorImpl<std::unordered_multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> > {
            static cJSON *invoke(const std::unordered_multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> &m) {
                cJSON *ret = cJSON_CreateObject();
                for (typename std::unordered_multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2>::const_iterator it = m.begin(); it != m.end(); ++it) {
                    cJSON_AddItemToObject(ret, it->first.c_str(), cJSON_Duplicate(it->second._root, 1));
                }
                return ret;
            }
        };

        template <>
        struct JsonValueConstructorImpl<std::initializer_list<std::pair<const char *, JSON_Value> > > {
            static cJSON *invoke(const std::initializer_list<std::pair<const char *, JSON_Value> > &l) {
                cJSON *ret = cJSON_CreateObject();
                for (std::initializer_list<std::pair<const char *, JSON_Value> >::const_iterator it = l.begin(); it != l.end(); ++it) {
                    cJSON_AddItemToObject(ret, it->first, cJSON_Duplicate(it->second._root, 1));
                }
                return ret;
            }
        };

        // --JsonValueAsImpl

        template <class _TRAITS, class _ALLOC1, class _P, class _ALLOC2>
        struct JsonValueAsImpl<std::map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> > {
            static std::map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> invoke(cJSON *root) {
                std::map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> ret;
                switch (root->type) {
                case cJSON_False:  throw std::logic_error("Cannot convert cJSON_False to 'object'"); break;
                case cJSON_True:  throw std::logic_error("Cannot convert cJSON_True to 'object'"); break;
                case cJSON_NULL: break;
                case cJSON_Int: throw std::logic_error("Cannot convert cJSON_Int to 'object'"); break;
                case cJSON_Int64: throw std::logic_error("Cannot convert cJSON_Int64 to 'object'"); break;
                case cJSON_Number:  throw std::logic_error("Cannot convert cJSON_Number to 'object'"); break;
                case cJSON_String: throw std::logic_error("Cannot convert cJSON_String to 'object'"); break;
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Object to 'object'"); break;
                case cJSON_Object:
                    for (cJSON *c = root->child; c != nullptr;  c = c->next) {
                        ret.insert(std::make_pair(c->string, JSON_Value(c, true)));
                    }
                    break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return ret;
            }
        };

        template <class _TRAITS, class _ALLOC1, class _P, class _ALLOC2>
        struct JsonValueAsImpl<std::multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> > {
            static std::multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> invoke(cJSON *root) {
                std::multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _P, _ALLOC2> ret;
                switch (root->type) {
                case cJSON_False:  throw std::logic_error("Cannot convert cJSON_False to 'object'"); break;
                case cJSON_True:  throw std::logic_error("Cannot convert cJSON_True to 'object'"); break;
                case cJSON_NULL: break;
                case cJSON_Int: throw std::logic_error("Cannot convert cJSON_Int to 'object'"); break;
                case cJSON_Int64: throw std::logic_error("Cannot convert cJSON_Int64 to 'object'"); break;
                case cJSON_Number:  throw std::logic_error("Cannot convert cJSON_Number to 'object'"); break;
                case cJSON_String: throw std::logic_error("Cannot convert cJSON_String to 'object'"); break;
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Object to 'object'"); break;
                case cJSON_Object:
                    for (cJSON *c = root->child; c != nullptr;  c = c->next) {
                        ret.insert(std::make_pair(c->string, JSON_Value(c, true)));
                    }
                    break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return ret;
            }
        };

        template <class _TRAITS, class _ALLOC1, class _HASHER, class _EQ, class _ALLOC2>
        struct JsonValueAsImpl<std::unordered_map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> > {
            static std::unordered_map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> invoke(cJSON *root) {
                std::unordered_map<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> ret;
                switch (root->type) {
                case cJSON_False:  throw std::logic_error("Cannot convert cJSON_False to 'object'"); break;
                case cJSON_True:  throw std::logic_error("Cannot convert cJSON_True to 'object'"); break;
                case cJSON_NULL: break;
                case cJSON_Int: throw std::logic_error("Cannot convert cJSON_Int to 'object'"); break;
                case cJSON_Int64: throw std::logic_error("Cannot convert cJSON_Int64 to 'object'"); break;
                case cJSON_Number:  throw std::logic_error("Cannot convert cJSON_Number to 'object'"); break;
                case cJSON_String: throw std::logic_error("Cannot convert cJSON_String to 'object'"); break;
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Object to 'object'"); break;
                case cJSON_Object:
                    for (cJSON *c = root->child; c != nullptr;  c = c->next) {
                        ret.insert(std::make_pair(c->string, JSON_Value(c, true)));
                    }
                    break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return ret;
            }
        };

        template <class _TRAITS, class _ALLOC1, class _HASHER, class _EQ, class _ALLOC2>
        struct JsonValueAsImpl<std::unordered_multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> > {
            static std::unordered_multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> invoke(cJSON *root) {
                std::unordered_multimap<std::basic_string<char, _TRAITS, _ALLOC1>, JSON_Value, _HASHER, _EQ, _ALLOC2> ret;
                switch (root->type) {
                case cJSON_False:  throw std::logic_error("Cannot convert cJSON_False to 'object'"); break;
                case cJSON_True:  throw std::logic_error("Cannot convert cJSON_True to 'object'"); break;
                case cJSON_NULL: break;
                case cJSON_Int: throw std::logic_error("Cannot convert cJSON_Int to 'object'"); break;
                case cJSON_Int64: throw std::logic_error("Cannot convert cJSON_Int64 to 'object'"); break;
                case cJSON_Number:  throw std::logic_error("Cannot convert cJSON_Number to 'object'"); break;
                case cJSON_String: throw std::logic_error("Cannot convert cJSON_String to 'object'"); break;
                case cJSON_Array: throw std::logic_error("Cannot convert cJSON_Object to 'object'"); break;
                case cJSON_Object:
                    for (cJSON *c = root->child; c != nullptr;  c = c->next) {
                        ret.insert(std::make_pair(c->string, JSON_Value(c, true)));
                    }
                    break;
                default: throw std::out_of_range("cJSON type out of range"); break;
                }
                return ret;
            }
        };
    }
}

#endif
