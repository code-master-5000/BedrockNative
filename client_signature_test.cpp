
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

    
SignatureScanner::Pattern shortPattern = {
    0x48, 0x8B, 0x15,
    -1, -1, -1, -1
};

SignatureScanner::Pattern mediumPattern = {
    0x48, 0x8B, 0x15,
    -1, -1, -1, -1,
    0x4C, 0x8B, 0x02
};

SignatureScanner::Pattern longerPattern = {
    0x48, 0x8B, 0x15,
    -1, -1, -1, -1,
    0x4C, 0x8B, 0x02,
    0x4C, 0x89, 0x06
};

auto testPattern = [&](const char* name,
                       const SignatureScanner::Pattern& p)
{
    auto matches = ModuleScanner::ScanText(reader, p);

    std::cout << name << ": "
              << std::dec << matches.size()
              << " matches\n";

    // Print only the first five candidate addresses.
    for (size_t i = 0; i < matches.size() && i < 5; ++i)
    {
        std::cout << "  RVA: 0x"
                  << std::hex
                  << (matches[i] - reader.GetBaseAddress())
                  << "\n";
    }
};

testPattern("7-byte pattern", shortPattern);
testPattern("10-byte pattern", mediumPattern);
testPattern("13-byte pattern", longerPattern);
testPattern("29-byte pattern", pattern);

reader.Detach();

std::cout << "SCAN COMPLETE!\n";
return 0;
}
