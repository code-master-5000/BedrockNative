
#include "SignatureScanner.h"

#include <iostream>
#include <vector>
#include <cstdint>
#include <algorithm>

int main()
{
    // Simulated memory containing 32 bytes.
    std::vector<uint8_t> memory(32, 0x90);

    // Put a four-byte pattern across a chunk boundary.
    memory[14] = 0xDE;
    memory[15] = 0xAD;
    memory[16] = 0xBE;
    memory[17] = 0xEF;

    SignatureScanner::Pattern pattern = {
        0xDE, 0xAD, 0xBE, 0xEF
    };

    const size_t chunkSize = 16;
    size_t totalMatches = 0;

    for (size_t offset = 0;
         offset < memory.size();
         offset += chunkSize)
    {
        size_t ownedSize = std::min(
            chunkSize, memory.size() - offset
        );

        size_t overlap = std::min(
            pattern.size() - 1,
            memory.size() - offset - ownedSize
        );

        std::vector<uint8_t> chunk(
            memory.begin() + offset,
            memory.begin() + offset + ownedSize + overlap
        );

        auto matches =
            SignatureScanner::FindAll(chunk, pattern);

        for (size_t match : matches)
        {
            if (match < ownedSize)
            {
                std::cout << "Pattern found at offset: "
                          << offset + match << "\n";

                ++totalMatches;
            }
        }
    }

    std::cout << "Total matches: "
              << totalMatches << "\n";

    if (totalMatches == 1)
    {
        std::cout << "BOUNDARY TEST PASSED!\n";
        return 0;
    }

    std::cout << "BOUNDARY TEST FAILED!\n";
    return 1;
}
