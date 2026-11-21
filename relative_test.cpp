
#include <iostream>
#include <cstdint>
#include <cstring>

int main()
{
    // Example instruction:
    // 48 8B 05 20 00 00 00
    // mov rax, [rip + 0x20]
    unsigned char instruction[] = {
        0x48, 0x8B, 0x05,
        0x20, 0x00, 0x00, 0x00
    };

    uintptr_t instructionAddress = 0x1000;

    int32_t displacement = 0;

    // The signed displacement starts at byte 3.
    std::memcpy(
        &displacement,
        instruction + 3,
        sizeof(displacement)
    );

    uintptr_t nextInstruction =
        instructionAddress + sizeof(instruction);

    uintptr_t target = static_cast<uintptr_t>(
        static_cast<int64_t>(nextInstruction) +
        displacement
    );

    std::cout << "Instruction: 0x"
              << std::hex << instructionAddress << "\n";

    std::cout << "Displacement: 0x"
              << displacement << "\n";

    std::cout << "Resolved target: 0x"
              << target << "\n";
}
