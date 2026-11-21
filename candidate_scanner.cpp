
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <utility>

struct Block
{
    uintptr_t address = 0;
    std::vector<unsigned char> baseline;
    std::vector<size_t> candidates;
};

DWORD FindMinecraft()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    DWORD pid = 0;

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(entry.szExeFile,
                        L"Minecraft.Windows.exe") == 0)
            {
                pid = entry.th32ProcessID;
                break;
            }
        }
        while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pid;
}

bool ReadBlock(
    HANDLE process,
    uintptr_t address,
    std::vector<unsigned char>& bytes)
{
    SIZE_T read = 0;

    return ReadProcessMemory(
        process,
        reinterpret_cast<LPCVOID>(address),
        bytes.data(),
        bytes.size(),
        &read
    ) && read == bytes.size();
}

std::vector<Block> Capture(HANDLE process)
{
    std::vector<Block> blocks;

    const SIZE_T limit = 32ull * 1024 * 1024;
    const SIZE_T chunkSize = 64 * 1024;

    SIZE_T captured = 0;
    uintptr_t address = 0;
    MEMORY_BASIC_INFORMATION info = {};

    while (captured < limit &&
           VirtualQueryEx(
               process,
               reinterpret_cast<LPCVOID>(address),
               &info,
               sizeof(info)) == sizeof(info))
    {
        uintptr_t start =
            reinterpret_cast<uintptr_t>(info.BaseAddress);

        uintptr_t next = start + info.RegionSize;

        if (next <= address)
            break;

        DWORD protection = info.Protect & 0xff;

        bool writable =
            protection == PAGE_READWRITE ||
            protection == PAGE_WRITECOPY ||
            protection == PAGE_EXECUTE_READWRITE ||
            protection == PAGE_EXECUTE_WRITECOPY;

        if (info.State == MEM_COMMIT &&
            writable &&
            !(info.Protect & PAGE_GUARD))
        {
            for (SIZE_T offset = 0;
                 offset < info.RegionSize && captured < limit;
                 offset += chunkSize)
            {
                SIZE_T size = std::min(
                    chunkSize, info.RegionSize - offset);

                size = std::min(size, limit - captured);

                Block block;
                block.address = start + offset;
                block.baseline.resize(size);

                if (ReadBlock(
                    process, block.address, block.baseline))
                {
                    captured += size;
                    blocks.push_back(std::move(block));
                }
            }
        }

        address = next;
    }

    std::cout << "Captured "
              << captured / 1024
              << " KB in "
              << blocks.size() << " blocks.\n";

    return blocks;
}

SIZE_T FilterCandidates(
    HANDLE process,
    std::vector<Block>& blocks,
    int scanNumber)
{
    const unsigned char expected =
    (scanNumber % 2 == 1) ? 1 : 0;

    SIZE_T total = 0;

    for (Block& block : blocks)
    {
        if (scanNumber > 1 && block.candidates.empty())
            continue;

        std::vector<unsigned char> current(
            block.baseline.size());

        if (!ReadBlock(process, block.address, current))
        {
            block.candidates.clear();
            continue;
        }

        if (scanNumber == 1)
        {
            // Baseline was ON. Find 1 -> 0 changes.
            for (size_t i = 0; i < current.size(); ++i)
            {
                if (block.baseline[i] == 0 &&
    current[i] == 1)
                {
                    block.candidates.push_back(i);
                }
            }
        }
        else
        {
            std::vector<size_t> survivors;

            for (size_t offset : block.candidates)
            {
                if (current[offset] == expected)
                    survivors.push_back(offset);
            }

            block.candidates = std::move(survivors);
        }

        total += block.candidates.size();
    }

    std::cout << "\nScan " << scanNumber
              << " (expected "
              << static_cast<int>(expected)
              << ")\n";

    std::cout << "Remaining candidates: "
              << total << "\n";

    int shown = 0;

    for (const Block& block : blocks)
    {
        for (size_t offset : block.candidates)
        {
            if (shown >= 15)
                break;

            std::cout << "0x" << std::hex
                      << block.address + offset
                      << std::dec << "\n";

            ++shown;
        }

        if (shown >= 15)
            break;
    }

    return total;
}

int main()
{
    std::cout << "Bedrock Candidate Scanner v2\n\n";

    DWORD pid = FindMinecraft();

    if (pid == 0)
    {
        std::cout << "Minecraft not found.\n";
        return 1;
    }

    HANDLE process = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        pid);

    if (!process)
    {
        std::cout << "Access failed: "
                  << GetLastError() << "\n";
        return 1;
    }

    std::cout << "Minecraft PID: " << pid << "\n";
    std::cout << "Hitboxes must be OFF now.\n";
    std::cout << "Press ENTER to capture baseline.\n";

    std::string input;
    std::getline(std::cin, input);

    std::vector<Block> blocks = Capture(process);

    if (blocks.empty())
    {
        std::cout << "No memory captured.\n";
        CloseHandle(process);
        return 1;
    }

    int scanNumber = 0;

    while (true)
    {
        bool turnOff = (scanNumber % 2 != 0);

        std::cout << "\nSet Hitboxes "
                  << (turnOff ? "OFF" : "ON")
                  << " in Minecraft.\n";

        std::cout << "Press ENTER to scan, "
                     "or Q then ENTER to quit.\n";

        std::getline(std::cin, input);

        if (input == "q" || input == "Q")
            break;

        ++scanNumber;

        SIZE_T remaining = FilterCandidates(
            process, blocks, scanNumber);

        if (remaining == 0)
        {
            std::cout <<
                "No candidates remain.\n"
                "The setting may be outside the scanned "
                "memory or use another representation.\n";
            break;
        }
    }

    CloseHandle(process);
    return 0;
}
