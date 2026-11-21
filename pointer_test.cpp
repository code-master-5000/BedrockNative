
#include <iostream>
#include <cstdint>
#include <unordered_map>

int main()
{
    // Simulated memory: address -> stored value.
    std::unordered_map<uintptr_t, uintptr_t> memory = {
        {0x1027, 0x5000},
        {0x5010, 0x7000},
        {0x7008, 42}
    };

    // The instruction references a pointer at 0x1027.
    uintptr_t pointerAddress = 0x1027;

    // Read the first pointer.
    uintptr_t objectAddress = memory.at(pointerAddress);

    std::cout << "Object address: 0x"
              << std::hex << objectAddress << "\n";

    // Follow a field at offset 0x10.
    uintptr_t nestedAddress =
        memory.at(objectAddress + 0x10);

    std::cout << "Nested object: 0x"
              << std::hex << nestedAddress << "\n";

    // Read a value at offset 0x8.
    uintptr_t value = memory.at(nestedAddress + 0x8);

    std::cout << std::dec
              << "Final value: " << value << "\n";

    if (value == 42)
    {
        std::cout << "POINTER TEST PASSED!\n";
        return 0;
    }

    return 1;
}
