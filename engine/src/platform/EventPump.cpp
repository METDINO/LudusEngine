#include <engine/platform/EventPump.h>

#include <SDL3/SDL.h>

#ifdef ENGINE_WITH_IMGUI
  #include <engine/tools/EditorGui.h>
#endif

#include <cstdio>

namespace eng {

const char* ToString(RawEventKind kind) {
    switch (kind) {
        case RawEventKind::None:            return "None";
        case RawEventKind::Quit:            return "Quit";
        case RawEventKind::KeyDown:         return "KeyDown";
        case RawEventKind::KeyUp:           return "KeyUp";
        case RawEventKind::MouseButtonDown: return "MouseButtonDown";
        case RawEventKind::MouseButtonUp:   return "MouseButtonUp";
        case RawEventKind::MouseMove:       return "MouseMove";
    }
    return "Unknown";
    // No `default:` on purpose. With every enumerator listed, adding a new one
    // in Week 8 produces a -Wswitch warning here pointing at the exact place
    // that needs updating. A `default` would silence that and let the new kind
    // quietly print "Unknown" forever.
}

void EventPump::Poll() {
    // clear(), not a new vector. See the storage note in the header.
    m_events.clear();
    m_quitRequested = false;

    SDL_Event event;

    // -------------------------------------------------------------------------
    //  DRAIN COMPLETELY. `while`, not `if`.
    //
    //  SDL_PollEvent removes one event per call and returns false when empty.
    //  Handling one per frame makes events back up, which presents as input lag
    //  that worsens the more the player does - a genuinely unpleasant bug to
    //  find later, because it looks like a performance problem.
    // -------------------------------------------------------------------------
    while (SDL_PollEvent(&event)) {

#ifdef ENGINE_WITH_IMGUI
        // ---------------------------------------------------------------------
        //  Give the IDE first refusal on every event, ALWAYS - even ones it
        //  will not consume. ImGui tracks mouse position and modifier state
        //  continuously, and skipping events it "does not need" leaves it with
        //  a stale view of the world.
        // ---------------------------------------------------------------------
        EditorGui::ProcessEvent(&event);
#endif

        // Quit is never swallowed by the GUI. Closing the window closes the
        // window, whatever has focus.
        if (event.type == SDL_EVENT_QUIT ||
            event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            m_quitRequested = true;
            m_events.push_back(RawEvent{RawEventKind::Quit, 0, 0.0f, 0.0f});
            continue;
        }

#ifdef ENGINE_WITH_IMGUI
        // ---------------------------------------------------------------------
        //  INPUT CAPTURE. Solve it here; it stays solved through Week 8.
        //
        //  When a panel's text field has focus, ImGui wants the keyboard. If
        //  we forward those keystrokes onward as well, typing an entity name
        //  in the Week 10 Inspector also makes the player jump.
        //
        //  Same for the mouse when the cursor is over a panel: clicking a
        //  button should not also fire a weapon.
        //
        //  Week 8's InputMap sits on top of this pump, so getting it right in
        //  Week 2 gets it right for the whole course.
        // ---------------------------------------------------------------------
        const bool isKeyboard = (event.type == SDL_EVENT_KEY_DOWN ||
                                 event.type == SDL_EVENT_KEY_UP);
        const bool isMouse    = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                                 event.type == SDL_EVENT_MOUSE_BUTTON_UP ||
                                 event.type == SDL_EVENT_MOUSE_MOTION);

        if ((isKeyboard && EditorGui::WantsKeyboard()) ||
            (isMouse    && EditorGui::WantsMouse())) {
            continue;
        }
#endif

        switch (event.type) {
            case SDL_EVENT_KEY_DOWN:
                // event.key.repeat is non-zero for auto-repeat from a held key.
                // Passing repeats through is fine for text; for gameplay it
                // makes a held key fire continuously. Week 8's Pressed/Held
                // distinction is the proper fix - noting it here so the
                // decision is visible rather than accidental.
                m_events.push_back(RawEvent{
                    RawEventKind::KeyDown,
                    static_cast<i32>(event.key.key),
                    0.0f, 0.0f});
                break;

            case SDL_EVENT_KEY_UP:
                m_events.push_back(RawEvent{
                    RawEventKind::KeyUp,
                    static_cast<i32>(event.key.key),
                    0.0f, 0.0f});
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                m_events.push_back(RawEvent{
                    RawEventKind::MouseButtonDown,
                    static_cast<i32>(event.button.button),
                    event.button.x, event.button.y});
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                m_events.push_back(RawEvent{
                    RawEventKind::MouseButtonUp,
                    static_cast<i32>(event.button.button),
                    event.button.x, event.button.y});
                break;

            case SDL_EVENT_MOUSE_MOTION:
                // SDL3 reports mouse position as float, which is why RawEvent
                // stores f32. SDL2 used int. One more place old sample code
                // will mislead you.
                m_events.push_back(RawEvent{
                    RawEventKind::MouseMove, 0,
                    event.motion.x, event.motion.y});
                break;

            default:
                // Everything else ignored this week. A `default` IS correct
                // here - SDL defines hundreds of event types and we genuinely
                // do not care about most of them.
                break;
        }
    }
}

const RawEvent& EventPump::At(usize index) const {
    // -------------------------------------------------------------------------
    //  OUT OF RANGE BEHAVIOUR (the template asks you to decide and document).
    //
    //  This week: clamp to a shared empty event and print a warning. Not
    //  elegant, but it is defined, it is visible, and it does not crash.
    //
    //  In WEEK 3 this becomes ENGINE_ASSERT(index < m_events.size()) - because
    //  by then the assert macro exists, and this is squarely a programmer
    //  error rather than an environment failure. A caller asking for event 12
    //  when there are 3 has a bug in the caller.
    //
    //  Note the `static` empty event rather than a local: returning a
    //  reference to a local is exactly bug 5 in ByteBuffer, and it would be
    //  embarrassing to fix it there and commit it here in the same afternoon.
    // -------------------------------------------------------------------------
    static const RawEvent kEmpty{};

    if (index >= m_events.size()) {
        std::fprintf(stderr,
                     "EventPump::At(%zu) out of range - only %zu events this frame\n",
                     index, m_events.size());
        return kEmpty;
    }

    return m_events[index];
}

} // namespace eng
