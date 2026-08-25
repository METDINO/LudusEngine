#include <engine/tools/EditorGui.h>

#include <engine/platform/Window.h>

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <cstdio>

namespace eng {

namespace {
// Cached so ProcessEvent and EndFrame do not need the Window passed in.
// A single global renderer pointer is acceptable for a tools layer that is
// initialised once and torn down once; if it bothers you, that instinct is
// correct and Week 7's subsystem model is where it gets addressed properly.
SDL_Renderer* g_renderer = nullptr;
bool          g_initialised = false;
} // namespace

bool EditorGui::Init(Window& window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    // -------------------------------------------------------------------------
    //  DOCKING IS NOT ON BY DEFAULT, even on the docking branch.
    //
    //  Cloning v1.92.9b-docking gets you the CAPABILITY. This line turns it on.
    //  Miss it and panels refuse to dock, and the natural conclusion is that
    //  you cloned the wrong branch - which sends people looking in entirely the
    //  wrong place.
    // -------------------------------------------------------------------------
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // ImGui writes its window layout here between runs. Already in .gitignore -
    // a panel arrangement is a personal preference, not source.
    io.IniFilename = "imgui.ini";

    ImGui::StyleColorsDark();

    // -------------------------------------------------------------------------
    //  TWO backends, and both are required.
    //
    //    platform backend - windowing, input, timing        (SDL3)
    //    renderer backend - turns ImGui's vertex data into  (SDL_Renderer3)
    //                       actual draw calls
    //
    //  Initialising only the platform backend produces a program that runs
    //  perfectly and draws nothing at all, with no error anywhere. It is the
    //  single most common ImGui setup mistake.
    //
    //  Note InitForSDLRenderer, not InitForOpenGL / InitForVulkan / InitForSDLGPU.
    //  It must match the renderer backend you are pairing it with.
    // -------------------------------------------------------------------------
    if (!ImGui_ImplSDL3_InitForSDLRenderer(window.m_window, window.m_renderer)) {
        std::fprintf(stderr, "ImGui_ImplSDL3_InitForSDLRenderer failed\n");
        ImGui::DestroyContext();
        return false;
    }

    if (!ImGui_ImplSDLRenderer3_Init(window.m_renderer)) {
        std::fprintf(stderr, "ImGui_ImplSDLRenderer3_Init failed\n");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    g_renderer    = window.m_renderer;
    g_initialised = true;
    return true;
}

void EditorGui::Shutdown() {
    if (!g_initialised) {
        return;
    }

    // Exact reverse of Init: renderer backend, platform backend, context.
    // Week 3's ordered-teardown discipline, arriving a week early - and the
    // same reason as always, that the later thing was built on the earlier one.
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    g_renderer    = nullptr;
    g_initialised = false;
}

bool EditorGui::ProcessEvent(const void* sdlEvent) {
    if (!g_initialised) {
        return false;
    }

    // The void* in the signature is what keeps SDL_Event out of the public
    // header. The cast is confined to this file, which is exactly the trade
    // the header describes: one ugly line here so that nothing above the
    // platform layer has to know SDL exists.
    const auto* event = static_cast<const SDL_Event*>(sdlEvent);
    return ImGui_ImplSDL3_ProcessEvent(event);
}

void EditorGui::BeginFrame() {
    if (!g_initialised) {
        return;
    }

    // Renderer backend first, then platform, then ImGui itself. The order is
    // what the backends expect; it is not arbitrary and it is not documented
    // anywhere obvious except the official examples.
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void EditorGui::EndFrame() {
    if (!g_initialised) {
        return;
    }

    ImGui::Render();

    // -------------------------------------------------------------------------
    //  RenderDrawData takes the renderer as a parameter.
    //
    //  Tutorials that call it with one argument predate a 2024 breaking change
    //  in this backend. If you find sample code that does not compile here,
    //  that is why - and the header is the authority, not the tutorial.
    // -------------------------------------------------------------------------
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_renderer);
}

bool EditorGui::WantsKeyboard() {
    return g_initialised && ImGui::GetIO().WantCaptureKeyboard;
}

bool EditorGui::WantsMouse() {
    return g_initialised && ImGui::GetIO().WantCaptureMouse;
}

void EditorGui::BeginDockspace() {
    if (!g_initialised) {
        return;
    }

    // -------------------------------------------------------------------------
    //  One call creates a full-window dock target that panels can be dragged
    //  into, and it leaves the central area transparent so the game renders
    //  through it.
    //
    //  PassthruCentralNode is the flag that makes the middle see-through. Drop
    //  it and the dockspace paints over your game, which looks like the game
    //  stopped rendering.
    //
    //  SIGNATURE NOTE: this took no arguments in older ImGui. Since 1.91 it
    //  takes a dock ID and a viewport. If your build disagrees with this line,
    //  open imgui.h and read the declaration - it is the authority.
    // -------------------------------------------------------------------------
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);
}

} // namespace eng
