#pragma once

#include <memory>
#include <vector>

#include "panels/Panel.h"

namespace eng { class Window; class EventPump; }

namespace editor {

class EditorApp {
public:
    EditorApp();
    ~EditorApp();

    bool Init();
    void Run();
    void Shutdown();

    void AddPanel(std::unique_ptr<Panel> panel);

private:
    void DrawMenuBar();

    std::unique_ptr<eng::Window>    m_window;
    std::unique_ptr<eng::EventPump> m_pump;

    std::vector<std::unique_ptr<Panel>> m_panels;

    bool m_showDemo = false;
};

} // namespace editor
