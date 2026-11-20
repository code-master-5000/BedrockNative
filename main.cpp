
#include <windows.h>
#include <iostream>
#include <string>

int main()
{
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
            std::cout << "Failed to create pipe.\n";
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
                std::cout << "Received: "
                            << command << "\n";
            }
        }

        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
}
