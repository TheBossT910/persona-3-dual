#include "MainMenu.hpp"
#include "core/globals.hpp"
#include "events/GenericEvents.hpp"
#include "events/SaveEvents.hpp"
#include <string>

MainMenu* MainMenu::instance = nullptr;

void MainMenu::create()
{
    if (instance == nullptr)
    {
        instance = new MainMenu();
    }
}

void MainMenu::destroy()
{
    if (instance != nullptr)
    {
        delete instance;
    }
    instance = nullptr;
}

MainMenu* MainMenu::getInstance()
{
    if (instance == nullptr)
    {
        create();
    }
    return instance;
}

void MainMenu::resetHook()
{
    pauseMessage = "";
    options = mainMenuOptions;
}

void MainMenu::closeHook()
{
    resetMenu();
    ae::BroadcastEvent(Event::SwitchView{ViewState::INTRO});
}

// option handlers
ViewState MainMenu::mainMenuOptionSelected()
{
    ViewState selectedView;
    switch (static_cast<MainMenuOptions>(selectedOption))
    {
    case MainMenuOptions::LOAD_GAME:
        changeMenu(levelOptions);
        selectedView = ViewState::KEEP_CURRENT;
        break;
    case MainMenuOptions::SETTINGS:
        changeMenu(settingOptions);
        selectedView = ViewState::KEEP_CURRENT;
        break;
    case MainMenuOptions::RETURN_TO_TITLE:
        selectedView = ViewState::INTRO;
        break;
    default:
        selectedView = ViewState::KEEP_CURRENT;
    }

    return selectedView;
}

ViewState MainMenu::levelOptionSelected()
{
    ViewState selectedView;
    switch (static_cast<LevelOptions>(selectedOption))
    {
    case LevelOptions::START_GAME:
        selectedView = ViewState::CUTSCENE_1;
        break;
    case LevelOptions::IWATODAI_DORM:
        selectedView = ViewState::IWATODAI_DORM;
        break;
    case LevelOptions::IWATODAI_STREETS:
        selectedView = ViewState::IWATODAI_STREETS;
        break;
    case LevelOptions::STATION:
        selectedView = ViewState::STATION;
        break;
    case LevelOptions::PAULOWNIA_MALL:
        selectedView = ViewState::PAULOWNIA_MALL;
        break;
    case LevelOptions::SIGN_CONTRACT:
        selectedView = ViewState::SIGN_CONTRACT;
        break;
    default:
        selectedView = ViewState::KEEP_CURRENT;
    }

    return selectedView;
}

ViewState MainMenu::settingOptionSelected()
{
    ViewState selectedView;
    switch (static_cast<SettingOptions>(selectedOption))
    {
    case SettingOptions::CHANGE_INTRO_VIDEO:
        changeMenu(settingIntroOptions);
        selectedView = ViewState::KEEP_CURRENT;
        break;

    default:
        selectedView = ViewState::KEEP_CURRENT;
    }

    return selectedView;
}

ViewState MainMenu::settingIntroOptionSelected()
{
    switch (static_cast<SettingIntroOptions>(selectedOption))
    {
    case SettingIntroOptions::ORIGINAL:
        strncpy(saveData.introVideoPath, "base.vid", sizeof(saveData.introVideoPath));
        break;
    case SettingIntroOptions::FES:
        strncpy(saveData.introVideoPath, "fes.vid", sizeof(saveData.introVideoPath));
        break;
    case SettingIntroOptions::PORTABLE:
        strncpy(saveData.introVideoPath, "portable.vid", sizeof(saveData.introVideoPath));
        break;
    case SettingIntroOptions::RELOAD:
        strncpy(saveData.introVideoPath, "reload.vid", sizeof(saveData.introVideoPath));
        break;
    default:
        strncpy(saveData.introVideoPath, "reload.vid", sizeof(saveData.introVideoPath));
    }

    updateSave();
    return ViewState::KEEP_CURRENT;
}

void MainMenu::updateSave()
{
    ae::BroadcastEvent(Event::WriteSave{});
}
