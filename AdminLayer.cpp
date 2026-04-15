#include "AdminLayer.hpp"
#include "RankingLayer.hpp"
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

AdminLayer* AdminLayer::create() {
    auto ret = new AdminLayer();
    if (ret && ret->init()) { ret->autorelease(); return ret; }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool AdminLayer::init() {
    if (!CCLayerColor::initWithColor({0, 0, 0, 210})) return false;
    this->setZOrder(150);

    auto ws = CCDirector::sharedDirector()->getWinSize();
    float cx = ws.width / 2, cy = ws.height / 2;

    // ── Panneau ──
    auto panel = CCScale9Sprite::create("GJ_square01.png");
    panel->setContentSize({460, 320});
    panel->setPosition({cx, cy});
    this->addChild(panel, 1);

    // ── Titre ──
    auto title = CCLabelBMFont::create("Panneau Admin", "goldFont.fnt");
    title->setScale(0.7f);
    title->setPosition({cx, cy + 142});
    this->addChild(title, 2);

    // ── Onglets ──
    auto tabMenu = CCMenu::create();
    tabMenu->setPosition(CCPointZero);

    auto makTab = [&](const char* lbl, SEL_MenuHandler sel, float x) {
        auto bg = CCScale9Sprite::create("GJ_button_04.png");
        bg->setContentSize({100, 28});
        auto txt = CCLabelBMFont::create(lbl, "bigFont.fnt");
        txt->setScale(0.38f);
        txt->setPosition(bg->getContentSize() / 2);
        bg->addChild(txt);
        auto btn = CCMenuItemSpriteExtra::create(bg, this, sel);
        btn->setPosition({x, cy + 115});
        tabMenu->addChild(btn);
    };
    makTab("Titres Joueurs", menu_selector(AdminLayer::onTabTitles), cx - 80);
    makTab("Equipes",        menu_selector(AdminLayer::onTabTeams),  cx + 80);

    // ── Bouton Fermer ──
    auto closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
    closeSpr->setScale(0.85f);
    auto closeBtn = CCMenuItemSpriteExtra::create(
        closeSpr, this, menu_selector(AdminLayer::onClose));
    closeBtn->setPosition({cx + 207, cy + 142});
    tabMenu->addChild(closeBtn);

    this->addChild(tabMenu, 3);

    // ── Zone de contenu ──
    m_content = CCNode::create();
    m_content->setID("admin-content");
    this->addChild(m_content, 2);

    showTitlesTab();
    return true;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

CCTextInputNode* AdminLayer::makeInput(const char* placeholder, float width) {
    auto input = CCTextInputNode::create(width, 28, placeholder, "bigFont.fnt");
    input->setMaxLabelScale(0.5f);
    input->setMaxLabelWidth(width - 10);
    return input;
}

void AdminLayer::clearContent() {
    m_content->removeAllChildren();
    m_inAccountId = m_inTitle = m_inColor = m_inTeamName = nullptr;
    m_inTName = m_inPoints = m_inWins = m_inLosses = m_inTColor = nullptr;
}

void AdminLayer::showFeedback(const std::string& msg, bool success) {
    auto ws = CCDirector::sharedDirector()->getWinSize();
    auto lbl = CCLabelBMFont::create(msg.c_str(), "bigFont.fnt");
    lbl->setScale(0.45f);
    lbl->setColor(success ? ccColor3B{100,255,100} : ccColor3B{255,100,100});
    lbl->setPosition({ws.width / 2, ws.height / 2 - 130});
    lbl->setZOrder(20);
    this->addChild(lbl);
    lbl->runAction(CCSequence::create(
        CCDelayTime::create(2.5f),
        CCFadeOut::create(0.4f),
        CCRemoveSelf::create(),
        nullptr));
}

// ─── Onglet Titres ────────────────────────────────────────────────────────────

void AdminLayer::showTitlesTab() {
    clearContent();
    m_tab = 0;
    auto ws = CCDirector::sharedDirector()->getWinSize();
    float cx = ws.width / 2, cy = ws.height / 2;

    struct Row { const char* label; CCTextInputNode** field; const char* hint; float y; };
    Row rows[] = {
        {"Account ID", &m_inAccountId, "Ex: 123456",          cy + 60},
        {"Titre",      &m_inTitle,     "Ex: Vainqueur Tournoi",cy + 20},
        {"Couleur",    &m_inColor,     "Hex: FFD700",          cy - 20},
        {"Equipe",     &m_inTeamName,  "Ex: Team Alpha",       cy - 60},
    };

    for (auto& r : rows) {
        auto lbl = CCLabelBMFont::create(r.label, "bigFont.fnt");
        lbl->setScale(0.38f);
        lbl->setAnchorPoint({1, 0.5f});
        lbl->setPosition({cx - 60, r.y});
        m_content->addChild(lbl);

        auto inp = makeInput(r.hint, 200);
        *r.field = inp;

        // fond input
        auto bg = CCScale9Sprite::create("square02b_001.png");
        bg->setOpacity(100);
        bg->setContentSize({204, 30});
        bg->setPosition({cx + 45, r.y});
        m_content->addChild(bg);

        inp->setPosition({cx + 45, r.y});
        m_content->addChild(inp);
    }

    // Bouton Ajouter
    auto addBg = CCScale9Sprite::create("GJ_button_01.png");
    addBg->setContentSize({160, 34});
    auto addLbl = CCLabelBMFont::create("Ajouter Titre", "bigFont.fnt");
    addLbl->setScale(0.42f);
    addLbl->setPosition(addBg->getContentSize() / 2);
    addBg->addChild(addLbl);

    auto addMenu = CCMenu::create();
    addMenu->setPosition(CCPointZero);
    auto addBtn = CCMenuItemSpriteExtra::create(
        addBg, this, menu_selector(AdminLayer::onAddTitle));
    addBtn->setPosition({cx, cy - 108});
    addMenu->addChild(addBtn);
    m_content->addChild(addMenu);
}

// ─── Onglet Équipes ───────────────────────────────────────────────────────────

void AdminLayer::showTeamsTab() {
    clearContent();
    m_tab = 1;
    auto ws = CCDirector::sharedDirector()->getWinSize();
    float cx = ws.width / 2, cy = ws.height / 2;

    struct Row { const char* label; CCTextInputNode** field; const char* hint; float y; };
    Row rows[] = {
        {"Nom",      &m_inTName,  "Ex: Team Alpha", cy + 60},
        {"Points",   &m_inPoints, "Ex: 1850",       cy + 20},
        {"Victoires",&m_inWins,   "Ex: 12",         cy - 20},
        {"Defaites", &m_inLosses, "Ex: 2",          cy - 60},
        {"Couleur",  &m_inTColor, "Hex: FFD700",    cy - 100},
    };

    for (auto& r : rows) {
        auto lbl = CCLabelBMFont::create(r.label, "bigFont.fnt");
        lbl->setScale(0.38f);
        lbl->setAnchorPoint({1, 0.5f});
        lbl->setPosition({cx - 60, r.y});
        m_content->addChild(lbl);

        auto inp = makeInput(r.hint, 200);
        *r.field = inp;

        auto bg = CCScale9Sprite::create("square02b_001.png");
        bg->setOpacity(100);
        bg->setContentSize({204, 30});
        bg->setPosition({cx + 45, r.y});
        m_content->addChild(bg);

        inp->setPosition({cx + 45, r.y});
        m_content->addChild(inp);
    }

    auto addBg = CCScale9Sprite::create("GJ_button_01.png");
    addBg->setContentSize({160, 34});
    auto addLbl = CCLabelBMFont::create("Ajouter Equipe", "bigFont.fnt");
    addLbl->setScale(0.42f);
    addLbl->setPosition(addBg->getContentSize() / 2);
    addBg->addChild(addLbl);

    auto addMenu = CCMenu::create();
    addMenu->setPosition(CCPointZero);
    auto addBtn = CCMenuItemSpriteExtra::create(
        addBg, this, menu_selector(AdminLayer::onAddTeam));
    addBtn->setPosition({cx, cy - 138});
    addMenu->addChild(addBtn);
    m_content->addChild(addMenu);
}

// ─── Actions ──────────────────────────────────────────────────────────────────

void AdminLayer::onTabTitles(CCObject*) { showTitlesTab(); }
void AdminLayer::onTabTeams(CCObject*)  { showTeamsTab();  }
void AdminLayer::onClose(CCObject*)     { this->removeFromParentAndCleanup(true); }

void AdminLayer::postData(const std::string& endpoint,
                          const std::string& jsonBody,
                          std::function<void(bool, std::string)> cb) {
    auto baseUrl = Mod::get()->getSettingValue<std::string>("api-url");
    auto token   = Mod::get()->getSettingValue<std::string>("admin-token");

    auto req = web::WebRequest();
    req.header("Content-Type", "application/json");
    req.header("Authorization", "Bearer " + token);
    req.bodyString(jsonBody);

    web::WebTask task = req.post(baseUrl + endpoint);
    task.listen([cb](web::WebTask::Event* e) {
        if (auto res = e->getValue()) {
            cb(res->ok(), res->string().unwrapOr("Erreur réseau"));
        }
    });
}

void AdminLayer::onAddTitle(CCObject*) {
    if (!m_inAccountId || !m_inTitle) return;

    std::string id    = m_inAccountId->getString();
    std::string title = m_inTitle->getString();
    std::string color = m_inColor    ? m_inColor->getString()   : "FFFFFF";
    std::string team  = m_inTeamName ? m_inTeamName->getString() : "";

    if (id.empty() || title.empty()) {
        showFeedback("Account ID et Titre requis !", false);
        return;
    }

    std::string body = fmt::format(
        R"({{"accountId":{},"title":"{}","color":"{}","team":"{}"}})",
        id, title, color, team);

    showFeedback("Envoi en cours...", true);
    postData("/api/admin/titles", body, [this](bool ok, std::string msg) {
        Loader::get()->queueInMainThread([this, ok, msg] {
            if (ok) {
                TitleManager::get()->forceReload();
                showFeedback("Titre ajouté !", true);
            } else {
                showFeedback("Erreur : " + msg, false);
            }
        });
    });
}

void AdminLayer::onAddTeam(CCObject*) {
    if (!m_inTName) return;

    std::string name   = m_inTName   ? m_inTName->getString()  : "";
    std::string points = m_inPoints  ? m_inPoints->getString()  : "0";
    std::string wins   = m_inWins    ? m_inWins->getString()    : "0";
    std::string losses = m_inLosses  ? m_inLosses->getString()  : "0";
    std::string color  = m_inTColor  ? m_inTColor->getString()  : "FFFFFF";

    if (name.empty()) {
        showFeedback("Nom de l'équipe requis !", false);
        return;
    }

    std::string body = fmt::format(
        R"({{"name":"{}","points":{},"wins":{},"losses":{},"color":"{}"}})",
        name, points, wins, losses, color);

    showFeedback("Envoi en cours...", true);
    postData("/api/admin/teams", body, [this](bool ok, std::string msg) {
        Loader::get()->queueInMainThread([this, ok, msg] {
            if (ok) {
                TitleManager::get()->forceReload();
                showFeedback("Equipe ajoutée !", true);
            } else {
                showFeedback("Erreur : " + msg, false);
            }
        });
    });
}
