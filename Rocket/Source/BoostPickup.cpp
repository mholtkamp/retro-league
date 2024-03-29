#include "BoostPickup.h"
#include "Car.h"
#include "Log.h"

#include "AudioManager.h"
#include "AssetManager.h"
#include "NetworkManager.h"

DEFINE_NODE(BoostPickup, Node3D);

bool OnRep_Alive(Datum* datum, uint32_t index, const void* value)
{
    BoostPickup* bp = (BoostPickup*) datum->mOwner;
    bool alive = *(bool*) value;
    bp->SetAlive(alive);
    return true;
}

bool OnRep_Mini(Datum* datum, uint32_t index, const void* value)
{
    BoostPickup* bp = (BoostPickup*) datum->mOwner;
    bool mini = *(bool*) value;
    bp->SetMini(mini);
    return true;
}

BoostPickup::BoostPickup()
{
    mReplicate = true;
    mReplicateTransform = true;
}

void BoostPickup::Create()
{
    Node3D::Create();
    SetName("Boost");

    mSphere3D = CreateChild<Sphere3D>("Boost Pickup Sphere");
    mSphere3D->EnablePhysics(false);
    mSphere3D->EnableCollision(false);
    mSphere3D->EnableOverlaps(true);

    mMesh3D = CreateChild<StaticMesh3D>("Boost Pickup Mesh");
    mMesh3D->EnableOverlaps(false);
    mMesh3D->EnableCollision(false);
    mMesh3D->EnablePhysics(false);
    mMesh3D->SetStaticMesh(mMini ? (StaticMesh*)LoadAsset("SM_MiniBoost") : (StaticMesh*)LoadAsset("SM_Sphere"));
    mMesh3D->SetMaterialOverride((Material*)LoadAsset("M_BoostPickup"));

    mParticle3D = CreateChild<Particle3D>("Boost Pickup Particle");
    mParticle3D->SetParticleSystem((ParticleSystem*)LoadAsset("P_BoostPickup"));
    mParticle3D->EnableEmission(false);
    mParticle3D->EnableAutoEmit(false);
}

void BoostPickup::Tick(float deltaTime)
{
    Node3D::Tick(deltaTime);

    if (NetIsAuthority() &&
        mSpawnTime > 0.0f)
    {
        mSpawnTime -= deltaTime;

        if (mSpawnTime <= 0.0f)
        {
            Reset();
        }
    }
}

void BoostPickup::BeginOverlap(Primitive3D* thisComp, Primitive3D* otherComp)
{
    if (NetIsAuthority() &&
        mSpawnTime <= 0.0f &&
        otherComp->Is(Car::ClassRuntimeId()))
    {
        Car* car = (Car*)otherComp;
        car->AddBoostFuel(mMini ? 10.0f : 100.0f);

        SetAlive(false);
    }
}

void BoostPickup::GatherReplicatedData(std::vector<NetDatum>& outData)
{
    Node3D::GatherReplicatedData(outData);
    outData.push_back(NetDatum(DatumType::Bool, this, &mMini, 1, OnRep_Mini));
    outData.push_back(NetDatum(DatumType::Bool, this, &mAlive, 1, OnRep_Alive));
}

void BoostPickup::SetMini(bool mini)
{
    if (mMini != mini)
    {
        mMini = mini;

        if (mini)
        {
            mMesh3D->SetStaticMesh((StaticMesh*)LoadAsset("SM_MiniBoost"));
            mMesh3D->SetScale(glm::vec3(1.0f));
        }
        else
        {
            mMesh3D->SetStaticMesh((StaticMesh*)LoadAsset("SM_Sphere"));
            mMesh3D->SetScale(glm::vec3(1.0f));
        }
    }
}

void BoostPickup::SetAlive(bool alive)
{
    if (mAlive != alive)
    {
        mAlive = alive;
        mMesh3D->SetVisible(alive);
        mSphere3D->EnableOverlaps(alive);

        if (alive)
        {
            mSpawnTime = 0.0f;
        }
        else
        {
            mSpawnTime = mMini ? 4.0f : 10.0f;
            mParticle3D->EnableEmission(true);
        }
    }
}

bool BoostPickup::IsMini() const
{
    return mMini;
}

void BoostPickup::Reset()
{
    SetAlive(true);
}
