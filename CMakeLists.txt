cmake_minimum_required(VERSION 3.20)
project(QuantumEngine VERSION 1.0.0 LANGUAGES CXX C)

# ============================================================================
# Quantum Engine - Graphical Browser Engine
# ============================================================================
# Architecture:
#   - C++17 for core logic and lifecycle management
#   - webview library for embedded web rendering (WebView2 on Windows)
#   - Browser chrome (UI) rendered as HTML/CSS/JS inside the webview
#   - Cross-platform: works on Windows, Linux, macOS
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# --- Fetch webview library ---
include(FetchContent)

FetchContent_Declare(
    webview
    GIT_REPOSITORY https://github.com/webview/webview.git
    GIT_TAG        0.12.0
    GIT_SHALLOW    TRUE
)

set(WEBVIEW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WEBVIEW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(webview)

# ============================================================================
# Source files
# ============================================================================
add_executable(QuantumEngine src/main.cpp src/browser.cpp)

target_include_directories(QuantumEngine PRIVATE src)
target_link_libraries(QuantumEngine PRIVATE webview_core_static)

# --- Platform-specific settings ---
if(WIN32)
    set_target_properties(QuantumEngine PROPERTIES WIN32_EXECUTABLE TRUE)
    set_property(TARGET QuantumEngine PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    target_link_libraries(QuantumEngine PRIVATE comctl32 shlwapi ole32 oleaut32 user32 gdi32)
    target_compile_definitions(QuantumEngine PRIVATE UNICODE _UNICODE)
endif()

if(MSVC)
    target_compile_options(QuantumEngine PRIVATE /W4)
else()
    target_compile_options(QuantumEngine PRIVATE -Wall -Wextra)
endif()
