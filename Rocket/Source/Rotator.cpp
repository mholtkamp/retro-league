#include "Rotator.h"

#include "Components/StaticMeshComponent.h"

DEFINE_ACTOR(Rotator, Actor);

void Rotator::Create()
{
    Actor::Create();
    SetRootComponent(CreateComponent<StaticMeshComponent>());
}

void Rotator::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);

#if EDITOR
    if (mEnableInEditor)
#endif
    {
        SetRotation(GetRotationEuler() + mAngularVelocity * deltaTime);
    }
}

void Rotator::GatherProperties(std::vector<Property>& outProps)
{
    Actor::GatherProperties(outProps);
    outProps.push_back(Property(DatumType::Vector, "Angular Velocity", this, &mAngularVelocity));
    outProps.push_back(Property(DatumType::Bool, "Enable In Editor", this, &mEnableInEditor));
}

void Rotator::SaveStream(Stream& stream)
{
    Actor::SaveStream(stream);
    stream.WriteVec3(mAngularVelocity);
    stream.WriteBool(mEnableInEditor);
}

void Rotator::LoadStream(Stream& stream)
{
    Actor::LoadStream(stream);
    mAngularVelocity = stream.ReadVec3();
    mEnableInEditor = stream.ReadBool();
}