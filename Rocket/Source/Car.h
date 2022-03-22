#pragma once

#include "Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ShadowMeshComponent.h"
#include "Components/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"

#include "RocketTypes.h"

struct CarInput
{
    float mMotionX = 0.0f;
    float mMotionY = 0.0f;
    float mCameraX = 0.0f;
    float mCameraY = 0.0f;
    float mReverse = 0.0f;
    float mAccelerate = 0.0f;

    bool mJump = false;
    bool mBoost = false;
    bool mBallCam = false;
    bool mSlide = false;
    bool mMenu = false;
};

class Car : public Actor
{
public:

    DECLARE_ACTOR(Car, Actor);
    
    Car();
    ~Car();

    virtual void Create() override;
    virtual void Tick(float deltaTime) override;
    virtual void GatherReplicatedData(std::vector<NetDatum>& outData);
    virtual void GatherNetFuncs(std::vector<NetFunc>& outFuncs) override;

    void HandleCollision(
        PrimitiveComponent* thisComp,
        PrimitiveComponent* otherComp,
        glm::vec3 impactPoint,
        glm::vec3 impactNormal,
        btPersistentManifold* manifold);

    SphereComponent* GetSphereComponent();
    SkeletalMeshComponent* GetMeshComponent();
    CameraComponent* GetCameraComponent();

    void Reset();

    void SetVelocity(glm::vec3 velocity);
    glm::vec3 GetVelocity() const;
    void ForceVelocity(glm::vec3 velocity);

    bool IsBot() const;
    void SetBot(bool bot);

    void SetBotBehavior(BotBehavior mode);
    BotBehavior GetBotBehavior() const;

    void ForceBotTargetBall();

    int32_t GetTeamIndex() const;
    void SetTeamIndex(int32_t index);

    int32_t GetCarIndex() const;
    void SetCarIndex(int32_t index);

    bool IsControlEnabled() const;
    void EnableControl(bool enable);

    void AddBoostFuel(float boost);
    float GetBoostFuel() const;

    void Kill();
    void Respawn();

    bool IsLocallyControlled() const;

protected:

    void UpdateBotInput(float deltaTime);
    void UpdateInput(float deltaTime);
    void UpdateRespawn(float deltaTime);
    void UpdateBoost(float deltaTime);
    void UpdateRotation(float deltaTime);
    void UpdateVelocity(float deltaTime);
    void UpdateJump(float deltaTime);
    void UpdateCamera(float deltaTime);
    void UpdateMotion(float deltaTime);
    void UpdateGrounded(float deltaTime);
    void UpdateAudio(float deltaTime);
    void UpdateDebug(float deltaTime);

    void BotUpdateTarget(float deltaTime);
    void BotUpdateHandling(float deltaTime);

    void ClearGrounded();
    Actor* FindClosestFullBoost();
    glm::vec3 FindRandomPointInCircleXZ(glm::vec3 center, float radius);
    void MoveToRandomSpawnPoint();
    void ResetState();
    void SetBoosting(bool boosting);

    // OnRep functions
    static bool OnRep_NetPosition(Datum* datum, const void* newValue);
    static bool OnRep_NetRotation(Datum* datum, const void* newValue);
    static bool OnRep_OwningHost(Datum* datum, const void* newValue);
    static bool OnRep_Alive(Datum* datum, const void* newValue);
    static bool OnRep_Boosting(Datum* datum, const void* newValue);
    static bool OnRep_TeamIndex(Datum* datum, const void* newValue);

    // RPCs
    static void S_UploadState(Actor* actor, Datum& vecPosition, Datum& vecRotation, Datum& vecVelocity, Datum& bBoosting);
    static void C_ForceVelocity(Actor* actor, Datum& vecVelocity);
    static void C_ForceTransform(Actor* actor, Datum& vecPosition, Datum& vecRotation);
    static void C_AddBoostFuel(Actor* actor, Datum& fBoostFuel);
    static void C_ResetState(Actor* actor);
    static void M_Demolish(Actor* actor);

    SphereComponent* mSphereComponent = nullptr;
    SkeletalMeshComponent* mMeshComponent = nullptr;
    ShadowMeshComponent* mShadowComponent = nullptr;
    ParticleComponent* mTrailComponent = nullptr;
    ParticleComponent* mDemoComponent = nullptr;
    CameraComponent* mCameraComponent = nullptr;
    AudioComponent* mEngineAudioComponent = nullptr;
    AudioComponent* mBoostAudioComponent = nullptr;
    AudioComponent* mJumpAudioComponent = nullptr;

    SoundWaveRef mBumpSound;
    SoundWaveRef mBoostPickupSound;

    CarInput mCurrentInput;
    CarInput mPreviousInput;

    float mBoostFuel = 35.0f;
    float mGravity = 9.8f;
    glm::vec3 mGravityDirection = { 0.0f, -1.0f, 0.0f };
    glm::vec3 mVelocity = {0.0f, 0.0f, 0.0f};
    glm::vec3 mSurfaceNormal = {0.0f, 1.0f, 0.0f};
    glm::vec3 mSmoothedSurfaceNormal = {0.0f, 1.0f, 0.0f};
    float mSpeedLimit = 20.0f;

    glm::vec3 mMotionDirection = { 0.0f, 0.0f, -1.0f };
    float mCameraRotationSpeed = 720.0f;
    float mCameraYaw = 0.0f;
    float mCameraPitch = 0.0f;
    float mCameraArmPitch = 0.0f;
    float mCameraYawOffset = 0.0f;
    float mCameraPitchOffset = 0.0f;

    float mSpinTime = 0.0f;
    float mSpinDirX = 0.0f;
    float mSpinDirZ = 0.0f;

    float mTurnRate = 1.0f;
    float mSlideTurnRate = 0.0f;
    float mTimeSinceLastGrounding = 1.0f;
    float mRespawnTime = 0.0f;
    float mWheelRotationX = 0.0f;
    float mWheelRotationY = 0.0f;

    int32_t mCarIndex = -1;
    int32_t mTeamIndex = -1;

    bool mControlEnabled = false;
    bool mAlive = false;
    bool mBoosting = false;
    bool mGrounded = false;
    bool mDoubleJump = false;
    bool mSurfaceAligned = false;
    bool mBallCam = false;
    bool mBot = false;
    bool mInitialPosSet = false;
    bool mInitialRotSet = false;

    // Mesh bones
    int32_t mBoneFenderL = -1;
    int32_t mBoneFenderR = -1;
    int32_t mBoneWheelFL = -1;
    int32_t mBoneWheelFR = -1;
    int32_t mBoneWheelBL = -1;
    int32_t mBoneWheelBR = -1;

    // Bot Data
    BotBehavior mBotBehavior = BotBehavior::Count;
    BotTargetType mBotTargetType = BotTargetType::Count;
    Actor* mBotTargetActor = nullptr;
    glm::vec3 mBotTargetPosition = { };
    float mBotTargetTime = 0.0f;
};
