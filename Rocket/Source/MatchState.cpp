#include "MatchState.h"
#include "GameState.h"
#include "Car.h"
#include "Ball.h"
#include "BoostPickup.h"
#include "Menu.h"
#include "Hud.h"

#include "Engine.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "NetworkManager.h"
#include "World.h"
#include "Log.h"
#include "InputDevices.h"
#include "Assets/Material.h"

#include "System/System.h"

DEFINE_NODE(MatchState, Node3D);

MatchState::MatchState()
{
    mName = "Match State";
    mReplicate = true;
    mReplicateTransform = false;

    memset(mGoalBoxes, 0, sizeof(Node*) * NUM_TEAMS);
    memset(mFullBoosts, 0, sizeof(Node*) * NUM_FULL_BOOSTS);
    memset(mSpawnPoints0, 0, sizeof(Node*) * NUM_SPAWN_POINTS);
    memset(mSpawnPoints1, 0, sizeof(Node*) * NUM_SPAWN_POINTS);
}

void MatchState::Create()
{
    Node3D::Create();

    if (IsPlaying())
    {
        // Self-register this actor in the game state.
        OCT_ASSERT(GetGameState()->mMatchState == nullptr);
        GetGameState()->mMatchState = this;
    }
}

void MatchState::Destroy()
{
    Node3D::Destroy();

    if (IsPlaying())
    {
        GetGameState()->mMatchState = nullptr;
    }
}

void MatchState::Start()
{
    Node3D::Start();

    World* world = GetWorld();

    GetGameState()->ShowMainMenuWidget(false);
    GetGameState()->ShowHudWidget(true);

    FindSpawnPointActors();
    PostLoadHandlePlatformTier();

    if (NetIsAuthority())
    {
        // Spawn Ball
        {
            Ball* ball = GetWorld()->SpawnNode<Ball>();
            ball->SetPosition(glm::vec3(0.0f, 8.0f, 0.0f));
            ball->UpdateTransform(true);
        }

        // Spawn Cars
        for (uint32_t i = 0; i < NUM_TEAMS * GetMatchOptions()->mTeamSize; ++i)
        {
            Car* newCar = GetWorld()->SpawnNode<Car>();
            newCar->SetPosition({ 10.0f * i, 2.0f, 0.0f });

            if (i == 0)
            {
                GetWorld()->SetActiveCamera(newCar->GetCamera3D());
                GetWorld()->SetAudioReceiver(newCar);
            }
        }

        // Spawn Boosts
        const std::vector<Node*> actors = GetWorld()->GatherNodes();
        for (int32_t i = (int32_t)actors.size() - 1; i >= 0; --i)
        {
            Node3D* comp = actors[i]->As<Node3D>();
            if (comp != nullptr &&
                (comp->GetName().find("FullBoost") == 0 ||
                comp->GetName().find("MiniBoost") == 0))
            {
                bool mini = (strncmp(comp->GetName().c_str(), "Mini", 4) == 0);
                glm::vec3 spawnLocation = comp->GetAbsolutePosition();

                comp->SetPendingDestroy(true);

                BoostPickup* pickup = GetWorld()->SpawnNode<BoostPickup>();
                pickup->SetPosition(spawnLocation);
                pickup->SetMini(mini);
                pickup->UpdateTransform(true);
            }
        }

        ResetMatchState();
    }
}

void MatchState::ResetMatchState()
{
    OCT_ASSERT(NetIsAuthority());
    mBall = GetWorld()->FindNode("Ball")->As<Ball>();
    mTime = GetGameState()->mMatchOptions.mDuration;
    mTeamSize = GetGameState()->mMatchOptions.mTeamSize;
    memset(mTeams, 0, sizeof(Team) * NUM_TEAMS);

    mPhaseTime = 0.0f;
    mOvertime = false;


    // Assign cars, full boosts, spawns, and goals
    const std::vector<Node*> nodes = GetWorld()->GatherNodes();
    mNumCars = 0;
    uint32_t boostCount = 0;

    for (uint32_t i = 0; i < nodes.size(); ++i)
    {
        const char* rootCompName = nodes[i]->GetName().c_str();
        Car* car = nodes[i]->As<Car>();

        if (car != nullptr)
        {
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
                GetWorld()->SetActiveCamera(car->GetCamera3D());
                GetWorld()->SetAudioReceiver(car);
                mOwnedCar = car;
            }

            ++mNumCars;
        }

        BoostPickup* pickup = nodes[i]->As<BoostPickup>();

        if (boostCount < NUM_FULL_BOOSTS &&
            pickup != nullptr)
        {
            if (pickup && !pickup->IsMini())
            {
                mFullBoosts[boostCount] = pickup;
                ++boostCount;
            }
        }


        if (strncmp(rootCompName, "Goal", 4) == 0)
        {
            if (strlen(rootCompName) == 6)
            {
                uint32_t teamIndex = (uint32_t)(rootCompName[5] - '0');

                if (teamIndex == 0 || teamIndex == 1)
                {
                    mGoalBoxes[teamIndex] = nodes[i]->As<Node3D>();
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
    Node3D::Tick(deltaTime);

    if (mBall == nullptr)
    {
        mBall = GetWorld()->FindNode("Ball")->As<Ball>();
    }

    if (NetIsAuthority())
    {
        mPhaseTime += deltaTime;
    }

    // Only show countdown text during Countdown phase
    if (GetGameState()->mHudWidget.Get()->IsVisible() &&
        mPhase != MatchPhase::Countdown)
    {
        GetGameState()->mHudWidget.Get<Hud>()->SetCountdownTime(0);
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
            GetGameState()->mHudWidget.Get<Hud>()->SetCountdownTime(countTime);
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
                OCT_ASSERT(mTeams[0].mScore != mTeams[1].mScore);
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
    Node3D::GatherReplicatedData(outData);
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
        OCT_ASSERT(scoringTeam < NUM_TEAMS);
        mTeams[scoringTeam].mScore++;

        SetMatchPhase(MatchPhase::Goal);
    }
}

void MatchState::AssignHostToCar(NetClient* client)
{
    OCT_ASSERT(NetIsServer());

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
    OCT_ASSERT(NUM_SPAWN_POINTS == 3);
    mSpawnPoints0[0] = GetWorld()->FindNode("Spawn.0.0")->As<Node3D>();
    mSpawnPoints0[1] = GetWorld()->FindNode("Spawn.0.1")->As<Node3D>();
    mSpawnPoints0[2] = GetWorld()->FindNode("Spawn.0.2")->As<Node3D>();

    mSpawnPoints1[0] = GetWorld()->FindNode("Spawn.1.0")->As<Node3D>();
    mSpawnPoints1[1] = GetWorld()->FindNode("Spawn.1.1")->As<Node3D>();
    mSpawnPoints1[2] = GetWorld()->FindNode("Spawn.1.2")->As<Node3D>();
}

void MatchState::PostLoadHandlePlatformTier()
{
    // For now, turn off particles on Old 3DS (Tier 0)
    const std::vector<Node*> nodes = GetWorld()->GatherNodes();

    if (SYS_GetPlatformTier() < 1)
    {
        for (int32_t i = int32_t(nodes.size()) - 1; i >= 0; --i)
        {
            if (nodes[i]->GetName().find("Particle") == 0)
            {
                nodes[i]->SetPendingDestroy(true);
            }
        }
    }
}

void MatchState::SetMatchPhase(MatchPhase phase)
{
    OCT_ASSERT(NetIsAuthority());

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
    OCT_ASSERT(NetIsAuthority());

    mBall->Reset();

    for (uint32_t t = 0; t < NUM_TEAMS; ++t)
    {
        for (uint32_t c = 0; c < MAX_TEAM_SIZE; ++c)
        {
            if (mTeams[t].mCars[c] != nullptr)
            {
                Car* car = mTeams[t].mCars[c];

                // Set position and orientation based on spawn component
                Node3D* spawnActor = (t == 0) ? mSpawnPoints0[c] : mSpawnPoints1[c];
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

    const std::vector<Node*> nodes = GetWorld()->GatherNodes();
    for (uint32_t i = 0; i < nodes.size(); ++i)
    {
        BoostPickup* boost = nodes[i]->As<BoostPickup>();

        if (boost != nullptr)
        {
            boost->Reset();
        }
    }
}

void MatchState::EnableCarControl(bool enable)
{
    OCT_ASSERT(NetIsAuthority());

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
    OCT_ASSERT(NetIsAuthority());
    OCT_ASSERT(mNumCars >= 1);

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
