
#include <windows.h>
#include <tlhelp32.h>

#include <iostream>
#include <cstdint>
#include <cwchar>

int main()
{
    // Find Minecraft's process.
    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0
    );

    if (snapshot == INVALID_HANDLE_VALUE)
        return 1;

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

    if (!pid)
    {
        std::cout << "Minecraft not found.\n";
        return 1;
    }

    HANDLE process = OpenProcess(
        PROCESS_QUERY_INFORMATION,
        FALSE,
        pid
    );

    if (!process)
    {
        std::cout << "Cannot open process.\n";
        return 1;
    }

    uintptr_t addresses[] = {
        0x4AB420400B0,
        0x4AB420301C0,
        0x4AB42030260,
        0x7FF8DA01CD00
    };

    for (uintptr_t address : addresses)
    {
        MEMORY_BASIC_INFORMATION info = {};

        SIZE_T result = VirtualQueryEx(
            process,
            reinterpret_cast<LPCVOID>(address),
            &info,
            sizeof(info)
        );

        if (!result)
        {
            std::cout << "Query failed for 0x"
                      << std::hex << address << "\n";
            continue;
        }

        std::cout << "\nAddress: 0x"
                  << std::hex << address << "\n";

        std::cout << "Region base: 0x"
                  << reinterpret_cast<uintptr_t>(
                         info.BaseAddress
                     ) << "\n";

        std::cout << "Region size: 0x"
                  << info.RegionSize << "\n";

        std::cout << "State: 0x"
                  << info.State << "\n";

        std::cout << "Type: 0x"
                  << info.Type << "\n";

        std::cout << "Protection: 0x"
                  << info.Protect << "\n";
    }

    CloseHandle(process);
    return 0;
}
