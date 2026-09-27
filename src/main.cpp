#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

// Central place for all Vertex feature toggle state. Add one bool per feature as we go.
struct VertexState {
    static inline bool noclip = false;
};

class VertexMenuPopup : public geode::Popup<> {
protected:
    bool setup() override {
        this->setTitle("Vertex");

        auto noclipToggle = CCMenuItemToggler::createWithStandardSprites(
            this,
            menu_selector(VertexMenuPopup::onNoclip),
            0.7f
        );
        noclipToggle->toggle(VertexState::noclip);
        noclipToggle->setPosition(-70.f, 0.f);
        m_buttonMenu->addChild(noclipToggle);

        auto label = CCLabelBMFont::create("Noclip", "bigFont.fnt");
        label->setScale(0.4f);
        label->setAnchorPoint({ 0.f, 0.5f });
        label->setPosition(-45.f, 0.f);
        m_mainLayer->addChild(label);

        return true;
    }

    void onNoclip(CCObject* sender) {
        auto toggler = static_cast<CCMenuItemToggler*>(sender);
        VertexState::noclip = toggler->isToggled();
    }

public:
    static VertexMenuPopup* create() {
        auto ret = new VertexMenuPopup();
        if (ret->initAnchored(240.f, 160.f)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

class $modify(PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto menu = this->getChildByID("left-button-menu");
        if (!menu) return;

        auto sprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        sprite->setScale(0.9f);

        auto btn = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(PauseLayer::onVertexMenu)
        );
        btn->setID("vertex-menu-button"_spr);
        menu->addChild(btn);
        menu->updateLayout();
    }

    void onVertexMenu(CCObject*) {
        VertexMenuPopup::create()->show();
    }
};

// First real feature: noclip. Skips all player collision resolution while enabled.
class $modify(PlayerObject) {
    bool checkCollisions(GJBaseGameLayer* layer, float dt) {
        if (VertexState::noclip) {
            return true;
        }
        return PlayerObject::checkCollisions(layer, dt);
    }
};
