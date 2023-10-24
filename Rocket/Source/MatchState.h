#pragma once

#include "RocketConstants.h"
#include "RocketTypes.h"

#include "Nodes/Node.h"
#include "Nodes/3D/Node3d.h"

class Car;
class Ball;


enum class MatchPhase
{
    Waiting,
    Countdown,
    Play,
    Goal,
    Finished,

    Count
};

struct Team
{
    uint32_t mScore = 0;
    Car* mCars[MAX_TEAM_SIZE] = {};
};

class MatchState : public Node3D
{
public:

    DECLARE_NODE(MatchState, Node3D);

    MatchState();
    virtual void Create() override;
    virtual void Destroy() override;
    virtual void Start() override;
    virtual void Tick(float deltaTime) override;
    virtual void GatherReplicatedData(std::vector<NetDatum>& outData) override;

    void HandleGoal(uint32_t scoringTeam);
    void AssignHostToCar(NetClient* client);

protected:

    void FindSpawnPointActors();
    void PostLoadHandlePlatformTier();
    void ResetMatchState();
    void SetupKickoff();
    void SetMatchPhase(MatchPhase phase);
    void EnableCarControl(bool enable);
    bool IsMatchOngoing() const;
    void AssignCarHostIds();

public:

    Ball* mBall = nullptr;
    Car* mCars[MAX_CARS] = {};
    Car* mOwnedCar = nullptr;
    uint32_t mNumCars = 0;

    float mTime = 0.0f;
    uint32_t mTeamSize = MAX_TEAM_SIZE;
    Team mTeams[NUM_TEAMS];
    MatchPhase mPhase = MatchPhase::Count;

    Node3D* mGoalBoxes[NUM_TEAMS] = {};
    Node3D* mFullBoosts[NUM_FULL_BOOSTS] = {};
    Node3D* mSpawnPoints0[NUM_SPAWN_POINTS] = {};
    Node3D* mSpawnPoints1[NUM_SPAWN_POINTS] = {};

    float mPhaseTime = 0.0f;
    bool mOvertime = false;


    // If editing, make sure to update ResetMatchState()
};
