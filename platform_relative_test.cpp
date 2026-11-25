#include <iostream>
#include <cstdint>
#include "RelativeAddress.h"

int main()
{
    // Simulated instruction at address 0x1000.
    uintptr_t instructionAddress = 0x1000;

    uint8_t instruction[] = {
        0x4C, 0x89, 0x3D,
        0xF9, 0x00, 0x00, 0x00
    };

    uintptr_t resolved = RelativeAddress::Resolve(
        instructionAddress,
        instruction,
        7,
        3
    );

    std::cout << "Instruction address: 0x"
              << std::hex << instructionAddress << "\n";

    std::cout << "Resolved address: 0x"
              << resolved << "\n";

    if (resolved == 0x1100)
    {
        std::cout << "TEST PASSED!\n";
        return 0;
    }

    std::cout << "TEST FAILED!\n";
    return 1;
}
