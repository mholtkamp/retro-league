#define _CRT_SECURE_NO_WARNINGS 1

#include "Hud.h"
#include "GameState.h"
#include "Car.h"

#include "NetworkManager.h"
#include "AssetManager.h"
#include "Renderer.h"

#include "InputDevices.h"
#include "AssetManager.h"

DEFINE_NODE(Hud, Widget);

Hud::Hud()
{
    SetAnchorMode(AnchorMode::FullStretch);
    SetMargins(0.0f, 0.0f, 0.0f, 0.0f);

    // BG Quads
    mMatchBg = CreateChild<Quad>("MatchBg");
    mMatchBg->SetAnchorMode(AnchorMode::TopMid);
    mMatchBg->SetPosition(-75.0f, 0.0f);
    mMatchBg->SetDimensions(160, 50);
    mMatchBg->SetColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.60f));

    mBoostBg = CreateChild<Quad>("BoostBg");
    mBoostBg->SetAnchorMode(AnchorMode::BottomRight);
    mBoostBg->SetPosition(-120.0f, -60.0f);
    mBoostBg->SetDimensions(120.0f, 28.0f);
    mBoostBg->SetColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.60f));

    mWinnerBg = CreateChild<Quad>("WinnerBg");
    mWinnerBg->SetAnchorMode(AnchorMode::MidHorizontalStretch);
    mWinnerBg->SetLeftMargin(0.0f);
    mWinnerBg->SetRightMargin(0.0f);
    mWinnerBg->SetY(-50.0f);
    mWinnerBg->SetHeight(100.0f);
    mWinnerBg->SetColor({ 0.0f, 0.0f, 0.0f, 0.6f });
    mWinnerBg->SetVisible(false);

    mCountdownBg = CreateChild<Quad>("CountdownBg");
    mCountdownBg->SetAnchorMode(AnchorMode::Mid);
    mCountdownBg->SetPosition(-50.0f, -50.0f);
    mCountdownBg->SetDimensions(100.0f, 100.0f);
    mCountdownBg->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    mCountdownBg->SetTexture((Texture*)LoadAsset("T_SoftCircle"));
    mCountdownBg->SetVisible(false);

    // Boost Gauge
    //mBoostFuel = CreateChild<Quad>();
    //mBoostCapacity = CreateChild<Quad>();

    // Text Elements
    mScore0 = CreateChild<Text>("Score0");
    mScore0->SetAnchorMode(AnchorMode::TopMid);
    mScore0->SetPosition(-40.0f, 5.0f);
    mScore0->SetDimensions(100, 100);
    mScore0->SetTextSize(24.0f);

    mScore1 = CreateChild<Text>("Score1");
    mScore1->SetAnchorMode(AnchorMode::TopMid);
    mScore1->SetPosition(40.0f, 5.0f);
    mScore1->SetDimensions(100, 100);
    mScore1->SetTextSize(24.0f);

    mTime = CreateChild<Text>("Time");
    mTime->SetAnchorMode(AnchorMode::TopMid);
    mTime->SetPosition(-10.0f, 25.0f);
    mTime->SetDimensions(100, 100);
    mTime->SetTextSize(20.0f);

    mBoost = CreateChild<Text>("Boost");
    mBoost->SetAnchorMode(AnchorMode::BottomRight);
    mBoost->SetPosition(-105.0f, -60.0f);
    mBoost->SetDimensions(100, 50);
    mBoost->SetColor({ 0.9f, 0.8f, 0.1f, 1.0f });
    mBoost->SetTextSize(20.0f);

    mWinner = mWinnerBg->CreateChild<Text>("WinnerText");
    mWinner->SetAnchorMode(AnchorMode::Mid);
    mWinner->SetPosition(-200.0f, -35.0f);
    mWinner->SetDimensions(1000.0f, 200.0f);
    mWinner->SetTextSize(60.0f);
    mWinner->SetVisible(false);

    mCountdown = mCountdownBg->CreateChild<Text>("CountdownText");
    mCountdown->SetPosition({30.0f, 10.0f });
    mCountdown->SetDimensions({ 100.0f, 200.0f });
    mCountdown->SetTextSize(60.0f);
}

Hud::~Hud()
{
    // Children deleted in ~Widget()
}

void Hud::Tick(float deltaTime)
{
    Widget::Tick(deltaTime);

    MatchState* match = GetMatchState();

    if (match == nullptr || 
        match->mPhase == MatchPhase::Count)
        return;

    char textBuffer[32];

    sprintf(textBuffer, "%d", match->mTeams[0].mScore);
    mScore0->SetText(textBuffer);
    mScore0->SetColor(glm::vec4(1.0f, 0.75f, 0.20f, 1.0f));

    sprintf(textBuffer, "%d", match->mTeams[1].mScore);
    mScore1->SetText(textBuffer);
    mScore1->SetColor(glm::vec4(0.20f, 0.75f, 1.0f, 1.0f));


    if (match->mOvertime)
    {
        sprintf(textBuffer, "+%d", (int32_t)match->mTime);
    }
    else
    {
        sprintf(textBuffer, "%d", (int32_t)match->mTime);
    }
    mTime->SetText(textBuffer);

    if (match->mOwnedCar != nullptr)
    {
        sprintf(textBuffer, "Boost: %d", (int32_t)match->mOwnedCar->GetBoostFuel());
        mBoost->SetText(textBuffer);
    }

    // If game is finished, show winning team.
    if (match->mPhase == MatchPhase::Finished &&
        !mWinner->IsVisible())
    {
        mWinner->SetVisible(true);
        mWinnerBg->SetVisible(true);

        if (match->mTeams[0].mScore > match->mTeams[1].mScore)
        {
            mWinner->SetText("Orange Wins!");
            mWinner->SetColor({ 0.96f, 0.57f, 0.26f, 1.0f });
        }
        else if (match->mTeams[0].mScore < match->mTeams[1].mScore)
        {
            mWinner->SetText("Blue Wins!");
            mWinner->SetColor({ 0.26f, 0.52f, 0.96f, 1.0f });
        }
        else
        {
            mWinner->SetText("Draw!");
            mWinner->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        }

        float width = mWinner->GetTextWidth();
        float height = mWinner->GetTextHeight();
        mWinner->SetPosition(-width / 2.0f,
                             -height / 2.0f);
    }
    else if (match->mPhase != MatchPhase::Finished)
    {
        mWinner->SetVisible(false);
        mWinnerBg->SetVisible(false);
    }
}

void Hud::SetCountdownTime(int32_t seconds)
{
    mCountdownBg->SetVisible(seconds != 0);

    char textString[16];
    snprintf(textString, 16, "%d", seconds);
    mCountdown->SetText(textString);
}
