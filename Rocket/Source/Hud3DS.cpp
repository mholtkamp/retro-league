#include "Hud3DS.h"
#include "Renderer.h"

Hud3DS::Hud3DS()
{
    mBoostBg->SetVisible(false);

    mWinner->SetTextSize(32.0f);

    mCountdownBg->SetAnchorMode(AnchorMode::FullStretch);
    mCountdownBg->SetMargins(0.0f, 0.0f, 0.0f, 0.0f);
    mCountdownBg->SetTexture(nullptr);
    mCountdownBg->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });

    mWinnerBg->SetTexture(nullptr);
    mWinnerBg->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });

    mCountdown->SetAnchorMode(AnchorMode::Mid);
    mCountdown->SetPosition(-10, -30);
    mWinner->SetPosition(0.0f, -50.0f);

    // Place winner/countdown background quads on top of everything else so that
    // they cover the entire screen.
    RemoveChild(mWinnerBg);
    RemoveChild(mCountdownBg);
    AddChild(mWinnerBg);
    AddChild(mCountdownBg);
}
