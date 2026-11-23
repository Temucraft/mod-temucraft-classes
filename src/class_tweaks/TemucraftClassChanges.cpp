/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Class and race changes in the spirit of the classic-plus servers (each with its own switch, Temucraft.ClassChanges.*):
// - Weapon masteries: the racial weapon specializations (Sword, Mace, Axe) give +1% critical strike chance and make
//   attacks 1% less likely to be dodged or parried (4 expertise) with those weapons, instead of a few expertise.
// - Racial heals: every race gets its own take on Cannibalize (10 sec channel, 7% health every 2 sec, 2 min cooldown)
//   with its own name and animation - Second Wind, Hearty Brew, Moonlit Prayer... Undead and skeletons keep Cannibalize.
// - Victory Rush from level 10 (was 62).
// - Shamans in Ghost Wolf walk on water when they know Water Walking.
// - Sentry Totem reveals stealthed and invisible enemies near it.
// - Rolling Thunder: Lightning Bolt and Chain Lightning hits while Lightning Shield is up have a chance to restore
//   mana and add a Lightning Shield charge (up to a maximum).
// - Wildkin Frenzy: taking a direct hit in Moonkin Form gives +5% spell damage, +10% attack power and no spell
//   pushback for 8 sec.
// - Tiger's Fury lasts 18 sec; each bleed tick during it adds a stack of Tiger's Bloodlust (+1% physical damage, up
//   to 10), lost when Tiger's Fury ends or the druid leaves Cat Form.
// - Judgement of Light that would heal someone at full health gives them Light's Reserve instead (+5% healing from
//   the next Judgement of Light heal per stack, up to 20).
// - Overwhelming Light (Holy paladins): Holy Light, Flash of Light or Holy Shock healing someone to full also heals
//   up to 3 injured allies near them for 10% of the heal.
// - Holy Fire: 10% more direct damage; its burn lasts 16 sec and deals 40% more in total.
// - Lightwell: 15 charges, and each player can drink from a Lightwell only once.
// - Banes: Curse of Agony, Doom, Tongues and Exhaustion are banes - a warlock can keep one bane and one other curse
//   on the same target.
// - Demonic Empowerment also empowers an Infernal (immune for 6 sec), a Doomguard (Shadow Bolt Volley) and an
//   enslaved demon (30% attack and casting speed for 15 sec, at the cost of 10% of its health).
// (Holy Shock already reaches allies at 40 yards and Expose Armor already lasts longer per combo point in 3.3.5.)

#include "TemucraftLocked.h"
#include "CellImpl.h"
#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "TemporarySummon.h"
#include <unordered_map>
#include <unordered_set>

namespace
{
    constexpr uint32 SPELL_CANNIBALIZE = 20577;
    constexpr uint32 MENDING_DEFAULT_FIRST = 100172, MENDING_COUNT = 16;
    uint32 MENDING_FIRST = MENDING_DEFAULT_FIRST; // Temucraft.ClassTweaks.RacialHeals.SpellFirst
    // race -> its racial heal (spells from tools/build_racial_mending.py; moved with RacialHeals.SpellFirst)
    std::unordered_map<uint8, uint32> const RACIAL_HEAL = {
        { 1, 100172 }, { 20, 100172 }, { 213, 100172 },                                  // Second Wind: human, Kul Tiran, Gilnean
        { 2, 100173 }, { 23, 100173 }, { 172, 100173 }, { 176, 100173 },                  // Blood Rite: orc, Mag'har, fel orc, vrykul
        { 3, 100174 }, { 22, 100174 },                                                    // Hearty Brew: dwarf, Dark Iron
        { 4, 100175 },                                                                    // Moonlit Prayer: night elf
        { 5, SPELL_CANNIBALIZE }, { 175, SPELL_CANNIBALIZE }, { 180, SPELL_CANNIBALIZE }, // Cannibalize: undead, skeletons
        { 6, 100176 }, { 16, 100176 }, { 179, 100176 },                                   // Earthmother's Embrace: tauren, Highmountain, taunka
        { 7, 100177 }, { 9, 100177 }, { 24, 100177 }, { 169, 100177 },                    // Tinker's Patch: gnome, goblin, mechagnome
        { 8, 100178 }, { 19, 100178 }, { 178, 100178 }, { 181, 100178 },                  // Voodoo Mending: trolls
        { 10, 100179 }, { 15, 100179 }, { 17, 100179 },                                   // Mana Siphon: blood elf, Nightborne, void elf
        { 11, 100180 }, { 18, 100180 }, { 174, 100180 },                                  // Crystal Communion: draenei, Lightforged, Broken
        { 12, 100181 },                                                                   // Lick Wounds: worgen
        { 13, 100182 }, { 14, 100182 },                                                   // Brewmaster's Tea: pandaren
        { 21, 100183 }, { 177, 100183 },                                                  // Campfire Meal: Vulpera, tuskarr
        { 25, 100184 }, { 26, 100184 },                                                   // Draconic Renewal: dracthyr
        { 27, 100185 }, { 28, 100185 },                                                   // Stone Mending: earthen
        { 29, 100186 }, { 30, 100186 },                                                   // Root Mending: Haranir
        { 173, 100187 },                                                                  // Tidal Mending: naga
    };
    constexpr uint32 SPELL_VICTORY_RUSH = 34428;
    constexpr uint32 SPELL_WATER_WALKING = 546;
    constexpr uint32 NPC_SENTRY_TOTEM = 3968;
    std::unordered_set<uint32> const WEAPON_MASTERIES = { 20597, 20864, 20574, 59224 };   // sword, mace, axe, mace (dwarf)
    std::unordered_set<uint32> const TIGERS_FURY = { 5217, 6793, 9845, 9846, 50212, 50213 };
    std::unordered_set<uint32> const HOLY_FIRE = { 14914, 15262, 15263, 15264, 15265, 15266, 15267, 15261, 25384, 48134, 48135 };
    std::unordered_set<uint32> const BANES = { 980, 1014, 6217, 11711, 11712, 11713, 27218, 47863, 47864,   // Agony
                                               603, 30910, 47867, 18223, 1714, 11719 };                    // Doom, Exhaustion, Tongues
    constexpr uint32 SPELL_LIGHTWELL_CHARGES = 59907;
    constexpr uint32 SPELL_JUDGEMENT_OF_LIGHT_HEAL = 20267;
    constexpr uint32 SPELL_DEMONIC_EMPOWERMENT = 47193;
    constexpr uint32 SPELL_SHADOW_BOLT_VOLLEY = 55851;
    // the module's own spells: Temucraft.ClassTweaks.SpellFirst and the next five
    uint32 SPELL_WILDKIN_FRENZY = 96400, SPELL_TIGERS_BLOODLUST = 96401, SPELL_LIGHTS_RESERVE = 96402,
           SPELL_OVERWHELMING_LIGHT = 96403, SPELL_EMPOWERED_INFERNAL = 96404, SPELL_EMPOWERED_THRALL = 96405;
    constexpr uint32 NPC_INFERNAL = 89, NPC_DOOMGUARD = 11859;
    constexpr uint32 TALENT_TAB_HOLY_PALADIN = 382;
    std::unordered_set<uint32> const LIGHTWELLS = { 31883, 31893, 31894, 31895, 31896, 31897 };
    std::unordered_map<uint32, uint32> const LIGHTWELL_RENEW = { { 31897, 7001 }, { 31896, 27873 }, { 31895, 27874 },
                                                                 { 31894, 28276 }, { 31893, 48084 }, { 31883, 48085 } };

    struct Settings
    {
        bool enable = true;
        bool weaponMasteries = true;
        bool cannibalize = true;
        uint32 victoryRushLevel = 10;
        bool ghostWolfWater = true;
        float sentryRadius = 10.0f;      // 0 = off
        float rollingThunderChance = 60.0f, rollingThunderMana = 2.0f;
        uint32 rollingThunderMax = 9;
        bool wildkin = true, tigersFury = true, judgementReserve = true, holyFire = true, banes = true, empowerment = true;
        float overwhelmingPct = 10.0f;
        uint32 lightwellCharges = 15;
        float overwhelmingRange = 10.0f;
        uint32 overwhelmingTargets = 3;
        float reservePerStack = 0.05f;
        int32 empowermentCost = 10;
        float holyFireDirect = 1.1f, holyFireDot = 1.4f;
    } cfg;
    Temucraft::LockedMap<ObjectGuid, std::unordered_set<ObjectGuid>> lightwellDrinkers;

    Temucraft::LockedSet<uint32> ourWaterWalk;            // players walking on water through Ghost Wolf
    Temucraft::LockedMap<ObjectGuid, uint32> totemTimers;

    void LoadSettings()
    {
        cfg.enable = sConfigMgr->GetOption<bool>("Temucraft.ClassChanges.Enable", true);
        cfg.weaponMasteries = sConfigMgr->GetOption<bool>("Temucraft.ClassChanges.WeaponMasteries", true);
        cfg.cannibalize = sConfigMgr->GetOption<bool>("Temucraft.ClassChanges.RacialHeals", true);
        cfg.victoryRushLevel = sConfigMgr->GetOption<uint32>("Temucraft.ClassChanges.VictoryRushLevel", 10);
        cfg.ghostWolfWater = sConfigMgr->GetOption<bool>("Temucraft.ClassChanges.GhostWolfWaterWalking", true);
        cfg.sentryRadius = sConfigMgr->GetOption<float>("Temucraft.ClassChanges.SentryTotemReveal", 10.0f);
        cfg.rollingThunderChance = sConfigMgr->GetOption<float>("Temucraft.ClassChanges.RollingThunder.Chance", 60.0f);
        cfg.rollingThunderMana = sConfigMgr->GetOption<float>("Temucraft.ClassChanges.RollingThunder.Mana", 2.0f);
        cfg.rollingThunderMax = sConfigMgr->GetOption<uint32>("Temucraft.ClassChanges.RollingThunder.MaxCharges", 9);
        cfg.wildkin = sConfigMgr->GetOption<bool>("Temucraft.ClassChanges.WildkinFrenzy", true);
        cfg.tigersFury = sConfigMgr->GetOption<bool>("Temucraft.ClassChanges.TigersFury", true);
        cfg.judgementReserve = sConfigMgr->GetOption<bool>("Temucraft.ClassChanges.JudgementOfLightReserve", true);
        cfg.overwhelmingPct = sConfigMgr->GetOption<float>("Temucraft.ClassChanges.OverwhelmingLight", 10.0f);
        cfg.holyFire = sConfigMgr->GetOption<bool>("Temucraft.ClassChanges.HolyFire", true);
        cfg.lightwellCharges = sConfigMgr->GetOption<uint32>("Temucraft.ClassChanges.LightwellCharges", 15);
        uint32 spellFirst = sConfigMgr->GetOption<uint32>("Temucraft.ClassTweaks.SpellFirst", 96400);
        SPELL_WILDKIN_FRENZY = spellFirst;
        SPELL_TIGERS_BLOODLUST = spellFirst + 1;
        SPELL_LIGHTS_RESERVE = spellFirst + 2;
        SPELL_OVERWHELMING_LIGHT = spellFirst + 3;
        SPELL_EMPOWERED_INFERNAL = spellFirst + 4;
        SPELL_EMPOWERED_THRALL = spellFirst + 5;
        MENDING_FIRST = sConfigMgr->GetOption<uint32>("Temucraft.ClassTweaks.RacialHeals.SpellFirst", MENDING_DEFAULT_FIRST);
        cfg.overwhelmingRange = sConfigMgr->GetOption<float>("Temucraft.ClassTweaks.OverwhelmingLight.Range", 10.0f);
        cfg.overwhelmingTargets = sConfigMgr->GetOption<uint32>("Temucraft.ClassTweaks.OverwhelmingLight.Targets", 3);
        cfg.reservePerStack = sConfigMgr->GetOption<float>("Temucraft.ClassTweaks.LightsReserve.PercentPerStack", 5.0f) / 100.0f;
        cfg.empowermentCost = sConfigMgr->GetOption<int32>("Temucraft.ClassTweaks.DemonicEmpowerment.HealthCost", 10);
        cfg.holyFireDirect = 1.0f + sConfigMgr->GetOption<float>("Temucraft.ClassTweaks.HolyFire.DirectBonus", 10.0f) / 100.0f;
        cfg.holyFireDot = 1.0f + sConfigMgr->GetOption<float>("Temucraft.ClassTweaks.HolyFire.PeriodicBonus", 40.0f) / 100.0f;
        cfg.banes = sConfigMgr->GetOption<bool>("Temucraft.ClassChanges.Banes", true);
        cfg.empowerment = sConfigMgr->GetOption<bool>("Temucraft.ClassChanges.DemonicEmpowerment", true);
    }

    // weapon masteries: effect 0 becomes +1% weapon crit, effect 1 is 4 expertise; the weapon mask stays
    void TuneOther(SpellInfo* spell);

    void TuneSpell(SpellInfo* spell)
    {
        if (!cfg.enable)
            return;
        TuneOther(spell);
        if (!cfg.weaponMasteries || !WEAPON_MASTERIES.count(spell->Id))
            return;
        auto& effects = spell->_GetEffects();
        if (effects[0].ApplyAuraName != SPELL_AURA_MOD_EXPERTISE || effects[1].Effect)
            return;
        effects[1] = effects[0];
        effects[1].EffectIndex = 1;
        effects[1].BasePoints = 4;
        effects[1].DieSides = 0;
        effects[0].ApplyAuraName = SPELL_AURA_MOD_WEAPON_CRIT_PERCENT;
        effects[0].BasePoints = 1;
        effects[0].DieSides = 0;
    }

    void TuneOther(SpellInfo* spell)
    {
        if (cfg.tigersFury && TIGERS_FURY.count(spell->Id))
            if (SpellDurationEntry const* d = sSpellDurationStore.LookupEntry(85))   // 18 sec
                spell->DurationEntry = d;
        if (cfg.holyFire && HOLY_FIRE.count(spell->Id))
        {
            auto& e = spell->_GetEffects();
            int32 const oldTicks = spell->DurationEntry && e[1].Amplitude ? spell->DurationEntry->Duration[0] / int32(e[1].Amplitude) : 7;
            if (SpellDurationEntry const* d = sSpellDurationStore.LookupEntry(387))  // 16 sec
                spell->DurationEntry = d;
            int32 const newTicks = e[1].Amplitude ? 16000 / int32(e[1].Amplitude) : 16;
            e[0].BasePoints = int32(e[0].BasePoints * cfg.holyFireDirect);
            e[0].DieSides = int32(e[0].DieSides * cfg.holyFireDirect);
            if (e[1].ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE && newTicks > 0)
                e[1].BasePoints = std::max<int32>(1, int32((e[1].BasePoints + 1) * cfg.holyFireDot * oldTicks / newTicks) - 1);
        }
        if (spell->Id == SPELL_LIGHTWELL_CHARGES && cfg.lightwellCharges)
            spell->ProcCharges = cfg.lightwellCharges;
        if (cfg.banes && BANES.count(spell->Id) && spell->GetSpellSpecific() == SPELL_SPECIFIC_CURSE)
            spell->_spellSpecific = SPELL_SPECIFIC_STING;   // its own per-caster group: one bane and one curse per target
    }

    Aura* FindOwnAura(Unit* u, uint32 id, ObjectGuid caster) { return u->GetAura(id, caster); }

    void HealAround(Player* paladin, Unit* target, uint32 heal)
    {
        int32 bp = int32(heal * cfg.overwhelmingPct / 100.0f);
        if (bp <= 0)
            return;
        std::list<Unit*> allies;
        Acore::AnyFriendlyUnitInObjectRangeCheck check(target, paladin, cfg.overwhelmingRange, true);
        Acore::UnitListSearcher<Acore::AnyFriendlyUnitInObjectRangeCheck> searcher(target, allies, check);
        Cell::VisitObjects(target, searcher, cfg.overwhelmingRange);
        allies.remove_if([&](Unit* u) { return u == target || !u->IsAlive() || u->IsFullHealth() || !paladin->IsInRaidWith(u); });
        allies.sort([](Unit* a, Unit* b) { return a->GetHealthPct() < b->GetHealthPct(); });
        uint32 n = 0;
        for (Unit* u : allies)
        {
            if (n++ >= cfg.overwhelmingTargets)
                break;
            paladin->CastCustomSpell(u, SPELL_OVERWHELMING_LIGHT, &bp, nullptr, nullptr, true);
        }
    }

    bool IsHolyPaladin(Player* p) { return p->getClass() == CLASS_PALADIN && p->GetMostPointsTalentTree() == TALENT_TAB_HOLY_PALADIN; }

    bool IsBigHeal(SpellInfo const* s)
    {
        return s->SpellFamilyName == SPELLFAMILY_PALADIN && ((s->SpellFamilyFlags[0] & 0xC0000000) || (s->SpellFamilyFlags[1] & 0x10000));
    }

    void Empower(Player* p)
    {
        auto empower = [p](Unit* demon)
        {
            if (!demon || !demon->IsAlive() || !demon->IsCreature())
                return;
            switch (demon->GetEntry())
            {
                case NPC_INFERNAL: demon->CastSpell(demon, SPELL_EMPOWERED_INFERNAL, true); return;
                case NPC_DOOMGUARD: demon->CastSpell(demon, SPELL_SHADOW_BOLT_VOLLEY, true); return;
                default: break;
            }
            if (demon->GetCharmerGUID() == p->GetGUID() && demon->GetCreatureType() == CREATURE_TYPE_DEMON)
            {
                demon->CastSpell(demon, SPELL_EMPOWERED_THRALL, true);
                Unit::DealDamage(p, demon, demon->CountPctFromMaxHealth(cfg.empowermentCost), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_SHADOW, nullptr, false);
            }
        };
        for (Unit* u : p->m_Controlled)
            empower(u);
        empower(p->GetCharm());
    }

    void TeachSpells(Player* p)
    {
        if (!cfg.enable)
            return;
        if (cfg.cannibalize)
        {
            auto it = RACIAL_HEAL.find(p->getRace());
            uint32 heal = it != RACIAL_HEAL.end() ? it->second : MENDING_DEFAULT_FIRST;
            if (heal >= MENDING_DEFAULT_FIRST && heal < MENDING_DEFAULT_FIRST + MENDING_COUNT)
                heal = heal - MENDING_DEFAULT_FIRST + MENDING_FIRST;
            // only this race's heal (a race change, or the earlier Cannibalize-for-all, may have left another)
            for (uint32 id = MENDING_FIRST; id < MENDING_FIRST + MENDING_COUNT; ++id)
                if (id != heal && p->HasSpell(id))
                    p->removeSpell(id, SPEC_MASK_ALL, false);
            if (heal != SPELL_CANNIBALIZE && p->HasSpell(SPELL_CANNIBALIZE) && p->getRace() != RACE_UNDEAD_PLAYER)
                p->removeSpell(SPELL_CANNIBALIZE, SPEC_MASK_ALL, false);
            if (!p->HasSpell(heal) && sSpellMgr->GetSpellInfo(heal))
                p->learnSpell(heal);
        }
        if (cfg.victoryRushLevel && p->getClass() == CLASS_WARRIOR && p->GetLevel() >= cfg.victoryRushLevel && !p->HasSpell(SPELL_VICTORY_RUSH))
            p->learnSpell(SPELL_VICTORY_RUSH);
    }

    bool IsLightningShield(SpellInfo const* s) { return s->SpellFamilyName == SPELLFAMILY_SHAMAN && (s->SpellFamilyFlags[0] & 0x400); }

    // looks for stealthed or invisible enemies of the totem (the usual searchers skip what can't be seen)
    struct HiddenEnemy
    {
        Unit const* totem; float range;
        bool operator()(Unit* u) const
        {
            return u->IsAlive() && totem->IsWithinDistInMap(u, range) && totem->IsHostileTo(u) &&
                (u->HasAuraType(SPELL_AURA_MOD_STEALTH) || u->HasAuraType(SPELL_AURA_MOD_INVISIBILITY));
        }
    };
}

class TemucraftClassChanges_World : public WorldScript
{
public:
    TemucraftClassChanges_World() : WorldScript("TemucraftClassChanges_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }
    void OnAfterConfigLoad(bool /*reload*/) override { LoadSettings(); }
};

class TemucraftClassChanges_Spells : public GlobalScript
{
public:
    TemucraftClassChanges_Spells() : GlobalScript("TemucraftClassChanges_Spells", { GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR }) { }
    void OnLoadSpellCustomAttr(SpellInfo* spell) override { TuneSpell(spell); }
};

class TemucraftClassChanges_Player : public PlayerScript
{
public:
    TemucraftClassChanges_Player() : PlayerScript("TemucraftClassChanges_Player", { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LEVEL_CHANGED, PLAYERHOOK_ON_LOGOUT, PLAYERHOOK_ON_SPELL_CAST }) { }
    void OnPlayerSpellCast(Player* p, Spell* spell, bool /*skipCheck*/) override
    {
        if (cfg.enable && cfg.empowerment && spell && spell->GetSpellInfo()->Id == SPELL_DEMONIC_EMPOWERMENT)
            Empower(p);
    }
    void OnPlayerLogin(Player* p) override { TeachSpells(p); }
    void OnPlayerLevelChanged(Player* p, uint8 /*oldLevel*/) override { TeachSpells(p); }
    void OnPlayerLogout(Player* p) override { ourWaterWalk.erase(p->GetGUID().GetCounter()); }
};

class TemucraftClassChanges_Unit : public UnitScript
{
public:
    TemucraftClassChanges_Unit() : UnitScript("TemucraftClassChanges_Unit", true, { UNITHOOK_ON_UNIT_SET_SHAPESHIFT_FORM, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN,
        UNITHOOK_MODIFY_MELEE_DAMAGE, UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK, UNITHOOK_MODIFY_HEAL_RECEIVED, UNITHOOK_ON_AURA_REMOVE }) { }

    static void Frenzy(Unit* victim, int32 damage)
    {
        if (cfg.enable && cfg.wildkin && damage > 0 && victim && victim->IsPlayer() && victim->GetShapeshiftForm() == FORM_MOONKIN)
            victim->CastSpell(victim, SPELL_WILDKIN_FRENZY, true);
    }

    void ModifyMeleeDamage(Unit* target, Unit* /*attacker*/, uint32& damage) override { Frenzy(target, int32(damage)); }

    // Tiger's Bloodlust: each bleed tick during Tiger's Fury
    void ModifyPeriodicDamageAurasTick(Unit* /*target*/, Unit* attacker, uint32& damage, SpellInfo const* spell) override
    {
        if (!cfg.enable || !cfg.tigersFury || !damage || !attacker || !spell || !attacker->IsPlayer() || attacker->GetShapeshiftForm() != FORM_CAT)
            return;
        if (!(spell->GetAllEffectsMechanicMask() & (1 << MECHANIC_BLEED)) && spell->Mechanic != MECHANIC_BLEED)
            return;
        bool fury = false;
        for (uint32 id : TIGERS_FURY)
            if (attacker->HasAura(id)) { fury = true; break; }
        if (!fury)
            return;
        if (Aura* a = attacker->GetAura(SPELL_TIGERS_BLOODLUST))
        {
            a->ModStackAmount(1);
            a->RefreshDuration();
        }
        else
            attacker->CastSpell(attacker, SPELL_TIGERS_BLOODLUST, true);
    }

    void OnAuraRemove(Unit* unit, AuraApplication* app, AuraRemoveMode /*mode*/) override
    {
        if (unit && app && TIGERS_FURY.count(app->GetBase()->GetId()))
            unit->RemoveAurasDueToSpell(SPELL_TIGERS_BLOODLUST);
    }

    // Light's Reserve and Overwhelming Light
    void ModifyHealReceived(Unit* target, Unit* healer, uint32& heal, SpellInfo const* spell) override
    {
        if (!cfg.enable || !target || !spell || !heal)
            return;
        if (spell->Id == SPELL_JUDGEMENT_OF_LIGHT_HEAL && cfg.judgementReserve)
        {
            if (target->IsFullHealth())
            {
                if (Aura* a = target->GetAura(SPELL_LIGHTS_RESERVE))
                {
                    a->ModStackAmount(1);
                    a->RefreshDuration();
                }
                else
                    target->CastSpell(target, SPELL_LIGHTS_RESERVE, true);
            }
            else if (Aura* a = target->GetAura(SPELL_LIGHTS_RESERVE))
            {
                heal = uint32(heal * (1.0f + cfg.reservePerStack * a->GetStackAmount()));
                a->Remove();
            }
            return;
        }
        Player* paladin = healer ? healer->ToPlayer() : nullptr;
        if (paladin && cfg.overwhelmingPct > 0.0f && IsBigHeal(spell) && IsHolyPaladin(paladin) &&
            heal >= target->GetMaxHealth() - target->GetHealth())
            HealAround(paladin, target, heal);
    }

    // Ghost Wolf walks on water
    void OnUnitSetShapeshiftForm(Unit* unit, uint8 form) override
    {
        Player* p = unit ? unit->ToPlayer() : nullptr;
        if (!p || !cfg.enable || !cfg.ghostWolfWater || p->getClass() != CLASS_SHAMAN)
            return;
        uint32 const guid = p->GetGUID().GetCounter();
        if (form == FORM_GHOSTWOLF && p->HasSpell(SPELL_WATER_WALKING))
        {
            if (!p->HasWaterWalkAura() && ourWaterWalk.insert(guid).second)
                p->SetWaterWalking(true);
        }
        else if (ourWaterWalk.erase(guid) && !p->HasWaterWalkAura())
            p->SetWaterWalking(false);
    }

    // Rolling Thunder
    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spell) override
    {
        Frenzy(target, damage);
        if (!cfg.enable || cfg.rollingThunderChance <= 0.0f || damage <= 0 || !spell || !attacker)
            return;
        Player* p = attacker->ToPlayer();
        if (!p || spell->SpellFamilyName != SPELLFAMILY_SHAMAN || !(spell->SpellFamilyFlags[0] & 0x3))   // Lightning Bolt, Chain Lightning
            return;
        Aura* shield = nullptr;
        for (auto const& [id, app] : p->GetAppliedAuras())
            if (IsLightningShield(app->GetBase()->GetSpellInfo()) && app->GetBase()->GetCasterGUID() == p->GetGUID())
            {
                shield = app->GetBase();
                break;
            }
        if (!shield || !roll_chance_f(cfg.rollingThunderChance))
            return;
        if (shield->GetCharges() < cfg.rollingThunderMax)
            shield->SetCharges(shield->GetCharges() + 1);
        p->ModifyPower(POWER_MANA, int32(p->GetMaxPower(POWER_MANA) * cfg.rollingThunderMana / 100.0f));
    }
};

// Sentry Totem: stealthed and invisible enemies near it are revealed (checked every second)
class TemucraftClassChanges_Creatures : public AllCreatureScript
{
public:
    TemucraftClassChanges_Creatures() : AllCreatureScript("TemucraftClassChanges_Creatures") { }

    void OnAllCreatureUpdate(Creature* c, uint32 diff) override
    {
        if (c->GetEntry() != NPC_SENTRY_TOTEM || !cfg.enable || cfg.sentryRadius <= 0.0f || !c->IsAlive())
            return;
        uint32& timer = totemTimers[c->GetGUID()];
        timer += diff;
        if (timer < 1000)
            return;
        timer = 0;
        std::list<Unit*> found;
        HiddenEnemy check{ c, cfg.sentryRadius };
        Acore::UnitListSearcher<HiddenEnemy> searcher(c, found, check);
        Cell::VisitObjects(c, searcher, cfg.sentryRadius);
        for (Unit* u : found)
        {
            u->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);
            u->RemoveAurasByType(SPELL_AURA_MOD_INVISIBILITY);
        }
    }

    void OnCreatureRemoveWorld(Creature* c) override
    {
        if (c->GetEntry() == NPC_SENTRY_TOTEM)
            totemTimers.erase(c->GetGUID());
        else if (LIGHTWELLS.count(c->GetEntry()))
            lightwellDrinkers.erase(c->GetGUID());
    }
};

// 60123 - Lightwell (replaces the core's spell_pri_lightwell): 15 charges, one drink per player per Lightwell
class spell_temucraft_lightwell : public SpellScript
{
    PrepareSpellScript(spell_temucraft_lightwell);

    bool Load() override { return GetCaster()->IsCreature(); }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        Creature* well = GetCaster()->ToCreature();
        Unit* target = GetHitUnit();
        if (!well || !well->IsSummon() || !target)
            return;
        auto renew = LIGHTWELL_RENEW.find(well->GetEntry());
        if (renew == LIGHTWELL_RENEW.end())
            return;
        if (cfg.enable && cfg.lightwellCharges && !lightwellDrinkers[well->GetGUID()].insert(target->GetGUID()).second)
        {
            if (Player* p = target->ToPlayer())
                ChatHandler(p->GetSession()).SendNotification("You have already drunk from this Lightwell.");
            return;
        }
        if (Aura* charges = well->GetAura(SPELL_LIGHTWELL_CHARGES))
        {
            well->CastSpell(target, renew->second, true, nullptr, nullptr, well->ToTempSummon()->GetSummonerGUID());
            if (charges->ModCharges(-1))
                well->ToTempSummon()->UnSummon();
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_temucraft_lightwell::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

void AddTemucraftClassChangesScripts()
{
    RegisterSpellScript(spell_temucraft_lightwell);
    new TemucraftClassChanges_World();
    new TemucraftClassChanges_Spells();
    new TemucraftClassChanges_Player();
    new TemucraftClassChanges_Unit();
    new TemucraftClassChanges_Creatures();
}
