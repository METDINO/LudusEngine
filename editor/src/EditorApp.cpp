#include "EditorApp.h"
#include "panels/EventInspectorPanel.h"

#include <engine/platform/EventPump.h>
#include <engine/platform/Window.h>
#include <engine/tools/EditorGui.h>

#include <imgui.h>


namespace editor {


EditorApp::EditorApp()  = default;
EditorApp::~EditorApp() = default;

bool EditorApp::Init() {
    // Order: window, then GUI. ImGui's backends need a window and a renderer
    // to attach to, so there is only one possible order and the failure if you
    // get it wrong is immediate.
    m_window = std::make_unique<eng::Window>("Engine2D Editor", 1600, 900);
    if (!m_window->IsValid()) {
        std::fprintf(stderr, "Editor: window creation failed.\n");
        return false;
    }

    if (!eng::EditorGui::Init(*m_window)) {
        std::fprintf(stderr, "Editor: ImGui initialisation failed.\n");
        return false;
    }

    m_pump = std::make_unique<eng::EventPump>();

    // One line per panel. This is the design goal for the editor environment
	// - adding a new panel is a one-line change, if it takes more than that,
	// the panel is doing too much and should be split into smaller panels.
    AddPanel(std::make_unique<EventInspectorPanel>(*m_pump));

    return true;
}

void EditorApp::AddPanel(std::unique_ptr<Panel> panel) {
    m_panels.push_back(std::move(panel));
}

void EditorApp::Run() {
    bool running = true;

    while (running) {
        m_pump->Poll();
        if (m_pump->QuitRequested()) {
            running = false;
        }

        eng::EditorGui::BeginFrame();
        eng::EditorGui::BeginDockspace();

        DrawMenuBar();

        for (auto& panel : m_panels) {
            if (!panel->IsOpen()) {
                continue;
            }

            // -----------------------------------------------------------------
            //  Begin/End live in the CALLER, not in the panel. One convention,
            //  applied everywhere, so visibility handling is identical for all
            //  panels.
            // 
            //  Passing the panel's open flag to Begin gives every window a
            //  close button for free, wired to the same flag the View menu
            //  toggles.
            // -----------------------------------------------------------------

            bool open = panel->IsOpen();
            if (ImGui::Begin(panel->Title(), &open)) {
                panel->Draw();
            }
            ImGui::End();
            panel->SetOpen(open);
        }

        // Game rendering goes here in later weeks. Clear first so the IDE is
        // drawn on top of the frame rather than under it.
        m_window->Clear(18, 20, 26);

        eng::EditorGui::EndFrame();   // ImGui draws over whatever came before
        m_window->Present();
    }
}

void EditorApp::Shutdown() {
    // Reverse of Init, again. The panels hold references to the pump, so they
    // go first; the GUI must be shut down before the window it attached to.
    m_panels.clear();
    eng::EditorGui::Shutdown();
    m_pump.reset();
    m_window.reset();
}

void EditorApp::DrawMenuBar() {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit")) {
			//TODO: set a flag to exit the main loop
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        // -------------------------------------------------------------------
        //  Generated from the panel list. Write this loop once and never touch
        //  the menu again - every panel added between now and Week 16 appears
        //  here automatically.
        //
        //  Note `&open` rather than a return value: MenuItem's bool* overload
        //  renders a checkmark and toggles in place.
        // -------------------------------------------------------------------
        for (auto& panel : m_panels) {
            bool open = panel->IsOpen();
            if (ImGui::MenuItem(panel->Title(), nullptr, &open)) {
                panel->SetOpen(open);
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
        // The ImGui demo, reachable from the menu. This is a great reference
        // that exists for the library and having it one click away means
        // students actually use it instead of searching for stale tutorials.
        ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemo);
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();

    if (m_showDemo) {
        ImGui::ShowDemoWindow(&m_showDemo);
    }
}

} // namespace editor
