#include "MemoryReader.h"
#include "ModuleScanner.h"
#include "RelativeAddress.h"
#include "PointerResolver.h"
#include "EntityTracker.h"
#include "EntityCache.h"
#include "StateVectorDiscovery.h"

#include <conio.h>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>


constexpr uint32_t Fnv1a32(const char* text)
{
    uint32_t hash = 2166136261u;

    while (*text)
    {
        hash ^= static_cast<unsigned char>(*text++);
        hash *= 16777619u;
    }

    return hash;
}

// Verify against the known StateVectorComponent hash.
static_assert(
    Fnv1a32("StateVectorComponent") == 0x1B5D5238,
    "Unexpected EnTT hash."
);

constexpr uint32_t AabbShapeComponentHash =
    Fnv1a32("AABBShapeComponent");


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

    // Discover AABBShapeComponent.
    uintptr_t aabbShapeStorage = 0;
    size_t aabbPoolCount = 0;

    std::cout << "AABBShapeComponent hash candidate: 0x"
              << std::hex << AabbShapeComponentHash
              << std::dec << "\n";

    if (FindComponentStorage(
            reader,
            clientInstance,
            AabbShapeComponentHash,
            aabbShapeStorage,
            aabbPoolCount))
    {
        std::cout << "AABBShapeComponent storage: 0x"
                  << std::hex << aabbShapeStorage
                  << std::dec << "\n";
    }
    else
    {
        std::cout
            << "AABBShapeComponent pool not found "
            << "(or discovery failed).\n";
    }

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


    // Create our entity cache.
    EntityCache cache;

    std::vector<TrackedEntity> entities;
    EntityScanStats stats{};

    std::cout << "\n=== CONTINUOUS ENTITY TRACKING ===\n";
    std::cout << "Press Q to stop tracking.\n";

    size_t scanCount = 0;
    size_t consecutiveFailures = 0;

    while (true)
    {
        // Check whether Minecraft is still running.
        ProcessStatus processStatus =
            reader.GetProcessStatus();

        if (processStatus == ProcessStatus::Exited)
        {
            std::cout << "\nMinecraft has closed.\n";
            break;
        }

        if (processStatus == ProcessStatus::Unknown)
        {
            std::cout << "\nCannot determine process status.\n";
            break;
        }

        // Allow the user to stop tracking.
        if (_kbhit())
        {
            int key = _getch();

            if (key == 'q' || key == 'Q')
            {
                std::cout << "\nTracking stopped manually.\n";
                break;
            }
        }

        // Scan entities.
        if (!tracker.Enumerate(entities, stats))
        {
            ++consecutiveFailures;

            // Avoid flooding the console with errors.
            if (consecutiveFailures == 1 ||
                consecutiveFailures % 20 == 0)
            {
                std::cout
                    << "\nScan rejected. Previous cache retained.\n"
                    << "Consecutive failures: "
                    << consecutiveFailures << "\n"
                    << "Invalid mappings: "
                    << stats.invalidMappings << "\n"
                    << "Position failures: "
                    << stats.positionReadFailures << "\n"
                    << "Changed during read: "
                    << stats.changedDuringRead << "\n";
            }

            Sleep(250);
            continue;
        }

        consecutiveFailures = 0;
        ++scanCount;

        // Only update the cache with a complete snapshot.
        EntityChanges changes = cache.Update(entities);

        // Report newly appeared entities.
        for (const auto& entity : changes.appeared)
        {
            std::cout << "\n+ Entity appeared: 0x"
                      << std::hex
                      << std::setw(8)
                      << std::setfill('0')
                      << entity.id
                      << std::setfill(' ')
                      << std::dec
                      << " at "
                      << entity.position.x << ", "
                      << entity.position.y << ", "
                      << entity.position.z
                      << "\n";
        }

        // Report disappeared entities.
        for (const auto& entity : changes.disappeared)
        {
            std::cout << "\n- Entity disappeared: 0x"
                      << std::hex
                      << std::setw(8)
                      << std::setfill('0')
                      << entity.id
                      << std::setfill(' ')
                      << std::dec
                      << "\n";
        }

        // Print a status update every 20 successful scans.
        if (scanCount % 20 == 0)
        {
            std::cout
                << "\rScans: " << scanCount
                << " | Cached entities: " << cache.Count()
                << " | Live entries: "
                << stats.nonTombstoneEntries
                << std::flush;
        }

        Sleep(250);
    }

    // Release cached memory and the process handle.
    cache.Reset();
    reader.Detach();

    std::cout << "\nEntity cache cleared.\n";
    std::cout << "Tracking stopped.\n";

    return 0;
}
