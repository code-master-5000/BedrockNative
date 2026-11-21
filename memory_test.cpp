
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <cwchar>
#include <cstdint>

int main()
{
    std::cout << "Bedrock Memory Test\n\n";

    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0
    );

    if (snapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "Cannot list processes.\n";
        return 1;
    }

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);

    DWORD minecraftPid = 0;

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(
                entry.szExeFile,
                L"Minecraft.Windows.exe"
            ) == 0)
            {
                minecraftPid = entry.th32ProcessID;
                break;
            }
        }
        while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);

    if (minecraftPid == 0)
    {
        std::cout << "Minecraft not found.\n";
        return 1;
    }

    std::cout << "Minecraft PID: "
              << minecraftPid << "\n";

    HANDLE process = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION |
        PROCESS_VM_READ,
        FALSE,
        minecraftPid
    );

    if (process == nullptr)
    {
        std::cout << "Memory access FAILED.\n";
        std::cout << "Windows error: "
                  << GetLastError() << "\n";
        return 1;
    }

    std::cout << "Memory-read permission: GRANTED\n";
    std::cout << "Process handle opened successfully.\n";

    HANDLE modules = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
        minecraftPid
    );

    if (modules == INVALID_HANDLE_VALUE)
    {
        std::cout << "Module lookup failed: "
                  << GetLastError() << "\n";
    }
    else
    {
        MODULEENTRY32W module = {};
        module.dwSize = sizeof(module);

        Module32FirstW
            SIZE_T bytesRead = 0;

            BOOL success = ReadProcessMemory(
                process,
                module.modBaseAddr,
                bytes,
                sizeof(bytes),
                &bytesRead
            );

            if (success && bytesRead == sizeof(bytes))
            {
                std::cout << "Memory read SUCCESS!\n";
                std::cout << std::hex;
                std::cout << "First bytes: "
                          << static_cast<int>(bytes[0])
                          << " "
                          << static_cast<int>(bytes[1])
                          << std::dec << "\n";
            }
            else
            {
                std::cout << "Memory read FAILED: "
                          << GetLastError() << "\n";
            }
        }
        else
        {
            std::cout << "Module enumeration failed: "
                      << GetLastError() << "\n";
        }

        CloseHandle(modules);
    }

    CloseHandle(process);

    std::cout << "\nTest complete.\n";
    std::cin.get();
    return 0;
}
