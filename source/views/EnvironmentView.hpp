#pragma once

#include <aegis/ndsTypes.hpp>
#include <aegis/types.hpp>

#include "views/BaseView.hpp"

// environments/data
#include "data/environmentDb.hpp"
#include "environment/Environment.hpp"
// components
#include "components/DialogueComponent.hpp"
#include "components/GraphicsComponent.hpp"
#include "components/MovementComponent.hpp"
#include "components/menus/BattleMenu.hpp"
#include "components/menus/PauseMenu.hpp"
#include "components/screens/DialogueScreen.hpp"
#include "components/screens/MenuHUDScreen.hpp"
// controllers
#include "controllers/AnimationController.hpp"
#include "controllers/MusicController.hpp"
// managers
#include "managers/RenderManager.hpp"
//systems
#include "systems/CameraSystem.hpp"

#include <cstdint>
#include <etl/array.h>
#include <etl/span.h>
#include <string>

enum class ViewPhase
{
    BATTLE,
    PAUSE,
    DIALOGUE,
    ENVIRONMENT
};

class EnvironmentView : public BaseView
{
  public:
    /**
     * @brief One-time setup for a room
     *
     * @note Resolves a room's EnvironmentDbEntry once into
     *       dbEntry. Everything below reads from that member instead of re-deriving it or
     *       relying on a per-room generated type. If no entry can be resolved,
     *       init() logs an error and returns immediately, since nothing below
     *       this point can run without a valid entry (setupEnvironment()
     *       immediately dereferences dbEntry->name).
     */
    void init() override;

    /**
     * @brief Per-frame update for this room's view
     *
     * @note  Advances the current ViewPhase, updates Controllers,
     *        and reports whether a phase transition to a different
     *        ViewState should occur.
     *
     * @return ViewState::KEEP_CURRENT to remain on this view for another
     *         frame, or another ViewState value to signal that the caller
     *         should transition away from this view entirely.
     */
    ViewState update() override;

    /**
     * @brief Tears down everything a room's view had set up
     */
    void cleanup() override;

  protected:
    // Room-specific hooks (implemented/overridden by derived rooms)
    virtual ae::q20_12_t getCameraYOffset() const
    {
        return ae::q20_12_t{0.1};
    } // default

    virtual const EnvironmentDbEntry* getEnvironmentDbEntry() = 0;

    virtual void setupText() = 0;

    virtual void setupMusic() = 0;

    virtual void setupMovement() = 0;

    virtual void setupUI()
    {
    }

    virtual void setupDialogue()
    {
    }

    // TODO: enforce?
    virtual void setupCamera()
    {
    }

    virtual void cleanupHook()
    {
    }

    virtual ViewState onTileCheck(TileType tile, uint32_t pressed) = 0;

    // -------------------------------------------------
    // Battle hooks
    virtual void startBattle()
    {
    }

    virtual void onBattleStart()
    {
    }

    // -------------------------------------------------
    // Shared state
    touchPosition touch;

    int8_t bgSharedSub1 = -1;
    int8_t bgSharedSub2 = -1;
    int8_t bgSharedSub3 = -1;

    ViewPhase phase = ViewPhase::ENVIRONMENT;

    bool prevPauseState = false;
    bool prevDialogueState = false;
    bool prevEnvironmentState = false;
    bool prevBattleState = false;
    bool isBattleMenuActive = false;
    bool promptDrawn = false;

    Event::CameraPosition camPos;
    const ae::q20_12_t tileSize{0.062500};

    // Override fields in setCameraConfig() — same struct for all modes
    Event::ConfigureCamera camConfig;

    // -------------------------------------------------
    // Player
    MovementComponent* movement = nullptr;
    // TODO: move dialogue, text component to actual actors!
    // In this case, it would be the Akihiko billboard
    DialogueComponent* dialogue = nullptr;

    // -------------------------------------------------
    // View / entity
    ae::Entity* environment = nullptr;
    GraphicsComponent* graphics = nullptr;
    TextComponent* text = nullptr;
    TextComponent* textSub = nullptr;
    TextComponent* textSubAlt = nullptr;

    AnimationController* animationCtrl = AnimationController::getInstance();
    MusicController* musicCtrl = MusicController::getInstance();

    // -------------------------------------------------
    // UI
    DialogueScreen* dialogueScreen = nullptr;
    MenuHUDScreen* menuHUDScreen = nullptr;

    BattleMenu* battleMenuCmpt = nullptr;
    PauseMenu* pauseMenuCmpt = nullptr;

    std::array<int, 2> bgMain;
    std::array<int, 3> bgSub;

    // -------------------------------------------------
    // Environment
    Environment env;
    const EnvironmentDbEntry* dbEntry = nullptr;

    // -------------------------------------------------
    // Text
    uint16_t* textVideoBuffer = nullptr;
    uint16_t* textVideoBufferSub = nullptr;
    std::string fontName = "cosmetica";
    uint8_t fontSize = 12;
    // set in init()
    uint8_t lineSpacing = 0;

    // -------------------------------------------------
    // Dialogue
    Dialogue* dialogueFirstLine = nullptr;

  private:
    // -------------------------------------------------
    // Fog properties
    uint8_t shift = 1;
    // how thick (translucent) the fog is
    uint8_t mass = 1;
    // how far the fog is (0x0000 to 0x8000)
    uint16_t depth = 0x6000;

    /**
     * @brief Loads a single .grit asset and returns its raw tile pointer.
     *
     * Stashes the owning GraphicAsset in @p asset so the caller can unload it
     * once the texture has been uploaded to VRAM.
     *
     * @param path  Full path (base path + grit base name) of the asset to load.
     * @param asset Output parameter that receives the loaded GraphicAsset,
     *              which the caller is responsible for unloading later.
     * @return Raw pointer to the asset's tile data, reinterpreted as
     *         unsigned int, suitable for passing to the texture upload code.
     */
    const unsigned int* loadBitmap(const std::string& path, GraphicAsset& asset);

    /**
     * @brief Loads and uploads a room's environment geometry and textures,
     *        driven entirely by dbEntry
     *
     * @note  No per-room texture-slot code and no per-room generated class needed.
     *
     * Loads each texture slot's texture assets to build display lists and upload
     * textures to VRAM, then unloads the texture assets. Logs a message if environment
     * loading fails, since a failed load otherwise leaves environments silently
     * rendering nothing.
     */
    void setupEnvironment();

    /**
     * Loads a model's bitmap textures into memory & passes them to AnimationController
     */
    void setupModel();
};
