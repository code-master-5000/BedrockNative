
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <string>

struct Block
{
    uintptr_t address;
    std::vector<unsigned char> bytes;
};

DWORD FindMinecraft()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0
    );

    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    DWORD pid = 0;

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(
                entry.szExeFile,
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

std::vector<Block> TakeSnapshot(HANDLE process)
{
    std::vector<Block> blocks;

    const SIZE_T limit = 32 * 1024 * 1024;
    const SIZE_T chunkSize = 64 * 1024;

    SIZE_T total = 0;
    uintptr_t address = 0;

    MEMORY_BASIC_INFORMATION info = {};

    while (total < limit &&
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
            SIZE_T offset = 0;

            while (offset < info.RegionSize &&
                   total < limit)
            {
                SIZE_T size = std::min(
                    chunkSize,
                    info.RegionSize - offset
                );

                size = std::min(size, limit - total);

                Block block;
                block.address = start + offset;
                block.bytes.resize(size);

                SIZE_T bytesRead = 0;

                if (ReadProcessMemory(
                    process,
                    reinterpret_cast<LPCVOID>(
                        block.address),
                    block.bytes.data(),
                    size,
                    &bytesRead) &&
                    bytesRead == size)
                {
                    total += size;
                    blocks.push_back(std::move(block));
                }

                offset += size;
            }
        }

        address = next;
    }

    std::cout << "Captured "
              << total / 1024
              << " KB in "
              << blocks.size()
              << " blocks.\n";

    return blocks;
}

void CompareSnapshots(
    HANDLE process,
    std::vector<Block>& blocks)
{
    SIZE_T changed = 0;
    SIZE_T booleanFlips = 0;
    int displayed = 0;

    for (Block& block : blocks)
    {
        std::vector<unsigned char> current(
            block.bytes.size()
        );

        SIZE_T bytesRead = 0;

        if (!ReadProcessMemory(
            process,
            reinterpret_cast<LPCVOID>(block.address),
            current.data(),
            current.size(),
            &bytesRead) ||
            bytesRead != current.size())
        {
            continue;
        }

        for (SIZE_T i = 0; i < current.size(); ++i)
        {
            unsigned char oldValue = block.bytes[i];
            unsigned char newValue = current[i];

            if (oldValue == newValue)
                continue;

            ++changed;

            // Highlight byte changes resembling Boolean toggles.
            if ((oldValue == 0 && newValue == 1) ||
                (oldValue == 1 && newValue == 0))
            {
                ++booleanFlips;

                if (displayed < 20)
                {
                    std::cout << "0x"
                              << std::hex
                              << block.address + i
                              << std::dec
                              << " : "
                              << static_cast<int>(oldValue)
                              << " -> "
                              << static_cast<int>(newValue)
                              << "\n";

                    ++displayed;
                }
            }
        }

        // Update baseline for the next comparison.
        block.bytes = std::move(current);
    }

    std::cout << "\nChanged bytes: " << changed << "\n";
    std::cout << "0/1 flips: " << booleanFlips << "\n";
    std::cout << "Displayed up to 20 candidate flips.\n";
}

int main()
{
    std::cout << "Bedrock Memory Change Scanner\n\n";

    DWORD pid = FindMinecraft();

    if (!pid)
    {
        std::cout << "Minecraft not found.\n";
        std::cin.get();
        return 1;
    }

    HANDLE process = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        pid
    );

    if (!process)
    {
        std::cout << "Cannot read Minecraft memory.\n";
        std::cout << "Error: " << GetLastError() << "\n";
        std::cin.get();
        return 1;
    }

    std::cout << "Minecraft PID: " << pid << "\n";
    std::cout << "Taking baseline...\n";

    std::vector<Block> blocks = TakeSnapshot(process);

    if (blocks.empty())
    {
        std::cout << "No readable blocks captured.\n";
        CloseHandle(process);
        std::cin.get();
        return 1;
    }

    std::cout << "\nScanner ready.\n";

    while (true)
    {
        std::cout <<
            "\nToggle a setting in Minecraft.\n"
            "Press ENTER to compare, or Q then ENTER to quit.\n";

        std::string input;
        std::getline(std::cin, input);

        if (input == "q" || input == "Q")
            break;

        CompareSnapshots(process, blocks);
    }

    CloseHandle(process);
    return 0;
}
