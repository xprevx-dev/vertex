#include "Menu.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <algorithm>
#include <cmath>
#include <filesystem>

using namespace geode::prelude;

namespace vertex {

namespace {
    const ImVec4 matte = ImVec4(0.094f, 0.094f, 0.094f, 0.90f);       // #181818
    const ImVec4 border = ImVec4(0.165f, 0.165f, 0.165f, 1.00f);      // #2A2A2A
    const ImVec4 surface = ImVec4(0.145f, 0.145f, 0.145f, 1.00f);     // #252525
    const ImVec4 surfaceHover = ImVec4(0.188f, 0.188f, 0.188f, 1.00f);// #303030
    const ImVec4 accent = ImVec4(0.290f, 0.565f, 0.886f, 1.00f);      // #4A90E2
    const ImVec4 text = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    const ImVec4 secondaryText = ImVec4(0.58f, 0.58f, 0.58f, 1.00f);

    ImVec4 lerp(ImVec4 const& a, ImVec4 const& b, float t) {
        t = std::clamp(t, 0.f, 1.f);
        return ImVec4(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t
        );
    }
}

Menu& Menu::get() {
    static Menu instance;
    return instance;
}

void Menu::initialize() {
    if (m_initialized) {
        return;
    }

    // ImGuiLayer owns the context.  This method is intentionally safe to call
    // after that context has been created and before the first NewFrame call.
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // Geode's save directory is used by the backend.

    // The project ships a small, neutral sans-serif font.  The default font is
    // retained as a fallback so a missing resource can never crash a hook.
    auto fontPath = Mod::get()->getResourcesDir() / "DejaVuSans.ttf";
    if (std::filesystem::exists(fontPath)) {
        auto path = fontPath.string();
        if (io.Fonts->AddFontFromFileTTF(path.c_str(), 17.f) == nullptr) {
            io.Fonts->AddFontDefault();
        }
    } else {
        io.Fonts->AddFontDefault();
    }

    applyStyle();
    m_initialized = true;
}

void Menu::shutdown() {
    m_initialized = false;
    m_targetVisible = false;
    m_animation = 0.f;
    m_hoverBlend.clear();
}

void Menu::toggle() {
    m_targetVisible = !m_targetVisible;
}

void Menu::close() {
    m_targetVisible = false;
}

bool Menu::onKey(int key, bool pressed) {
    if (!pressed) {
        return false;
    }

    // KEY_Tab is kept out of this header so the UI layer remains a plain ImGui
    // component.  The input bridge passes cocos2d::KEY_Tab as an integer.
    if (key == static_cast<int>(cocos2d::KEY_Tab)) {
        toggle();
        return true;
    }

    if (key == static_cast<int>(cocos2d::KEY_Escape) && m_targetVisible) {
        close();
        return true;
    }

    return false;
}

float Menu::easeOutCubic(float t) {
    t = std::clamp(t, 0.f, 1.f);
    float inverse = 1.f - t;
    return 1.f - inverse * inverse * inverse;
}

void Menu::applyStyle() {
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 6.f;
    style.ChildRounding = 6.f;
    style.FrameRounding = 6.f;
    style.PopupRounding = 6.f;
    style.ScrollbarRounding = 6.f;
    style.GrabRounding = 6.f;
    style.WindowBorderSize = 1.f;
    style.ChildBorderSize = 1.f;
    style.FrameBorderSize = 1.f;
    style.WindowPadding = ImVec2(14.f, 12.f);
    style.FramePadding = ImVec2(9.f, 6.f);
    style.ItemSpacing = ImVec2(9.f, 8.f);
    style.ItemInnerSpacing = ImVec2(8.f, 5.f);
    style.ScrollbarSize = 12.f;

    auto* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = matte;
    colors[ImGuiCol_ChildBg] = ImVec4(0.094f, 0.094f, 0.094f, 0.70f);
    colors[ImGuiCol_PopupBg] = matte;
    colors[ImGuiCol_Border] = border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = secondaryText;
    colors[ImGuiCol_FrameBg] = surface;
    colors[ImGuiCol_FrameBgHovered] = surfaceHover;
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.21f, 0.21f, 0.21f, 1.f);
    colors[ImGuiCol_Button] = surface;
    colors[ImGuiCol_ButtonHovered] = surfaceHover;
    colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.f);
    colors[ImGuiCol_Header] = surface;
    colors[ImGuiCol_HeaderHovered] = surfaceHover;
    colors[ImGuiCol_HeaderActive] = ImVec4(0.21f, 0.21f, 0.21f, 1.f);
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.36f, 0.64f, 0.93f, 1.f);
    colors[ImGuiCol_Separator] = border;
    colors[ImGuiCol_SeparatorHovered] = surfaceHover;
    colors[ImGuiCol_SeparatorActive] = accent;

    // A platform can replace the font after the first frame; leave the style
    // independent from the font backend and let ImGui handle DPI scaling.
    style.ScaleAllSizes(1.f);
}

bool Menu::drawToggle(char const* label, Feature feature) {
    auto& state = ModState::get();
    bool value = state.enabled(feature);

    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    float width = 38.f;
    float height = 20.f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - width);
    ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##toggle", ImVec2(width, height));
    ImVec2 max = ImGui::GetItemRectMax();

    ImGuiID id = ImGui::GetID("##toggle");
    float& hover = m_hoverBlend[id];
    float delta = ImGui::GetIO().DeltaTime > 0.f ? ImGui::GetIO().DeltaTime : 1.f / 60.f;
    float goal = ImGui::IsItemHovered() ? 1.f : 0.f;
    hover += (goal - hover) * std::min(1.f, delta / 0.10f);

    if (ImGui::IsItemClicked()) {
        value = !value;
        state.setEnabled(feature, value);
    }

    auto* draw = ImGui::GetWindowDrawList();
    ImVec4 track = value
        ? lerp(accent, ImVec4(0.36f, 0.64f, 0.93f, 1.f), hover)
        : lerp(surface, surfaceHover, hover);
    draw->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(track), height * 0.5f);
    float radius = height * 0.5f - 3.f;
    float knobX = value ? max.x - height * 0.5f : min.x + height * 0.5f;
    draw->AddCircleFilled(ImVec2(knobX, (min.y + max.y) * 0.5f), radius, IM_COL32(242, 242, 242, 255));

    ImGui::PopID();
    return value;
}

bool Menu::drawActionButton(char const* label, bool enabled) {
    ImGui::BeginDisabled(!enabled);
    bool clicked = ImGui::Button(label, ImVec2(-1.f, 0.f));
    ImGui::EndDisabled();
    return clicked;
}

void Menu::beginPanel(char const* title, ImVec2 firstPosition, ImVec2 baseSize, float scale) {
    ImGui::SetNextWindowPos(firstPosition, ImGuiCond_FirstUseEver);
    if (m_animation < 0.999f) {
        ImGui::SetNextWindowSize(ImVec2(baseSize.x * scale, baseSize.y * scale), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(baseSize, ImGuiCond_FirstUseEver);
    }
    ImGui::SetNextWindowBgAlpha(0.90f * easeOutCubic(m_animation));
    ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoCollapse);
}

void Menu::endPanel() {
    ImGui::End();
}

void Menu::drawPlayerWindow(float scale) {
    beginPanel("Player", ImVec2(24.f, 24.f), ImVec2(300.f, 280.f), scale);
    ImGui::TextDisabled("RUN CONTROL");
    ImGui::Separator();
    drawToggle("Noclip", Feature::Noclip);
    drawToggle("Practice music", Feature::PracticeMusic);
    drawToggle("Ignore ESC", Feature::IgnoreEsc);
    drawToggle("Instant respawn", Feature::InstantRespawn);
    ImGui::Spacing();
    ImGui::TextWrapped("Hooks PlayLayer and the 2.2 collision pass. Changes are saved by Geode.");
    endPanel();
}

void Menu::drawBypassWindow(float scale) {
    beginPanel("Bypass", ImVec2(340.f, 24.f), ImVec2(320.f, 340.f), scale);
    ImGui::TextDisabled("ENGINE / ACCOUNT");
    ImGui::Separator();
    drawToggle("FPS / TPS bypass", Feature::FpsBypass);
    int tps = ModState::get().targetTPS();
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::SliderInt("Target tick rate", &tps, 60, 1000, "%d TPS")) {
        ModState::get().setTargetTPS(tps);
    }
    drawToggle("Unlock all icons", Feature::UnlockAll);
    drawToggle("Character filter bypass", Feature::CharacterFilter);
    drawToggle("Custom object limit", Feature::CustomObjectBypass);
    ImGui::Spacing();
    ImGui::TextWrapped("The tick hook supplies a fixed delta to the scheduler; it does not alter the display refresh rate.");
    endPanel();
}

void Menu::drawCreatorWindow(float scale) {
    beginPanel("Creator / Editor", ImVec2(24.f, 325.f), ImVec2(300.f, 330.f), scale);
    ImGui::TextDisabled("LEVEL AUTHORING");
    ImGui::Separator();
    drawToggle("Copy levels", Feature::CopyHack);
    drawToggle("Edit server levels", Feature::LevelEdit);
    drawToggle("Extended scale", Feature::ScaleHack);
    float limit = ModState::get().scaleLimit();
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::SliderFloat("Scale limit", &limit, 2.f, 999.f, "%.0f")) {
        ModState::get().setScaleLimit(limit);
    }
    drawToggle("Free scroll", Feature::FreeScroll);
    ImGui::Spacing();
    ImGui::TextWrapped("Editor hooks use the 2.2081 LevelEditorLayer and EditorUI transform paths.");
    endPanel();
}

void Menu::drawVisualsWindow(float scale) {
    beginPanel("Visuals", ImVec2(340.f, 385.f), ImVec2(320.f, 290.f), scale);
    ImGui::TextDisabled("RENDERING");
    ImGui::Separator();
    drawToggle("Show hitboxes", Feature::ShowHitboxes);
    drawToggle("Layout mode", Feature::LayoutMode);
    drawToggle("No particles", Feature::NoParticles);
    drawToggle("Hide game UI", Feature::HideUI);
    ImGui::Spacing();
    ImGui::TextWrapped("Hitboxes are drawn after PlayLayer::draw with the native object rectangles.");
    endPanel();
}

void Menu::drawOverlayWindow(float scale) {
    beginPanel("Overlay", ImVec2(680.f, 24.f), ImVec2(280.f, 260.f), scale);
    ImGui::TextDisabled("DRAGGABLE HUD");
    ImGui::Separator();
    drawToggle("Session time", Feature::SessionTime);
    drawToggle("CPS counter", Feature::CPS);
    drawToggle("Best run", Feature::BestRun);
    ImGui::Spacing();
    ImGui::TextWrapped("HUD elements remain independent from the category panels and can be dragged.");
    if (drawActionButton("Reset current best")) {
        ModState::get().beginLevel();
    }
    endPanel();
}

void Menu::drawHUD() {
    auto& state = ModState::get();
    if (!state.enabled(Feature::SessionTime) && !state.enabled(Feature::CPS) && !state.enabled(Feature::BestRun)) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(20.f, 700.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::Begin("##vertex-hud", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoCollapse
    );
    if (state.enabled(Feature::SessionTime)) {
        ImGui::Text("Session  %06.1fs", state.sessionSeconds());
    }
    if (state.enabled(Feature::CPS)) {
        ImGui::Text("CPS      %zu", state.clicksPerSecond());
    }
    if (state.enabled(Feature::BestRun)) {
        ImGui::Text("Best run  %05.1f%%", state.bestPercent());
    }
    ImGui::End();
}

void Menu::draw() {
    if (!m_initialized) {
        return;
    }

    float dt = ImGui::GetIO().DeltaTime;
    if (dt <= 0.f || dt > 0.2f) {
        dt = 1.f / 60.f;
    }
    float direction = m_targetVisible ? 1.f : -1.f;
    m_animation = std::clamp(m_animation + direction * (dt / 0.15f), 0.f, 1.f);
    if (!m_targetVisible && m_animation <= 0.f) {
        return;
    }

    applyStyle();
    float alpha = easeOutCubic(m_animation);
    float scale = 0.95f + 0.05f * alpha;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    drawPlayerWindow(scale);
    drawBypassWindow(scale);
    drawCreatorWindow(scale);
    drawVisualsWindow(scale);
    drawOverlayWindow(scale);
    drawHUD();
    ImGui::PopStyleVar();
}

} // namespace vertex
