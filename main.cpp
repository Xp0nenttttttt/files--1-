#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "RankingLayer.hpp"
#include "AdminLayer.hpp"

using namespace geode::prelude;

// ═══════════════════════════════════════════════════════════════════
//  HOOK ProfilePage — titre sous le nom du joueur
// ═══════════════════════════════════════════════════════════════════
class $modify(ProfilePage) {

    void loadPageFromUserInfo(GJUserScore* score) {
        ProfilePage::loadPageFromUserInfo(score);
        if (!Mod::get()->getSettingValue<bool>("show-titles")) return;

        // Si données pas encore chargées, on attend le fetch
        if (!TitleManager::get()->isLoaded()) return;

        auto titleOpt = TitleManager::get()->getTitleForAccount(score->m_accountID);
        if (!titleOpt) return;
        auto& pt = *titleOpt;

        auto mainLayer = this->m_mainLayer;
        CCLabelBMFont* nameLbl = typeinfo_cast<CCLabelBMFont*>(
            mainLayer ? mainLayer->getChildByID("username-label") : nullptr);

        float baseY = nameLbl ? nameLbl->getPositionY() - 20.f : 195.f;
        auto ws = CCDirector::sharedDirector()->getWinSize();
        float cx = ws.width / 2;

        // Badge titre
        auto titleLabel = CCLabelBMFont::create(pt.title.c_str(), "chatFont.fnt");
        titleLabel->setScale(0.65f);
        titleLabel->setColor(TitleManager::colorFromEntry(pt.colorHex));

        auto badge = CCScale9Sprite::create("square02b_001.png");
        badge->setOpacity(160); badge->setColor({0,0,0});
        badge->setContentSize({
            titleLabel->getContentSize().width * 0.65f + 16,
            titleLabel->getContentSize().height * 0.65f + 10});

        badge->setPosition({cx, baseY});
        titleLabel->setPosition({cx, baseY});

        auto target = mainLayer ? mainLayer : (CCNode*)this;
        target->addChild(badge, 10);
        target->addChild(titleLabel, 11);

        if (!pt.team.empty()) {
            auto teamLbl = CCLabelBMFont::create(
                fmt::format("[{}]", pt.team).c_str(), "chatFont.fnt");
            teamLbl->setScale(0.45f);
            teamLbl->setColor({180,180,180});
            teamLbl->setPosition({cx, baseY - 17.f});
            target->addChild(teamLbl, 11);
        }
    }
};

// ═══════════════════════════════════════════════════════════════════
//  HOOK MenuLayer — bouton classement + chargement distant
// ═══════════════════════════════════════════════════════════════════
class $modify(MenuLayer) {

    bool init() {
        if (!MenuLayer::init()) return false;

        // Bouton classement
        if (Mod::get()->getSettingValue<bool>("show-ranking-button")) {
            auto icon = CCSprite::createWithSpriteFrameName("GJ_trophy_001.png");
            float sc = icon ? 30.f / icon->getContentSize().width : 1.f;
            if (icon) icon->setScale(sc);

            auto lbl = CCLabelBMFont::create("Classement", "bigFont.fnt");
            lbl->setScale(0.38f);

            auto btnNode = CCNode::create();
            btnNode->setContentSize({56,56});
            auto bg = CCScale9Sprite::create("GJ_button_02.png");
            bg->setContentSize({56,56}); bg->setPosition({28,28});
            btnNode->addChild(bg,0);
            if (icon) { icon->setPosition({28,34}); btnNode->addChild(icon,1); }
            lbl->setPosition({28,10}); btnNode->addChild(lbl,1);

            auto btn = CCMenuItemSpriteExtra::create(
                btnNode, this, menu_selector(ModifiedMenuLayer::onOpenRanking));

            auto menu = CCMenu::create();
            menu->addChild(btn);
            menu->setPosition({46,46});
            btn->setPosition(CCPointZero);
            this->addChild(menu,10);
        }

        return true;
    }

    void onOpenRanking(CCObject*) {
        // Affiche le classement, reload depuis le serveur à chaque ouverture
        TitleManager::get()->loadFromRemote([](bool ok) {
            Loader::get()->queueInMainThread([] {
                auto layer = RankingLayer::create();
                if (layer)
                    CCDirector::sharedDirector()
                        ->getRunningScene()->addChild(layer,200);
            });
        });
    }
};

// ═══════════════════════════════════════════════════════════════════
//  Chargement initial
// ═══════════════════════════════════════════════════════════════════
$on_mod(Loaded) {
    log::info("Player Titles & Team Rankings chargé — fetch serveur...");
    TitleManager::get()->loadFromRemote([](bool ok) {
        log::info("Données {} depuis le serveur.",
                  ok ? "chargées" : "en cache (serveur indisponible)");
    });
}
