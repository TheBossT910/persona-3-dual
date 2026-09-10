#pragma once
#include "components/menus/UIMenu.hpp"
#include "controllers/AnimationController.hpp"
#include "managers/RenderManager.hpp"
#include "systems/CameraSystem.hpp"
#include <etl/array.h>

enum class DebugOption
{
    DISCLAIMER_VIEW = 0,
    INTRO_VIEW,
    MAIN_MENU_VIEW,
    IWATODAI_DORM_VIEW,
    IWATODAI_STREETS_VIEW,
    STATION_VIEW,
    SIGN_CONTRACT_VIEW,
    PAULOWNIA_MALL_VIEW,
    INTRO_VIDEO,
    CUTSCENE_1,
    CUTSCENE_2,
    TOGGLE_BILLBOARDS,
    TOGGLE_DEBUG_PRINT,
    PLAY_CHARACTER_ANIM,
    CYCLE_CAMERA_MODE
};

class PauseMenu : public UIMenu
{
  private:
    PauseMenu() {};
    virtual ~PauseMenu() = default;
    static PauseMenu* instance;

    CameraSystem& cameraSystem = CameraSystem::GetInstance();
    RenderManager& rm = RenderManager::GetInstance();

    etl::array<CameraMode, 4> cameraModes = {
        CameraMode::Free, CameraMode::Static, CameraMode::CCTV, CameraMode::Follow};

    etl::array<MenuOption, 8> menuOptions = {
        MenuOption{"Debug", -1, MENU_BIND(PauseMenu, openDebugMenu)},
        {"Skill", -1, MENU_BIND(PauseMenu, openSkillMenu)},
        {"Item", -1, MENU_BIND(PauseMenu, openItemMenu)},
        {"Persona", -1, MENU_BIND(PauseMenu, openPersonaMenu)},
        {"Equip", -1, MENU_BIND(PauseMenu, openEquipMenu)},
        {"Status", -1, MENU_BIND(PauseMenu, openStatusMenu)},
        {"S.Link", -1, MENU_BIND(PauseMenu, openSLinkMenu)},
        {"System", -1, MENU_BIND(PauseMenu, openSystemMenu)},
    };

    etl::array<MenuOption, 15> debugOptions = {
        MenuOption{"DisclaimerView", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"IntroView", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"MainMenuView", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"IwatodaiDormView", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"IwatodaiStreetView", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"StationView", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"SignContractView", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"PaulowniaMallView", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"IntroVideo", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"Cutscene1", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"Cutscene2", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"Toggle Billboards", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"Toggle Debug Print", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
        {"Play Character Animations", -1, MENU_BIND(PauseMenu, openCharacterAnimMenu)},
        {"Cycle Camera Mode", -1, MENU_BIND(PauseMenu, debugOptionSelected)},
    };

    etl::array<MenuOption, 9> skillOptions = {
        MenuOption{"Makoto", 0, MENU_BIND(PauseMenu, skillOptionSelected)},
        {"Yukari", 1, MENU_BIND(PauseMenu, skillOptionSelected)},
        {"Junpei", 3, MENU_BIND(PauseMenu, skillOptionSelected)},
        {"Akihiko", 2, MENU_BIND(PauseMenu, skillOptionSelected)},
        {"Mitsuru", -1, MENU_BIND(PauseMenu, skillOptionSelected)},
        {"Aigis", -1, MENU_BIND(PauseMenu, skillOptionSelected)},
        {"Ken", -1, MENU_BIND(PauseMenu, skillOptionSelected)},
        {"Koromaru", -1, MENU_BIND(PauseMenu, skillOptionSelected)},
        {"Shinjiro", -1, MENU_BIND(PauseMenu, skillOptionSelected)},
    };

    etl::array<MenuOption, 3> itemOptions = {
        MenuOption{"Life Stone", -1, MENU_BIND(PauseMenu, itemOptionSelected)},
        {"Medicine", -1, MENU_BIND(PauseMenu, itemOptionSelected)},
        {"Bead", -1, MENU_BIND(PauseMenu, itemOptionSelected)},
    };

    etl::array<MenuOption, 9> equipOptions = {
        MenuOption{"Makoto", -1, MENU_BIND(PauseMenu, equipOptionSelected)},
        {"Yukari", -1, MENU_BIND(PauseMenu, equipOptionSelected)},
        {"Junpei", -1, MENU_BIND(PauseMenu, equipOptionSelected)},
        {"Akihiko", -1, MENU_BIND(PauseMenu, equipOptionSelected)},
        {"Mitsuru", -1, MENU_BIND(PauseMenu, equipOptionSelected)},
        {"Aigis", -1, MENU_BIND(PauseMenu, equipOptionSelected)},
        {"Ken", -1, MENU_BIND(PauseMenu, equipOptionSelected)},
        {"Koromaru", -1, MENU_BIND(PauseMenu, equipOptionSelected)},
        {"Shinjiro", -1, MENU_BIND(PauseMenu, equipOptionSelected)},
    };

    etl::array<MenuOption, 3> personaOptions = {
        MenuOption{"Jack Frost", -1, MENU_BIND(PauseMenu, personaOptionSelected)},
        {"Black Frost", -1, MENU_BIND(PauseMenu, personaOptionSelected)},
        {"King Frost", -1, MENU_BIND(PauseMenu, personaOptionSelected)},
    };

    etl::array<MenuOption, 9> statsOptions = {
        MenuOption{"Makoto", -1, MENU_BIND(PauseMenu, statsOptionSelected)},
        {"Yukari", -1, MENU_BIND(PauseMenu, statsOptionSelected)},
        {"Junpei", -1, MENU_BIND(PauseMenu, statsOptionSelected)},
        {"Akihiko", -1, MENU_BIND(PauseMenu, statsOptionSelected)},
        {"Mitsuru", -1, MENU_BIND(PauseMenu, statsOptionSelected)},
        {"Aigis", -1, MENU_BIND(PauseMenu, statsOptionSelected)},
        {"Ken", -1, MENU_BIND(PauseMenu, statsOptionSelected)},
        {"Koromaru", -1, MENU_BIND(PauseMenu, statsOptionSelected)},
        {"Shinjiro", -1, MENU_BIND(PauseMenu, statsOptionSelected)},
    };

    etl::array<MenuOption, 3> sLinkOptions = {
        MenuOption{"Fool", -1, MENU_BIND(PauseMenu, sLinkOptionSelected)},
        {"Magician", -1, MENU_BIND(PauseMenu, sLinkOptionSelected)},
        {"Emperor", -1, MENU_BIND(PauseMenu, sLinkOptionSelected)},
    };

    etl::array<MenuOption, 6> systemOptions = {
        MenuOption{"Tutorial", -1, MENU_BIND(PauseMenu, systemOptionSelected)},
        {"Config", -1, MENU_BIND(PauseMenu, systemOptionSelected)},
        {"Dictionary", -1, MENU_BIND(PauseMenu, systemOptionSelected)},
        {"Load Data", -1, MENU_BIND(PauseMenu, systemOptionSelected)},
        {"Save Data", -1, MENU_BIND(PauseMenu, systemOptionSelected)},
        {"Return to Title", -1, MENU_BIND(PauseMenu, systemOptionSelected)},
    };

    etl::array<MenuOption, 2> skills = {
        MenuOption{"Skill 1", -1, nullptr},
        {"Skill 2", -1, nullptr},
    };

    etl::array<MenuOption, 25> characterAnimOptions = {
        MenuOption{"Toggle Auto Animations", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0000", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0001", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0002", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0003", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0004", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0005", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0006", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0007", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0008", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0009", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0010", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0011", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0012", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0013", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0014", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0015", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0016", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0017", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0018", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0019", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0020", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0021", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"0022", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
        {"root", -1, MENU_BIND(PauseMenu, characterAnimOptionSelected)},
    };

    // menu navigation handlers
    ViewState openDebugMenu();
    ViewState openSkillMenu();
    ViewState openItemMenu();
    ViewState openPersonaMenu();
    ViewState openEquipMenu();
    ViewState openStatusMenu();
    ViewState openSLinkMenu();
    ViewState openSystemMenu();
    ViewState openCharacterAnimMenu();

    // selection handlers
    ViewState debugOptionSelected();
    ViewState skillOptionSelected();
    ViewState itemOptionSelected();
    ViewState equipOptionSelected();
    ViewState personaOptionSelected();
    ViewState statsOptionSelected();
    ViewState sLinkOptionSelected();
    ViewState systemOptionSelected();
    ViewState characterAnimOptionSelected();

    AnimationController* animationCtrl = AnimationController::getInstance();

    void resetHook() override;
    void closeHook() override;

  public:
    static void create();
    static void destroy();
    static PauseMenu* getInstance();

    ViewState updateHook() override;

    bool isClosed = false;
};
