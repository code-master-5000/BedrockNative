
#pragma once

#include <cstdint>
#include <cstring>
#include <cstddef>

class RelativeAddress
{
public:
    static uintptr_t Resolve(
        uintptr_t instructionAddress,
        const uint8_t* instruction,
        size_t instructionLength,
        size_t displacementOffset)
    {
        if (!instruction ||
            instructionLength < 4 ||
            displacementOffset > instructionLength - 4)
        {
            return 0;
        }

        int32_t displacement = 0;

        std::memcpy(
            &displacement,
            instruction + displacementOffset,
            sizeof(displacement)
        );

        uintptr_t nextInstruction =
            instructionAddress + instructionLength;

        return static_cast<uintptr_t>(
            static_cast<int64_t>(nextInstruction) +
            displacement
        );
    }
};
