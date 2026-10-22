#pragma once

// ============================================================================
//  ScriptLibrary.h - loading a project's compiled scripts while the program is
//  running.
//
//  ==========================================================================
//  WHY THIS EXISTS
//
//  A project's scripts are not part of the editor. They are compiled - by the
//  editor itself, see the editor's ScriptBuild - into a single library called
//  userContent.dll (or .so, or .dylib), which is then loaded here.
//
//  That is what makes the editor a finished program rather than something that
//  has to be rebuilt every time somebody writes a script. Adding a script
//  touches the project. It never touches the editor.
//
//  ==========================================================================
//  HOW A SCRIPT INSIDE THE LIBRARY FINDS THE ENGINE
//
//  It links against the same engine.dll the editor is already running. There
//  is exactly one engine in the process, so when a script calls
//  InputMap::IsDown it reads the same input the editor is feeding. That is the
//  whole reason the engine is a shared library - see engine/CMakeLists.txt.
//
//  Registration needs no extra work either. ENGINE_REGISTER_SCRIPT creates an
//  object at file scope, and the operating system runs those constructors as
//  part of loading the library - so the scripts announce themselves to
//  ScriptRegistry the moment Load() returns.
//
//  ==========================================================================
//  THE ORDER A RELOAD HAS TO HAPPEN IN, because getting it wrong is a crash
//  rather than a mistake you can see.
//
//  Everything the library created lives in the library's memory: the script
//  objects themselves, and the functions the registry stores for making more
//  of them. The instant the library is unloaded, all of that is gone. So:
//
//    1. every live script object is destroyed (its name is remembered)
//    2. the registry is emptied, because its entries point into the library
//    3. only now is the library unloaded
//    4. the new one is compiled and loaded, and re-registers itself
//    5. every script is rebound BY NAME to the new code
//
//  Unload() does 1 to 3. Load() does 4 and 5. Doing them in any other order
//  means calling a function that no longer exists.
// ============================================================================

#include <engine/core/Subsystem.h>

#include <string>
#include <string_view>

namespace eng {

class ScriptLibrary : public Subsystem {
public:
    // Loads the library from DefaultVirtualPath. A project with no scripts yet
    // is not a failure; see Load below.
    bool Init(const BootConfig& config) override;
    void Shutdown() override;

    // ---- where the compiled scripts live ---------------------------------
    //
    // THERE ARE TWO PLACES, because a project being developed and a game that
    // has shipped want different things from the same file.
    //
    //   WHILE DEVELOPING: .build/userContent.dll, beside the object files, the
    //     build script and the source list the editor writes next to it. It is
    //     build output, it is in .gitignore with the rest of the build output,
    //     and it is regenerated whenever a script changes.
    //
    //   IN A BUILT GAME: assets/userContent.dll, beside the packed scenes and
    //     textures. A shipped game has no build folder and should not look as
    //     though it does - a hidden directory called .build sitting next to
    //     the executable reads as leftover rubbish, and on Linux and macOS it
    //     is literally hidden from the person who just unzipped the game.
    //
    //     The compiled game code living in the data folder is also what Unity
    //     does, for what it is worth: <Game>_Data/Managed/.
    //
    // Both are VIRTUAL paths, so each resolves the same way on every platform
    // and from any working directory. The extension differs per platform -
    // .dll, .so, .dylib - which is why these are functions and not constants.
    static std::string DefaultVirtualPath();
    static std::string ShippedVirtualPath();

    // The one of the two above that actually exists, or an empty string when
    // neither does. This is what Init loads, and what the editor shows.
    static std::string FindLibraryPath();

    // Loads the compiled scripts, unloading anything already loaded first, and
    // rebinds every ScriptComponent in the current scene by name.
    //
    // A missing file is NOT an error: a project with no scripts yet is a
    // perfectly ordinary state. It returns false with an explanation only when
    // the file exists and could not be loaded.
    static bool Load(std::string_view virtualPath, std::string& outError);

    // Destroys every live script object, empties the registry and unloads the
    // library - in that order. Safe to call when nothing is loaded.
    static void Unload();

    static bool IsLoaded();

    // Where the currently loaded library came from. Empty when none is loaded.
    static const std::string& LoadedPath();

    // How many scripts the loaded library registered. Shown in the editor,
    // because "nothing happens when I press Play" and "no scripts loaded" are
    // the same fact and only one of them tells you what to do.
    static std::size_t ScriptCount();
};

} // namespace eng
