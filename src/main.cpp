/**
 * Vertex entry point and the first, high-risk hooks.
 *
 * This source targets the Geode 2.2.0 / GD 2.2081 binding set.  The numeric
 * addresses shown in the comments are the Windows addresses from the public
 * Geode bindings; Geode resolves the platform-specific address at load time,
 * so this mod never hard-codes an RVA in its own binary.
 */
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "ModState.hpp"

using namespace geode::prelude;
using namespace cocos2d;

namespace vertex {

namespace {
    bool isHazard(GameObject* object) {
        if (!object) {
            return false;
        }
        auto type = object->getType();
        return type == GameObjectType::Hazard ||
               type == GameObjectType::AnimatedHazard ||
               (type == GameObjectType::Slope && object->m_slopeIsHazard);
    }

    void drawBox(CCRect rect, ccColor4B outline, ccColor4F fill) {
        CCPoint points[4] = {
            ccp(rect.getMinX(), rect.getMinY()),
            ccp(rect.getMaxX(), rect.getMinY()),
            ccp(rect.getMaxX(), rect.getMaxY()),
            ccp(rect.getMinX(), rect.getMaxY()),
        };

        // Cocos2d's immediate primitives are used after PlayLayer::draw, so
        // the overlay remains in the same world transform as gameplay.  The
        // translucent fill is drawn first and the one-pixel outline second.
        ccDrawSolidPoly(points, 4, fill);
        ccDrawColor4B(outline.r, outline.g, outline.b, outline.a);
        ccDrawPoly(points, 4, true);
    }

    void drawHitboxes(PlayLayer* layer) {
        if (!layer || !ModState::get().enabled(Feature::ShowHitboxes)) {
            return;
        }

        if (layer->m_player1) {
            drawBox(
                layer->m_player1->getBoundingBox(),
                ccc4(67, 157, 255, 255),
                ccc4f(0.26f, 0.62f, 1.f, 0.16f)
            );
        }
        if (layer->m_player2) {
            drawBox(
                layer->m_player2->getBoundingBox(),
                ccc4(67, 157, 255, 255),
                ccc4f(0.26f, 0.62f, 1.f, 0.16f)
            );
        }

        // In 2.2 the gameplay object list is GJBaseGameLayer::m_objects.
        // GameObject::getBoundingBox() includes the current object transform,
        // while getType() distinguishes the hazard/solid outline colors.
        CCObject* raw = nullptr;
        if (!layer->m_objects) {
            return;
        }
        CCARRAY_FOREACH(layer->m_objects, raw) {
            auto* object = static_cast<GameObject*>(raw);
            if (!object || object->m_isInvisible || object->isTrigger()) {
                continue;
            }
            if (isHazard(object)) {
                drawBox(
                    object->getBoundingBox(),
                    ccc4(225, 92, 92, 255),
                    ccc4f(0.88f, 0.36f, 0.36f, 0.10f)
                );
            } else if (object->getType() == GameObjectType::Solid ||
                       object->getType() == GameObjectType::Breakable ||
                       object->getType() == GameObjectType::Slope) {
                drawBox(
                    object->getBoundingBox(),
                    ccc4(210, 210, 210, 230),
                    ccc4f(0.82f, 0.82f, 0.82f, 0.035f)
                );
            }
        }
    }
}

/**
 * Player hooks.  In the 2.2081 bindings:
 *  - PlayLayer::destroyPlayer(PlayerObject*, GameObject*) is the virtual at
 *    Windows 0x3B39D0.
 *  - GJBaseGameLayer::checkCollisions(PlayerObject*, float, bool) is the
 *    collision pass at Windows 0x2137F0. Older examples call this method
 *    collisionChecks; the 2.2 binding name is checkCollisions.
 *  - PlayLayer::toggleMusicInPractice() is Windows 0x3B2900.
 *  - PlayLayer::resetLevel() is Windows 0x3B8EB0.
 *  - PlayLayer::keyDown(enumKeyCodes, double) is Windows 0x3CBE40.
 *
 * Using the generated signatures is safer than copying these addresses: the
 * same class resolves to different addresses on macOS, Android, and iOS.
 */
class $modify(VertexPlayLayerPlayerHooks, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }
        ModState::get().beginLevel();
        return true;
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto& state = ModState::get();
        if (state.enabled(Feature::Noclip)) {
            // Skipping this virtual prevents the death transition, particles,
            // and delayed reset from being scheduled for the active player.
            if (player) {
                player->resetStateVariables();
            }
            return;
        }

        if (state.enabled(Feature::InstantRespawn)) {
            // The normal implementation starts a death animation and later
            // invokes resetLevel. Calling the bound reset path directly removes
            // that delay while retaining GD's own checkpoint bookkeeping.
            this->resetLevel();
            return;
        }

        PlayLayer::destroyPlayer(player, object);
    }

    void toggleMusicInPractice() {
        if (ModState::get().enabled(Feature::PracticeMusic)) {
            // This is exactly the 2.2 practice-mode switch point. Returning
            // here leaves the active normal-level FMOD channel untouched.
            return;
        }
        PlayLayer::toggleMusicInPractice();
    }

    void keyDown(enumKeyCodes key, double timestamp) {
        if (ModState::get().enabled(Feature::IgnoreEsc) && key == KEY_Escape) {
            return;
        }
        PlayLayer::keyDown(key, timestamp);
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        if (this->isGameplayActive()) {
            ModState::get().updateBestPercent(this->getCurrentPercent());
        }
    }

    bool pushButton(PlayerButton button) {
        bool result = PlayLayer::pushButton(button);
        if (result) {
            ModState::get().recordPress();
        }
        return result;
    }

    bool releaseButton(PlayerButton button) {
        bool result = PlayLayer::releaseButton(button);
        ModState::get().recordRelease();
        return result;
    }

    void draw() {
        PlayLayer::draw();
        drawHitboxes(this);
    }
};

/**
 * Current GD 2.2 does not expose a PlayLayer::collisionChecks symbol.  The
 * generated class moved the operation into GJBaseGameLayer::checkCollisions.
 * This hook is the equivalent collision early-return and is kept separate so
 * it composes with other PlayLayer hooks above.
 */
class $modify(VertexCollisionHooks, GJBaseGameLayer) {
    int checkCollisions(PlayerObject* player, float dt, bool ignoreDamage) {
        if (ModState::get().enabled(Feature::Noclip)) {
            return 0;
        }
        return GJBaseGameLayer::checkCollisions(player, dt, ignoreDamage);
    }
};

} // namespace vertex
