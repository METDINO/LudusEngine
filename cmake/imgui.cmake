
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.9b-docking
    GIT_SHALLOW    TRUE)

# Populate only. There is nothing here for CMake to configure.
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp          # keep it - see note below
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)

# SYSTEM so ImGui's own headers do not trip your -Wall -Wextra settings.
target_include_directories(imgui SYSTEM PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)

target_link_libraries(imgui PUBLIC SDL3::SDL3)

add_library(imgui::imgui ALIAS imgui)

