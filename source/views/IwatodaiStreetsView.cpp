#include "IwatodaiStreetsView.hpp"
#include "core/globals.hpp"
#include "events/BattleEvents.hpp"
#include "events/GenericEvents.hpp"
#include "events/UIEvents.hpp"

IwatodaiStreetsView::IwatodaiStreetsView()
{
    // Battle setup
    characterProfiles.push_back(CharacterProfileDb::junpei);
    characterProfiles.push_back(CharacterProfileDb::yukari);

    enemyProfiles.push_back(EnemyProfileDb::cowardlyMaya);
    enemyProfiles.push_back(EnemyProfileDb::mercilessMaya);
}

IwatodaiStreetsView::~IwatodaiStreetsView()
{
    enemyProfiles.clear();
    characterProfiles.clear();
}

void IwatodaiStreetsView::startBattle()
{
    // start battle
    Event::ExecuteBattle msg(CharacterProfileDb::player, characterProfiles, enemyProfiles, battleStartCondition);
    ae::BroadcastEvent(msg);
}

void IwatodaiStreetsView::setupCamera()
{
    camConfig.mode = CameraMode::Follow;
    camConfig.initialAngle = ae::q20_12_t{1.5708 * 2};
    camConfig.distance = ae::q20_12_t{1};
    camConfig.height = height + ae::q20_12_t{0.6};
    camConfig.lookAhead = ae::q20_12_t{0.2};
    camConfig.angleIncrement = ae::q20_12_t{0.05};
    camConfig.isRotationLocked = true;
}

void IwatodaiStreetsView::setupMovement()
{
    movement->configureMovement(MovementConfig(IWATODAI_STREETS_MAP_WIDTH,
                                               IWATODAI_STREETS_MAP_HEIGHT,
                                               &iwatodai_streets_map[0][0],
                                               tileSize,
                                               dbEntry->worldOffsetX,
                                               dbEntry->worldOffsetZ,
                                               characterSize,
                                               speed,
                                               height,
                                               characterTranslate,
                                               characterFacingAngle));
}

void IwatodaiStreetsView::setupMusic()
{
    musicCtrl->init((fatBasePath + "music/locations/iwatodaiStreets/changing_seasons.pcm").c_str(),
                    ae::q20_12_t{31},
                    ae::q20_12_t{177.587});
}

ViewState IwatodaiStreetsView::onTileCheck(TileType tile, u32 pressed)
{
    switch (tile)
    {
    case TileType::SCENE_0:
    {
        musicCtrl->pause();
        return ViewState::IWATODAI_DORM;
    }

    case TileType::SCENE_1:
    {
        musicCtrl->pause();
        return ViewState::PAULOWNIA_MALL;
    }

    case TileType::SCENE_2:
    {
        musicCtrl->pause();
        return ViewState::STATION;
    }

    case TileType::SHD_W:
    {
        if (!promptDrawn)
        {
            textSub->drawText("\xFF\x02\x01 Battle Zone", 0, 0, TextColor::Black);
            promptDrawn = true;
        }
        if (pressed & KEY_A)
        {
            phase = ViewPhase::BATTLE;
            prevEnvironmentState = false;
        }

        break;
    }

    default:
    {
        if (promptDrawn)
        {
            textSub->clearScreen();
            promptDrawn = false;
        }
        break;
    }
    }

    return ViewState::KEEP_CURRENT;
}

void IwatodaiStreetsView::setupText()
{
    text->configureText(TextConfig(textVideoBuffer, &fontName, fontSize));
    textSub->configureText(TextConfig(textVideoBufferSub, &fontName, fontSize));
}

void IwatodaiStreetsView::setupUI()
{
    textSub->configureText(TextConfig(textVideoBufferSub, &fontName, fontSize));

    battleMenuCmpt = BattleMenu::getInstance();
    pauseMenuCmpt = PauseMenu::getInstance();

    menuHUDScreen = MenuHUDScreen::getInstance();

    std::array<UIScreen*, 5> screens = {menuHUDScreen};
    std::array<UIMenu*, 10> menus = {pauseMenuCmpt, battleMenuCmpt};

    ae::BroadcastEvent(Event::ConfigureUIScreen{bgSub, bgMain, &oamSub, &oamMain, screens});
    ae::BroadcastEvent(Event::ConfigureUIMenu{textSub, menus});
}
