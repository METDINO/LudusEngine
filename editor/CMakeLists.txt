set(EDITOR_SOURCES
    src/main.cpp
    src/EditorApp.cpp
    src/panels/EventInspectorPanel.cpp
    # Week 3:  src/panels/LogPanel.cpp
    # Week 4:  src/panels/ProfilerPanel.cpp
    # Week 5:  src/panels/JobPanel.cpp
    # Week 6:  src/panels/ViewportPanel.cpp
    # Week 7:  src/panels/MemoryPanel.cpp
    # Week 8:  src/panels/CVarPanel.cpp
    # Week 9:  src/panels/HierarchyPanel.cpp, src/panels/ResourcePanel.cpp
    # Week 10: src/panels/InspectorPanel.cpp, src/panels/ToolbarPanel.cpp
)

add_executable(editor ${EDITOR_SOURCES})

target_include_directories(editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(editor PRIVATE Engine2D::engine)

engine_set_warnings(editor)

if(WIN32)
    set_target_properties(editor PROPERTIES WIN32_EXECUTABLE FALSE)
endif()
