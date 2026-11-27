#include "GameTelemetry.h"

#include "MemoryReader.h"
#include "EntityTracker.h"
#include "StateVectorDiscovery.h"

#include <windows.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

namespace
{

constexpr uint8_t signature[] =
{
    0x4C, 0x89, 0x3D,
    0, 0, 0, 0,
    0x4D, 0x85, 0xFF
};

bool ReadPointer(
    const MemoryReader& reader,
    uintptr_t address,
    uintptr_t& result
)
{
    result = 0;

    return address != 0 &&
        reader.Read(
            address,
            &result,
            sizeof(result)
        ) &&
        result != 0;
}

bool ResolveClient(
    const MemoryReader& reader,
    uintptr_t global,
    uintptr_t& client
)
{
    uintptr_t node = global;

    for (uintptr_t offset :
        {
            uintptr_t(0),
            uintptr_t(0x8),
            uintptr_t(0x118),
            uintptr_t(0x48)
        })
    {
        uintptr_t next = 0;

        if (!ReadPointer(reader, node + offset, next))
            return false;

        node = next;
    }

    uintptr_t game = node;
    uintptr_t head = 0;

    if (!ReadPointer(reader, game + 0x9D0, head))
        return false;

    uintptr_t instance = 0;

    if (!ReadPointer(reader, head + 0x38, instance))
        return false;

    uintptr_t back = 0;

    if (!ReadPointer(reader, instance + 0x1C8, back))
        return false;

    if (back != game)
        return false;

    client = instance;
    return true;
}

bool FindClient(
    const MemoryReader& reader,
    uintptr_t& client
)
{
    uintptr_t base = reader.GetBaseAddress();

    if (!base)
        return false;

    IMAGE_DOS_HEADER dos{};

    if (!reader.Read(base, &dos, sizeof(dos)))
        return false;

    if (dos.e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    if (dos.e_lfanew < 0 || dos.e_lfanew > 0x100000)
        return false;

    uintptr_t ntAddress =
        base + static_cast<uintptr_t>(dos.e_lfanew);

    IMAGE_NT_HEADERS64 nt{};

    if (!reader.Read(
            ntAddress,
            &nt,
            sizeof(nt)
        ))
        return false;

    if (nt.Signature != IMAGE_NT_SIGNATURE)
        return false;

    if (nt.OptionalHeader.Magic !=
        IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return false;

    if (nt.FileHeader.NumberOfSections > 96)
        return false;

    uintptr_t sections =
        ntAddress +
        4 +
        sizeof(IMAGE_FILE_HEADER) +
        nt.FileHeader.SizeOfOptionalHeader;

    constexpr size_t step = 32768;

    std::vector<uint8_t> bytes(
        step + sizeof(signature) - 1
    );

    for (
        WORD sectionIndex = 0;
        sectionIndex < nt.FileHeader.NumberOfSections;
        ++sectionIndex
    )
    {
        IMAGE_SECTION_HEADER section{};

        if (!reader.Read(
                sections +
                sectionIndex * sizeof(section),
                &section,
                sizeof(section)
            ))
            return false;

        if (std::memcmp(
                section.Name,
                ".text",
                5
            ) != 0)
            continue;

        uintptr_t start =
            base + section.VirtualAddress;

        size_t length =
            section.Misc.VirtualSize;

        for (
            size_t position = 0;
            position < length;
            position += step
        )
        {
            size_t count = std::min(
                bytes.size(),
                length - position
            );

            if (!reader.Read(
                    start + position,
                    bytes.data(),
                    count
                ))
                continue;

            for (
                size_t i = 0;
                i + sizeof(signature) <= count;
                ++i
            )
            {
                if (std::memcmp(
                        bytes.data() + i,
                        signature,
                        3
                    ) != 0)
                    continue;

                if (std::memcmp(
                        bytes.data() + i + 7,
                        signature + 7,
                        3
                    ) != 0)
                    continue;

                int32_t relative = 0;

                std::memcpy(
                    &relative,
                    bytes.data() + i + 3,
                    sizeof(relative)
                );

                int64_t address =
                    static_cast<int64_t>(
                        start + position + i + 7
                    ) + relative;

                if (address <= 0)
                    continue;

                if (ResolveClient(
                        reader,
                        static_cast<uintptr_t>(address),
                        client
                    ))
                    return true;
            }
        }
    }

    return false;
}

void Pause()
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds(250)
    );
}

}

// ============================================================
// WORKER THREAD
// ============================================================

GameTelemetry::~GameTelemetry()
{
    Stop();
}

void GameTelemetry::Start()
{
    if (worker_.joinable())
        return;

    stop_ = false;

    worker_ = std::thread(
        &GameTelemetry::Run,
        this
    );
}

void GameTelemetry::Stop()
{
    stop_ = true;

    if (worker_.joinable())
        worker_.join();
}

void GameTelemetry::Publish(
    const TelemetrySnapshot& value
)
{
    std::lock_guard<std::mutex> lock(mutex_);

    snapshot_ = value;
}

TelemetrySnapshot GameTelemetry::Get() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    return snapshot_;
}

void GameTelemetry::Run()
{
    while (!stop_)
    {
        MemoryReader reader;

        if (!reader.Attach())
        {
            Publish({"Waiting for Minecraft"});
            Pause();
            continue;
        }

        uintptr_t clientInstance = 0;

        if (!FindClient(reader, clientInstance))
        {
            Publish({"ClientInstance not found"});

            for (int i = 0; i < 8 && !stop_; ++i)
                Pause();

            continue;
        }

        uintptr_t storage = 0;
        size_t poolCount = 0;

        if (!FindStateVectorStorage(
                reader,
                clientInstance,
                storage,
                poolCount
            ))
        {
            Publish({"Position component not found"});

            for (int i = 0; i < 8 && !stop_; ++i)
                Pause();

            continue;
        }

        EntityTracker tracker(reader);

        if (!tracker.Initialize(storage))
        {
            Publish({"Tracker initialization failed"});

            for (int i = 0; i < 8 && !stop_; ++i)
                Pause();

            continue;
        }

        int failures = 0;

        while (
            !stop_ &&
            reader.GetProcessStatus() ==
                ProcessStatus::Running
        )
        {
            std::vector<TrackedEntity> entities;
            EntityScanStats stats{};

            if (!tracker.Enumerate(entities, stats))
            {
                if (++failures >= 8)
                    break;

                Pause();
                continue;
            }

            failures = 0;

            uint32_t playerId = 0;

            if (!reader.Read(
                    clientInstance + 0x278,
                    &playerId,
                    sizeof(playerId)
                ))
                break;

            TelemetrySnapshot snapshot{};

            snapshot.status =
                "Searching for local player";

            snapshot.entityCount = entities.size();

            for (const auto& entity : entities)
            {
                if (entity.id == playerId)
                {
                    snapshot.hasPosition = true;

                    snapshot.x = entity.position.x;
                    snapshot.y = entity.position.y;
                    snapshot.z = entity.position.z;

                    snapshot.status = "Connected";
                    break;
                }
            }

            Publish(snapshot);
            Pause();
        }

        Publish({"Reconnecting"});

        for (int i = 0; i < 4 && !stop_; ++i)
            Pause();
    }
}
