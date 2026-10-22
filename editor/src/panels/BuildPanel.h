#pragma once

// ============================================================================
//  BuildPanel.h - which scenes are in the game, and which one starts it.
//
//  This is the same panel Unity calls Build Settings, and it does the same
//  thing: an ordered list of the scenes that ship, where the order is the
//  numbering and the top one is where the game begins.
//
//  Three things follow from the list, and all three are worth saying out loud
//  because they are the reason this panel exists at all:
//
//    * A SCENE NOT ON THE LIST IS NOT IN THE BUILD. File -> Build Game... packs
//      what the list names, and nothing else. The test level you made to try
//      one collision, and the copy of level 2 from before the rewrite, stay on
//      your machine.
//
//    * THE TOP ONE LOADS FIRST. Drag a different scene to the top - or press
//      its up arrow until it gets there - and you have changed which level the
//      game opens on, with no code and no rebuild.
//
//    * EVERY SCENE GETS A NUMBER, shown beside it here, and a script can use
//      that number instead of a file name somebody will eventually rename:
//
//          eng::Engine::Get().LoadSceneByIndex(2, error);
//
//  ==========================================================================
//  THE LIST IS SAVED TO assets/scenes/build.json WHEN YOU PRESS SAVE.
//
//  Not on every click. Reordering a list is a fiddly business with several
//  intermediate states that are not what you meant, and writing each of them
//  to disk would make a mess of the file somebody is reviewing in version
//  control. The panel says plainly when there is something unsaved.
//
//  THE EDITOR STILL OPENS ANY SCENE whether it is on the list or not. The list
//  governs the BUILD. A tool that refused to open a file because it was not
//  shipping yet would be fighting the person using it - but the Hierarchy's
//  title does say when the scene you are looking at will not ship, because
//  that is a surprise worth having early rather than after an export.
// ============================================================================

#include "Panel.h"

#include <string>
#include <vector>

namespace editor {

class BuildPanel final : public Panel {
public:
    const char* Title() const override { return "Build"; }
    void        Draw() override;

private:
    void RefreshAvailable();
    void MoveEntry(int index, int delta);

    // The list being edited. Loaded from BuildScenes on the first frame, and
    // written back only when Save is pressed - see the note above.
    std::vector<std::string> m_included;

    // Every .oscene under assets/scenes/, for the "add a scene" list. Read
    // when the panel asks for it rather than every frame, because it touches
    // the disk.
    std::vector<std::string> m_available;

    bool m_loaded = false;
    bool m_dirty  = false;

    char m_status[160] = {};
};

} // namespace editor
