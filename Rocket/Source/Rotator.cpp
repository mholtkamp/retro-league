#include "Rotator.h"

#include "Nodes/3D/StaticMesh3d.h"

DEFINE_NODE(Rotator, Node3D);

void Rotator::Create()
{
    Node3D::Create();
    CreateChild<StaticMesh3D>();
}

void Rotator::Tick(float deltaTime)
{
    Node3D::Tick(deltaTime);

#if EDITOR
    if (mEnableInEditor)
#endif
    {
        SetRotation(GetRotationEuler() + mAngularVelocity * deltaTime);
    }
}

void Rotator::GatherProperties(std::vector<Property>& outProps)
{
    Node3D::GatherProperties(outProps);
    outProps.push_back(Property(DatumType::Vector, "Angular Velocity", this, &mAngularVelocity));
    outProps.push_back(Property(DatumType::Bool, "Enable In Editor", this, &mEnableInEditor));
}

void Rotator::SaveStream(Stream& stream)
{
    Node3D::SaveStream(stream);
    stream.WriteVec3(mAngularVelocity);
    stream.WriteBool(mEnableInEditor);
}

void Rotator::LoadStream(Stream& stream)
{
    // Scene Conversion, need to load these two vars from stream
    //Node3D::LoadStream(stream);
    mAngularVelocity = stream.ReadVec3();
    mEnableInEditor = stream.ReadBool();
}