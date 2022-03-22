#pragma once

#include "Widgets/Widget.h"
#include "Widgets/Text.h"
#include "NetworkManager.h"

typedef void(*MenuOptionCallbackFP)(class MenuOption* menuOption);

class MenuPage;

class MenuOption : public Widget
{
public:
    MenuOption(MenuPage* page, const char* label, MenuOptionCallbackFP callback);

    void SetSelected(bool selected);
    bool IsSelected() const;

    void SetLocked(bool locked);
    bool IsLocked() const;

    MenuPage* GetPage();

    const std::string& GetLabel();
    void SetLabel(const char* label);

    virtual void Activate();

protected:

    void UpdateTextColor();

    MenuPage* mPage = nullptr;
    Text* mText = nullptr;
    std::string mLabel;
    MenuOptionCallbackFP mCallback = nullptr;
    bool mSelected = false;
    bool mLocked = false;
};

// TODO: Implement this when adding Name field or something like that.
#if 0
class MenuOptionString : public MenuOption
{
    // Allows player to edit a text string
public:
    MenuOptionString(
        const char* label,
        const char* editText,
        MenuOptionCallbackFP callback);
    virtual void Activate() override;

    std::string mEditString;
};
#endif

class MenuOptionEnum : public MenuOption
{
    // Selecting this option will cycle through an array of values
public:
    MenuOptionEnum(
        MenuPage* page,
        const char* label,
        MenuOptionCallbackFP callback,
        uint32_t enumCount,
        const char** enumStrings);
    virtual void Activate() override;

    virtual void Update() override;

    
    void SetEnumValue(uint32_t value);
    uint32_t GetEnumValue() const;

protected:


    const char** mEnumStrings = nullptr;
    uint32_t mEnumCount = 0;
    uint32_t mEnumValue = 0;
};

class MenuOptionSession : public MenuOption
{
public:

    MenuOptionSession(MenuPage* page, const char* label, MenuOptionCallbackFP callback);

    GameSession mSession;
};