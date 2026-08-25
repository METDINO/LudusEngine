#pragma once
#include <engine/core/Types.h>
#include <vector>

namespace eng {

// Deliberately coarse this week. Week 8 replaces this with named actions.
enum class RawEventKind : u8 {
    None,
    Quit,
    KeyDown,
    KeyUp,
    MouseButtonDown,
    MouseButtonUp,
    MouseMove,
};

// Added for the Event Inspector panel. Note that it returns a string literal,
// not a constructed string - so there is no allocation and no lifetime
// question, unlike Week 2's DescribeBuffer.
const char* ToString(RawEventKind kind);

struct RawEvent {
    RawEventKind kind   = RawEventKind::None;
    i32          code   = 0;      // key code or mouse button, depending on kind
    f32          mouseX = 0.0f;
    f32          mouseY = 0.0f;
};

class EventPump {
public:
    // Drains the SDL queue COMPLETELY. Call exactly once per frame.
    void Poll();

    usize Count() const { return m_events.size(); }
    const RawEvent&  At(usize index) const;
    bool QuitRequested() const { return m_quitRequested; }

private:
    std::vector<RawEvent> m_events;
    bool    m_quitRequested = false;

public:
    EventPump() { m_events.reserve(64); }
};

} // namespace eng
