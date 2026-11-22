#include "MemoryReader.h"

#include <cstdint>
#include <iomanip>
#include <iostream>

struct Vec3
{
    float x;
    float y;
    float z;
};

static_assert(sizeof(Vec3) == 12);

int main()
{
    MemoryReader reader;

    if (!reader.Attach())
    {
        std::cout << "Failed to attach to Minecraft.\n";
        return 1;
    }

    std::cout << "Attached to Minecraft!\n";

    // Temporary address discovered in our live memory tests.
    constexpr uintptr_t positionAddress = 0x4433D282820ULL;

    Vec3 position{};

    if (!reader.Read(positionAddress, &position, sizeof(position)))
    {
        std::cout << "Failed to read player position.\n";
        return 1;
    }

    std::cout << std::fixed << std::setprecision(3);

    std::cout << "Player position:\n";
    std::cout << "X: " << position.x << '\n';
    std::cout << "Y: " << position.y << '\n';
    std::cout << "Z: " << position.z << '\n';

    return 0;
}
