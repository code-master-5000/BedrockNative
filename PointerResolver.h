
#pragma once

#include <cstdint>
#include <vector>

class PointerResolver
{
public:

    // Original resolver: Read pointer, then add offset.
    template<typename ReadPointer>
    static uintptr_t Resolve(
        uintptr_t start,
        const std::vector<uintptr_t>& offsets,
        ReadPointer readPointer)
    {
        uintptr_t current = start;

        for (uintptr_t offset : offsets)
        {
            uintptr_t pointer = 0;

            if (!readPointer(current, pointer) ||
                pointer == 0)
            {
                return 0;
            }

            if (offset > UINTPTR_MAX - pointer)
                return 0;

            current = pointer + offset;
        }

        return current;
    }

    // Borion-style resolver: Add offset, then read pointer.
    template<typename ReadPointer>
    static uintptr_t ResolveValue(
        uintptr_t start,
        const std::vector<uintptr_t>& offsets,
        ReadPointer readPointer)
    {
        if (offsets.empty())
            return 0;

        uintptr_t current = start;

        for (uintptr_t offset : offsets)
        {
            if (offset > UINTPTR_MAX - current)
                return 0;

            uintptr_t value = 0;

            if (!readPointer(current + offset, value) ||
                value == 0)
            {
                return 0;
            }

            current = value;
        }

        return current;
    }
};
