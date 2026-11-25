#include "MemoryReader.h"
#include "ModuleScanner.h"
#include "RelativeAddress.h"
#include "PointerResolver.h"
#include "EntityTracker.h"
#include "StateVectorDiscovery.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

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


    // Discover StateVectorComponent using our reusable module.
    uintptr_t stateVectorStorage = 0;
    size_t poolCount = 0;

    if (!FindStateVectorStorage(
            reader,
            clientInstance,
            stateVectorStorage,
            poolCount))
    {
        std::cout << "StateVectorComponent discovery failed.\n";
        return 1;
    }

    std::cout << "Component pools: "
              << poolCount << " entries\n";

    std::cout << "StateVectorComponent storage: 0x"
              << std::hex << stateVectorStorage
              << std::dec << "\n";

    // Resolve this entity's component index using EnTT's sparse set.
    constexpr uint32_t entityMask = 0x3FFFF;
    constexpr uintptr_t sparsePageSize = 2048;
    constexpr uintptr_t componentPageSize = 128;

    uint32_t entityIndex = entityId & entityMask;

    uintptr_t sparseBegin = 0;
    uintptr_t sparseEnd = 0;

    if (!reader.Read(stateVectorStorage + 0x08,
                     &sparseBegin, sizeof(sparseBegin)) ||
        !reader.Read(stateVectorStorage + 0x10,
                     &sparseEnd, sizeof(sparseEnd)) ||
        sparseBegin == 0 ||
        sparseEnd < sparseBegin)
    {
        std::cout << "Invalid sparse-page array.\n";
        return 1;
    }

    uintptr_t sparsePageIndex = entityIndex / sparsePageSize;

    if (sparsePageIndex >= (sparseEnd - sparseBegin) / 8)
    {
        std::cout << "Entity has no sparse page.\n";
        return 1;
    }

    uintptr_t sparsePage = 0;

    if (!reader.Read(sparseBegin + sparsePageIndex * 8,
                     &sparsePage, sizeof(sparsePage)) ||
        sparsePage == 0)
    {
        std::cout << "Sparse page is missing.\n";
        return 1;
    }

    uint32_t sparseEntry = 0;

    if (!reader.Read(
            sparsePage + (entityIndex % sparsePageSize) * 4,
            &sparseEntry, sizeof(sparseEntry)) ||
        (sparseEntry & ~entityMask) != (entityId & ~entityMask))
    {
        std::cout << "Entity not present in position storage.\n";
        return 1;
    }

    uintptr_t componentIndex = sparseEntry & entityMask;

    std::cout << "Position component index: "
              << componentIndex << "\n";

    // EnTT stores component values in pages of 128 elements.
    uintptr_t dataPagesBegin = 0;
    uintptr_t dataPagesEnd = 0;

    if (!reader.Read(stateVectorStorage + 0x50,
                     &dataPagesBegin, sizeof(dataPagesBegin)) ||
        !reader.Read(stateVectorStorage + 0x58,
                     &dataPagesEnd, sizeof(dataPagesEnd)) ||
        dataPagesBegin == 0 ||
        dataPagesEnd < dataPagesBegin)
    {
        std::cout << "Invalid component-data pages.\n";
        return 1;
    }

    uintptr_t dataPageIndex = componentIndex / componentPageSize;

    if (dataPageIndex >= (dataPagesEnd - dataPagesBegin) / 8)
    {
        std::cout << "Component data page is missing.\n";
        return 1;
    }

    uintptr_t dataPage = 0;

    if (!reader.Read(dataPagesBegin + dataPageIndex * 8,
                     &dataPage, sizeof(dataPage)) ||
        dataPage == 0)
    {
        std::cout << "Cannot read component data page.\n";
        return 1;
    }

    // StateVectorComponent = position, old position, velocity.
    // Each Vec3 contains three floats, giving a 36-byte component.
    constexpr uintptr_t componentStride = sizeof(Vec3) * 3;

    uintptr_t positionAddress =
        dataPage +
        (componentIndex % componentPageSize) * componentStride;

    std::cout << "Resolved position address: 0x"
              << std::hex << positionAddress
              << std::dec << "\n";

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


    // Enumerate entities through our reusable EntityTracker.

    EntityTracker tracker(reader);

    if (!tracker.Initialize(stateVectorStorage))
    {
        std::cout << "EntityTracker initialization failed.\n";
        return 1;
    }

    std::vector<TrackedEntity> entities;
    EntityScanStats stats{};

    if (!tracker.Enumerate(entities, stats))
    {
        std::cout << "Entity enumeration failed.\n";
        return 1;
    }

    std::cout << "\n=== LIVE ECS ENTITIES ===\n";

    for (const auto& entity : entities)
    {
        std::cout << "Slot " << std::dec << entity.slot
                  << ": Entity ID 0x"
                  << std::hex
                  << std::setw(8)
                  << std::setfill('0')
                  << entity.id
                  << std::setfill(' ')
                  << std::dec << "\n";

        std::cout << "  Position: "
                  << entity.position.x << ", "
                  << entity.position.y << ", "
                  << entity.position.z << "\n";
    }

    std::cout << "\n=== ENTITY SCAN STATISTICS ===\n";
    std::cout << "Non-tombstone entries: "
              << stats.nonTombstoneEntries << "\n";

    std::cout << "Successfully tracked: "
              << entities.size() << "\n";

    std::cout << "Invalid mappings: "
              << stats.invalidMappings << "\n";

    std::cout << "Position read failures: "
              << stats.positionReadFailures << "\n";

    std::cout << "Changed during read: "
              << stats.changedDuringRead << "\n";

    return 0;
}
