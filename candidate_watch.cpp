
#include "MemoryReader.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdint>

int main()
{
    constexpr uintptr_t candidate = 0x20E019E085A;

    MemoryReader reader;

    std::cout << "Bedrock Candidate Watch\n\n";

    if (!reader.Attach())
    {
        std::cout << "Cannot attach to Minecraft.\n";
        return 1;
    }

    std::cout << "Attached to Minecraft.\n";
    std::cout << "Watching address: 0x"
              << std::hex << candidate
              << std::dec << "\n";

    unsigned char previous = 0;

    if (!reader.Read(candidate, &previous, 1))
    {
        std::cout << "Cannot read candidate address.\n";
        std::cout << "The memory location may have moved.\n";
        return 1;
    }

    std::cout << "Initial value: "
              << static_cast<int>(previous) << "\n";

    std::cout << "Watching for changes...\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    while (true)
    {
        unsigned char current = 0;

        if (!reader.Read(candidate, &current, 1))
        {
            std::cout << "Memory read failed.\n";
            break;
        }

        if (current != previous)
        {
            std::cout << "Value changed: "
                      << static_cast<int>(previous)
                      << " -> "
                      << static_cast<int>(current)
                      << "\n";

            previous = current;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(100)
        );
    }

    return 0;
}
