
#include "PointerResolver.h"

#include <iostream>
#include <unordered_map>
#include <cstdint>

int main()
{
    std::unordered_map<uintptr_t, uintptr_t> memory = {
        {0x1027, 0x5000},
        {0x5010, 0x7000},
        {0x7008, 42}
    };

    auto readPointer = [&](uintptr_t address,
                           uintptr_t& result) -> bool
    {
        auto it = memory.find(address);

        if (it == memory.end())
            return false;

        result = it->second;
        return true;
    };

    uintptr_t finalAddress = PointerResolver::Resolve(
        0x1027,
        {0x10, 0x08},
        readPointer
    );

    uintptr_t value = 0;

    if (!readPointer(finalAddress, value))
    {
        std::cout << "Read failed.\n";
        return 1;
    }

    std::cout << "Final address: 0x"
              << std::hex << finalAddress << "\n";

    std::cout << "Value: "
              << std::dec << value << "\n";

    if (finalAddress == 0x7008 && value == 42)
    {
        std::cout << "POINTER RESOLVER PASSED!\n";
        return 0;
    }

    return 1;
}