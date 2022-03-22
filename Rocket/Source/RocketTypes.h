#pragma once

#include "EngineTypes.h"

enum RocketCollisionTypes
{
    ColGroupCar = ColGroup0,
    ColGroupEnvironment = ColGroup1,
    ColGroupBall = ColGroup2,
    ColGroupTrigger = ColGroup3
};

enum class BotBehavior
{
    Defense,
    Offense,
    Support,

    Count
};

enum class BotTargetType
{
    Ball,
    Boost,
    Position,

    Count
};

enum class EnvironmentType
{
    None,
    Lagoon,

    Count
};
