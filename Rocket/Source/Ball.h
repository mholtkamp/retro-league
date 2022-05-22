#pragma once

#include "Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ShadowMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "Components/ParticleComponent.h"

class Ball : public Actor
{
public:

    DECLARE_ACTOR(Ball, Actor);

    Ball();
    ~Ball();

    virtual void Create() override;
    virtual void Tick(float deltaTime) override;
    virtual void GatherReplicatedData(std::vector<NetDatum>& outData) override;
    virtual void GatherNetFuncs(std::vector<NetFunc>& outFuncs) override;

    virtual void OnCollision(
        PrimitiveComponent* thisComp,
        PrimitiveComponent* otherComp,
        glm::vec3 impactPoint,
        glm::vec3 impactNormal,
        btPersistentManifold* manifold) override;

    virtual void BeginOverlap(
        PrimitiveComponent* thisComp,
        PrimitiveComponent* otherComp
    ) override;

    void Reset();
    void SetAlive(bool alive);

    static bool OnRep_Alive(Datum* datum, const void* value);

    static void M_GoalExplode(Actor* actor);

protected:

    StaticMeshComponent* mMeshComponent = nullptr;
    ShadowMeshComponent* mShadowComponent = nullptr;
    AudioComponent* mAudioComponent = nullptr;
    ParticleComponent* mParticleComponent = nullptr;

    SoundWaveRef mGoalSound;

    float mTimeSinceLastHit = 0.0f;
    float mTimeSinceLastGrounded = 0.0f;
    int32_t mLastHitTeam = -1;
    bool mGrounded = false;
    bool mAlive = true;
};