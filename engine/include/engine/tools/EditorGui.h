#pragma once



namespace eng {

class Window;
struct RawEvent;

class EditorGui {
public:
    // TODO(week2): create the ImGui context, enable the docking config flag,
    //   and initialise BOTH backends - the SDL3 platform backend and the
    //   SDL_Renderer3 renderer backend. Two calls, and forgetting the second
    //   produces a window that runs fine and draws nothing.
    //
    //   Docking is not on by default. You must set the docking config flag on
    //   the io struct or none of the panels will dock, and you will conclude
    //   you cloned the wrong branch.
    static bool Init(Window& window);

    // TODO(week2): shut down renderer backend, platform backend, context -
    //   in that order, the exact reverse of Init. Week 3's ordered-teardown
    //   discipline, arriving a week early.
    static void Shutdown();

    // TODO(week2): hand one SDL event to ImGui. Called from EventPump.
    //   Returns true if ImGui consumed it and the engine should ignore it.
    static bool ProcessEvent(const void* sdlEvent);

    // TODO(week2): begin a frame. Call before any panel code runs.
    static void BeginFrame();

    // TODO(week2): render every queued panel. Call AFTER the game has drawn,
    //   BEFORE the frame is presented, so the IDE lands on top of the game.
    static void EndFrame();

    // TODO(week2): does ImGui currently want the keyboard / mouse?
    //   Your EventPump checks these before routing input to gameplay.
    static bool WantsKeyboard();
    static bool WantsMouse();

    // TODO(week2): a full-window dockspace, so panels can be arranged and the
    //   layout persists between runs. ImGui writes its own .ini file for that;
    //   decide where it goes and add it to .gitignore - a layout is a personal
    //   preference, not source.
    static void BeginDockspace();
};

} // namespace eng
