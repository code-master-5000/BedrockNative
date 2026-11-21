
#include "RelativeAddress.h"
#include <iostream>

int main()
{
    uint8_t instruction[] = {
        0x48, 0x8B, 0x05,
        0x20, 0x00, 0x00, 0x00
    };

    uintptr_t result = RelativeAddress::Resolve(
        0x1000,
        instruction,
        sizeof(instruction),
        3
    );

    std::cout << "Resolved: 0x"
              << std::hex << result << "\n";

    if (result == 0x1027)
    {
        std::cout << "TEST PASSED!\n";
        return 0;
    }

    std::cout << "TEST FAILED!\n";
    return 1;
}
