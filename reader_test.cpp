
#include "MemoryReader.h"
#include <iostream>
#include <iomanip>

int main()
{
    std::cout << "MemoryReader Class Test\n\n";

    MemoryReader reader;

    if (!reader.Attach())
    {
        std::cout << "Could not attach to Minecraft.\n";
        std::cin.get();
        return 1;
    }

    std::cout << "Attached successfully!\n";

    uintptr_t base = reader.GetBaseAddress();

    std::cout << "Module base: 0x"
              << std::hex << base
              << std::dec << "\n";

    unsigned char bytes[2] = {};

    if (reader.Read(base, bytes, sizeof(bytes)))
    {
        std::cout << "Memory read SUCCESS!\n";
        std::cout << "First bytes: "
                  << std::hex
                  << static_cast<int>(bytes[0]) << " "
                  << static_cast<int>(bytes[1])
                  << std::dec << "\n";
    }
    else
    {
        std::cout << "Memory read FAILED.\n";
    }

    reader.Detach();

    std::cout << "\nTest complete.\n";
    std::cin.get();

    return 0;
}
