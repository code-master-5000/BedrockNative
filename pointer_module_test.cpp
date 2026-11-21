
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <cstdint>
#include <cwchar>

int main()
{
    // Address found in our previous test.
    uintptr_t target = 0x7FF8B1202830;

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

    HANDLE modules = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
        pid
    );

    if (modules == INVALID_HANDLE_VALUE)
    {
        std::cout << "Cannot enumerate modules.\n";
        return 1;
    }

    MODULEENTRY32W module = {};
    module.dwSize = sizeof(module);
    bool found = false;

    if (Module32FirstW(modules, &module))
    {
        do
        {
            uintptr_t start =
                reinterpret_cast<uintptr_t>(
                    module.modBaseAddr
                );

            uintptr_t size = module.modBaseSize;

            if (target >= start &&
                target - start < size)
            {
                std::wcout << L"Address belongs to: "
                           << module.szModule << L"\n";

                std::cout << "Module base: 0x"
                          << std::hex << start << "\n";

                std::cout << "Offset within module: 0x"
                          << target - start << "\n";

                found = true;
                break;
            }
        }
        while (Module32NextW(modules, &module));
    }

    CloseHandle(modules);

    if (!found)
    {
        std::cout << "Address not found in listed modules.\n";
    }

    return found ? 0 : 1;
}
