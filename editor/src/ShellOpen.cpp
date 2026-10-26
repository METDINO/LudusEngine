// ============================================================================
//  ShellOpen.cpp - handing a file to the operating system. See ShellOpen.h
//  for why this is more than one line and why it used to fail without a word.
// ============================================================================

#include "ShellOpen.h"

#include <engine/fs/FileSystem.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace editor {
namespace {

namespace fs = std::filesystem;

// The extension, lower-cased, including the dot. Only used to make the error
// message say which kind of file had no program ("nothing opens .oscene
// files" is actionable; "could not open" is not).
std::string ExtensionOf(std::string_view virtualPath) {
    std::string ext = fs::path(std::string(virtualPath)).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) {
        // The unsigned char cast is not optional: tolower takes an int that
        // must be representable as unsigned char, and a plain char is signed
        // here - so a byte above 127 would be a negative int and the
        // behaviour would be undefined.
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });
    return ext;
}

// Hands a real path to the system, reporting WHY if it would not take it.
bool Shell(const std::string& realPath, std::string& outError) {
    if (SDL_OpenURL(realPath.c_str())) {
        outError.clear();
        return true;
    }

    const char* reason = SDL_GetError();
    outError = (reason != nullptr && reason[0] != '\0')
                   ? reason
                   : "the operating system did not say why";
    return false;
}

} // namespace

bool OpenExternally(std::string_view virtualPath, std::string& outError) {
    const std::string real = eng::FileSystem::Resolve(virtualPath);

    std::error_code ec;
    if (!fs::exists(fs::path(real), ec)) {
        // Checked first so that "there is no such file" and "there is no
        // program for this kind of file" are not reported as the same thing.
        // They need completely different responses from whoever reads it.
        outError = "there is no file at " + real;
        return false;
    }

    if (Shell(real, outError)) {
        return true;
    }

    // The likeliest reason by far, spelled out. The raw message from the
    // operating system is "Couldn't open given URL", which explains nothing
    // to somebody who just double-clicked a script.
    const std::string extension = ExtensionOf(virtualPath);
    outError += " (most likely nothing on this machine is set up to open " +
                (extension.empty() ? std::string("files of this kind") : extension) +
                " files - try Show in File Explorer)";
    return false;
}

bool RevealInFolder(std::string_view virtualPath, std::string& outError) {
    const std::string real = eng::FileSystem::Resolve(virtualPath);

    std::error_code ec;
    fs::path        target = fs::path(real);

    // A folder opens itself; a file opens the folder it is in. Asking for the
    // parent of a folder would open the one ABOVE it, which is not what
    // "show me where this is" means.
    if (!fs::is_directory(target, ec)) {
        target = target.parent_path();
    }

    if (!fs::exists(target, ec)) {
        outError = "there is no folder at " + target.string();
        return false;
    }
    return Shell(target.string(), outError);
}

} // namespace editor
