#include <co_ecs/co_ecs.hpp>

struct s1 {
    uint32_t i1;
    uint64_t i2;
};

struct s2 {
    float f1;
    int i1;
};

struct s3 {
    char c, e;
};

void test_create(co_ecs::registry& reg) {
    auto a = reg.create<s1, s3>({1, 2}, {92, 93});
    auto b = reg.create<s1, s3>({7, 3}, {75, 76});
    auto c = reg.create<s2>({});
}

int main() {
    co_ecs::registry reg;
    test_create(reg);
    return 0;
}
