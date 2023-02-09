#include "BoostPickup.h"
#include "Car.h"
#include "Log.h"

#include "AudioManager.h"
#include "AssetManager.h"
#include "NetworkManager.h"

DEFINE_ACTOR(BoostPickup, Actor);

bool OnRep_Alive(Datum* datum, const void* value)
{
    BoostPickup* bp = (BoostPickup*) datum->mOwner;
    bool alive = *(bool*) value;
    bp->SetAlive(alive);
    return true;
}

bool OnRep_Mini(Datum* datum, const void* value)
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
    Actor::Create();
    SetName("Boost");

    mSphereComponent = CreateComponent<SphereComponent>();
    SetRootComponent(mSphereComponent);
    mSphereComponent->SetName("Boost Pickup Sphere");
    mSphereComponent->EnablePhysics(false);
    mSphereComponent->EnableCollision(false);
    mSphereComponent->EnableOverlaps(true);

    mMeshComponent = CreateComponent<StaticMeshComponent>();
    mMeshComponent->Attach(mSphereComponent);
    mMeshComponent->SetName("Boost Pickup Mesh");
    mMeshComponent->EnableOverlaps(false);
    mMeshComponent->EnableCollision(false);
    mMeshComponent->EnablePhysics(false);
    mMeshComponent->SetStaticMesh(mMini ? (StaticMesh*)LoadAsset("SM_MiniBoost") : (StaticMesh*)LoadAsset("SM_Sphere"));
    mMeshComponent->SetMaterialOverride((Material*)LoadAsset("M_BoostPickup"));

    mParticleComponent = CreateComponent<ParticleComponent>();
    mParticleComponent->Attach(mSphereComponent);
    mParticleComponent->SetName("Boost Pickup Particle");
    mParticleComponent->SetParticleSystem((ParticleSystem*)LoadAsset("P_BoostPickup"));
    mParticleComponent->EnableEmission(false);
    mParticleComponent->EnableAutoEmit(false);
}

void BoostPickup::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);

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

void BoostPickup::BeginOverlap(PrimitiveComponent* thisComp, PrimitiveComponent* otherComp)
{
    if (NetIsAuthority() &&
        mSpawnTime <= 0.0f &&
        otherComp->GetOwner()->GetName() == "Car")
    {
        Car* car = (Car*)otherComp->GetOwner();
        car->AddBoostFuel(mMini ? 10.0f : 100.0f);

        SetAlive(false);
    }
}

void BoostPickup::GatherReplicatedData(std::vector<NetDatum>& outData)
{
    Actor::GatherReplicatedData(outData);
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
            mMeshComponent->SetStaticMesh((StaticMesh*)LoadAsset("SM_MiniBoost"));
            mMeshComponent->SetScale(glm::vec3(1.0f));
        }
        else
        {
            mMeshComponent->SetStaticMesh((StaticMesh*)LoadAsset("SM_Sphere"));
            mMeshComponent->SetScale(glm::vec3(1.0f));
        }
    }
}

void BoostPickup::SetAlive(bool alive)
{
    if (mAlive != alive)
    {
        mAlive = alive;
        mMeshComponent->SetVisible(alive);
        mSphereComponent->EnableOverlaps(alive);

        if (alive)
        {
            mSpawnTime = 0.0f;
        }
        else
        {
            mSpawnTime = mMini ? 4.0f : 10.0f;
            mParticleComponent->EnableEmission(true);
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
