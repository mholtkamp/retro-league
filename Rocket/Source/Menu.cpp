#include "Menu.h"
#include "MenuPage.h"

#include "Renderer.h"
#include "InputDevices.h"
#include "AudioManager.h"
#include "AssetManager.h"
#include "Widgets/Quad.h"

#include "Engine.h"

#include "Input/Input.h"

Menu::Menu()
{
    mBlipSound = LoadAsset("SW_Blip");
}

void Menu::Update()
{
    Widget::Update();

}

void Menu::AddPage(MenuPage* page)
{
    AddChild(page);
    mPages.push_back(page);

    page->SetAnchorMode(AnchorMode::Mid);
    page->SetDimensions(400, 300);
#if PLATFORM_3DS
    page->SetAnchorMode(AnchorMode::TopLeft);
    page->SetPosition(60.0f, 50.0f);
#else
    page->SetPosition(-140.0f, 0.0f);
#endif
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
    glm::vec2 ires = Renderer::Get()->GetScreenResolution();
    glm::vec2 mid = ires / 2.0f;

    SetAnchorMode(AnchorMode::FullStretch);
    SetMargins(0.0f, 0.0f, 0.0f, 0.0f);

    mTitleBanner = new TitleBanner();
    mTitleBanner->SetAnchorMode(AnchorMode::MidHorizontalStretch);
    mTitleBanner->SetLeftMargin(0.0f);
    mTitleBanner->SetRightMargin(0.0f);
    mTitleBanner->SetY(-170.0f);
    mTitleBanner->SetHeight(100.0f);

#if PLATFORM_3DS
    mTitleBanner->SetY(-50.0f);

    // Place the title banner on the top screen
    Renderer::Get()->AddWidget(mTitleBanner, -1, 0);

    mBackButton = new Quad();
    mBackButton->SetRect(0.0f, 240.0f - 32.0f, 32.0f, 32.0f);
    mBackButton->SetTexture((Texture*)LoadAsset("T_BackButton"));
    mBackButton->SetVisible(false);
    AddChild(mBackButton);
#else
    AddChild(mTitleBanner);
#endif

    AddPage(new MenuPageMain(this));
    AddPage(new MenuPageCreate(this));
    AddPage(new MenuPageJoin(this));
    AddPage(new MenuPageAbout(this));

    // TODO: Menu song?
    mSongSound = LoadAsset("SW_Song2_Mini");
    AudioManager::PlaySound2D(mSongSound.Get<SoundWave>(), 1.0f, 1.0f, 0.0f, true, 1);

    PushPage("Main");

    if (INPUT_MOUSE_SUPPORT)
    {
        mCursor = new Quad();
        mCursor->SetRect(0, 0, 32, 32);
        mCursor->SetTexture((Texture*)LoadAsset("T_Ring"));
        AddChild(mCursor);
    }
}

MainMenu::~MainMenu()
{
    AudioManager::StopAllSounds();

#if PLATFORM_3DS
    // We need to manually remove the title and free the memory.
    // On all other platforms the title will remain a child so ~Widget() will free it.
    Renderer::Get()->RemoveWidget(mTitleBanner, 0);
    delete mTitleBanner;
    mTitleBanner = nullptr;
#endif
}

void MainMenu::AddPage(MenuPage* page)
{
    Menu::AddPage(page);
}

void MainMenu::Update()
{
    Menu::Update();

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
}

void MainMenu::PushPage(const char* name)
{
    Menu::PushPage(name);

#if PLATFORM_3DS
    // Show a back button on 3DS
    mBackButton->SetVisible(mPageStack.size() > 1);
#endif
}

void MainMenu::PopPage()
{
    Menu::PopPage();

#if PLATFORM_3DS
    // Show a back button on 3DS
    mBackButton->SetVisible(mPageStack.size() > 1);
#endif
}

TitleBanner::TitleBanner()
{
    glm::vec2 ires = Renderer::Get()->GetScreenResolution();

    mBackgroundQuad = new Quad();
    mBackgroundQuad->SetAnchorMode(AnchorMode::FullStretch);
    mBackgroundQuad->SetMargins(0.0f, 0.0f, 0.0f, 0.0f);
    mBackgroundQuad->SetTexture((Texture*)LoadAsset("T_Checkers"));
    mBackgroundQuad->SetUvScale({ ires.x / 20.0f, 100.0f / 20.0f });
    mBackgroundQuad->SetColor(
        { 1.0f, 0.5f, 0.2f, 1.0f },
        { 1.0f, 0.5f, 0.2f, 1.0f },
        { 0.0f, 0.3f, 1.0f, 1.0f },
        { 0.0f, 0.3f, 1.0f, 1.0f });
    AddChild(mBackgroundQuad);

    mTitleQuad = new Quad();
    mTitleQuad->SetAnchorMode(AnchorMode::Mid);
    mTitleQuad->SetTexture((Texture*)LoadAsset("T_TitleText"));
    mTitleQuad->SetDimensions(384, 48);
    mTitleQuad->SetPosition(-196.0f, -26.0f);
    AddChild(mTitleQuad);
}

void TitleBanner::Update()
{
    Widget::Update();

    glm::vec2 uvOffset = mBackgroundQuad->GetUvOffset();
    uvOffset += GetAppClock()->DeltaTime() * glm::vec2(0.03f, 0.05f);
    mBackgroundQuad->SetUvOffset(uvOffset);
}