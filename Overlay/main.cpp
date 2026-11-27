#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <tlhelp32.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cwchar>
#include <iostream>
#include <cstdio>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND, UINT, WPARAM, LPARAM
);

// Window and DirectX state
static HWND overlay = nullptr;
static HWND minecraft = nullptr;

static ID3D11Device* device = nullptr;
static ID3D11DeviceContext* context = nullptr;
static IDXGISwapChain* swapChain = nullptr;
static ID3D11RenderTargetView* renderTarget = nullptr;

// Menu state
static bool menuOpen = false;
static int selectedTab = 0;
static bool showHud = true;

static bool esp = false;
static bool hitboxes = true;
static bool xray = false;
static bool freecam = false;
static bool fly = false;
static bool speed = false;

static float boxThickness = 2.0f;
static float speedMultiplier = 1.0f;

void CleanupTarget()
{
    if (renderTarget) {
        renderTarget->Release();
        renderTarget = nullptr;
    }
}

bool CreateTarget()
{
    ID3D11Texture2D* buffer = nullptr;

    if (FAILED(swapChain->GetBuffer(
        0, IID_PPV_ARGS(&buffer)))) {
        return false;
    }

    HRESULT result = device->CreateRenderTargetView(
        buffer, nullptr, &renderTarget
    );

    buffer->Release();
    return SUCCEEDED(result);
}

bool InitDirectX(HWND window)
{
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = window;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL level{};
    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_0
    };

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        levels,
        1,
        D3D11_SDK_VERSION,
        &desc,
        &swapChain,
        &device,
        &level,
        &context
    );

    return SUCCEEDED(result) && CreateTarget();
}

void CleanupDirectX()
{
    CleanupTarget();

    if (swapChain) swapChain->Release();
    if (context) context->Release();
    if (device) device->Release();
}

// Find the Minecraft process
DWORD FindMinecraftPID()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0
    );

    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    DWORD result = 0;

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(
                entry.szExeFile,
                L"Minecraft.Windows.exe"
            ) == 0) {
                result = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return result;
}

struct WindowSearch
{
    DWORD pid;
    HWND found;
    long long area;
};

BOOL CALLBACK FindWindowCallback(HWND hwnd, LPARAM param)
{
    auto* search = reinterpret_cast<WindowSearch*>(param);

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid != search->pid || !IsWindowVisible(hwnd))
        return TRUE;

    if (GetWindow(hwnd, GW_OWNER))
        return TRUE;

    RECT rect{};
    if (!GetWindowRect(hwnd, &rect))
        return TRUE;

    long long width = rect.right - rect.left;
    long long height = rect.bottom - rect.top;

    if (width < 320 || height < 240)
        return TRUE;

    long long area = width * height;

    if (area > search->area) {
        search->area = area;
        search->found = hwnd;
    }

    return TRUE;
}

HWND FindMinecraftWindow()
{
    DWORD pid = FindMinecraftPID();

    if (!pid)
        return nullptr;

    WindowSearch search{pid, nullptr, 0};

    EnumWindows(
        FindWindowCallback,
        reinterpret_cast<LPARAM>(&search)
    );

    return search.found;
}

// Switch between clickable and click-through mode
void UpdateInputMode()
{
    LONG_PTR style = GetWindowLongPtrW(
        overlay, GWL_EXSTYLE
    );

    if (menuOpen)
        style &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
    else
        style |= WS_EX_TRANSPARENT;

    SetWindowLongPtrW(
        overlay, GWL_EXSTYLE, style
    );

    SetWindowPos(
        overlay,
        HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE |
        SWP_NOSIZE |
        SWP_FRAMECHANGED |
        SWP_NOACTIVATE
    );

    if (menuOpen)
        SetForegroundWindow(overlay);
    else if (IsWindow(minecraft))
        SetForegroundWindow(minecraft);
}

// Draw clickable menu
void DrawMenu()
{
    ImGui::SetNextWindowSize(
        ImVec2(650, 420),
        ImGuiCond_FirstUseEver
    );

    ImGui::SetNextWindowPos(
        ImVec2(60, 60),
        ImGuiCond_FirstUseEver
    );

    if (ImGui::Begin("BedrockNative", &menuOpen)) {

        ImGui::Text("BEDROCK NATIVE");
        ImGui::SameLine();
        ImGui::TextDisabled("Minecraft 1.26.65");

        ImGui::Separator();

        ImGui::BeginChild(
            "Sidebar",
            ImVec2(145, 0)
        );

        const char* categories[] = {
            "Visuals",
            "Movement",
            "Combat",
            "World",
            "Settings"
        };

        for (int i = 0; i < 5; i++) {
            if (ImGui::Selectable(
                categories[i],
                selectedTab == i
            )) {
                selectedTab = i;
            }
        }

        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("Features");

        if (selectedTab == 0) {
            ImGui::Text("VISUALS");
            ImGui::Separator();

            ImGui::Checkbox("ESP", &esp);
            ImGui::Checkbox("Hitboxes", &hitboxes);
            ImGui::Checkbox("X-ray", &xray);

            ImGui::SliderFloat(
                "Box Thickness",
                &boxThickness,
                1.0f,
                5.0f
            );
        }

        if (selectedTab == 1) {
            ImGui::Text("MOVEMENT");
            ImGui::Separator();

            ImGui::Checkbox("Freecam", &freecam);
            ImGui::Checkbox("Fly", &fly);
            ImGui::Checkbox("Speed", &speed);

            ImGui::SliderFloat(
                "Speed Multiplier",
                &speedMultiplier,
                1.0f,
                10.0f
            );
        }

        if (selectedTab == 2) {
            ImGui::Text("COMBAT");
            ImGui::Separator();
            ImGui::Text("Modules coming later");
        }

        if (selectedTab == 3) {
            ImGui::Text("WORLD");
            ImGui::Separator();
            ImGui::Text("Modules coming later");
        }

        if (selectedTab == 4) {
            ImGui::Text("SETTINGS");
            ImGui::Separator();

            ImGui::Text("Right Shift: Menu");
            ImGui::Text("End: Exit");
            ImGui::Separator();
            ImGui::Checkbox("Show HUD", &showHud);
        }

        ImGui::Separator();
        ImGui::TextDisabled(
            "GUI prototype - switches are not connected yet."
        );

        ImGui::EndChild();
    }

    ImGui::End();
}


// ============================================================
// LIVE HUD
// ============================================================

void DrawHud()
{
    if (!showHud)
        return;

    DWORD minecraftPid = 0;

    if (IsWindow(minecraft))
    {
        GetWindowThreadProcessId(
            minecraft,
            &minecraftPid
        );
    }

    char hudText[160]{};

    std::snprintf(
        hudText,
        sizeof(hudText),
        "BedrockNative | Minecraft PID: %lu | Overlay: %.0f FPS",
        static_cast<unsigned long>(minecraftPid),
        ImGui::GetIO().Framerate
    );

    ImDrawList* draw = ImGui::GetForegroundDrawList();

    const ImVec2 position(16.0f, 16.0f);
    const ImVec2 textSize = ImGui::CalcTextSize(hudText);

    const ImVec2 bottomRight(
        position.x + textSize.x + 24.0f,
        position.y + textSize.y + 16.0f
    );

    // Dark HUD background.
    draw->AddRectFilled(
        position,
        bottomRight,
        IM_COL32(20, 25, 36, 235),
        6.0f
    );

    // Blue outline.
    draw->AddRect(
        position,
        bottomRight,
        IM_COL32(62, 126, 210, 220),
        6.0f
    );

    // Live status text.
    draw->AddText(
        ImVec2(position.x + 12.0f, position.y + 8.0f),
        IM_COL32(235, 240, 250, 255),
        hudText
    );
}

LRESULT CALLBACK WindowProc(
    HWND hwnd, UINT msg, WPARAM wp, LPARAM lp
)
{
    if (ImGui::GetCurrentContext() &&
        ImGui_ImplWin32_WndProcHandler(
            hwnd, msg, wp, lp
        )) {
        return 1;
    }

    switch (msg) {
    case WM_SIZE:
        if (swapChain && wp != SIZE_MINIMIZED) {
            UINT w = LOWORD(lp);
            UINT h = HIWORD(lp);

            if (w && h) {
                CleanupTarget();

                if (SUCCEEDED(swapChain->ResizeBuffers(
                    0, w, h, DXGI_FORMAT_UNKNOWN, 0
                ))) {
                    CreateTarget();
                }
            }
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

int main()
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"BedrockNativeOverlay";

    if (!RegisterClassExW(&wc))
        return 1;

    overlay = CreateWindowExW(
        WS_EX_TOPMOST |
        WS_EX_LAYERED |
        WS_EX_TOOLWINDOW |
        WS_EX_TRANSPARENT,
        wc.lpszClassName,
        L"BedrockNative",
        WS_POPUP,
        0, 0, 1280, 720,
        nullptr, nullptr,
        wc.hInstance, nullptr
    );

    if (!overlay)
        return 1;

    SetLayeredWindowAttributes(
        overlay,
        RGB(0, 0, 0),
        0,
        LWA_COLORKEY
    );

    if (!InitDirectX(overlay)) {
        CleanupDirectX();
        DestroyWindow(overlay);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui::GetStyle().WindowRounding = 8.0f;
    ImGui::GetStyle().FrameRounding = 5.0f;
    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplWin32_Init(overlay);
    ImGui_ImplDX11_Init(device, context);

    std::cout << "BedrockNative overlay started\n";

    MSG msg{};
    bool running = true;
    bool shiftWasDown = false;

    ULONGLONG nextSearch = 0;
    RECT lastRect{};
    bool rectKnown = false;

    while (running) {

        while (PeekMessageW(
            &msg, nullptr, 0, 0, PM_REMOVE
        )) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (!running)
            break;

        if (GetAsyncKeyState(VK_END) & 1)
            break;

        if (!IsWindow(minecraft) ||
            GetTickCount64() >= nextSearch) {

            HWND found = FindMinecraftWindow();

            if (found != minecraft)
                rectKnown = false;

            minecraft = found;
            nextSearch = GetTickCount64() + 1000;
        }

        if (!minecraft || IsIconic(minecraft)) {
            ShowWindow(overlay, SW_HIDE);
            Sleep(80);
            continue;
        }

        RECT rect{};

        if (!GetWindowRect(minecraft, &rect)) {
            Sleep(20);
            continue;
        }

        if (!rectKnown ||
            !EqualRect(&rect, &lastRect)) {

            SetWindowPos(
                overlay,
                HWND_TOPMOST,
                rect.left,
                rect.top,
                rect.right - rect.left,
                rect.bottom - rect.top,
                SWP_NOACTIVATE | SWP_SHOWWINDOW
            );

            lastRect = rect;
            rectKnown = true;
        }

        HWND foreground = GetForegroundWindow();

        if (foreground != minecraft &&
            foreground != overlay) {

            ShowWindow(overlay, SW_HIDE);
            Sleep(20);
            continue;
        }

        if (!IsWindowVisible(overlay))
            ShowWindow(overlay, SW_SHOWNOACTIVATE);

        bool shiftDown =
            (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0;

        if (shiftDown && !shiftWasDown) {
            menuOpen = !menuOpen;
            UpdateInputMode();
        }

        shiftWasDown = shiftDown;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        DrawHud();

        bool wasOpen = menuOpen;

        if (menuOpen)
            DrawMenu();

        ImGui::Render();

        if (wasOpen && !menuOpen)
            UpdateInputMode();

        if (!renderTarget) {
            Sleep(15);
            continue;
        }

        const float clearColor[4] = {
            0.0f, 0.0f, 0.0f, 1.0f
        };

        context->OMSetRenderTargets(
            1, &renderTarget, nullptr
        );

        context->ClearRenderTargetView(
            renderTarget, clearColor
        );

        ImGui_ImplDX11_RenderDrawData(
            ImGui::GetDrawData()
        );

        swapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDirectX();

    DestroyWindow(overlay);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}
