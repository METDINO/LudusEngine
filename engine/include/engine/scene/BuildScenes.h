#pragma once

// ============================================================================
//  BuildScenes.h - which scenes are in the game, and in what order.
//
//  This is the engine's equivalent of Unity's Build Settings scene list, and
//  it does the same three jobs:
//
//    1. IT DECIDES WHAT SHIPS. A scene that is not on this list is not packed
//       into the exported game. A project accumulates test levels, a scratch
//       scene somebody was debugging in, and a copy of level 3 from before the
//       rewrite - and none of that belongs in front of a player.
//
//    2. IT DECIDES WHAT LOADS FIRST. Index 0 is the scene the game opens with.
//       Reorder the list and you have changed which level the game starts on,
//       with no code and no rebuild.
//
//    3. IT GIVES EVERY SCENE A NUMBER. A script can say "load scene 2" instead
//       of repeating a file name that somebody will eventually rename:
//
//           eng::Engine::Get().LoadSceneByIndex(2, error);
//
//  ==========================================================================
//  THE FILE: assets/scenes/build.json
//
//      {
//        "scenes": [
//          "scenes/menu.oscene",
//          "scenes/collector.oscene"
//        ]
//      }
//
//  An ORDERED ARRAY, not an object with numbers in it, because the order IS
//  the numbering. Writing the indices down as data would let the file disagree
//  with itself - two scenes claiming index 1, or a gap at 3 - and every reader
//  would then need to decide what that means.
//
//  It lives in scenes/ so that it is packed into scenes.bin with the scenes it
//  names, rather than being a loose file a player can edit to point the game
//  at something else. The editor's Build panel is what writes it.
//
//  ==========================================================================
//  IT IS ADVICE, NOT A RESTRICTION, INSIDE THE EDITOR.
//
//  You can still open any scene in the editor whether it is on the list or
//  not - the list is about what the BUILD contains. An editor that refused to
//  open a file because it was not shipping yet would be a tool fighting the
//  person using it.
//
//  A project with no build.json at all still works: the engine falls back to
//  the "startup.scene" value in config/engine.json, which is how every scene
//  was chosen before this existed.
// ============================================================================

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace eng {

class BuildScenes {
public:
    // Where the list lives, as a virtual path. One definition, used by the
    // engine that reads it and the editor that writes it.
    static constexpr const char* kVirtualPath = "scenes/build.json";

    // Reads the list. A MISSING FILE IS NOT AN ERROR - it means this project
    // has not used the Build panel yet, and the engine should fall back to
    // config/engine.json. Only a file that exists and cannot be understood
    // fills in outError.
    static bool Load(std::string& outError);

    static void Clear();

    static std::size_t Count();

    // The scene at an index, or an empty string when the index is out of
    // range. Returning empty rather than throwing because "scene 7 in a game
    // with three scenes" is a mistake in a script, and a script's mistake
    // should be a message in the Console rather than a crash the player sees.
    static const std::string& PathAt(std::size_t index);

    // Where a scene sits in the list, or -1 when it is not on it. This is what
    // answers "is the scene I am editing actually going to ship?".
    static int IndexOf(std::string_view virtualPath);

    static const std::vector<std::string>& All();

    // ---- the editor's side -----------------------------------------------

    // Replaces the list and writes it to disk. Editor only; a shipped game has
    // nothing to say about which scenes it contains.
    static bool Save(const std::vector<std::string>& scenes, std::string& outError);
};

} // namespace eng
