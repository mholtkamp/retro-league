#include "MenuPage.h"
#include "Menu.h"
#include "GameState.h"

#include "Renderer.h"
#include "AudioManager.h"
#include "AssetManager.h"
#include "InputDevices.h"

#include "NetworkManager.h"

MenuPage::MenuPage(Menu* menu)
{
    mMenu = menu;
    SetVisible(false);
}

void MenuPage::SetOpen(bool open)
{
    if (mOpen != open)
    {
        SetVisible(open);
        mOpen = open;

        if (open)
        {
            mOpenedThisFrame = true;
        }
    }
}

void MenuPage::Update()
{
    Widget::Update();

    if (IsVisible() && !mOpenedThisFrame)
    {
        if (IsGamepadButtonJustDown(GAMEPAD_DOWN, 0))
        {
            NextOption();
        }
        else if (IsGamepadButtonJustDown(GAMEPAD_UP, 0))
        {
            PrevOption();
        }

        if (GetGamepadType(0) != GamepadType::Wiimote &&
            IsGamepadButtonJustDown(GAMEPAD_A, 0) &&
            mOptions.size() > 0)
        {
            mOptions[mSelectedOption]->Activate();
        }
        else if (IsGamepadButtonJustDown(GAMEPAD_B, 0))
        {
            mMenu->PopPage();
        }

        // See if the pointer contains any of the options.
        bool pointerJustUp = IsPointerJustUp(0);
        bool pointerDown = IsPointerDown(0);

        if (GetGamepadType(0) == GamepadType::Wiimote)
        {
            pointerJustUp = IsGamepadButtonJustUp(GAMEPAD_A, 0);
            pointerDown = IsGamepadButtonDown(GAMEPAD_A, 0);
        }

        if (pointerJustUp || pointerDown)
        {
            int32_t pointerX;
            int32_t pointerY;
            GetPointerPosition(pointerX, pointerY, 0);

            for (uint32_t i = 0; i < mOptions.size(); ++i)
            {
                bool containsPointer = mOptions[i]->ContainsPoint(pointerX, pointerY);

                if (containsPointer)
                {
                    if (pointerJustUp)
                    {
                        SetSelectedOption(i);
                        mOptions[i]->Activate();
                    }
                    else
                    {
                        SetSelectedOption(i);
                    }

                    break;
                }
            }
        }
    }

    mOpenedThisFrame = false;
}

const char* MenuPage::GetName() const
{
    return mName;
}

Menu* MenuPage::GetMenu()
{
    return mMenu;
}

void MenuPage::AddOption(MenuOption* option)
{
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
}

void MenuPage::NextOption()
{
    uint32_t numOptions = uint32_t(mOptions.size());
    uint32_t lastOption = (numOptions == 0) ? 0 : numOptions - 1u;

    if (mSelectedOption < lastOption)
    {
        mOptions[mSelectedOption]->SetSelected(false);
        mSelectedOption = (mSelectedOption + 1);
        mOptions[mSelectedOption]->SetSelected(true);
        AudioManager::PlaySound2D((SoundWave*)LoadAsset("SW_Blip"));
    }
}

void MenuPage::PrevOption()
{
    if (mSelectedOption > 0)
    {
        mOptions[mSelectedOption]->SetSelected(false);
        mSelectedOption = (mSelectedOption - 1);
        mOptions[mSelectedOption]->SetSelected(true);
        AudioManager::PlaySound2D((SoundWave*)LoadAsset("SW_Blip"));
    }
}

void MenuPage::SetSelectedOption(uint32_t optionIndex)
{
    if (optionIndex < mOptions.size() &&
        mSelectedOption != optionIndex)
    {
        mOptions[mSelectedOption]->SetSelected(false);
        mSelectedOption = optionIndex;
        mOptions[optionIndex]->SetSelected(true);
    }
}

MenuPageMain::MenuPageMain(Menu* menu) :
    MenuPage(menu)
{
    mName = "Main";
    MenuOption* createOption = new MenuOption(this, "Create Game", ActivateCreate);
    MenuOption* joinOption = new MenuOption(this, "Join Game", ActivateJoin);
    MenuOption* settingsOption = new MenuOption(this, "Settings", nullptr /*ActivateSettings*/);
    MenuOption* aboutOption = new MenuOption(this, "About", ActivateAbout);

    // Temporarily lock Settings
    settingsOption->SetLocked(true);

    AddOption(createOption);
    AddOption(joinOption);
    AddOption(settingsOption);
    AddOption(aboutOption);
}

void MenuPageMain::ActivateCreate(MenuOption* option)
{
    option->GetPage()->GetMenu()->PushPage("Create");
}

void MenuPageMain::ActivateJoin(MenuOption* option)
{
    option->GetPage()->GetMenu()->PushPage("Join");

    //NetworkManager::Get()->Connect("127.0.0.1");

    //NetworkManager::Get()->BeginSessionSearch();

}

void MenuPageMain::ActivateAbout(MenuOption* option)
{
    option->GetPage()->GetMenu()->PushPage("About");
}

MenuPageCreate::MenuPageCreate(Menu* mainMenu) :
    MenuPage(mainMenu)
{
    mName = "Create";

    static const char* teamSizeStrings[3] =
    {
        "1",
        "2",
        "3"
    };

    static const char* environmentStrings[2] =
    {
        "Pitch Black",
        "Lagoon"
    };

    static const char* networkStrings[3] = 
    {
        "Offline",
        "LAN",
        "Online"
    };

    mTeamSizeOption = new MenuOptionEnum(this, "Team Size", nullptr, 3, teamSizeStrings);
    mEnvironmentOption = new MenuOptionEnum(this, "Stage", nullptr, 2, environmentStrings);
    mNetworkOption = new MenuOptionEnum(this, "Network", nullptr, 3, networkStrings);
    mStartOption = new MenuOption(this, "Start", ActivateStart);

    PullOptions();

    AddOption(mTeamSizeOption);
    AddOption(mEnvironmentOption);
    AddOption(mNetworkOption);
    AddOption(mStartOption);
}

void MenuPageCreate::ActivateStart(MenuOption* option)
{
    MenuPageCreate* thisPage = (MenuPageCreate*) option->GetPage();
    thisPage->PushOptions();

    GetGameState()->mTransitionToGame = true;
}

void MenuPageCreate::PullOptions()
{
    mTeamSizeOption->SetEnumValue(GetGameState()->mMatchOptions.mTeamSize - 1);
    mEnvironmentOption->SetEnumValue((uint32_t)GetGameState()->mMatchOptions.mEnvironmentType);
    mNetworkOption->SetEnumValue((uint32_t)GetGameState()->mMatchOptions.mNetworkMode);
}

void MenuPageCreate::PushOptions()
{
    GetGameState()->mMatchOptions.mTeamSize = mTeamSizeOption->GetEnumValue() + 1;
    GetGameState()->mMatchOptions.mEnvironmentType = (EnvironmentType)mEnvironmentOption->GetEnumValue();
    GetGameState()->mMatchOptions.mNetworkMode = (NetworkMode)mNetworkOption->GetEnumValue();
}

MenuPageAbout::MenuPageAbout(Menu* mainMenu) :
    MenuPage(mainMenu)
{
    mName = "About";
    mAboutText = new Text();
    mAboutText->SetDimensions(400, 400);
    mAboutText->SetPosition(-100, 0);
    mAboutText->SetText("Created by Martin Holtkamp\nTwitter: @martin_holtkamp\n\nReleases and source code will be published at\ngithub.com/mholtkamp/retro-league\n\nVersion 0.2");
    
#if PLATFORM_3DS
    mAboutText->SetSize(14.0f);
    mAboutText->SetPosition(-60.0f, 0.0f);
#else
    mAboutText->SetSize(24.0f);
#endif
    AddChild(mAboutText);
}

void MenuPageAbout::Update()
{
    MenuPage::Update();

    if (mOpen && IsGamepadButtonJustDown(GAMEPAD_B, 0))
    {
        mMenu->PopPage();
    }
}

void MenuPageJoin::ActivateGame(MenuOption* option)
{
    MenuOptionSession* sessionOpt = (MenuOptionSession*) option;
    NetHost& host = sessionOpt->mSession.mHost;

    if (host.mIpAddress != 0)
    {
        NetworkManager::Get()->EndSessionSearch();
        NetworkManager::Get()->Connect(host.mIpAddress, host.mPort);
    }
}

MenuPageJoin::MenuPageJoin(Menu* mainMenu) :
    MenuPage(mainMenu)
{
    mName = "Join";

    for (uint32_t i = 0; i < sNumGameOptions; ++i)
    {
        mGameOptions[i] = new MenuOptionSession(this, "...", ActivateGame);
        AddOption(mGameOptions[i]);
    }

    mSearchText = new Text();
    mSearchText->SetText("Searching...");
    mSearchText->SetPosition(100, 145);
    mSearchText->SetDimensions(100, 40);
    mSearchText->SetSize(20);
    AddChild(mSearchText);
}

void MenuPageJoin::Update()
{
    MenuPage::Update();

    NetworkManager* netMan = NetworkManager::Get();

    if (netMan->IsSearching())
    {
        const std::vector<GameSession>& sessions = netMan->GetSessions();

        for (uint32_t i = 0; i < sNumGameOptions; ++i)
        {
            mGameOptions[i]->SetVisible(false);
        }

        mNumFilledOptions = 0;
        for (uint32_t i = 0; i < sessions.size() && mNumFilledOptions < sNumGameOptions; ++i)
        {
            mGameOptions[mNumFilledOptions]->mSession = sessions[i];

            char sessionName[64] = {};
            snprintf(sessionName, 63, "%s - (%d/%d)", sessions[i].mName, sessions[i].mNumPlayers, sessions[i].mMaxPlayers);
            mGameOptions[mNumFilledOptions]->SetLabel(sessionName);
            mGameOptions[mNumFilledOptions]->SetVisible(true);
            mNumFilledOptions++;
        }

        mSearchText->SetVisible(mNumFilledOptions == 0);
    }
}

void MenuPageJoin::SetOpen(bool open)
{
    if (mOpen != open)
    {
        if (open)
        {
            NetworkManager::Get()->BeginSessionSearch();

            for (uint32_t i = 0; i < sNumGameOptions; ++i)
            {
                mGameOptions[i]->mSession = {};
                mGameOptions[i]->SetLabel("...");
                mGameOptions[i]->SetVisible(false);
            }
        }
        else
        {
            NetworkManager::Get()->EndSessionSearch();
        }
    }

    MenuPage::SetOpen(open);
}


void MenuPageJoin::NextOption()
{
    if (mSelectedOption + 1 < mNumFilledOptions)
    {
        MenuPage::NextOption();
    }
}
void MenuPageJoin::PrevOption()
{
    MenuPage::PrevOption();
}

void MenuPageJoin::SetSelectedOption(uint32_t optionIndex)
{
    if (optionIndex < mNumFilledOptions)
    {
        MenuPage::SetSelectedOption(optionIndex);
    }
}