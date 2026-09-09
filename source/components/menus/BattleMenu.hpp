#pragma once

#include "battleActions/BattleParticipant.hpp"
#include "battleActions/actions/ActionBase.hpp"
#include "battleActions/personas/PersonaBase.hpp"

#include "components/menus/UIMenu.hpp"
#include "types/UITypes.hpp"

#include <etl/vector.h>

enum class BattleMenuOptions
{
    NONE = 0,
    ACTION,
    SKILL,
    PERSONA,
    TARGET_ENEMY,
    TARGET_HEAL,
    ALL_OUT_ATTACK,
    ALERT
};

class BattleMenu : public UIMenu
{
  private:
    friend class BattleSystem;
    BattleMenu() {};
    virtual ~BattleMenu() = default;
    static BattleMenu* instance;

    BattleMenuOptions loadedOption = BattleMenuOptions::NONE;
    int selectedBattleOption = -1;
    bool isCancelled = false;

    etl::vector<MenuOption, 10> battleOptions;
    int alertStartFrame = 0;
    bool messagePrinted = false;

    void resetHook() override;
    void closeHook() override
    {
    }

    // option handlers
    ViewState battleOptionSelected();

  public:
    static void create();
    static void destroy();
    static BattleMenu* getInstance();

    ViewState updateHook() override;
    void prevOption() override;

  protected:
    // helpers
    void resetLoadedOptions();
    int consumeSelectedBattleOption();
    bool consumeCancel();

    // option loaders
    void loadActionOptions(std::array<ActionBase*, 4>* actions, std::string name);
    void loadSkillOptions(PersonaBase* persona);
    void loadPersonaOptions(etl::vector<PersonaBase*, 13>* personas);
    void loadTargetOptions(etl::vector<BattleParticipant*, 13>* targets, bool healTarget);
    void loadAllOutAttackConfirmation();
    void loadAlertOptions(const std::string& text);
    bool isAlertExpired(int durationFrames) const;
};
