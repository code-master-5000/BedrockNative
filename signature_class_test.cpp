
#include "MemoryReader.h"
#include "SignatureScanner.h"

#include <iostream>
#include <vector>
#include <cstdint>

int main()
{
    std::cout << "SignatureScanner Class Test\n\n";

    // First test: known data with two matches.
    std::vector<uint8_t> sample = {
        0x90, 0x48, 0x8B, 0x12,
        0x90, 0x48, 0x8B, 0x34
    };

    auto sampleMatches = SignatureScanner::FindAll(
        sample, {0x48, 0x8B, -1}
    );

    std::cout << "Sample matches: "
              << sampleMatches.size() << "\n";

    for (size_t offset : sampleMatches)
    {
        std::cout << "Sample offset: "
                  << offset << "\n";
    }

    // Second test: real Minecraft memory.
    MemoryReader reader;

    if (!reader.Attach())
    {
        std::cout << "Minecraft not found.\n";
        return 1;
    }

    uintptr_t base = reader.GetBaseAddress();

    std::vector<uint8_t> bytes(4096);

    if (!reader.Read(base, bytes.data(), bytes.size()))
    {
        std::cout << "Minecraft memory read failed.\n";
        return 1;
    }

    auto matches = SignatureScanner::FindAll(
        bytes, {0x4D, 0x5A}
    );

    std::cout << "\nMinecraft base: 0x"
              << std::hex << base << "\n";

    std::cout << std::dec
              << "MZ matches: "
              << matches.size() << "\n";

    for (size_t offset : matches)
    {
        std::cout << "Match offset: 0x"
                  << std::hex << offset
                  << "\n";

        std::cout << "Match address: 0x"
                  << base + offset
                  << std::dec << "\n";
    }

    if (matches.empty())
    {
        std::cout << "No matches found.\n";
    }
    else if (matches.size() == 1)
    {
        std::cout << "Unique match found!\n";
    }
    else
    {
        std::cout << "Multiple matches found.\n";
    }

    reader.Detach();
    return 0;
}
