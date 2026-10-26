// ============================================================================
//  ObjectMenu.cpp - the one list of things you can add. See ObjectMenu.h.
// ============================================================================

#include "ObjectMenu.h"

#include "EditorApp.h"

#include <engine/Engine.h>
#include <engine/core/Log.h>
#include <engine/scene/CameraComponent.h>
#include <engine/scene/Entity.h>
#include <engine/scene/Scene.h>

#include <imgui.h>

namespace editor {
namespace {

struct ObjectKind {
    const char* name;
    const char* component;   // nullptr for a bare entity
    const char* tip;
};

// THE LIST. Adding a kind of object is one row here and it appears in both
// menus at once.
constexpr ObjectKind kKinds[] = {
    {"Empty", nullptr, "Just a transform. Add components to it yourself."},

    {"Camera", eng::CameraComponent::kTypeName,
     "The view the game is drawn through. It is an entity like any other, so "
     "you can parent it, script it, and see where it is."},

    {"Sprite", eng::SpriteComponent::kTypeName,
     "An entity with a SpriteComponent, ready for an image to be dropped on "
     "it."},
};

} // namespace

eng::EntityId CreateObject(std::string_view kind) {
    eng::Scene& scene = eng::Engine::Get().GetScene();

    const ObjectKind* chosen = nullptr;
    for (const ObjectKind& candidate : kKinds) {
        if (kind == candidate.name) {
            chosen = &candidate;
            break;
        }
    }
    if (chosen == nullptr) {
        ENGINE_LOG_WARN(eng::Channels::kEditor, "there is no object kind called '{}'",
                        kind);
        return {};
    }

    const eng::EntityId created = scene.CreateEntity(scene.MakeUniqueName(chosen->name));
    eng::Entity*        entity  = scene.Get(created);
    if (entity == nullptr) {
        return {};
    }

    // Placed where the Scene view is looking, so it appears on screen rather
    // than at the world origin somewhere off in the distance. The same rule
    // the old "+ Create Entity" button used.
    entity->Transform().SetLocalPosition(eng::Engine::Get().GetCamera().Position());

    if (chosen->component != nullptr && entity->AddComponent(chosen->component) == nullptr) {
        ENGINE_LOG_ERROR(eng::Channels::kEditor,
                         "could not add a {} to the new {}", chosen->component,
                         chosen->name);
    }

    return created;
}

eng::EntityId DrawObjectMenuItems() {
    eng::EntityId created;

    for (const ObjectKind& kind : kKinds) {
        if (ImGui::MenuItem(kind.name)) {
            created = CreateObject(kind.name);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", kind.tip);
        }
    }

    // ---- a note about the camera, where it is actually useful -----------
    //
    // Said here rather than only in the docs, because "why is my Game view
    // not moving" is a question somebody asks while looking at this menu.
    ImGui::Separator();

    if (eng::CameraSystem::Count() == 0) {
        ImGui::TextDisabled("no camera in this scene - the view uses the");
        ImGui::TextDisabled("scene's own saved camera position");
    } else if (const eng::CameraComponent* active = eng::CameraSystem::Active()) {
        const eng::Entity* owner = active->Owner();
        ImGui::TextDisabled("view driven by: %s",
                            owner != nullptr ? owner->Name().c_str() : "a camera");
        if (eng::CameraSystem::Count() > 1) {
            ImGui::TextDisabled("(%zu cameras; highest priority wins)",
                                eng::CameraSystem::Count());
        }
    } else {
        ImGui::TextDisabled("%zu camera(s), all disabled", eng::CameraSystem::Count());
    }

    return created;
}

} // namespace editor
