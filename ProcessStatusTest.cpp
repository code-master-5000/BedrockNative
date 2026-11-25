#include "MemoryReader.h"

#include <conio.h>
#include <iostream>

int main()
{
    MemoryReader reader;

    if (!reader.Attach())
    {
        std::cout << "Failed to attach to Minecraft.\n";
        return 1;
    }

    std::cout << "Attached to Minecraft!\n";
    std::cout << "Close Minecraft to test detection.\n";
    std::cout << "Press Q to quit manually.\n\n";

    while (true)
    {
        ProcessStatus status = reader.GetProcessStatus();

        if (status == ProcessStatus::Running)
        {
            std::cout << "\rMinecraft status: RUNNING  "
                      << std::flush;
        }
        else if (status == ProcessStatus::Exited)
        {
            std::cout << "\nPASS: Minecraft exit detected.\n";
            return 0;
        }
        else
        {
            std::cout << "\nFAIL: Unknown process status.\n";
            return 1;
        }

        if (_kbhit())
        {
            int key = _getch();

            if (key == 'q' || key == 'Q')
            {
                std::cout << "\nTracking stopped manually.\n";
                return 0;
            }
        }

        Sleep(500);
    }
}
