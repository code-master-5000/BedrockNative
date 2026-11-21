
#define NOMINMAX
#include "ModuleScanner.h"

#include <windows.h>
#include <algorithm>
#include <cstring>
#include <vector>

bool ModuleScanner::FindTextSection(
    const MemoryReader& reader,
    Section& result)
{
    result = {};

    if (!reader.IsAttached())
        return false;

    uintptr_t base = reader.GetBaseAddress();

    IMAGE_DOS_HEADER dos = {};

    if (!reader.Read(base, &dos, sizeof(dos)) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew <= 0 ||
        dos.e_lfanew > 4096)
    {
        return false;
    }

    uintptr_t ntAddress =
        base + static_cast<uintptr_t>(dos.e_lfanew);

    IMAGE_NT_HEADERS64 nt = {};

    if (!reader.Read(ntAddress, &nt, sizeof(nt)) ||
        nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.OptionalHeader.Magic !=
            IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        return false;
    }

    if (nt.FileHeader.NumberOfSections == 0 ||
        nt.FileHeader.NumberOfSections > 96)
    {
        return false;
    }

    uintptr_t sectionTable =
        ntAddress +
        sizeof(DWORD) +
        sizeof(IMAGE_FILE_HEADER) +
        nt.FileHeader.SizeOfOptionalHeader;

    for (unsigned int i = 0;
         i < nt.FileHeader.NumberOfSections;
         ++i)
    {
        IMAGE_SECTION_HEADER section = {};

        uintptr_t headerAddress =
            sectionTable + i * sizeof(section);

        if (!reader.Read(
            headerAddress, &section, sizeof(section)))
        {
            return false;
        }

        char name[9] = {};
        std::memcpy(name, section.Name, 8);

        if (std::strcmp(name, ".text") == 0)
        {
            if (section.Misc.VirtualSize == 0 ||
                section.VirtualAddress >=
                    nt.OptionalHeader.SizeOfImage ||
                section.Misc.VirtualSize >
                    nt.OptionalHeader.SizeOfImage -
                    section.VirtualAddress)
            {
                return false;
            }

            result.address =
                base + section.VirtualAddress;

            result.size =
                section.Misc.VirtualSize;

            return true;
        }
    }

    return false;
}

std::vector<uintptr_t> ModuleScanner::ScanText(
    const MemoryReader& reader,
    const SignatureScanner::Pattern& pattern,
    size_t maxBytes)
{
    std::vector<uintptr_t> results;

    if (pattern.empty())
        return results;

    for (int byte : pattern)
    {
        if (byte < -1 || byte > 255)
            return results;
    }

    Section text = {};

    if (!FindTextSection(reader, text))
        return results;

    size_t scanSize = text.size;

    if (maxBytes != 0)
        scanSize = std::min(scanSize, maxBytes);

    if (pattern.size() > scanSize)
        return results;

    constexpr size_t chunkSize = 64 * 1024;

    for (size_t offset = 0;
         offset < scanSize;
         offset += chunkSize)
    {
        // Each chunk owns these bytes.
        size_t ownedSize =
            std::min(chunkSize, scanSize - offset);

        // Extra bytes allow patterns to cross the boundary.
        size_t overlap =
            std::min(pattern.size() - 1,
                     scanSize - offset - ownedSize);

        std::vector<uint8_t> bytes(
            ownedSize + overlap);

        uintptr_t address = text.address + offset;

        if (!reader.Read(
            address, bytes.data(), bytes.size()))
        {
            // Skip chunks that aren't fully readable.
            continue;
        }

        auto matches =
            SignatureScanner::FindAll(bytes, pattern);

        for (size_t match : matches)
        {
            // Avoid counting an overlapping match twice.
            if (match < ownedSize)
            {
                results.push_back(address + match);
            }
        }
    }

    return results;
}
