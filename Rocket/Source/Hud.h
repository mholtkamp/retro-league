#pragma once

#include "Nodes/Widgets/Widget.h"
#include "Nodes/Widgets/Text.h"
#include "Nodes/Widgets/Quad.h"

class Hud : public Widget
{
public:

    DECLARE_NODE(Hud, Widget);

    Hud();
    virtual ~Hud();

    virtual void Tick(float deltaTime) override;

    void SetCountdownTime(int32_t seconds);

protected:

    Text* mScore0 = nullptr;
    Text* mScore1 = nullptr;
    Text* mTime = nullptr;
    Text* mBoost = nullptr;
    Text* mWinner = nullptr;
    Text* mCountdown = nullptr;

    Quad* mMatchBg = nullptr;
    Quad* mBoostBg = nullptr;
    Quad* mBoostFuel = nullptr;
    Quad* mBoostCapacity = nullptr;
    Quad* mWinnerBg = nullptr;
    Quad* mCountdownBg = nullptr;
};