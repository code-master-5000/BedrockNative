
#include "MemoryReader.h"
#include "SignatureScanner.h"

#include <iostream>
#include <vector>
#include <cstdint>

int main()
{
    MemoryReader reader;

    if (!reader.Attach())
    {
        std::cout << "Minecraft not found.\n";
        return 1;
    }

    uintptr_t base = reader.GetBaseAddress();

    // RVA discovered in our section test.
    uintptr_t textStart = base + 0x1000;

    const size_t chunkSize = 64 * 1024;
    const int chunkCount = 4;

    // Actual bytes from your Minecraft .text section.
    SignatureScanner::Pattern signature = {
        0x8B, 0x01, 0x4C, 0x8D,
        0x04, 0x02, 0xF0, 0x44,
        0x0F, 0xB1, 0x01, 0x75,
        0xF5, 0x44, 0x89, 0xC0
    };

    size_t totalMatches = 0;

    for (int chunk = 0; chunk < chunkCount; ++chunk)
    {
        uintptr_t address =
            textStart + chunk * chunkSize;

        std::vector<uint8_t> bytes(chunkSize);

        if (!reader.Read(
            address, bytes.data(), bytes.size()))
        {
            std::cout << "Chunk " << chunk
                      << ": read failed\n";
            continue;
        }

        auto matches = SignatureScanner::FindAll(
            bytes, signature);

        std::cout << "Chunk " << chunk
                  << ": " << matches.size()
                  << " matches\n";

        for (size_t offset : matches)
        {
            uintptr_t result = address + offset;

            std::cout << "  Address: 0x"
                      << std::hex << result
                      << std::dec << "\n";

            ++totalMatches;
        }
    }

    std::cout << "\nTotal matches: "
              << totalMatches << "\n";

    return 0;
}
