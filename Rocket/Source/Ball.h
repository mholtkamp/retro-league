#pragma once

#include "Nodes/Node.h"
#include "Nodes/3D/StaticMesh3d.h"
#include "Nodes/3D/ShadowMesh3d.h"
#include "Nodes/3D/Sphere3d.h"
#include "Nodes/3D/Audio3d.h"
#include "Nodes/3D/Particle3d.h"

#include "Assets/ParticleSystem.h"
#include "Assets/SoundWave.h"

class Ball : public StaticMesh3D
{
public:

    DECLARE_NODE(Ball, StaticMesh3D);

    Ball();
    ~Ball();

    virtual void Create() override;
    virtual void Tick(float deltaTime) override;
    virtual void GatherReplicatedData(std::vector<NetDatum>& outData) override;
    virtual void GatherNetFuncs(std::vector<NetFunc>& outFuncs) override;

    virtual void OnCollision(
        Primitive3D* thisComp,
        Primitive3D* otherComp,
        glm::vec3 impactPoint,
        glm::vec3 impactNormal,
        btPersistentManifold* manifold) override;

    virtual void BeginOverlap(
        Primitive3D* thisComp,
        Primitive3D* otherComp
    ) override;

    void Reset();
    void SetAlive(bool alive);

    static bool OnRep_Alive(Datum* datum, uint32_t index, const void* value);

    static void M_GoalExplode(Node* node);

protected:

    ShadowMesh3D* mShadowComponent = nullptr;
    Audio3D* mAudio3D = nullptr;

    ParticleSystemRef mGoalParticle;
    SoundWaveRef mGoalSound;

    float mTimeSinceLastHit = 0.0f;
    float mTimeSinceLastGrounded = 0.0f;
    int32_t mLastHitTeam = -1;
    bool mGrounded = false;
    bool mAlive = true;
};