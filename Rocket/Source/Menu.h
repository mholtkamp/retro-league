#pragma once

#include "Nodes/Widgets/Canvas.h"
#include "Nodes/Widgets/Text.h"

#include "Assets/SoundWave.h"

#include "MenuPage.h"
#include "MenuOption.h"
#include "AssetRef.h"

class Menu : public Canvas
{
public:
    DECLARE_NODE(Menu, Canvas);
    Menu();

    virtual void Tick(float deltaTime) override;

    virtual void AddPage(MenuPage* page);
    virtual void PushPage(const char* name);
    virtual void PopPage();

protected:

    std::vector<MenuPage*> mPages;
    std::vector<MenuPage*> mPageStack;

    SoundWaveRef mBlipSound;
};

class TitleBanner : public Widget
{
public:
    DECLARE_NODE(TitleBanner, Widget);
    TitleBanner();
    virtual void Tick(float deltaTime) override;

protected:

    Quad* mBackgroundQuad = nullptr;
    Quad* mTitleQuad = nullptr;
};


class MainMenu : public Menu
{
public:
    DECLARE_NODE(MainMenu, Menu);

    MainMenu();
    virtual ~MainMenu();
    virtual void AddPage(MenuPage* page) override;
    virtual void Tick(float deltaTime) override;

    virtual void PushPage(const char* name) override;
    virtual void PopPage() override;

protected:

    SoundWaveRef mSongSound;
    TitleBanner* mTitleBanner = nullptr;
    Quad* mBackButton = nullptr;
    Quad* mCursor = nullptr;
};

