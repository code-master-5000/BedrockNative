
#include "StateVectorDiscovery.h"

#include <limits>

namespace
{
    constexpr uintptr_t EntityRegistryOffset = 0x268;
    constexpr uintptr_t ComponentPoolsOffset = 0x78;

    constexpr uintptr_t PackedBeginOffset = 0x20;
    constexpr uintptr_t PackedEndOffset = 0x28;
    constexpr uintptr_t PackedCapacityOffset = 0x30;

    constexpr size_t PoolEntryStride = 32;
    constexpr size_t MaxComponentPools = 4096;

    constexpr uint32_t StateVectorHash = 0x1B5D5238;

    bool AddOffset(
        uintptr_t base,
        uintptr_t offset,
        uintptr_t& result)
    {
        if (base == 0 ||
            base > (std::numeric_limits<uintptr_t>::max)() - offset)
        {
            return false;
        }

        result = base + offset;
        return true;
    }
}

bool FindStateVectorStorage(
    const MemoryReader& reader,
    uintptr_t clientInstance,
    uintptr_t& storage,
    size_t& poolCount)
{
    storage = 0;
    poolCount = 0;

    if (!reader.IsAttached() || clientInstance == 0)
        return false;

    // Find EntityRegistry.
    uintptr_t address = 0;

    if (!AddOffset(
            clientInstance,
            EntityRegistryOffset,
            address))
    {
        return false;
    }

    uintptr_t entityRegistry = 0;

    if (!reader.Read(
            address,
            &entityRegistry,
            sizeof(entityRegistry)) ||
        entityRegistry == 0)
    {
        return false;
    }

    // Find component pools.
    uintptr_t pools = 0;

    if (!AddOffset(
            entityRegistry,
            ComponentPoolsOffset,
            pools))
    {
        return false;
    }

    uintptr_t packedBeginAddress = 0;
    uintptr_t packedEndAddress = 0;
    uintptr_t packedCapacityAddress = 0;

    if (!AddOffset(pools, PackedBeginOffset, packedBeginAddress) ||
        !AddOffset(pools, PackedEndOffset, packedEndAddress) ||
        !AddOffset(pools, PackedCapacityOffset, packedCapacityAddress))
    {
        return false;
    }

    uintptr_t packedBegin = 0;
    uintptr_t packedEnd = 0;
    uintptr_t packedCapacity = 0;

    if (!reader.Read(packedBeginAddress,
                     &packedBegin, sizeof(packedBegin)) ||
        !reader.Read(packedEndAddress,
                     &packedEnd, sizeof(packedEnd)) ||
        !reader.Read(packedCapacityAddress,
                     &packedCapacity, sizeof(packedCapacity)))
    {
        return false;
    }

    if (packedBegin == 0 ||
        packedEnd < packedBegin ||
        packedCapacity < packedEnd)
    {
        return false;
    }

    uintptr_t packedBytes = packedEnd - packedBegin;

    if (packedBytes % PoolEntryStride != 0)
        return false;

    size_t count = static_cast<size_t>(
        packedBytes / PoolEntryStride
    );

    if (count > MaxComponentPools)
        return false;

    uintptr_t foundStorage = 0;

    // Search for StateVectorComponent.
    for (size_t i = 0; i < count; ++i)
    {
        uintptr_t entry = 0;

        uintptr_t offset =
            static_cast<uintptr_t>(i) * PoolEntryStride;

        if (!AddOffset(packedBegin, offset, entry))
            return false;

        uintptr_t hashAddress = 0;
        uintptr_t storageAddress = 0;

        if (!AddOffset(entry, 0x08, hashAddress) ||
            !AddOffset(entry, 0x10, storageAddress))
        {
            return false;
        }

        uint32_t hash = 0;

        if (!reader.Read(hashAddress, &hash, sizeof(hash)))
            return false;

        if (hash != StateVectorHash)
            continue;

        if (foundStorage != 0)
            return false;

        if (!reader.Read(
                storageAddress,
                &foundStorage,
                sizeof(foundStorage)) ||
            foundStorage == 0)
        {
            return false;
        }
    }

    if (foundStorage == 0)
        return false;

    storage = foundStorage;
    poolCount = count;

    return true;
}
