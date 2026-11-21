
#include "PointerResolver.h"

#include <iostream>
#include <unordered_map>
#include <cstdint>

int main()
{
    std::unordered_map<uintptr_t, uintptr_t> memory = {
        {0x1000, 0x2000},
        {0x2000, 0x3000},
        {0x3050, 0x5000}
    };

    auto readPointer = [&](uintptr_t address,
                           uintptr_t& value) -> bool
    {
        auto it = memory.find(address);

        if (it == memory.end())
            return false;

        value = it->second;
        return true;
    };

    uintptr_t result = PointerResolver::ResolveValue(
        0x1000,
        {0x0, 0x0, 0x50},
        readPointer
    );

    std::cout << "Resolved ClientInstance: 0x"
              << std::hex << result << "\n";

    if (result == 0x5000)
    {
        std::cout << "RESOLVER CLASS PASSED!\n";
        return 0;
    }

    return 1;
}
