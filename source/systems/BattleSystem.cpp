#include "BattleSystem.hpp"
#include "battleActions/skills/BattleCalcs.hpp"
#include "managers/MathManager.hpp"

#include "core/globals.hpp"
#include <cstdlib>
#include <ctime>

void BattleSystem::on_receive(const Event::ExecuteBattle& msg)
{
    isActive = true;
    musicCtrl = MusicController::getInstance();
    battleMenuCmpt = BattleMenu::getInstance();

    std::string path = fatBasePath + "music/battle/" + "mass_destruction.pcm";
    musicCtrl->init(path.c_str(), ae::q20_12_t{0}, ae::q20_12_t{-1});

    this->player = new Player(msg.player);
    battleParticipants.push_back(this->player);
    this->partyMembers.push_back(this->player);

    for (CharacterProfile& characterProfile : msg.characterProfiles)
    {
        PartyMember* partyMember = new PartyMember(characterProfile);
        this->partyMembers.push_back(partyMember);
        battleParticipants.push_back(partyMember);
    }

    for (EnemyProfile& enemyProfile : msg.enemyProfiles)
    {
        Enemy* enemy = new Enemy(enemyProfile);
        this->enemies.push_back(enemy);
        battleParticipants.push_back(enemy);
    }

    this->battleStartCondition = msg.battleStartCondition;

    turnsTaken = 0;
    currentParticipantIndex = 0;
    selectedSkill = nullptr;
    pendingAlert.clear();
    battleResult = Event::BattleResult();
    allOutAttackWasPossibleThisKnockDown = false;
    pendingPersonaSwitch = false;
    switchedPersonaThisTurn = false;
    personaBeforeSwitch = nullptr;

    calculateTurnOrder();

    currentParticipantTurn = battleParticipants[0];

    selectedBattleOption = -1;
    phase = currentParticipantTurn->getInitalTurnPhase();
}

void BattleSystem::Init()
{
    isActive = false;
}

void BattleSystem::Update(ae::q20_12_t)
{
    switch (phase)
    {
    case BattlePhase::ChooseAction:
    {
        PartyMember* actor = static_cast<PartyMember*>(currentParticipantTurn);

        // render battleMenu
        battleMenuCmpt->loadActionOptions(&actions, actor->name);
        selectedBattleOption = -1;
        selectedBattleOption = battleMenuCmpt->consumeSelectedBattleOption();

        if ((selectedBattleOption != -1) && actor->actorCanUse(actions[selectedBattleOption]))
        {
            if (selectedBattleOption == ACTION_ATTACK)
            {
                selectedSkill = actor->baseAttackAction;
                phase = BattlePhase::ChooseTarget;
            }
            else if (selectedBattleOption == ACTION_GUARD)
            {
                TurnResult turnResult = guard.resolve(actor, nullptr);
                applyResult(turnResult);
                advanceTurn();
            }
            else if (selectedBattleOption == ACTION_PERSONA)
            {
                phase = BattlePhase::ChooseSkill;
            }
            else if (selectedBattleOption == ACTION_SWITCH)
            {
                if (switchedPersonaThisTurn)
                {
                    return;
                }
                phase = BattlePhase::ChoosePersona;
            }
        }
        break;
    }

    case BattlePhase::ChooseSkill:
    {
        PartyMember* actor = static_cast<PartyMember*>(currentParticipantTurn);

        // render battleMenu
        battleMenuCmpt->loadSkillOptions(actor->curPersona);
        //TODO: why not just return nullptr if nothing happens instead of setting -1 manually everywhere?

        selectedBattleOption = -1;
        selectedBattleOption = battleMenuCmpt->consumeSelectedBattleOption();

        if (selectedBattleOption != -1)
        {
            Skill* s = actor->curPersona->skills[selectedBattleOption];

            s32* resource;
            if (s->skillRace == SkillRace::mag)
                resource = &actor->sp;
            else
                resource = &actor->hp;

            bool canAfford = *resource >= s->cost;
            if (canAfford)
            {
                *resource -= s->cost;
                selectedSkill = s;
                selectedBattleOption = 0;
                phase = BattlePhase::ChooseTarget;
            }
            else
            {
                pendingAlert = (s->skillRace == SkillRace::mag) ? "Not enough SP\n" : "Not enough HP\n";
                alertReturnPhase = BattlePhase::ChooseSkill;
                battleMenuCmpt->resetLoadedOptions();
                phase = BattlePhase::ShowAlert;
            }
        }

        if (battleMenuCmpt->consumeCancel())
            phase = BattlePhase::ChooseAction;
        break;
    }

    case BattlePhase::ChoosePersona:
    {
        Player* actor = static_cast<Player*>(currentParticipantTurn);

        //inital capture of start persona of this turn, so that if we switch personas and
        //then switch back to the same old starting persona it wont count it as a switch
        if (personaBeforeSwitch == nullptr)
        {
            personaBeforeSwitch = actor->curPersona;
        }

        battleMenuCmpt->loadPersonaOptions(&actor->personas);
        selectedBattleOption = -1;
        selectedBattleOption = battleMenuCmpt->consumeSelectedBattleOption();

        if (selectedBattleOption != -1)
        {
            if (actor->curPersona == actor->personas[selectedBattleOption])
            {
                pendingAlert = "Already using this Persona\n";
                alertReturnPhase = BattlePhase::ChoosePersona;
            }
            else
            {
                actor->curPersona = actor->personas[selectedBattleOption];
                pendingPersonaSwitch = (actor->curPersona != personaBeforeSwitch);
                pendingAlert = "Switched to: " + actor->curPersona->name + "\n";
                alertReturnPhase = BattlePhase::ChooseAction;
            }

            battleMenuCmpt->resetLoadedOptions();
            phase = BattlePhase::ShowAlert;
        }

        if (battleMenuCmpt->consumeCancel())
            phase = BattlePhase::ChooseAction;
        break;
    }

    case BattlePhase::ChooseTarget:
    {
        PartyMember* actor = static_cast<PartyMember*>(currentParticipantTurn);

        bool healTarget = selectedSkill && (selectedSkill->skillType == SkillType::Heal ||
                                            selectedSkill->skillType == SkillType::MultiHeal);

        etl::vector<BattleParticipant*, 13> targets;
        if (healTarget)
        {
            for (PartyMember* partyMember : partyMembers)
            {
                targets.push_back(partyMember);
            }
        }
        else
        {
            for (Enemy* enemy : enemies)
            {
                targets.push_back(enemy);
            }
        }

        targets.erase(std::remove_if(targets.begin(), targets.end(), [](BattleParticipant* t) { return t->hp <= 0; }),
                      targets.end());

        battleMenuCmpt->loadTargetOptions(&targets, healTarget);
        selectedBattleOption = -1;
        selectedBattleOption = battleMenuCmpt->consumeSelectedBattleOption();

        if (selectedBattleOption != -1)
        {
            if (isSingleTarget(selectedSkill->skillType))
            {
                BattleParticipant* selectedTarget = targets[selectedBattleOption];

                targets.clear();
                targets.push_back(selectedTarget);
            }

            // Check so you cant heal target that has max hp
            if (selectedSkill &&
                (selectedSkill->skillType == SkillType::Heal || selectedSkill->skillType == SkillType::MultiHeal))
            {
                bool canHealAnyTarget = false;
                for (BattleParticipant* target : targets)
                {
                    if (target->hp < target->maxHp)
                    {
                        canHealAnyTarget = true;
                        break;
                    }
                }
                if (!canHealAnyTarget)
                    return;
            }

            bool usingBaseAttack = selectedSkill == actor->baseAttackAction;

            for (BattleParticipant* target : targets)
            {
                TurnResult turnResult =
                    (usingBaseAttack) ? attack.resolve(actor, target) : persona.resolve(actor, target, selectedSkill);
                applyResult(turnResult, target);
            }

            advanceTurn();
        }

        if (battleMenuCmpt->consumeCancel())
        {
            phase = (selectedSkill == actor->baseAttackAction) ? BattlePhase::ChooseAction : BattlePhase::ChooseSkill;
            selectedBattleOption = -1;
        }
        break;
    }

    case BattlePhase::ConfirmAllOutAttack:
    {
        battleMenuCmpt->loadAllOutAttackConfirmation();
        selectedBattleOption = -1;
        selectedBattleOption = battleMenuCmpt->consumeSelectedBattleOption();

        if (selectedBattleOption != -1)
        {
            allOutAttackWasPossibleThisKnockDown = true;

            //if yes
            if (selectedBattleOption == 0)
            {
                etl::vector<BattleParticipant*, 13> aliveEnemies = getAliveEnemies();

                uint8_t participantCount = 0;
                for (PartyMember* partyMember : partyMembers)
                {
                    if (partyMember->canParticipateInAllOutAttack())
                    {
                        participantCount++;
                    }
                }

                for (BattleParticipant* enemy : aliveEnemies)
                {
                    u32 damage = BattleCalcs::allOutAttack(*player, *enemy, participantCount);
                    enemy->knockedDown = false;
                    TurnResult turnResult = {true, -(s32)damage, false, enemy->name + ": "};
                    applyResult(turnResult, enemy);
                }

                advanceTurn();
            } //no
            else
            {
                currentParticipantTurn->oneMore = false;
                battleMenuCmpt->resetLoadedOptions();
                phase = BattlePhase::ChooseAction;
            }
        }
        break;
    }

    case BattlePhase::ShowAlert:
    {
        battleMenuCmpt->loadAlertOptions(pendingAlert);
        if (battleMenuCmpt->isAlertExpired(120))
        {
            pendingAlert.clear();
            battleMenuCmpt->resetLoadedOptions();
            phase = alertReturnPhase;
        }
        break;
    }

    case BattlePhase::EnemyTurn:
    {
        Enemy* enemy = static_cast<Enemy*>(currentParticipantTurn);
        Skill* skill = enemy->pickSkill();
        etl::vector<BattleParticipant*, 13> targets;
        //TODO: branch in future if using healing / buff
        for (PartyMember* partyMember : partyMembers)
        {
            targets.push_back(partyMember);
        }

        BattleParticipant* target = enemy->pickTarget(targets);
        TurnResult turnResult = enemy->resolve(target, skill);
        applyResult(turnResult, target);
        advanceTurn();
        //cant be set in advanceTurn, needs to be specifically cleared when an enemy has recoverd
        allOutAttackWasPossibleThisKnockDown = false;
        break;
    }

    case BattlePhase::Done:
        break;
    }

    return;
}

void BattleSystem::Shutdown()
{
    musicCtrl->pause();

    isActive = false;

    turnsTaken = 0;
    currentParticipantIndex = 0;

    currentParticipantTurn = nullptr;

    selectedSkill = nullptr;

    pendingAlert.clear();

    pendingPersonaSwitch = false;
    switchedPersonaThisTurn = false;
    personaBeforeSwitch = nullptr;

    allOutAttackWasPossibleThisKnockDown = false;

    for (BattleParticipant* participant : battleParticipants)
    {
        delete participant;
    }

    battleParticipants.clear();
    partyMembers.clear();
    enemies.clear();
    player = nullptr;
}

void BattleSystem::applyResult(const TurnResult& turnResult, BattleParticipant* target)
{
    if (!turnResult.log.empty())
        pendingAlert += turnResult.log + "\n";

    if (turnResult.hit && target && turnResult.hpDelta != 0)
    {
        s32 hpBefore = target->hp;
        s32 actuallyHealedHp = turnResult.hpDelta;
        target->hp += turnResult.hpDelta;

        if (target->hp > target->maxHp)
        {
            target->hp = target->maxHp;
            actuallyHealedHp = target->maxHp - hpBefore;
        }

        char buf[48];
        if (turnResult.hpDelta < 0)
            std::sprintf(buf, "Damage: %ld\n", (long)-turnResult.hpDelta);
        else
            std::sprintf(buf, "HP healed: %ld\n", (long)actuallyHealedHp);
        pendingAlert += buf;

        std::sprintf(buf, "%s HP: %ld\n", target->name.c_str(), (long)target->hp);
        pendingAlert += buf;
    }

    if (turnResult.oneMore)
    {
        pendingAlert += "One More!\n";
        currentParticipantTurn->oneMore = true;
    }
}

void BattleSystem::advanceTurn()
{
    handleDeadParticipants();

    //all out attack availability check
    if (allEnemiesKnockedDown() && !allOutAttackWasPossibleThisKnockDown)
    {
        alertReturnPhase = BattlePhase::ChooseAction;
        battleMenuCmpt->resetLoadedOptions();
        setNextPhase(BattlePhase::ConfirmAllOutAttack);
        return;
    }

    pendingAlert += "Previous attacker: " + currentParticipantTurn->name + "\n";

    selectedSkill = nullptr;

    switchedPersonaThisTurn = pendingPersonaSwitch;
    pendingPersonaSwitch = false;

    if (currentParticipantTurn->oneMore)
    {
        currentParticipantTurn->oneMore = false;
        BattlePhase nextPhase = currentParticipantTurn->getInitalTurnPhase();

        setNextPhase(nextPhase);
        return;
    }

    switchedPersonaThisTurn = false;
    personaBeforeSwitch = nullptr;

    currentParticipantTurn->knockedDown = false;

    u32 next = (currentParticipantIndex + 1) % battleParticipants.size();
    while (battleParticipants.at(next)->hp <= 0)
        next = (next + 1) % battleParticipants.size();

    currentParticipantIndex = next;
    turnsTaken++;
    currentParticipantTurn = battleParticipants.at(next);

    selectedBattleOption = -1;
    BattlePhase nextPhase = currentParticipantTurn->getInitalTurnPhase();

    setNextPhase(nextPhase);
}

void BattleSystem::setNextPhase(BattlePhase nextPhase)
{
    if (!pendingAlert.empty())
    {
        alertReturnPhase = nextPhase;
        battleMenuCmpt->resetLoadedOptions();
        phase = BattlePhase::ShowAlert;
    }
    else
    {
        phase = nextPhase;
    }
}

// TODO:: potentially rework since this would do a double turn if called again after 1st turn
void BattleSystem::calculateTurnOrder()
{
    // random boost from 1.2 to 1.4 that priorizes party
    ae::q20_12_t boost = ae::q20_12_t{1.2} + MathManager::GetInstance().randFrac() * ae::q20_12_t{0.2};

    for (BattleParticipant* battleParticipant : battleParticipants)
    {
        battleParticipant->setCurrentTurnOrderAgility(boost);
    }

    std::sort(partyMembers.begin(), partyMembers.end(), getParticipantByHigherAgility);
    std::sort(enemies.begin(), enemies.end(), getParticipantByHigherAgility);

    battleParticipants.clear();

    if (battleStartCondition == BattleStartCondition::PartyAdvantage)
    {
        for (auto p : partyMembers)
        {
            battleParticipants.push_back(p);
        }
        for (auto e : enemies)
        {
            battleParticipants.push_back(e);
        }
    }
    else if (battleStartCondition == BattleStartCondition::EnemyAdvantage)
    {
        for (auto e : enemies)
        {
            battleParticipants.push_back(e);
        }
        for (auto p : partyMembers)
        {
            battleParticipants.push_back(p);
        }
    }
    else
    {
        std::merge(partyMembers.begin(),
                   partyMembers.end(),
                   enemies.begin(),
                   enemies.end(),
                   std::back_inserter(battleParticipants),
                   getParticipantByHigherAgility);
    }
}

void BattleSystem::handleDeadParticipants()
{
    for (u32 i = 0; i < battleParticipants.size(); i++)
    {
        if (battleParticipants.at(i)->hp > 0)
        {
            continue;
        }

        BattleParticipant* dead = battleParticipants.at(i);

        dead->onDead(battleResult);

        if (battleResult.playerDied)
        {
            /// Don't display any battle results
            Shutdown();
            return;
        }
    }

    bool enemiesAlive = false;
    for (BattleParticipant* enemy : enemies)
    {
        if (enemy->hp > 0)
        {
            enemiesAlive = true;
            break;
        }
    }

    if (!enemiesAlive)
    {
        /// Display battle results
        ae::BroadcastEvent(battleResult);
        Shutdown();
        return;
    }
}

etl::vector<BattleParticipant*, 13> BattleSystem::getAliveEnemies()
{
    etl::vector<BattleParticipant*, 13> alive;
    for (BattleParticipant* enemy : enemies)
    {
        if (enemy->hp > 0)
            alive.push_back(enemy);
    }
    return alive;
}

bool BattleSystem::allEnemiesKnockedDown()
{
    uint8_t aliveCount = 0;
    uint8_t knockedDownCount = 0;
    for (BattleParticipant* enemy : enemies)
    {
        if (enemy->hp > 0)
        {
            aliveCount++;
            if (enemy->knockedDown)
                knockedDownCount++;
        }
    }

    if (knockedDownCount >= aliveCount)
        return true;

    return false;
}

bool BattleSystem::isSingleTarget(SkillType type)
{
    switch (type)
    {
    case SkillType::RegularAttack:
    case SkillType::Attack:
    case SkillType::Heal:
    case SkillType::Buff:
    case SkillType::Debuff:
        return true;
    default:
        return false;
    }
}
