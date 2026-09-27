#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <chrono>

namespace vertex {

/**
 * All user-facing switches live here rather than in the rendering code or in a
 * hook.  Keeping this as one small state object is important for GD: a scene
 * can be destroyed and recreated while a hook object remains installed.
 */
enum class Feature : std::uint8_t {
    Noclip,
    PracticeMusic,
    IgnoreEsc,
    InstantRespawn,
    FpsBypass,
    UnlockAll,
    CharacterFilter,
    CustomObjectBypass,
    CopyHack,
    LevelEdit,
    ScaleHack,
    FreeScroll,
    ShowHitboxes,
    LayoutMode,
    NoParticles,
    HideUI,
    SessionTime,
    CPS,
    BestRun,
    Count,
};

class ModState final {
public:
    static ModState& get();

    bool enabled(Feature feature) const;
    void setEnabled(Feature feature, bool enabled);

    int targetTPS() const { return m_targetTPS; }
    void setTargetTPS(int value);

    float scaleLimit() const { return m_scaleLimit; }
    void setScaleLimit(float value);

    /** Starts a new per-level best-run session and clears the CPS queue. */
    void beginLevel();
    void updateBestPercent(float percent);
    float bestPercent() const { return m_bestPercent; }

    void recordPress();
    void recordRelease();
    std::size_t clicksPerSecond();
    bool buttonDown() const { return m_buttonDown; }

    /** Seconds since the mod was loaded, which is the game session start. */
    double sessionSeconds() const;

private:
    ModState();
    ModState(ModState const&) = delete;
    ModState& operator=(ModState const&) = delete;

    static constexpr std::size_t featureCount = static_cast<std::size_t>(Feature::Count);
    static char const* keyFor(Feature feature);
    void pruneClicks();

    bool m_enabled[featureCount]{};
    int m_targetTPS = 240;
    float m_scaleLimit = 999.f;
    float m_bestPercent = 0.f;
    bool m_buttonDown = false;
    std::deque<std::chrono::steady_clock::time_point> m_clicks;
    std::chrono::steady_clock::time_point m_launchTime;
};

} // namespace vertex
