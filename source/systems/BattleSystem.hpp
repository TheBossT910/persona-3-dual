/**
 * @file BattleSystem.hpp
 * @brief System for managing battle logic & states
 * @author Nolan Kolb (TrueGiles / themoonwalker8692)
 */

#pragma once

#include "components/TextComponent.hpp"
#include "core/routerIDs.hpp"
#include "events/BattleEvents.hpp"
#include "events/GenericEvents.hpp"
#include <aegis/system.hpp>

#include "components/menus/BattleMenu.hpp"
#include "controllers/MusicController.hpp"

#include <algorithm>
#include <array>
#include <etl/vector.h>
#include <nds.h>
#include <string>

#include "battleActions/actions/AttackAction.hpp"
#include "battleActions/actions/Guard.hpp"
#include "battleActions/actions/PersonaAction.hpp"
#include "battleActions/actions/SwitchPersona.hpp"

#include "battleActions/BattleParticipant.hpp"
#include "battleActions/BattlePhase.hpp"
#include "battleActions/BattleStartCondition.hpp"
#include "battleActions/TurnResult.hpp"
#include "battleActions/enemies/Enemy.hpp"
#include "battleActions/enemies/EnemyProfileDb.hpp"
#include "battleActions/party/CharacterProfileDb.hpp"
#include "battleActions/party/PartyMember.hpp"
#include "battleActions/party/Player.hpp"

// TODO: check for dead code/unfeasible paths
class BattleSystem : public ae::SystemRouter<BattleSystem, Event::ExecuteBattle>, public ae::Singleton<BattleSystem>
{
  public:
    void Init() override;

    void Shutdown() override;

    /**
     * @brief Core update loop that processes the battle state machine and turn resolution.
     *
     * @details Executes every engine tick to advance the current BattlePhase. This state
     * machine governs both the user interface flow (via battleMenuCmpt) and the underlying
     * combat mechanics. Key responsibilities include:
     *
     * - Menu Navigation: Handling hardware input for base actions, skill selection, and
     *   target filtering (e.g., preventing healing on max-HP targets, skipping dead entities).
     * - Persona Mechanics: Managing mid-turn Persona switching and validating SP/HP
     *   resource costs before skill execution.
     * - Combat Flow: Resolving attacks, evaluating "1 More" states to trigger All-Out
     *   Attack confirmations, and routing enemy AI behavior.
     * - UI Alerts: Temporarily overriding the menu to display asynchronous combat
     *   messages using the `pendingAlert` buffer during the `ShowAlert` phase.
     *
     * @param dt Fixed-point delta time passed from the aegis engine loop (currently unused).
     */
    void Update(ae::q20_12_t /*dt*/) override;

    /**
     * @brief ETL message handler that initializes and starts a new battle.
     *
     * @details Acts as the initialization routine for the BattleSystem when an
     * ExecuteBattle event is fired over the message bus.
     *
     * This function dynamically allocates the active combat entities (Player,
     * PartyMember, and Enemy objects) based on the profiles provided in the
     * message payload. It then resets all internal state machine variables, clears
     * any pending UI alerts
     *
     * Finally, it calculates the initial turn order based on the battleStartCondition
     * and advances the state machine to the first participant's initial phase.
     *
     * @param msg The event payload containing the character/enemy profiles and encounter conditions.
     */
    void on_receive(const Event::ExecuteBattle& msg);

    /**
     * @brief Fallback handler for unhandled ETL messages.
     *
     * @details Required by the ETL message router interface. Safely ignores
     * any messages routed to the BattleSystem that do not have a specific handler.
     *
     * @param msg The unhandled incoming message (unused).
     */
    void on_receive_unknown(const etl::imessage& msg)
    {
    }

  private:
    friend class Singleton<BattleSystem>;
    BattleSystem() : SystemRouter(kBattleSystemRouterID)
    {
    }

    static constexpr u32 ACTION_ATTACK = 0;
    static constexpr u32 ACTION_GUARD = 1;
    static constexpr u32 ACTION_PERSONA = 2;
    static constexpr u32 ACTION_SWITCH = 3;

    u32 turnsTaken = 0;

    BattlePhase phase;
    Event::BattleResult battleResult;

    BattleParticipant* currentParticipantTurn = nullptr;
    u32 currentParticipantIndex = 0;

    int selectedBattleOption = -1;
    Skill* selectedSkill = nullptr;

    bool pendingPersonaSwitch = false;
    bool switchedPersonaThisTurn = false;
    PersonaBase* personaBeforeSwitch = nullptr;

    std::string pendingAlert;
    BattlePhase alertReturnPhase = BattlePhase::Done;

    bool allOutAttackWasPossibleThisKnockDown = false;

    // Current battle data
    etl::vector<BattleParticipant*, 13> battleParticipants;
    etl::vector<Enemy*, 8> enemies;
    etl::vector<PartyMember*, 4> partyMembers;
    Player* player = nullptr;

    BattleStartCondition battleStartCondition = BattleStartCondition::Even;

    // Actions
    AttackAction attack;
    Guard guard;
    PersonaAction persona;
    SwitchPersona switchPersona;

    std::array<ActionBase*, 4> actions = {&attack, &guard, &persona, &switchPersona};

    // Internal helpers
    /**
     * @brief Resolves the mathematical and state changes of a combat action against a target.
     *
     * @details Parses a generated TurnResult struct to apply HP modifications, clamp healing
     * to the target's maximum HP, and buffer damage/healing numbers into pendingAlert for UI
     * feedback. Also checks for and triggers the "1 More" state if the hit warrants it.
     *
     * @param r The mathematical result of the action (damage delta, hit flag, 1-More flag).
     * @param target The participant receiving the action (can be null for non-targeted actions).
     */
    void applyResult(const TurnResult& r, BattleParticipant* target = nullptr);

    /**
     * @brief Progresses the battle state machine to the next logical step.
     *
     * @details Evaluates the board state after an action resolves. Responsibilities include:
     * - Checking for win/loss conditions via handleDeadParticipants().
     * - Intercepting the turn flow to prompt an All-Out Attack if all enemies are knocked down.
     * - Granting an immediate follow-up turn if the current participant earned a "1 More".
     * - Cycling to the next living participant's turn if the current actor is finished,
     *   clearing their knockdown and Persona switch states in the process.
     */
    void advanceTurn();

    /**
     * @brief Safely transitions the state machine to a new BattlePhase.
     *
     * @details Acts as a router to ensure UI alerts are displayed before phase changes.
     * If there is text buffered in pendingAlert, it intercepts the transition, forcing
     * the system into the ShowAlert phase and saving the intended next phase into
     * alertReturnPhase.
     *
     * @param nextPhase The phase the state machine should enter after all alerts are cleared.
     */
    void setNextPhase(BattlePhase nextPhase);

    /**
     * @brief Calculates the initial turn queue at the start of an encounter.
     *
     * @details Applies a random agility boost (1.2x to 1.4x) to all participants to prevent
     * static turn orders, sorts the party and enemies individually, and then merges them
     * based on the BattleStartCondition (forcing Party Advantage or Enemy Advantage).
     *
     * @note TODO: This function currently assumes it is only called once per encounter.
     * Calling it mid-battle could result in a participant taking a double turn.
     */
    void calculateTurnOrder();

    /**
     * @brief Sweeps the field for dead participants and evaluates encounter end conditions.
     *
     * @details Iterates through all combatants to check if HP has reached 0.
     * Triggers the individual onDead() callbacks to populate the battleResult.
     * Immediately halts and shuts down the battle if the main player dies, or if
     * the entire enemy side is wiped out.
     */
    void handleDeadParticipants();

    /**
     * @brief Filters the enemy roster for living combatants.
     *
     * @return A vector containing pointers to all enemies currently above 0 HP.
     */
    etl::vector<BattleParticipant*, 13> getAliveEnemies();

    /**
     * @brief Evaluates if an All-Out Attack condition is met.
     *
     * @return true if every living enemy is currently in the knocked-down state, false otherwise.
     */
    bool allEnemiesKnockedDown();

    /**
     * @brief Determines if a skill targets a single entity.
     *
     * @param type The enumerated type classification of the skill.
     * @return true if the skill is a single-target action (e.g., Attack, single Heal/Buff),
     *         false if it is multi-target.
     */
    bool isSingleTarget(SkillType type);

    /**
     * @brief Sorting predicate used to order combatants by agility.
     *
     * @param a The first participant to compare.
     * @param b The second participant to compare.
     * @return true if participant 'a' should act before participant 'b'.
     */
    static bool getParticipantByHigherAgility(BattleParticipant* a, BattleParticipant* b)
    {
        return a->currentTurnOrderAgility > b->currentTurnOrderAgility;
    }

    MusicController* musicCtrl = nullptr;
    BattleMenu* battleMenuCmpt = nullptr;
};
