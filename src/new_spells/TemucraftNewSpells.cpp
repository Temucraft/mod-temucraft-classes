/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// New spells (rows in spell_dbc and the client's Spell.dbc, built by tools/build_spells.py):
// - Steady crowd control: a copy of Polymorph, Entangling Roots, Hibernate, Sap, Gouge, Blind, Repentance, Hex, Shackle
//   Undead and Scatter Shot that damage doesn't break. Learned with the usual spell (the copy of the highest rank you
//   know), same cooldown, level and diminishing returns.
// - Slap: wakes a sleeping player, friend or foe (sleep effects, or lying down asleep); an enemy also takes your basic
//   attack. 5% base mana, 30 sec cooldown, melee range. Every class that uses mana learns it.
// - Later spells made learnable: Heroic Leap (warrior trainers, level 40, 45 sec), Redirect (rogue trainers, level 42:
//   moves your combo points to the new target), Power Word: Barrier (discipline priests from level 60: the raid within
//   10 yards takes 25% less damage for 10 sec, 3 min).
// - Rune of Power (mage trainers, 50) and Call of the Pack (hunter trainers, 40): data only, see the README.

#include "Config.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "TemucraftNewSpellsIds.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <unordered_map>
#include <unordered_set>

namespace
{
    bool steady = true;
    bool slap = true;
    bool barrier = true;
    uint32 barrierLevel = 60;
    std::unordered_map<uint32, uint32> copyOf;     // original -> copy
    std::unordered_set<uint32> copies;

    void Build()
    {
        copyOf.clear();
        copies.clear();
        for (auto const& [orig, copy] : STEADY_COPIES)
            if (sSpellMgr->GetSpellInfo(copy))
            {
                copyOf[orig] = copy;
                copies.insert(copy);
            }
    }

    // the copy of the highest rank known in each spell chain; other copies go
    void Sync(Player* p)
    {
        std::unordered_map<uint32, uint32> best;   // first rank -> highest known original
        if (steady)
            for (auto const& [orig, copy] : copyOf)
                if (p->HasSpell(orig))
                {
                    uint32 const first = sSpellMgr->GetFirstSpellInChain(orig) ? sSpellMgr->GetFirstSpellInChain(orig) : orig;
                    uint32& b = best[first];
                    if (!b || sSpellMgr->GetSpellRank(orig) > sSpellMgr->GetSpellRank(b))
                        b = orig;
                }
        std::unordered_set<uint32> want;
        for (auto const& [first, orig] : best)
            want.insert(copyOf[orig]);
        for (uint32 copy : copies)
        {
            bool const has = p->HasSpell(copy);
            if (want.count(copy) && !has)
                p->learnSpell(copy);
            else if (!want.count(copy) && has)
                p->removeSpell(copy, SPEC_MASK_ALL, false);
        }
        // Power Word: Barrier: discipline priests (most talent points in the first tree) from level 60
        uint8 points[3] = { 0, 0, 0 };
        p->GetTalentTreePoints(points);
        bool const disc = p->getClass() == CLASS_PRIEST && (points[0] || points[1] || points[2]) && p->GetMostPointsTalentTree() == 0;
        bool const wantBarrier = barrier && disc && p->GetLevel() >= barrierLevel;
        if (wantBarrier && !p->HasSpell(SPELL_BARRIER))
            p->learnSpell(SPELL_BARRIER);
        else if (!wantBarrier && p->HasSpell(SPELL_BARRIER))
            p->removeSpell(SPELL_BARRIER, SPEC_MASK_ALL, false);

        bool const wantSlap = slap && p->getPowerType() == POWER_MANA && sSpellMgr->GetSpellInfo(SPELL_SLAP);
        if (wantSlap && !p->HasSpell(SPELL_SLAP))
            p->learnSpell(SPELL_SLAP);
        else if (!wantSlap && p->HasSpell(SPELL_SLAP))
            p->removeSpell(SPELL_SLAP, SPEC_MASK_ALL, false);
    }

    bool Asleep(Unit* u)
    {
        if (u->getStandState() == UNIT_STAND_STATE_SLEEP)
            return true;
        for (auto const& [id, app] : u->GetAppliedAuras())
            if (app->GetBase()->GetSpellInfo()->GetAllEffectsMechanicMask() & (1 << MECHANIC_SLEEP))
                return true;
        return false;
    }
}

class spell_temucraft_slap : public SpellScript
{
    PrepareSpellScript(spell_temucraft_slap);

    SpellCastResult Check()
    {
        Unit* target = GetExplTargetUnit();
        if (!target || !target->IsPlayer() || target == GetCaster())
            return SPELL_FAILED_BAD_TARGETS;
        if (!Asleep(target))
            return SPELL_FAILED_BAD_TARGETS;   // only sleeping players
        return SPELL_CAST_OK;
    }

    void Wake(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;
        target->RemoveAurasWithMechanic(1 << MECHANIC_SLEEP);
        if (target->getStandState() == UNIT_STAND_STATE_SLEEP)
            target->SetStandState(UNIT_STAND_STATE_STAND);
        target->HandleEmoteCommand(EMOTE_ONESHOT_WOUND);
        // the caster does /slap at the target: its animation, sound and "slaps you across the face" line
        if (Player* player = caster->ToPlayer())
        {
            WorldPacket emote(CMSG_TEXT_EMOTE, 4 + 4 + 8);
            emote << uint32(TEXT_EMOTE_SLAP) << uint32(0) << target->GetGUID();
            player->GetSession()->HandleTextEmoteOpcode(emote);
        }
        if (!caster->IsFriendlyTo(target) && caster->IsValidAttackTarget(target))
            caster->AttackerStateUpdate(target, BASE_ATTACK, true);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_temucraft_slap::Check);
        OnEffectHitTarget += SpellEffectFn(spell_temucraft_slap::Wake, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Redirect: the combo points on the old target move to the new one
class spell_temucraft_redirect : public SpellScript
{
    PrepareSpellScript(spell_temucraft_redirect);

    SpellCastResult Check()
    {
        Unit* caster = GetCaster();
        Unit* target = GetExplTargetUnit();
        if (!caster || !caster->GetComboPoints())
            return SPELL_FAILED_NO_COMBO_POINTS;
        if (target && caster->GetComboTarget() == target)
            return SPELL_FAILED_BAD_TARGETS;
        return SPELL_CAST_OK;
    }

    void Move(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;
        uint8 const points = caster->GetComboPoints();
        caster->ClearComboPoints();
        caster->AddComboPoints(target, int8(points));
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_temucraft_redirect::Check);
        OnEffectHitTarget += SpellEffectFn(spell_temucraft_redirect::Move, EFFECT_0, SPELL_EFFECT_ADD_COMBO_POINTS);
    }
};

class TemucraftNewSpellsWorld : public WorldScript
{
public:
    TemucraftNewSpellsWorld() : WorldScript("TemucraftNewSpellsWorld", { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_STARTUP }) { }
    void OnAfterConfigLoad(bool /*reload*/) override
    {
        steady = sConfigMgr->GetOption<bool>("Temucraft.NewSpells.SteadyControl", true);
        slap = sConfigMgr->GetOption<bool>("Temucraft.NewSpells.Slap", true);
        barrier = sConfigMgr->GetOption<bool>("Temucraft.NewSpells.PowerWordBarrier", true);
        barrierLevel = sConfigMgr->GetOption<uint32>("Temucraft.NewSpells.PowerWordBarrier.Level", 60);
    }
    void OnStartup() override { Build(); }
};

class TemucraftNewSpellsPlayer : public PlayerScript
{
public:
    TemucraftNewSpellsPlayer() : PlayerScript("TemucraftNewSpellsPlayer", { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LEARN_SPELL, PLAYERHOOK_ON_FORGOT_SPELL,
        PLAYERHOOK_ON_LEVEL_CHANGED, PLAYERHOOK_ON_TALENTS_RESET, PLAYERHOOK_ON_AFTER_SPEC_SLOT_CHANGED, PLAYERHOOK_ON_PLAYER_LEARN_TALENTS }) { }

    void OnPlayerLogin(Player* p) override { p->m_Events.AddEventAtOffset([p]() { Sync(p); }, 1s); }
    void OnPlayerLearnSpell(Player* p, uint32 spell) override { if (copyOf.count(spell)) Sync(p); }
    void OnPlayerForgotSpell(Player* p, uint32 spell) override { if (copyOf.count(spell)) Sync(p); }
    void OnPlayerLevelChanged(Player* p, uint8 /*old*/) override { Sync(p); }
    void OnPlayerTalentsReset(Player* p, bool /*noCost*/) override { Sync(p); }
    void OnPlayerAfterSpecSlotChanged(Player* p, uint8 /*slot*/) override { Sync(p); }
    void OnPlayerLearnTalents(Player* p, uint32, uint32, uint32) override { p->m_Events.AddEventAtOffset([p]() { Sync(p); }, 100ms); }
};

void AddTemucraftNewSpellsScripts()
{
    RegisterSpellScript(spell_temucraft_slap);
    RegisterSpellScript(spell_temucraft_redirect);
    new TemucraftNewSpellsWorld();
    new TemucraftNewSpellsPlayer();
}
