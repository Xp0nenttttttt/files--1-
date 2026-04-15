#pragma once
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

struct PlayerTitle {
    int accountId;
    std::string title;
    std::string colorHex;
    std::string team;
};

struct TeamEntry {
    int rank;
    std::string name;
    int points;
    int wins;
    int losses;
    std::string colorHex;
};

// ─── TitleManager (singleton) ─────────────────────────────────────────────────
class TitleManager {
public:
    static TitleManager* get();

    // Charge depuis le serveur distant (async)
    void loadFromRemote(std::function<void(bool)> callback = nullptr);
    // Force un rechargement (après modification admin)
    void forceReload();
    // Fallback : charge depuis le JSON local packagé
    void loadFromLocalFile();

    std::optional<PlayerTitle> getTitleForAccount(int accountId);
    std::vector<TeamEntry>     getTeamRankings();
    bool                       isAdmin(int accountId);
    bool                       isLoaded() const { return m_loaded; }

    static ccColor3B colorFromEntry(const std::string& hex);

private:
    TitleManager() = default;
    void parseJson(const std::string& json);

    std::vector<PlayerTitle> m_titles;
    std::vector<TeamEntry>   m_teams;
    std::vector<int>         m_adminIds;
    bool m_loaded = false;

    static ccColor3B hexToColor(const std::string& hex);
};

// ─── RankingLayer ─────────────────────────────────────────────────────────────
class RankingLayer : public CCLayerColor {
public:
    static RankingLayer* create();
    bool init() override;

private:
    void onClose(CCObject*);
    void onWebsite(CCObject*);
    void onAdmin(CCObject*);
    void buildTable(const std::vector<TeamEntry>& teams);
};
