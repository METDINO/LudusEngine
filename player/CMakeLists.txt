# =============================================================================
#  The player executable - a game, running on its own without the editor.
#
#  THIS IS THE EXPORT TEMPLATE. It is what a finished game ships as: this
#  executable, engine.dll, SDL3.dll, the C++ runtime, assets/ and config/.
#  "Building a game" is copying those things into a folder - there is no
#  compiler involved, which is why shipping a game does not require the person
#  doing it to have Visual Studio installed.
#
#  It contains NO GAME. The scene file places the entities and the scripts in
#  assets/scripts/ give them their behaviour, so the same executable runs any
#  game built with this engine. See src/main.cpp.
#
#  Everything in here uses the engine's public headers and nothing else; it
#  cannot reach SDL, because the engine links SDL privately. That constraint is
#  the point: if a game can be written entirely against the engine's public
#  interface, the interface is complete.
# =============================================================================

# Globbed for the same reason the engine is - see engine/CMakeLists.txt. Adding
# a file to the player never needs an edit here.
file(GLOB_RECURSE PLAYER_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE PLAYER_HEADERS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_executable(player ${PLAYER_SOURCES} ${PLAYER_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PLAYER_SOURCES} ${PLAYER_HEADERS})

target_include_directories(player PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# The engine, and nothing else.
#
# The project's scripts are deliberately NOT linked here. They are LOADED at
# run time from .build/, exactly as the editor loads them - see
# engine/include/engine/scene/ScriptLibrary.h. One mechanism, used by both
# programs, so a script cannot work in one and not the other.
target_link_libraries(player PRIVATE Engine2D::engine)


engine_set_warnings(player)

# ---------------------------------------------------------------------------
#  NO CONSOLE WINDOW. A game that opens a black terminal beside itself looks
#  unfinished, and a player has nothing to read in it.
#
#  WIN32_EXECUTABLE asks the linker for /SUBSYSTEM:WINDOWS, which is what stops
#  Windows creating that window. On its own it also changes which function the
#  program starts at - the linker then wants WinMain and the build fails with
#
#      LNK2019: unresolved external symbol WinMain
#
#  because this program has an ordinary main(). /ENTRY:mainCRTStartup says
#  "keep starting at main", which is the whole fix. SDL ships a WinMain shim
#  for this, but the player deliberately cannot reach SDL - the engine keeps it
#  private - so the linker option is the right answer here.
#
#  NOTHING IS LOST BY HIDING IT. The log writes to three places at once: the
#  terminal, logs/engine.log, and an in-memory buffer the editor's Console panel
#  reads. Only the first one goes away. See engine/src/core/Log.cpp.
# ---------------------------------------------------------------------------
if(WIN32)
    set_target_properties(player PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_link_options(player PRIVATE /ENTRY:mainCRTStartup)
    endif()
endif()
