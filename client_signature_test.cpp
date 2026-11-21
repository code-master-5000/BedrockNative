
#include "MemoryReader.h"
#include "ModuleScanner.h"
#include "SignatureScanner.h"

#include <iostream>
#include <cstdint>

int main()
{
    MemoryReader reader;

    if (!reader.Attach())
    {
        std::cout << "Minecraft not found.\n";
        return 1;
    }

    std::cout << "Minecraft base: 0x"
              << std::hex
              << reader.GetBaseAddress() << "\n";

    // Beginning of Borion's historical
    // ClientInstance signature.
    //
    // -1 represents a wildcard byte.

    SignatureScanner::Pattern pattern = {
        0x48, 0x8B, 0x15,
        -1, -1, -1, -1,
        0x4C, 0x8B, 0x02,
        0x4C, 0x89, 0x06,
        0x40, 0x84, 0xFF,
        0x74, -1,
        0x48, 0x8B, 0xCD,
        0xE8, -1, -1, -1, -1,
        0x48, 0x8B, 0xC6
    };

    std::cout << "Scanning Minecraft .text...\n";

    auto matches = ModuleScanner::ScanText(
        reader,
        pattern
    );

    std::cout << std::dec
              << "Matches found: "
              << matches.size() << "\n";

    for (uintptr_t address : matches)
    {
        std::cout << "Match: 0x"
                  << std::hex << address << "\n";

        std::cout << "RVA: 0x"
                  << address - reader.GetBaseAddress()
                  << "\n";
    }

    reader.Detach();

    std::cout << "SCAN COMPLETE!\n";
    return 0;
}
