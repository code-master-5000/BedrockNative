
#pragma once

#include <cstdint>
#include <vector>

class PointerResolver
{
public:
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
};
