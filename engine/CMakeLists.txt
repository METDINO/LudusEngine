# =============================================================================
#  The engine library.
#
#  This is a STATIC LIBRARY, not an executable. It knows nothing about your
#  game. In Week 10 you will be handed a game spec and asked to build it
#  without opening any file in this directory. That test only means something
#  if the boundary is real from the start.
#
#  Add every new .cpp to ENGINE_SOURCES below. Headers do not strictly need
#  listing, but listing them makes them show up in IDE project trees, which
#  is worth the two seconds.
# =============================================================================

set(ENGINE_SOURCES
    src/platform/Window.cpp
    # Week 2: src/core/MemUtil.cpp, src/platform/EventPump.cpp
    src/core/ByteBuffer.cpp
    src/core/MemUtil.cpp
    src/platform/EventPump.cpp
    # Week 3: src/core/Log.cpp, src/core/Assert.cpp
    # ... one or two lines per week from here on
)

set(ENGINE_HEADERS
    include/engine/core/Types.h
    include/engine/platform/Window.h
    include/engine/core/ByteBuffer.h
    include/engine/core/MemUtil.h
    include/engine/platform/EventPump.h
)

add_library(engine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})
add_library(Engine2D::engine ALIAS engine)

# PUBLIC: anything linking engine also gets this include path, so the sandbox
# can write #include <engine/platform/Window.h>
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# SDL3 is PUBLIC for now because Window.h exposes SDL types. Part of your job
# in Week 3 is to make that stop being true, at which point this becomes
# PRIVATE and the sandbox loses the ability to touch SDL directly. That is a
# real architectural milestone - notice when it happens.
target_link_libraries(engine PUBLIC SDL3::SDL3)

if(ENGINE_WITH_IMGUI)
    target_sources(engine PRIVATE src/tools/EditorGui.cpp)
    target_link_libraries(engine PUBLIC imgui::imgui)
    target_compile_definitions(engine PUBLIC ENGINE_WITH_IMGUI)
endif()

engine_set_warnings(engine)
