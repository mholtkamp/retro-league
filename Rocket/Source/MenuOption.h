#pragma once

#include "Nodes/Widgets/Widget.h"
#include "Nodes/Widgets/Text.h"
#include "NetworkManager.h"

typedef void(*MenuOptionCallbackFP)(class MenuOption* menuOption);

class MenuPage;

class MenuOption : public Widget
{
public:

    DECLARE_NODE(MenuOption, Widget);

    virtual void Create() override;

    void SetSelected(bool selected);
    bool IsSelected() const;

    void SetLocked(bool locked);
    bool IsLocked() const;

    MenuPage* GetPage();
    void SetPage(MenuPage* page);

    const std::string& GetLabel();
    void SetLabel(const char* label);

    void SetCallback(MenuOptionCallbackFP callback);

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

    DECLARE_NODE(MenuOptionEnum, MenuOption);

    virtual void Activate() override;

    virtual void Tick(float deltaTime) override;

    void SetEnumData(uint32_t enumCount, const char** enumStrings);

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
    DECLARE_NODE(MenuOptionSession, MenuOption);

    NetSession mSession;
};