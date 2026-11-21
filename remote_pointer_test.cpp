
#include "MemoryReader.h"
#include "PointerResolver.h"

#include <windows.h>
#include <iostream>
#include <cstdint>

int main()
{
    MemoryReader reader;

    if (!reader.Attach())
    {
        std::cout << "Cannot attach to Minecraft.\n";
        return 1;
    }

    uintptr_t base = reader.GetBaseAddress();

    // Read Minecraft's executable headers.
    IMAGE_DOS_HEADER dos = {};

    if (!reader.Read(base, &dos, sizeof(dos)) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew <= 0 ||
        dos.e_lfanew > 4096)
    {
        std::cout << "Invalid DOS header.\n";
        return 1;
    }

    IMAGE_NT_HEADERS64 nt = {};

    if (!reader.Read(
            base + static_cast<uintptr_t>(dos.e_lfanew),
            &nt,
            sizeof(nt)) ||
        nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.OptionalHeader.Magic !=
            IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        std::cout << "Invalid PE header.\n";
        return 1;
    }

    // Find the import directory.
    auto imports = nt.OptionalHeader.DataDirectory[
        IMAGE_DIRECTORY_ENTRY_IMPORT
    ];

    if (imports.VirtualAddress == 0 ||
        imports.Size < sizeof(IMAGE_IMPORT_DESCRIPTOR))
    {
        std::cout << "No standard import directory found.\n";
        return 1;
    }

    IMAGE_IMPORT_DESCRIPTOR descriptor = {};

    if (!reader.Read(
            base + imports.VirtualAddress,
            &descriptor,
            sizeof(descriptor)) ||
        descriptor.FirstThunk == 0)
    {
        std::cout << "Could not find an import table.\n";
        return 1;
    }

    // The first IAT slot stores a function pointer.
    uintptr_t pointerLocation =
        base + descriptor.FirstThunk;

    auto readPointer =
        [&](uintptr_t address, uintptr_t& result) -> bool
    {
        return reader.Read(
            address,
            &result,
            sizeof(result)
        );
    };

    uintptr_t functionAddress =
        PointerResolver::Resolve(
            pointerLocation,
            {0},
            readPointer
        );

    std::cout << "Minecraft base: 0x"
              << std::hex << base << "\n";

    std::cout << "IAT pointer location: 0x"
              << pointerLocation << "\n";

    std::cout << "Resolved function address: 0x"
              << functionAddress << "\n";

    if (functionAddress == 0)
    {
        std::cout << "POINTER TEST FAILED!\n";
        return 1;
    }

    std::cout << "REAL POINTER READ PASSED!\n";

    reader.Detach();
    return 0;
}
