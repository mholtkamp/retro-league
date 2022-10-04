#include "Ball.h"
#include "Car.h"
#include "GameState.h"
#include "RocketTypes.h"

#include "InputDevices.h"
#include "AssetManager.h"
#include "Log.h"
#include "World.h"
#include "Utilities.h"
#include "AudioManager.h"
#include "NetworkManager.h"

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Bullet/btBulletDynamicsCommon.h"

const float BallLaunchSpeedMult = 1.5f;
const float BallSoftSpeedLimit = 30.0f;
const float BallHardSpeedLimit = 80.0f;

const glm::vec3 RootRelativeShadowPos = glm::vec3(0.0f, -20.0f, 0.0f);

DEFINE_ACTOR(Ball, Actor);

bool Ball::OnRep_Alive(Datum* datum, const void* value)
{
    Ball* ball = (Ball*) datum->mOwner;
    bool alive = *(bool*)value;
    ball->SetAlive(alive);
    return true;
}

void Ball::M_GoalExplode(Actor* actor)
{
    Ball* ball = (Ball*) actor;

    ball->mParticleComponent->EnableEmission(true);

    AudioManager::PlaySound3D(
        ball->mGoalSound.Get<SoundWave>(),
        ball->GetPosition(),
        0.0f,
        300.0f);
}

Ball::Ball()
{
    mReplicate = true;
    mReplicateTransform = true;
}

Ball::~Ball()
{

}

void Ball::Create()
{
    Actor::Create();
    SetName("Ball");

    mMeshComponent = CreateComponent<StaticMeshComponent>();
    SetRootComponent(mMeshComponent);
    mMeshComponent->SetName("Ball Mesh");
    mMeshComponent->SetMass(1.0f);
    mMeshComponent->SetScale(glm::vec3(1.5f, 1.5f, 1.5f));
    mMeshComponent->EnablePhysics(NetIsAuthority());
    mMeshComponent->EnableCollision(true);
    mMeshComponent->EnableOverlaps(true);
    mMeshComponent->SetCollisionGroup(ColGroupBall);
    mMeshComponent->SetRestitution(0.4f);
    mMeshComponent->SetAngularFactor({ 1.0f, 1.0f, 1.0f });
    mMeshComponent->SetRollingFriction(1.0f);
    mMeshComponent->SetFriction(1.0f);
    mMeshComponent->EnableCastShadows(true);
    mMeshComponent->EnableReceiveSimpleShadows(false);
    //mBodyComponent->SetFriction(0.0f);
    //mBodyComponent->SetAngularFactor(glm::vec3(0.0f, 0.0f, 0.0f));
    //mBodyComponent->SetAngularDamping(1.0f);
    mMeshComponent->SetStaticMesh((StaticMesh*)LoadAsset("SM_Sphere"));

    Material* ballMat = (Material*)LoadAsset("M_Ball");
    ballMat->SetFresnelEnabled(true);
    ballMat->SetFresnelColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    mMeshComponent->SetMaterialOverride(ballMat);

    mShadowComponent = CreateComponent<ShadowMeshComponent>();
    mShadowComponent->Attach(mMeshComponent);
    mShadowComponent->SetName("Shadow");
    mShadowComponent->SetStaticMesh(LoadAsset<StaticMesh>("SM_Cone"));
    mShadowComponent->SetRotation(glm::vec3(180.0f, 0.0f, 0.0f));
    mShadowComponent->SetPosition(RootRelativeShadowPos);
    mShadowComponent->SetScale(glm::vec3(1.0f, 20.0f, 1.0f));

    mAudioComponent = CreateComponent<AudioComponent>();
    mAudioComponent->SetName("Audio Comp");
    mAudioComponent->Attach(mMeshComponent);
    mAudioComponent->SetSoundWave((SoundWave*)LoadAsset("SW_Item"));
    mAudioComponent->SetLoop(true);
    //mAudioComponent->Play();

    mParticleComponent = CreateComponent<ParticleComponent>();
    mParticleComponent->SetName("Explosion");
    mParticleComponent->Attach(mMeshComponent);
    mParticleComponent->SetParticleSystem((ParticleSystem*)LoadAsset("P_GoalExplosion"));
    mParticleComponent->EnableEmission(false);

    mGoalSound = LoadAsset("SW_Goal");
}

void Ball::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);

    if (NetIsAuthority())
    {
        mTimeSinceLastHit += deltaTime;
        mTimeSinceLastGrounded += deltaTime;

        if (mGrounded && mTimeSinceLastGrounded > 0.3f)
        {
            mGrounded = false;
        }

        glm::vec3 velocity = mMeshComponent->GetLinearVelocity();
        float speed = glm::length(velocity);
        glm::vec3 direction = (speed != 0.0f) ? velocity / speed : glm::vec3(0.0f);

        if (speed > BallSoftSpeedLimit)
        {
            if (speed > BallHardSpeedLimit)
            {
                speed = BallHardSpeedLimit;
            }

            speed = Maths::Damp(speed, BallSoftSpeedLimit, 0.005f, deltaTime);

            mMeshComponent->SetLinearVelocity(speed * direction);
        }
    }

    mShadowComponent->SetAbsoluteRotation(glm::vec3(180.0f, 0.0f, 0.0f));
    mShadowComponent->SetAbsolutePosition(GetRootComponent()->GetPosition() + RootRelativeShadowPos * GetRootComponent()->GetScale());

    // Update fresnel color based on last hit.
    glm::vec4 fresnelColor = mMeshComponent->GetMaterial()->GetFresnelColor();
    glm::vec4 targetColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    if (mLastHitTeam == 0)
    {
        targetColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
    }
    else if (mLastHitTeam == 1)
    {
        targetColor = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);
    }

    fresnelColor = Maths::Damp(fresnelColor, targetColor, 0.005f, deltaTime);
    mMeshComponent->GetMaterial()->SetFresnelColor(fresnelColor);
}

void Ball::GatherReplicatedData(std::vector<NetDatum>& outData)
{
    Actor::GatherReplicatedData(outData);
    outData.push_back(NetDatum(DatumType::Bool, this, &mAlive, 1, OnRep_Alive));
    outData.push_back(NetDatum(DatumType::Integer, this, &mLastHitTeam, 1));
}

void Ball::GatherNetFuncs(std::vector<NetFunc>& outFuncs)
{
    Actor::GatherNetFuncs(outFuncs);
    ADD_NET_FUNC(outFuncs, Multicast, M_GoalExplode);
}

void Ball::OnCollision(
    PrimitiveComponent* thisComp,
    PrimitiveComponent* otherComp,
    glm::vec3 impactPoint,
    glm::vec3 impactNormal,
    btPersistentManifold* manifold)
{
    if (NetIsAuthority())
    {
        bool hitCar = (otherComp->GetOwner()->GetName() == "Car");

        if (hitCar)
        {
            Car* car = static_cast<Car*>(otherComp->GetOwner());

            glm::vec3 ballVelocity = mMeshComponent->GetLinearVelocity();
            glm::vec3 carVelocity = car->GetVelocity();

            // (1) Cancel out velocity along collision axis
            float canceledSpeed = -glm::dot(ballVelocity, impactNormal);
            ballVelocity = ballVelocity + impactNormal * canceledSpeed;

            // (2) Add the (scaled) component of the car velocity along the collision axis.
            float launchSpeed = glm::max(glm::length(carVelocity * BallLaunchSpeedMult), canceledSpeed * 0.5f);
            glm::vec3 launchVelocity = impactNormal * launchSpeed;
            ballVelocity += launchVelocity;

            mMeshComponent->SetLinearVelocity(ballVelocity);

    #if 0
            // Debug impact
            glm::vec3 lineStart = mMeshComponent->GetAbsolutePosition();
            glm::vec3 lineEnd = lineStart + 6.0f * impactNormal;
            Line debugLine(lineStart, lineEnd, glm::vec4(0.3f, 0.2f, 1.0f, 1.0f), 10.0f);
            GetWorld()->AddLine(debugLine);
    #endif

            // Update last hit team
            mLastHitTeam = car->GetTeamIndex();

            if (mTimeSinceLastHit > 0.3f)
            {
                //SoundWave* hitSound = (SoundWave*)LoadAsset("SW_Cannon");
                //AudioManager::PlaySound3D(
                //    hitSound,
                //    GetPosition(),
                //    0.0f,
                //    15.0f);
            }

            mTimeSinceLastHit = 0.0f;
        }
        else
        {
            if (impactNormal.y > 0.8f)
            {
                mGrounded = true;
                mTimeSinceLastGrounded = 0.0f;
            }
        }
    }
}

void Ball::BeginOverlap(
    PrimitiveComponent* thisComp,
    PrimitiveComponent* otherComp
)
{
    if (NetIsAuthority())
    {
        const char* otherName = otherComp->GetName().c_str();
        if (strncmp(otherName, "Goal", 4) == 0)
        {
            // Goal hit
            if (strlen(otherName) == 6)
            {
                uint32_t teamGoal = otherName[5] - '0';
                teamGoal = (teamGoal == 0 || teamGoal == 1) ? teamGoal : 0;
                uint32_t scoringTeam = (teamGoal + 1) % 2;

                GetMatchState()->HandleGoal(scoringTeam);
                SetAlive(false);
                InvokeNetFunc("M_GoalExplode");
            }
        }
    }
}

void Ball::Reset()
{
    if (NetIsAuthority())
    {
        SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        mMeshComponent->SetLinearVelocity(glm::vec3(0));
        mMeshComponent->SetAngularVelocity(glm::vec3(0));
        mLastHitTeam = -1;
        SetAlive(true);
    }
}

void Ball::SetAlive(bool alive)
{
    if (mAlive != alive)
    {
        mAlive = alive;

        if (NetIsAuthority())
        {
            mMeshComponent->EnablePhysics(alive);
        }

        mMeshComponent->EnableCollision(alive);
        mMeshComponent->EnableOverlaps(alive);
        mMeshComponent->SetVisible(alive);

        mShadowComponent->SetVisible(alive);
    }
}
