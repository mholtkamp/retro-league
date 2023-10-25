#pragma once

#include "RocketConstants.h"
#include "RocketTypes.h"
#include "MatchState.h"
#include "Nodes/Node.h"
#include "ObjectRef.h"

class Car;
class Ball;

enum class NetworkMode
{
    Local,
    LAN,
    Online,

    Count
};

struct MatchOptions
{
    float mDuration = 60.0f * 5.0f;
    uint32_t mTeamSize = MAX_TEAM_SIZE;
    uint32_t mNumPlayers = 1;
    NetworkMode mNetworkMode = NetworkMode::Local;
    EnvironmentType mEnvironmentType = EnvironmentType::Lagoon;
    bool mBots = true;
};

struct GameState
{
    MatchState* mMatchState = nullptr;
    MatchOptions mMatchOptions;
    bool mMainMenuOpen = false;
    bool mTransitionToGame = false;
    bool mTransitionToMainMenu = false;
    NodeRef mMainMenuWidget = nullptr;
    NodeRef mHudWidget = nullptr;

    void Initialize();
    void Shutdown();
    void LoadArena();
    void LoadMainMenu();
    void ShowMainMenuWidget(bool show);
    void ShowHudWidget(bool show);
    bool IsInMainMenu() const;
    void ClearPlayer(NetHostId hostId);

    void SavePreferredMatchOptions();
    void LoadPreferredMatchOptions();
    void DeletePreferredMatchOptions();

    void Update(float deltaTime);

private:

    void LoadMaterials();
    void UpdateMaterials(float deltaTime);

    MaterialRef mGhWaterMat;
    MaterialRef mGhWaterfallMat1;
    MaterialRef mGhWaterfallMat2;
    MaterialRef mGoalSpinnerMat;
};

GameState* GetGameState();
MatchState* GetMatchState();
MatchOptions* GetMatchOptions();
