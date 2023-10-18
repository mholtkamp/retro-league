#pragma once

#include "Nodes/Node.h"

#include "Nodes/3D/StaticMesh3d.h"
#include "Nodes/3D/Sphere3d.h"
#include "Nodes/3D/Particle3d.h"

#include "Assets/SoundWave.h"

class BoostPickup : public Actor
{
public:

    DECLARE_ACTOR(BoostPickup, Actor);

    BoostPickup();
    virtual void Create() override;
    virtual void Tick(float deltaTime) override;
    virtual void BeginOverlap(Primitive3D* thisComp, Primitive3D* otherComp) override;
    virtual void GatherReplicatedData(std::vector<NetDatum>& outData) override;

    bool IsMini() const;
    void SetMini(bool mini);
    void SetAlive(bool alive);
    void Reset();

protected:

    StaticMesh3D* mMesh3D = nullptr;
    Sphere3D* mSphere3D = nullptr;
    Particle3D* mParticle3D = nullptr;

    float mSpawnTime = 0.0f;
    bool mMini = true;
    bool mAlive = true;
};