#include <EC/Manager.hpp>

struct NoDef
{
    NoDef(int a) : a(a) {}

    int a;
};

struct WithDef
{
    WithDef() : a(0) {}

    int a;
};

using EC::Meta::TypeList;

int main()
{
    // should fail to compile because "NoDef" is not default constructible
    EC::Manager<TypeList<WithDef, NoDef>, TypeList<>> manager;
    return 0;
}
