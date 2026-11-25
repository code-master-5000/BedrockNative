#include <iostream>
#include <vector>
#include <cstdint>

#include "SignatureScanner.h"

int main()
{
    // Simulated Minecraft machine-code bytes.
    std::vector<uint8_t> code = {
        0x90, 0x90, 0x90,
        0x4C, 0x89, 0x3D,
        0x12, 0x34, 0x56, 0x78,
        0x4D, 0x85, 0xFF,
        0x90, 0x90
    };

    // -1 means any byte is accepted.
    SignatureScanner::Pattern pattern = {
        0x4C, 0x89, 0x3D,
        -1, -1, -1, -1,
        0x4D, 0x85, 0xFF
    };

    auto matches = SignatureScanner::FindAll(code, pattern);

    std::cout << "PlatformGameCore signature test\n";
    std::cout << "Matches: " << matches.size() << "\n";

    for (auto offset : matches)
    {
        std::cout << "Match offset: " << offset << "\n";
    }

    if (matches.size() == 1 && matches[0] == 3)
    {
        std::cout << "TEST PASSED!\n";
        return 0;
    }

    std::cout << "TEST FAILED!\n";
    return 1;
}
