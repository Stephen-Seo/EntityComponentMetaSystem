#include <EC/Manager.hpp>

#include <iostream>

struct CPosition {
    CPosition(float x = 0.0F, float y = 0.0F) : x(x), y(y) {}
    float x, y;
};

using EmptyTypeList = EC::Meta::TypeList<>;
using ComponentsTypeList = EC::Meta::TypeList<CPosition>;

int main() {
    auto manager = EC::Manager<ComponentsTypeList, EmptyTypeList>{};

    auto entity_id = manager.addEntity();
    manager.addComponent<CPosition>(entity_id, 1.0F, 2.0F);

    manager.forMatchingSignature<ComponentsTypeList>(
        [] (std::size_t, void *, CPosition *pos)
        {
            if (pos->x != 1.0F) {
                std::clog << "WARNING: pos->x is not 1.0F! (" << pos->x
                          << ")\n";
            }
            if (pos->y != 2.0F) {
                std::clog << "WARNING: pos->y is not 2.0F! (" << pos->y
                          << ")\n";
            }
        },
        nullptr,
        false);

    return 0;
}
