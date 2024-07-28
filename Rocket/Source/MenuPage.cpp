#include "MenuPage.h"
#include "Menu.h"
#include "GameState.h"

#include "Renderer.h"
#include "AudioManager.h"
#include "AssetManager.h"
#include "InputDevices.h"

#include "NetworkManager.h"

DEFINE_NODE(MenuPage, Widget);
DEFINE_NODE(MenuPageMain, MenuPage);
DEFINE_NODE(MenuPageCreate, MenuPage);
DEFINE_NODE(MenuPageAbout, MenuPage);
DEFINE_NODE(MenuPageJoin, MenuPage);

void MenuPage::Create()
{
    SetVisible(false);
    EnableScissor(false);
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

void MenuPage::Tick(float deltaTime)
{
    Widget::Tick(deltaTime);

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

void MenuPage::SetMenu(Menu* menu)
{
    mMenu = menu;
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

void MenuPageMain::Create()
{
    MenuPage::Create();

    mName = "Main";
    MenuOption* createOption = CreateOption<MenuOption>("Create Game", ActivateCreate);
    MenuOption* joinOption = CreateOption<MenuOption>("Join Game", ActivateJoin);
    MenuOption* settingsOption = CreateOption<MenuOption>("Settings", nullptr /*ActivateSettings*/);
    MenuOption* aboutOption = CreateOption<MenuOption>("About", ActivateAbout);

    // Temporarily lock Settings
    settingsOption->SetLocked(true);
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

void MenuPageCreate::Create()
{
    MenuPage::Create();

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

    mTeamSizeOption = CreateOption<MenuOptionEnum>("Team Size", nullptr);
    mEnvironmentOption = CreateOption<MenuOptionEnum>("Stage", nullptr);
    mNetworkOption = CreateOption<MenuOptionEnum>("Network", nullptr);
    mStartOption = CreateOption<MenuOption>("Start", ActivateStart);

    mTeamSizeOption->SetEnumData(3, teamSizeStrings);
    mEnvironmentOption->SetEnumData(2, environmentStrings);
    mNetworkOption->SetEnumData(3, networkStrings);


    PullOptions();
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

void MenuPageAbout::Create()
{
    MenuPage::Create();

    mName = "About";
    mAboutText = CreateChild<Text>();
    mAboutText->SetDimensions(400, 400);
    mAboutText->SetPosition(-100, 0);
    mAboutText->SetText("Created by Martin Holtkamp\nTwitter: @martin_holtkamp\n\nReleases and source code will be published at\ngithub.com/mholtkamp/retro-league\n\nVersion 0.2");
    
#if PLATFORM_3DS
    mAboutText->SetTextSize(14.0f);
    mAboutText->SetPosition(-60.0f, 0.0f);
#else
    mAboutText->SetTextSize(24.0f);
#endif
}

void MenuPageAbout::Tick(float deltaTime)
{
    MenuPage::Tick(deltaTime);

    if (mOpen && IsGamepadButtonJustDown(GAMEPAD_B, 0))
    {
        mMenu->PopPage();
    }
}

void MenuPageJoin::ActivateGame(MenuOption* option)
{
    MenuOptionSession* sessionOpt = (MenuOptionSession*) option;
    NetSession& session = sessionOpt->mSession;
    NetHost& host = session.mHost;

    NetworkManager::Get()->JoinSession(session);

    NetworkManager::Get()->EndSessionSearch();
}

void MenuPageJoin::Create()
{
    MenuPage::Create();

    mName = "Join";

    for (uint32_t i = 0; i < sNumGameOptions; ++i)
    {
        mGameOptions[i] = CreateOption<MenuOptionSession>("...", ActivateGame);
    }

    mSearchText = CreateChild<Text>("SearchText");
    mSearchText->SetText("Searching...");
    mSearchText->SetPosition(100, 145);
    mSearchText->SetDimensions(100, 40);
    mSearchText->SetTextSize(20);
}

void MenuPageJoin::Tick(float deltaTime)
{
    MenuPage::Tick(deltaTime);

    NetworkManager* netMan = NetworkManager::Get();

    if (netMan->IsSearching())
    {
        const std::vector<NetSession>& sessions = netMan->GetSessions();

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