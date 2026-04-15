#include "RankingLayer.hpp"
#include "AdminLayer.hpp"
#include <Geode/utils/web.hpp>

// ═══════════════════════════════════════════════════════════════════
//  TitleManager
// ═══════════════════════════════════════════════════════════════════

TitleManager* TitleManager::get() {
    static TitleManager instance;
    return &instance;
}

ccColor3B TitleManager::hexToColor(const std::string& hex) {
    std::string h = hex;
    if (!h.empty() && h[0] == '#') h = h.substr(1);
    if (h.size() < 6) return {255, 255, 255};
    unsigned r = std::stoul(h.substr(0,2),nullptr,16);
    unsigned g = std::stoul(h.substr(2,2),nullptr,16);
    unsigned b = std::stoul(h.substr(4,2),nullptr,16);
    return {(GLubyte)r,(GLubyte)g,(GLubyte)b};
}

ccColor3B TitleManager::colorFromEntry(const std::string& hex) {
    return hexToColor(hex);
}

void TitleManager::parseJson(const std::string& jsonStr) {
    m_titles.clear();
    m_teams.clear();
    m_adminIds.clear();

    auto root = matjson::parse(jsonStr);
    if (!root) { log::warn("JSON invalide reçu du serveur"); return; }

    if (root->contains("titles")) {
        for (auto& t : root->get<matjson::Array>("titles").unwrapOrDefault()) {
            PlayerTitle pt;
            pt.accountId = t["accountId"].asInt().unwrapOr(0);
            pt.title     = t["title"].asString().unwrapOr("");
            pt.colorHex  = t["color"].asString().unwrapOr("FFFFFF");
            pt.team      = t["team"].asString().unwrapOr("");
            if (pt.accountId && !pt.title.empty()) m_titles.push_back(pt);
        }
    }
    if (root->contains("teams")) {
        for (auto& e : root->get<matjson::Array>("teams").unwrapOrDefault()) {
            TeamEntry te;
            te.rank   = e["rank"].asInt().unwrapOr(0);
            te.name   = e["name"].asString().unwrapOr("???");
            te.points = e["points"].asInt().unwrapOr(0);
            te.wins   = e["wins"].asInt().unwrapOr(0);
            te.losses = e["losses"].asInt().unwrapOr(0);
            te.colorHex = e["color"].asString().unwrapOr("FFFFFF");
            m_teams.push_back(te);
        }
    }
    if (root->contains("admins")) {
        for (auto& a : root->get<matjson::Array>("admins").unwrapOrDefault())
            m_adminIds.push_back(a.asInt().unwrapOr(0));
    }

    m_loaded = true;
    log::info("TitleManager: {} titres, {} equipes, {} admins.",
              m_titles.size(), m_teams.size(), m_adminIds.size());
}

void TitleManager::loadFromRemote(std::function<void(bool)> callback) {
    auto apiUrl = Mod::get()->getSettingValue<std::string>("api-url");

    auto req = web::WebRequest();
    web::WebTask task = req.get(apiUrl + "/api/data");

    task.listen([this, callback](web::WebTask::Event* e) {
        if (auto res = e->getValue()) {
            if (res->ok()) {
                auto body = res->string().unwrapOr("{}");
                Loader::get()->queueInMainThread([this, body, callback] {
                    parseJson(body);
                    // Sauvegarde cache local
                    auto cache = Mod::get()->getSaveDir() / "titles_cache.json";
                    (void)file::writeString(cache, body);
                    if (callback) callback(true);
                });
            } else {
                log::warn("Serveur injoignable, chargement du cache local.");
                Loader::get()->queueInMainThread([this, callback] {
                    loadFromLocalFile();
                    if (callback) callback(false);
                });
            }
        }
    });
}

void TitleManager::forceReload() {
    m_loaded = false;
    loadFromRemote();
}

void TitleManager::loadFromLocalFile() {
    // 1. Essaie le cache sauvegardé
    auto cache = Mod::get()->getSaveDir() / "titles_cache.json";
    auto cached = file::readString(cache);
    if (cached) { parseJson(cached.unwrap()); return; }

    // 2. Fallback sur le JSON packagé dans le mod
    auto bundled = Mod::get()->getResourcesDir() / "titles_data.json";
    auto content = file::readString(bundled);
    if (content) parseJson(content.unwrap());
}

std::optional<PlayerTitle> TitleManager::getTitleForAccount(int id) {
    for (auto& t : m_titles) if (t.accountId == id) return t;
    return std::nullopt;
}

std::vector<TeamEntry> TitleManager::getTeamRankings() { return m_teams; }

bool TitleManager::isAdmin(int id) {
    for (auto a : m_adminIds) if (a == id) return true;
    return false;
}

// ═══════════════════════════════════════════════════════════════════
//  RankingLayer
// ═══════════════════════════════════════════════════════════════════

RankingLayer* RankingLayer::create() {
    auto ret = new RankingLayer();
    if (ret && ret->init()) { ret->autorelease(); return ret; }
    CC_SAFE_DELETE(ret); return nullptr;
}

bool RankingLayer::init() {
    if (!CCLayerColor::initWithColor({0,0,0,200})) return false;
    this->setZOrder(100);

    auto ws = CCDirector::sharedDirector()->getWinSize();
    float cx = ws.width/2, cy = ws.height/2;

    auto panel = CCScale9Sprite::create("GJ_square01.png");
    panel->setContentSize({440, 320});
    panel->setPosition({cx, cy});
    this->addChild(panel, 1);

    auto title = CCLabelBMFont::create("Classement des Equipes", "goldFont.fnt");
    title->setScale(0.75f);
    title->setPosition({cx, cy + 142});
    this->addChild(title, 2);

    buildTable(TitleManager::get()->getTeamRankings());

    // ── Menu boutons ──
    auto menu = CCMenu::create();
    menu->setPosition(CCPointZero);

    // Fermer
    auto closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
    closeSpr->setScale(0.9f);
    auto closeBtn = CCMenuItemSpriteExtra::create(
        closeSpr, this, menu_selector(RankingLayer::onClose));
    closeBtn->setPosition({cx+198, cy+142});
    menu->addChild(closeBtn);

    // Site web
    auto webBg = CCScale9Sprite::create("GJ_button_01.png");
    webBg->setContentSize({140, 34});
    auto webLbl = CCLabelBMFont::create("Voir le Site", "bigFont.fnt");
    webLbl->setScale(0.45f);
    webLbl->setPosition(webBg->getContentSize()/2);
    webBg->addChild(webLbl);
    auto webBtn = CCMenuItemSpriteExtra::create(
        webBg, this, menu_selector(RankingLayer::onWebsite));
    webBtn->setPosition({cx - 80, cy - 142});
    menu->addChild(webBtn);

    // Bouton Admin (visible seulement si l'utilisateur connecté est admin)
    auto myAccount = GJAccountManager::sharedState()->m_accountID;
    if (TitleManager::get()->isAdmin(myAccount)) {
        auto adminBg = CCScale9Sprite::create("GJ_button_03.png");
        adminBg->setContentSize({140, 34});
        auto adminLbl = CCLabelBMFont::create("Panel Admin", "bigFont.fnt");
        adminLbl->setScale(0.45f);
        adminLbl->setPosition(adminBg->getContentSize()/2);
        adminBg->addChild(adminLbl);
        auto adminBtn = CCMenuItemSpriteExtra::create(
            adminBg, this, menu_selector(RankingLayer::onAdmin));
        adminBtn->setPosition({cx + 80, cy - 142});
        menu->addChild(adminBtn);
    }

    this->addChild(menu, 3);
    return true;
}

void RankingLayer::buildTable(const std::vector<TeamEntry>& teams) {
    auto ws = CCDirector::sharedDirector()->getWinSize();
    float cx = ws.width/2, cy = ws.height/2;
    float startY = cy + 100, rowH = 38.f;
    float cols[] = {-185,-95,65,115,160};

    const char* headers[] = {"#","Equipe","Pts","V","D"};
    for (int i=0;i<5;i++) {
        auto l = CCLabelBMFont::create(headers[i],"goldFont.fnt");
        l->setScale(0.42f);
        l->setPosition({cx+cols[i],startY});
        this->addChild(l,2);
    }
    auto sep = CCLayerColor::create({255,255,255,50},380,1);
    sep->setPosition({cx-190,startY-12});
    this->addChild(sep,2);

    int n=0;
    for (auto& t : teams) {
        if (n>=5) break;
        float y = startY - 20 - n*rowH;
        if (n%2==0) {
            auto bg = CCLayerColor::create({255,255,255,12},380,32);
            bg->setPosition({cx-190,y-16});
            this->addChild(bg,1);
        }
        ccColor3B c = TitleManager::colorFromEntry(t.colorHex);
        auto addLbl = [&](std::string s, float x, ccColor3B col={255,255,255}, float anchor=0.5f){
            auto l = CCLabelBMFont::create(s.c_str(),"bigFont.fnt");
            l->setScale(0.37f); l->setColor(col);
            l->setAnchorPoint({anchor,0.5f});
            l->setPosition({cx+x,y});
            this->addChild(l,2);
        };
        addLbl(std::to_string(t.rank),  cols[0], c);
        addLbl(t.name,                   cols[1], c, 0.f);
        addLbl(std::to_string(t.points), cols[2]);
        addLbl(std::to_string(t.wins),   cols[3], {100,255,100});
        addLbl(std::to_string(t.losses), cols[4], {255,100,100});
        n++;
    }
}

void RankingLayer::onClose(CCObject*)   { this->removeFromParentAndCleanup(true); }
void RankingLayer::onAdmin(CCObject*)   {
    auto layer = AdminLayer::create();
    if (layer) CCDirector::sharedDirector()->getRunningScene()->addChild(layer,200);
}
void RankingLayer::onWebsite(CCObject*) {
    web::openLinkInBrowser(Mod::get()->getSettingValue<std::string>("website-url"));
}
