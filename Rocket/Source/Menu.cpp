#include "Menu.h"
#include "MenuPage.h"
#include "GameState.h"

#include "Renderer.h"
#include "InputDevices.h"
#include "AudioManager.h"
#include "AssetManager.h"
#include "Nodes/Widgets/Quad.h"

#include "Engine.h"

#include "Input/Input.h"

DEFINE_NODE(Menu, Canvas);
DEFINE_NODE(TitleBanner, Widget);
DEFINE_NODE(MainMenu, Menu);

Menu::Menu()
{
    mBlipSound = LoadAsset("SW_Blip");
}

void Menu::Tick(float deltaTime)
{
    Widget::Tick(deltaTime);

}

void Menu::AddPage(MenuPage* page)
{
    AddChild(page);
    mPages.push_back(page);
    page->SetMenu(this);

    page->SetAnchorMode(AnchorMode::Mid);
    page->SetDimensions(400, 300);
    page->SetPosition(-140.0f, 0.0f);
}

void Menu::PushPage(const char* name)
{
    // Save off old page
    MenuPage* oldPage = mPageStack.empty() ? nullptr : *(mPageStack.end() - 1);

    // Find the new page
    MenuPage* newPage = nullptr;
    for (uint32_t i = 0; i < mPages.size(); ++i)
    {
        if (strncmp(mPages[i]->GetName(), name, 128) == 0)
        {
            newPage = mPages[i];
            break;
        }
    }

    if (newPage != nullptr)
    {
        // If we found the new page, then close the old page and open the new page.
        // And push the new page on to the stack.
        if (oldPage != nullptr)
        {
            oldPage->SetOpen(false);
        }

        newPage->SetOpen(true);

        mPageStack.push_back(newPage);
    }
}

void Menu::PopPage()
{
    if (int32_t(mPageStack.size()) > 1)
    {
        MenuPage* poppedPage = mPageStack[int32_t(mPageStack.size()) - 1];
        mPageStack.erase(mPageStack.end() - 1);
        poppedPage->SetOpen(false);

        MenuPage* newPage = mPageStack[int32_t(mPageStack.size()) - 1];
        newPage->SetOpen(true);

        AudioManager::PlaySound2D(mBlipSound.Get<SoundWave>(), 1.0f, 0.5f);
    }
}

MainMenu::MainMenu()
{
    SetAnchorMode(AnchorMode::FullStretch);
    SetMargins(0.0f, 0.0f, 0.0f, 0.0f);

    mTitleBanner = Node::Construct<TitleBanner>();
    mTitleBanner->SetAnchorMode(AnchorMode::MidHorizontalStretch);
    mTitleBanner->SetLeftMargin(0.0f);
    mTitleBanner->SetRightMargin(0.0f);
    mTitleBanner->SetY(-170.0f);
    mTitleBanner->SetHeight(100.0f);
    AddChild(mTitleBanner);

    MenuPageMain* pageMain = Node::Construct<MenuPageMain>();
    MenuPageCreate* pageCreate = Node::Construct<MenuPageCreate>();
    MenuPageJoin* pageJoin = Node::Construct<MenuPageJoin>();
    MenuPageAbout* pageAbout = Node::Construct<MenuPageAbout>();

    AddPage(pageMain);
    AddPage(pageCreate);
    AddPage(pageJoin);
    AddPage(pageAbout);

    // TODO: Menu song?
    mSongSound = LoadAsset("SW_Song2_Mini");
    AudioManager::PlaySound2D(mSongSound.Get<SoundWave>(), 1.0f, 1.0f, 0.0f, true, 1);

    PushPage("Main");

    if (INPUT_MOUSE_SUPPORT)
    {
        mCursor = CreateChild<Quad>("Cursor");
        mCursor->SetRect(0, 0, 32, 32);
        mCursor->SetTexture((Texture*)LoadAsset("T_Ring"));
    }
}

MainMenu::~MainMenu()
{
    AudioManager::StopAllSounds();
}

void MainMenu::AddPage(MenuPage* page)
{
    Menu::AddPage(page);
}

void MainMenu::Tick(float deltaTime)
{
    Menu::Tick(deltaTime);

    int32_t x;
    int32_t y;
    GetPointerPosition(x, y, 0);

    if (mBackButton != nullptr &&
        mBackButton->IsVisible() &&
        IsPointerJustUp(0))
    {
        if (mBackButton->ContainsPoint(x, y))
        {
            PopPage();
        }
    }

    if (INPUT_MOUSE_SUPPORT)
    {
        if (GetGamepadType(0) == GamepadType::Wiimote)
        {
            mCursor->SetVisible(true);
            mCursor->SetPosition(x - 16.0f, y - 16.0f);
        }
        else
        {
            mCursor->SetVisible(false);
        }
    }

    if (IsGamepadButtonJustDown(GAMEPAD_Y, 0))
    {
        SYS_UnmountMemoryCard();
    }

    if (IsGamepadButtonJustDown(GAMEPAD_X, 0))
    {
        GetGameState()->DeletePreferredMatchOptions();
    }

    if (IsGamepadButtonJustDown(GAMEPAD_L1, 0))
    {
        GetGameState()->LoadPreferredMatchOptions();

        // Refresh Options
        for (uint32_t i = 0; i < mPages.size(); ++i)
        {
            if (strcmp(mPages[i]->GetName(), "Create") == 0)
            {
                ((MenuPageCreate*)mPages[i])->PullOptions();
                break;
            }
        }
    }

    if (IsGamepadButtonJustDown(GAMEPAD_R1, 0))
    {
        // Sync Options
        for (uint32_t i = 0; i < mPages.size(); ++i)
        {
            if (strcmp(mPages[i]->GetName(), "Create") == 0)
            {
                ((MenuPageCreate*)mPages[i])->PushOptions();
                break;
            }
        }
        GetGameState()->SavePreferredMatchOptions();
    }
}

void MainMenu::PushPage(const char* name)
{
    Menu::PushPage(name);
}

void MainMenu::PopPage()
{
    Menu::PopPage();
}

TitleBanner::TitleBanner()
{
    glm::vec2 ires = Renderer::Get()->GetScreenResolution();

    mBackgroundQuad = CreateChild<Quad>("Background");
    mBackgroundQuad->SetAnchorMode(AnchorMode::FullStretch);
    mBackgroundQuad->SetMargins(0.0f, 0.0f, 0.0f, 0.0f);
    mBackgroundQuad->SetTexture((Texture*)LoadAsset("T_Checkers"));
    mBackgroundQuad->SetUvScale({ ires.x / 20.0f, 100.0f / 20.0f });
    mBackgroundQuad->SetColor(
        { 1.0f, 0.5f, 0.2f, 1.0f },
        { 1.0f, 0.5f, 0.2f, 1.0f },
        { 0.0f, 0.3f, 1.0f, 1.0f },
        { 0.0f, 0.3f, 1.0f, 1.0f });

    mTitleQuad = CreateChild<Quad>("Title");
    mTitleQuad->SetAnchorMode(AnchorMode::Mid);
    mTitleQuad->SetTexture((Texture*)LoadAsset("T_TitleText"));
    mTitleQuad->SetDimensions(384, 48);
    mTitleQuad->SetPosition(-196.0f, -26.0f);
}

void TitleBanner::Tick(float deltaTime)
{
    Widget::Tick(deltaTime);

    glm::vec2 uvOffset = mBackgroundQuad->GetUvOffset();
    uvOffset += GetAppClock()->DeltaTime() * glm::vec2(0.03f, 0.05f);
    mBackgroundQuad->SetUvOffset(uvOffset);
}