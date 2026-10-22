// ============================================================================
//  AssetArchive.cpp - packing and unpacking one folder. See AssetArchive.h.
// ============================================================================

#include <engine/fs/AssetArchive.h>

#include <engine/core/Log.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace eng {
namespace {

namespace fs = std::filesystem;

// One file inside an archive.
struct Entry {
    std::uint64_t offset = 0;
    std::uint64_t size   = 0;
};

// One mounted archive: where it is, and what is in it.
//
// THE INDEX IS READ AT MOUNT TIME AND THE PAYLOAD IS NOT. Reading every
// texture into memory on startup would make a game with a large sheet slow to
// start and wasteful for the levels it never reaches; reading the index is a
// few kilobytes and answers every "where is this?" question without touching
// the disk again.
struct Mounted {
    fs::path                     file;
    std::map<std::string, Entry> entries;   // sorted, so listings are stable
};

std::map<std::string, Mounted> g_mounted;   // keyed by folder name

// ---------------------------------------------------------------------------
//  Numbers, written and read a byte at a time. See the note in the header.
// ---------------------------------------------------------------------------

void PutU32(std::ostream& out, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        out.put(static_cast<char>((value >> (i * 8)) & 0xFF));
    }
}

void PutU64(std::ostream& out, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        out.put(static_cast<char>((value >> (i * 8)) & 0xFF));
    }
}

bool GetU32(std::istream& in, std::uint32_t& outValue) {
    outValue = 0;
    for (int i = 0; i < 4; ++i) {
        const int byte = in.get();
        if (byte == std::char_traits<char>::eof()) {
            return false;
        }
        outValue |= static_cast<std::uint32_t>(byte & 0xFF) << (i * 8);
    }
    return true;
}

bool GetU64(std::istream& in, std::uint64_t& outValue) {
    outValue = 0;
    for (int i = 0; i < 8; ++i) {
        const int byte = in.get();
        if (byte == std::char_traits<char>::eof()) {
            return false;
        }
        outValue |= static_cast<std::uint64_t>(byte & 0xFF) << (i * 8);
    }
    return true;
}

// Splits "textures/Blue/x.png" into "textures" and "Blue/x.png".
// Returns false for a path with no folder in it at all.
bool SplitVirtualPath(std::string_view virtualPath, std::string& outFolder,
                      std::string& outEntry) {
    const std::size_t slash = virtualPath.find('/');
    if (slash == std::string_view::npos || slash == 0 ||
        slash + 1 >= virtualPath.size()) {
        return false;
    }
    outFolder.assign(virtualPath.substr(0, slash));
    outEntry.assign(virtualPath.substr(slash + 1));
    return true;
}

// Backslashes to forward slashes, so an archive written on Windows reads
// identically everywhere.
std::string ToVirtualSeparators(std::string text) {
    std::replace(text.begin(), text.end(), '\\', '/');
    return text;
}

// Finds one virtual path in the mounted archives, or returns null.
//
// TWO RULES, AND THE SECOND ONE EXISTS FOR THE SETTINGS FILE.
//
//   1. The usual case: the first path segment names the archive and the rest
//      names the entry, so "textures/Blue/x.png" is "Blue/x.png" inside
//      textures.bin. That is the rule the exporter follows for everything
//      under assets/.
//
//   2. Then, as a fallback, the FULL path is looked for as an entry name in
//      every mounted archive. This is how "config/engine.json" is found inside
//      scenes.bin: the settings choose the starting scene, so they are packed
//      with the scenes rather than left loose beside the game where that
//      choice could be edited. There is no config.bin, and inventing one for a
//      single file would be worse than this one extra lookup.
//
// Rule 1 is tried first so the common case costs one map lookup and never
// walks the archives.
const Entry* FindEntry(std::string_view virtualPath, const Mounted** outArchive) {
    std::string folder;
    std::string entryName;
    if (SplitVirtualPath(virtualPath, folder, entryName)) {
        const auto archive = g_mounted.find(folder);
        if (archive != g_mounted.end()) {
            const auto found = archive->second.entries.find(entryName);
            if (found != archive->second.entries.end()) {
                *outArchive = &archive->second;
                return &found->second;
            }
        }
    }

    const std::string full(virtualPath);
    for (const auto& [name, archive] : g_mounted) {
        (void)name;
        const auto found = archive.entries.find(full);
        if (found != archive.entries.end()) {
            *outArchive = &archive;
            return &found->second;
        }
    }

    *outArchive = nullptr;
    return nullptr;
}

} // namespace

unsigned char AssetArchive::MaskByte(std::uint64_t positionInEntry) {
    // A fixed repeating pattern. Deliberately simple, deliberately documented,
    // and deliberately NOT a secret - see the note at the top of the header.
    // Its whole job is to stop assets/textures.bin opening in an image viewer
    // and to stop a scene's text being found with a search.
    static constexpr unsigned char kPattern[8] = {0x4F, 0x72, 0x69, 0x6F,
                                                  0x6E, 0x32, 0x44, 0x21};
    return kPattern[positionInEntry % sizeof(kPattern)];
}

int AssetArchive::MountAll(const std::string& assetsDirectory) {
    UnmountAll();

    std::error_code ec;
    if (!fs::is_directory(assetsDirectory, ec)) {
        return 0;
    }

    int mounted = 0;
    for (const fs::directory_entry& item : fs::directory_iterator(assetsDirectory, ec)) {
        if (ec) {
            break;
        }
        if (!item.is_regular_file(ec) || item.path().extension() != kExtension) {
            continue;
        }

        std::ifstream file(item.path(), std::ios::binary);
        if (!file) {
            ENGINE_LOG_WARN(Channels::kFileSys, "could not open the archive '{}'",
                            item.path().filename().string());
            continue;
        }

        char magic[4] = {};
        file.read(magic, sizeof(magic));
        if (!file || std::memcmp(magic, kMagic, sizeof(magic)) != 0) {
            // Not ours. Said plainly rather than ignored, because a .bin in
            // assets/ that this cannot read is almost certainly meant to be an
            // archive and the game is about to behave as though it is empty.
            ENGINE_LOG_WARN(Channels::kFileSys,
                            "'{}' is not an engine archive - it does not start with "
                            "OBIN, so it will be ignored",
                            item.path().filename().string());
            continue;
        }

        std::uint32_t version    = 0;
        std::uint32_t entryCount = 0;
        if (!GetU32(file, version) || !GetU32(file, entryCount)) {
            ENGINE_LOG_WARN(Channels::kFileSys, "'{}' ends inside its own header",
                            item.path().filename().string());
            continue;
        }
        if (version != kVersion) {
            ENGINE_LOG_WARN(Channels::kFileSys,
                            "'{}' is version {}, but this engine reads version {}",
                            item.path().filename().string(), version, kVersion);
            continue;
        }

        Mounted archive;
        archive.file = item.path();

        bool indexOk = true;
        for (std::uint32_t i = 0; i < entryCount && indexOk; ++i) {
            std::uint32_t nameLength = 0;
            if (!GetU32(file, nameLength) || nameLength == 0 || nameLength > 4096) {
                indexOk = false;
                break;
            }
            std::string name(nameLength, '\0');
            file.read(name.data(), static_cast<std::streamsize>(nameLength));

            Entry entry;
            if (!file || !GetU64(file, entry.offset) || !GetU64(file, entry.size)) {
                indexOk = false;
                break;
            }
            archive.entries.emplace(std::move(name), entry);
        }

        if (!indexOk) {
            ENGINE_LOG_WARN(Channels::kFileSys,
                            "the index in '{}' is damaged; it will be ignored",
                            item.path().filename().string());
            continue;
        }

        const std::string folder = item.path().stem().string();
        ENGINE_LOG_INFO(Channels::kFileSys, "mounted '{}' as '{}/' ({} file(s))",
                        item.path().filename().string(), folder,
                        archive.entries.size());
        g_mounted.emplace(folder, std::move(archive));
        ++mounted;
    }

    return mounted;
}

void AssetArchive::UnmountAll() {
    g_mounted.clear();
}

bool AssetArchive::AnyMounted() {
    return !g_mounted.empty();
}

bool AssetArchive::Contains(std::string_view virtualPath) {
    const Mounted* archive = nullptr;
    return FindEntry(virtualPath, &archive) != nullptr;
}

bool AssetArchive::Read(std::string_view virtualPath,
                        std::vector<unsigned char>& outBytes, std::string& outError) {
    const Mounted* archive = nullptr;
    const Entry*   located = FindEntry(virtualPath, &archive);
    if (located == nullptr || archive == nullptr) {
        outError = "'" + std::string(virtualPath) + "' is not in any mounted archive";
        return false;
    }

    const Entry& entry = *located;

    std::ifstream file(archive->file, std::ios::binary);
    if (!file) {
        outError = "could not reopen '" + archive->file.string() + "'";
        return false;
    }

    file.seekg(static_cast<std::streamoff>(entry.offset), std::ios::beg);
    outBytes.resize(static_cast<std::size_t>(entry.size));
    if (entry.size > 0 &&
        !file.read(reinterpret_cast<char*>(outBytes.data()),
                   static_cast<std::streamsize>(entry.size))) {
        outBytes.clear();
        outError = "'" + std::string(virtualPath) + "' ends sooner than its index says";
        return false;
    }

    // Unmask. The same operation as masking, because XOR undoes itself - which
    // is why there is one function rather than two that have to agree.
    for (std::size_t i = 0; i < outBytes.size(); ++i) {
        outBytes[i] ^= MaskByte(i);
    }

    outError.clear();
    return true;
}

void AssetArchive::ListFolder(std::string_view virtualDirectory,
                              std::vector<std::string>& outPaths) {
    outPaths.clear();

    std::string folder(virtualDirectory);
    while (!folder.empty() && folder.back() == '/') {
        folder.pop_back();
    }

    const auto archive = g_mounted.find(folder);
    if (archive == g_mounted.end()) {
        return;
    }
    for (const auto& [name, entry] : archive->second.entries) {
        (void)entry;
        outPaths.push_back(folder + "/" + name);
    }
}

bool AssetArchive::Write(const std::string& destinationFile,
                         const std::vector<SourceFile>& files, std::string& outError) {
    std::error_code ec;

    // ---- measure, so the index can be written before the payload ----------
    struct Pending {
        std::string name;   // relative, forward slashes
        fs::path    path;
        std::size_t size = 0;
    };
    std::vector<Pending> pending;
    pending.reserve(files.size());

    for (const SourceFile& file : files) {
        Pending item;
        item.path = fs::path(file.sourcePath);
        item.name = ToVirtualSeparators(file.entryName);
        item.size = static_cast<std::size_t>(fs::file_size(item.path, ec));
        if (ec || item.name.empty()) {
            outError = "could not measure '" + file.sourcePath + "'";
            return false;
        }
        pending.push_back(std::move(item));
    }

    // Sorted so that packing the same folder twice produces the same bytes.
    // A build that is not reproducible is one you cannot diff, and "did the
    // assets change?" becomes a question nobody can answer.
    std::sort(pending.begin(), pending.end(),
              [](const Pending& a, const Pending& b) { return a.name < b.name; });

    fs::create_directories(fs::path(destinationFile).parent_path(), ec);
    std::ofstream out(destinationFile, std::ios::binary | std::ios::trunc);
    if (!out) {
        outError = "could not create '" + destinationFile + "'";
        return false;
    }

    // ---- header ----------------------------------------------------------
    out.write(kMagic, sizeof(kMagic));
    PutU32(out, kVersion);
    PutU32(out, static_cast<std::uint32_t>(pending.size()));

    // ---- index -----------------------------------------------------------
    //
    // The payload offsets have to be known BEFORE the index is written, so
    // they are worked out from the index's own size rather than by writing the
    // index twice or seeking back over it.
    std::uint64_t indexSize = 0;
    for (const Pending& item : pending) {
        indexSize += 4 + item.name.size() + 8 + 8;
    }
    std::uint64_t cursor = sizeof(kMagic) + 4 + 4 + indexSize;

    for (const Pending& item : pending) {
        PutU32(out, static_cast<std::uint32_t>(item.name.size()));
        out.write(item.name.data(), static_cast<std::streamsize>(item.name.size()));
        PutU64(out, cursor);
        PutU64(out, static_cast<std::uint64_t>(item.size));
        cursor += item.size;
    }

    // ---- payload ---------------------------------------------------------
    std::vector<unsigned char> buffer;
    for (const Pending& item : pending) {
        std::ifstream in(item.path, std::ios::binary);
        if (!in) {
            outError = "could not read '" + item.path.string() + "'";
            return false;
        }
        buffer.assign(std::istreambuf_iterator<char>(in),
                      std::istreambuf_iterator<char>());
        if (buffer.size() != item.size) {
            outError = "'" + item.name + "' changed size while it was being packed";
            return false;
        }
        for (std::size_t i = 0; i < buffer.size(); ++i) {
            buffer[i] ^= MaskByte(i);
        }
        out.write(reinterpret_cast<const char*>(buffer.data()),
                  static_cast<std::streamsize>(buffer.size()));
    }

    if (!out) {
        outError = "writing '" + destinationFile + "' failed part way through";
        return false;
    }

    outError.clear();
    return true;
}

} // namespace eng
