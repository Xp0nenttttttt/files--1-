#pragma once
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

// ─── Panneau admin in-game ────────────────────────────────────────────────────
class AdminLayer : public CCLayerColor {
public:
    static AdminLayer* create();
    bool init() override;

private:
    // UI tabs
    void showTitlesTab();
    void showTeamsTab();
    void clearContent();

    // Actions
    void onTabTitles(CCObject*);
    void onTabTeams(CCObject*);
    void onAddTitle(CCObject*);
    void onAddTeam(CCObject*);
    void onClose(CCObject*);
    void onSave(CCObject*);

    // Helpers
    CCTextInputNode* makeInput(const char* placeholder, float width);
    void showFeedback(const std::string& msg, bool success);
    void postData(const std::string& endpoint,
                  const std::string& jsonBody,
                  std::function<void(bool, std::string)> cb);

    // State
    int  m_tab = 0;           // 0 = titres, 1 = équipes
    CCNode* m_content = nullptr;

    // Inputs — onglet Titres
    CCTextInputNode* m_inAccountId  = nullptr;
    CCTextInputNode* m_inTitle      = nullptr;
    CCTextInputNode* m_inColor      = nullptr;
    CCTextInputNode* m_inTeamName   = nullptr;

    // Inputs — onglet Équipes
    CCTextInputNode* m_inTName   = nullptr;
    CCTextInputNode* m_inPoints  = nullptr;
    CCTextInputNode* m_inWins    = nullptr;
    CCTextInputNode* m_inLosses  = nullptr;
    CCTextInputNode* m_inTColor  = nullptr;
};
