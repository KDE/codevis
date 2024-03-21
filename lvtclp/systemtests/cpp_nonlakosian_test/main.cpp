#include <mylibs/lib1/lib1.h>
#include <mylibs/lib2/lib2.h>

// This include was specifically added for extra argument passing test.
// This project will not compile without extra arguments currently.
#include "myfoo.h"

int main()
{
    return lib1::foo() + lib2::bar();
}
