#pragma once

#include "Widgets/Widget.h"
#include "Widgets/Text.h"
#include "Widgets/Quad.h"

class Hud : public Widget
{
public:

    Hud();
    virtual ~Hud();

    virtual void Update() override;

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