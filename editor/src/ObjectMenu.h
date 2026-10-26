#pragma once

// ============================================================================
//  ObjectMenu.h - "what can I add to the scene?", in one list.
//
//  The same menu appears in two places: the Objects menu in the menu bar, and
//  the Hierarchy's right-click menu. Writing it twice would mean a new kind
//  of object appearing in one and not the other - so it is written once here
//  and both call it.
//
//  That is the same argument the View menu makes by generating itself from
//  the panel list rather than being typed out: a menu that has to be edited
//  in two places eventually only gets edited in one.
// ============================================================================

#include <engine/scene/EntityId.h>

#include <string_view>

namespace editor {

// Draws the menu ITEMS - no BeginMenu, no EndMenu. The caller decides whether
// they sit in a menu, a submenu or a popup, which is exactly the difference
// between the two places this is used.
//
// Returns the entity created, or a null id when nothing was chosen this
// frame. The caller selects it, marks the scene dirty, and so on - those are
// editor-state decisions this file should not be making.
eng::EntityId DrawObjectMenuItems();

// Creates one by name, without any menu. Used by the items above and
// available to anything else that wants to make a standard object - a test,
// or a future toolbar button.
//
// Known kinds: "Empty", "Camera", "Sprite".
eng::EntityId CreateObject(std::string_view kind);

} // namespace editor
