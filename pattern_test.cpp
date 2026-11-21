
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
            // -1 means wildcard: accept any byte.
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
    std::vector<uint8_t> bytes = {
        0x90, 0x90, 0x48, 0x8B,
        0x12, 0x34, 0x48, 0x85,
        0xC0, 0x90
    };

    std::vector<int> signature = {
        0x48, 0x8B, -1, -1,
        0x48, 0x85, 0xC0
    };

    int offset = FindPattern(bytes, signature);

    std::cout << "Pattern offset: "
              << offset << "\n";
}
