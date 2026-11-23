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


    // Locate the client through MinecraftGame's client map.
    uintptr_t mapHead = 0;
    uintptr_t mapRoot = 0;
    uintptr_t clientInstance = 0;
    uintptr_t backPointer = 0;
    uint64_t clientCount = 0;

    if (!reader.Read(minecraftGame + 0x9D0,
                     &mapHead, sizeof(mapHead)) ||
        !reader.Read(minecraftGame + 0x9D8,
                     &clientCount, sizeof(clientCount)) ||
        mapHead == 0 || clientCount != 1)
    {
        std::cout << "Unexpected client map.\n";
        return 1;
    }

    if (!reader.Read(mapHead, &mapRoot, sizeof(mapRoot)) ||
        mapRoot == 0 || mapRoot == mapHead ||
        !reader.Read(mapRoot + 0x38,
                     &clientInstance, sizeof(clientInstance)) ||
        clientInstance == 0)
    {
        std::cout << "Failed to locate ClientInstance.\n";
        return 1;
    }

    // Confirm this client belongs to our MinecraftGame.
    if (!reader.Read(clientInstance + 0x1C8,
                     &backPointer, sizeof(backPointer)) ||
        backPointer != minecraftGame)
    {
        std::cout << "ClientInstance validation failed.\n";
        return 1;
    }

    std::cout << "ClientInstance: 0x"
              << std::hex << clientInstance << std::dec << "\n";

    // EntityContext begins at ClientInstance + 0x268.
    // Its entity ID is at context + 0x10.
    uint32_t entityId = 0;

    if (!reader.Read(clientInstance + 0x278,
                     &entityId, sizeof(entityId)))
    {
        std::cout << "Could not read player entity ID.\n";
        return 1;
    }

    std::cout << "Local player entity ID: 0x"
              << std::hex << entityId << std::dec << "\n";

    if ((entityId & 0x3FFFF) == 0x3FFFF)
    {
        std::cout << "No valid local player. Enter a world.\n";
        return 1;
    }


    // Find EnTT component pools through the local EntityRegistry.
    uintptr_t entityRegistry = 0;

    if (!reader.Read(clientInstance + 0x268,
                     &entityRegistry, sizeof(entityRegistry)) ||
        entityRegistry == 0)
    {
        std::cout << "EntityRegistry not found.\n";
        return 1;
    }

    // Offset observed in Minecraft 1.26.65.1.
    uintptr_t pools = entityRegistry + 0x78;

    uintptr_t packedBegin = 0;
    uintptr_t packedEnd = 0;
    uintptr_t packedCapacity = 0;

    if (!reader.Read(pools + 0x20,
                     &packedBegin, sizeof(packedBegin)) ||
        !reader.Read(pools + 0x28,
                     &packedEnd, sizeof(packedEnd)) ||
        !reader.Read(pools + 0x30,
                     &packedCapacity, sizeof(packedCapacity)) ||
        packedBegin == 0 ||
        packedEnd < packedBegin ||
        packedCapacity < packedEnd ||
        (packedEnd - packedBegin) % 32 != 0 ||
        (packedEnd - packedBegin) / 32 > 4096)
    {
        std::cout << "Invalid component-pool layout.\n";
        return 1;
    }

    size_t entryCount = (packedEnd - packedBegin) / 32;

    std::cout << "Component pools: "
              << entryCount << " entries\n";

    uintptr_t stateVectorStorage = 0;

    for (size_t i = 0; i < entryCount; ++i)
    {
        uintptr_t entry = packedBegin + i * 32;
        uint32_t hash = 0;

        if (!reader.Read(entry + 0x08, &hash, sizeof(hash)))
        {
            std::cout << "Could not read component hash.\n";
            return 1;
        }

        if (hash == 0x1B5D5238)
        {
            if (stateVectorStorage != 0 ||
                !reader.Read(entry + 0x10,
                             &stateVectorStorage,
                             sizeof(stateVectorStorage)) ||
                stateVectorStorage == 0)
            {
                std::cout << "Invalid or duplicate StateVectorComponent.\n";
                return 1;
            }
        }
    }

    if (stateVectorStorage == 0)
    {
        std::cout << "StateVectorComponent storage not found.\n";
        return 1;
    }

    std::cout << "StateVectorComponent storage: 0x"
              << std::hex << stateVectorStorage
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
