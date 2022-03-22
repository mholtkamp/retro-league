#pragma once

#include "Actor.h"


class Rotator : public Actor
{
public:

    DECLARE_ACTOR(Rotator, Actor);

    virtual void Create() override;
    virtual void Tick(float deltaTime) override;
    virtual void GatherProperties(std::vector<Property>& outProps) override;
    virtual void SaveStream(Stream& stream) override;
    virtual void LoadStream(Stream& stream) override;


protected:

    glm::vec3 mAngularVelocity = {};
    bool mEnableInEditor = true;
};

