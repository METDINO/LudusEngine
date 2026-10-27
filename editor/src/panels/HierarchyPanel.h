#pragma once

// ============================================================================
//  HierarchyPanel.h - the Hierarchy: every entity in the scene, as a tree.
//
//  This is the same panel Unity puts on the left. Parent/child relationships
//  show as nesting, clicking selects, and the selection is what the Inspector
//  displays.
//
//  What it can do:
//    * click to select
//    * right-click for Rename, Duplicate, Unparent and Destroy
//    * "+ Create Entity" for a new empty one
//    * a filter box for finding something in a big scene
//    * drag a texture or a script onto a row to attach it
//    * drag one ENTITY onto another to make it a child of it, and drag it
//      out onto the empty space below the tree to give it no parent again
//
//  ==========================================================================
//  REPARENTING IS RECORDED WHILE DRAWING AND APPLIED AFTERWARDS.
//
//  Never in the middle of the tree. DrawNode sits between TreeNodeEx and
//  TreePop while it draws a row's children, and it is walking the parent's
//  list of children to do it. Changing who is whose child at that moment
//  moves entries in the very vector being walked, and rewrites the shape of
//  a tree ImGui is half-way through laying out.
//
//  So a drop writes down what was asked for and nothing else. Draw() carries
//  it out at the end, when the tree is finished and no node is open. This is
//  the same reason Destroy goes through DeferredOps rather than deleting the
//  entity where it stands.
//
//  The selected entity is outlined in yellow in the Scene view, which is one
//  extra call and is what makes selection feel real rather than like a row in
//  a list.
//
//  ==========================================================================
//  ONE RULE: THIS PANEL REMEMBERS AN EntityId, NEVER AN Entity*.
//
//  A panel that cached a pointer would crash the first time an entity was
//  destroyed while it was still selected - which happens constantly, because
//  Destroy is a menu item right there. The selection is an EntityId held in
//  EditorState, and it is looked up fresh every frame; an id belonging to
//  something that no longer exists simply comes back as nothing.
//
//  That is exactly the argument EntityId.h makes, arriving from the tools
//  side. It is also why real engines adopt ids the moment they grow an editor.
// ============================================================================

#include "Panel.h"

#include <engine/scene/Entity.h>

namespace editor {

// The id ImGui matches a dragged ENTITY on, as opposed to a dragged file -
// those live in AssetDragDrop.h. Defined once and used by both ends for the
// reason given there: a typo in a payload id fails silently, and a drop that
// is quietly never accepted gives you nothing to debug.
inline constexpr const char* kPayloadEntity = "HIERARCHY_ENTITY";

class HierarchyPanel final : public Panel {
public:
    const char* Title() const override { return "Hierarchy"; }
    void        Draw() override;

private:
    void DrawNode(eng::Entity& entity);
    void DrawContextMenu(eng::Entity& entity);
    void DrawRenamePopup();

    // Makes the row just drawn draggable, and lets an entity be dropped onto
    // it. Both are called straight after the row, because ImGui attaches
    // drag sources and drop targets to the most recent item.
    void DrawEntityDragSource(eng::Entity& entity);
    void DrawEntityDropTarget(eng::EntityId newParent);

    // The empty space under the tree, which only becomes a drop target while
    // an entity is actually being dragged. Dropping there means "no parent".
    void DrawRootDropZone();

    // Whether making `child` a child of `parent` is allowed, asked BEFORE the
    // drop is accepted so an impossible target never lights up. A null
    // `parent` means the scene root, which is always allowed.
    bool ReparentIsLegal(eng::EntityId child, eng::EntityId parent) const;

    void RequestReparent(eng::EntityId child, eng::EntityId parent);
    void ApplyPendingReparent();

    // What a drop asked for, carried out at the end of Draw. Ids rather than
    // pointers, like everything else here: either end can be destroyed
    // between the drop and the moment this is acted on.
    //
    // A NULL parent is not "nothing to do" - it means "give this entity no
    // parent at all", which is how unparenting is expressed. `requested` is
    // what tells the two apart.
    struct PendingReparent {
        eng::EntityId child{};
        eng::EntityId parent{};
        bool          requested = false;
    };
    PendingReparent m_pendingReparent{};

    char m_filter[96] = {};

    // The rename dialog targets an ID, not a pointer or a row number: the
    // entity can be destroyed between opening the dialog and pressing OK.
    eng::EntityId m_renameTarget{};
    char          m_renameBuffer[128] = {};
    bool          m_openRenamePopup   = false;
};

} // namespace editor
