
#include "MemoryReader.h"

#include <windows.h>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <iomanip>

int main()
{
    MemoryReader reader;

    if (!reader.Attach())
    {
        std::cout << "Cannot attach to Minecraft.\n";
        return 1;
    }

    uintptr_t base = reader.GetBaseAddress();

    // Read the DOS header.
    IMAGE_DOS_HEADER dos = {};

    if (!reader.Read(base, &dos, sizeof(dos)) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE)
    {
        std::cout << "Invalid DOS header.\n";
        return 1;
    }

    // Locate the PE header.
    if (dos.e_lfanew <= 0 ||
        dos.e_lfanew > 4096)
    {
        std::cout << "Invalid PE offset.\n";
        return 1;
    }

    uintptr_t ntAddress =
        base + static_cast<uintptr_t>(dos.e_lfanew);

    IMAGE_NT_HEADERS64 nt = {};

    if (!reader.Read(ntAddress, &nt, sizeof(nt)) ||
        nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        std::cout << "Invalid PE header.\n";
        return 1;
    }

    std::cout << "Minecraft base: 0x"
              << std::hex << base << "\n";

    std::cout << std::dec
              << "Number of sections: "
              << nt.FileHeader.NumberOfSections << "\n\n";

    uintptr_t sectionTable =
        ntAddress +
        sizeof(DWORD) +
        sizeof(IMAGE_FILE_HEADER) +
        nt.FileHeader.SizeOfOptionalHeader;

    for (unsigned int i = 0;
         i < nt.FileHeader.NumberOfSections && i < 96;
         ++i)
    {
        IMAGE_SECTION_HEADER section = {};

        if (!reader.Read(
            sectionTable + i * sizeof(section),
            &section,
            sizeof(section)))
        {
            std::cout << "Could not read section.\n";
            break;
        }

        char name[9] = {};
        std::memcpy(name, section.Name, 8);

        if (std::strcmp(name, ".text") == 0)
        {
            uintptr_t textAddress =
                base + section.VirtualAddress;

            std::cout << "Found .text section!\n";

            std::cout << "Section RVA: 0x"
                      << std::hex
                      << section.VirtualAddress << "\n";

            std::cout << "Section address: 0x"
                      << textAddress << "\n";

            
std::cout << std::dec
          << "Section size: "
          << section.Misc.VirtualSize
          << " bytes\n";

// Read the first 16 bytes of .text.
unsigned char bytes[16] = {};

if (reader.Read(textAddress, bytes, sizeof(bytes)))
{
    std::cout << "\nFirst 16 bytes of .text:\n";

    for (unsigned char byte : bytes)
    {
        std::cout
            << std::hex
            << std::uppercase
            << std::setfill('0')
            << std::setw(2)
            << static_cast<int>(byte)
            << " ";
    }

    std::cout << std::dec << "\n";
}
else
{
    std::cout << "Could not read .text bytes.\n";
}

return 0;


            return 0;
        }
    }

    std::cout << ".text section not found.\n";
    return 1;
}
