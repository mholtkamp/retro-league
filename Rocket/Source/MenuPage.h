#pragma once

#include "Widgets/Widget.h"
#include "MenuOption.h"

class Menu;

class MenuPage : public Widget
{
public:

    MenuPage(Menu* mainMenu);
    virtual void SetOpen(bool open);
    virtual void Update() override;
    const char* GetName() const;
    Menu* GetMenu();

protected:

    virtual void AddOption(MenuOption* option);
    virtual void NextOption();
    virtual void PrevOption();
    virtual void SetSelectedOption(uint32_t optionIndex);

    std::vector<MenuOption*> mOptions;
    Menu* mMenu = nullptr;
    const char* mName = "";
    uint32_t mSelectedOption = 0;
    bool mOpen = false;
    bool mOpenedThisFrame = false;
};

class MenuPageMain : public MenuPage
{
public:
    MenuPageMain(Menu* menu);

    static void ActivateCreate(MenuOption* option);
    static void ActivateAbout(MenuOption* option);
    static void ActivateJoin(MenuOption* option);
    //static void ActivateSettings(MenuOption* option);
};

class MenuPageCreate : public MenuPage
{
public:
    MenuPageCreate(Menu* mainMenu);

    static void ActivateStart(MenuOption* option);

    MenuOptionEnum* mTeamSizeOption = nullptr;
    MenuOptionEnum* mEnvironmentOption = nullptr;
    MenuOptionEnum* mNetworkOption = nullptr;
    MenuOption* mStartOption = nullptr;
};

class MenuPageAbout : public MenuPage
{
public:
    MenuPageAbout(Menu* mainMenu);

    virtual void Update() override;

    Text* mAboutText = nullptr;
};

class MenuPageJoin : public MenuPage
{
public:
    MenuPageJoin(Menu* mainMenu);
    virtual void Update() override;
    virtual void SetOpen(bool open) override;

    virtual void NextOption() override;
    virtual void PrevOption() override;
    virtual void SetSelectedOption(uint32_t optionIndex) override;

    static const uint32_t sNumGameOptions = 4;
    static void ActivateGame(MenuOption* option);

    MenuOptionSession* mGameOptions[sNumGameOptions] = {};
    Text* mSearchText = nullptr;
    uint32_t mNumFilledOptions = 0;
};

//class MenuPageSettings : public MenuPage
//{
//public:
//    MenuPageSettings(Menu* mainMenu);
//};
