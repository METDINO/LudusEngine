// =============================================================================
//  Engine.cpp - a skeleton. Every function is here with the right signature and
//  an empty body. Engine.h is the specification; read it before filling one in.
// =============================================================================

#include <engine/Engine.h>

namespace eng {

// Returns the one and only engine. Created the first time it is asked for, so
// it is guaranteed to exist before anything tries to use it.
Engine& Engine::Get() {
    return instance;
}

// Hands back the game window, so the editor can attach its interface to it.
Window& Engine::GetWindow() {
    return *m_window;
}


bool Engine::RendererSubsystem::Init(const BootConfig&)
{
    Engine& engine = Engine::Get();

    if (!rRenderer::Init(engine.m_window)) {
        return false;
    }

    engine.m_camera.SetViewportSize(Renderer::OutputSize());

    return true;

}

void Engine::RenderSubsystem::Shutdown()
{
    Renderer::Shutdown();
}

bool Engine::GuiSubsystem::Init(const BootConfig&) 
{
    return m_init ? m_init() : true; //return true by default
}

void Engine::GuiSubsystem::Use(std::function < bool()> init, std::function<void()> shutdown)
{
    m_init = std::move(init);
    m_shutdown = std::move(init);
}

void Engine::GuiSubsystem::Shutdown()
{
    if (m_shutdown)
        m_shutdown;
}

// Builds the ordered list of subsystems. Registration order IS dependency
// order, and shutdown runs it in reverse: Log, FileSystem, Window, Renderer,
// EditorGui, Input, Resources, Gizmos, Messaging, Scripts, Scene, Collision.
void Engine::RegisterBuiltinSubsystems(const Options& options) {

    m_subsystems.Add("Log", m_log);
    m_subsystems.Add("FileSystem", m_filesystem);
    m_subsystems.Add("Window", m_window);
    m_subsystems.Add("Renderer", m_renderer);
    m_subsystems.Add("EditorGui", m_gui);
    m_subsystems.Add("Input", m_input);
    m_subsystems.Add("Resources", m_resources);
    m_subsystems.Add("Gizmos", m_gismo);
    m_subsystems.Add("MessageBus", m_messaging);
    m_subsystems.Add("ScriptLibrary", m_scripts);
    m_subsystems.Add("Scene", m_sceneSubsystem);
    m_subsystems.Add("Collisions", m_collisionSubsystem);

}

// Starts everything: reads the settings file, brings the subsystems up in
// order, sets the clock, and loads the starting scene. Returns false if the
// engine cannot run at all.
bool Engine::Init(const Options& /*options*/) {
    return false;
}

// Stops everything, in the exact reverse of the order it was started in.
void Engine::Shutdown() {
}

// Replaces the current scene with the one in the named file, and moves the
// camera to wherever that file says it should be.
bool Engine::LoadScene(std::string_view /*virtualPath*/, std::string& /*outError*/) {
    return false;
}

// Writes the current scene back out to a file, including where the camera is.
bool Engine::SaveScene(std::string_view /*virtualPath*/, std::string& /*outError*/) {
    return false;
}

// Takes a snapshot of the scene and starts running it. The snapshot is what
// makes pressing Play safe on a level you have been building.
bool Engine::EnterPlayMode(std::string& /*outError*/) {
    return false;
}

// Stops play mode and puts the snapshot back, undoing everything the running
// game did to the scene.
void Engine::ExitPlayMode() {
}

// Starts one frame: measures real time, reads input, and works out how many
// fixed simulation steps this frame owes. Returns false when it is time to quit.
bool Engine::BeginFrame() {
    return false;
}

// Runs the simulation steps this frame owes, in system order: gameplay,
// movement, collision, messages, create/destroy, camera.
void Engine::Simulate() {
}

// Draws the world through any camera into whatever is currently being drawn
// into. The editor calls this twice - once per view.
void Engine::RenderWorld(Camera& /*camera*/, bool /*includeGizmos*/) {
}

// Draws one frame for the standalone game, gizmos included.
void Engine::RenderFrame() {
}

// Shows the frame that was just drawn.
void Engine::PresentFrame() {
}

// The standalone game's whole loop: begin, simulate, render, present, repeat.
void Engine::Run() {

    while (BeginFrame()) {
        Simulate();
        RenderFrame();
        PresentFrame();
    }
}

} // namespace eng
