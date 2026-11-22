#include "MemoryReader.h"
#include "ModuleScanner.h"
#include "RelativeAddress.h"
#include "PointerResolver.h"

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

    // Modern Bedrock PlatformGameCore signature.
    SignatureScanner::Pattern pattern = {
        0x4C, 0x89, 0x3D,
        -1, -1, -1, -1,
        0x4D, 0x85, 0xFF
    };

    auto matches = ModuleScanner::ScanText(reader, pattern);

    if (matches.size() != 1)
    {
        std::cout << "Expected one signature match, found "
                  << matches.size() << "\n";
        return 1;
    }

    uint8_t instruction[7] = {};

    if (!reader.Read(matches[0], instruction, sizeof(instruction)))
    {
        std::cout << "Failed to read signature instruction.\n";
        return 1;
    }

    uintptr_t globalAddress = RelativeAddress::Resolve(
        matches[0], instruction, sizeof(instruction), 3
    );

    auto readPointer = [&](uintptr_t address, uintptr_t& value) {
        return reader.Read(address, &value, sizeof(value));
    };

    uintptr_t minecraftGame = PointerResolver::ResolveValue(
        globalAddress,
        {0x0, 0x8, 0x118, 0x48},
        readPointer
    );

    if (minecraftGame == 0)
    {
        std::cout << "MinecraftGame pointer chain failed.\n";
        return 1;
    }

    std::cout << "MinecraftGame: 0x"
              << std::hex << minecraftGame
              << std::dec << "\n";

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
