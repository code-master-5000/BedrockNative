
#include <iostream>
#include <cstddef>

struct Vec3 {
    float x, y, z;
};

struct AABB {
    Vec3 lower;
    Vec3 upper;
    bool isZero = false;
    char padding[3];
};

struct EntityDimensions {
    AABB aabb;
    float width;
    float height;
};

int main()
{
    std::cout << "Vec3 size: "
              << sizeof(Vec3) << "\n";

    std::cout << "AABB size: "
              << sizeof(AABB) << "\n";

    std::cout << "Width offset: 0x"
              << std::hex
              << offsetof(EntityDimensions, width)
              << "\n";

    std::cout << "Height offset: 0x"
              << offsetof(EntityDimensions, height)
              << "\n";

    if (sizeof(AABB) == 0x1C &&
        offsetof(EntityDimensions, width) == 0x1C &&
        offsetof(EntityDimensions, height) == 0x20)
    {
        std::cout << "AABB LAYOUT TEST PASSED!\n";
        return 0;
    }

    return 1;
}
