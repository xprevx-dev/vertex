#pragma once

#include "ModState.hpp"

#include <imgui.h>
#include <unordered_map>

namespace vertex {

/**
 * Dear ImGui front end.  This class never hooks a GD class and never touches
 * game memory directly; it only edits ModState.  Hooks.cpp and main.cpp are the
 * other half of the design.
 */
class Menu final {
public:
    static Menu& get();

    void initialize();
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    bool visible() const { return m_targetVisible; }
    void toggle();
    void close();

    /** Returns true when the key was consumed by the menu. */
    bool onKey(int key, bool pressed);

    /** Called once per rendered frame by ImGuiLayer. */
    void draw();

private:
    Menu() = default;
    Menu(Menu const&) = delete;
    Menu& operator=(Menu const&) = delete;

    static float easeOutCubic(float t);
    void applyStyle();
    void drawPlayerWindow(float scale);
    void drawBypassWindow(float scale);
    void drawCreatorWindow(float scale);
    void drawVisualsWindow(float scale);
    void drawOverlayWindow(float scale);

    bool drawToggle(char const* label, Feature feature);
    bool drawActionButton(char const* label, bool enabled = true);
    void beginPanel(char const* title, ImVec2 firstPosition, ImVec2 baseSize, float scale);
    void endPanel();
    void drawHUD();

    bool m_initialized = false;
    bool m_targetVisible = false;
    float m_animation = 0.f;
    std::unordered_map<ImGuiID, float> m_hoverBlend;
};

} // namespace vertex
