/**
 * Secondary hooks for the Bypass, Creator, and Visuals panels.
 *
 * Every hook below uses the generated Geode 2.2081 signature.  The addresses
 * in the adjacent comments are Windows 2.2081 references only; Geode chooses
 * the matching macOS/Android/iOS address from the same binding at load time.
 */
#include <Geode/Geode.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/CCTextInputNode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/UILayer.hpp>

#include "ModState.hpp"

#include <algorithm>

using namespace geode::prelude;
using namespace cocos2d;

namespace vertex {

/** Bypass: feed a stable custom delta into the cocos scheduler. */
class $modify(VertexSchedulerHooks, CCScheduler) {
    void update(float dt) {
        if (ModState::get().enabled(Feature::FpsBypass)) {
            // CCScheduler::update is the 2.2 engine entry at Windows 0x443DB0.
            // This changes the game-loop delta consumed by GD's physics and
            // does not claim to create additional monitor refreshes.
            dt = 1.f / static_cast<float>(ModState::get().targetTPS());
        }
        CCScheduler::update(dt);
    }
};

/** Bypass: garage unlock queries. */
class $modify(VertexGameManagerHooks, GameManager) {
    bool isIconUnlocked(int id, IconType type) {
        if (ModState::get().enabled(Feature::UnlockAll)) {
            return true;
        }
        return GameManager::isIconUnlocked(id, type);
    }

    bool isColorUnlocked(int id, UnlockType type) {
        if (ModState::get().enabled(Feature::UnlockAll)) {
            return true;
        }
        return GameManager::isColorUnlocked(id, type);
    }
};

class $modify(VertexStatsHooks, GameStatsManager) {
    bool canItemBeUnlocked(int id, UnlockType type) {
        if (ModState::get().enabled(Feature::UnlockAll)) {
            return true;
        }
        return GameStatsManager::canItemBeUnlocked(id, type);
    }
};

/**
 * Bypass: CCTextInputNode owns both filters in 2.2.  m_allowedChars is the
 * special-character whitelist and m_filterSwearWords drives the profanity
 * check.  Clearing them at the insertion boundary covers profile text,
 * comments, and other fields without patching a process-wide string routine.
 */
class $modify(VertexTextInputHooks, CCTextInputNode) {
    void visit() {
        if (ModState::get().enabled(Feature::CharacterFilter)) {
            m_allowedChars = "";
            m_filterSwearWords = false;
        }
        CCTextInputNode::visit();
    }

    bool onTextFieldInsertText(CCTextFieldTTF* sender, char const* text, int length, enumKeyCodes key) {
        if (!ModState::get().enabled(Feature::CharacterFilter)) {
            return CCTextInputNode::onTextFieldInsertText(sender, text, length, key);
        }

        bool oldSwearFilter = m_filterSwearWords;
        auto oldAllowed = m_allowedChars;
        m_filterSwearWords = false;
        m_allowedChars = "";
        bool result = CCTextInputNode::onTextFieldInsertText(sender, text, length, key);
        m_filterSwearWords = oldSwearFilter;
        m_allowedChars = oldAllowed;
        return result;
    }
};

/**
 * Bypass: LevelEditorLayer::createObjectsFromString has an explicit noLimit
 * parameter in GD 2.2.  LevelEditorLayer::init also carries the authoritative
 * GJGameLevel flags used by the normal editor object counter.
 */
class $modify(VertexEditorBypassHooks, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool noUI) {
        if (!LevelEditorLayer::init(level, noUI)) {
            return false;
        }
        if (ModState::get().enabled(Feature::CustomObjectBypass) && m_level) {
            m_level->m_highObjectsEnabled = true;
            m_level->m_unlimitedObjectsEnabled = true;
        }
        return true;
    }

    void updateOptions() {
        if (ModState::get().enabled(Feature::CustomObjectBypass) && m_level) {
            m_level->m_highObjectsEnabled = true;
            m_level->m_unlimitedObjectsEnabled = true;
        }
        LevelEditorLayer::updateOptions();
    }

    CCArray* createObjectsFromString(gd::string const& string, bool noLimit) {
        if (ModState::get().enabled(Feature::CustomObjectBypass)) {
            noLimit = true;
        }
        return LevelEditorLayer::createObjectsFromString(string, false, noLimit);
    }
};

/** Creator: remove the local copy-password gate before GD evaluates cloning. */
class $modify(VertexLevelInfoHooks, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) {
            return false;
        }

        if (ModState::get().enabled(Feature::CopyHack) && m_level) {
            // GJGameLevel::m_password is the SeedValueRS field read by
            // LevelInfoLayer::tryCloneLevel/confirmClone.  Zero is GD's
            // no-password representation.
            m_level->setPassword(0);
            if (m_cloneBtn) {
                m_cloneBtn->setVisible(true);
                m_cloneBtn->setEnabled(true);
            }
        }

        if (ModState::get().enabled(Feature::LevelEdit)) {
            addEditButton();
        }
        return true;
    }

    void onClone(CCObject* sender) {
        if (ModState::get().enabled(Feature::CopyHack) && m_level) {
            // Keep the value normalized for the complete synchronous GD clone
            // path, including confirmClone called by onClone.
            m_level->setPassword(0);
        }
        LevelInfoLayer::onClone(sender);
    }

private:
    void addEditButton() {
        if (!m_playBtnMenu || m_playBtnMenu->getChildByID("vertex-edit-button")) {
            return;
        }

        auto* sprite = CCSprite::createWithSpriteFrameName("GJ_editBtn_001.png");
        if (!sprite) {
            return;
        }
        auto* button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(VertexLevelInfoHooks::onVertexEdit)
        );
        button->setID("vertex-edit-button");
        m_playBtnMenu->addChild(button);
        m_playBtnMenu->updateLayout();
    }

    void onVertexEdit(CCObject*) {
        if (!m_level) {
            return;
        }
        // LevelEditorLayer::scene is the same scene constructor used by the
        // vanilla hammer button, with noUI=false to retain EditorUI.
        CCDirector::sharedDirector()->replaceScene(
            LevelEditorLayer::scene(m_level, false)
        );
    }
};

/** Creator: allow the EditorUI transform callbacks to receive the extended range. */
class $modify(VertexEditorScaleHooks, EditorUI) {
    void scaleXChanged(float value, bool lock) {
        if (ModState::get().enabled(Feature::ScaleHack)) {
            value = std::clamp(value, -ModState::get().scaleLimit(), ModState::get().scaleLimit());
        }
        EditorUI::scaleXChanged(value, lock);
    }

    void scaleYChanged(float value, bool lock) {
        if (ModState::get().enabled(Feature::ScaleHack)) {
            value = std::clamp(value, -ModState::get().scaleLimit(), ModState::get().scaleLimit());
        }
        EditorUI::scaleYChanged(value, lock);
    }

    void scaleXYChanged(float x, float y, bool lock) {
        if (ModState::get().enabled(Feature::ScaleHack)) {
            auto limit = ModState::get().scaleLimit();
            x = std::clamp(x, -limit, limit);
            y = std::clamp(y, -limit, limit);
        }
        EditorUI::scaleXYChanged(x, y, lock);
    }

    void updateZoom(float zoom) {
        if (ModState::get().enabled(Feature::FreeScroll)) {
            // Vanilla clamps the lower bound near 0.5.  Keep the value finite
            // for matrix math but remove the practical zoom-out limit.
            zoom = std::max(zoom, 0.001f);
        }
        EditorUI::updateZoom(zoom);
    }
};

class $modify(VertexGameObjectTransformHooks, GameObject) {
    void setScale(float scale) {
        if (ModState::get().enabled(Feature::ScaleHack) && LevelEditorLayer::get()) {
            auto limit = ModState::get().scaleLimit();
            scale = std::clamp(scale, -limit, limit);
        }
        GameObject::setScale(scale);
    }

    void setVisible(bool visible) {
        if (ModState::get().enabled(Feature::LayoutMode) &&
            m_objectType == GameObjectType::Decoration) {
            visible = false;
        }
        GameObject::setVisible(visible);
    }

    void setObjectColor(ccColor3B const& color) {
        if (ModState::get().enabled(Feature::LayoutMode) &&
            m_objectType != GameObjectType::Decoration) {
            static const ccColor3B layoutColor = { 150, 150, 150 };
            GameObject::setObjectColor(layoutColor);
            return;
        }
        GameObject::setObjectColor(color);
    }
};

/** Visuals: stop particle emission as soon as a 2.2 quad emitter is created. */
class $modify(VertexParticleHooks, CCParticleSystemQuad) {
    bool initWithTotalParticles(unsigned int count, bool batch) {
        if (!CCParticleSystemQuad::initWithTotalParticles(count, batch)) {
            return false;
        }
        if (ModState::get().enabled(Feature::NoParticles)) {
            stopSystem();
        }
        return true;
    }
};

/** Visuals: UILayer owns pause/progress/attempt widgets in PlayLayer. */
class $modify(VertexUILayerHooks, UILayer) {
    bool init(GJBaseGameLayer* layer) {
        if (!UILayer::init(layer)) {
            return false;
        }
        if (ModState::get().enabled(Feature::HideUI)) {
            setVisible(false);
        }
        return true;
    }

    void draw() {
        if (ModState::get().enabled(Feature::HideUI)) {
            return;
        }
        UILayer::draw();
    }
};

/** Visuals/Creator: bypass the camera-limit correction used by editor teleports. */
class $modify(VertexCameraHooks, GJBaseGameLayer) {
    void checkCameraLimitAfterTeleport(PlayerObject* player, float yOffset) {
        if (ModState::get().enabled(Feature::FreeScroll) && LevelEditorLayer::get()) {
            return;
        }
        GJBaseGameLayer::checkCameraLimitAfterTeleport(player, yOffset);
    }
};

} // namespace vertex
