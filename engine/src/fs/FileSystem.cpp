// =============================================================================
//  FileSystem.cpp - a skeleton. Every function is here with the right signature
//  and an empty body. FileSystem.h is the specification; read it first.
// =============================================================================

#include <engine/fs/FileSystem.h>
#include <engine/core/Log.h>
#include <SDL3/SDL.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>


namespace eng {
namespace {

namespace fs = std::filesystem;

std::string g_root;

bool LooksLikeRoot(const fs::path& directory) {
    std::error_code ec;
    return fs::is_directory(directory / "assets", ec);
}

} // namespace

// Works out where the project is by starting at the program's own location and
// walking up until it finds a folder containing assets/. Where it settled is
// written to the log, because that line is the first thing to check when a file
// will not load on somebody else's machine.
bool FileSystem::Init(const BootConfig&) {

    if (const char* overridePath = std::getenv("ENGINE_ASSET_ROOT");
        overridePath != nullptr && overridePath[0] != '\0') {
        std::error_code ec;
        const fs::path candidate = fs::absolute(fs::path(overridePath), ec);
        if (LooksLikeRoot(candidate)) {
            g_root = candidate.string();
            ENGINE_LOG_INFO(Channels::kFileSys, "asset folder taken from ENGINE ASSET_ROOT: {}", g_root);
            return true;
        }
        ENGINE_LOG_WARN(Channels::kFileSys, "ENGINE_ASSET_ROOT is set to '{}' but there is no 'assets' folder there; searching instead", overridePath);
    }

    fs::path start;

    if (const char* base = SDL_GetBasePath(); base != nullptr && base[0] != '\0') {
        start = fs::path(base);
    } else {
        std::error_code ec;
        start = fs::current_path();
        ENGINE_LOG_WARN(Channels::kFileSys, "could not find the program's own folder; falling back to the current directory, which is not reliable:");

    }

    std::error_code ec;
    fs::path current = fs::absolute(start, ec);

    for (int depth = 0; depth < 12; depth++)
        ;

    return false;
}

// Forgets the project location.
void FileSystem::Shutdown() {
}

// The folder that was found - the one containing assets/.
const std::string& FileSystem::AssetRoot() {
    static const std::string root;
    return root;
}

// Turns a short name like "textures/player.bmp" into a real path on this
// machine. Never fails; whether the file exists is a separate question.
std::string FileSystem::Resolve(std::string_view /*virtualPath*/) {
    return {};
}

// Is there actually a file there?
bool FileSystem::Exists(std::string_view /*virtualPath*/) {
    return false;
}

// Lists the files in one folder, optionally filtered by extension, giving back
// short names that can be handed straight back to ReadTextFile or Scene::Load.
bool FileSystem::ListFiles(std::string_view /*virtualDirectory*/,
                           std::string_view /*extension*/,
                           std::vector<std::string>& /*out*/) {
    return false;
}

// The listing a file BROWSER needs rather than a menu: sub-folders included,
// folders first, each group sorted by name.
bool FileSystem::ListDirectory(std::string_view /*virtualDirectory*/,
                               std::vector<DirEntry>& /*out*/) {
    return false;
}

// Creates a folder, including any missing parent folders.
bool FileSystem::CreateDirectory(std::string_view /*virtualDirectory*/,
                                 std::string& /*outError*/) {
    return false;
}

// Reads a whole text file - a scene, the settings - into a string.
bool FileSystem::ReadTextFile(std::string_view /*virtualPath*/, std::string& /*outText*/,
                              std::string& /*outError*/) {
    return false;
}

// Reads a whole binary file - an image - into a list of bytes.
bool FileSystem::ReadFile(std::string_view /*virtualPath*/,
                          std::vector<unsigned char>& /*outBytes*/,
                          std::string& /*outError*/) {
    return false;
}

// Writes a text file, creating any folders it needs on the way.
bool FileSystem::WriteTextFile(std::string_view /*virtualPath*/, std::string_view /*text*/,
                               std::string& /*outError*/) {
    return false;
}

} // namespace eng
