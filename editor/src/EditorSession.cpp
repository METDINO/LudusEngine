// ============================================================================
//  EditorSession.cpp - saving and restoring where you were. See the header.
// ============================================================================

#include "EditorSession.h"

#include "EditorApp.h"
#include "panels/ScenePanel.h"

#include <engine/Engine.h>
#include <engine/core/Json.h>
#include <engine/core/Log.h>
#include <engine/fs/FileSystem.h>

#include <string>

namespace editor {
namespace {

// Reads a [x, y] pair, leaving the output alone when the value is not one.
// Anything in this file may have been hand-edited or written by an older
// version, so nothing in it is trusted to be the shape it should be.
void ReadVec2(const eng::Json& node, const char* key, eng::Vec2& out) {
    const auto found = node.find(key);
    if (found == node.end() || !found->is_array() || found->size() != 2) {
        return;
    }
    const eng::Json& x = (*found)[0];
    const eng::Json& y = (*found)[1];
    if (x.is_number() && y.is_number()) {
        out = eng::Vec2{x.get<float>(), y.get<float>()};
    }
}

} // namespace

void EditorSession::Restore(ScenePanel* scenePanel) {
    if (!eng::FileSystem::Exists(kVirtualPath)) {
        return;   // first run on this machine
    }

    std::string text;
    std::string error;
    if (!eng::FileSystem::ReadTextFile(kVirtualPath, text, error)) {
        ENGINE_LOG_WARN(eng::Channels::kEditor, "could not read the last session: {}",
                        error);
        return;
    }

    const eng::Json document = eng::ParseJson(text, error);
    if (!error.empty()) {
        // Not worth stopping over, and not worth a red line either: the
        // remedy is automatic, because closing the editor will overwrite it
        // with something valid.
        ENGINE_LOG_WARN(eng::Channels::kEditor,
                        "'{}' is not valid JSON ({}), so the editor opened fresh",
                        kVirtualPath, error);
        return;
    }

    // ---- the scene --------------------------------------------------------
    //
    // ONLY WHEN IT IS NOT ALREADY OPEN. Engine::Init has already loaded the
    // build list's first scene by the time this runs, and in the common case -
    // you were working on that scene - reloading it would throw away a perfectly
    // good scene and build the same one again.
    if (const auto scene = document.find("scene");
        scene != document.end() && scene->is_string()) {
        const std::string wanted = scene->get<std::string>();
        const std::string open   = eng::Engine::Get().GetScene().SourcePath();

        if (!wanted.empty() && wanted != open) {
            if (!eng::FileSystem::Exists(wanted)) {
                // Said plainly, because "the scene I was working on is gone"
                // is something the person needs to know rather than silently
                // being put somewhere else.
                ENGINE_LOG_WARN(eng::Channels::kEditor,
                                "the scene from last session, '{}', no longer exists; "
                                "opening '{}' instead",
                                wanted, open.empty() ? "nothing" : open);
            } else {
                std::string loadError;
                if (eng::Engine::Get().LoadScene(wanted, loadError)) {
                    ENGINE_LOG_INFO(eng::Channels::kEditor,
                                    "resumed last session's scene '{}'", wanted);
                } else {
                    ENGINE_LOG_WARN(eng::Channels::kEditor,
                                    "could not reopen '{}': {}", wanted, loadError);
                }
            }
        }
    }

    // ---- the selection ----------------------------------------------------
    //
    // By NAME, looked up in whatever scene is now loaded. See the header for
    // why an EntityId would be the wrong thing to have written down.
    if (const auto selected = document.find("selected");
        selected != document.end() && selected->is_string()) {
        const std::string name = selected->get<std::string>();
        if (!name.empty()) {
            const eng::EntityId found = eng::Engine::Get().GetScene().Find(name);
            if (!found.IsNull()) {
                EditorState::Get().SelectOnly(found);
            }
        }
    }

    // ---- where the Scene view was looking ---------------------------------
    if (scenePanel != nullptr) {
        const auto view = document.find("sceneView");
        if (view != document.end() && view->is_object()) {
            eng::Vec2 position = scenePanel->Camera().Position();
            ReadVec2(*view, "position", position);
            scenePanel->Camera().SetPosition(position);

            if (const auto zoom = view->find("zoom");
                zoom != view->end() && zoom->is_number()) {
                const float value = zoom->get<float>();
                // Guarded because a zoom of zero or a negative one would make
                // the view undrawable, and SetZoom is not obliged to defend
                // itself against a hand-edited file.
                if (value > 0.0f) {
                    scenePanel->Camera().SetZoom(value);
                }
            }
        }
    }
}

void EditorSession::Save(ScenePanel* scenePanel) {
    eng::Json document = eng::Json::object();
    document["_comment"] =
        "Where the editor was when it last closed, so it can open there again. "
        "Written automatically; delete it to start fresh. Panel layout is not here - "
        "ImGui keeps that in imgui.ini. See editor/src/EditorSession.h.";

    document["scene"] = eng::Engine::Get().GetScene().SourcePath();

    // The selected entity's NAME, or "" when nothing is selected. Looked up
    // now rather than remembered, because the id may have gone stale while the
    // editor was running.
    std::string selectedName;
    if (const eng::Entity* entity =
            eng::Engine::Get().GetScene().Get(EditorState::Get().selected)) {
        selectedName = entity->Name();
    }
    document["selected"] = selectedName;

    if (scenePanel != nullptr) {
        const eng::Vec2 position = scenePanel->Camera().Position();
        eng::Json       view     = eng::Json::object();
        view["position"]         = {position.x, position.y};
        view["zoom"]             = scenePanel->Camera().Zoom();
        document["sceneView"]    = view;
    }

    std::string error;
    if (!eng::FileSystem::WriteTextFile(kVirtualPath, document.dump(2), error)) {
        // A warning, not an error. Failing to remember where you were is a
        // small loss, and the editor is on its way out - there is nothing left
        // for it to do about it.
        ENGINE_LOG_WARN(eng::Channels::kEditor, "could not save the session: {}", error);
        return;
    }

    ENGINE_LOG_INFO(eng::Channels::kEditor, "session saved: scene '{}'",
                    document["scene"].get<std::string>());
}

} // namespace editor
