#include "UISystem.hpp"
#include "components/menus/BattleMenu.hpp"
#include "components/menus/MainMenu.hpp"
#include "components/menus/PauseMenu.hpp"
#include "components/screens/DialogueScreen.hpp"
#include "components/screens/MenuHUDScreen.hpp"
#include "core/globals.hpp"
#include "events/GenericEvents.hpp"
#include <aegis/ndsTypes.hpp>
#include <aegis/types.hpp>

void UISystem::Init()
{
    isActive = false;
    renderUIText = false;

    // load sfx
    musicCtrl->loadSFX(SFX_MENU);
    musicCtrl->loadSFX(SFX_SELECT);
    musicCtrl->loadSFX(SFX_CANCEL);
}

void UISystem::Update(ae::q20_12_t dt)
{
    // skip if nullptr or not active
    if ((activeMenu == nullptr) || !activeMenu->isActive)
    {
        return;
    }

    // run the hook
    ViewState updateHookState = activeMenu->updateHook();
    if (updateHookState != ViewState::DEFAULT)
    {
        ae::BroadcastEvent(Event::SwitchView{updateHookState});
        // TODO: remove after musicCtrl refactor for aegis engine compliance
        musicCtrl->update();
        return;
    }

    // navigate options
    if (systemKeysDown & KEY_DOWN)
    {
        sfxMenuHandle = musicCtrl->playSFX(SFX_MENU, 255, 128);
        activeMenu->selectedOption = (activeMenu->selectedOption + 1) % activeMenu->options.size();
        renderUIText = true;
    }
    else if (systemKeysDown & KEY_UP)
    {
        sfxMenuHandle = musicCtrl->playSFX(SFX_MENU, 255, 128);
        activeMenu->selectedOption =
            (activeMenu->selectedOption + activeMenu->options.size() - 1) % activeMenu->options.size();
        renderUIText = true;
    }

    // Adjust scroll position
    if (activeMenu->selectedOption < activeMenu->startIndex)
    {
        activeMenu->startIndex = activeMenu->selectedOption;
        text->clearScreen();
        renderUIText = true;
    }
    else if (activeMenu->selectedOption >= activeMenu->startIndex + activeMenu->visibleOptions)
    {
        activeMenu->startIndex = activeMenu->selectedOption - activeMenu->visibleOptions + 1;
        text->clearScreen();
        renderUIText = true;
    }

    if (systemKeysDown & KEY_A)
    {
        cancelSFX();
        sfxSelectHandle = musicCtrl->playSFX(SFX_SELECT, 255, 128);
        text->clearScreen();
        renderUIText = true;

        if (activeMenu->options[activeMenu->selectedOption].onSelect != nullptr)
        {
            ViewState result = (activeMenu->*(activeMenu->options[activeMenu->selectedOption].onSelect))();
            if (result != ViewState::KEEP_CURRENT)
            {
                activeMenu->nextViewState = result;
                activeMenu->isActive = false;
            }
        }
    }
    else if (systemKeysDown & KEY_B)
    {
        cancelSFX();
        musicCtrl->playSFX(SFX_CANCEL, 255, 128);
        activeMenu->selectedOption = 0;
        activeMenu->startIndex = 0;
        text->clearScreen();
        renderUIText = true;
        activeMenu->prevOption();
    }

    // blink the "Pause" text
    if (activeMenu->pauseMessage.length() != 0)
    {
        if (frame % 60 < 30)
        {
            text->drawText(activeMenu->pauseMessage, 0, 0, 2);
        }
        else
        {
            text->clearArea(0, 0, 256, text->getFontSize() + text->getLineSpacing());
        }
    }

    // display options
    int textSize = text->getFontSize();
    if (renderUIText)
    {
        renderUIText = false;
        for (int i = 0; i < activeMenu->visibleOptions && activeMenu->startIndex + i < int(activeMenu->options.size());
             i++)
        {
            int option = activeMenu->startIndex + i;
            TextColor color = option == activeMenu->selectedOption ? TextColor::Blue : TextColor::White;
            text->drawText(activeMenu->options[option].name, 10, textSize + textSize * i, color);
        }
    }

    if (activeMenu->nextViewState != ViewState::KEEP_CURRENT)
    {
        ae::BroadcastEvent(Event::SwitchView{activeMenu->nextViewState});
    }
}

void UISystem::Shutdown()
{
    resetUIResources();
}

void UISystem::on_receive(const Event::SwitchView& msg)
{
    nextView = msg.view;
}

void UISystem::on_receive(const Event::ResetUIResources& /*msg*/)
{
    resetUIResources();
}

void UISystem::on_receive(const Event::ConfigureUIScreen& config)
{
    // reset previous config
    cleanupScreens();

    // set background
    oamSub = config.oamSub;
    oamMain = config.oamMain;

    lruBgSub = config.bgSub;
    hwBgSub = config.bgSub;

    lruBgMain = config.bgMain;
    hwBgMain = config.bgMain;

    hideAllScreens();

    // register screens
    for (UIScreen* screen : config.screens)
    {
        if (screen == nullptr)
        {
            continue;
        }

        registerScreen(screen);
    }
}

void UISystem::on_receive(const Event::ConfigureUIMenu& config)
{
    // reset previous config
    cleanupMenus();

    isActive = true;
    menus = config.menus;
    text = config.text;

    int textSize = text->getFontSize();

    for (UIMenu*& menu : menus)
    {
        // skip if nullptr
        if (menu == nullptr)
        {
            continue;
        }

        menu->isActive = false;
        menu->text = text;
        /// @note 192px is the screen height. 1 row is reserved for the flashing text. Each line takes the font size height
        menu->visibleOptions = (192 / textSize) - 1;
    }
}

void UISystem::on_receive(const Event::ShowScreen& msg)
{
    // check if it is already loaded. If not, load it in
    if (!msg.screen->isLoaded)
    {
        // add screen if space
        if (msg.screen->isMain ? (screenMainCount < 2) : (screenSubCount < 3))
        {
            registerScreen(msg.screen);
        }
        // swap out screens if no space
        else
        {
            UIScreen* oldScreen = nullptr;

            // remove the least recently displayed screen
            if (msg.screen->isMain)
            {
                int targetBgId = lruBgMain[0];
                for (int i = 0; i < 2; i++)
                {
                    if ((loadedMain[i] != nullptr) && (loadedMain[i]->bgId == targetBgId))
                    {
                        oldScreen = loadedMain[i];
                        loadedMain[i] = msg.screen;
                        break;
                    }
                }
            }
            else
            {
                int targetBgId = lruBgSub[0];
                for (int i = 0; i < 3; i++)
                {
                    if ((loadedSub[i] != nullptr) && (loadedSub[i]->bgId == targetBgId))
                    {
                        oldScreen = loadedSub[i];
                        loadedSub[i] = msg.screen;
                        break;
                    }
                }
            }

            oldScreen->unload();
            oldScreen->isLoaded = false;

            msg.screen->bgId = oldScreen->bgId;
            msg.screen->oam = oldScreen->oam;
            oldScreen->bgId = -1;
            oldScreen->oam = nullptr;

            msg.screen->load();
            msg.screen->isLoaded = true;
        }
    }

    // load screen
    hideAllScreens();

    /// display sprites on screen
    msg.screen->renderSprites();

    /// display background on screen
    render.showBg(msg.screen->bgId);

    // update the "least recently updated" index
    lruUpdate(msg.screen->bgId, msg.screen->isMain);
}

void UISystem::on_receive(const Event::HideAllScreens& /*msg*/)
{
    hideAllScreens();
}

void UISystem::on_receive(const Event::ShowMenu& msg)
{
    if (activeMenu != nullptr)
    {
        activeMenu->isActive = false;
    }

    renderUIText = true;
    activeMenu = msg.menu;
    activeMenu->resetMenu();
    activeMenu->isActive = true;
}

void UISystem::on_receive(const Event::HideAllMenus& /*msg*/)
{
    renderUIText = false;
    if (activeMenu != nullptr)
    {
        activeMenu->resetMenu();
        activeMenu = nullptr;
    }
}

void UISystem::on_receive(const Event::RenderUIText& /*msg*/)
{
    renderUIText = true;
}

void UISystem::lruUpdate(int id, bool isMain)
{
    if (isMain)
    {
        int pos = 0;

        // find where the existing id currently is
        for (int i = 0; i < 2; i++)
        {
            if (lruBgMain[i] == id)
            {
                pos = i;
                break;
            }
        }

        // shift everything after that position to the left to close the gap
        for (int i = pos; i < 1; i++)
        {
            lruBgMain[i] = lruBgMain[i + 1];
        }

        // place the updated id at the very end (most recently used)
        lruBgMain[1] = id;
    }
    else
    {
        int pos = 0;

        // find where the existing id currently is
        for (int i = 0; i < 3; i++)
        {
            if (lruBgSub[i] == id)
            {
                pos = i;
                break;
            }
        }

        // shift everything after that position to the left to close the gap
        for (int i = pos; i < 2; i++)
        {
            lruBgSub[i] = lruBgSub[i + 1];
        }

        // place the updated id at the very end (most recently used)
        lruBgSub[2] = id;
    }
}

void UISystem::registerScreen(UIScreen* screen)
{
    sassert(screen != nullptr, "UIScreen cannot be nullptr.");

    // load screen
    if (screen->isMain && screenMainCount < 2)
    {
        loadedMain[screenMainCount] = screen;
        screen->bgId = hwBgMain[screenMainCount];
        screenMainCount++;
        screen->oam = oamMain;
        screen->load();
        screen->isLoaded = true;
        return;
    }
    else if (!screen->isMain && screenSubCount < 3)
    {
        loadedSub[screenSubCount] = screen;
        screen->bgId = hwBgSub[screenSubCount];

        screenSubCount++;
        screen->oam = oamSub;
        screen->load();
        screen->isLoaded = true;
        return;
    }

    // throw error (too many screens registered)
    if (screen->isMain)
    {
        sassert(screenMainCount < 2,
                "Too many screens registered. A maximum of 2 main screens and 3 sub screens can be registered.");
    }
    else
    {
        sassert(screenSubCount < 3,
                "Too many screens registered. A maximum of 2 main screens and 3 sub screens can be registered.");
    }
}

void UISystem::hideAllScreens()
{
    // hide sub screens
    for (UIScreen* val : loadedSub)
    {
        if (val == nullptr)
        {
            continue;
        }
        render.hideBg(val->bgId);
        val->removeSprites();
    }

    // NOTE: do not hide the 3D layer
    // hide main screens
    for (UIScreen* val : loadedMain)
    {
        if (val == nullptr)
        {
            continue;
        }
        render.hideBg(val->bgId);
        val->removeSprites();
    }
}

void UISystem::cleanupScreens()
{
    // reset all bg ids, UIScreens
    // sub
    for (int i = 0; i < 3; i++)
    {
        lruBgSub[i] = hwBgSub[i];
        if (loadedSub[i] == nullptr)
        {
            continue;
        }
        loadedSub[i]->unload();
        loadedSub[i]->isLoaded = false;
        loadedSub[i]->oam = nullptr;
        loadedSub[i]->bgId = -1;
        loadedSub[i] = nullptr;
    }

    // main
    for (int j = 0; j < 2; j++)
    {
        lruBgMain[j] = hwBgMain[j];
        if (loadedMain[j] == nullptr)
        {
            continue;
        }
        loadedMain[j]->unload();
        loadedMain[j]->isLoaded = false;
        loadedMain[j]->oam = nullptr;
        loadedMain[j]->bgId = -1;
        loadedMain[j] = nullptr;
    }

    screenMainCount = 0;
    screenSubCount = 0;
}

void UISystem::cancelSFX()
{
    musicCtrl->stopSFX(sfxMenuHandle);
    musicCtrl->stopSFX(sfxSelectHandle);
    musicCtrl->stopSFX(sfxCancelHandle);
    sfxMenuHandle = 0;
    sfxSelectHandle = 0;
    sfxCancelHandle = 0;
}

void UISystem::resetUIResources()
{
    cancelSFX();
    cleanupScreens();
    cleanupMenus();

    isActive = false;
    renderUIText = false;
}

void UISystem::cleanupMenus()
{
    if (activeMenu != nullptr)
    {
        activeMenu->resetMenu();
        activeMenu = nullptr;
    }

    for (UIMenu* menu : menus)
    {
        if (menu != nullptr)
        {
            menu->text = nullptr;
        }
    }

    menus = {};
    text = nullptr;
}
