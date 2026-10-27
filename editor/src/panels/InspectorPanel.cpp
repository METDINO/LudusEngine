// ============================================================================
//  InspectorPanel.cpp - the Inspector. See InspectorPanel.h for the three
//  answers about editing while the game is running.
//
//  Each component type gets its own small draw function below. Adding a new
//  one means writing another and adding a line to the chain in
//  InspectorPanel::Draw - editor work, which touches no engine code at all.
// ============================================================================

#include "panels/InspectorPanel.h"

#include "AssetDragDrop.h"
#include "EditorApp.h"
#include "ShellOpen.h"

#include <imgui.h>
#include <imgui_internal.h>   // GetCurrentWindow, for a whole-window drop target

#include <algorithm>
#include <cstdio>
#include <set>
#include <string>

namespace editor {
namespace {

// ---------------------------------------------------------------------------
//  TransformComponent
// ---------------------------------------------------------------------------
void DrawTransform(eng::TransformComponent& component, bool running) {
    eng::Transform2D& transform = component.Transform();

    // Answer 2 from the header: while the game is running, a system may own
    // these values. They are shown but not editable, and the reason is written
    // on screen rather than left as a mystery.
    ImGui::BeginDisabled(running);

    // ImGui's DragFloat widgets want a plain array of floats, so the Vec2 is
    // unpacked into one, edited, and packed back if it changed. Each of these
    // returns true only on a frame the value was actually changed.
    float position[2] = {transform.LocalPosition().x, transform.LocalPosition().y};
    if (ImGui::DragFloat2("Position", position, 0.5f)) {
        transform.SetLocalPosition(eng::Vec2{position[0], position[1]});
        EditorState::Get().dirty = true;
    }

    // Shown in DEGREES, stored in radians. Every angle a person sees in this
    // editor is degrees; every angle the engine holds is radians. The
    // conversion happens here, at the edge, exactly once.
    float degrees = transform.LocalRotation() * eng::kRadToDeg;
    if (ImGui::DragFloat("Rotation", &degrees, 0.5f, -360.0f, 360.0f, "%.1f deg")) {
        transform.SetLocalRotation(degrees * eng::kDegToRad);
        EditorState::Get().dirty = true;
    }

    float scale[2] = {transform.LocalScale().x, transform.LocalScale().y};
    if (ImGui::DragFloat2("Scale", scale, 0.01f)) {
        transform.SetLocalScale(eng::Vec2{scale[0], scale[1]});
        EditorState::Get().dirty = true;
    }

    ImGui::EndDisabled();
    if (running) {
        ImGui::TextDisabled("read-only while playing - a system may own these. "
                            "Pause to edit.");
    }

    // The LOCAL values are above; the WORLD position is shown as well, because
    // for a child entity those two are different and the difference is exactly
    // what confuses people about parenting.
    const eng::Vec2 world = transform.WorldPosition();
    ImGui::TextDisabled("world position: %.2f, %.2f   (%d parent(s) above it)",
                        static_cast<double>(world.x), static_cast<double>(world.y),
                        transform.Depth());
}

// ---------------------------------------------------------------------------
//  SpriteComponent
// ---------------------------------------------------------------------------
void DrawSprite(eng::SpriteComponent& sprite) {
    char path[192];
    std::snprintf(path, sizeof(path), "%s", sprite.TexturePath().c_str());
    if (ImGui::InputText("Texture", path, sizeof(path),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        sprite.SetTexture(path);
        EditorState::Get().dirty = true;
    }
    ImGui::TextDisabled("press Enter to load, or drag an image from the Assets panel");

    // ImGui's colour picker works in 0-1 floats; the engine stores 0-255
    // bytes, because that is what an image editor shows you. Converted here.
    const eng::Color tint = sprite.Tint();
    float rgba[4] = {tint.r / 255.0f, tint.g / 255.0f, tint.b / 255.0f, tint.a / 255.0f};
    if (ImGui::ColorEdit4("Tint", rgba)) {
        sprite.SetTint(eng::Color{static_cast<unsigned char>(rgba[0] * 255.0f),
                                  static_cast<unsigned char>(rgba[1] * 255.0f),
                                  static_cast<unsigned char>(rgba[2] * 255.0f),
                                  static_cast<unsigned char>(rgba[3] * 255.0f)});
        EditorState::Get().dirty = true;
    }

    int layer = sprite.Layer();
    if (ImGui::DragInt("Draw order", &layer, 0.2f)) {
        sprite.SetLayer(layer);
        EditorState::Get().dirty = true;
    }
    ImGui::TextDisabled("higher numbers are drawn on top");

    // A preview of the actual image, plus how many things are sharing it.
    if (const eng::TextureRef& texture = sprite.GetTexture(); texture) {
        ImGui::Image(reinterpret_cast<ImTextureID>(texture->native), ImVec2(64, 64));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("%d x %d pixels", texture->width, texture->height);

        // use_count() is how many shared_ptrs are keeping this image loaded -
        // one per sprite using it, plus this preview. It is the clearest
        // possible demonstration of what shared ownership means.
        ImGui::TextDisabled("shared by %ld thing(s)",
                            static_cast<long>(texture.use_count()));
        if (texture->isPlaceholder) {
            ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.40f, 1.0f),
                               "this image failed to load");
        }
        ImGui::EndGroup();
    }
}

// ---------------------------------------------------------------------------
//  Colliders
// ---------------------------------------------------------------------------

// Collects every layer name mentioned anywhere in the scene, plus a few common
// ones, so the checkbox list below offers something sensible without anybody
// having to register layer names in advance.
//
// std::set keeps them sorted and refuses duplicates, which is exactly what is
// wanted here.
std::set<std::string> GatherLayerNames(eng::Scene& scene) {
    std::set<std::string> names{"Default", "Player", "Enemy", "Pickup", "World"};

    scene.ForEach([&](eng::Entity& entity) {
        entity.ForEachComponent([&](eng::Component& component) {
            if (auto* collider = dynamic_cast<eng::ColliderComponent*>(&component)) {
                names.insert(collider->Layer());
                for (const std::string& other : collider->CollidesWith()) {
                    if (other != eng::kCollisionLayerAll) {
                        names.insert(other);
                    }
                }
            }
        });
    });
    return names;
}

void DrawCollider(eng::ColliderComponent& collider, eng::Scene& scene) {
    bool trigger = collider.IsTrigger();
    if (ImGui::Checkbox("Trigger", &trigger)) {
        collider.SetTrigger(trigger);
        EditorState::Get().dirty = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(reports overlaps but does not block movement)");

    float offset[2] = {collider.Offset().x, collider.Offset().y};
    if (ImGui::DragFloat2("Offset", offset, 0.5f)) {
        collider.SetOffset(eng::Vec2{offset[0], offset[1]});
        EditorState::Get().dirty = true;
    }

    // The shape-specific field. Which one depends on what this actually is.
    if (collider.Shape() == eng::ColliderShape::Box) {
        auto& box = static_cast<eng::AABBColliderComponent&>(collider);
        float half[2] = {box.HalfExtents().x, box.HalfExtents().y};
        if (ImGui::DragFloat2("Half size", half, 0.5f, 0.0f, 10000.0f)) {
            box.SetHalfExtents(eng::Vec2{half[0], half[1]});
            EditorState::Get().dirty = true;
        }
        ImGui::TextDisabled("half the width and half the height");
    } else {
        auto& circle = static_cast<eng::CircleColliderComponent&>(collider);
        float radius = circle.Radius();
        if (ImGui::DragFloat("Radius", &radius, 0.5f, 0.0f, 10000.0f)) {
            circle.SetRadius(radius);
            EditorState::Get().dirty = true;
        }
    }

    ImGui::SeparatorText("Layers");

    char layer[64];
    std::snprintf(layer, sizeof(layer), "%s", collider.Layer().c_str());
    if (ImGui::InputText("Layer (what it is)", layer, sizeof(layer),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        collider.SetLayer(layer);
        EditorState::Get().dirty = true;
    }

    // "Collides with" as a set of checkboxes. Untick one and the collision
    // events for that pairing stop immediately - no rebuild, no restart, which
    // makes it easy to see the layer rules actually working.
    ImGui::TextUnformatted("Collides with:");

    std::vector<std::string> wanted = collider.CollidesWith();
    bool changed = false;

    // "All" is a special entry meaning "everything". When it is ticked the
    // individual boxes are irrelevant, so they are greyed out.
    bool all = std::find(wanted.begin(), wanted.end(), eng::kCollisionLayerAll) !=
               wanted.end();
    if (ImGui::Checkbox("All", &all)) {
        wanted.clear();
        if (all) {
            wanted.emplace_back(eng::kCollisionLayerAll);
        }
        changed = true;
    }

    ImGui::BeginDisabled(all);
    int column = 0;
    for (const std::string& name : GatherLayerNames(scene)) {
        bool on = std::find(wanted.begin(), wanted.end(), name) != wanted.end();
        if (column++ % 3 != 0) {
            ImGui::SameLine();
        }
        // The hidden id after ## keeps two checkboxes with the same visible
        // label apart. See the note about widget identity in Panel.h.
        char id[96];
        std::snprintf(id, sizeof(id), "%s##collideswith", name.c_str());
        if (ImGui::Checkbox(id, &on)) {
            if (on) {
                wanted.push_back(name);
            } else {
                std::erase(wanted, name);
            }
            changed = true;
        }
    }
    ImGui::EndDisabled();

    if (changed) {
        collider.SetCollidesWith(std::move(wanted));
        EditorState::Get().dirty = true;
    }

    ImGui::TextDisabled("Two colliders are only tested when EACH one's list "
                        "includes the other's layer.");
}

// ---------------------------------------------------------------------------
//  SpinComponent
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//  CameraComponent
// ---------------------------------------------------------------------------
void DrawCamera(eng::CameraComponent& camera) {
    // WHICH CAMERA IS ACTUALLY LIVE, said first and plainly. With more than
    // one camera in a scene, "why is the Game view not where I put this?" is
    // the question this panel exists to answer.
    if (camera.IsActive()) {
        ImGui::TextColored(ImVec4(0.55f, 0.90f, 0.60f, 1.0f),
                           "This camera is driving the Game view.");
    } else if (!camera.Enabled()) {
        ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.78f, 1.0f),
                           "Disabled - it is ignored entirely.");
    } else if (const eng::CameraComponent* active = eng::CameraSystem::Active()) {
        const eng::Entity* owner = active->Owner();
        ImGui::TextColored(ImVec4(0.95f, 0.78f, 0.30f, 1.0f),
                           "NOT in use - '%s' has a higher priority (%d vs %d).",
                           owner != nullptr ? owner->Name().c_str() : "another camera",
                           active->Priority(), camera.Priority());
    }

    float zoom = camera.Zoom();
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::DragFloat("Zoom", &zoom, 0.01f, 0.01f, 50.0f, "%.2fx")) {
        camera.SetZoom(zoom);
        EditorState::Get().dirty = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Bigger is closer in. This is the game's zoom - the Scene "
                          "view has its own and is not affected.");
    }

    int priority = camera.Priority();
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::DragInt("Priority", &priority, 0.2f)) {
        camera.SetPriority(priority);
        EditorState::Get().dirty = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("With several cameras in a scene, the highest priority is "
                          "the one used. Ties go to whichever was found first.");
    }

    bool enabled = camera.Enabled();
    if (ImGui::Checkbox("Enabled", &enabled)) {
        camera.SetEnabled(enabled);
        EditorState::Get().dirty = true;
    }

    ImGui::Separator();

    // THE POSITION IS THE TRANSFORM'S, and saying so stops somebody looking
    // for a position field that deliberately does not exist.
    ImGui::TextDisabled("Its position is the Transform above - there is no separate");
    ImGui::TextDisabled("camera position. Parent it to something to make it follow.");

    if (eng::CameraSystem::Count() > 1) {
        ImGui::TextDisabled("%zu cameras in this scene.", eng::CameraSystem::Count());
    }
}

void DrawSpin(eng::SpinComponent& spin) {
    // Edited in degrees, stored in radians - same reason as the transform's
    // rotation field above.
    float degrees = spin.RadiansPerSecond() * eng::kRadToDeg;
    if (ImGui::DragFloat("Degrees/sec", &degrees, 1.0f, -720.0f, 720.0f, "%.1f")) {
        spin.SetRadiansPerSecond(degrees * eng::kDegToRad);
        EditorState::Get().dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("0")) {
        spin.SetRadiansPerSecond(0.0f);
        EditorState::Get().dirty = true;
    }

    ImGui::TextDisabled("negative numbers spin the other way");
    ImGui::TextWrapped("Anything parented to this entity ORBITS it, because a child's "
                       "position in the world is worked out through its parent. There "
                       "is no orbit code anywhere - drag this and watch.");
}

// ---------------------------------------------------------------------------
//  ScriptComponent
// ---------------------------------------------------------------------------
// The script's name, drawn as a link that opens the file it came from in
// whatever this machine uses for .cpp files - the same thing double-clicking
// it in the Assets panel does. See ShellOpen.h.
//
// WHY A LOOKUP IS NEEDED AT ALL: a ScriptComponent stores a name, not a path,
// so the file has to be found again. That walks the assets tree, which is why
// it happens on the CLICK rather than every frame while the link is merely on
// screen - see ScriptPathFromName.
//
// The consequence is that the link is drawn without knowing whether there is
// a file behind it. Reporting a miss when somebody asks is honest and costs
// nothing; scanning the disk sixty times a second to grey out a label would
// cost a great deal.
void DrawScriptNameLink(const eng::ScriptComponent& script, ImVec4 colour) {
    ImGui::PushStyleColor(ImGuiCol_TextLink, colour);
    const bool clicked = ImGui::TextLink(script.ScriptName().c_str());
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Open %s in the program this machine uses for that kind "
                          "of file", script.ScriptName().c_str());
    }
    if (!clicked) {
        return;
    }

    const std::string path = ScriptPathFromName(script.ScriptName());
    if (path.empty()) {
        // AT ERROR, not Warning: the project's log threshold discards
        // warnings, and a link that does nothing and says nothing is the
        // exact failure this feature exists to avoid. Same reasoning as
        // ShellOpen.h.
        ENGINE_LOG_ERROR(eng::Channels::kEditor,
                         "no file for the script '{}' was found under assets/ - a "
                         "script is opened by looking for a file named after it",
                         script.ScriptName());
        return;
    }

    std::string error;
    if (OpenExternally(path, error)) {
        ENGINE_LOG_INFO(eng::Channels::kEditor, "opened {}", path);
    } else {
        ENGINE_LOG_ERROR(eng::Channels::kEditor, "could not open '{}': {}", path, error);
    }
}

void DrawScript(eng::ScriptComponent& script) {
    if (script.IsResolved()) {
        DrawScriptNameLink(script, ImVec4(0.55f, 0.85f, 0.60f, 1.0f));

        // WHICH HOOKS THIS SCRIPT ACTUALLY HAS.
        //
        // Hooks are found by name while the script compiles, so a misspelled
        // OnUpdate is not an error - it is a function nobody calls. Showing
        // the list here turns "why is nothing happening?" into something you
        // can answer by looking at the entity you are already looking at.
        const std::string hooks = eng::DescribeHooks(script.Hooks());
        ImGui::TextDisabled("compiled in and running");
        ImGui::TextWrapped("Hooks: %s", hooks.c_str());
    } else {
        // RED AND LOUD. A script that silently does nothing because it was
        // never compiled is the worst failure this feature can have, so it is
        // the most prominent thing in the panel.
        //
        // STILL A LINK, and if anything more useful as one: "this is not
        // running" is the moment you most want to open the file and find out
        // why. Not being compiled in does not mean the file is missing.
        DrawScriptNameLink(script, ImVec4(0.95f, 0.45f, 0.40f, 1.0f));
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.40f, 1.0f), " -  NOT FOUND");
        ImGui::TextWrapped("No script with that name is compiled into this build, so it "
                           "is attached and will be saved, but it does not run. Rebuild "
                           "the project and start the editor again - the connection is "
                           "by name, so nothing needs reattaching.");
    }

    // A list of what IS available, so a typo is one click from fixed rather
    // than a hunt through the scripts folder.
    if (ImGui::BeginCombo("Bind to", script.ScriptName().c_str())) {
        if (eng::ScriptRegistry::Count() == 0) {
            ImGui::TextDisabled("no scripts are compiled into this build");
        }
        eng::ScriptRegistry::ForEachScript([&](const char* name) {
            if (ImGui::Selectable(name, script.ScriptName() == name)) {
                script.SetScriptName(name);
                EditorState::Get().dirty = true;
            }
        });
        ImGui::EndCombo();
    }

    ImGui::TextDisabled("or drag a .cpp from the Assets panel onto this window");
}

} // namespace

void InspectorPanel::ApplyPendingRemoval() {
    if (!m_pendingRemoval.requested) {
        return;
    }

    // Taken and CLEARED FIRST, so an early return below cannot leave a
    // request that tries again on every frame for the rest of the session.
    const PendingRemoval request = m_pendingRemoval;
    m_pendingRemoval             = {};

    eng::Entity* entity = eng::Engine::Get().GetScene().Get(request.entity);
    if (entity == nullptr) {
        return;   // destroyed between the click and here; the id did its job
    }

    // CHECKED AGAIN, not trusted. The index was true when the x was clicked,
    // and anything that ran in between could have changed the list - so the
    // component at that position is confirmed to still be the kind that may
    // be removed before anything is erased.
    eng::Component* component = entity->ComponentAt(request.index);
    if (component == nullptr) {
        return;
    }
    const std::string typeName = component->TypeName();
    if (typeName == eng::TransformComponent::kTypeName) {
        ENGINE_LOG_ERROR(eng::Channels::kEditor,
                         "refused to remove the transform from '{}' - every entity "
                         "has one", entity->Name());
        return;
    }

    if (entity->RemoveComponentAt(request.index)) {
        EditorState::Get().dirty = true;
        ENGINE_LOG_INFO(eng::Channels::kEditor, "removed {} from '{}'", typeName,
                        entity->Name());
    }
}

void InspectorPanel::Draw() {
    // FIRST, before anything reads the component list - including the early
    // return below for "nothing is selected". Carrying it out at the end of
    // Draw instead would look the same almost always, and then quietly strand
    // the request on the one path that returns early: click the x, click away
    // before the next frame, and the removal would sit there unapplied.
    //
    // The cost is that the component stays on screen for the remainder of the
    // frame its x was clicked on, which at sixty frames a second nobody sees.
    ApplyPendingRemoval();

    EditorState& state = EditorState::Get();
    eng::Scene&  scene = eng::Engine::Get().GetScene();

    // Looked up from the ID every frame. A selection that has been destroyed
    // comes back as nothing and is reported, rather than crashing - which is
    // the entire reason the selection is an id and not a pointer.
    eng::Entity* entity = scene.Get(state.selected);
    if (entity == nullptr) {
        if (!state.selected.IsNull()) {
            ImGui::TextColored(ImVec4(0.95f, 0.78f, 0.30f, 1.0f),
                               "the selected entity no longer exists");
            if (ImGui::Button("Clear selection")) {
                state.ClearSelection();
            }
        } else {
            ImGui::TextDisabled("select an entity in the Hierarchy");
        }
        return;
    }

    // ---- the WHOLE PANEL is a drop target --------------------------------
    //
    // Not a strip or a header - the entire window, so an image or a script can
    // be dropped anywhere on it. Done before the fields are drawn so that a
    // widget directly under the cursor still wins.
    if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->Rect(),
                                         ImGui::GetID("##inspector_drop"))) {
        std::string message;
        for (const char* payloadId : {kPayloadTexture, kPayloadScript}) {
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadId);
            if (payload == nullptr) {
                continue;
            }
            const auto* path = static_cast<const char*>(payload->Data);
            if (ApplyAssetToEntity(state.selected, path, message)) {
                ENGINE_LOG_INFO(eng::Channels::kEditor, "{}", message);
            } else {
                ENGINE_LOG_WARN(eng::Channels::kEditor, "{}", message);
            }
        }
        ImGui::EndDragDropTarget();
    }

    const bool running = !eng::Engine::Get().Clock().IsPaused();

    // ---- when several things are selected ---------------------------------
    //
    // The components below belong to the ACTIVE entity only, and saying so
    // matters: editing a field here with eight entities selected changes one
    // of them, and somebody who thought otherwise would not find out until
    // much later. The alternative - editing every selected entity at once -
    // is a real feature with real questions behind it (what happens when
    // they disagree?) and is not something to do by accident.
    if (state.selection.size() > 1) {
        ImGui::TextColored(ImVec4(0.55f, 0.78f, 0.95f, 1.0f), "%zu entities selected",
                           state.selection.size());

        if (ImGui::Button("Destroy all")) {
            for (const eng::EntityId id : state.selection) {
                eng::DeferredOps::QueueDestroy(id);
            }
            state.ClearSelection();
            state.dirty = true;
        }

        if (ImGui::TreeNode("Show the list")) {
            for (const eng::EntityId id : state.selection) {
                if (const eng::Entity* member = scene.Get(id)) {
                    // Clicking one makes it the active entity, so the list
                    // doubles as a way of stepping through the selection
                    // without losing it.
                    if (ImGui::Selectable(member->Name().c_str(), id == state.selected)) {
                        state.selected = id;
                    }
                }
            }
            ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::TextDisabled("below: the components of the active entity only");
    }

    ImGui::Text("%s", entity->Name().c_str());

    ImGui::SameLine(ImGui::GetWindowWidth() - 90.0f);
    if (ImGui::Button("Destroy")) {
        // Answer 3: through the deferred queue, like everything else.
        eng::DeferredOps::QueueDestroy(entity->Id());
    }

    ImGui::Separator();

    // WALKED BY INDEX rather than with ForEachComponent, because removing
    // needs to say WHICH component - not which type. An entity can hold
    // several scripts, and "remove the ScriptComponent" would always take the
    // first one however far down the list the x was clicked. See
    // Entity::RemoveComponentAt.
    //
    // Re-reading ComponentCount each time round is what makes it safe for the
    // body to add a component, which is the same reason ForEachComponent is
    // written this way. Removal is deferred and so does not come into it.
    for (std::size_t i = 0; i < entity->ComponentCount(); ++i) {
        eng::Component* componentPtr = entity->ComponentAt(i);
        if (componentPtr == nullptr) {
            continue;
        }
        eng::Component& component = *componentPtr;

        // PushID makes every widget inside this block unique to this
        // component, so two components with a field called "Offset" do not end
        // up sharing one widget.
        //
        // Keyed on the ADDRESS rather than the index, so that a header you
        // opened stays open when something above it is removed and everything
        // below shifts up a place.
        ImGui::PushID(&component);

        const std::string_view typeName = component.TypeName();

        // THE TRANSFORM IS THE ONE THAT STAYS. Every entity has one by
        // definition; see the note in InspectorPanel.h.
        const bool removable = typeName != eng::TransformComponent::kTypeName;

        // A SCRIPT IS NAMED AFTER THE SCRIPT, not after the class that holds
        // it. "ScriptComponent" is an implementation detail of the engine;
        // what you attached was PlayerMovement, and that is what you are
        // looking for when you scan the panel.
        //
        // It also makes several scripts on one entity tellable apart. Three
        // headers all reading "ScriptComponent" mean opening each one to find
        // out which is which - and now that a header carries a remove button,
        // guessing is expensive.
        //
        // The "(Script)" suffix is how Unity writes the same thing, and it
        // earns its place: without it a bare "PlayerMovement" sitting between
        // "SpriteComponent" and "AABBColliderComponent" gives no clue what
        // kind of thing it is.
        //
        // EVERYTHING AFTER ### IS THE IDENTITY, and the part before it is
        // only what you read. ImGui normally derives a widget's identity from
        // its label, which would mean rebinding a script - renaming this
        // header - counted as a different header, and the one you had open
        // would spring back to its default. The visible text is now free to
        // change without the panel rearranging itself underneath you. See the
        // note about widget identity in Panel.h.
        char header[128];
        if (typeName == eng::ScriptComponent::kTypeName) {
            const std::string& scriptName =
                static_cast<eng::ScriptComponent&>(component).ScriptName();
            if (scriptName.empty()) {
                std::snprintf(header, sizeof(header),
                              "Script (nothing bound yet)###component");
            } else {
                std::snprintf(header, sizeof(header), "%s (Script)###component",
                              scriptName.c_str());
            }
        } else {
            std::snprintf(header, sizeof(header), "%s###component",
                          component.TypeName());
        }

        // CollapsingHeader draws its own x when given somewhere to report it,
        // which is the ordinary way a panel like this offers "remove". `keep`
        // comes back false on the frame the x is clicked.
        bool       keep = true;
        const bool open = removable
                              ? ImGui::CollapsingHeader(header, &keep,
                                                        ImGuiTreeNodeFlags_DefaultOpen)
                              : ImGui::CollapsingHeader(header,
                                                        ImGuiTreeNodeFlags_DefaultOpen);

        // The same thing by right-click, because an x is easy to miss and
        // impossible to find by looking for the word "remove". Attached to the
        // header, which is the item just drawn.
        if (ImGui::BeginPopupContextItem("component-actions")) {
            if (ImGui::MenuItem("Remove Component", nullptr, false, removable)) {
                keep = false;
            }
            if (!removable) {
                ImGui::TextDisabled("Every entity has a transform.");
            }
            ImGui::EndPopup();
        }

        if (!removable && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Every entity has a transform, so this one cannot "
                              "be removed.");
        }

        if (!keep) {
            // RECORDED, NOT DONE. This loop is walking the component list;
            // erasing from it here shifts every later component down a place
            // and the loop would step straight over one. Carried out by
            // ApplyPendingRemoval at the end of Draw.
            m_pendingRemoval.entity    = entity->Id();
            m_pendingRemoval.index     = i;
            m_pendingRemoval.requested = true;
        }

        if (open) {
            // Work out which kind of component this is and call the matching
            // draw function. Comparing type NAMES is the same way the scene
            // file identifies components.
            const std::string_view type = typeName;

            if (type == eng::TransformComponent::kTypeName) {
                DrawTransform(static_cast<eng::TransformComponent&>(component), running);
            } else if (type == eng::SpriteComponent::kTypeName) {
                DrawSprite(static_cast<eng::SpriteComponent&>(component));
            } else if (type == eng::AABBColliderComponent::kTypeName ||
                       type == eng::CircleColliderComponent::kTypeName) {
                DrawCollider(static_cast<eng::ColliderComponent&>(component), scene);
            } else if (type == eng::SpinComponent::kTypeName) {
                DrawSpin(static_cast<eng::SpinComponent&>(component));
            } else if (type == eng::ScriptComponent::kTypeName) {
                DrawScript(static_cast<eng::ScriptComponent&>(component));
            } else if (type == eng::CameraComponent::kTypeName) {
                DrawCamera(static_cast<eng::CameraComponent&>(component));
            } else {
                // Not an error. Somebody may write their own component type,
                // and the Inspector should say so rather than pretend it is
                // not there.
                ImGui::TextDisabled("no editor written for this component type yet");
            }
        }
        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::SeparatorText("Add component");

    // The list of buttons is generated from the factory, so a component type
    // registered anywhere appears here with no edit to this file.
    eng::ComponentFactory::ForEachType([&](const char* typeName) {
        if (entity->FindComponent(typeName) != nullptr) {
            return;   // it already has one
        }
        char label[96];
        std::snprintf(label, sizeof(label), "+ %s", typeName);
        if (ImGui::SmallButton(label)) {
            entity->AddComponent(typeName);
            EditorState::Get().dirty = true;
        }
    });
}

} // namespace editor
