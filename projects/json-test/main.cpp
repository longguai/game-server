#if (defined _DEBUG) || (defined DEBUG)
#   define CRTDBG_MAP_ALLOC
#   include <crtdbg.h>
#endif

#include "JsonValue.h"

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

int main(int argc, char *argv[])
{
#if (defined _DEBUG) || (defined DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(1217);
#endif

    using jw::JSON_Value;

    std::cout << "==========TEST ARRAY AS==========" << std::endl;

    JSON_Value js;
    js.parse("[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]");
    std::cout << js << std::endl;

    func(js.as<std::vector<std::string> >());
    func(js.as<std::list<std::string> >());
    func(js.as<std::set<std::string> >());
    func(js.as<std::multiset<std::string> >());
    func(js.as<std::unordered_set<std::string> >());
    func(js.as<std::unordered_multiset<std::string> >());

    std::cout << "==========TEST OBJECT==========" << std::endl;
    JSON_Value js1;
    js1.parse("{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":       \"rect\", \n\"width\":      1920, \n\"height\":     1080, \n\"interlace\":  false,\"frame rate\": 24\n}\n}");
    std::cout << js1 << std::endl;
    func2(js1.as<std::map<std::string, JSON_Value> >());

    std::cout << "==========TEST OBJECT MERGE==========" << std::endl;
    JSON_Value temp;
    temp.parse("{\"name2\":\"Jack\",\"name3\":\"Jack\",\"name4\":\"Jack\",\"name5\":\"Jack\",\"name6\":\"Jack\",\"name7\":\"Jack\"}");
    js1.merge(temp);
    std::cout << js1 << std::endl;
    func2(js1.as<std::map<std::string, JSON_Value> >());

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

    std::cout << "==========TEST ARRAY CONSTRUCT4==========" << std::endl;
    JSON_Value js6({ 1, 2, 3, 4, 5 });
    std::cout << js6 << std::endl;

    std::cout << "==========TEST ARRAY CONSTRUCT5==========" << std::endl;
    auto a = { "1", "2", "3", "c" };
    JSON_Value js7(a);
    std::cout << js7 << std::endl;

    std::cout << "==========TEST ARRAY MERGE==========" << std::endl;
    JSON_Value js8({ "abc" });
    js8.merge(js7);
    std::cout << js8 << std::endl;

    std::cout << "==========TEST OBJECT CONSTRUCT==========" << std::endl;
    auto bbb = std::map<std::string, JSON_Value>({{ "1", JSON_Value(1) }, { "2", JSON_Value(2) }, { "3", JSON_Value(3) }});
    JSON_Value js9(bbb);
    std::cout << js9 << std::endl;

    std::cout << "==========TEST OBJECT CONSTRUCT(VS2013 BUG)==========" << std::endl;
    auto ccc = std::initializer_list<std::pair<const char*, JSON_Value> >({ { "ab", JSON_Value(1) }, { "b13", JSON_Value("b") }, { "c", JSON_Value(0.5F) } });
    JSON_Value js10(ccc);  // 这里是VS2013的BUG
    std::cout << js10 << std::endl;

    std::cout << "==========TEST OBJECT CONSTRUCT==========" << std::endl;
    JSON_Value js11;
    js11.add("123", JSON_Value("123"));
    js11.add("456", JSON_Value({1, 2, 3, 4, 5, 6, 7}));
    js11.add("789", JSON_Value(0.5F));
    std::cout << js11 << std::endl;

    std::cout << "==========TEST OBEJCT AS==========" << std::endl;
    auto data11 = js11.as<std::unordered_map<std::string, JSON_Value> >();
    auto it = data11.find("123");
    if (it != data11.end())
    {
        std::cout << it->second.as<int>() << std::endl;
    }

    std::cout << (js11 == nullptr)  << (js11 != nullptr) << (nullptr == js11) << (nullptr != js11) << std::endl;
    return 0;
}

