# =============================================================================
#  2D Game Engine Architecture - root build
#  You will edit this file a handful of times across the semester.
#  Weeks 2, 5, 6, 8 and 9 each add one line or one option here.
# =============================================================================
cmake_minimum_required(VERSION 3.28) 


project(Engine2D
    VERSION 0.1.0
    DESCRIPTION "2D game engine, built one week at a time"
    LANGUAGES CXX)

# --- Language standard ------------------------------------------------------
# C++23. See docs/CPP23-TOOLCHAIN.md if your compiler rejects this.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Where build output goes ------------------------------------------------
# Everything lands in one place so the sandbox executable can find assets/
# by walking a known relative path. Week 9 depends on this being predictable.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# --- Options ----------------------------------------------------------------
option(ENGINE_BUILD_TESTS "Build the unit test target" ON)   # Week 2 turns this on for real
option(ENGINE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(ENGINE_WITH_IMGUI "Build the ImGui editor layer" ON)

include(cmake/Dependencies.cmake)
include(cmake/CompilerWarnings.cmake)

set(ENGINE_SANITIZER "none" CACHE STRING "none | address | thread")

add_subdirectory(engine)
add_subdirectory(sandbox)

# Week 2 adds a third target here: `editor`, the ImGui-based IDE you will
# build one panel at a time. It links `engine` exactly as `sandbox` does.
if(ENGINE_WITH_IMGUI)
    add_subdirectory(editor)
endif()

if(ENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/fsanitize=address")
        # Strip out incompatible runtime checks if CMake added them by default
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    endif()
endif()