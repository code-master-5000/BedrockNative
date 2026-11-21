
#pragma once

#include "MemoryReader.h"
#include "SignatureScanner.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class ModuleScanner
{
public:
    struct Section
    {
        uintptr_t address = 0;
        size_t size = 0;
    };

    // Find Minecraft's executable-code section.
    static bool FindTextSection(
        const MemoryReader& reader,
        Section& result
    );

    // Find all pattern matches and return absolute addresses.
    // maxBytes = 0 means scan the whole .text section.
    static std::vector<uintptr_t> ScanText(
        const MemoryReader& reader,
        const SignatureScanner::Pattern& pattern,
        size_t maxBytes = 0
    );
};
