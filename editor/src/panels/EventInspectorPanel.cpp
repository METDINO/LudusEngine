#include "panels/EventInspectorPanel.h"

#include <engine/platform/EventPump.h>

#include <imgui.h>

namespace editor {

EventInspectorPanel::EventInspectorPanel(const eng::EventPump& pump)
    : m_pump(&pump) {}

void EventInspectorPanel::Draw() {
    ImGui::Checkbox("Pause capture", &m_paused);
    ImGui::SameLine();
    if (ImGui::Button("Reset totals")) {
        m_totals.fill(0);
    }

    ImGui::Separator();

    const eng::usize count = m_pump->Count();

    if (!m_paused) {
        for (eng::usize i = 0; i < count; ++i) {
            const auto kind = m_pump->At(i).kind;
            m_totals[static_cast<eng::usize>(kind)] += 1;
        }
    }

    ImGui::Text("Events this frame: %zu", count);

    // -------------------------------------------------------------------------
    //  Running totals per kind. This is the panel doing the job the old Week 2
    //  stretch goal did with a debug key and console spam - except you can
    //  watch the numbers move while wiggling the mouse, which is the entire
    //  improvement.
    // -------------------------------------------------------------------------
    if (ImGui::CollapsingHeader("Totals", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("totals", 2,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Kind");
            ImGui::TableSetupColumn("Count");
            ImGui::TableHeadersRow();

            for (eng::usize k = 0; k < m_totals.size(); ++k) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(
                    eng::ToString(static_cast<eng::RawEventKind>(k)));
                ImGui::TableNextColumn();
                ImGui::Text("%llu", static_cast<unsigned long long>(m_totals[k]));
            }
            ImGui::EndTable();
        }
    }

    ImGui::Separator();

    // -------------------------------------------------------------------------
    //  This frame's events in detail.
    //
    //  Note that this reads straight from the pump every frame and stores
    //  nothing. That is immediate mode working for you: the panel physically
    //  cannot show stale data, because it has none to show.
    // -------------------------------------------------------------------------
    if (ImGui::BeginTable("events", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_ScrollY,
                          ImVec2(0.0f, 220.0f))) {
        ImGui::TableSetupColumn("#");
        ImGui::TableSetupColumn("Kind");
        ImGui::TableSetupColumn("Code");
        ImGui::TableSetupColumn("Mouse");
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (eng::usize i = 0; i < count; ++i) {
            const eng::RawEvent& e = m_pump->At(i);

            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%zu", i);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(eng::ToString(e.kind));
            ImGui::TableNextColumn(); ImGui::Text("%d", e.code);
            ImGui::TableNextColumn(); ImGui::Text("%.0f, %.0f", static_cast<double>(e.mouseX),
                                                                static_cast<double>(e.mouseY));
        }
        ImGui::EndTable();
    }

    ImGui::TextDisabled("Move the mouse over the game area, then over this panel.");
    ImGui::TextDisabled("Mouse events should stop arriving while the cursor is here.");
    // ^ that is the input-capture check from Week 2, built into the tool.
}

} // namespace editor
