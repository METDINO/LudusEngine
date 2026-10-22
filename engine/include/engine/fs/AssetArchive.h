#pragma once

// ============================================================================
//  AssetArchive.h - one folder of assets, packed into one .bin file.
//
//  An exported game ships assets/scenes.bin and assets/textures.bin instead of
//  the folders themselves. Everything that reads an asset goes through
//  FileSystem, and FileSystem looks in the mounted archives before it looks on
//  disk - so nothing else in the engine changes, and no call site knows which
//  of the two it got.
//
//  ==========================================================================
//  BE HONEST ABOUT WHAT THIS IS FOR.
//
//  It stops a player opening assets/textures/ and painting over your sprites,
//  or editing a scene file to give themselves a hundred lives. It is a LOCK ON
//  A GARDEN GATE: it keeps honest people out and announces that inside is not
//  meant to be rearranged.
//
//  IT IS NOT ENCRYPTION AND IT IS NOT SECURITY. The bytes are masked with a
//  fixed repeating pattern, which anybody who opens this header can undo in
//  twenty minutes, and the file names sit in the index in plain text. Do not
//  put anything secret in here - not a password, not an API key, nothing you
//  would mind a determined fourteen-year-old reading. Every commercial game
//  that has tried to win this fight has lost it; the realistic goal is to make
//  casual modification inconvenient, and that this does.
//
//  ==========================================================================
//  THE FORMAT, which is deliberately small enough to read in one sitting.
//
//      magic        4 bytes   "OBIN"
//      version      uint32    1
//      entryCount   uint32
//
//      then entryCount of:
//          nameLength  uint32
//          name        nameLength bytes, e.g. "Blue/Blue_walk_1.png"
//          dataOffset  uint64    from the start of the file
//          dataSize    uint64
//
//      then the file contents, in order, each masked.
//
//  NAMES ARE RELATIVE TO THE FOLDER and always use forward slashes, so
//  textures.bin holds "checker_red.bmp" and "Blue/Blue_walk_1.png". The
//  virtual path "textures/Blue/Blue_walk_1.png" splits at the first slash: the
//  part before it picks the archive, the part after it picks the entry.
//
//  EVERY NUMBER IS WRITTEN LITTLE-ENDIAN BY HAND rather than by copying the
//  bytes of a uint32 straight out of memory. Those happen to be the same thing
//  on a PC, which is exactly why doing it the lazy way is a trap: the file
//  would silently stop being readable the day somebody built this for a
//  machine that orders bytes the other way, and the symptom would be an
//  absurd entry count rather than anything that points at the cause.
// ============================================================================

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace eng {

class AssetArchive {
public:
    static constexpr char          kMagic[4]  = {'O', 'B', 'I', 'N'};
    static constexpr std::uint32_t kVersion   = 1;
    static constexpr const char*   kExtension = ".bin";

    // ---- reading, used by the game ----------------------------------------

    // Mounts every .bin directly inside `assetsDirectory`. The archive's file
    // name without the extension is the folder it stands in for, so
    // scenes.bin answers for every path beginning "scenes/".
    //
    // Returns how many archives were mounted. Zero is NOT an error: that is a
    // project running from loose files, which is what the editor always does.
    static int MountAll(const std::string& assetsDirectory);

    static void UnmountAll();

    // Is anything mounted at all? When false, FileSystem goes straight to disk
    // and this whole file costs nothing.
    static bool AnyMounted();

    // Does a mounted archive hold this virtual path?
    static bool Contains(std::string_view virtualPath);

    // Reads one entry. Returns false with a reason when the path is not in any
    // mounted archive, or the archive is damaged.
    static bool Read(std::string_view virtualPath, std::vector<unsigned char>& outBytes,
                     std::string& outError);

    // The entries in one mounted archive, as virtual paths. Empty when that
    // folder has no archive.
    static void ListFolder(std::string_view virtualDirectory,
                           std::vector<std::string>& outPaths);

    // ---- writing, used by the editor's exporter ---------------------------

    // One file to go into an archive.
    struct SourceFile {
        std::string entryName;    // inside the archive, e.g. "Blue/walk_1.png"
        std::string sourcePath;   // where to read it from, on disk
    };

    // Packs exactly these files into `destinationFile`.
    //
    // AN EXPLICIT LIST RATHER THAN A FOLDER TO WALK, because the exporter has
    // already worked out which assets the game actually refers to and ships
    // only those. A writer that walked the folder itself would quietly undo
    // that and pack the whole asset pack back in.
    //
    // The writer lives beside the reader on purpose. A format with its two
    // halves in different programs drifts, and the failure then shows up as a
    // corrupt archive rather than as the mismatch it really is.
    static bool Write(const std::string& destinationFile,
                      const std::vector<SourceFile>& files, std::string& outError);

    // The mask applied to the stored bytes. Exposed because it is part of the
    // format, not a secret - see the note at the top of this file.
    static unsigned char MaskByte(std::uint64_t positionInEntry);
};

} // namespace eng
