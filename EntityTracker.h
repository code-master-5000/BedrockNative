
#pragma once

#include "MemoryReader.h"

#include <cstdint>
#include <cstddef>
#include <vector>

// Position returned by the entity tracker.
struct EntityPosition
{
    float x;
    float y;
    float z;
};

static_assert(sizeof(EntityPosition) == 12);

// One successfully resolved entity.
struct TrackedEntity
{
    uint32_t id = 0;
    size_t slot = 0;
    EntityPosition position{};
};

// Diagnostic information from each scan.
struct EntityScanStats
{
    size_t nonTombstoneEntries = 0;
    size_t invalidMappings = 0;
    size_t positionReadFailures = 0;
    size_t changedDuringRead = 0;
};

class EntityTracker
{
public:
    explicit EntityTracker(const MemoryReader& reader);

    bool Initialize(uintptr_t stateVectorStorage);

    bool Enumerate(
        std::vector<TrackedEntity>& entities,
        EntityScanStats& stats
    ) const;

private:
    struct Layout
    {
        uintptr_t packedBegin = 0;
        uintptr_t packedEnd = 0;
        uintptr_t packedCapacity = 0;

        uintptr_t sparseBegin = 0;
        uintptr_t sparseEnd = 0;

        uintptr_t dataPagesBegin = 0;
        uintptr_t dataPagesEnd = 0;
    };

    bool ReadLayout(Layout& layout) const;

    bool ValidateMapping(
        const Layout& layout,
        uint32_t packedId,
        size_t expectedSlot
    ) const;

    bool ReadPosition(
        const Layout& layout,
        size_t slot,
        EntityPosition& position
    ) const;

    const MemoryReader& reader_;

    uintptr_t stateVectorStorage_ = 0;

    // Minecraft / EnTT layout assumptions.
    static constexpr uint32_t EntityMask = 0x3FFFF;
    static constexpr uint32_t TombstoneVersion = 0x3FFF;

    static constexpr size_t SparsePageSize = 2048;
    static constexpr size_t ComponentPageSize = 128;

    // StateVectorComponent:
    // position + old position + velocity.
    static constexpr size_t ComponentStride = 36;
};
