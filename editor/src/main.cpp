// ============================================================================
//  main.cpp - where the editor starts.
//
//  Deliberately dull: build the application object, start it, run it, stop it.
//  Everything interesting is in EditorApp or in one of the panels.
//
//  There are two programs in this project:
//    editor   - this one, the development environment
//    player   - a game running on its own, with no editor at all
// ============================================================================

#include "EditorApp.h"
#include "GameExport.h"
#include "ScriptBuild.h"

#include <engine/core/Config.h>
#include <engine/core/Log.h>
#include <engine/fs/FileSystem.h>

#include <cstdio>
#include <cstring>

#if defined(_WIN32)
// WIN32_LEAN_AND_MEAN drops the half of windows.h nothing here uses, and
// NOMINMAX stops it defining min and max as MACROS, which would then break
// every std::min and std::max in anything included after it. Both are
// standard practice and the reason is worth knowing once.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace {

// ---------------------------------------------------------------------------
//  The editor is built as a WINDOWS program rather than a console one, so no
//  black terminal window opens beside it - the Console panel shows the same
//  log, better. See CMakeLists.txt.
//
//  The cost is that `editor --build-scripts`, which is meant to be run FROM a
//  terminal, would print into nowhere: a /SUBSYSTEM:WINDOWS program is not
//  connected to the console that launched it, even though one is right there.
//
//  AttachConsole(ATTACH_PARENT_PROCESS) borrows it. It fails harmlessly when
//  the program was started by double-clicking, because then there is no parent
//  console to borrow - which is exactly the case where the window should stay
//  hidden. So this is safe to call unconditionally and needs no flag.
//
//  CONOUT$ is Windows' name for "the console I am attached to, for writing".
//  Reopening stdout and stderr onto it is what makes printf and the log's
//  terminal output land in the right place.
// ---------------------------------------------------------------------------
void UseParentConsoleIfThereIsOne() {
#if defined(_WIN32)
    // ALREADY GOING SOMEWHERE? Leave it alone.
    //
    // `editor --build-scripts > build.txt` hands this program a real file
    // handle for stdout, and an automated build almost certainly does the
    // same. Pointing stdout at the console below would throw that redirection
    // away and write to the screen instead, so the file the caller asked for
    // would come back empty. A GUI program that was NOT redirected has no
    // stdout handle at all, which is how the two cases are told apart.
    const HANDLE existing = GetStdHandle(STD_OUTPUT_HANDLE);
    if (existing != nullptr && existing != INVALID_HANDLE_VALUE &&
        GetFileType(existing) != FILE_TYPE_UNKNOWN) {
        return;
    }

    if (AttachConsole(ATTACH_PARENT_PROCESS) == 0) {
        return;
    }
    FILE* reopened = nullptr;
    freopen_s(&reopened, "CONOUT$", "w", stdout);
    freopen_s(&reopened, "CONOUT$", "w", stderr);
#endif
}

// --build-game <folder>: export a standalone game and exit, without opening a
// window.
//
// The same work File -> Build Game... does, for the same reason
// --build-scripts exists: so that an automated build, or a script that
// packages a release, can do it with nobody at the keyboard. The menu item is
// the only way most people will ever want it; this is the way a build server
// does.
int BuildGameAndExit(const char* destination) {
    const eng::BootConfig config;
    eng::Log              log;
    eng::FileSystem       fileSystem;

    // THE FILE SYSTEM FIRST, which is the opposite of what reads naturally -
    // the log is the thing everything else writes to, so it looks like it
    // should come up first. But Log::Init resolves its own file path through
    // FileSystem, and with no project root found yet it would fall back to a
    // path relative to wherever this was launched from. Engine::Init brings
    // them up in this same order, for this same reason.
    fileSystem.Init(config);
    log.Init(config);

    editor::ScriptBuild::Init();
    const editor::GameExport::Result result = editor::GameExport::ExportTo(destination);

    fileSystem.Shutdown();
    log.Shutdown();

    return result.ok ? 0 : 1;
}

// --build-scripts: compile the project's scripts and exit, without opening a
// window.
//
// The editor does this on its own when it starts and whenever it regains
// focus, so this exists for the times there is nobody to press anything - a
// freshly cloned project, or an automated build that wants the library to be
// there before it runs the game.
int BuildScriptsAndExit() {
    // Only the two pieces this actually needs, started by hand: somewhere to
    // write messages, and the ability to turn a virtual path into a real one.
    // There is no window, no renderer and no scene.
    //
    // Both are subsystems, so both are objects with Init and Shutdown - the
    // same two the engine's ordered start-up calls. The settings are the
    // built-in defaults (logs/engine.log, at Info) because there is no engine
    // here to have read config/engine.json.
    const eng::BootConfig config;
    eng::Log              log;
    eng::FileSystem       fileSystem;

    // THE FILE SYSTEM FIRST, which is the opposite of what reads naturally -
    // the log is the thing everything else writes to, so it looks like it
    // should come up first. But Log::Init resolves its own file path through
    // FileSystem, and with no project root found yet it would fall back to a
    // path relative to wherever this was launched from. Engine::Init brings
    // them up in this same order, for this same reason.
    fileSystem.Init(config);
    log.Init(config);

    editor::ScriptBuild::Init();
    const editor::ScriptBuild::Result result = editor::ScriptBuild::BuildAndReload();

    fileSystem.Shutdown();
    log.Shutdown();
    return result.ok ? 0 : 1;
}

} // namespace

int main(int argc, char** argv) {
    // Done FIRST, and for every mode, so that anything printed below - the
    // usage text, a build's compiler errors, the log's terminal output - is
    // visible when the editor was launched from a terminal, and silently goes
    // nowhere when it was not.
    UseParentConsoleIfThereIsOne();

    // The editor takes at most one option, so there is no argument loop to
    // write - anything other than the few below is a mistake worth reporting.
    int frameLimit = 0;

    if (argc > 1) {
        if (std::strcmp(argv[1], "--build-scripts") == 0) {
            return BuildScriptsAndExit();
        }
        if (std::strcmp(argv[1], "--build-game") == 0) {
            if (argc < 3) {
                std::printf("--build-game needs a folder to export into\n");
                return 1;
            }
            return BuildGameAndExit(argv[2]);
        }
        if (std::strcmp(argv[1], "--frames") == 0) {
            if (argc < 3) {
                std::printf("--frames needs a number after it\n");
                return 1;
            }
            // Falls THROUGH to opening the editor, unlike the two above - this
            // option changes how long the editor runs rather than replacing
            // what it does. See EditorApp::Run.
            frameLimit = std::atoi(argv[2]);
        } else {
            const bool askedForHelp = std::strcmp(argv[1], "--help") == 0;
            std::printf(
                "editor - the development environment\n\n"
                "  (no options)             open the editor\n"
                "  --build-scripts          compile the project's scripts and exit\n"
                "  --build-game <folder>    export a standalone game and exit\n"
                "  --frames <N>             open the editor, run N frames, then exit\n");
            // Zero when they asked for help, non-zero when they mistyped.
            return askedForHelp ? 0 : 1;
        }
    }

    editor::EditorApp app;

    if (!app.Init()) {
        std::fprintf(stderr,
                     "the editor could not start. The messages above name the part "
                     "that failed.\n");
        app.Shutdown();
        return 1;   // a non-zero exit code means "something went wrong"
    }

    app.Run(frameLimit);
    app.Shutdown();
    return 0;
}
