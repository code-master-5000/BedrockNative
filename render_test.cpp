
#include <windows.h>

bool outlineEnabled = true;
bool wasF9Down = false;

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

const COLORREF transparentColor = RGB(1, 2, 3);

LRESULT CALLBACK WindowProc(
    HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
        if (msg == WM_TIMER)
    {
        bool f9Down =
            (GetAsyncKeyState(VK_F9) & 0x8000) != 0;

        if (f9Down && !wasF9Down)
        {
            outlineEnabled = !outlineEnabled;

            // Redraw the overlay.
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        wasF9Down = f9Down;
        return 0;
    }
    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);

        RECT area;
        GetClientRect(hwnd, &area);

        HBRUSH background = CreateSolidBrush(transparentColor);
        FillRect(dc, &area, background);
        DeleteObject(background);

        // Bright green test outline.
        if (outlineEnabled){
            HPEN pen = CreatePen(PS_SOLID, 3, RGB(0, 255, 100));
            HGDIOBJ oldPen = SelectObject(dc, pen);
            HGDIOBJ oldBrush =
                SelectObject(dc, GetStockObject(NULL_BRUSH));

            Rectangle(dc, 150, 80, 300, 260);

            SelectObject(dc, oldBrush);
            SelectObject(dc, oldPen);
            DeleteObject(pen);

            EndPaint(hwnd, &ps);
            return 0;
        }
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(
    HINSTANCE instance, HINSTANCE, LPSTR, int)
{
    const wchar_t* className = L"BedrockRenderTest";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = className;

    if (!RegisterClassW(&wc))
        return 1;

    HWND window = CreateWindowExW(
        WS_EX_TOPMOST |
        WS_EX_LAYERED |
        WS_EX_TRANSPARENT |
        WS_EX_TOOLWINDOW,
        className,
        L"Bedrock Render Test",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, instance, nullptr
    );

    if (!window)
        return 1;

    SetLayeredWindowAttributes(
        window,
        transparentColor,
        0,
        LWA_COLORKEY
    );

    ShowWindow(window, SW_SHOW);
    SetTimer(window, 1, 30, nullptr);

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return 0;
}
