#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

// Central place for all Vertex feature toggle state. Add one bool per feature as we go.
struct VertexState {
    static inline bool noclip = false;
};

class VertexMenuPopup : public CCLayer {
protected:
    bool init() {
        if (!CCLayer::init()) {
            return false;
        }

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto bg = CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({ 240.f, 160.f });
        bg->setPosition(winSize.width / 2, winSize.height / 2);
        this->addChild(bg);

        auto title = CCLabelBMFont::create("Vertex", "bigFont.fnt");
        title->setScale(0.6f);
        title->setPosition(winSize.width / 2, winSize.height / 2 + 60.f);
        this->addChild(title);

        auto menu = CCMenu::create();
        menu->setPosition(winSize.width / 2, winSize.height / 2);
        this->addChild(menu);

        auto noclipToggle = CCMenuItemToggler::createWithStandardSprites(
            this,
            menu_selector(VertexMenuPopup::onNoclip),
            0.7f
        );
        noclipToggle->toggle(VertexState::noclip);
        noclipToggle->setPosition(-70.f, 0.f);
        menu->addChild(noclipToggle);

        auto label = CCLabelBMFont::create("Noclip", "bigFont.fnt");
        label->setScale(0.4f);
        label->setAnchorPoint({ 0.f, 0.5f });
        label->setPosition(-45.f, 0.f);
        menu->addChild(label);

        auto closeSprite = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        auto closeBtn = CCMenuItemSpriteExtra::create(
            closeSprite,
            this,
            menu_selector(VertexMenuPopup::onClose)
        );
        closeBtn->setPosition(-110.f, 70.f);
        menu->addChild(closeBtn);

        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        return true;
    }

    void onNoclip(CCObject* sender) {
        auto toggler = static_cast<CCMenuItemToggler*>(sender);
        VertexState::noclip = toggler->isToggled();
    }

    void onClose(CCObject*) {
        this->removeFromParentAndCleanup(true);
    }

    void keyBackClicked() {
        this->removeFromParentAndCleanup(true);
    }

public:
    static VertexMenuPopup* create() {
        auto ret = new VertexMenuPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    static void show() {
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (!scene) {
            return;
        }
        auto popup = VertexMenuPopup::create();
        popup->setZOrder(1000);
        scene->addChild(popup);
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
        VertexMenuPopup::show();
    }
};

// First real feature: noclip. Prevents the death sequence from firing while enabled.
class $modify(PlayerObject) {
    void playerDestroyed(bool noEffects) {
        if (VertexState::noclip) {
            return;
        }
        PlayerObject::playerDestroyed(noEffects);
    }
};
