#include "PaulowniaMallView.hpp"
#include "core/globals.hpp"
#include "events/UIEvents.hpp"

PaulowniaMallView::PaulowniaMallView()
{
}

void PaulowniaMallView::setupMusic()
{
    musicCtrl->init((fatBasePath + "music/locations/paulowniaMall/overworld/color_your_night.pcm").c_str(),
                    ae::q20_12_t{2.05},
                    ae::q20_12_t{204.191});
}

void PaulowniaMallView::setupCamera()
{
    camConfig.mode = CameraMode::Follow;
    camConfig.initialAngle = ae::q20_12_t{1.5708 * 2};
    camConfig.distance = ae::q20_12_t{1};
    camConfig.height = height + ae::q20_12_t{0.4};
    camConfig.lookAhead = ae::q20_12_t{0.2};
    camConfig.angleIncrement = ae::q20_12_t{0.05};
    camConfig.isRotationLocked = true;
}

void PaulowniaMallView::setupMovement()
{
    movement->configureMovement(MovementConfig(PAULOWNIA_MALL_MAP_WIDTH,
                                               PAULOWNIA_MALL_MAP_HEIGHT,
                                               &paulownia_mall_map[0][0],
                                               tileSize,
                                               dbEntry->worldOffsetX,
                                               dbEntry->worldOffsetZ,
                                               characterSize,
                                               speed,
                                               height,
                                               characterTranslate,
                                               characterFacingAngle));
}

ViewState PaulowniaMallView::onTileCheck(TileType tile, u32 pressed)
{
    switch (tile)
    {
    // left
    case TileType::SCENE_0:
    {
        return ViewState::IWATODAI_STREETS;
    }

    // right
    case TileType::SCENE_1:
    {
        return ViewState::IWATODAI_DORM;
    }

    // middle
    case TileType::SCENE_2:
    case TileType::SCENE_3:
    case TileType::SCENE_4:
    case TileType::SCENE_5:
    case TileType::SCENE_6:
    case TileType::SCENE_7:
    case TileType::SCENE_8:
    case TileType::SCENE_9:
    {
        return ViewState::STATION;
    }
    default:
    {
        break;
    }
    }

    return ViewState::KEEP_CURRENT;
}

void PaulowniaMallView::setupText()
{
    text->configureText(TextConfig(textVideoBuffer, &fontName, fontSize));
    textSub->configureText(TextConfig(textVideoBufferSub, &fontName, fontSize));
}

void PaulowniaMallView::setupUI()
{
    textSub->configureText(TextConfig(textVideoBufferSub, &fontName, fontSize));

    pauseMenuCmpt = PauseMenu::getInstance();

    menuHUDScreen = MenuHUDScreen::getInstance();

    std::array<UIScreen*, 5> screens = {menuHUDScreen};
    std::array<UIMenu*, 10> menus = {pauseMenuCmpt};

    ae::BroadcastEvent(Event::ConfigureUIScreen{bgSub, bgMain, &oamSub, &oamMain, screens});
    ae::BroadcastEvent(Event::ConfigureUIMenu{textSub, menus});
}
