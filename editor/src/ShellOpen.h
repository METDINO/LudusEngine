#pragma once

// ============================================================================
//  ShellOpen.h - handing a file to the operating system.
//
//  "Open this in whatever opens it" is one call, SDL_OpenURL, which is
//  ShellExecute on Windows, `open` on a Mac and xdg-open on Linux. It opens
//  a .cpp in whatever you have set up for C++ files, and a .png in your image
//  viewer, because it asks the system rather than keeping a list of its own.
//
//  ==========================================================================
//  IT CAN FAIL, AND THE FAILURE USED TO BE INVISIBLE.
//
//  ShellExecute only works when the extension has a program REGISTERED
//  against it. Plenty of machines have nothing associated with .cpp, and
//  NOTHING anywhere is associated with .oscene.
//
//  That alone would be fine. What made it a problem is where the failure
//  went: a warning in the log, and `config/engine.json` sets a log threshold.
//  With that threshold at "Error" - which it is in this project - every
//  warning is discarded before it reaches the Console OR the log file. So
//  the whole symptom was a double-click that did nothing, with no way to
//  find out why.
//
//  THE RULE THAT CAME OUT OF IT: a failure in something the user just asked
//  for is reported at ERROR, so it survives the project's own log settings -
//  and shown on screen as well, not only logged. "Warning" is for things the
//  program noticed on its own; this is an answer to a question somebody
//  asked.
//
//  ==========================================================================
//  REVEAL IN FOLDER IS THE ONE THAT ALWAYS WORKS.
//
//  A directory always has something registered against it, so opening the
//  containing folder succeeds where opening the file itself does not - and
//  from there the file can be dragged into whatever editor you actually use.
//  That is why it sits next to Open in the menu rather than being hidden.
// ============================================================================

#include <string>
#include <string_view>

namespace editor {

// Hands the file to whatever the system opens it with. False with a reason
// when there is no such program, which is an ordinary outcome rather than a
// fault - see above.
bool OpenExternally(std::string_view virtualPath, std::string& outError);

// Opens the folder the file is in. Works for a file or a folder, and works
// for extensions nothing is registered against.
bool RevealInFolder(std::string_view virtualPath, std::string& outError);

} // namespace editor
