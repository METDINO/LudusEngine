#pragma once

// ============================================================================
//  GameExport.h - turning the project into a folder somebody can double-click.
//
//  File -> Build Game... asks for a destination and copies everything a
//  standalone game needs into it. NOTHING IS COMPILED except the project's
//  scripts, which the editor already knows how to build. That is the whole
//  design, and it is the same one Unity and Godot use: the player executable
//  is built ahead of time and shipped with the editor, so "building a game"
//  is a copy. A student exporting a game does not need Visual Studio, and
//  neither does the person they send it to.
//
//  ==========================================================================
//  WHAT A STANDALONE GAME IS, EXACTLY
//
//      MyGame.exe                   player.exe, renamed after the folder
//      engine.dll
//      SDL3.dll
//      assets/scenes.bin            the scenes, and the settings
//      assets/textures.bin          the images
//      assets/userContent.dll       the compiled scripts
//
//  FOUR FILES AND A FOLDER. Everything that is not the program itself lives in
//  assets/, which is the shape somebody who has just unzipped a game expects:
//  the thing you double-click, the libraries beside it, and one folder of
//  content. There is no hidden directory, nothing called .build, and no
//  config/ for a player to go rummaging in - the settings are inside
//  scenes.bin, because they name the scene the game starts on.
//
//  THE SHAPE OF THAT FOLDER IS NOT ARBITRARY. FileSystem::Init finds the
//  project by starting at the executable and walking UP until it sees a folder
//  containing `assets`, so the exe has to sit beside assets/ for a game to
//  find its own files. And FileSystem::Resolve sends three prefixes -
//  config/, logs/ and .build/ - to that folder rather than into assets/, which
//  is why config/ and .build/ are siblings of assets/ and not inside it. Get
//  either of those wrong and the game starts, finds nothing, and draws an
//  empty window.
//
//  ==========================================================================
//  ONLY WHAT THE GAME USES IS PACKED. This is the most important rule here.
//
//  Being in assets/ is NOT a reason to ship something. A project collects
//  reference art, asset packs bought for a later level, half-finished sprites
//  and last week's experiments, and a folder beside a game is something
//  players can simply open. Shipping everything present is how unannounced
//  content, art you have no licence to redistribute, and the source of a
//  bought asset pack reach people who should not have them.
//
//  So the exporter reads the scene files, finds every string in them that
//  names a real file under assets/, and packs those. Measured on this project:
//  9 files packed out of 3,879 present. The log names every packed file and
//  reports how many were left behind, because "did anything go out that I did
//  not mean to send?" deserves a better answer than comparing folders by hand.
//
//  SCRIPT SOURCE is never packed either. The game runs from userContent.dll
//  and has no compiler to use .cpp files with, and a shipped game containing
//  its own source is a surprise nobody asked for. Unity does not ship your
//  .cs files either.
//
//  THE ONE THING THIS CANNOT SEE is an asset a script loads by a name it
//  builds while running - LoadTexture("textures/" + name). No scene mentions
//  it, so nothing can find it without running the game. assets/export.json
//  lists those; it is optional and most projects never need one.
//
//  THE C++ RUNTIME. engine.dll needs MSVCP140.dll and friends, and they are
//  not in the editor's folder to copy - they come from the system. A Release
//  build needs the Visual C++ Redistributable on the target machine, which is
//  a normal thing for a game installer to include. A DEBUG build needs the
//  debug runtime, which Microsoft does not allow anyone to redistribute, so a
//  Debug export only runs on a machine that has Visual Studio installed. The
//  export says so rather than letting somebody discover it from a stranger.
// ============================================================================

#include <string>

namespace editor {

class GameExport {
public:
    struct Result {
        bool        ok = false;
        std::string summary;   // one line, for the status bar
    };

    // What came back from the folder picker.
    //
    // THE OUTCOME IS CARRIED, NOT JUST THE PATH, because "the user cancelled"
    // and "the dialog failed" both used to look exactly like "nothing has
    // happened yet" - and a Build that silently does nothing is the single
    // worst way for this feature to fail. Every one of these now reaches the
    // status bar.
    enum class Choice {
        None,        // nobody has picked anything; the ordinary case every frame
        Chosen,      // a folder is waiting in `path`
        Cancelled,   // the dialog was dismissed
        Failed,      // the dialog itself could not run; `error` says why
    };

    struct Pending {
        Choice      choice = Choice::None;
        std::string path;
        std::string error;
    };

    // Opens the operating system's folder picker.
    //
    // RETURNS IMMEDIATELY. The choice arrives later, and - per SDL's
    // documentation - on a different thread on Windows. Nothing here touches
    // the engine.
    static void AskForDestination();

    // Main thread: collects whatever the picker decided, clearing it so one
    // choice runs one export.
    //
    // POLLED RATHER THAN PUSHED, and it has to be. The callback arrives on
    // SDL's dialog thread, and this engine's Log and LogBuffer have no locks
    // in them at all - so the callback cannot log, cannot touch the scene, and
    // cannot report anything itself. All it may do is leave a note. This is
    // where that note is read, on the thread that is allowed to act on it.
    static Pending TakePending();

    // Builds the scripts, then copies the game into `destination`.
    //
    // CALL THIS WHERE RELOADING SCRIPTS IS SAFE - the top of a frame, next to
    // RebuildScriptsIfChanged - because building them replaces the library the
    // running scene's script objects live in. Doing that in the middle of a
    // frame would pull them out from under the systems holding them.
    static Result ExportTo(const std::string& destination);
};

} // namespace editor
