
#pragma once

#include "MemoryReader.h"

#include <cstdint>
#include <cstddef>

bool FindStateVectorStorage(
    const MemoryReader& reader,
    uintptr_t clientInstance,
    uintptr_t& storage,
    size_t& poolCount
);
