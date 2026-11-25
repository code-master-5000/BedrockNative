#include <iostream>
#include <cstdint>
#include <map>
#include "PointerResolver.h"

int main()
{
    // Simulated addresses, not actual Minecraft pointers.
    const uintptr_t globalAddress = 0x1100;
    const uintptr_t winMain       = 0x2000;
    const uintptr_t platform      = 0x3000;
    const uintptr_t gameHolder    = 0x4000;
    const uintptr_t minecraftGame = 0x5000;

    // Simulated remote memory:
    // address -> pointer value.
    std::map<uintptr_t, uintptr_t> memory = {
        {globalAddress,       winMain},
        {winMain + 0x8,       platform},
        {platform + 0x118,    gameHolder},
        {gameHolder + 0x48,   minecraftGame}
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
        globalAddress,
        {0x0, 0x8, 0x118, 0x48},
        readPointer
    );

    std::cout << "MinecraftGame address: 0x"
              << std::hex << result << "\n";

    if (result == minecraftGame)
    {
        std::cout << "TEST PASSED!\n";
        return 0;
    }

    std::cout << "TEST FAILED!\n";
    return 1;
}
