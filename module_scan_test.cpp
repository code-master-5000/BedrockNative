
#include "ModuleScanner.h"

#include <iostream>
#include <cstdint>

int main()
{
    MemoryReader reader;

    if (!reader.Attach())
    {
        std::cout << "Cannot attach to Minecraft.\n";
        return 1;
    }

    ModuleScanner::Section text = {};

    if (!ModuleScanner::FindTextSection(reader, text))
    {
        std::cout << "Could not find .text.\n";
        return 1;
    }

    std::cout << "Found .text automatically!\n";

    std::cout << "Section address: 0x"
              << std::hex << text.address << "\n";

    std::cout << "Section size: "
              << std::dec << text.size
              << " bytes\n";

    SignatureScanner::Pattern pattern = {
        0x8B, 0x01, 0x4C, 0x8D,
        0x04, 0x02, 0xF0, 0x44,
        0x0F, 0xB1, 0x01, 0x75,
        0xF5, 0x44, 0x89, 0xC0
    };

    auto matches = ModuleScanner::ScanText(
        reader,
        pattern,
        256 * 1024
    );

    std::cout << "\nMatches: "
              << matches.size() << "\n";

    for (uintptr_t address : matches)
    {
        std::cout << "Found at: 0x"
                  << std::hex << address << "\n";
    }

    if (matches.size() == 1 &&
        matches[0] == text.address)
    {
        std::cout << "\nTEST PASSED!\n";
    }
    else
    {
        std::cout << "\nUnexpected match results.\n";
        return 1;
    }

    return 0;
}
