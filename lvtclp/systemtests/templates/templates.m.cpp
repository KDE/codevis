template<class T>
struct Foo {
    class Bar;

    static void run(Bar const& bar)
    {
        T::run(bar.val);
    }
};

struct IntRunner {
    static void run(int i){};
};

template<>
struct Foo<IntRunner>::Bar {
    int val;
};

int main()
{
    Foo<IntRunner>::run(Foo<IntRunner>::Bar{42});
    return 0;
}
