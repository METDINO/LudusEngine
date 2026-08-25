#pragma once
#include "panels/Panel.h"
#include <engine/core/Types.h>
#include <array>

namespace eng { class EventPump; }

namespace editor {

class EventInspectorPanel final : public Panel {
public:
    explicit EventInspectorPanel(const eng::EventPump& pump);

    const char* Title() const override { return "Event Inspector"; }
    void        Draw() override;

private:
    const eng::EventPump* m_pump = nullptr;

    // One slot per RawEventKind. Sized generously to plan ahead 
    // and note that this is a FIXED array, not a growing container, 

    std::array<eng::u64, 16> m_totals{};

    bool m_paused = false;
};

} // namespace editor
