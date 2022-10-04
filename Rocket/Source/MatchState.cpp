#include "MatchState.h"
#include "GameState.h"
#include "Car.h"
#include "Ball.h"
#include "BoostPickup.h"
#include "Menu.h"
#include "Hud.h"
#include "Hud3DS.h"

#include "Engine.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "NetworkManager.h"
#include "World.h"
#include "Log.h"
#include "InputDevices.h"
#include "Assets/Material.h"

#include "System/System.h"

DEFINE_ACTOR(MatchState, Actor);

MatchState::MatchState()
{
    mName = "Match State";
    mReplicate = true;
    mReplicateTransform = false;

    memset(mGoalBoxes, 0, sizeof(Actor*) * NUM_TEAMS);
    memset(mFullBoosts, 0, sizeof(Actor*) * NUM_FULL_BOOSTS);
    memset(mSpawnPoints0, 0, sizeof(Actor*) * NUM_SPAWN_POINTS);
    memset(mSpawnPoints1, 0, sizeof(Actor*) * NUM_SPAWN_POINTS);
}

void MatchState::Create()
{
    Actor::Create();

    // Self-register this actor in the game state.
    assert(GetGameState()->mMatchState == nullptr);
    GetGameState()->mMatchState = this;

    SetRootComponent(CreateComponent<TransformComponent>());
}

void MatchState::Destroy()
{
    Actor::Destroy();
    GetGameState()->mMatchState = nullptr;
}

void MatchState::BeginPlay()
{
    Actor::BeginPlay();

    World* world = GetWorld();

    GetGameState()->ShowMainMenuWidget(false);
    GetGameState()->ShowHudWidget(true);

    FindSpawnPointActors();
    PostLoadHandlePlatformTier();

    if (NetIsAuthority())
    {
        // Spawn Ball
        {
            Ball* ball = GetWorld()->SpawnActor<Ball>();
            ball->SetPosition(glm::vec3(0.0f, 8.0f, 0.0f));
            ball->UpdateComponentTransforms();
        }

        // Spawn Cars
        for (uint32_t i = 0; i < NUM_TEAMS * GetMatchOptions()->mTeamSize; ++i)
        {
            Car* newCar = GetWorld()->SpawnActor<Car>();
            newCar->SetPosition({ 10.0f * i, 2.0f, 0.0f });

            if (i == 0)
            {
                GetWorld()->SetActiveCamera(newCar->GetCameraComponent());
                GetWorld()->SetAudioReceiver(newCar->GetSphereComponent());
            }
        }

        // Spawn Boosts
        const std::vector<Actor*>& actors = GetWorld()->GetActors();
        for (int32_t i = (int32_t)actors.size() - 1; i >= 0; --i)
        {
            TransformComponent* comp = actors[i]->GetRootComponent();
            if (comp != nullptr &&
                (comp->GetName() == "FullBoost" || comp->GetName() == "MiniBoost"))
            {
                bool mini = (strncmp(comp->GetName().c_str(), "Mini", 4) == 0);
                glm::vec3 spawnLocation = comp->GetAbsolutePosition();

                world->DestroyActor(uint32_t(i));

                BoostPickup* pickup = GetWorld()->SpawnActor<BoostPickup>();
                pickup->SetPosition(spawnLocation);
                pickup->SetMini(mini);
                pickup->UpdateComponentTransforms();
            }
        }

        ResetMatchState();
    }
}

void MatchState::ResetMatchState()
{
    assert(NetIsAuthority());
    mBall = (Ball*)GetWorld()->FindActor("Ball");
    mTime = GetGameState()->mMatchOptions.mDuration;
    mTeamSize = GetGameState()->mMatchOptions.mTeamSize;
    memset(mTeams, 0, sizeof(Team) * NUM_TEAMS);

    mPhaseTime = 0.0f;
    mOvertime = false;


    // Assign cars, full boosts, spawns, and goals
    const std::vector<Actor*>& actors = GetWorld()->GetActors();
    mNumCars = 0;
    uint32_t boostCount = 0;

    for (uint32_t i = 0; i < actors.size(); ++i)
    {
        const char* rootCompName = actors[i]->GetRootComponent()->GetName().c_str();

        if (actors[i]->GetName() == "Car")
        {
            Car* car = (Car*)actors[i];
            mCars[mNumCars] = car;
            
            uint32_t team = mNumCars % 2;
            uint32_t carIndex = mNumCars / 2;
            mTeams[team].mCars[carIndex] = car;

            bool isBot = (mNumCars >= GetMatchOptions()->mNumPlayers);
            car->SetBot(isBot);
            car->SetCarIndex(isBot ? -1 : mNumCars);
            car->SetTeamIndex(team);

            BotBehavior behavior = BotBehavior::Offense;
            switch (carIndex)
            {
            case 0: behavior = BotBehavior::Offense; break;
            case 1: behavior = BotBehavior::Support; break;
            case 2: behavior = BotBehavior::Defense; break;
            default: behavior = BotBehavior::Offense; break;
            }
            car->SetBotBehavior(behavior);

            if (mNumCars == 0)
            {
                GetWorld()->SetActiveCamera(car->GetCameraComponent());
                GetWorld()->SetAudioReceiver(car->GetSphereComponent());
                mOwnedCar = car;
            }

            ++mNumCars;
        }

        if (boostCount < NUM_FULL_BOOSTS &&
            actors[i]->GetName() == "Boost" &&
            !static_cast<BoostPickup*>(actors[i])->IsMini())
        {
            mFullBoosts[boostCount] = actors[i];
            ++boostCount;
        }


        if (strncmp(rootCompName, "Goal", 4) == 0)
        {
            if (strlen(rootCompName) == 6)
            {
                uint32_t teamIndex = (uint32_t)(rootCompName[5] - '0');

                if (teamIndex == 0 || teamIndex == 1)
                {
                    mGoalBoxes[teamIndex] = actors[i];
                }
            }
        }
    }

    AssignCarHostIds();

    SetupKickoff();
    SetMatchPhase(MatchPhase::Waiting);
}

void MatchState::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);

    if (mBall == nullptr)
    {
        mBall = (Ball*) GetWorld()->FindActor("Ball");
    }

    if (NetIsAuthority())
    {
        mPhaseTime += deltaTime;
    }

    // Only show countdown text during Countdown phase
    if (GetGameState()->mHudWidget->IsVisible() &&
        mPhase != MatchPhase::Countdown)
    {
        GetGameState()->mHudWidget->SetCountdownTime(0);
    }


    switch (mPhase)
    {
    case MatchPhase::Waiting:
        if (NetIsAuthority() &&
            mPhaseTime >= 3.0f)
        {
            SetMatchPhase(MatchPhase::Countdown);
        }
        break;

    case MatchPhase::Countdown:
        if (NetIsAuthority() &&
            mPhaseTime >= 3.0f)
        {
            SetMatchPhase(MatchPhase::Play);
        }
        else
        {
            int32_t countTime = int32_t(3.0f - mPhaseTime) + 1;
            countTime = glm::clamp<int32_t>(countTime, 1, 3);
            GetGameState()->mHudWidget->SetCountdownTime(countTime);
        }
        break;

    case MatchPhase::Play:
        if (NetIsAuthority())
        {
            if (mOvertime)
            {
                mTime += deltaTime;
            }
            else
            {
                mTime -= deltaTime;

                if (mTime <= 0.0f)
                {
                    if (mTeams[0].mScore != mTeams[1].mScore)
                    {
                        SetMatchPhase(MatchPhase::Finished);
                    }
                    else
                    {
                        mOvertime = true;
                        SetupKickoff();
                        SetMatchPhase(MatchPhase::Countdown);
                    }
                }
            }
        }
        break;

    case MatchPhase::Goal:
        if (NetIsAuthority() &&
            mPhaseTime >= 5.0f)
        {
            if (mOvertime)
            {
                // This goal should have broken the tie.
                assert(mTeams[0].mScore != mTeams[1].mScore);
                SetMatchPhase(MatchPhase::Finished);
            }
            else
            {
                SetupKickoff();
                SetMatchPhase(MatchPhase::Countdown);
            }
        }
        break;

    case MatchPhase::Finished:
        if (NetIsAuthority() &&
            mPhaseTime >= 2.0f)
        {
            if (IsGamepadButtonJustDown(GAMEPAD_B, 0))
            {
                // Quit to menu
            }
            else if (IsGamepadButtonJustDown(GAMEPAD_A, 0))
            {
                // Rematch
                ResetMatchState();
            }
        }
        break;

    case MatchPhase::Count:
        // No match active (main menu)
        break;
    }
}

void MatchState::GatherReplicatedData(std::vector<NetDatum>& outData)
{
    Actor::GatherReplicatedData(outData);
    //outData.push_back(NetDatum(DatumType::Actor, this, &mBall));
    outData.push_back(NetDatum(DatumType::Float, this, &mTime));
    outData.push_back(NetDatum(DatumType::Integer, this, &mPhase));
    outData.push_back(NetDatum(DatumType::Float, this, &mPhaseTime));
    outData.push_back(NetDatum(DatumType::Bool, this, &mOvertime));
    outData.push_back(NetDatum(DatumType::Integer, this, &mTeams[0].mScore));
    outData.push_back(NetDatum(DatumType::Integer, this, &mTeams[1].mScore));
}

void MatchState::HandleGoal(uint32_t scoringTeam)
{
    // It's possible the ball can go into goal after game ends
    if (mPhase == MatchPhase::Play)
    {
        assert(scoringTeam < NUM_TEAMS);
        mTeams[scoringTeam].mScore++;

        SetMatchPhase(MatchPhase::Goal);
    }
}

void MatchState::AssignHostToCar(NetClient* client)
{
    assert(NetIsServer());

    for (uint32_t i = 0; i < mNumCars; ++i)
    {
        if (mCars[i]->GetOwningHost() == INVALID_HOST_ID)
        {
            mCars[i]->SetOwningHost(client->mHost.mId);
            mCars[i]->SetBot(false);
            mCars[i]->ForceReplication();
            break;
        }
    }
}

bool MatchState::IsMatchOngoing() const
{
    return mPhase != MatchPhase::Count;
}

void MatchState::FindSpawnPointActors()
{
    // Gather spawn points
    assert(NUM_SPAWN_POINTS == 3);
    mSpawnPoints0[0] = GetWorld()->FindComponent("Spawn.0.0")->GetOwner();
    mSpawnPoints0[1] = GetWorld()->FindComponent("Spawn.0.1")->GetOwner();
    mSpawnPoints0[2] = GetWorld()->FindComponent("Spawn.0.2")->GetOwner();

    mSpawnPoints1[0] = GetWorld()->FindComponent("Spawn.1.0")->GetOwner();
    mSpawnPoints1[1] = GetWorld()->FindComponent("Spawn.1.1")->GetOwner();
    mSpawnPoints1[2] = GetWorld()->FindComponent("Spawn.1.2")->GetOwner();
}

void MatchState::PostLoadHandlePlatformTier()
{
    // For now, turn off particles on Old 3DS (Tier 0)
    const std::vector<Actor*>& actors = GetWorld()->GetActors();

    if (SYS_GetPlatformTier() < 1)
    {
        for (int32_t i = int32_t(actors.size()) - 1; i >= 0; --i)
        {
            if (actors[i]->GetRootComponent()->GetName() == "Particle")
            {
                GetWorld()->DestroyActor(i);
            }
        }
    }
}

void MatchState::SetMatchPhase(MatchPhase phase)
{
    assert(NetIsAuthority());

    if (mPhase != phase)
    {
        mPhase = phase;
        const char* phaseName = "???";

        switch (phase)
        {
        case MatchPhase::Waiting:
            phaseName = "Waiting";
            EnableCarControl(false);
            break;

        case MatchPhase::Countdown:
            phaseName = "Countdown";
            EnableCarControl(false);
            break;

        case MatchPhase::Play:
            phaseName = "Play";
            EnableCarControl(true);
            break;

        case MatchPhase::Goal:
            phaseName = "Goal";
            EnableCarControl(true);
            break;

        case MatchPhase::Finished:
            phaseName = "Finished";
            EnableCarControl(false);
            break;

        case MatchPhase::Count:
            phaseName = "---";
            // No match active (main menu)
            break;
        }

        OCT_UNUSED(phaseName);
        //LogDebug("Match Phase: %s", phaseName);

        mPhaseTime = 0.0f;
    }
}



void MatchState::SetupKickoff()
{
    assert(NetIsAuthority());

    mBall->Reset();

    for (uint32_t t = 0; t < NUM_TEAMS; ++t)
    {
        for (uint32_t c = 0; c < MAX_TEAM_SIZE; ++c)
        {
            if (mTeams[t].mCars[c] != nullptr)
            {
                Car* car = mTeams[t].mCars[c];

                // Set position and orientation based on spawn component
                Actor* spawnActor = (t == 0) ? mSpawnPoints0[c] : mSpawnPoints1[c];
                car->InvokeNetFunc("C_ForceTransform",
                    spawnActor->GetPosition(),
                    glm::vec3(0.0f, (t == 0) ? -90.0f : 90.0f, 0.0f));
                car->Reset();

                // Make one of the bots charge straight for the ball on kickoff.
                if (car->IsBot() &&
                    c == (GetMatchOptions()->mTeamSize - 1))
                {
                    //car->ForceBotTargetBall();
                }
            }
        }
    }

    const std::vector<Actor*>& actors = GetWorld()->GetActors();
    for (uint32_t i = 0; i < actors.size(); ++i)
    {
        if (actors[i]->GetName() == "Boost")
        {
            static_cast<BoostPickup*>(actors[i])->Reset();
        }
    }
}

void MatchState::EnableCarControl(bool enable)
{
    assert(NetIsAuthority());

    for (uint32_t t = 0; t < NUM_TEAMS; ++t)
    {
        for (uint32_t c = 0; c < MAX_TEAM_SIZE; ++c)
        {
            if (mTeams[t].mCars[c] != nullptr)
            {
                mTeams[t].mCars[c]->EnableControl(enable);
            }
        }
    }
}

void MatchState::AssignCarHostIds()
{
    assert(NetIsAuthority());
    assert(mNumCars >= 1);

    if (NetIsServer())
    {
        mCars[0]->SetOwningHost(SERVER_HOST_ID);
        mCars[0]->SetBot(false);

        const std::vector<NetClient>& clients = NetworkManager::Get()->GetClients();

        for (uint32_t i = 0; i < clients.size(); ++i)
        {
            if (i + 1 < mNumCars)
            {
                mCars[i + 1]->SetOwningHost(clients[i].mHost.mId);
                mCars[i + 1]->SetBot(false);
            }
        }
    }
}
