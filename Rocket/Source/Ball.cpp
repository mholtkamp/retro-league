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

DEFINE_NODE(Ball, Node3D);

bool Ball::OnRep_Alive(Datum* datum, uint32_t index, const void* value)
{
    Ball* ball = (Ball*) datum->mOwner;
    bool alive = *(bool*)value;
    ball->SetAlive(alive);
    return true;
}

void Ball::M_GoalExplode(Node* node)
{
    Ball* ball = (Ball*) node;

    ball->mParticle3D->EnableEmission(true);

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
    StaticMesh3D::Create();

    SetName("Ball");
    SetMass(1.0f);
    SetScale(glm::vec3(1.5f, 1.5f, 1.5f));
    EnablePhysics(NetIsAuthority());
    EnableCollision(true);
    EnableOverlaps(true);
    SetCollisionGroup(ColGroupBall);
    SetRestitution(0.4f);
    SetAngularFactor({ 1.0f, 1.0f, 1.0f });
    SetRollingFriction(1.0f);
    SetFriction(1.0f);
    EnableCastShadows(true);
    EnableReceiveSimpleShadows(false);
    //SetFriction(0.0f);
    //SetAngularFactor(glm::vec3(0.0f, 0.0f, 0.0f));
    //SetAngularDamping(1.0f);
    SetStaticMesh((StaticMesh*)LoadAsset("SM_Sphere"));

    Material* ballMat = (Material*)LoadAsset("M_Ball");
    ballMat->SetFresnelEnabled(true);
    ballMat->SetFresnelColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    SetMaterialOverride(ballMat);

    mShadowComponent = CreateChild<ShadowMesh3D>("Shadow");
    mShadowComponent->SetStaticMesh(LoadAsset<StaticMesh>("SM_Cone"));
    mShadowComponent->SetRotation(glm::vec3(180.0f, 0.0f, 0.0f));
    mShadowComponent->SetPosition(RootRelativeShadowPos);
    mShadowComponent->SetScale(glm::vec3(1.0f, 20.0f, 1.0f));

    mAudio3D = CreateChild<Audio3D>("Audio");
    mAudio3D->SetSoundWave((SoundWave*)LoadAsset("SW_Item"));
    mAudio3D->SetLoop(true);
    //mAudio3D->Play();

    mParticle3D = CreateChild<Particle3D>("Explosion");
    mParticle3D->SetParticleSystem((ParticleSystem*)LoadAsset("P_GoalExplosion"));
    mParticle3D->EnableEmission(false);
    mParticle3D->EnableAutoEmit(false);

    mGoalSound = LoadAsset("SW_Goal");
}

void Ball::Tick(float deltaTime)
{
    StaticMesh3D::Tick(deltaTime);

    if (NetIsAuthority())
    {
        mTimeSinceLastHit += deltaTime;
        mTimeSinceLastGrounded += deltaTime;

        if (mGrounded && mTimeSinceLastGrounded > 0.3f)
        {
            mGrounded = false;
        }

        glm::vec3 velocity = GetLinearVelocity();
        float speed = glm::length(velocity);
        glm::vec3 direction = (speed != 0.0f) ? velocity / speed : glm::vec3(0.0f);

        if (speed > BallSoftSpeedLimit)
        {
            if (speed > BallHardSpeedLimit)
            {
                speed = BallHardSpeedLimit;
            }

            speed = Maths::Damp(speed, BallSoftSpeedLimit, 0.005f, deltaTime);

            SetLinearVelocity(speed * direction);
        }
    }

    mShadowComponent->SetAbsoluteRotation(glm::vec3(180.0f, 0.0f, 0.0f));
    // TODO-NODE: Shouldn't the following line call SetPosition() instead of SetAbsolutePosition()
    mShadowComponent->SetAbsolutePosition(GetPosition() + RootRelativeShadowPos * GetScale());

    // Update fresnel color based on last hit.
    glm::vec4 fresnelColor = GetMaterial()->GetFresnelColor();
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
    GetMaterial()->SetFresnelColor(fresnelColor);
}

void Ball::GatherReplicatedData(std::vector<NetDatum>& outData)
{
    StaticMesh3D::GatherReplicatedData(outData);
    outData.push_back(NetDatum(DatumType::Bool, this, &mAlive, 1, OnRep_Alive));
    outData.push_back(NetDatum(DatumType::Integer, this, &mLastHitTeam, 1));
}

void Ball::GatherNetFuncs(std::vector<NetFunc>& outFuncs)
{
    StaticMesh3D::GatherNetFuncs(outFuncs);
    ADD_NET_FUNC(outFuncs, Multicast, M_GoalExplode);
}

void Ball::OnCollision(
    Primitive3D* thisComp,
    Primitive3D* otherComp,
    glm::vec3 impactPoint,
    glm::vec3 impactNormal,
    btPersistentManifold* manifold)
{
    if (NetIsAuthority())
    {
        Car* car = otherComp->As<Car>();

        if (car != nullptr)
        {
            glm::vec3 ballVelocity = GetLinearVelocity();
            glm::vec3 carVelocity = car->GetVelocity();

            // (1) Cancel out velocity along collision axis
            float canceledSpeed = -glm::dot(ballVelocity, impactNormal);
            ballVelocity = ballVelocity + impactNormal * canceledSpeed;

            // (2) Add the (scaled) component of the car velocity along the collision axis.
            float launchSpeed = glm::max(glm::length(carVelocity * BallLaunchSpeedMult), canceledSpeed * 0.5f);
            glm::vec3 launchVelocity = impactNormal * launchSpeed;
            ballVelocity += launchVelocity;

            SetLinearVelocity(ballVelocity);

    #if 0
            // Debug impact
            glm::vec3 lineStart = mMesh3D->GetAbsolutePosition();
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
    Primitive3D* thisComp,
    Primitive3D* otherComp
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
        SetLinearVelocity(glm::vec3(0));
        SetAngularVelocity(glm::vec3(0));
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
            EnablePhysics(alive);
        }

        EnableCollision(alive);
        EnableOverlaps(alive);
        SetVisible(alive);

        mShadowComponent->SetVisible(alive);
    }
}
