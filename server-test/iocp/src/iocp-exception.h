#ifndef _IOCP_EXCEPTION_H_
#define _IOCP_EXCEPTION_H_

#pragma warning(push)
#pragma warning(disable: 4996)

#include <exception>
#include <string.h>

namespace iocp {
    template <size_t _N> class Exception final : public std::exception {
    public:
        Exception<_N>(const char (&what)[_N]) { strcpy(_what, what); }
        virtual const char *what() const throw() override { return _what; }
    private:
        char _what[_N];
    };
}

#define MAKE_EXCEPTION(what) iocp::Exception<sizeof(what)>(what)

#pragma warning(pop)

#endif
