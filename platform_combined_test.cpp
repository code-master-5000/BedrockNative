#include <iostream>
#include <vector>
#include <cstdint>

#include "SignatureScanner.h"
#include "RelativeAddress.h"

int main()
{
    const uintptr_t simulatedBase = 0x1000;

    // Simulated Minecraft instructions.
    std::vector<uint8_t> code = {
        0x90, 0x90, 0x90,
        0x4C, 0x89, 0x3D,
        0xF6, 0x00, 0x00, 0x00,
        0x4D, 0x85, 0xFF,
        0x90, 0x90
    };

    SignatureScanner::Pattern pattern = {
        0x4C, 0x89, 0x3D,
        -1, -1, -1, -1,
        0x4D, 0x85, 0xFF
    };

    auto matches = SignatureScanner::FindAll(code, pattern);

    if (matches.size() != 1)
    {
        std::cout << "Signature test failed\n";
        return 1;
    }

    uintptr_t matchAddress = simulatedBase + matches[0];

    uintptr_t globalAddress = RelativeAddress::Resolve(
        matchAddress,
        code.data() + matches[0],
        7,
        3
    );

    std::cout << "Signature offset: "
              << matches[0] << "\n";

    std::cout << "Instruction address: 0x"
              << std::hex << matchAddress << "\n";

    std::cout << "Resolved global: 0x"
              << globalAddress << "\n";

    if (globalAddress == 0x1100)
    {
        std::cout << "TEST PASSED!\n";
        return 0;
    }

    std::cout << "TEST FAILED!\n";
    return 1;
}
