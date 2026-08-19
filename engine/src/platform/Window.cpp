// =============================================================================
//  WEEK 1 TEMPLATE - you implement this.
//
//  Every function declared in Window.h must be defined here, or the LINKER
//  will reject the build; not the compiler. Try it: comment out one of these
//  definitions and read the error carefully. It will not name a line number in
//  a source file, because there is no source line to name.
// =============================================================================

#include <engine/platform/Window.h>
#include <SDL3/SDL.h>
#include <print>

namespace eng {

Window::Window(const char* title, i32 width, i32 height) {
    // TODO(week1):
    //   1. SDL_Init with the video subsystem.
    //   2. Create a window and a renderer.
    //      SDL_CreateWindowAndRenderer does both in one call - read its docs
    //   3. If anything fails, leave m_window/m_renderer null and log
    //      SDL_GetError() to stderr. Do not throw. Do not silently continue.
    //
    // Read the return value of every SDL call. All of them can fail.
    
    (void)title; (void)width; (void)height;
}

Window::~Window() {
    // TODO(week1): tear down in the exact reverse of construction.
    //   renderer, then window, then SDL_Quit().
    //
    // Ask yourself what happens if construction failed halfway through and
    // one of these pointers is null. Then go read what SDL does when handed
    // a null pointer, rather than guessing.
}

bool Window::IsValid() const {
    // TODO(week1)
    return false;
}

void Window::Clear(u8 r, u8 g, u8 b) {
    // TODO(week1): SDL_SetRenderDrawColor, then SDL_RenderClear.
    (void)r; (void)g; (void)b;
}

void Window::Present() {
    // TODO(week1): SDL_RenderPresent.
}

} // namespace eng
