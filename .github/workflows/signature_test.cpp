
#include "MemoryReader.h"
#include <iostream>
#include <vector>
#include <cstdint>

int FindPattern(
    const std::vector<uint8_t>& data,
    const std::vector<int>& pattern)
{
    if (pattern.empty() || pattern.size() > data.size())
        return -1;

    for (size_t i = 0;
         i <= data.size() - pattern.size();
         ++i)
    {
        bool match = true;

        for (size_t j = 0; j < pattern.size(); ++j)
        {
            if (pattern[j] != -1 &&
                data[i + j] != pattern[j])
            {
                match = false;
                break;
            }
        }

        if (match)
            return static_cast<int>(i);
    }

    return -1;
}

int main()
{
    MemoryReader reader;

    if (!reader.Attach())
    {
        std::cout << "Minecraft not found.\n";
        return 1;
    }

    uintptr_t base = reader.GetBaseAddress();

    // Read only the first 4096 bytes.
    std::vector<uint8_t> bytes(4096);

    if (!reader.Read(base, bytes.data(), bytes.size()))
    {
        std::cout << "Memory read failed.\n";
        return 1;
    }

    // Known Windows executable signature: MZ
    std::vector<int> signature = {0x4D, 0x5A};

    int offset = FindPattern(bytes, signature);

    if (offset == -1)
    {
        std::cout << "Signature not found.\n";
        return 1;
    }

    std::cout << "Minecraft base: 0x"
              << std::hex << base << "\n";

    std::cout << "Signature offset: 0x"
              << offset << "\n";

    std::cout << "Signature address: 0x"
              << base + static_cast<uintptr_t>(offset)
              << "\n";
}
