#include "StationView.hpp"
#include "core/globals.hpp"
#include "events/UIEvents.hpp"

StationView::StationView()
{
}

void StationView::setupMusic()
{
    musicCtrl->init((fatBasePath + "music/locations/paulowniaMall/station/paulownia_mall.pcm").c_str(),
                    ae::q20_12_t{2.002},
                    ae::q20_12_t{73.93});
}

void StationView::setupCamera()
{
    camConfig.mode = CameraMode::Follow;
    camConfig.initialAngle = ae::q20_12_t{1.5708f} * ae::q20_12_t{2};
    camConfig.distance = ae::q20_12_t{1.0};
    camConfig.height = height + ae::q20_12_t{0.6};
    camConfig.lookAhead = ae::q20_12_t{0.2};
    camConfig.angleIncrement = ae::q20_12_t{0.05};
    camConfig.isRotationLocked = true;
}

void StationView::setupMovement()
{
    movement->configureMovement(MovementConfig(STATION_MAP_WIDTH,
                                               STATION_MAP_HEIGHT,
                                               &station_map[0][0],
                                               tileSize,
                                               dbEntry->worldOffsetX,
                                               dbEntry->worldOffsetZ,
                                               characterSize,
                                               speed,
                                               height,
                                               characterTranslate,
                                               characterFacingAngle));
}

ViewState StationView::onTileCheck(TileType tile, u32 pressed)
{
    switch (tile)
    {
    case TileType::SCENE_0:
    {
        return ViewState::PAULOWNIA_MALL;
    }
    default:
    {
        break;
    }
    }

    return ViewState::KEEP_CURRENT;
}

void StationView::setupText()
{
    text->configureText(TextConfig(textVideoBuffer, &fontName, fontSize));
    textSub->configureText(TextConfig(textVideoBufferSub, &fontName, fontSize));
}

void StationView::setupUI()
{
    textSub->configureText(TextConfig(textVideoBufferSub, &fontName, fontSize));

    pauseMenuCmpt = PauseMenu::getInstance();

    menuHUDScreen = MenuHUDScreen::getInstance();

    std::array<UIScreen*, 5> screens = {menuHUDScreen};
    std::array<UIMenu*, 10> menus = {pauseMenuCmpt};

    ae::BroadcastEvent(Event::ConfigureUIScreen{bgSub, bgMain, &oamSub, &oamMain, screens});
    ae::BroadcastEvent(Event::ConfigureUIMenu{textSub, menus});
}
