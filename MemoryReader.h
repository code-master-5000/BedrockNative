
#pragma once

#include <windows.h>
#include <cstdint>

class MemoryReader
{
public:
    MemoryReader() = default;
    ~MemoryReader();
    MemoryReader(const MemoryReader&) = delete;
    MemoryReader& operator=(const MemoryReader&) = delete;

    bool Attach();
    void Detach();

    bool IsAttached() const;
    uintptr_t GetBaseAddress() const;

    bool Read(
        uintptr_t address,
        void* destination,
        SIZE_T size
    ) const;

private:
    HANDLE process_ = nullptr;
    uintptr_t baseAddress_ = 0;
};
