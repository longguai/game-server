#include "../tcc/tcc-0.9.26/tcc.h"
#undef malloc
#undef free
#pragma comment(lib, "../../build/game-server/win32/Debug/tcc.lib")

int main(int, char **)
{
    char program[] = "int foo() { return 123; }";
    TCCState *s = tcc_new();

    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

    if (tcc_compile_string(s, program) != -1)
    {
        int size = tcc_relocate(s, nullptr);
        if (size > 0)
        {
            void *mem = malloc(size);
            tcc_relocate(s, mem);

            int(*foo)() = (int(*)())tcc_get_symbol(s, "foo");
            if (foo != nullptr)
            {
                printf("%d", foo());
            }

            free(mem);
        }
    }
    tcc_delete(s);

    return 0;
}
