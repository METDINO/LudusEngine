// ============================================================================
//  BuildScenes.cpp - the build's scene list. See BuildScenes.h.
// ============================================================================

#include <engine/scene/BuildScenes.h>

#include <engine/core/Json.h>
#include <engine/core/Log.h>
#include <engine/fs/FileSystem.h>

#include <algorithm>

namespace eng {
namespace {

std::vector<std::string> g_scenes;

// Returned by PathAt for an index nobody should have asked for. A function
// returning a reference needs something to refer to, and a static empty string
// is the honest answer - see the note in the header about why this is not an
// exception.
const std::string g_empty;

} // namespace

bool BuildScenes::Load(std::string& outError) {
    g_scenes.clear();
    outError.clear();

    // A MISSING LIST IS A PROJECT THAT HAS NOT USED THE BUILD PANEL YET, which
    // is not a problem and must not be reported as one. Engine::Init falls
    // back to config/engine.json when the list comes back empty.
    if (!FileSystem::Exists(kVirtualPath)) {
        return true;
    }

    std::string text;
    std::string readError;
    if (!FileSystem::ReadTextFile(kVirtualPath, text, readError)) {
        outError = readError;
        ENGINE_LOG_WARN(Channels::kScene, "could not read '{}': {}", kVirtualPath,
                        readError);
        return false;
    }

    std::string parseError;
    const Json  document = ParseJson(text, parseError);
    if (!parseError.empty()) {
        outError = parseError;
        ENGINE_LOG_ERROR(Channels::kScene,
                         "'{}' is not valid JSON ({}), so no scene list is available",
                         kVirtualPath, parseError);
        return false;
    }

    const auto scenes = document.find("scenes");
    if (scenes == document.end() || !scenes->is_array()) {
        outError = "there is no \"scenes\" array in the file";
        ENGINE_LOG_ERROR(Channels::kScene, "'{}' has no \"scenes\" array", kVirtualPath);
        return false;
    }

    for (const Json& entry : *scenes) {
        if (!entry.is_string()) {
            ENGINE_LOG_WARN(Channels::kScene,
                            "an entry in '{}' is not text and was skipped", kVirtualPath);
            continue;
        }
        std::string path = entry.get<std::string>();
        if (path.empty()) {
            continue;
        }

        // A SCENE LISTED TWICE WOULD HAVE TWO INDICES, and then "load scene 3"
        // depends on which copy somebody counted. Rejected here rather than
        // trusted, because the file can be edited by hand.
        if (std::find(g_scenes.begin(), g_scenes.end(), path) != g_scenes.end()) {
            ENGINE_LOG_WARN(Channels::kScene,
                            "'{}' appears more than once in the build list; the later "
                            "one was ignored",
                            path);
            continue;
        }
        g_scenes.push_back(std::move(path));
    }

    ENGINE_LOG_INFO(Channels::kScene, "build list: {} scene(s), starting with '{}'",
                    g_scenes.size(), g_scenes.empty() ? "<none>" : g_scenes.front());
    for (std::size_t i = 0; i < g_scenes.size(); ++i) {
        ENGINE_LOG_INFO(Channels::kScene, "  [{}] {}", i, g_scenes[i]);
    }

    return true;
}

void BuildScenes::Clear() {
    g_scenes.clear();
}

std::size_t BuildScenes::Count() {
    return g_scenes.size();
}

const std::string& BuildScenes::PathAt(std::size_t index) {
    return index < g_scenes.size() ? g_scenes[index] : g_empty;
}

int BuildScenes::IndexOf(std::string_view virtualPath) {
    for (std::size_t i = 0; i < g_scenes.size(); ++i) {
        if (g_scenes[i] == virtualPath) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

const std::vector<std::string>& BuildScenes::All() {
    return g_scenes;
}

bool BuildScenes::Save(const std::vector<std::string>& scenes, std::string& outError) {
    Json document      = Json::object();
    document["_comment"] =
        "Which scenes are in the build, and in what order. Index 0 is the scene the "
        "game opens with. Scenes not listed here are not packed into an exported game. "
        "The editor's Build panel writes this file; see "
        "engine/include/engine/scene/BuildScenes.h.";
    document["scenes"] = scenes;

    if (!FileSystem::WriteTextFile(kVirtualPath, document.dump(2), outError)) {
        ENGINE_LOG_ERROR(Channels::kScene, "could not save the build list: {}", outError);
        return false;
    }

    g_scenes = scenes;
    ENGINE_LOG_INFO(Channels::kScene, "build list saved: {} scene(s)", g_scenes.size());
    return true;
}

} // namespace eng
