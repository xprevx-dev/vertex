#pragma once

#include <imgui.h>

namespace cocos2d { class CCTexture2D; }

namespace vertex {

/**
 * Small Cocos2d/OpenGL backend for Dear ImGui.  Geode does not provide a
 * universal ImGui renderer in the base SDK, so this layer owns the context,
 * font atlas, frame lifecycle, and the Cocos scene hook.
 */
class ImGuiLayer final {
public:
    static ImGuiLayer& get();

    void initialize();
    void shutdown();
    void render();

    void onTouch(float x, float y, unsigned int type);
    void onScroll(float x, float y);
    void onKeyboard(int key, bool down);

    bool isInitialized() const { return m_initialized; }
    bool wantsMouse() const;

private:
    ImGuiLayer() = default;
    ImGuiLayer(ImGuiLayer const&) = delete;
    ImGuiLayer& operator=(ImGuiLayer const&) = delete;

    void beginFrame();
    void renderDrawData(ImDrawData* drawData);
    void renderDrawDataFallback(ImDrawData* drawData);

    ImGuiContext* m_context = nullptr;
    cocos2d::CCTexture2D* m_fontTexture = nullptr;
    bool m_initialized = false;
};

} // namespace vertex
