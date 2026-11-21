
#include "MemoryReader.h"
#include "ModuleScanner.h"
#include "SignatureScanner.h"
#include "RelativeAddress.h"

#include <iostream>
#include <cstdint>
#include <iomanip>

int main()
{
    MemoryReader reader;

    if (!reader.Attach())
    {
        std::cout << "Minecraft not found.\n";
        return 1;
    }

    uintptr_t base = reader.GetBaseAddress();

    std::cout << "Minecraft base: 0x"
              << std::hex << base << "\n";

    // Historical Borion ClientInstance signature.
    // -1 means any byte (wildcard).
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

    std::cout << "Scanning Minecraft .text...\n";

    // Scan once for the 7-byte pattern.
    auto shortMatches = ModuleScanner::ScanText(
        reader,
        shortPattern
    );

    std::cout << std::dec
              << "7-byte pattern: "
              << shortMatches.size()
              << " matches\n";

    // Display the first five matching RVAs.
    for (size_t i = 0;
         i < shortMatches.size() && i < 5;
         ++i)
    {
        std::cout << "  RVA: 0x"
                  << std::hex
                  << (shortMatches[i] - base)
                  << "\n";
    }

    std::cout << "\nFirst 5 instruction samples:\n";

    // Inspect the first five matches.
    for (size_t i = 0;
         i < shortMatches.size() && i < 5;
         ++i)
    {
        uint8_t bytes[16] = {};

        if (!reader.Read(
                shortMatches[i],
                bytes,
                sizeof(bytes)))
        {
            std::cout << "Instruction read failed.\n";
            continue;
        }

        // Print the instruction bytes.
        std::cout << "RVA 0x"
                  << std::hex
                  << (shortMatches[i] - base)
                  << ": ";

        for (uint8_t b : bytes)
        {
            std::cout << std::setw(2)
                      << std::setfill('0')
                      << static_cast<unsigned>(b)
                      << " ";
        }

        std::cout << "\n";

        // Resolve the RIP-relative address.
        uintptr_t target = RelativeAddress::Resolve(
            shortMatches[i],
            bytes,
            7,
            3
        );

        std::cout << "  Referenced address: 0x"
                  << std::hex
                  << target << "\n";

        std::cout << "  Referenced RVA: 0x"
                  << std::hex
                  << (target - base) << "\n";

        // Read 8 bytes from the referenced address.
        uint64_t storedValue = 0;

        if (reader.Read(
                target,
                &storedValue,
                sizeof(storedValue)))
        {
            std::cout << "  Stored 8-byte value: 0x"
                      << std::hex
                      << storedValue << "\n";

            // Check whether the stored value
            // points to readable memory.
            uint8_t probe = 0;

            if (storedValue != 0 &&
                reader.Read(
                    static_cast<uintptr_t>(storedValue),
                    &probe,
                    sizeof(probe)))
            {
                std::cout
                    << "  Pointer target: READABLE\n";

                std::cout << "  First byte: 0x"
                          << std::hex
                          << static_cast<unsigned>(probe)
                          << "\n";
            }
            else
            {
                std::cout
                    << "  Pointer target: NOT READABLE\n";
            }
        }
        else
        {
            std::cout
                << "  Referenced memory unreadable.\n";
        }

    } // End of five-sample inspection loop

    // Test the longer historical patterns.
    auto testPattern = [&](const char* name,
                           const SignatureScanner::Pattern& p)
    {
        auto matches = ModuleScanner::ScanText(reader, p);

        std::cout << name << ": "
                  << std::dec
                  << matches.size()
                  << " matches\n";
    };

    testPattern("10-byte pattern", mediumPattern);
    testPattern("13-byte pattern", longerPattern);
    testPattern("29-byte pattern", pattern);

    reader.Detach();

    std::cout << "SCAN COMPLETE!\n";

    return 0;

} // End of main()
