# =============================================================================
#  WEEK 2 - the unit test target.
#
#  This replaces the Week 1 placeholder. From here on you have three targets:
#  engine (library), sandbox (executable), tests (executable).
#
#  Tests link against `engine`, exactly like the sandbox does. They can only
#  see the engine's PUBLIC headers. That is a feature: if something is hard to
#  test, it is usually because it is hard to use.
# =============================================================================

set(TEST_SOURCES
    src/test_main.cpp
    src/test_bytebuffer.cpp
    src/test_memutil.cpp
    # Week 5: src/test_queue.cpp
    # Week 6: src/test_vec2.cpp, src/test_overlap.cpp, src/test_transform.cpp
    # Week 7: src/test_allocators.cpp   <- provided, do not edit
)

add_executable(tests ${TEST_SOURCES})

target_link_libraries(tests PRIVATE Engine2D::engine doctest::doctest)

engine_set_warnings(tests)

# Registers the whole binary as one CTest test. Good enough all semester;
# doctest reports which individual cases failed in its own output.
add_test(NAME unit_tests COMMAND tests)
