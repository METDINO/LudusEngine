#pragma once
namespace editor {

class Panel {
public:
    virtual ~Panel() = default;

    Panel()                        = default;
    Panel(const Panel&)            = delete;
    Panel& operator=(const Panel&) = delete;

    // The window title, and also the ImGui ID. Must be unique across panels -
    // two windows with the same title are the SAME window to ImGui, which
    // presents as one panel mysteriously containing another's contents.
    virtual const char* Title() const = 0;

    // Contents only. See the convention note below.
    virtual void Draw() = 0;

    bool IsOpen() const   { return m_open; }
    void SetOpen(bool on) { m_open = on; }

protected:
    bool m_open = true;
};

// -----------------------------------------------------------------------------
//  THE CONVENTION, decided once and applied to all ten panels:
//
//  ImGui::Begin() and ImGui::End() live in the CALLER (EditorApp::Run), not in
//  Draw(). Draw() emits contents and nothing else.
//
//  Visibility, the close button, and docking behaviour are then identical
//  for every panel, handled in one place.
// -----------------------------------------------------------------------------

} // namespace editor
