#include "ImGuiLayer.hpp"
#include "Menu.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/CCTouchDispatcher.hpp>
#ifndef GEODE_IS_IOS
#include <Geode/modify/CCMouseDispatcher.hpp>
#endif

#include <array>
#include <cstdint>
#include <type_traits>

using namespace geode::prelude;
using namespace cocos2d;

namespace vertex {

namespace {
    ImVec2 toImGui(CCPoint point) {
        auto* director = CCDirector::sharedDirector();
        auto win = director->getWinSize();
        auto size = ImGui::GetIO().DisplaySize;
        return ImVec2(
            point.x / win.width * size.x,
            (1.f - point.y / win.height) * size.y
        );
    }

    CCPoint toCocos(ImVec2 point) {
        auto* director = CCDirector::sharedDirector();
        auto win = director->getWinSize();
        auto size = ImGui::GetIO().DisplaySize;
        return CCPoint(
            point.x / size.x * win.width,
            (1.f - point.y / size.y) * win.height
        );
    }

    ImGuiKey cocosKey(enumKeyCodes key) {
        if (key >= KEY_A && key <= KEY_Z) {
            return static_cast<ImGuiKey>(ImGuiKey_A + (key - KEY_A));
        }
        if (key >= KEY_Zero && key <= KEY_Nine) {
            return static_cast<ImGuiKey>(ImGuiKey_0 + (key - KEY_Zero));
        }
        switch (key) {
            case KEY_Tab:       return ImGuiKey_Tab;
            case KEY_Up:        return ImGuiKey_UpArrow;
            case KEY_Down:      return ImGuiKey_DownArrow;
            case KEY_Left:      return ImGuiKey_LeftArrow;
            case KEY_Right:     return ImGuiKey_RightArrow;
            case KEY_Enter:     return ImGuiKey_Enter;
            case KEY_Escape:    return ImGuiKey_Escape;
            case KEY_Backspace: return ImGuiKey_Backspace;
            case KEY_Delete:    return ImGuiKey_Delete;
            case KEY_Space:     return ImGuiKey_Space;
            default:            return ImGuiKey_None;
        }
    }

    ImTextureID textureID(GLuint name) {
        if constexpr (std::is_pointer_v<ImTextureID>) {
            return reinterpret_cast<ImTextureID>(static_cast<std::uintptr_t>(name));
        } else {
            return static_cast<ImTextureID>(name);
        }
    }

    GLuint textureName(ImTextureID texture) {
        if constexpr (std::is_pointer_v<ImTextureID>) {
            return static_cast<GLuint>(reinterpret_cast<std::uintptr_t>(texture));
        } else {
            return static_cast<GLuint>(texture);
        }
    }

    void drawTriangle(ImDrawVert const& a, ImDrawVert const& b, ImDrawVert const& c) {
        auto shader = CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor);
        shader->use();
        shader->setUniformsForBuiltins();

        std::array<CCPoint, 3> positions = {
            toCocos(a.pos), toCocos(b.pos), toCocos(c.pos)
        };
        auto color = [](ImDrawVert const& vertex) {
            auto value = ImGui::ColorConvertU32ToFloat4(vertex.col);
            return ccc4f(value.x, value.y, value.z, value.w);
        };
        std::array<ccColor4F, 3> colors = { color(a), color(b), color(c) };
        std::array<ccTex2F, 3> uvs = {
            ccTex2F{a.uv.x, a.uv.y},
            ccTex2F{b.uv.x, b.uv.y},
            ccTex2F{c.uv.x, c.uv.y}
        };

        ccGLEnableVertexAttribs(kCCVertexAttribFlag_PosColorTex);
        glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, positions.data());
        glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_FLOAT, GL_FALSE, 0, colors.data());
        glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, 0, uvs.data());
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

ImGuiLayer& ImGuiLayer::get() {
    static ImGuiLayer instance;
    return instance;
}

void ImGuiLayer::initialize() {
    if (m_initialized) {
        return;
    }

    m_context = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_context);
    ImGui::StyleColorsDark();

    auto& io = ImGui::GetIO();
    io.BackendPlatformName = "vertex-cocos2d-platform";
    io.BackendRendererName = "vertex-cocos2d-renderer";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

    // Configure the menu, including its font, before baking the atlas.  The
    // texture ID is the native Cocos texture name.  The renderer below
    // deliberately uses the same position/texture/color shader that GD 2.2
    // uses for CCSprite, avoiding a second shader pipeline in the game.
    Menu::get().initialize();

    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    m_fontTexture = new CCTexture2D();
    if (m_fontTexture->initWithData(
        pixels,
        kCCTexture2DPixelFormat_RGBA8888,
        width,
        height,
        CCSize(width, height)
    )) {
        m_fontTexture->retain();
        io.Fonts->SetTexID(textureID(m_fontTexture->getName()));
    }

    m_initialized = true;
}

void ImGuiLayer::shutdown() {
    if (!m_initialized) {
        return;
    }

    Menu::get().shutdown();
    ImGui::SetCurrentContext(m_context);
    ImGui::DestroyContext(m_context);
    m_context = nullptr;
    if (m_fontTexture) {
        m_fontTexture->release();
        m_fontTexture = nullptr;
    }
    m_initialized = false;
}

void ImGuiLayer::beginFrame() {
    ImGui::SetCurrentContext(m_context);
    auto* director = CCDirector::sharedDirector();
    auto win = director->getWinSize();
    auto frame = director->getOpenGLView()->getFrameSize();
    if (frame.width <= 0.f || frame.height <= 0.f) {
        frame = win;
    }

    auto& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(frame.width, frame.height);
    io.DisplayFramebufferScale = ImVec2(
        win.width / frame.width,
        win.height / frame.height
    );
    float dt = director->getDeltaTime();
    io.DeltaTime = dt > 0.f ? dt : 1.f / 60.f;
#ifndef GEODE_IS_MOBILE
    auto mouse = geode::cocos::getMousePos();
    auto mousePosition = toImGui(mouse);
    io.AddMousePosEvent(mousePosition.x, mousePosition.y);
#endif
    ImGui::NewFrame();
}

bool ImGuiLayer::wantsMouse() const {
    return m_initialized && Menu::get().visible() && ImGui::GetIO().WantCaptureMouse;
}

void ImGuiLayer::onTouch(float x, float y, unsigned int type) {
    if (!m_initialized) {
        return;
    }
    auto position = toImGui(CCPoint(x, y));
    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(position.x, position.y);
    if (type == CCTOUCHBEGAN) {
        io.AddMouseButtonEvent(0, true);
    } else if (type == CCTOUCHENDED || type == CCTOUCHCANCELLED) {
        io.AddMouseButtonEvent(0, false);
    }
}

void ImGuiLayer::onScroll(float x, float y) {
    if (!m_initialized) {
        return;
    }
    ImGui::GetIO().AddMouseWheelEvent(x / 10.f, -y / 10.f);
}

void ImGuiLayer::onKeyboard(int key, bool down) {
    if (!m_initialized) {
        return;
    }
    auto cocos = static_cast<enumKeyCodes>(key);
    auto imKey = cocosKey(cocos);
    if (imKey != ImGuiKey_None) {
        ImGui::GetIO().AddKeyEvent(imKey, down);
    }
}

void ImGuiLayer::renderDrawDataFallback(ImDrawData* drawData) {
    if (!drawData || drawData->CmdListsCount == 0) {
        return;
    }

    glEnable(GL_BLEND);
    ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_SCISSOR_TEST);

    for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex) {
        auto* list = drawData->CmdLists[listIndex];
        for (auto const& command : list->CmdBuffer) {
            if (command.UserCallback) {
                command.UserCallback(list, &command);
                continue;
            }

            auto texture = textureName(command.GetTexID());
            ccGLBindTexture2D(texture);
            auto clip = command.ClipRect;
            auto origin = toCocos(ImVec2(clip.x, clip.y));
            auto end = toCocos(ImVec2(clip.z, clip.w));
            auto* view = CCDirector::sharedDirector()->getOpenGLView();
            if (end.x <= origin.x || end.y >= origin.y) {
                continue;
            }
            view->setScissorInPoints(
                origin.x,
                end.y,
                end.x - origin.x,
                origin.y - end.y
            );

            for (unsigned int i = 0; i + 2 < command.ElemCount; i += 3) {
                auto ia = list->IdxBuffer[command.IdxOffset + i];
                auto ib = list->IdxBuffer[command.IdxOffset + i + 1];
                auto ic = list->IdxBuffer[command.IdxOffset + i + 2];
                drawTriangle(list->VtxBuffer[ia], list->VtxBuffer[ib], list->VtxBuffer[ic]);
            }
        }
    }

    glDisable(GL_SCISSOR_TEST);
}

void ImGuiLayer::renderDrawData(ImDrawData* drawData) {
    // A per-triangle path is intentionally used here.  It is slower than a
    // VBO, but it works on GD's desktop OpenGL and GLES 2 renderer without
    // assuming VAO support.  The panel contains very few vertices in practice.
    renderDrawDataFallback(drawData);
}

void ImGuiLayer::render() {
    if (!m_initialized) {
        return;
    }
    beginFrame();
    Menu::get().draw();
    ImGui::Render();
    renderDrawData(ImGui::GetDrawData());
}

class $modify(VertexDirectorRenderHook, CCDirector) {
    void drawScene() {
        // drawScene is the cross-platform Cocos2d frame boundary and is called
        // by the director before the platform swaps its back buffer. Rendering
        // here keeps the panels above PlayLayer, UILayer, and transitions.
        CCDirector::drawScene();
        ImGuiLayer::get().render();
    }
};

class $modify(VertexTouchHook, CCTouchDispatcher) {
    void touches(CCSet* touches, CCEvent* event, unsigned int type) {
        auto* touch = touches ? static_cast<CCTouch*>(touches->anyObject()) : nullptr;
        if (touch) {
            auto location = touch->getLocation();
            ImGuiLayer::get().onTouch(location.x, location.y, type);
        }

        // ImGui gets first refusal after it has evaluated the previous frame.
        // This lets dragging a panel stop the same touch from reaching GD.
        if (ImGuiLayer::get().wantsMouse()) {
            return;
        }
        CCTouchDispatcher::touches(touches, event, type);
    }
};

#ifndef GEODE_IS_IOS
class $modify(VertexMouseHook, CCMouseDispatcher) {
    bool dispatchScrollMSG(float y, float x) {
        ImGuiLayer::get().onScroll(x, y);
        if (ImGuiLayer::get().wantsMouse()) {
            return true;
        }
        return CCMouseDispatcher::dispatchScrollMSG(y, x);
    }
};
#endif

$on_mod(Loaded) {
    ImGuiLayer::get().initialize();

    KeyboardInputEvent().listen([](KeyboardInputData& data) {
        bool isPress = data.action == KeyboardInputData::Action::Press;
        bool isDown = data.action != KeyboardInputData::Action::Release;
        ImGuiLayer::get().onKeyboard(static_cast<int>(data.key), isDown);

        if (Menu::get().onKey(static_cast<int>(data.key), isPress)) {
            return ListenerResult::Stop;
        }
        if (ImGuiLayer::get().isInitialized() && ImGui::GetIO().WantCaptureKeyboard) {
            return ListenerResult::Stop;
        }
        return ListenerResult::Propagate;
    }).leak();
}

} // namespace vertex
