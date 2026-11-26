
#pragma once

#include "MemoryReader.h"

#include <cstdint>
#include <cstddef>

bool FindComponentStorage(
    const MemoryReader& reader,
    uintptr_t clientInstance,
    uint32_t componentHash,
    uintptr_t& storage,
    size_t& poolCount
);

bool FindStateVectorStorage(
    const MemoryReader& reader,
    uintptr_t clientInstance,
    uintptr_t& storage,
    size_t& poolCount
);
