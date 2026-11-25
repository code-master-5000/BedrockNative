
#include "MemoryReader.h"

#include <tlhelp32.h>
#include <cwchar>

// Release the process handle when the object is destroyed.
MemoryReader::~MemoryReader()
{
    Detach();
}

void MemoryReader::Detach()
{
    if (process_ != nullptr)
    {
        CloseHandle(process_);
        process_ = nullptr;
    }

    baseAddress_ = 0;
}

bool MemoryReader::IsAttached() const
{
    return process_ != nullptr;
}

ProcessStatus MemoryReader::GetProcessStatus() const
{
    if (process_ == nullptr)
        return ProcessStatus::Unknown;

    DWORD result = WaitForSingleObject(process_, 0);

    if (result == WAIT_TIMEOUT)
        return ProcessStatus::Running;

    if (result == WAIT_OBJECT_0)
        return ProcessStatus::Exited;

    return ProcessStatus::Unknown;
}

uintptr_t MemoryReader::GetBaseAddress() const
{
    return baseAddress_;
}

bool MemoryReader::Attach()
{
    // Clean up any previous connection.
    Detach();

    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0
    );

    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);

    DWORD pid = 0;

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(
                entry.szExeFile,
                L"Minecraft.Windows.exe"
            ) == 0)
            {
                pid = entry.th32ProcessID;
                break;
            }
        }
        while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);

    if (pid == 0)
        return false;

    process_ = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION |
        PROCESS_VM_READ |
        SYNCHRONIZE,
        FALSE,
        pid
    );

    if (process_ == nullptr)
        return false;

    HANDLE modules = INVALID_HANDLE_VALUE;

    // Module snapshots can briefly fail while modules change.
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        modules = CreateToolhelp32Snapshot(
            TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
            pid
        );

        if (modules != INVALID_HANDLE_VALUE)
            break;

        if (GetLastError() != ERROR_BAD_LENGTH)
            break;
    }

    if (modules == INVALID_HANDLE_VALUE)
    {
        Detach();
        return false;
    }

    MODULEENTRY32W module = {};
    module.dwSize = sizeof(module);

    bool found = false;

    if (Module32FirstW(modules, &module))
    {
        do
        {
            if (_wcsicmp(
                module.szModule,
                L"Minecraft.Windows.exe"
            ) == 0)
            {
                baseAddress_ =
                    reinterpret_cast<uintptr_t>(
                        module.modBaseAddr
                    );

                found = true;
                break;
            }
        }
        while (Module32NextW(modules, &module));
    }

    CloseHandle(modules);

    if (!found)
    {
        Detach();
        return false;
    }

    return true;
}

bool MemoryReader::Read(
    uintptr_t address,
    void* destination,
    SIZE_T size
) const
{
    if (!IsAttached() ||
        address == 0 ||
        destination == nullptr ||
        size == 0)
    {
        return false;
    }

    SIZE_T bytesRead = 0;

    BOOL success = ReadProcessMemory(
        process_,
        reinterpret_cast<LPCVOID>(address),
        destination,
        size,
        &bytesRead
    );

    return success && bytesRead == size;
}
