#include <cstddef>
#include <iostream>
int g()
{
    std::byte ble{9};
    return std::to_integer<int>(ble);
}

int f()
{
    return 0;
}

int main()
{
    return f() + g();
}
