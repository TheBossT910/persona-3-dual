#include "BattleMenu.hpp"
#include "core/globals.hpp"
#include "events/UIEvents.hpp"

BattleMenu* BattleMenu::instance = nullptr;

void BattleMenu::create()
{
    if (instance == nullptr)
    {
        instance = new BattleMenu();
    }
}

void BattleMenu::destroy()
{
    if (instance != nullptr)
    {
        delete instance;
    }
    instance = nullptr;
}

BattleMenu* BattleMenu::getInstance()
{
    if (instance == nullptr)
    {
        create();
    }
    return instance;
}

// option loaders
void BattleMenu::loadActionOptions(std::array<ActionBase*, 4>* actions, std::string name)
{
    // skip if action options have already been loaded
    if (loadedOption == BattleMenuOptions::ACTION)
    {
        return;
    };

    text->clearScreen();
    ae::BroadcastEvent(Event::RenderUIText{});

    // set new options
    battleOptions.clear();

    // indicate we loaded option
    loadedOption = BattleMenuOptions::ACTION;
    pauseMessage = name.c_str();
    int count = actions->size();

    for (int i = 0; i < count; i++)
    {
        MenuOption option = {actions->at(i)->name.c_str(), -1, MENU_BIND(BattleMenu, battleOptionSelected)};
        battleOptions.push_back(option);
    }

    options = battleOptions;
}

void BattleMenu::loadSkillOptions(PersonaBase* persona)
{
    // skip if action options have already been loaded
    if (loadedOption == BattleMenuOptions::SKILL)
    {
        return;
    };

    text->clearScreen();
    ae::BroadcastEvent(Event::RenderUIText{});

    // set new options
    battleOptions.clear();

    // indicate we loaded option
    loadedOption = BattleMenuOptions::SKILL;
    pauseMessage = "Skills";
    int count = persona->skillCount;

    for (int i = 0; i < count; i++)
    {
        MenuOption option = {persona->skills[i]->name.c_str(), -1, MENU_BIND(BattleMenu, battleOptionSelected)};
        battleOptions.push_back(option);
    }

    options = battleOptions;
}

void BattleMenu::loadPersonaOptions(etl::vector<PersonaBase*, 13>* personas)
{
    if (loadedOption == BattleMenuOptions::PERSONA)
    {
        return;
    }

    text->clearScreen();
    ae::BroadcastEvent(Event::RenderUIText{});

    battleOptions.clear();
    loadedOption = BattleMenuOptions::PERSONA;
    pauseMessage = "Persona";
    int count = (int)personas->size();

    for (int i = 0; i < count; i++)
    {
        MenuOption option = {personas->at(i)->name.c_str(), -1, MENU_BIND(BattleMenu, battleOptionSelected)};
        battleOptions.push_back(option);
    }

    options = battleOptions;
}

void BattleMenu::loadTargetOptions(etl::vector<BattleParticipant*, 13>* targets, bool healTarget)
{
    BattleMenuOptions targetLoadedOption =
        healTarget ? BattleMenuOptions::TARGET_HEAL : BattleMenuOptions::TARGET_ENEMY;

    if ((loadedOption == BattleMenuOptions::TARGET_HEAL) || (loadedOption == BattleMenuOptions::TARGET_ENEMY))
    {
        return;
    }

    text->clearScreen();
    ae::BroadcastEvent(Event::RenderUIText{});

    battleOptions.clear();
    loadedOption = targetLoadedOption;
    pauseMessage = "Target";
    int count = (int)targets->size();

    for (int i = 0; i < count; i++)
    {
        if (targets->at(i)->hp <= 0)
            continue;

        MenuOption option = {targets->at(i)->name.c_str(), -1, MENU_BIND(BattleMenu, battleOptionSelected)};
        battleOptions.push_back(option);
    }

    options = battleOptions;
}

void BattleMenu::loadAllOutAttackConfirmation()
{
    if (loadedOption == BattleMenuOptions::ALL_OUT_ATTACK)
    {
        return;
    };

    text->clearScreen();
    ae::BroadcastEvent(Event::RenderUIText{});

    battleOptions.clear();
    loadedOption = BattleMenuOptions::ALL_OUT_ATTACK;
    pauseMessage = "Confirm All-out-attack?";

    MenuOption yes = {"Yes", -1, MENU_BIND(BattleMenu, battleOptionSelected)};
    MenuOption no = {"No", -1, MENU_BIND(BattleMenu, battleOptionSelected)};
    battleOptions.push_back(yes);
    battleOptions.push_back(no);

    options = battleOptions;
}

void BattleMenu::loadAlertOptions(const std::string& text)
{
    if (loadedOption == BattleMenuOptions::ALERT)
    {
        return;
    }

    battleOptions.clear();
    loadedOption = BattleMenuOptions::ALERT;
    pauseMessage = text;
    alertStartFrame = frame;

    options = {};
}

bool BattleMenu::isAlertExpired(int durationFrames) const
{
    return (frame - alertStartFrame) >= durationFrames;
}

void BattleMenu::resetHook()
{
    pauseMessage = "";
    options = {};

    loadedOption = BattleMenuOptions::NONE;
    messagePrinted = false;
    selectedBattleOption = -1;
}

void BattleMenu::resetLoadedOptions()
{
    loadedOption = BattleMenuOptions::NONE;
    messagePrinted = false;
    text->clearScreen();

    pauseMessage = "";

    selectedOption = 0;
    startIndex = 0;
}

ViewState BattleMenu::updateHook()
{
    if (loadedOption == BattleMenuOptions::ALERT)
    {
        if (!messagePrinted)
        {
            text->clearScreen();
            messagePrinted = true;
            text->drawText(pauseMessage, 0, 0, 2);
        }
        return ViewState::KEEP_CURRENT;
    }

    return ViewState::DEFAULT;
}

// option handlers
ViewState BattleMenu::battleOptionSelected()
{
    selectedBattleOption = selectedOption;
    resetLoadedOptions();
    return ViewState::KEEP_CURRENT;
}

int BattleMenu::consumeSelectedBattleOption()
{
    int battleOption = selectedBattleOption;
    selectedBattleOption = -1;
    return battleOption;
}

bool BattleMenu::consumeCancel()
{
    bool result = isCancelled;
    isCancelled = false;
    return result;
}

void BattleMenu::prevOption()
{
    isCancelled = true;
    selectedBattleOption = -1;
    resetLoadedOptions();
}
