// =============================================================================
//  Everything interesting is in EditorApp or a panel.
// =============================================================================

#include "EditorApp.h"
#include <cstdio>

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    editor::EditorApp app;

    if (!app.Init()) {
        std::fprintf(stderr, "Editor failed to start.\n");
        return 1;
    }

    app.Run();
    app.Shutdown();

    std::printf("Editor exited cleanly.\n");


    return 0;
}
