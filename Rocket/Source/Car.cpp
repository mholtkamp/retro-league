#define _CRT_SECURE_NO_WARNINGS 1

#include "Car.h"
#include "Ball.h"
#include "RocketTypes.h"
#include "GameState.h"

#include "InputDevices.h"
#include "AudioManager.h"
#include "AssetManager.h"
#include "NetworkManager.h"
#include "Log.h"
#include "Profiler.h"
#include "World.h"
#include "Utilities.h"
#include "Maths.h"
#include "Assets/SkeletalMesh.h"

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Bullet/btBulletDynamicsCommon.h"

const float CameraHeight = 2.0f;
const float CameraDistanceXZ = 6.5f;
const float CameraSpeed = 800.0f;
const float SpeedLimit = 20.0f;
const float BoostSpeedLimit = 40.0f;
const float BoostDepletionSpeed = 50.0f;
const float StartingBoost = 35.0f;
const float DefaultGravity = 9.8f;
const float WallGravity = 8 * 9.8f;
const float JumpSpeed = 8.0f;
const float SpinJumpSpeed = JumpSpeed * 2.0f;
const float DefaultAcceleration = 30.0f;
const float BoostAcceleration = DefaultAcceleration;
const float AerialBoostAcceleration = DefaultAcceleration * 0.8f;
const float DoubleJumpTimeLimit = 2.0f;
const float DoubleJumpSpinDuration = 0.8f;
const float GroundedTurnRate = 2.0f;
const float SlideTurnRate = 3.0f * GroundedTurnRate;
const float AerialTurnRate = GroundedTurnRate;
const float DemoSpeedReq = 35.0f;
const float DemoSpeedDiff = 5.0f;

const glm::vec3 RootRelativeShadowPos = glm::vec3(0.0f, -2.3f, 0.0f);

DEFINE_NODE(Car, Sphere3D);

bool Car::OnRep_NetPosition(Datum* datum, uint32_t index, const void* newValue)
{
    Car* car = (Car*) datum->mOwner;
    OCT_ASSERT(car);

    if (!car->IsLocallyControlled() || !car->mInitialPosSet)
    {
        car->mInitialPosSet = true;
        return Sphere3D::OnRep_RootPosition(datum, index, newValue);
    }
    else
    {
        // We don't want to assign the position if this is locally controlled.
        return true;
    }
}

bool Car::OnRep_NetRotation(Datum* datum, uint32_t index, const void* newValue)
{
    Car* car = (Car*) datum->mOwner;
    OCT_ASSERT(car);

    if (!car->IsLocallyControlled() || !car->mInitialRotSet)
    {
        car->mInitialRotSet = true;
        return Sphere3D::OnRep_RootRotation(datum, index, newValue);
    }
    else
    {
        // We don't want to assign the rotation if this is locally controlled.
        return true;
    }
}

bool Car::OnRep_OwningHost(Datum* datum, uint32_t index, const void* newValue)
{
    OCT_ASSERT(datum->mOwner != nullptr);
    Car* car = static_cast<Car*>(datum->mOwner);
    car->mOwningHost = *((uint8_t*) newValue);

    if (car->mOwningHost == NetGetHostId())
    {
        car->GetWorld()->SetActiveCamera(car->mCamera3D);

        if (GetMatchState())
        {
            GetMatchState()->mOwnedCar = car;
        }
    }

    return true;
}

bool Car::OnRep_Alive(Datum* datum, uint32_t index, const void* newValue)
{
    Car* car = (Car*) datum->mOwner;
    bool alive = *(bool*)newValue;
    car->mAlive = alive;

    car->EnableCollision(alive);
    car->EnableOverlaps(alive);
    car->mMesh3D->SetVisible(alive);
    car->mShadowComponent->SetVisible(alive);

    if (!alive)
    {
        car->SetVelocity(glm::vec3(0));
    }

    return true;
}

bool Car::OnRep_Boosting(Datum* datum, uint32_t index, const void* newValue)
{
    Car* car = (Car*) datum->mOwner;
    bool boosting = *(bool*) newValue;

    if (!car->IsLocallyControlled())
    {
        car->SetBoosting(boosting);
    }

    return true;
}

bool Car::OnRep_TeamIndex(Datum* datum, uint32_t index, const void* newValue)
{
    Car* car = (Car*) datum->mOwner;
    int32_t teamIndex = *(int32_t*) newValue;
    car->SetTeamIndex(teamIndex);
    return true;
}

void Car::S_UploadState(Node* node, Datum& vecPosition, Datum& vecRotation, Datum& vecVelocity, Datum& bBoosting)
{
    OCT_ASSERT(NetIsServer());
    Car* car = (Car*)node;
    glm::vec3 position = vecPosition.GetVector();
    glm::vec3 rotation = vecRotation.GetVector();
    glm::vec3 velocity = vecVelocity.GetVector();
    bool boosting = bBoosting.GetBool();
    car->SetPosition(position);
    car->SetRotation(rotation);
    car->SetVelocity(velocity);
    car->SetBoosting(boosting);
}

void Car::C_ForceVelocity(Node* node, Datum& vecVelocity)
{
    Car* car = (Car*)node;
    glm::vec3 velocity = vecVelocity.GetVector();
    car->SetVelocity(velocity);    
}

void Car::C_ForceTransform(Node* node, Datum& vecPosition, Datum& vecRotation)
{
    Car* car = (Car*)node;
    glm::vec3 position = vecPosition.GetVector();
    glm::vec3 rotation = vecRotation.GetVector();
    car->SetPosition(position);
    car->SetRotation(rotation);
}

void Car::C_AddBoostFuel(Node* node, Datum& fBoostFuel)
{
    Car* car = (Car*)node;
    float boostFuel = fBoostFuel.GetFloat();
    car->mBoostFuel += boostFuel;
    car->mBoostFuel = glm::clamp(car->mBoostFuel, 0.0f, 100.0f);

    if (car->IsLocallyControlled())
    {
        float pitch = glm::clamp(1.0f + car->GetBoostFuel() / 100.0f, 1.0f, 2.0f);
        AudioManager::PlaySound3D(car->mBoostPickupSound.Get<SoundWave>(), car->GetPosition(), 5.0f, 15.0f, AttenuationFunc::Linear, 1.0f, pitch);
    }
}

void Car::C_ResetState(Node* node)
{
    Car* car = (Car*)node;
    car->ResetState();
}

void Car::M_Demolish(Node* node)
{
    Car* car = (Car*)node;
    car->mDemoComponent->SetActive(true);
    car->mDemoComponent->EnableEmission(true);
}

Car::Car()
{
    mReplicate = true;

    // Handle transform ourselves since we want to skip replication updates for locally controlled cars.
    mReplicateTransform = false;
}

Car::~Car()
{

}

void Car::Create()
{
    Sphere3D::Create();

    SetName("Car");
    SetRadius(1.0f);
    SetMass(100.0f);
    EnablePhysics(false);
    EnableCollision(true);
    EnableOverlaps(true);
    SetCollisionGroup(ColGroupCar);

    SkeletalMesh* skeletalMesh = (SkeletalMesh*)LoadAsset("SK_CarM2");
    mMesh3D = CreateChild<SkeletalMesh3D>("Mesh");
    mMesh3D->SetSkeletalMesh(skeletalMesh);
    mMesh3D->EnablePhysics(false);
    mMesh3D->EnableCollision(false);
    mMesh3D->EnableOverlaps(false);
    mMesh3D->EnableCastShadows(true);
    mMesh3D->EnableReceiveSimpleShadows(false);

    mShadowComponent = CreateChild<ShadowMesh3D>("Shadow");
    mShadowComponent->SetStaticMesh(LoadAsset<StaticMesh>("SM_Cone"));
    mShadowComponent->SetRotation(glm::vec3(180.0f, 0.0f, 0.0f));
    mShadowComponent->SetPosition(RootRelativeShadowPos);
    mShadowComponent->SetScale(glm::vec3(1.4f, 2.0f, 1.4f));

    mCamera3D = CreateChild<Camera3D>("Camera");
    mCamera3D->SetPosition(glm::vec3(0.0f, CameraHeight, CameraDistanceXZ));
    mCamera3D->SetRotation(glm::vec3(5.0f, 0.0f, 0.0f));

    mTrailComponent = CreateChild<Particle3D>("Trail Particle");
    mTrailComponent->SetPosition(glm::vec3(0.0f, 0.0f, 0.5f));
    mTrailComponent->SetParticleSystem((ParticleSystem*)LoadAsset("P_Trail"));
    mTrailComponent->EnableEmission(false);
    mTrailComponent->EnableAutoEmit(false);

    mDemoComponent = CreateChild<Particle3D>("Explosion Particle");
    mDemoComponent->SetPosition(glm::vec3(0.0f, 0.0f, 0.5f));
    mDemoComponent->SetParticleSystem((ParticleSystem*)LoadAsset("P_DemoExplosion"));
    mDemoComponent->EnableEmission(false);
    mDemoComponent->EnableAutoEmit(false);
    mDemoComponent->SetActive(false);

    mEngineAudio3D = CreateChild<Audio3D>("Engine Audio");
    mEngineAudio3D->SetInnerRadius(5.0f);
    mEngineAudio3D->SetOuterRadius(30.0f);
    mEngineAudio3D->SetSoundWave((SoundWave*)LoadAsset("SW_EngineLoop"));
    mEngineAudio3D->SetLoop(true);
    mEngineAudio3D->PlayAudio();

    mBoostAudio3D = CreateChild<Audio3D>("Boost Audio");
    mBoostAudio3D->SetInnerRadius(5.0f);
    mBoostAudio3D->SetOuterRadius(30.0f);
    mBoostAudio3D->SetSoundWave((SoundWave*)LoadAsset("SW_Boost"));
    mBoostAudio3D->SetLoop(false);

    mJumpAudio3D = CreateChild<Audio3D>("Jump Audio");
    mJumpAudio3D->SetInnerRadius(5.0f);
    mJumpAudio3D->SetOuterRadius(20.0f);
    mJumpAudio3D->SetSoundWave((SoundWave*)LoadAsset("SW_Jump"));
    mJumpAudio3D->SetLoop(false);

    mBumpSound = LoadAsset("SW_Bump");
    mBoostPickupSound = LoadAsset("SW_BoostPickup");

    mBoneFenderL = skeletalMesh->FindBoneIndex("Fender.L");
    mBoneFenderR = skeletalMesh->FindBoneIndex("Fender.R");
    mBoneWheelFL = skeletalMesh->FindBoneIndex("FrontTire.L");
    mBoneWheelFR = skeletalMesh->FindBoneIndex("FrontTire.R");
    mBoneWheelBL = skeletalMesh->FindBoneIndex("BackTire.L");
    mBoneWheelBR = skeletalMesh->FindBoneIndex("BackTire.R");

    // SK_Car has a Wiggle animation that can be used for testing. Doesn't loop correctly.
    //mMesh3D->SetAnimation("Wiggle");
    //mMesh3D->Play();

    OCT_ASSERT(
        mBoneFenderL != -1 &&
        mBoneFenderR != -1 &&
        mBoneWheelFL != -1 &&
        mBoneWheelFR != -1 &&
        mBoneWheelBL != -1 &&
        mBoneWheelBR != -1);
}

void Car::Tick(float deltaTime)
{
    Sphere3D::Tick(deltaTime);

    if (IsLocallyControlled() ||
        (NetIsAuthority() && IsBot()))
    {
        if (IsBot())
        {
            UpdateBotInput(deltaTime);
        }
        else
        {
            UpdateInput(deltaTime);
        }

        UpdateBoost(deltaTime);
        UpdateRotation(deltaTime);
        UpdateVelocity(deltaTime);
        UpdateJump(deltaTime);
        UpdateMotion(deltaTime);
        UpdateGrounded(deltaTime);
        UpdateDebug(deltaTime);
        UpdateCamera(deltaTime);

        if (IsLocallyControlled() &&
            NetIsClient())
        {
            // Upload our new transform to the server
            InvokeNetFunc("S_UploadState", GetPosition(), GetRotationEuler(), mVelocity, mBoosting);
        }
    }
    else if (NetIsAuthority() &&
        !IsLocallyControlled())
    {
        // Update motion on client cars to determine demolition/bumps/ballhits.
        UpdateMotion(deltaTime);
    }

    if (NetIsAuthority())
    {
        UpdateRespawn(deltaTime);
    }

    UpdateAudio(deltaTime);

    mShadowComponent->SetAbsoluteRotation(glm::vec3(180.0f, 0.0f, 0.0f));
    // TODO-NODE: Shouldn't the following line call SetPosition() instead of SetAbsolutePosition()
    mShadowComponent->SetAbsolutePosition(GetPosition() + RootRelativeShadowPos);
}

void Car::GatherReplicatedData(std::vector<NetDatum>& outData)
{
    Sphere3D::GatherReplicatedData(outData);

    // Add OnRep callback for mOwningHost
    for (uint32_t i = 0; i < outData.size(); ++i)
    {
        if (outData[i].mData.vp == &mOwningHost)
        {
            outData[i].mChangeHandler = OnRep_OwningHost;
            break;
        }
    }

    outData.push_back(NetDatum(DatumType::Vector, this, &mPosition, 1, OnRep_NetPosition));
    outData.push_back(NetDatum(DatumType::Vector, this, &mRotationEuler, 1, OnRep_NetRotation));
    outData.push_back(NetDatum(DatumType::Integer, this, &mTeamIndex, 1, OnRep_TeamIndex));
    outData.push_back(NetDatum(DatumType::Bool, this, &mControlEnabled, 1, nullptr, true));
    outData.push_back(NetDatum(DatumType::Bool, this, &mAlive, 1, OnRep_Alive, true));
    outData.push_back(NetDatum(DatumType::Bool, this, &mBoosting, 1 , OnRep_Boosting, true));
}

void Car::GatherNetFuncs(std::vector<NetFunc>& outFuncs)
{
    Sphere3D::GatherNetFuncs(outFuncs);
    ADD_NET_FUNC(outFuncs, Server, S_UploadState);
    ADD_NET_FUNC_RELIABLE(outFuncs, Client, C_ForceVelocity);
    ADD_NET_FUNC_RELIABLE(outFuncs, Client, C_ForceTransform);
    ADD_NET_FUNC_RELIABLE(outFuncs, Client, C_AddBoostFuel);
    ADD_NET_FUNC_RELIABLE(outFuncs, Client, C_ResetState);
    ADD_NET_FUNC_RELIABLE(outFuncs, Multicast, M_Demolish);

}

void Car::HandleCollision(
    Primitive3D* thisComp,
    Primitive3D* otherComp,
    glm::vec3 impactPoint,
    glm::vec3 impactNormal,
    btPersistentManifold* manifold)
{

    float landingDot = glm::dot(GetUpVector(), impactNormal);
    if (impactNormal.y > -0.5f &&
        landingDot > 0.5f)
    {
        mGrounded = true;
        mDoubleJump = true;
        mSurfaceAligned = true;
        mTimeSinceLastGrounding = 0.0f;
        mSurfaceNormal = impactNormal;
    }
    else if (impactNormal.y > 0.5f)
    {
        // In the case that we aren't aligned but we hit something that is mostly a floor,
        // we still want to update the surface normal and begin to flip the car upright.
        mSurfaceNormal = impactNormal;
        mGrounded = true;
        mDoubleJump = true;
        mSurfaceAligned = false;
        mTimeSinceLastGrounding = 0.0f;
    }

    bool bumped = false;

    if (otherComp->Is(Ball::ClassRuntimeId()))
    {
        Ball* ball = static_cast<Ball*>(otherComp);
        ball->OnCollision(otherComp, thisComp, impactPoint, -impactNormal, nullptr);
        bumped = true;
    }
    else if (otherComp->Is(Car::ClassRuntimeId()))
    {
        Car* otherCar = static_cast<Car*>(otherComp);

        float thisDot = fabs(glm::dot(impactNormal, mVelocity));
        float otherDot = fabs(glm::dot(impactNormal, otherCar->GetVelocity()));

        if (NetIsAuthority() &&
            mTeamIndex != otherCar->GetTeamIndex() &&
            thisDot > DemoSpeedReq &&
            thisDot > (otherDot + DemoSpeedDiff))
        {
            // Demo
            otherCar->Kill();

            // Play demo particle
            otherCar->InvokeNetFunc("M_Demolish");
        }
        else
        {
            // Bump
            if (NetIsAuthority())
            {
                float newSpeed = (glm::length(mVelocity) + glm::length(otherCar->GetVelocity())) / 2.0f;
                newSpeed = glm::max(newSpeed, 20.0f);
                ForceVelocity(newSpeed * impactNormal);
                otherCar->ForceVelocity(newSpeed * -impactNormal);
            }
            bumped = true;
        }
    }

    if (bumped)
    {
        AudioManager::PlaySound3D(mBumpSound.Get<SoundWave>(), GetPosition(), 3.0f, 30.0f);
    }
}

SkeletalMesh3D* Car::GetMesh3D()
{
    return mMesh3D;
}

Camera3D* Car::GetCamera3D()
{
    return mCamera3D;
}

void Car::Reset()
{
    InvokeNetFunc("C_ResetState");

    mMesh3D->SetVisible(true);
    mShadowComponent->SetVisible(true);

    Respawn();

    mBotTargetType = BotTargetType::Count;
}

void Car::SetVelocity(glm::vec3 velocity)
{
    mVelocity = velocity;
}

glm::vec3 Car::GetVelocity() const
{
    return mVelocity;
}

void Car::ForceVelocity(glm::vec3 velocity)
{
    OCT_ASSERT(NetIsAuthority());
    SetVelocity(velocity);
    InvokeNetFunc("C_ForceVelocity", mVelocity);
}

bool Car::IsBot() const
{
    return mBot;
}

void Car::SetBot(bool bot)
{
    mBot = bot;
}

void Car::SetBotBehavior(BotBehavior mode)
{
    mBotBehavior = mode;
}

BotBehavior Car::GetBotBehavior() const
{
    return mBotBehavior;
}

void Car::ForceBotTargetBall()
{
    mBotTargetType = BotTargetType::Ball;
    mBotTargetActor = GetMatchState()->mBall;
    mBotTargetPosition = {};
    mBotTargetTime = 0.0f;
}

int32_t Car::GetCarIndex() const
{
    return mCarIndex;
}

int32_t Car::GetTeamIndex() const
{
    return mTeamIndex;
}

void Car::SetTeamIndex(int32_t index)
{
    mTeamIndex = index;

    Material* colorMat = nullptr;
    if (mTeamIndex == 0)
    {
        colorMat = (Material*)LoadAsset("M_CarM2_Orange");
    }
    else
    {
        colorMat = (Material*)LoadAsset("M_CarM2_Blue");
    }

    mMesh3D->SetMaterialOverride(colorMat);
}

void Car::SetCarIndex(int32_t index)
{
    mCarIndex = index;
}

bool Car::IsControlEnabled() const
{
    return mControlEnabled;
}

void Car::EnableControl(bool enable)
{
    mControlEnabled = enable;
}

void Car::AddBoostFuel(float boost)
{
    InvokeNetFunc("C_AddBoostFuel", boost);
}

float Car::GetBoostFuel() const
{
    return mBoostFuel;
}

void Car::Kill()
{
    OCT_ASSERT(NetIsAuthority());

    if (mAlive)
    {
        mAlive = false;
        SetVelocity(glm::vec3(0));
        EnableControl(false);

        EnableCollision(false);
        EnableOverlaps(false);

        mMesh3D->SetVisible(false);
        mShadowComponent->SetVisible(false);

        mRespawnTime = 4.0f;
    }
}

void Car::Respawn()
{
    OCT_ASSERT(NetIsAuthority());
    if (!mAlive)
    {
        mAlive = true;
        SetVelocity(glm::vec3(0));
        EnableControl(true);

        EnableCollision(true);
        EnableOverlaps(true);

        mMesh3D->SetVisible(true);
        mShadowComponent->SetVisible(true);

        mRespawnTime = 0.0f;

        mDemoComponent->EnableEmission(false);
        mDemoComponent->SetActive(false);
    }
}

bool Car::IsLocallyControlled() const
{
    if (NetIsLocal())
    {
        return mCarIndex == 0;
    }
    else
    {
        return mOwningHost == NetGetHostId();
    }
}

void Car::UpdateBotInput(float deltaTime)
{
    mPreviousInput = mCurrentInput;

    if (mControlEnabled)
    {
        // (1) Acquire a target.
        BotUpdateTarget(deltaTime);

        // (2) Adjust steering and acceleration
        BotUpdateHandling(deltaTime);
    }
    else
    {
        mCurrentInput = {};
    }
}

void Car::UpdateInput(float deltaTime)
{
    mPreviousInput = mCurrentInput;

    if (mControlEnabled)
    {
        if (GetGamepadType(0) == GamepadType::Wiimote)
        {
            // Axes
            mCurrentInput.mMotionX = GetGamepadAxisValue(GAMEPAD_AXIS_LTHUMB_X, 0);
            mCurrentInput.mMotionY = GetGamepadAxisValue(GAMEPAD_AXIS_LTHUMB_Y, 0);
            mCurrentInput.mCameraX = 0.0f;
            mCurrentInput.mCameraY = 0.0f;
            mCurrentInput.mAccelerate = IsGamepadButtonDown(GAMEPAD_B, 0) ? 1.0f : 0.0f;
            mCurrentInput.mReverse = IsGamepadButtonDown(GAMEPAD_Z, 0) ? 1.0f : 0.0f;

            // Buttons
            mCurrentInput.mJump = IsGamepadButtonDown(GAMEPAD_A, 0);
            mCurrentInput.mBoost = IsGamepadButtonDown(GAMEPAD_C, 0);
            mCurrentInput.mSlide = IsGamepadButtonDown(GAMEPAD_DOWN, 0);
            mCurrentInput.mBallCam = IsGamepadButtonDown(GAMEPAD_UP, 0);
            mCurrentInput.mMenu = IsGamepadButtonDown(GAMEPAD_START, 0);
        }
        else
        {
            // Axes
            mCurrentInput.mMotionX = GetGamepadAxisValue(GAMEPAD_AXIS_LTHUMB_X, 0);
            mCurrentInput.mMotionY = GetGamepadAxisValue(GAMEPAD_AXIS_LTHUMB_Y, 0);
            mCurrentInput.mCameraX = GetGamepadAxisValue(GAMEPAD_AXIS_RTHUMB_X, 0);
            mCurrentInput.mCameraY = GetGamepadAxisValue(GAMEPAD_AXIS_RTHUMB_Y, 0);
            mCurrentInput.mAccelerate = GetGamepadAxisValue(GAMEPAD_AXIS_RTRIGGER, 0);
            mCurrentInput.mReverse = GetGamepadAxisValue(GAMEPAD_AXIS_LTRIGGER, 0);

            // Buttons
            mCurrentInput.mJump = IsGamepadButtonDown(GAMEPAD_A, 0);
            mCurrentInput.mBoost = IsGamepadButtonDown(GAMEPAD_B, 0);
            mCurrentInput.mSlide = IsGamepadButtonDown(GAMEPAD_X, 0);
            mCurrentInput.mBallCam = IsGamepadButtonDown(GAMEPAD_Y, 0);
            mCurrentInput.mMenu = IsGamepadButtonDown(GAMEPAD_START, 0);
        }

#if PLATFORM_WINDOWS || PLATFORM_LINUX
        mCurrentInput.mMotionX = IsKeyDown(KEY_RIGHT) ? 1.0f : mCurrentInput.mMotionX;
        mCurrentInput.mMotionX = IsKeyDown(KEY_LEFT) ? -1.0f : mCurrentInput.mMotionX;
        mCurrentInput.mMotionY = IsKeyDown(KEY_UP) ? 1.0f : mCurrentInput.mMotionY;
        mCurrentInput.mMotionY = IsKeyDown(KEY_DOWN) ? -1.0f : mCurrentInput.mMotionY;
        mCurrentInput.mAccelerate = IsKeyDown(KEY_Z) ? 1.0f : mCurrentInput.mAccelerate;
        mCurrentInput.mReverse = IsKeyDown(KEY_SHIFT_L) ? 1.0f : mCurrentInput.mReverse;

        mCurrentInput.mJump = IsKeyDown(KEY_C) ? true : mCurrentInput.mJump;
        mCurrentInput.mBoost = IsKeyDown(KEY_X) ? true : mCurrentInput.mBoost;
        mCurrentInput.mSlide = IsKeyDown(KEY_S) ? true : mCurrentInput.mSlide;
        mCurrentInput.mBallCam = IsKeyDown(KEY_A) ? true : mCurrentInput.mBallCam;
        mCurrentInput.mMenu = IsKeyDown(KEY_ENTER) ? true : mCurrentInput.mMenu;
#endif
    }
    else
    {
        mCurrentInput = {};
    }
}

void Car::UpdateRespawn(float deltaTime)
{
    if (!mAlive)
    {
        mRespawnTime -= deltaTime;

        if (mRespawnTime <= 0.0f)
        {
            Respawn();
            MoveToRandomSpawnPoint();
        }
    }
}

void Car::UpdateBoost(float deltaTime)
{
    if (mCurrentInput.mBoost &&
        mBoostFuel > 0.0f)
    {
        SetBoosting(true);

        mBoostFuel -= deltaTime * BoostDepletionSpeed;
        mBoostFuel = glm::max(mBoostFuel, 0.0f);
    }
    else
    {
        SetBoosting(false);
    }
}

void Car::UpdateRotation(float deltaTime)
{
    float motionX = mCurrentInput.mMotionX;
    float motionY = mCurrentInput.mMotionY;

    // Account for deadzone
    if (fabs(motionX) < 0.2f)
    {
        motionX = 0.0f;
    }

    if (fabs(motionY) < 0.2f)
    {
        motionY = 0.0f;
    }

    // Update turn rate
    if (mGrounded && mSurfaceAligned && mCurrentInput.mSlide)
    {
        float slideDir = 0.0f;

        if (motionX > 0.0f)
            slideDir = -1.0f;
        else if (motionX < 0.0f)
            slideDir = 1.0f;

        bool reversing = mCurrentInput.mAccelerate < mCurrentInput.mReverse;
        if (reversing)
        {
            slideDir *= -1.0f;
        }

        mSlideTurnRate = Maths::Approach(mSlideTurnRate, SlideTurnRate * slideDir, 2.0f, deltaTime);
    }
    else
    {
        mSlideTurnRate = Maths::Approach(mSlideTurnRate, 0.0f, 2.0f, deltaTime);
    }

    float turnRate = (mGrounded && mSurfaceAligned) ? GroundedTurnRate : AerialTurnRate;

    bool reversing =
        mGrounded &&
        mSurfaceAligned &&
        glm::dot(mVelocity, GetForwardVector()) < 0.0f;

    float rotX = 0.0f;
    float rotY = 0.0f;
    float rotZ = 0.0f;
    if (!mGrounded)
    {
        if (mCurrentInput.mSlide)
        {
            rotZ = turnRate * -motionX * deltaTime;
        }

        rotX = turnRate * -motionY * deltaTime;
    }

    if (mGrounded || !mCurrentInput.mSlide)
    {
        rotY = turnRate * -motionX * deltaTime;

        if (mGrounded)
        {
            // Shouldn't be able to turn when not moving
            float speed = glm::length(mVelocity);
            float turnAlpha = glm::clamp(speed / SpeedLimit, 0.0f, 1.0f);
            turnAlpha = glm::max(glm::pow(turnAlpha, 0.5f) - 0.05f, 0.0f);
            rotY *= turnAlpha;

            if (speed > 0.1f && reversing)
                rotY *= -1.0f;

            rotY += mSlideTurnRate * deltaTime;
        }
    }

    if (!(mGrounded && mSurfaceAligned) &&
        mSpinTime > 0.0f)
    {
        rotX = 0.0f;
        rotY = 0.0f;
        rotZ = 0.0f;
    }

    glm::quat quatRotX = glm::quat({ rotX, 0.0f, 0.0f });
    glm::quat quatRotY = glm::quat({ 0.0f, rotY, 0.0f });
    glm::quat quatRotZ = glm::quat({ 0.0f, 0.0f, rotZ});
    glm::quat currentRot = GetRotationQuat();
    SetRotation(currentRot * quatRotY * quatRotZ * quatRotX);

    float speed = glm::length(mVelocity);

    if (mGrounded && mSurfaceAligned)
    {
        // Convert Forward Velocity
        const float velocityConversionRate = mCurrentInput.mSlide ? 0.4f : 0.9f;
        
        glm::vec3 retainedVelocity = mVelocity * (1.0f - velocityConversionRate);

        mVelocity = (speed > 0.0f) ? glm::normalize(mVelocity) : mVelocity;
        mVelocity = glm::rotate(mVelocity, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
        mVelocity *= (speed * velocityConversionRate);
        mVelocity += retainedVelocity;
    }

    // Adjust front fender / wheel rotation to match input direction
    // NOTE: This function is only firing on local player or net authority, so clients
    // will not see other car wheel rotation or turning (hard to notice anyway). To fix this,
    // we can probably add replicated data for steering direction + speed.
    float targetRotY = -30.0f * motionX;
    mWheelRotationY = Maths::Approach(mWheelRotationY, targetRotY, 500.0f, deltaTime);

    float wheelSpeed = -800.0f * glm::clamp(speed / 20.0f, 0.0f, 1.0f);
    wheelSpeed *= glm::dot(mVelocity, GetForwardVector()) >= 0.0f ? 1.0f : -1.0f;
    float deltaWheelAngle = wheelSpeed * deltaTime;
    mWheelRotationX += deltaWheelAngle;
    mWheelRotationX = fmod(mWheelRotationX, 360.0f);

    // Force an animation update so we can adjust bones afterwards.
    // Animation usually happens during the culling step before rendering,
    // But calling it here will do it ahead of time (and skip animation during culling).
    mMesh3D->UpdateAnimation(deltaTime, true);

    {
        // Fender rotation (only adjust yaw)
        glm::quat rotY = glm::quat(glm::vec3(0.0f, 0.0f, DEGREES_TO_RADIANS * mWheelRotationY));
        glm::mat4 fenderTransform = glm::toMat4(rotY);

        mMesh3D->SetBoneTransform(mBoneFenderL, fenderTransform);
        mMesh3D->SetBoneTransform(mBoneFenderR, fenderTransform);
    }

    {
        // Wheel rotation (adjust yaw and pitch)
        glm::quat rotX = glm::quat(glm::vec3(DEGREES_TO_RADIANS * mWheelRotationX, 0.0f, 0.0f));
        glm::quat rotY = glm::quat(glm::vec3(0.0f, 0.0f, DEGREES_TO_RADIANS * mWheelRotationY));
        glm::quat rotQuat = rotY * rotX;
        glm::mat4 frontWheelTransform = glm::toMat4(rotQuat);
        glm::mat4 backWheelTransform = glm::toMat4(rotX);

        mMesh3D->SetBoneTransform(mBoneWheelFL, frontWheelTransform);
        mMesh3D->SetBoneTransform(mBoneWheelFR, frontWheelTransform);
        mMesh3D->SetBoneTransform(mBoneWheelBL, backWheelTransform);
        mMesh3D->SetBoneTransform(mBoneWheelBR, backWheelTransform);
    }
}

void Car::UpdateVelocity(float deltaTime)
{
    // Update the speed limit. Increases when boost is held.
    float targetSpeedLimit = mBoosting ? BoostSpeedLimit : SpeedLimit;
    float approachRate = (targetSpeedLimit > mSpeedLimit) ? 40.0f : 20.0f;
    mSpeedLimit = Maths::Approach(
        mSpeedLimit,
        targetSpeedLimit,
        approachRate,
        deltaTime);

    if (mGrounded)
    {
        float speed = glm::length(mVelocity);
        glm::vec3 velDir = (speed != 0.0f) ? (mVelocity / speed) : glm::vec3(0.0f, 0.0f, 0.0f);

        // Apply drag first
        float dragAcceleration = mSurfaceAligned ? 10.0f : 70.0f;

        if (mSurfaceAligned)
        {
            const float transverseDrag = 50.0f;
            float transverseAlpha = 1.0f - fabs(glm::dot(velDir, GetForwardVector()));
            dragAcceleration = glm::mix(dragAcceleration, transverseDrag, transverseAlpha);
        }

        speed = Maths::Approach(speed, 0.0f, dragAcceleration, deltaTime);
        glm::vec3 newVel = glm::vec3(velDir.x * speed, (velDir.y < 0.0f) ? mVelocity.y : velDir.y * speed, velDir.z * speed);
        mVelocity = newVel;

        if (mSurfaceAligned)
        {
            const float minAcceleration = -40.0f;

            glm::vec3 thrustDir = GetForwardVector();
            thrustDir = thrustDir - glm::dot(thrustDir, mSurfaceNormal) * mSurfaceNormal;

            float accelDir = mBoosting ? 1.0f : mCurrentInput.mAccelerate - mCurrentInput.mReverse;
            float acceleration = accelDir * (mBoosting ? BoostAcceleration : DefaultAcceleration);
            acceleration = glm::max(acceleration, minAcceleration);
            mVelocity += (thrustDir * acceleration * deltaTime);

            // Clamp to speed limit
            float speed = glm::length(mVelocity);
            if (speed > mSpeedLimit)
            {
                speed = Maths::Damp(speed, mSpeedLimit, 0.005f, deltaTime);
                mVelocity = Maths::SafeNormalize(mVelocity) * speed;
            }
        }
    }
    else
    {
        if (mBoosting)
        {
            mVelocity += (AerialBoostAcceleration * GetForwardVector() * deltaTime);
        }

        if (mSpinTime > 0.0f)
        {
            mVelocity.y = 0.0f;
        }
    }

    // Update gravity
    float targetGravity = DefaultGravity;
    glm::vec3 targetGravityDir = glm::vec3(0.0f, -1.0f, 0.0f);

    if (mGrounded && mSurfaceAligned)
    {
        float speed = length(mVelocity);
        float gravityAlpha = glm::clamp((speed - 5.0f) / 5.0f, 0.0f, 1.0f);
        targetGravityDir = glm::mix(glm::vec3(0.0f, -1.0f, 0.0f), -mSurfaceNormal, gravityAlpha);
        targetGravity = glm::mix(WallGravity, DefaultGravity, fabs(mSurfaceNormal.y));
    }

    mGravity = Maths::Approach(mGravity, targetGravity, 100.0f, deltaTime);
    mGravityDirection = Maths::Damp(mGravityDirection, targetGravityDir, 0.005f, deltaTime);
    mGravityDirection = Maths::SafeNormalize(mGravityDirection);

    // Don't apply gravity while double jump spinning
    if (mSpinTime <= 0.0f)
    {
        mVelocity += (mGravity * mGravityDirection * deltaTime);
    }
}

void Car::UpdateJump(float deltaTime)
{
    if (mCurrentInput.mJump &&
        !mPreviousInput.mJump)
    {
        bool jumped = false;

        if (mGrounded && mSurfaceAligned)
        {
            mVelocity += mSmoothedSurfaceNormal * JumpSpeed;
            mGravity = DefaultGravity;
            ClearGrounded();
            jumped = true;
        }
        else if (mDoubleJump && (mTimeSinceLastGrounding < DoubleJumpTimeLimit))
        {
            glm::vec3 jumpDir = { 0.0f, 1.0f, 0.0f };
            float jumpSpeed = JumpSpeed;

            // Perform a double jump.
            float absMotionX = fabs(mCurrentInput.mMotionX);
            float absMotionY = fabs(mCurrentInput.mMotionY);

            if (absMotionX < 0.5f && absMotionY < 0.5f)
            {
                // Undirected double jump
                jumpDir = GetUpVector();
                mSpinDirX = 0.0f;
                mSpinDirZ = 0.0f;
            }
            else if (absMotionX > absMotionY)
            {
                // Right/Left double jump
                jumpDir = GetForwardVector();
                jumpDir = glm::rotate(jumpDir, -90.0f * DEGREES_TO_RADIANS, glm::vec3(0.0f, 1.0f, 0.0f));

                mSpinDirX = 0.0f;
                mSpinDirZ = (mCurrentInput.mMotionX < 0.0f) ? 1.0f : -1.0f;
                jumpDir *= -mSpinDirZ;
                mSpinTime = DoubleJumpSpinDuration;

                jumpSpeed = SpinJumpSpeed;
            }
            else
            {
                // Forward/Backward double jump
                jumpDir = GetForwardVector();
                jumpDir.y = 0.0f;
                jumpDir = Maths::SafeNormalize(jumpDir);

                mSpinDirX = (mCurrentInput.mMotionY < 0.0f) ? 1.0f : -1.0f;
                mSpinDirZ = 0.0f;
                jumpDir *= -mSpinDirX;
                mSpinTime = DoubleJumpSpinDuration;

                jumpSpeed = SpinJumpSpeed;
            }

            mVelocity += (jumpDir * jumpSpeed);
            mDoubleJump = false;
            jumped = true;
        }

        if (jumped)
        {
            if (mJumpAudio3D->IsPlaying())
            {
                mJumpAudio3D->ResetAudio();
                mJumpAudio3D->PlayAudio();
            }

            mJumpAudio3D->PlayAudio();
        }
    }

    if (mSpinTime > 0.0f)
    {
        mSpinTime -= deltaTime;

        if (mSpinTime <= 0.0f ||
            (mGrounded && mSurfaceAligned))
        {
            // Stop spin
            mSpinTime = 0.0f;
        }

        const float RotationSpeed = 360.0f / DoubleJumpSpinDuration;

        glm::vec3 deltaRot = { mSpinDirX, 0.0f, mSpinDirZ };
        deltaRot *= (RotationSpeed * deltaTime * DEGREES_TO_RADIANS);
        glm::quat quatRot = glm::quat(deltaRot);
        SetRotation(GetRotationQuat() * quatRot);
    }
}

void Car::UpdateCamera(float deltaTime)
{
    static glm::vec3 smoothedBallPos = {};
    if (!IsLocallyControlled())
        return;

    if (mCurrentInput.mBallCam &&
        !mPreviousInput.mBallCam)
    {
        mBallCam = !mBallCam;
    }

    Ball* ball = GetMatchState()->mBall;
    glm::vec3 focusDirection = mMotionDirection;

    if (ball != nullptr)
    {
        if (NetIsAuthority())
        {
            smoothedBallPos = ball->GetPosition();
        }
        else
        {
            smoothedBallPos = Maths::Damp(smoothedBallPos, ball->GetPosition(), 0.0005f, deltaTime);
        }
    }

    if (mBallCam &&
        ball != nullptr)
    {
        focusDirection = smoothedBallPos - GetPosition();
        focusDirection = Maths::SafeNormalize(focusDirection);
    }

    // The focus direction determines our camera yaw and pitch.
    float x = focusDirection.x;
    float y = focusDirection.y;
    float z = -focusDirection.z;
    float xzHyp = glm::sqrt(x * x + z * z);
    float yaw = acosf(z / xzHyp) * RADIANS_TO_DEGREES;
    if (x < 0.0f)
    {
        yaw *= -1.0f;
    }

    // Approach target yaw. Needs to smooth in the correct direction.
    // (Source vs Target yaws should be within 180 degrees of each other)
    float sourceYaw = mCameraYaw;
    float targetYaw = yaw;

    if (fabs(targetYaw - sourceYaw) <= 180.0f)
    {
        mCameraYaw = Maths::Approach(sourceYaw, targetYaw, CameraSpeed, deltaTime);
    }
    else
    {
        // Convert to 0 to 360 range
        if (sourceYaw < 0.0f)
            sourceYaw += 360.0f;
        if (targetYaw < 0.0f)
            targetYaw += 360.0f;

        mCameraYaw = Maths::Approach(sourceYaw, targetYaw, CameraSpeed, deltaTime);

        // Convert current yaw back into -180 to 180 range
        if (mCameraYaw > 180.0f)
        {
            mCameraYaw -= 360.0f;
        }
    }

    // Handle free camera tilting with right stick
    float targetYawOffset = 0.0f;
    float targetPitchOffset = 0.0f;
    if (!mBallCam)
    {
        float cameraX = (fabs(mCurrentInput.mCameraX) < 0.1f) ? 0.0f : mCurrentInput.mCameraX;
        float cameraY = (fabs(mCurrentInput.mCameraY) < 0.1f) ? 0.0f : mCurrentInput.mCameraY;
        targetYawOffset = cameraX * 120.0f;
        targetPitchOffset = cameraY * 60.0f;
    }

    mCameraYawOffset = Maths::Damp(mCameraYawOffset, targetYawOffset, 0.005f, deltaTime);
    mCameraPitchOffset = Maths::Damp(mCameraPitchOffset, targetPitchOffset, 0.005f, deltaTime);

    float pitch = 0.0f;

    // We've determined the rotation of the camera, but now we need to position it correctly.
    glm::vec3 cameraPos = {0.0f, CameraHeight, CameraDistanceXZ};

    if (mBallCam)
    {
        mCameraArmPitch = Maths::Approach(mCameraArmPitch, 0.0f, CameraSpeed, deltaTime);
        cameraPos = glm::rotate(cameraPos, DEGREES_TO_RADIANS * mCameraArmPitch, glm::vec3(1.0f, 0.0f, 0.0f));
        cameraPos = glm::rotate(cameraPos, DEGREES_TO_RADIANS * -mCameraYaw, glm::vec3(0.0f, 1.0f, 0.0f));
        cameraPos = GetPosition() + cameraPos;

        // Ball cam uses the camera to ball direction for determining pitch.
        glm::vec3 cameraToBall = smoothedBallPos - cameraPos;
        cameraToBall = Maths::SafeNormalize(cameraToBall);
        pitch = asinf(cameraToBall.y / 1.0f) * RADIANS_TO_DEGREES;
        mCameraPitch = Maths::Approach(mCameraPitch, pitch, CameraSpeed, deltaTime);
    }
    else
    {
        // Free cam uses the focus direction as the pitch.
        pitch = asinf(y / 1.0f) * RADIANS_TO_DEGREES;

        if (mGrounded && mSurfaceAligned)
        {
            glm::vec3 groundedDir = focusDirection - glm::dot(focusDirection, mSurfaceNormal) * mSurfaceNormal;
            groundedDir = Maths::SafeNormalize(groundedDir);
            pitch = asinf(groundedDir.y / 1.0f) * RADIANS_TO_DEGREES;
        }

        //mCameraPitch = Maths::Approach(mCameraPitch, pitch, CameraSpeed, deltaTime);
        //mCameraArmPitch = Maths::Approach(mCameraArmPitch, pitch, CameraSpeed, deltaTime);
        mCameraPitch = Maths::Damp(mCameraPitch, pitch, 0.005f, deltaTime);
        mCameraArmPitch = Maths::Damp(mCameraArmPitch, pitch, 0.005f, deltaTime);

        cameraPos = glm::rotate(cameraPos, DEGREES_TO_RADIANS * (mCameraArmPitch - mCameraPitchOffset), glm::vec3(1.0f, 0.0f, 0.0f));
        cameraPos = glm::rotate(cameraPos, DEGREES_TO_RADIANS * (-mCameraYaw - mCameraYawOffset), glm::vec3(0.0f, 1.0f, 0.0f));
        cameraPos = GetPosition() + cameraPos;
    }

    UpdateTransform(true);

    mCamera3D->SetAbsolutePosition(cameraPos);
    mCamera3D->SetAbsoluteRotation(glm::vec3(mCameraPitch - mCameraPitchOffset, -mCameraYaw - mCameraYawOffset, 0.0f));
}

void Car::UpdateGrounded(float deltaTime)
{
    mTimeSinceLastGrounding += deltaTime;

    if (mTimeSinceLastGrounding > 0.050f)
    {
        ClearGrounded();
    }
    else if (mGrounded)
    {
        if (mSurfaceAligned)
        {
            mSmoothedSurfaceNormal = Maths::Damp(mSmoothedSurfaceNormal, mSurfaceNormal, 0.005f, deltaTime);

            // Align car with surface normal when grounded.
            glm::vec3 forward = GetForwardVector();
            glm::vec3 normal = mSmoothedSurfaceNormal;
            glm::vec3 newForward = forward - glm::dot(normal, forward) * normal;
            newForward = glm::normalize(newForward);
            LookAt(GetPosition() + newForward, normal);

            // Sweep towards surface normal a tiny bit to check if we are still grounded.
            if (mSurfaceNormal.y > 0.0f)
            {
                glm::vec3 truePosition = GetPosition();
                const float groundingThreshold = 0.3f;
                SweepTestResult sweepResult;
                SweepToWorldPosition(truePosition - mSurfaceNormal * groundingThreshold, sweepResult, ColGroupEnvironment);

                if (sweepResult.mHitFraction != 1.0f)
                {
                    mTimeSinceLastGrounding = 0.0f;
                }

                SetPosition(truePosition);
            }
        }
        else
        {
            // Magnetise car rotation to surface. (usually try to flip it upright)
            mSmoothedSurfaceNormal = GetUpVector();
            glm::vec3 up = GetUpVector();
            glm::vec3 normal = mSurfaceNormal;

            glm::vec3 newUp = Maths::Damp(up, normal, 0.005f, deltaTime);
            glm::quat rot = glm::rotation(up, newUp);
            AddRotation(rot);
        }

        mMotionDirection = GetForwardVector();
    }
}

void Car::UpdateAudio(float deltaTime)
{
    float speed = glm::length(mVelocity);
    float volumeAlpha = glm::clamp(speed / SpeedLimit, 0.1f, 1.0f);
    float pitchAlpha = glm::clamp(speed / SpeedLimit, 0.0f, 1.0f);
    mEngineAudio3D->SetVolume(volumeAlpha * 1.0f);
    mEngineAudio3D->SetPitch(glm::mix(1.0f, 1.3f, pitchAlpha));
}

void Car::UpdateMotion(float deltaTime)
{
    float remainingTime = deltaTime;
    uint8_t collisionMask = GetCollisionMask();
    glm::vec3 startPos = GetPosition();

    SweepTestResult sweepResult;
    bool hit = SweepToWorldPosition(GetPosition() + mVelocity * remainingTime, sweepResult, collisionMask);

    if (hit && sweepResult.mHitComponent != nullptr &&
        sweepResult.mHitComponent->Is(Ball::ClassRuntimeId()))
    {
        HandleCollision(this, sweepResult.mHitComponent, sweepResult.mHitPosition, sweepResult.mHitNormal, nullptr);

        // Sweep again, but this time ignoring the ball.
        remainingTime = remainingTime * (1.0f - sweepResult.mHitFraction);
        collisionMask = (collisionMask & (~ColGroupBall));
        hit = SweepToWorldPosition(GetPosition() + mVelocity * remainingTime, sweepResult, collisionMask);
    }

    if (hit)
    {
        HandleCollision(this, sweepResult.mHitComponent, sweepResult.mHitPosition, sweepResult.mHitNormal, nullptr);

        remainingTime = remainingTime * (1.0f - sweepResult.mHitFraction);
        glm::vec3 normal = sweepResult.mHitNormal;
        glm::vec3 parallelVelocity = mVelocity - (normal * glm::dot(mVelocity, normal));
        mVelocity = parallelVelocity;

        // Slide along surface
        hit = SweepToWorldPosition(GetPosition() + mVelocity * remainingTime, sweepResult, collisionMask);

        if (hit)
        {
            HandleCollision(this, sweepResult.mHitComponent, sweepResult.mHitPosition, sweepResult.mHitNormal, nullptr);
        }
    }

    // If this is a client's car and we are updating on the server, then reset to its initial position
    // because we want to give clients control over their transform. We only do the above sweeps because
    // we want the server to determine ball hits / bumps / demolitions.
    if (NetIsAuthority() &&
        mOwningHost != SERVER_HOST_ID &&
        mOwningHost != INVALID_HOST_ID)
    {
        SetPosition(startPos);
    }
}

void Car::BotUpdateTarget(float deltaTime)
{
    Ball* ball = GetMatchState()->mBall;
    glm::vec3 ballPos = ball->GetPosition();
    glm::vec3 carPos = GetPosition();
    glm::vec3 toBall = ballPos - carPos;
    float distToBall = glm::length(toBall);
    toBall = glm::normalize(toBall);

    glm::vec3 forwardDir = (mTeamIndex == 0) ? glm::vec3(1, 0, 0) : glm::vec3(-1, 0, 0);
    float ballForwardness = glm::dot(forwardDir, toBall);
    float ballAlignment = glm::dot(GetForwardVector(), toBall);

    const float forwardingRadius = 45.0f;
    const float approachRadius = 10.0f;
    const float ballTargetRadius = (mBotBehavior == BotBehavior::Offense) ? 50.0f : 25.0f;
    const float lowBoostFuel = (mBotBehavior == BotBehavior::Support) ? 40.0f : 5.0f;
    const float defendBallLimitX = 0.0f;
    const float ballForwardnessThreshold = 0.0f;

    mBotTargetTime += deltaTime;

    bool needsNewTarget = false;
    float targetDistance = glm::distance(carPos, mBotTargetActor ? mBotTargetActor->GetPosition() : mBotTargetPosition);

    // (1) Determine if we need a new target.
    // (2) If so, pick a new target.

    if (mBotTargetType == BotTargetType::Count)
    {
        // Invalid target
        needsNewTarget = true;
    }
    else if (mBotTargetType == BotTargetType::Ball &&
                ballForwardness < ballForwardnessThreshold)
    {
        // Car is behind ball. Don't want to hit it wrong way.
        needsNewTarget = true;
    }
    else if (mBotTargetType == BotTargetType::Boost &&
                (mBoostFuel >= 40.0f || mBotTargetTime >= 8.0f || targetDistance < 1.0f))
    {
        // Car has enough boost now. Target something else.
        needsNewTarget = true;
    }
    else if (mBotTargetType == BotTargetType::Position &&
                (mBotTargetTime >= 8.0f || targetDistance < 2.0f))
    {
        // Bot is possibly stuck?
        needsNewTarget = true;
    }
    else if (mBotTargetType != BotTargetType::Ball &&
             ballForwardness >= ballForwardnessThreshold &&
             distToBall <= ballTargetRadius &&
             ballAlignment >= 0.0f)
    {
        // Prioritize hitting the ball!
        needsNewTarget = true;
    }


    if (needsNewTarget)
    {
        mBotTargetType = BotTargetType::Count;
        mBotTargetActor = nullptr;
        mBotTargetPosition = {};
        mBotTargetTime = 0.0f;

        if (ballForwardness < ballForwardnessThreshold)
        {
            // If behind the ball, pick a random point in front of the ball
            glm::vec3 centerPos = ballPos - forwardDir * forwardingRadius;
            mBotTargetPosition = FindRandomPointInCircleXZ(centerPos, forwardingRadius);
            mBotTargetType = BotTargetType::Position;
        }
        else if (distToBall <= ballTargetRadius)
        {
            // If the ball is within XXm, target the ball.
            mBotTargetActor = ball;
            mBotTargetType = BotTargetType::Ball;
        }
        else if (mBoostFuel < lowBoostFuel)
        {
            // If the ball is farther than XXm and boost < 5, target big boost
            mBotTargetActor = FindClosestFullBoost();
            mBotTargetType = BotTargetType::Boost;
        }
        else if (mBotBehavior != BotBehavior::Defense ||
                (ballPos.x * forwardDir.x < defendBallLimitX))
        {
            // If the ball is farther than XXmm and boost >= 5, target random point in front of ball
            glm::vec3 centerPos = ballPos - forwardDir * approachRadius;
            mBotTargetPosition = FindRandomPointInCircleXZ(centerPos, approachRadius);
            mBotTargetType = BotTargetType::Position;
        }
        else
        {
            // Head to someplace near own goal
            OCT_ASSERT(mTeamIndex == 0 || mTeamIndex == 1);
            glm::vec3 centerPos = GetMatchState()->mGoalBoxes[mTeamIndex]->GetPosition() + forwardDir * 20.0f;
            mBotTargetPosition = FindRandomPointInCircleXZ(centerPos, 10.0f);
            mBotTargetType = BotTargetType::Position;
        }

        if (mBotTargetType == BotTargetType::Position)
        {
            // Clamp within reasonable bounds
            mBotTargetPosition = glm::clamp(
                mBotTargetPosition,
                glm::vec3(-ARENA_BOT_EXTENT_X, 0.0f, -ARENA_BOT_EXTENT_Z),
                glm::vec3(ARENA_BOT_EXTENT_X, ARENA_BOT_EXTENT_Y, ARENA_BOT_EXTENT_Z));
        }
    }

    // Debug target
    //glm::vec3 targetPos = (mBotTargetType == BotTargetType::Position) ? mBotTargetPosition : mBotTargetActor->GetPosition();
    //Line line(GetPosition(), targetPos, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 0.03f);
    //GetWorld()->AddLine(line);
}

void Car::BotUpdateHandling(float deltaTime)
{
    glm::vec3 carPos = GetPosition();
    glm::vec3 targetPos = (mBotTargetType == BotTargetType::Position) ? mBotTargetPosition : mBotTargetActor->GetPosition();
    targetPos.y = carPos.y;

    // For Ball targeting, bias the target position slightly so that the car will hit it toward the goal.
    if (mBotTargetType == BotTargetType::Ball)
    {
        int32_t enemyTeam = (mTeamIndex == 0) ? 1 : 0;
        Node3D* enemyGoalBox = GetMatchState()->mGoalBoxes[enemyTeam];
        Ball* ball = GetMatchState()->mBall;
        glm::vec3 goalDir = enemyGoalBox->GetPosition() - ball->GetPosition();
        goalDir.y = 0.0f;
        goalDir = glm::normalize(goalDir);
        
        const float biasStrength = 0.75f;
        targetPos -= goalDir * biasStrength;
    }

    glm::vec3 forwardXZ = GetForwardVector();
    forwardXZ.y = 0.0f;
    forwardXZ = glm::normalize(forwardXZ);

    glm::vec3 toTarget = targetPos - carPos;
    float dist = glm::length(toTarget);
    toTarget = glm::normalize(toTarget);

    float alignment = glm::dot(toTarget, forwardXZ);
    float carSpeed = glm::length(mVelocity);

    if (dist < 5.0f &&
        alignment < -0.8f)
    {
        // If we are close to the target, facing away from it, reverse into it.
        mCurrentInput.mAccelerate = 0.0f;
        mCurrentInput.mReverse = 1.0f;
        mCurrentInput.mMotionX = 0.0f;
        mCurrentInput.mMotionY = 0.0f;
        mCurrentInput.mSlide = false;
        mCurrentInput.mBoost = false;
        mCurrentInput.mJump = false;
    }
    else
    {
        // Otherwise, we just want to move motionX so we turn toward the target.
        glm::vec3 crossProd = glm::cross(forwardXZ, toTarget);
        bool rightSide = (crossProd.y <= 0.0f);

        float accel = (mBotTargetType == BotTargetType::Ball) ? 1.0f : (dist / 10.0f);
        accel = glm::clamp(accel, 0.0f, 1.0f);

        mCurrentInput.mMotionX = rightSide ? 1.0f : -1.0f;
        mCurrentInput.mAccelerate = accel;
        mCurrentInput.mReverse = 0.0f;
        mCurrentInput.mMotionY = 0.0f;
        mCurrentInput.mSlide = (alignment < 0.8f) || (carSpeed < 0.5f);
        mCurrentInput.mBoost = (mBotTargetType == BotTargetType::Ball && alignment >= 0.75f);
        mCurrentInput.mJump = false;
    }
}

void Car::UpdateDebug(float deltaTime)
{

}

void Car::ClearGrounded()
{
    mGrounded = false;
    mSurfaceAligned = false;
    mSurfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    mSmoothedSurfaceNormal = GetUpVector();
}

Node3D* Car::FindClosestFullBoost()
{
    Node3D* retBoost = nullptr;

    MatchState* match = GetMatchState();

    float closestDistSq = FLT_MAX;
    glm::vec3 carPos = GetPosition();

    for (uint32_t i = 0; i < NUM_FULL_BOOSTS; ++i)
    {
        float distSq = glm::distance2(match->mFullBoosts[i]->GetPosition(), carPos);

        if (distSq <= closestDistSq)
        {
            closestDistSq = distSq;
            retBoost = match->mFullBoosts[i];
        }
    }

    return retBoost;
}

glm::vec3 Car::FindRandomPointInCircleXZ(glm::vec3 center, float radius)
{
    center.y = 0.5f;
    float randAngle = Maths::RandRange(0.0f, 360.0f);
    float randDist = Maths::RandRange(0.0f, radius);
    float xOffset = cosf(randAngle) * randDist;
    float zOffset = sinf(randAngle) * randDist;

    return (center + glm::vec3(xOffset, 0.0f, zOffset));
}

void Car::MoveToRandomSpawnPoint()
{
    MatchState* match = GetMatchState();

    int32_t spawnIndex = Maths::RandRange(int32_t(0), int32_t(2));
    Node3D* spawnActor = (mTeamIndex == 0) ? match->mSpawnPoints0[spawnIndex] : match->mSpawnPoints1[spawnIndex];
    SetPosition(spawnActor->GetPosition());
    SetRotation(glm::vec3(0.0f, (mTeamIndex == 0) ? -90.0f : 90.0f, 0.0f));
    Reset();
}

void Car::ResetState()
{
    mVelocity = glm::vec3(0.0f);
    mSlideTurnRate = 0.0f;
    mSpinTime = 0.0f;
    mGravityDirection = { 0.0f, -1.0f, 0.0f };
    mSurfaceNormal = { 0.0f, 1.0f, 0.0f };
    mGravity = 9.8f;
    mBoostFuel = StartingBoost;
    mSmoothedSurfaceNormal = { 0.0f, 1.0f, 0.0f };
}

void Car::SetBoosting(bool boosting)
{
    if (mBoosting != boosting)
    {
        mBoosting = boosting;
        mTrailComponent->EnableEmission(boosting);

        if (boosting)
        {
            mBoostAudio3D->PlayAudio();
        }
    }
}