#pragma once
#include "components/GraphicsComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/menus/MainMenu.hpp"
#include "controllers/MusicController.hpp"
#include "managers/MathManager.hpp"
#include "managers/RenderManager.hpp"
#include "views/BaseView.hpp"

class MainMenuView : public BaseView
{
  private:
    MainMenu* mainMenuCmpt = nullptr;
    int bg[3];

    // for silhouette animation
    int silhouetteX = -256;
    int silhouetteY = 192;
    bool isSilhouetteStillMoving = true;

    // for bottom screen text animation
    int brightness = 0;

    // for fogBackground
    bool displayFog = false;
    int fogOpacity = 0;
    // NOTE: we use u16 to allow overflow (and naturally reset values back to 0)
    u16 waveAngle = 0;
    u16 currentRotation = 0;
    int baseSpeed = 20;
    int fluctuation = 50;

    // text
    std::string FONT_NAME = "cosmetica";
    int FONT_SIZE = 12;

    ae::Entity* mainMenu = nullptr;
    GraphicsComponent* graphics = nullptr;
    TextComponent* textMenu = nullptr;

    MathManager& math = MathManager::GetInstance();
    RenderManager& render = RenderManager::GetInstance();
    MusicController* musicCtrl = MusicController::getInstance();

  public:
    void init() override;
    ViewState update() override;
    void cleanup() override;
};
