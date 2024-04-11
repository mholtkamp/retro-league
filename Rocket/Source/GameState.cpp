#include "GameState.h"
#include "Car.h"
#include "Ball.h"
#include "BoostPickup.h"
#include "Menu.h"
#include "Hud.h"
#include "Hud3DS.h"
#include "Nodes/Node.h"

#include "Engine.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "NetworkManager.h"
#include "World.h"
#include "Log.h"
#include "InputDevices.h"
#include "Assets/MaterialLite.h"
#include "Assets/Scene.h"

#include "System/System.h"

constexpr const char* kRocketSaveName = "RocketSave.dat";

GameState gGameState;

GameState* GetGameState()
{
    return &gGameState;
}

MatchState* GetMatchState()
{
    return gGameState.mMatchState;
}

MatchOptions* GetMatchOptions()
{
    return &gGameState.mMatchOptions;
}

void NetworkConnectCb(NetClient* newClient)
{
    if (GetMatchState() != nullptr)
    {
        GetMatchState()->AssignHostToCar(newClient);
    }
}

void NetworkAcceptCb()
{
    // Moving into game, close the menu.
    // Arena level is loaded by NetMsgLoadLevel
    gGameState.ShowMainMenuWidget(false);
}

void NetworkKickCb(NetMsgKick::Reason reason)
{
    // We've been kicked. Load up the main menu.
    // In the future also display a popup saying why.
    gGameState.LoadMainMenu();
}

void NetworkDisconnectCb(NetClient* client)
{
    gGameState.ClearPlayer(client->mHost.mId);
}

void GameState::Initialize()
{
    LoadMaterials();
    NetworkManager* netMan = NetworkManager::Get();
    netMan->SetConnectCallback(NetworkConnectCb);
    netMan->SetAcceptCallback(NetworkAcceptCb);
    netMan->SetKickCallback(NetworkKickCb);
    netMan->SetDisconnectCallback(NetworkDisconnectCb);
}

void GameState::Shutdown()
{
    ShowMainMenuWidget(false);
    ShowHudWidget(false);
}

void GameState::LoadArena()
{
    if (mMatchOptions.mNetworkMode == NetworkMode::LAN ||
        mMatchOptions.mNetworkMode == NetworkMode::Online)
    {
        NetworkManager::Get()->SetMaxClients(mMatchOptions.mTeamSize * 2 - 1);
        NetworkManager::Get()->OpenSession();
    }

    ShowMainMenuWidget(false);

    GetWorld(0)->LoadScene("L_Arena", true);

    if (mMatchOptions.mEnvironmentType == EnvironmentType::Lagoon)
    {
        Node* lagoonScene = GetWorld(0)->SpawnScene("L_Lagoon");
        lagoonScene->SetReplicate(true);
    }

    // Spawn HUD after L_Arena (which will be the root node)
    ShowHudWidget(true);

    // The MatchState actor relies on L_Arena being loaded.
    //GetWorld()->SpawnActor<MatchState>();
}

void GameState::LoadMainMenu()
{
    ShowHudWidget(false);

    GetWorld(0)->DestroyRootNode();

    ShowMainMenuWidget(true);

    AssetManager::Get()->RefSweep();

    if (NetIsClient())
    {
        NetworkManager::Get()->Disconnect();
    }
    else if (NetIsServer())
    {
        NetworkManager::Get()->CloseSession();
    }
}

void GameState::ShowMainMenuWidget(bool show)
{
    if (show && mMainMenuWidget == nullptr)
    {
#if PLATFORM_3DS
        // On 3DS, set the main menu on the bottom screen
        //Renderer::Get()->AddWidget(mMainMenuWidget, -1, 1);
        mMainMenuWidget = GetWorld(1)->SpawnNode<MainMenu>();

#else
        mMainMenuWidget = GetWorld(0)->SpawnNode<MainMenu>();
#endif
    }
    else if (!show && mMainMenuWidget != nullptr)
    {
        Node::Destruct(mMainMenuWidget.Get());
        mMainMenuWidget = nullptr;
    }
}

void GameState::ShowHudWidget(bool show)
{
    if (show && mHudWidget == nullptr)
    {
#if PLATFORM_3DS
        mHudWidget = GetWorld(1)->SpawnNode<Hud>();
#else
        mHudWidget = GetWorld(0)->SpawnNode<Hud>();
#endif
    }
    else if (!show && mHudWidget != nullptr)
    {
        Node::Destruct(mHudWidget.Get());
        mHudWidget = nullptr;
    }
}

bool GameState::IsInMainMenu() const
{
    return mMainMenuOpen;
}

void GameState::ClearPlayer(NetHostId hostId)
{
    if (NetIsAuthority() &&
        mMatchState != nullptr)
    {
        for (uint32_t i = 0; i < MAX_CARS; ++i)
        {
            if (mMatchState->mCars[i] != nullptr &&
                mMatchState->mCars[i]->GetOwningHost() == hostId)
            {
                mMatchState->mCars[i]->SetOwningHost(INVALID_HOST_ID);
                mMatchState->mCars[i]->SetBot(true);
            }
        }
    }
}

void GameState::SavePreferredMatchOptions()
{
    Stream saveData;
    saveData.WriteFloat(mMatchOptions.mDuration);
    saveData.WriteUint32(mMatchOptions.mTeamSize);
    saveData.WriteUint32(mMatchOptions.mNumPlayers);
    saveData.WriteUint32((uint32_t)mMatchOptions.mNetworkMode);
    saveData.WriteUint32((uint32_t)mMatchOptions.mEnvironmentType);
    saveData.WriteBool(mMatchOptions.mBots);

    SYS_WriteSave(kRocketSaveName, saveData);
    // 21 bytes?
}

void GameState::LoadPreferredMatchOptions()
{
    Stream saveData;

    if (SYS_DoesSaveExist(kRocketSaveName))
    {
        SYS_ReadSave(kRocketSaveName, saveData);

        mMatchOptions.mDuration = saveData.ReadFloat();
        mMatchOptions.mTeamSize = saveData.ReadUint32();
        mMatchOptions.mNumPlayers = saveData.ReadUint32();
        mMatchOptions.mNetworkMode = (NetworkMode)saveData.ReadUint32();
        mMatchOptions.mEnvironmentType = (EnvironmentType)saveData.ReadUint32();
        mMatchOptions.mBots = saveData.ReadBool();
    }
}

void GameState::DeletePreferredMatchOptions()
{
    SYS_DeleteSave(kRocketSaveName);
}

void GameState::Update(float deltaTime)
{
    // Tick time
    // End game when time is out.

    if (mTransitionToGame)
    {
        //SavePreferredMatchOptions();
        LoadArena();
        mTransitionToGame = false;
    }

    if (mTransitionToMainMenu)
    {
        LoadMainMenu();
        mTransitionToMainMenu = false;
    }

    UpdateMaterials(deltaTime);
}

void GameState::LoadMaterials()
{
    mGhWaterMat = LoadAsset("M_GH_Water");
    mGhWaterfallMat1 = LoadAsset("M_GH_Waterfall1");
    mGhWaterfallMat2 = LoadAsset("M_GH_Waterfall2");
    mGoalSpinnerMat = LoadAsset("M_ArenaM2_GoalSpinner");
}

void GameState::UpdateMaterials(float deltaTime)
{
    MaterialLite* ghWaterMat = mGhWaterMat.Get<MaterialLite>();
    ghWaterMat->SetUvOffset(ghWaterMat->GetUvOffset() + deltaTime * glm::vec2(0.2f, 0.2f));

    MaterialLite* ghWaterfallMat1 = mGhWaterfallMat1.Get<MaterialLite>();
    ghWaterfallMat1->SetUvOffset(ghWaterfallMat1->GetUvOffset() + deltaTime * glm::vec2(0.0f, -1.0f));

    MaterialLite* ghWaterfallMat2 = mGhWaterfallMat2.Get<MaterialLite>();
    ghWaterfallMat2->SetUvOffset(ghWaterfallMat2->GetUvOffset() + deltaTime * glm::vec2(0.0f, -0.5f));

    MaterialLite* spinnerMat = mGoalSpinnerMat.Get<MaterialLite>();
    spinnerMat->SetUvOffset(spinnerMat->GetUvOffset() + deltaTime * glm::vec2(0.0f, 0.5f));
}
