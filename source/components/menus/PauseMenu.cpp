#include "PauseMenu.hpp"
#include "core/globals.hpp"
#include <nds.h>
#include <string>

// sfx
#include "soundbank.h"
// dialogue
#include "demo/demo_dialogue.hpp"

PauseMenu* PauseMenu::instance = nullptr;

void PauseMenu::create()
{
    if (instance == nullptr)
    {
        instance = new PauseMenu();
    }
}

void PauseMenu::destroy()
{
    if (instance != nullptr)
    {
        instance->cleanup();
        delete instance;
    }
    instance = nullptr;
}

PauseMenu* PauseMenu::getInstance()
{
    if (instance == nullptr)
    {
        create();
    }
    return instance;
}

void PauseMenu::cleanup()
{
    if (pauseMenu != nullptr)
    {
        engine.DestroyEntity(pauseMenu);
        pauseMenu = nullptr;
        dialogue = nullptr;
    }
}

void PauseMenu::resetHook()
{
    pauseMessage = "Pause";
    options = menuOptions;
    isClosed = false;

    if (pauseMenu == nullptr)
    {
        pauseMenu = engine.CreateEntity();
        dialogue = engine.CreateComponent<DialogueComponent>();
        pauseMenu->AddComponent(dialogue);
    }
}

ViewState PauseMenu::updateHook()
{
    // dialogue should be started, but has not
    if (isDialogueStarted && !dialogue->IsActive())
    {
        dialogue->configureDialogue(DialogueConfig(text));
        dialogue->start(demo_dialogue_init());

        isDialogueStarted = false;
        isDialoguePrevActive = false;
    }

    // dialogue controller takes full control when active
    if (dialogue->IsActive())
    {
        isDialoguePrevActive = true;
        return ViewState::KEEP_CURRENT;
    }

    if (!dialogue->IsActive() && isDialoguePrevActive)
    {
        isDialoguePrevActive = false;
        ae::BroadcastEvent(Event::RenderUIText{});
    }

    return ViewState::DEFAULT;
}

void PauseMenu::closeHook()
{
    resetMenu();

    // set flag
    isClosed = true;
}

// menu navigation handlers

ViewState PauseMenu::openDebugMenu()
{
    switch (cameraSystem.getMode())
    {
    case CameraMode::Follow:
        debugOptions[static_cast<int>(DebugOption::CYCLE_CAMERA_MODE)].name = "Camera: Follow";
        break;
    case CameraMode::Static:
        debugOptions[static_cast<int>(DebugOption::CYCLE_CAMERA_MODE)].name = "Camera: Static";
        break;
    case CameraMode::CCTV:
        debugOptions[static_cast<int>(DebugOption::CYCLE_CAMERA_MODE)].name = "Camera: CCTV";
        break;
    case CameraMode::Free:
        debugOptions[static_cast<int>(DebugOption::CYCLE_CAMERA_MODE)].name = "Camera: Free";
        break;
    case CameraMode::Path:
        debugOptions[static_cast<int>(DebugOption::CYCLE_CAMERA_MODE)].name = "Camera: Path";
        break;
    default:
        debugOptions[static_cast<int>(DebugOption::CYCLE_CAMERA_MODE)].name = "Camera: ?";
        break;
    }
    return changeMenu(debugOptions);
}

ViewState PauseMenu::openSkillMenu()
{
    return changeMenu(skillOptions);
}

ViewState PauseMenu::openItemMenu()
{
    return changeMenu(itemOptions);
}

ViewState PauseMenu::openPersonaMenu()
{
    return changeMenu(personaOptions);
}

ViewState PauseMenu::openEquipMenu()
{
    return changeMenu(equipOptions);
}

ViewState PauseMenu::openStatusMenu()
{
    return changeMenu(statsOptions);
}

ViewState PauseMenu::openSLinkMenu()
{
    return changeMenu(sLinkOptions);
}

ViewState PauseMenu::openSystemMenu()
{
    return changeMenu(systemOptions);
}

ViewState PauseMenu::openCharacterAnimMenu()
{
    return changeMenu(characterAnimOptions);
}

// selection handlers

ViewState PauseMenu::skillOptionSelected()
{
    return changeMenu(skills);
}

ViewState PauseMenu::itemOptionSelected()
{
    return ViewState::KEEP_CURRENT;
}

ViewState PauseMenu::equipOptionSelected()
{
    return ViewState::KEEP_CURRENT;
}

ViewState PauseMenu::personaOptionSelected()
{
    return ViewState::KEEP_CURRENT;
}

ViewState PauseMenu::statsOptionSelected()
{
    return ViewState::KEEP_CURRENT;
}

ViewState PauseMenu::sLinkOptionSelected()
{
    return ViewState::KEEP_CURRENT;
}

ViewState PauseMenu::systemOptionSelected()
{
    return ViewState::KEEP_CURRENT;
}

ViewState PauseMenu::debugOptionSelected()
{
    ViewState selectedView;
    switch (static_cast<DebugOption>(selectedOption))
    {
    case DebugOption::DISCLAIMER_VIEW:
        selectedView = ViewState::DISCLAIMER;
        break;
    case DebugOption::INTRO_VIEW:
        selectedView = ViewState::INTRO;
        break;
    case DebugOption::MAIN_MENU_VIEW:
        selectedView = ViewState::MAIN_MENU;
        break;
    case DebugOption::IWATODAI_DORM_VIEW:
        selectedView = ViewState::IWATODAI_DORM;
        break;
    case DebugOption::IWATODAI_STREETS_VIEW:
        selectedView = ViewState::IWATODAI_STREETS;
        break;
    case DebugOption::STATION_VIEW:
        selectedView = ViewState::STATION;
        break;
    case DebugOption::SIGN_CONTRACT_VIEW:
        selectedView = ViewState::SIGN_CONTRACT;
        break;
    case DebugOption::PAULOWNIA_MALL_VIEW:
        selectedView = ViewState::PAULOWNIA_MALL;
        break;
    case DebugOption::INTRO_VIDEO:
        selectedView = ViewState::INTRO_VIDEO;
        break;
    case DebugOption::CUTSCENE_1:
        selectedView = ViewState::CUTSCENE_1;
        break;
    case DebugOption::CUTSCENE_2:
        selectedView = ViewState::CUTSCENE_2;
        break;
    case DebugOption::DEBUG_DIALOGUE:
        isDialogueStarted = true;
        selectedView = ViewState::KEEP_CURRENT;
        break;
    case DebugOption::TOGGLE_BILLBOARDS:
        Globals::enableBillboards = !Globals::enableBillboards;
        isActive = false;
        selectedView = ViewState::KEEP_CURRENT;
        break;
    case DebugOption::TOGGLE_DEBUG_PRINT:
        Globals::enableDebugPrint = !Globals::enableDebugPrint;
        isActive = false;
        selectedView = ViewState::KEEP_CURRENT;
        break;
    case DebugOption::CYCLE_CAMERA_MODE:
        if (cameraSystem.getMode() == CameraMode::Path)
        {
            ae::BroadcastEvent(Event::SetCameraMode{CameraMode::Follow});
        }
        else
        {
            CameraMode mode = cameraModes[(static_cast<int>(cameraSystem.getMode()) + 1) % cameraModes.size()];
            ae::BroadcastEvent(Event::SetCameraMode{mode});
        }
        isActive = false;
        openDebugMenu();
        selectedView = ViewState::KEEP_CURRENT;
        break;
    default:
        selectedView = ViewState::KEEP_CURRENT;
    }
    return selectedView;
}

ViewState PauseMenu::characterAnimOptionSelected()
{
    animationCtrl->stop();

    ViewState selectedView = ViewState::KEEP_CURRENT;
    switch (static_cast<CharacterAnimOption>(selectedOption))
    {
    case CharacterAnimOption::TOGGLE_AUTO_ANIM:
        Globals::enableCharacterAnim = !Globals::enableCharacterAnim;
        break;
    case CharacterAnimOption::ANIM_1:
        animationCtrl->set(0, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_2:
        animationCtrl->set(1, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_3:
        animationCtrl->set(2, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_4:
        animationCtrl->set(3, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_5:
        animationCtrl->set(4, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_6:
        animationCtrl->set(5, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_7:
        animationCtrl->set(6, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_8:
        animationCtrl->set(7, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_9:
        animationCtrl->set(8, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_10:
        animationCtrl->set(9, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_11:
        animationCtrl->set(10, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_12:
        animationCtrl->set(11, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_13:
        animationCtrl->set(12, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_14:
        animationCtrl->set(13, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_15:
        animationCtrl->set(14, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_16:
        animationCtrl->set(15, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_17:
        animationCtrl->set(16, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_18:
        animationCtrl->set(17, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_19:
        animationCtrl->set(18, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_20:
        animationCtrl->set(19, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_21:
        animationCtrl->set(20, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_22:
        animationCtrl->set(21, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_23:
        animationCtrl->set(22, true);
        Globals::enableCharacterAnim = false;
        break;
    case CharacterAnimOption::ANIM_24:
        animationCtrl->set(23, true);
        Globals::enableCharacterAnim = false;
        break;
    default:
        break;
    }

    isActive = false;
    animationCtrl->play();
    return selectedView;
}
