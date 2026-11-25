
#include "EntityTracker.h"

#include <limits>

static_assert(sizeof(uintptr_t) == 8,
              "EntityTracker requires a 64-bit build.");

EntityTracker::EntityTracker(const MemoryReader& reader)
    : reader_(reader)
{
}

bool EntityTracker::Initialize(uintptr_t storage)
{
    stateVectorStorage_ = 0;

    if (!reader_.IsAttached() || storage == 0)
        return false;

    stateVectorStorage_ = storage;

    Layout layout{};

    if (!ReadLayout(layout))
    {
        stateVectorStorage_ = 0;
        return false;
    }

    return true;
}

bool EntityTracker::ReadLayout(Layout& layout) const
{
    if (stateVectorStorage_ == 0)
        return false;

    // Packed entity array.
    if (!reader_.Read(
            stateVectorStorage_ + 0x20,
            &layout.packedBegin,
            sizeof(uintptr_t)) ||
        !reader_.Read(
            stateVectorStorage_ + 0x28,
            &layout.packedEnd,
            sizeof(uintptr_t)) ||
        !reader_.Read(
            stateVectorStorage_ + 0x30,
            &layout.packedCapacity,
            sizeof(uintptr_t)))
    {
        return false;
    }

    // Sparse page array.
    if (!reader_.Read(
            stateVectorStorage_ + 0x08,
            &layout.sparseBegin,
            sizeof(uintptr_t)) ||
        !reader_.Read(
            stateVectorStorage_ + 0x10,
            &layout.sparseEnd,
            sizeof(uintptr_t)))
    {
        return false;
    }

    // Component data pages.
    if (!reader_.Read(
            stateVectorStorage_ + 0x50,
            &layout.dataPagesBegin,
            sizeof(uintptr_t)) ||
        !reader_.Read(
            stateVectorStorage_ + 0x58,
            &layout.dataPagesEnd,
            sizeof(uintptr_t)))
    {
        return false;
    }

    if (layout.packedBegin == 0 ||
        layout.packedEnd < layout.packedBegin ||
        layout.packedCapacity < layout.packedEnd)
    {
        return false;
    }

    uintptr_t packedBytes =
        layout.packedEnd - layout.packedBegin;

    if (packedBytes % sizeof(uint32_t) != 0 ||
        packedBytes / sizeof(uint32_t) > 1000000)
    {
        return false;
    }

    if (layout.sparseBegin == 0 ||
        layout.sparseEnd < layout.sparseBegin ||
        (layout.sparseEnd - layout.sparseBegin)
            % sizeof(uintptr_t) != 0)
    {
        return false;
    }

    if (layout.dataPagesBegin == 0 ||
        layout.dataPagesEnd < layout.dataPagesBegin ||
        (layout.dataPagesEnd - layout.dataPagesBegin)
            % sizeof(uintptr_t) != 0)
    {
        return false;
    }

    return true;
}

bool EntityTracker::ValidateMapping(
    const Layout& layout,
    uint32_t packedId,
    size_t expectedSlot
) const
{
    uint32_t entityIndex = packedId & EntityMask;

    size_t sparsePageNumber =
        entityIndex / SparsePageSize;

    size_t sparsePageCount =
        (layout.sparseEnd - layout.sparseBegin)
        / sizeof(uintptr_t);

    if (sparsePageNumber >= sparsePageCount ||
        expectedSlot > EntityMask)
    {
        return false;
    }

    uintptr_t sparsePage = 0;

    if (!reader_.Read(
            layout.sparseBegin +
                sparsePageNumber * sizeof(uintptr_t),
            &sparsePage,
            sizeof(sparsePage)) ||
        sparsePage == 0)
    {
        return false;
    }

    uint32_t mappedEntry = 0;

    uintptr_t entryAddress =
        sparsePage +
        (entityIndex % SparsePageSize) * sizeof(uint32_t);

    if (!reader_.Read(
            entryAddress,
            &mappedEntry,
            sizeof(mappedEntry)))
    {
        return false;
    }

    // Check entity generation.
    if ((mappedEntry & ~EntityMask) !=
        (packedId & ~EntityMask))
    {
        return false;
    }

    // Check packed position.
    return (mappedEntry & EntityMask) == expectedSlot;
}

bool EntityTracker::ReadPosition(
    const Layout& layout,
    size_t slot,
    EntityPosition& position
) const
{
    size_t pageIndex =
        slot / ComponentPageSize;

    size_t pageCount =
        (layout.dataPagesEnd - layout.dataPagesBegin)
        / sizeof(uintptr_t);

    if (pageIndex >= pageCount)
        return false;

    uintptr_t dataPage = 0;

    if (!reader_.Read(
            layout.dataPagesBegin +
                pageIndex * sizeof(uintptr_t),
            &dataPage,
            sizeof(dataPage)) ||
        dataPage == 0)
    {
        return false;
    }

    uintptr_t offset =
        (slot % ComponentPageSize) * ComponentStride;

    if (dataPage >
        (std::numeric_limits<uintptr_t>::max)() - offset)
    {
        return false;
    }

    uintptr_t positionAddress = dataPage + offset;

    return reader_.Read(
        positionAddress,
        &position,
        sizeof(position));
}


bool EntityTracker::Enumerate(
    std::vector<TrackedEntity>& entities,
    EntityScanStats& stats
) const
{
    stats = {};

    Layout layout{};

    if (!ReadLayout(layout))
        return false;

    size_t entityCount =
        (layout.packedEnd - layout.packedBegin)
        / sizeof(uint32_t);

    std::vector<TrackedEntity> results;
    EntityScanStats currentStats{};

    bool complete = true;

    for (size_t slot = 0; slot < entityCount; ++slot)
    {
        uint32_t packedId = 0;

        uintptr_t address =
            layout.packedBegin + slot * sizeof(uint32_t);

        if (!reader_.Read(
                address,
                &packedId,
                sizeof(packedId)))
        {
            stats = currentStats;
            return false;
        }

        if ((packedId >> 18) == TombstoneVersion)
            continue;

        ++currentStats.nonTombstoneEntries;

        if (!ValidateMapping(layout, packedId, slot))
        {
            ++currentStats.invalidMappings;
            complete = false;
            continue;
        }

        EntityPosition position{};

        if (!ReadPosition(layout, slot, position))
        {
            ++currentStats.positionReadFailures;
            complete = false;
            continue;
        }

        uint32_t confirmId = 0;

        if (!reader_.Read(
                address,
                &confirmId,
                sizeof(confirmId)) ||
            confirmId != packedId)
        {
            ++currentStats.changedDuringRead;
            complete = false;
            continue;
        }

        results.push_back({
            packedId,
            slot,
            position
        });
    }

    stats = currentStats;

    // Never publish an incomplete snapshot.
    if (!complete)
        return false;

    // Replace the previous snapshot only after success.
    entities.swap(results);

    return true;
}
