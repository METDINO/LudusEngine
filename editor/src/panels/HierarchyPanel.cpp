// ============================================================================
//  HierarchyPanel.cpp - the tree of entities. See HierarchyPanel.h.
// ============================================================================

#include "panels/HierarchyPanel.h"

#include "AssetDragDrop.h"
#include "EditorApp.h"
#include "ObjectMenu.h"

#include <imgui.h>

#include <cstdio>
#include <cstring>   // memcpy, for reading a drag payload back out
#include <vector>

namespace editor {

void HierarchyPanel::DrawNode(eng::Entity& entity) {
    EditorState&      state     = EditorState::Get();
    eng::Scene&       scene     = eng::Engine::Get().GetScene();
    eng::Transform2D& transform = entity.Transform();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                               ImGuiTreeNodeFlags_SpanAvailWidth |
                               ImGuiTreeNodeFlags_DefaultOpen;

    // An entity with no children is a leaf: no expand arrow, and no matching
    // TreePop needed later.
    //
    // ASKED ONCE AND REMEMBERED, rather than asking Children() again further
    // down to decide whether to call TreePop. Those two answers have to
    // agree: ImGui pushes onto its id stack only for a non-leaf, and popping
    // when nothing was pushed takes the stack depth negative, which it then
    // shifts by. Deciding twice means a child list that changed in between
    // produces a crash nowhere near the code that caused it.
    const bool isLeaf = transform.Children().empty();
    if (isLeaf) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (state.selected == entity.Id()) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    // The hidden part after ## is the entity's id rather than its name,
    // because two entities can end up displaying the same name and ImGui would
    // treat them as one row. See the note about widget identity in Panel.h.
    char label[192];
    std::snprintf(label, sizeof(label), "%s##%d_%d", entity.Name().c_str(),
                  entity.Id().index, entity.Id().generation);

    const bool open = ImGui::TreeNodeEx(label, flags);

    // This row can be picked up and dropped on another one. Before the click
    // handling below, because a drag is not a click and ImGui needs to see
    // the source first to decide which it is.
    DrawEntityDragSource(entity);

    // IsItemToggledOpen tells clicking the expand arrow apart from clicking
    // the row, so opening a branch does not also change the selection.
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        state.selected = entity.Id();   // an ID. Never a pointer.
    }

    // A drop target for this row, placed immediately after the row was drawn
    // because ImGui attaches drop targets to the most recently drawn item.
    // Drop a texture here to give this entity a sprite; drop a .cpp to attach
    // that script.
    AcceptAssetDropOnEntity(entity.Id());

    // ...and a second one for a dropped ENTITY, which parents it to this row.
    // Two separate target blocks on the same item is fine - the first one has
    // closed itself before this one opens.
    DrawEntityDropTarget(entity.Id());

    DrawContextMenu(entity);

    if (open && !isLeaf) {
        // A COPY OF THE LIST, not the list itself.
        //
        // Drawing a child can change this vector while it is being walked.
        // The Duplicate menu item is the one that does it today: a duplicated
        // entity is given the same parent as the original, so duplicating a
        // child appends to the very vector this loop is reading, and that
        // append reallocates. Both of a range-for's iterators then point at
        // freed memory, and the loop reads it on its next step.
        //
        // Copying a handful of pointers once per open branch is nothing, and
        // it makes the loop independent of anything the rows do. New rows
        // appear on the next frame, which is exactly when they should.
        const std::vector<eng::Transform2D*> children = transform.Children();

        for (eng::Transform2D* child : children) {
            // The transform tree knows about parenting; the scene's list of
            // entities is flat. So finding the child ENTITY for a child
            // TRANSFORM means searching for the entity whose transform this is.
            eng::Entity* childEntity = nullptr;
            scene.ForEach([&](eng::Entity& candidate) {
                if (&candidate.Transform() == child) {
                    childEntity = &candidate;
                }
            });
            if (childEntity != nullptr) {
                DrawNode(*childEntity);   // recursion: a child may have children
            }
        }
        ImGui::TreePop();
    }
}

void HierarchyPanel::DrawEntityDragSource(eng::Entity& entity) {
    if (!ImGui::BeginDragDropSource()) {
        return;
    }

    // THE ID IS COPIED INTO THE PAYLOAD, not a pointer to it. ImGui keeps
    // this handful of bytes for the whole drag and hands them back at the
    // drop, so nothing has to stay alive in the meantime - and an EntityId
    // that refers to something destroyed mid-drag is still safe to look up
    // and simply comes back as nothing.
    const eng::EntityId id = entity.Id();
    ImGui::SetDragDropPayload(kPayloadEntity, &id, sizeof(id));

    // What rides under the cursor. Without this the drag is invisible and
    // feels broken even when it works.
    ImGui::Text("%s", entity.Name().c_str());

    ImGui::EndDragDropSource();
}

bool HierarchyPanel::ReparentIsLegal(eng::EntityId child, eng::EntityId parent) const {
    eng::Scene&  scene       = eng::Engine::Get().GetScene();
    eng::Entity* childEntity = scene.Get(child);
    if (childEntity == nullptr) {
        return false;
    }

    // The scene root. Always allowed - anything can stop having a parent.
    if (parent.IsNull()) {
        return true;
    }

    // Onto itself. Checked separately from the descendant test below only
    // because it reads clearly; Transform2D refuses both.
    if (child == parent) {
        return false;
    }

    eng::Entity* parentEntity = scene.Get(parent);
    if (parentEntity == nullptr) {
        return false;
    }

    // THE ONE THAT MATTERS: dropping something onto its own descendant would
    // make a loop in the parent chain, and asking for a world position walks
    // that chain - so it would hang rather than misbehave visibly. Refused
    // here so the target never lights up; Transform2D::SetParent refuses it
    // again, because a rule this consequential is worth enforcing where it
    // cannot be bypassed.
    return !parentEntity->Transform().IsDescendantOf(&childEntity->Transform());
}

void HierarchyPanel::DrawEntityDropTarget(eng::EntityId newParent) {
    if (!ImGui::BeginDragDropTarget()) {
        return;
    }

    // LOOKED AT BEFORE IT IS ACCEPTED. GetDragDropPayload just reports what
    // is being dragged without claiming it, so an illegal target can decline
    // to light up at all rather than highlighting and then refusing on
    // release. A target that accepts and then does nothing is the worse of
    // the two: it looks like the feature is broken.
    if (const ImGuiPayload* peek = ImGui::GetDragDropPayload();
        peek != nullptr && peek->IsDataType(kPayloadEntity) &&
        peek->DataSize == static_cast<int>(sizeof(eng::EntityId))) {

        eng::EntityId dragged{};
        std::memcpy(&dragged, peek->Data, sizeof(dragged));

        if (ReparentIsLegal(dragged, newParent)) {
            if (ImGui::AcceptDragDropPayload(kPayloadEntity) != nullptr) {
                RequestReparent(dragged, newParent);
            }
        }
    }

    ImGui::EndDragDropTarget();
}

void HierarchyPanel::DrawRootDropZone() {
    // ONLY EXISTS WHILE AN ENTITY IS BEING DRAGGED.
    //
    // An invisible button sitting over the empty part of the panel the rest
    // of the time would swallow the right-click that opens the New Object
    // menu: that menu is asked for with NoOpenOverItems, and this would be an
    // item covering everything it needs empty space for.
    const ImGuiPayload* peek = ImGui::GetDragDropPayload();
    if (peek == nullptr || !peek->IsDataType(kPayloadEntity) ||
        peek->DataSize != static_cast<int>(sizeof(eng::EntityId))) {
        return;
    }

    eng::EntityId dragged{};
    std::memcpy(&dragged, peek->Data, sizeof(dragged));

    // Nothing to offer for something that is already at the root - saying
    // "drop here to remove the parent" to an entity with no parent is noise.
    eng::Entity* draggedEntity = eng::Engine::Get().GetScene().Get(dragged);
    if (draggedEntity == nullptr || draggedEntity->Transform().Parent() == nullptr) {
        return;
    }

    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float  height    = (available.y > 56.0f) ? available.y : 56.0f;
    const ImVec2 topLeft   = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton("##scene-root-drop", ImVec2(available.x, height));
    const bool hovered     = ImGui::IsItemHovered();
    const ImVec2 bottomRight = ImGui::GetItemRectMax();

    // Drawn by hand rather than as a button, because this is a target rather
    // than something to press: an outline that brightens when the drag is
    // over it says "let go here" without looking clickable the rest of the
    // time.
    ImDrawList*    draw   = ImGui::GetWindowDrawList();
    const ImU32    colour = hovered ? IM_COL32(120, 190, 250, 255)
                                    : IM_COL32(120, 190, 250, 110);
    draw->AddRect(topLeft, bottomRight, colour, 4.0f, 0, hovered ? 2.0f : 1.0f);

    const char*  text     = "drop here to remove the parent";
    const ImVec2 textSize = ImGui::CalcTextSize(text);
    draw->AddText(ImVec2(topLeft.x + (available.x - textSize.x) * 0.5f,
                         topLeft.y + (height - textSize.y) * 0.5f),
                  colour, text);

    if (ImGui::BeginDragDropTarget()) {
        if (ImGui::AcceptDragDropPayload(kPayloadEntity) != nullptr) {
            RequestReparent(dragged, eng::EntityId{});   // null: the scene root
        }
        ImGui::EndDragDropTarget();
    }
}

void HierarchyPanel::RequestReparent(eng::EntityId child, eng::EntityId parent) {
    m_pendingReparent.child     = child;
    m_pendingReparent.parent    = parent;
    m_pendingReparent.requested = true;
}

void HierarchyPanel::ApplyPendingReparent() {
    if (!m_pendingReparent.requested) {
        return;
    }

    // Taken and CLEARED FIRST, so that an early return below cannot leave a
    // request that tries again on every frame for the rest of the session.
    const PendingReparent request = m_pendingReparent;
    m_pendingReparent             = {};

    eng::Scene&  scene = eng::Engine::Get().GetScene();
    eng::Entity* child = scene.Get(request.child);
    if (child == nullptr) {
        // Destroyed between the drop and here. Nothing alarming - the id did
        // its job, which was to make this a question rather than a crash.
        return;
    }

    eng::Transform2D* parentTransform = nullptr;
    if (!request.parent.IsNull()) {
        eng::Entity* parent = scene.Get(request.parent);
        if (parent == nullptr) {
            ENGINE_LOG_ERROR(eng::Channels::kEditor,
                             "could not reparent '{}': what it was dropped on no "
                             "longer exists", child->Name());
            return;
        }
        parentTransform = &parent->Transform();
    }

    if (child->Transform().Parent() == parentTransform) {
        return;   // dropped back where it already was
    }

    // keepWorldTransform: TRUE. The entity stays exactly where it looks and
    // its local numbers are recomputed to suit the new parent. Anything else
    // makes things leap across the scene when you organise the tree, which
    // is the opposite of what tidying up should do.
    child->Transform().SetParent(parentTransform, true);
    EditorState::Get().dirty = true;

    if (parentTransform == nullptr) {
        ENGINE_LOG_INFO(eng::Channels::kEditor, "'{}' now has no parent",
                        child->Name());
    } else {
        ENGINE_LOG_INFO(eng::Channels::kEditor, "'{}' is now a child of '{}'",
                        child->Name(), scene.Get(request.parent)->Name());
    }
}

void HierarchyPanel::DrawContextMenu(eng::Entity& entity) {
    // BeginPopupContextItem opens a menu when the previous item is
    // right-clicked, and returns false on every frame it is not open.
    if (!ImGui::BeginPopupContextItem()) {
        return;
    }
    eng::Scene& scene = eng::Engine::Get().GetScene();

    // The same items as the menu bar's Objects menu, from the one list. See
    // ObjectMenu.h.
    if (ImGui::BeginMenu("New Object")) {
        if (const eng::EntityId created = DrawObjectMenuItems(); !created.IsNull()) {
            EditorState::Get().selected = created;
            EditorState::Get().dirty    = true;
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Rename...")) {
        m_renameTarget    = entity.Id();
        m_openRenamePopup = true;
        std::snprintf(m_renameBuffer, sizeof(m_renameBuffer), "%s", entity.Name().c_str());
    }

    // THE KEYBOARD-AND-MENU WAY OUT OF A PARENT, next to Rename because it
    // is the same kind of edit. Dragging onto the empty space below the tree
    // does the same thing, but a drag is a poor way to discover that the
    // option exists at all - and it is no use when the tree fills the panel
    // and there is no empty space left to drop into.
    //
    // Greyed out rather than hidden when there is no parent to remove: an
    // item that vanishes teaches nothing, one that is visibly unavailable
    // says what it would do and why it cannot right now.
    const bool hasParent = entity.Transform().Parent() != nullptr;
    if (ImGui::MenuItem("Unparent", nullptr, false, hasParent)) {
        RequestReparent(entity.Id(), eng::EntityId{});
    }
    if (!hasParent && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("This entity is already at the top level of the scene.");
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Duplicate")) {
        std::string        error;
        const eng::EntityId copy = scene.DuplicateEntity(entity.Id(), error);
        if (copy.IsNull()) {
            ENGINE_LOG_ERROR(eng::Channels::kEditor, "could not duplicate: {}", error);
        } else {
            // Select the copy, because the next thing anybody does after
            // duplicating something is move it.
            EditorState::Get().selected = copy;
            EditorState::Get().dirty    = true;
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Destroy")) {
        // Through the DEFERRED QUEUE, not straight away. Deleting an entity
        // from inside a panel, mid-frame, is exactly the problem DeferredOps
        // exists to prevent - and the editor does not get an exemption from
        // the engine's own rules. The entity disappears at the end of the tick.
        eng::DeferredOps::QueueDestroy(entity.Id());
        EditorState::Get().dirty = true;
    }
    ImGui::EndPopup();
}

void HierarchyPanel::DrawRenamePopup() {
    if (m_openRenamePopup) {
        ImGui::OpenPopup("Rename Entity");
        m_openRenamePopup = false;
    }

    // Centred, because a dialog that opens under the mouse in a docked editor
    // is as likely to be half off the screen as not.
    const ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (!ImGui::BeginPopupModal("Rename Entity", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    eng::Scene&  scene  = eng::Engine::Get().GetScene();
    eng::Entity* target = scene.Get(m_renameTarget);

    if (target == nullptr) {
        // Destroyed while the dialog was open. Noticed, not crashed on - which
        // is the whole reason the dialog remembers an id.
        ImGui::TextUnformatted("that entity no longer exists");
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
        return;
    }

    ImGui::SetNextItemWidth(320.0f);
    const bool submitted =
        ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer),
                         ImGuiInputTextFlags_EnterReturnsTrue);

    // Names are how the scene looks entities up, so two entities cannot share
    // one. Refused up front rather than after pressing the button.
    const bool taken = !scene.Find(m_renameBuffer).IsNull() &&
                       scene.Find(m_renameBuffer) != m_renameTarget;
    if (taken) {
        ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.35f, 1.0f),
                           "something else is already called that");
    }

    ImGui::BeginDisabled(taken || m_renameBuffer[0] == '\0');
    if ((submitted && !taken) || ImGui::Button("Rename", ImVec2(120, 0))) {
        if (scene.RenameEntity(m_renameTarget, m_renameBuffer)) {
            EditorState::Get().dirty = true;
        }
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void HierarchyPanel::Draw() {
    eng::Scene& scene = eng::Engine::Get().GetScene();

    // Routed through the same CreateObject every menu uses, so "an empty
    // entity" means one thing in one place. See ObjectMenu.h.
    if (ImGui::Button("+ Create Entity")) {
        if (const eng::EntityId created = CreateObject("Empty"); !created.IsNull()) {
            EditorState::Get().selected = created;
            EditorState::Get().dirty    = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("+ Object")) {
        ImGui::OpenPopup("newobject");
    }
    if (ImGui::BeginPopup("newobject")) {
        if (const eng::EntityId created = DrawObjectMenuItems(); !created.IsNull()) {
            EditorState::Get().selected = created;
            EditorState::Get().dirty    = true;
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(appears where the camera is looking)");

    ImGui::SetNextItemWidth(-1.0f);   // -1 means "use all the remaining width"
    ImGui::InputTextWithHint("##filter", "filter by name...", m_filter, sizeof(m_filter));

    ImGui::TextDisabled("%zu entities", scene.EntityCount());
    ImGui::Separator();

    const bool filtering = m_filter[0] != '\0';

    if (filtering) {
        // A flat list while filtering. A tree with most of its branches hidden
        // is harder to read than a list, and the point of a filter is to find
        // one thing.
        scene.ForEach([&](eng::Entity& entity) {
            if (entity.Name().find(m_filter) == std::string::npos) {
                return;
            }
            char label[192];
            std::snprintf(label, sizeof(label), "%s##%d_%d", entity.Name().c_str(),
                          entity.Id().index, entity.Id().generation);
            if (ImGui::Selectable(label, EditorState::Get().selected == entity.Id())) {
                EditorState::Get().selected = entity.Id();
            }
            DrawContextMenu(entity);
        });
    } else {
        // Only the entities with no parent are drawn here; DrawNode draws each
        // one's children itself.
        scene.ForEach([&](eng::Entity& entity) {
            if (entity.Transform().Parent() == nullptr) {
                DrawNode(entity);
            }
        });
    }

    // Outline whatever is selected, in the Scene view.
    //
    // Looked up from the ID every frame, which is the rule. A selection that
    // was destroyed simply comes back as nothing and no outline is drawn.
    if (eng::Entity* selected = scene.Get(EditorState::Get().selected);
        selected != nullptr) {
        const eng::Vec2 position = selected->Transform().WorldPosition();
        eng::Gizmos::Box(
            eng::AABB::FromCenterHalfExtents(position, eng::Vec2{22.0f, 22.0f}),
            eng::Color::Yellow(), 0.0f, eng::GizmoSpace::World,
            eng::GizmoCategory::Bounds);

        // ABOVE the outline rather than beside it. Beside it is where the
        // Scene view puts its move handle, and a name label sitting on top of
        // the handle you are trying to grab makes a tool feel broken.
        eng::Gizmos::Text(position + eng::Vec2{-20.0f, 34.0f}, selected->Name(),
                          eng::Color::Yellow(), 0.0f, eng::GizmoSpace::World,
                          eng::GizmoCategory::Bounds);
    }

    // The empty space below the tree, which becomes "give this no parent"
    // while an entity is being dragged and is nothing at all otherwise.
    DrawRootDropZone();

    // RIGHT-CLICKING EMPTY SPACE offers the same menu, which is where people
    // reach for it when the scene is empty and there is no row to click on.
    if (ImGui::BeginPopupContextWindow("hierarchy-background",
                                       ImGuiPopupFlags_MouseButtonRight |
                                           ImGuiPopupFlags_NoOpenOverItems)) {
        if (const eng::EntityId created = DrawObjectMenuItems(); !created.IsNull()) {
            EditorState::Get().selected = created;
            EditorState::Get().dirty    = true;
        }
        ImGui::EndPopup();
    }

    // ---- the keys this panel owns ----------------------------------------
    //
    // Both need the panel FOCUSED, not merely hovered, and both need
    // WantTextInput to be false. Delete while renaming an entity would
    // otherwise delete the entity instead of a character.
    //
    // Destroying goes through the DEFERRED QUEUE, like every other delete in
    // the editor: this runs while panels are still drawing and still holding
    // on to what they are describing. See scene/DeferredOps.h.
    const ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        !io.WantTextInput) {

        if (eng::Entity* selected = scene.Get(EditorState::Get().selected)) {
            if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
                eng::DeferredOps::QueueDestroy(selected->Id());
                EditorState::Get().dirty = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
                m_renameTarget    = selected->Id();
                m_openRenamePopup = true;
                std::snprintf(m_renameBuffer, sizeof(m_renameBuffer), "%s",
                              selected->Name().c_str());
            }
        }
    }

    DrawRenamePopup();

    // LAST, when the tree is finished and no node is still open. Changing
    // who is whose child rewrites the lists the tree above was walking, so
    // it waits until there is nothing left walking them. See HierarchyPanel.h.
    ApplyPendingReparent();
}

} // namespace editor
