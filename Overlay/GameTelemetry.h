#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

struct TelemetrySnapshot
{
    std::string status = "Starting";

    bool hasPosition = false;

    float x = 0;
    float y = 0;
    float z = 0;

    std::size_t entityCount = 0;
};

class GameTelemetry
{
public:
    ~GameTelemetry();

    void Start();
    void Stop();

    TelemetrySnapshot Get() const;

private:
    void Run();
    void Publish(const TelemetrySnapshot& value);

    std::atomic<bool> stop_{false};
    std::thread worker_;

    mutable std::mutex mutex_;
    TelemetrySnapshot snapshot_;
};
