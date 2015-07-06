#if (defined _DEBUG) || (defined DEBUG)
#   define CRTDBG_MAP_ALLOC
#   include <crtdbg.h>
#endif

#include "cppJSON.hpp"
//#include "JsonValue.h"

#include <iostream>

template <class _T> void func(const _T &t)
{
    for (auto &v : t) {
        std::cout << v << std::endl;
    }
    std::cout << std::endl;
}

template <class _MAP> void func2(const _MAP &map) {
    for (auto &v : map) {
        std::cout << v.first << '\t' << v.second << std::endl;
    }
    std::cout << std::endl;
}

template <template <class> class T, template <class> class A>
struct Test {
    std::basic_string<char, T<char>, A<char> > str;
    std::char_traits<char> a;
};

struct TestCharTraits {
    typedef char _Elem;
    typedef _Elem char_type;
    typedef int int_type;
    typedef std::streampos pos_type;
    typedef std::streamoff off_type;
    typedef int state_type;

    static int compare(const _Elem *_First1, const _Elem *_First2,
        size_t _Count) {
        return (_Count == 0 ? 0 : memcmp(_First1, _First2, _Count));
    }

    static size_t length(const _Elem *_First) {	// find length of null-terminated string
        return (*_First == 0 ? 0 : strlen(_First));
    }

    static _Elem *copy(_Elem *_First1, const _Elem *_First2, size_t _Count) {	// copy [_First2, _First2 + _Count) to [_First1, ...)
        return (_Count == 0 ? _First1 : (_Elem *)memcpy(_First1, _First2, _Count));
    }
    /*
    static _Elem *_Copy_s(
        _Out_writes_(_Size_in_bytes) _Elem *_First1, size_t _Size_in_bytes,
        _In_reads_(_Count) const _Elem *_First2, size_t _Count)
    {	// copy [_First2, _First2 + _Count) to [_First1, ...)
        if (0 < _Count)
            _CRT_SECURE_MEMCPY(_First1, _Size_in_bytes, _First2, _Count);
        return (_First1);
    }
    */
    static const _Elem * find(const _Elem *_First, size_t _Count, const _Elem& _Ch) {	// look for _Ch in [_First, _First + _Count)
        return (_Count == 0 ? (const _Elem *)0 : (const _Elem *)memchr(_First, _Ch, _Count));
    }

    static _Elem * move(_Elem *_First1, const _Elem *_First2, size_t _Count) {	// copy [_First2, _First2 + _Count) to [_First1, ...)
        return (_Count == 0 ? _First1 : (_Elem *)memmove(_First1, _First2, _Count));
    }

    static _Elem *assign(_Elem *_First, size_t _Count, _Elem _Ch) {	// assign _Count * _Ch to [_First, ...)
        return ((_Elem *)memset(_First, _Ch, _Count));
    }

    static void assign(_Elem& _Left, const _Elem& _Right) throw() {	// assign an element
        _Left = _Right;
    }

    static bool eq(const _Elem& _Left, const _Elem& _Right) throw() {	// test for element equality
        return (_Left == _Right);
    }
    
    static bool lt(const _Elem& _Left, const _Elem& _Right) throw() {	// test if _Left precedes _Right
        return ((unsigned char)_Left < (unsigned char)_Right);
    }

    static _Elem to_char_type(const int_type& _Meta) throw() {	// convert metacharacter to character
        return ((_Elem)_Meta);
    }

    static int_type to_int_type(const _Elem& _Ch) throw() {	// convert character to metacharacter
        return ((unsigned char)_Ch);
    }

    static bool eq_int_type(const int_type& _Left, const int_type& _Right) throw() {	// test for metacharacter equality
        return (_Left == _Right);
    }

    static int_type not_eof(const int_type& _Meta) throw() {	// return anything but EOF
        return (_Meta != eof() ? _Meta : !eof());
    }

    static int_type eof() throw() {	// return end-of-file metacharacter
        return (EOF);
    }
};

#include <windows.h>
#undef max

template <class _T> struct TestAllocator {
    // typedefs
    typedef _T                  value_type;
    typedef value_type          *pointer;
    typedef value_type          &reference;
    typedef const value_type    *const_pointer;
    typedef const value_type    &const_reference;
    typedef size_t              size_type;
    typedef ptrdiff_t           difference_type;

    // rebind
    template <class _Other> struct rebind {
        typedef TestAllocator<_Other> other;
    };

    pointer address(reference val) const {
        return &val;
    }

    const_pointer address(const_reference val) const {
        return &val;
    }

    // construct default allocator (do nothing)
    TestAllocator() throw() {
    }

    // construct by copying (do nothing)
    TestAllocator(const TestAllocator<_T> &) throw() {
    }

    // construct from a related allocator (do nothing)
    template<class _Other>
    TestAllocator(const TestAllocator<_Other> &) throw() {
    }

    // assign from a related allocator (do nothing)
    template<class _Other>
    TestAllocator<_T> &operator=(const TestAllocator<_Other> &) {
        return *this;
    }

    // deallocate object at _Ptr, ignore size
    void deallocate(pointer ptr, size_type) {
        //delete ptr;
        //free(ptr);
        ::HeapFree(::GetProcessHeap(), 0, ptr);
    }

    // allocate array of cnt elements
    pointer allocate(size_type cnt) {
        //return _Allocate(cnt, (pointer)0);
        //return calloc(cnt, sizeof(_T));
        return (pointer)::HeapAlloc(::GetProcessHeap(), 0, cnt * sizeof(_T));
    }

    // allocate array of cnt elements, ignore hint
    pointer allocate(size_type cnt, const void *) {
        return allocate(cnt);
    }

    // default construct object at ptr
    void construct(_T *ptr) {
        if (ptr == nullptr) throw std::invalid_argument("");
        new ((void *)ptr) _T();
    }

    // construct object at ptr with value val
    void construct(_T *ptr, const _T &val) {
        if (ptr == nullptr) throw std::invalid_argument("");
        new ((void *)ptr) _T(val);
    }

    // construct _Obj(_Types...) at ptr
    template <class _Obj, class... _Types>
    void construct(_Obj *ptr, _Types &&...args) {
        if (ptr == nullptr) throw std::invalid_argument("");
        new ((void *)ptr) _Obj(std::forward<_Types>(args)...);
    }

    // destroy object at ptr
    template<class _U>
    void destroy(_U *ptr) {
        ptr->~_U();
    }

    // estimate maximum array size
    size_t max_size() const throw() {
        return ((size_t)(-1) / sizeof(_T));
    }
};

// test for allocator equality
template <class _T, class _Other>
inline bool operator==(const TestAllocator<_T> &, const TestAllocator<_Other> &) throw() {
    return true;
}

// test for allocator inequality
template <class _T, class _Other>
inline bool operator!=(const TestAllocator<_T> &left, const TestAllocator<_Other> &right) throw() {
    return !(left == right);
}

#include <algorithm>
#include <functional>

int main(int argc, char *argv[])
{
#if (defined _DEBUG) || (defined DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(147);
#endif
    typedef jw::BasicJSON<TestCharTraits, TestAllocator<char> > cppJSON;

    {
        cppJSON jv = cppJSON(std::vector<int>({ 1, 2, 3, 4, 5, 6, 7 }));
        cppJSON jvt = jv;

        cppJSON jvtt = std::move(jvt);

        std::map<std::string, int> bbb;
        std::transform(jv.begin(), jv.end(),
            std::insert_iterator<std::map<std::string, int> >(bbb, bbb.begin()), //[](const int &j) { return std::make_pair(0, 0); });//std::bind(&cppJSON::as<int>, std::placeholders::_1));
            std::bind(&std::make_pair<std::string, int>, "abc", std::bind(&cppJSON::as<int>, std::placeholders::_1)));
            //[](const cppJSON &j) { return std::make_pair("abc", j.as<int>()); });

        std::for_each(jv.begin(), jv.end(), [](const cppJSON &j) {
            std::cout << j.Print() << std::endl;
        });
        //std::for_each(jv.cbegin(), jv.cend(), std::ostream_iterator<const cppJSON>(std::cout, ", "));

        std::for_each(jv.rbegin(), jv.rend(), [](const cppJSON &j) {
            std::cout << j.Print() << std::endl;
        });
        std::cout << jv.Print() << std::endl;

        jv.Parse("[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]");
        std::cout << jv.Print() << std::endl;
        func(jv.as<std::vector<std::string> >());
        func(jv.as<std::list<std::string> >());
        func(jv.as<std::set<std::string> >());
        func(jv.as<std::multiset<std::string> >());
        func(jv.as<std::unordered_set<std::string> >());
        func(jv.as<std::unordered_multiset<std::string> >());
        jv.clear();

        jv.Parse("{\"name2\":\"Jack\",\"name3\":\"Jack\",\"name4\":\"Jack\",\"name5\":\"Jack\",\"name6\":\"Jack\",\"name7\":\"Jack\"}");
        std::cout << jv.Print() << std::endl;

        func2(jv.as<std::map<std::string, std::string> >());
        func2(jv.as<std::multimap<std::string, std::string> >());
        func2(jv.as<std::unordered_map<std::string, std::string> >());
        func2(jv.as<std::unordered_multimap<std::string, std::string> >());

        std::unordered_map<int, int> t;
        t.insert(std::unordered_map<int, int>::value_type(1, 1));
    }
    //return 0;
    //using jw::JSON_Value;
    typedef cppJSON JSON_Value;

    std::cout << "==========TEST INT64==========" << std::endl;
    std::cout << JSON_Value(std::numeric_limits<int64_t>::max()).stringfiy() << std::endl;

    std::cout << "==========TEST ARRAY AS==========" << std::endl;
    JSON_Value js;
    js.Parse("[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]");
    std::cout << js << std::endl;

    func(js.as<std::vector<std::string> >());
    func(js.as<std::list<std::string> >());
    func(js.as<std::set<std::string> >());
    func(js.as<std::multiset<std::string> >());
    func(js.as<std::unordered_set<std::string> >());
    func(js.as<std::unordered_multiset<std::string> >());

    std::cout << "==========TEST OBJECT==========" << std::endl;
    JSON_Value js1;
    js1.Parse("{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":       \"rect\", \n\"width\":      1920, \n\"height\":     1080, \n\"interlace\":  false,\"frame rate\": 24\n}\n}");
    std::cout << js1 << std::endl;
    //func2(js1.as<std::map<std::string, JSON_Value> >());

    std::cout << "==========TEST OBJECT MERGE==========" << std::endl;
    JSON_Value temp;
    temp.Parse("{\"name2\":\"Jack\",\"name3\":\"Jack\",\"name4\":\"Jack\",\"name5\":\"Jack\",\"name6\":\"Jack\",\"name7\":\"Jack\"}");
    //js1.merge(temp);
    std::cout << js1 << std::endl;
    //func2(js1.as<std::map<std::string, JSON_Value> >());

    std::cout << "==========TEST ARRAY CONSTRUCT1==========" << std::endl;
    JSON_Value js2(std::vector<int>({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }));
    std::cout << js2 << std::endl;

    std::cout << "==========TEST ARRAY CONSTRUCT2==========" << std::endl;
    JSON_Value js3(std::list<double>({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }));
    std::cout << js3 << std::endl;

    std::cout << "==========TEST ARRAY CONSTRUCT3==========" << std::endl;
    const float arr[] = { 0, 1, 2, 3, 4, 5 };
    JSON_Value js4(arr);
    std::cout << js4 << std::endl;

    std::cout << "==========TEST STRING CONSTRUCT==========" << std::endl;
    JSON_Value js5("abcd");
    std::cout << js5 << std::endl;
    // TODO:
    //std::wcout << js5.as<std::wstring>();// << std::endl;

    std::cout << "==========TEST ARRAY CONSTRUCT4==========" << std::endl;
    JSON_Value js6({ 1, 2, 3, 4, 5 });
    std::cout << js6 << std::endl;

    std::cout << "==========TEST ARRAY CONSTRUCT5==========" << std::endl;
    //auto a = { "1", "2", "3", "c" };
    //JSON_Value js7(a);
    //std::cout << js7 << std::endl;

    std::cout << "==========TEST ARRAY MERGE==========" << std::endl;
    JSON_Value js8({ "abc" });
    //js8.merge(js7);
    std::cout << js8 << std::endl;

    std::cout << "==========TEST OBJECT CONSTRUCT==========" << std::endl;
    //auto bbb = std::map<std::string, JSON_Value>({{ "1", JSON_Value(1) }, { "2", JSON_Value(2) }, { "3", JSON_Value(3) }});
    //JSON_Value js9(bbb);
    //std::cout << js9 << std::endl;

    std::cout << "==========TEST OBJECT CONSTRUCT(VS2013 BUG)==========" << std::endl;
    //auto ccc = std::initializer_list<std::pair<const char*, JSON_Value> >({ { "ab", JSON_Value(1) }, { "b13", JSON_Value("b") }, { "c", JSON_Value(0.5F) } });
    //JSON_Value js10(ccc);  // 这里是VS2013的BUG
    //std::cout << js10 << std::endl;

    std::cout << "==========TEST OBJECT CONSTRUCT==========" << std::endl;
    JSON_Value js11;
    //js11.insert("123", JSON_Value("123"));
    //js11.insert("456", JSON_Value({1, 2, 3, 4, 5, 6, 7}));
    //js11.insert("789", JSON_Value(0.5F));
    std::cout << js11 << std::endl;

    std::cout << "==========TEST OBEJCT AS==========" << std::endl;
    //auto data11 = js11.as<std::unordered_map<std::string, JSON_Value> >();
    //auto it = data11.find("123");
    //if (it != data11.end())
    //{
    //    std::cout << it->second.as<int>() << std::endl;
    //}

    std::cout << (js11 == nullptr)  << (js11 != nullptr) << (nullptr == js11) << (nullptr != js11) << std::endl;

    return 0;
}
