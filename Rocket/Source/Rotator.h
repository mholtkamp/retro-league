#pragma once

#include "Nodes/Node.h"


class Rotator : public Node3D
{
public:

    DECLARE_NODE(Rotator, Node3D);

    virtual void Create() override;
    virtual void Tick(float deltaTime) override;
    virtual void GatherProperties(std::vector<Property>& outProps) override;
    virtual void SaveStream(Stream& stream) override;
    virtual void LoadStream(Stream& stream) override;


protected:

    glm::vec3 mAngularVelocity = {};
    bool mEnableInEditor = true;
};

