// =============================================================================
//  GameRules - the score, the clock, and whether you have won.
//
//  Attach it to ONE entity in the scene. It does not matter which, and it does
//  not need a sprite or a collider - an empty entity called "GameRules" is the
//  clearest thing to put it on. A script that is about the whole round rather
//  than about one object still has to live on an entity, because an entity is
//  the only thing the engine knows how to attach a script to.
//
//  -----------------------------------------------------------------------------
//  IT COUNTS THE WORLD RATHER THAN KEEPING A SCORE.
//
//  There is no `++m_collected` anywhere in this file. Every step it asks the
//  scene how many pickups are left and works the score out from that:
//
//      collected = total at the start - still there now
//
//  The alternative is for this script to keep a counter and for Pickup to tell
//  it to go up, through a message. That is one more moving part, and it has a
//  failure mode this version simply does not have: if a pickup is ever
//  destroyed by something other than being collected - the Inspector's delete
//  button, a script of your own, a reloaded scene - a counter drifts out of
//  step with the world and nothing reports it. Counting cannot drift.
//
//  It costs a loop over the entities once per step. For ten pickups that is
//  nothing. If you had ten thousand, the counter would be worth its extra
//  moving part - but you would have measured first.
//
//  -----------------------------------------------------------------------------
//  WHY THE TIMER USES deltaSeconds AND NEVER READS A CLOCK
//
//  deltaSeconds is the FIXED simulation step, handed in by the scheduler. A
//  function that asked the clock what time it was would make the countdown
//  depend on the frame rate, which is the whole thing this engine's fixed step
//  exists to avoid. Sixty seconds here is sixty seconds on every machine, and
//  the Toolbar's Step button advances it by exactly one tick.
//
//  -----------------------------------------------------------------------------
//  WHAT IS DELIBERATELY NOT HERE: quitting.
//
//  Reading a "Quit" action and shutting the program down belongs to the
//  PROGRAM, not to the game's rules - and this script runs inside the editor
//  too, where quitting would close the editor out from under somebody who was
//  only play-testing. The player executable handles that; see player/src/main.cpp.
// =============================================================================

#include <engine/Engine.h>

#include <cstdio>

namespace {

// Is this entity one of the collectibles? Answered from the entity's own DATA -
// a collider on the "Pickup" layer - rather than from its name. Somebody adding
// an eleventh pickup to the scene file needs no code change at all, which is
// the whole point of the component model.
bool IsPickup(eng::Entity& entity) {
    const auto* collider = entity.Find<eng::AABBColliderComponent>();
    return collider != nullptr && collider->Layer() == "Pickup";
}

class GameRules final : public eng::ScriptBehaviour {
public:
    void OnStart() {
        m_totalPickups = CountPickups();
        m_secondsLeft  = kTimeLimit;
        m_phase        = Phase::Playing;

        ENGINE_LOG_INFO(eng::Channels::kGame,
                        "GameRules: {} pickup(s) placed by the scene file, {:.0f} "
                        "seconds on the clock",
                        m_totalPickups, static_cast<double>(kTimeLimit));

        if (m_totalPickups == 0) {
            ENGINE_LOG_WARN(eng::Channels::kGame,
                            "no pickups in this scene - nothing has a collider on the "
                            "'Pickup' layer, so there is nothing to collect");
        }
    }

    void OnUpdate(float deltaSeconds) {
        // The score, derived rather than remembered. Anything already queued
        // for destruction counts as collected, so the number on screen changes
        // on the step you touched it rather than the step after.
        const int remaining = CountPickups();
        const int collected = m_totalPickups - remaining;

        if (m_phase == Phase::Playing) {
            m_secondsLeft -= deltaSeconds;

            if (collected >= m_totalPickups && m_totalPickups > 0) {
                m_phase = Phase::Won;
                ENGINE_LOG_INFO(eng::Channels::kGame,
                                "WIN, with {:.1f} seconds to spare",
                                static_cast<double>(m_secondsLeft));
            } else if (m_secondsLeft <= 0.0f) {
                m_secondsLeft = 0.0f;
                m_phase       = Phase::Lost;
                ENGINE_LOG_INFO(eng::Channels::kGame, "out of time, with {}/{} collected",
                                collected, m_totalPickups);
            }
        }

        DrawHud(collected);
    }

private:
    enum class Phase { Playing, Won, Lost };

    int CountPickups() const {
        eng::Scene* scene = GetScene();
        if (scene == nullptr) {
            return 0;
        }
        int count = 0;
        scene->ForEach([&count](eng::Entity& entity) {
            if (IsPickup(entity) && !eng::DeferredOps::IsPendingDestroy(entity.Id())) {
                ++count;
            }
        });
        return count;
    }

    void DrawHud(int collected) const {
        // SCREEN space, so the score and the timer stay put when the camera
        // moves. GizmoSpace::World would have them slide off the side.
        char line[96];

        std::snprintf(line, sizeof(line), "COLLECTED  %d / %d", collected,
                      m_totalPickups);
        eng::Gizmos::Text(eng::Vec2{16.0f, 16.0f}, line, eng::Color::White(), 0.0f,
                          eng::GizmoSpace::Screen);

        std::snprintf(line, sizeof(line), "TIME       %5.1f",
                      static_cast<double>(m_secondsLeft));
        eng::Gizmos::Text(eng::Vec2{16.0f, 34.0f}, line,
                          m_secondsLeft < 10.0f ? eng::Color::Red() : eng::Color::White(),
                          0.0f, eng::GizmoSpace::Screen);

        if (m_phase == Phase::Won) {
            eng::Gizmos::Text(eng::Vec2{16.0f, 64.0f}, "YOU WIN", eng::Color::Green(),
                              0.0f, eng::GizmoSpace::Screen);
        } else if (m_phase == Phase::Lost) {
            eng::Gizmos::Text(eng::Vec2{16.0f, 64.0f}, "OUT OF TIME", eng::Color::Red(),
                              0.0f, eng::GizmoSpace::Screen);
        }
    }

    int   m_totalPickups = 0;
    float m_secondsLeft  = 0.0f;
    Phase m_phase        = Phase::Playing;

    // How long the round lasts. Try changing it.
    static constexpr float kTimeLimit = 60.0f;
};

} // namespace

// Registers the name "GameRules" so that a scene file and the editor can find
// it. WITHOUT THIS LINE the file compiles and the script can never be attached.
ENGINE_REGISTER_SCRIPT(GameRules)
