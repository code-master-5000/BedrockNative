
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <vector>

int main()
{
    // Simulated process memory.
    std::unordered_map<uintptr_t, uintptr_t> memory = {
        {0x1000, 0x2000},
        {0x2000, 0x3000},
        {0x3050, 0x5000}
    };

    uintptr_t current = 0x1000;

    std::vector<uintptr_t> offsets = {
        0x0, 0x0, 0x50
    };

    bool success = true;

    for (uintptr_t offset : offsets)
    {
        auto it = memory.find(current + offset);

        if (it == memory.end())
        {
            success = false;
            break;
        }

        current = it->second;
    }

    if (!success)
    {
        std::cout << "Pointer read failed.\n";
        return 1;
    }

    std::cout << "Resolved: 0x"
              << std::hex << current << "\n";

    if (current == 0x5000)
    {
        std::cout << "BORION POINTER TEST PASSED!\n";
        return 0;
    }

    return 1;
}
