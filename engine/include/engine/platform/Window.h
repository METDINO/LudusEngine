#pragma once

// =============================================================================
//  WEEK 1 TEMPLATE - you implement this.
//
//  This header exists to give you the Ch. 2.2 exercise in a form you will
//  actually keep: a class DECLARED here and DEFINED in Window.cpp. Read the
//  declaration/definition distinction in the chapter, then notice that this
//  file promises things it does not contain. The linker is what checks that
//  promise, and it checks it much later than the compiler does.
//
//  Deliberately holding raw SDL pointers is wrong, and we will fix it.
//  Leave it wrong for now as the conversion is more instructive
//  when you have working code to convert.
// =============================================================================

#include <engine/core/Types.h>

struct SDL_Window;     // forward declaration: we promise these types exist
struct SDL_Renderer;   // without dragging all of SDL into every file

namespace eng {

class Window {
public:
    // TODO(week1): construct a window of the given size and title.
    Window(const char* title, i32 width, i32 height);

    // TODO(week1): destroy the renderer, then the window, then quit SDL video.
    // Order matters. Get it backwards and you will find out at shutdown.
    ~Window();

    // TODO(week1): why are these deleted? Write a one-line comment answering
    // that, and be ready to say it out loud. Hint: what would happen if a
    // Window were copied and both copies ran their destructor?

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    // TODO(week1): true if construction succeeded.
    bool IsValid() const;

    // TODO(week1): fill the whole window with one colour.
    void Clear(u8 r, u8 g, u8 b);

    // TODO(week1): push the finished frame to the screen.
    void Present();

    // Temporary escape hatch so the sandbox can poll events in Week 1.
    SDL_Renderer* RawRenderer() const { return m_renderer; }

private:
    SDL_Window*   m_window   = nullptr;
    SDL_Renderer* m_renderer = nullptr;

    bool sdl_videoInitialized = false;

};

} // namespace eng
