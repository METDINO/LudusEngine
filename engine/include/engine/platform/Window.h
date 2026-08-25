#pragma once

#include <engine/core/Types.h>

struct SDL_Window;     // forward declaration: we promise these types exist
struct SDL_Renderer;   // without dragging all of SDL into every file

namespace eng {

class Window {
    friend class EditorGui;

public:
    Window(const char* title, i32 width, i32 height);

    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    bool IsValid() const;

    void Clear(u8 r, u8 g, u8 b);

    void Present();

    // Temporary escape hatch so the sandbox can poll events in Week 1.
    SDL_Renderer* RawRenderer() const { return m_renderer; }


private:
    SDL_Window*   m_window   = nullptr;
    SDL_Renderer* m_renderer = nullptr;
	bool sdl_videoInitialized = false; // Track if SDL video subsystem was initialized

};

} // namespace eng
