#pragma once

#include "Nodes/Widgets/Widget.h"
#include "MenuOption.h"

class Menu;

class MenuPage : public Widget
{
public:

    DECLARE_NODE(MenuPage, Widget);

    virtual void Create() override;;
    virtual void SetOpen(bool open);
    virtual void Tick(float deltaTime) override;
    const char* GetName() const;
    Menu* GetMenu();
    void SetMenu(Menu* menu);



    template<class T>
    T* CreateOption(const char* label, MenuOptionCallbackFP callback)
    {
        T* option = CreateChild<T>(label);
        option->SetPage(this);
        option->SetLabel(label);
        option->SetCallback(callback);

        uint32_t prevNum = (uint32_t)mOptions.size();
#if PLATFORM_3DS
        option->SetPosition(50.0f, prevNum * 24.0f);
#else
        option->SetPosition(50.0f, prevNum * 32.0f);
#endif
        option->SetDimensions(200.0f, 25.0f);
        AddChild(option);
        mOptions.push_back(option);

        if (prevNum == 0)
        {
            option->SetSelected(true);
            mSelectedOption = 0;
        }

        return option;
    }

protected:

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
    DECLARE_NODE(MenuPageMain, MenuPage);

    virtual void Create() override;

    static void ActivateCreate(MenuOption* option);
    static void ActivateAbout(MenuOption* option);
    static void ActivateJoin(MenuOption* option);
    //static void ActivateSettings(MenuOption* option);
};

class MenuPageCreate : public MenuPage
{
public:
    DECLARE_NODE(MenuPageCreate, MenuPage);

    virtual void Create() override;

    static void ActivateStart(MenuOption* option);
    void PullOptions();
    void PushOptions();

    MenuOptionEnum* mTeamSizeOption = nullptr;
    MenuOptionEnum* mEnvironmentOption = nullptr;
    MenuOptionEnum* mNetworkOption = nullptr;
    MenuOption* mStartOption = nullptr;
};

class MenuPageAbout : public MenuPage
{
public:
    DECLARE_NODE(MenuPageAbout, MenuPage);

    virtual void Create() override;

    virtual void Tick(float deltaTime) override;

    Text* mAboutText = nullptr;
};

class MenuPageJoin : public MenuPage
{
public:
    DECLARE_NODE(MenuPageJoin, MenuPage);

    virtual void Create() override;

    virtual void Tick(float deltaTime) override;
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
