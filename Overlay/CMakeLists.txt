cmake_minimum_required(VERSION 3.21)
project(BedrockNativeOverlay LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.93.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

set(IMGUI "${imgui_SOURCE_DIR}")

add_executable(BedrockNativeOverlay
    main.cpp
    GameTelemetry.cpp
    ../MemoryReader.cpp
    ../EntityTracker.cpp
    ../StateVectorDiscovery.cpp
    "${IMGUI}/imgui.cpp"
    "${IMGUI}/imgui_draw.cpp"
    "${IMGUI}/imgui_tables.cpp"
    "${IMGUI}/imgui_widgets.cpp"
    "${IMGUI}/backends/imgui_impl_win32.cpp"
    "${IMGUI}/backends/imgui_impl_dx11.cpp"
)

target_include_directories(BedrockNativeOverlay PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${IMGUI}"
    "${IMGUI}/backends"
)

target_compile_features(BedrockNativeOverlay PRIVATE cxx_std_17)

target_compile_definitions(BedrockNativeOverlay PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)

target_link_libraries(BedrockNativeOverlay PRIVATE
    d3d11 dxgi user32 gdi32
)

if(MSVC)
    target_compile_options(BedrockNativeOverlay PRIVATE /W4 /EHsc)
endif()
