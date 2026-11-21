
#include <windows.h>
#include <iostream>
#include <string>
#include "MemoryReader.h"

// Our native module states
bool hitboxesEnabled = false;
bool espEnabled = false;

void ProcessCommand(const std::string& command)
{
    if (command == "HITBOXES_ON")
        hitboxesEnabled = true;

    else if (command == "HITBOXES_OFF")
        hitboxesEnabled = false;

    else if (command == "ESP_ON")
        espEnabled = true;

    else if (command == "ESP_OFF")
        espEnabled = false;

    else
    {
        std::cout << "Unknown command: "
                  << command << "\n";
        return;
    }

    std::cout << "\n--- Bedrock Native State ---\n";

    std::cout << "Hitboxes: "
              << (hitboxesEnabled ? "ON" : "OFF")
              << "\n";

    std::cout << "ESP Mode: "
              << (espEnabled ? "ON" : "OFF")
              << "\n";

    std::cout << "----------------------------\n";
}

int main()
{

    MemoryReader reader;

    std::cout << "Connecting to Minecraft...\n";

    if (reader.Attach())
    {
        std::cout << "Minecraft memory attached!\n";

        unsigned char signature[2] = {};

        if (reader.Read(
            reader.GetBaseAddress(),
            signature,
            sizeof(signature)))
        {
            std::cout << "Memory read: "
                      << std::hex
                      << static_cast<int>(signature[0])
                      << " "
                      << static_cast<int>(signature[1])
                      << std::dec << "\n";
        }
        else
        {
            std::cout << "Memory read failed.\n";
        }
    }
    else
    {
        std::cout << "Minecraft not detected or access denied.\n";
    }

    std::cout << "Bedrock Native Bridge started!\n";

    while (true)
    {
        HANDLE pipe = CreateNamedPipeW(
            L"\\\\.\\pipe\\BedrockUtilityControl",
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE |
            PIPE_READMODE_MESSAGE |
            PIPE_WAIT,
            1,
            128,
            128,
            0,
            nullptr
        );

        if (pipe == INVALID_HANDLE_VALUE)
        {
            std::cout << "Pipe creation failed: "
                      << GetLastError() << "\n";
            return 1;
        }

        std::cout << "Waiting for ClickGUI...\n";

        BOOL connected = ConnectNamedPipe(pipe, nullptr);

        if (!connected &&
            GetLastError() == ERROR_PIPE_CONNECTED)
        {
            connected = TRUE;
        }

        if (connected)
        {
            char buffer[128] = {};
            DWORD bytesRead = 0;

            if (ReadFile(
                pipe,
                buffer,
                sizeof(buffer) - 1,
                &bytesRead,
                nullptr
            ))
            {
                std::string command(buffer, bytesRead);
                ProcessCommand(command);
            }
        }

        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
}
