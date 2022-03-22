#pragma once

#include "Actor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/ParticleComponent.h"

#include "Assets/SoundWave.h"

class BoostPickup : public Actor
{
public:

    DECLARE_ACTOR(BoostPickup, Actor);

    BoostPickup();
    virtual void Create() override;
    virtual void Tick(float deltaTime) override;
    virtual void BeginOverlap(PrimitiveComponent* thisComp, PrimitiveComponent* otherComp) override;
    virtual void GatherReplicatedData(std::vector<NetDatum>& outData) override;

    bool IsMini() const;
    void SetMini(bool mini);
    void SetAlive(bool alive);
    void Reset();

protected:

    StaticMeshComponent* mMeshComponent = nullptr;
    SphereComponent* mSphereComponent = nullptr;
    ParticleComponent* mParticleComponent = nullptr;

    float mSpawnTime = 0.0f;
    bool mMini = true;
    bool mAlive = true;
};