#include "ModState.hpp"

#include <algorithm>
#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace vertex {

namespace {
    constexpr auto now = std::chrono::steady_clock::now;
}

ModState& ModState::get() {
    static ModState instance;
    return instance;
}

ModState::ModState() : m_launchTime(now()) {
    for (std::size_t i = 0; i < featureCount; ++i) {
        auto feature = static_cast<Feature>(i);
        m_enabled[i] = Mod::get()->getSavedValue<bool>(keyFor(feature), false);
    }

    m_targetTPS = std::clamp(
        Mod::get()->getSavedValue<int>("target-tps", 240),
        60,
        1000
    );
    m_scaleLimit = std::clamp(
        Mod::get()->getSavedValue<float>("scale-limit", 999.f),
        0.5f,
        999.f
    );
}

char const* ModState::keyFor(Feature feature) {
    switch (feature) {
        case Feature::Noclip:              return "player.noclip";
        case Feature::PracticeMusic:       return "player.practice-music";
        case Feature::IgnoreEsc:           return "player.ignore-esc";
        case Feature::InstantRespawn:      return "player.instant-respawn";
        case Feature::FpsBypass:           return "bypass.fps";
        case Feature::UnlockAll:           return "bypass.unlock-all";
        case Feature::CharacterFilter:     return "bypass.character-filter";
        case Feature::CustomObjectBypass:  return "bypass.custom-objects";
        case Feature::CopyHack:            return "creator.copy";
        case Feature::LevelEdit:           return "creator.edit";
        case Feature::ScaleHack:           return "creator.scale";
        case Feature::FreeScroll:          return "creator.free-scroll";
        case Feature::ShowHitboxes:        return "visuals.hitboxes";
        case Feature::LayoutMode:          return "visuals.layout";
        case Feature::NoParticles:         return "visuals.no-particles";
        case Feature::HideUI:              return "visuals.hide-ui";
        case Feature::SessionTime:         return "overlay.session-time";
        case Feature::CPS:                 return "overlay.cps";
        case Feature::BestRun:             return "overlay.best-run";
        case Feature::Count:               return "invalid";
    }
    return "invalid";
}

bool ModState::enabled(Feature feature) const {
    auto index = static_cast<std::size_t>(feature);
    return index < featureCount && m_enabled[index];
}

void ModState::setEnabled(Feature feature, bool value) {
    auto index = static_cast<std::size_t>(feature);
    if (index >= featureCount) {
        return;
    }

    m_enabled[index] = value;
    Mod::get()->setSavedValue(keyFor(feature), value);
}

void ModState::setTargetTPS(int value) {
    m_targetTPS = std::clamp(value, 60, 1000);
    Mod::get()->setSavedValue("target-tps", m_targetTPS);
}

void ModState::setScaleLimit(float value) {
    m_scaleLimit = std::clamp(value, 0.5f, 999.f);
    Mod::get()->setSavedValue("scale-limit", m_scaleLimit);
}

void ModState::beginLevel() {
    m_bestPercent = 0.f;
    m_buttonDown = false;
    m_clicks.clear();
}

void ModState::updateBestPercent(float percent) {
    percent = std::clamp(percent, 0.f, 100.f);
    if (percent > m_bestPercent) {
        m_bestPercent = percent;
        // This is deliberately a single session-level value.  It is useful for
        // recordings and survives a scene transition without pretending to be
        // an official online level statistic.
        Mod::get()->setSavedValue("last-best-percent", m_bestPercent);
    }
}

void ModState::pruneClicks() {
    auto cutoff = now() - std::chrono::seconds(1);
    while (!m_clicks.empty() && m_clicks.front() < cutoff) {
        m_clicks.pop_front();
    }
}

void ModState::recordPress() {
    m_buttonDown = true;
    m_clicks.push_back(now());
    pruneClicks();
}

void ModState::recordRelease() {
    m_buttonDown = false;
    pruneClicks();
}

std::size_t ModState::clicksPerSecond() {
    pruneClicks();
    return m_clicks.size();
}

double ModState::sessionSeconds() const {
    return std::chrono::duration<double>(now() - m_launchTime).count();
}

} // namespace vertex
