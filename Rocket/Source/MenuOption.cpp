#include "MenuOption.h"

#include "AudioManager.h"
#include "AssetManager.h"

#include "InputDevices.h"

DEFINE_NODE(MenuOption, Widget);
DEFINE_NODE(MenuOptionEnum, Widget);
DEFINE_NODE(MenuOptionSession, Widget);

void MenuOption::Create()
{
    Widget::Create();

    mText = CreateChild<Text>();

#if PLATFORM_3DS
    mText->SetTextSize(20.0f);
#else
    mText->SetTextSize(24.0f);
#endif
    mText->SetDimensions(400.0f, 50.0f);
    mText->SetText("NO LABEL");

    SetSelected(false);
}

void MenuOption::SetSelected(bool selected)
{
    mSelected = selected;
    UpdateTextColor();
}

bool MenuOption::IsSelected() const
{
    return mSelected;
}

void MenuOption::SetLocked(bool locked)
{
    if (mLocked != locked)
    {
        mLocked = locked;
        UpdateTextColor();
    }
}

bool MenuOption::IsLocked() const
{
    return mLocked;
}

MenuPage* MenuOption::GetPage()
{
    return mPage;
}

void MenuOption::SetPage(MenuPage* page)
{
    mPage = page;
}

void MenuOption::Activate()
{
    if (mCallback != nullptr &&
        !IsLocked())
    {
        mCallback(this);
    }

    AudioManager::PlaySound2D((SoundWave*)LoadAsset("SW_Blip"), 1.0f, 1.2f);
}

void MenuOption::UpdateTextColor()
{
    glm::vec4 textColor = mSelected ? glm::vec4(1.0f, 0.7f, 0.5f, 1.0f) : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    if (IsLocked())
    {
        textColor = mSelected ? glm::vec4(0.5f, 0.35f, 0.25f, 1.0f) : glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
    }

    mText->SetColor(textColor);
}

const std::string& MenuOption::GetLabel()
{
    return mLabel;
}

void MenuOption::SetLabel(const char* label)
{
    mLabel = label;
    mText->SetText(mLabel);
}

void MenuOption::SetCallback(MenuOptionCallbackFP callback)
{
    mCallback = callback;
}

void MenuOptionEnum::Activate()
{
    if (!mLocked)
    {
        SetEnumValue((mEnumValue + 1) % mEnumCount);
        MenuOption::Activate();
    }
}

void MenuOptionEnum::Tick(float deltaTime)
{
    MenuOption::Tick(deltaTime);

    if (mSelected && !mLocked)
    {
        if (IsGamepadButtonJustDown(GAMEPAD_LEFT, 0))
        {
            uint32_t newValue = (mEnumValue == 0) ? mEnumCount - 1 : mEnumValue - 1;
            SetEnumValue(newValue);
            AudioManager::PlaySound2D((SoundWave*)LoadAsset("SW_Blip"), 1.0f, 0.8f);
        }
        else if (IsGamepadButtonJustDown(GAMEPAD_RIGHT, 0))
        {
            SetEnumValue(mEnumValue + 1);
            AudioManager::PlaySound2D((SoundWave*)LoadAsset("SW_Blip"), 1.0f, 1.2f);
        }
    }
}

void MenuOptionEnum::SetEnumData(uint32_t enumCount, const char** enumStrings)
{
    mEnumCount = enumCount;
    mEnumStrings = enumStrings;
    SetEnumValue(0);
}

void MenuOptionEnum::SetEnumValue(uint32_t value)
{
    uint32_t modValue = value % mEnumCount;

    mEnumValue = modValue;
    mText->SetText(mLabel + ": " + mEnumStrings[mEnumValue]);
}

uint32_t MenuOptionEnum::GetEnumValue() const
{
    return mEnumValue;
}
