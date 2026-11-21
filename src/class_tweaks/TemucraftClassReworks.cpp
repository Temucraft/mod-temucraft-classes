/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Class reworks:
// - Affliction warlocks feed the group: damage from their curses and corruptions heals nearby group members a little
//   and gives them a little mana.
// - Curses come at a price in a group: a warlock who curses an enemy while grouped carries a weaker copy of that
//   curse for a while (Weakness, Recklessness, Tongues - longer casts -, Elements, Exhaustion). The Malediction talent
//   removes the price and makes Curse of Recklessness lower armor only (no attack power for the target).
// Tank mode and healer threat moved to mod-temucraft-threat.
// - Rogues: every tick of their bleeds adds 1 sec to Slice and Dice (up to its full length).

#include "Config.h"
#include "Group.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace
{
    constexpr char const* MODULE = "mod-temucraft-class-reworks";
    std::unordered_set<uint32> const MALEDICTION = { 32477, 32483, 32484 };
    constexpr uint32 SPELL_CURSE_OF_RECKLESSNESS = 16231;

    struct Settings
    {
        bool enable = true;
        bool affliction = true;
        float afflictionHeal = 2.0f, afflictionMana = 1.0f, afflictionRange = 40.0f;
        bool curses = true;
        bool sliceBleeds = true;
        float cursePct = 25.0f;
        uint32 curseSeconds = 30;
        int32 sliceBleedMs = 1000;
        std::unordered_set<uint32> curseFirstRanks;      // curses with a price
    } settings;

    uint32 FirstRank(uint32 spell)
    {
        uint32 const first = sSpellMgr->GetFirstSpellInChain(spell);
        return first ? first : spell;
    }

    void LoadSettings()
    {
        settings.enable = sConfigMgr->GetOption<bool>("Temucraft.ClassReworks.Enable", true);
        settings.affliction = sConfigMgr->GetOption<bool>("Temucraft.ClassReworks.Affliction", true);
        settings.afflictionHeal = sConfigMgr->GetOption<float>("Temucraft.ClassReworks.Affliction.Health", 2.0f);
        settings.afflictionMana = sConfigMgr->GetOption<float>("Temucraft.ClassReworks.Affliction.Mana", 1.0f);
        settings.afflictionRange = sConfigMgr->GetOption<float>("Temucraft.ClassReworks.Affliction.Range", 40.0f);
        settings.curses = sConfigMgr->GetOption<bool>("Temucraft.ClassReworks.CursePrice", true);
        settings.sliceBleeds = sConfigMgr->GetOption<bool>("Temucraft.ClassReworks.SliceAndDiceBleeds", true);
        settings.cursePct = sConfigMgr->GetOption<float>("Temucraft.ClassReworks.CursePrice.Percent", 25.0f);
        settings.curseSeconds = sConfigMgr->GetOption<uint32>("Temucraft.ClassReworks.CursePrice.Seconds", 30);
        settings.sliceBleedMs = sConfigMgr->GetOption<int32>("Temucraft.ClassReworks.SliceAndDiceBleeds.Milliseconds", 1000);
    }

    // spell lists need the spell store: read once it's loaded
    void LoadSpellLists()
    {
        settings.curseFirstRanks.clear();
        std::string curses = sConfigMgr->GetOption<std::string>("Temucraft.ClassReworks.CursePrice.Curses", "702 16231 1714 1490 18223");
        std::replace(curses.begin(), curses.end(), ',', ' ');
        std::istringstream cin(curses);
        uint32 id;
        while (cin >> id)
            if (sSpellMgr->GetSpellInfo(id))
                settings.curseFirstRanks.insert(FirstRank(id));
    }

    int32 Spec(Player* p)
    {
        uint8 points[3] = { 0, 0, 0 };
        p->GetTalentTreePoints(points);
        if (!points[0] && !points[1] && !points[2])
            return -1;
        return int32(p->getClass()) * 10 + p->GetMostPointsTalentTree();
    }

    bool HasMalediction(Player* p)
    {
        for (uint32 s : MALEDICTION)
            if (p->HasSpell(s) || p->HasTalent(s, p->GetActiveSpec()))
                return true;
        return false;
    }

    // a weaker copy of the curse on the warlock (grouped only)
    void CursePrice(Player* p, SpellInfo const* info)
    {
        if (!settings.curses || !p->GetGroup() || HasMalediction(p))
            return;
        if (!settings.curseFirstRanks.count(FirstRank(info->Id)))
            return;
        uint32 const id = info->Id;
        p->m_Events.AddEventAtOffset([p, id]()
        {
            if (!p->IsAlive())
                return;
            Aura* aura = p->AddAura(id, p);
            if (!aura)
                return;
            int32 const duration = std::min<int32>(aura->GetMaxDuration() > 0 ? aura->GetMaxDuration() : INT32_MAX, int32(settings.curseSeconds * IN_MILLISECONDS));
            aura->SetMaxDuration(duration);
            aura->SetDuration(duration);
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (AuraEffect* eff = aura->GetEffect(i))
                    eff->ChangeAmount(int32(eff->GetAmount() * settings.cursePct / 100.0f));
        }, 200ms);
    }
}

class TemucraftClassReworksWorld : public WorldScript
{
public:
    TemucraftClassReworksWorld() : WorldScript("TemucraftClassReworksWorld", { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_STARTUP }) { }
    void OnAfterConfigLoad(bool reload) override { LoadSettings(); if (reload) LoadSpellLists(); }
    void OnStartup() override { LoadSpellLists(); }
};

class TemucraftClassReworksUnit : public UnitScript
{
public:
    TemucraftClassReworksUnit() : UnitScript("TemucraftClassReworksUnit", true,
        { UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK, UNITHOOK_ON_AURA_APPLY }) { }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* spellInfo) override
    {
        // Rogue bleeds keep Slice and Dice going
        if (settings.enable && settings.sliceBleeds && attacker && attacker->IsPlayer() && attacker->getClass() == CLASS_ROGUE && spellInfo &&
            (spellInfo->GetAllEffectsMechanicMask() & (1 << MECHANIC_BLEED)))
            for (uint32 snd : { 5171u, 6774u })
                if (Aura* aura = attacker->GetAura(snd))
                    aura->SetDuration(std::min(aura->GetDuration() + settings.sliceBleedMs, aura->GetMaxDuration()));
        // Affliction: the warlock's damage over time feeds the group
        Player* p = attacker ? attacker->ToPlayer() : nullptr;
        if (!settings.enable || !settings.affliction || !p || !damage || !spellInfo || p->getClass() != CLASS_WARLOCK ||
            spellInfo->SpellFamilyName != SPELLFAMILY_WARLOCK || Spec(p) != CLASS_WARLOCK * 10 + 0 || !target || target->IsPlayer())
            return;
        int32 const heal = int32(damage * settings.afflictionHeal / 100.0f);
        int32 const mana = int32(damage * settings.afflictionMana / 100.0f);
        if (heal <= 0 && mana <= 0)
            return;
        p->m_Events.AddEventAtOffset([p, heal, mana]()
        {
            auto feed = [heal, mana](Player* m)
            {
                if (!m->IsAlive())
                    return;
                if (heal > 0 && m->GetHealth() < m->GetMaxHealth())
                    m->ModifyHealth(heal);
                if (mana > 0 && m->getPowerType() == POWER_MANA)
                    m->ModifyPower(POWER_MANA, mana);
            };
            if (Group* g = p->GetGroup())
            {
                for (GroupReference* ref = g->GetFirstMember(); ref; ref = ref->next())
                    if (Player* m = ref->GetSource())
                        if (m->GetMap() == p->GetMap() && m->IsWithinDistInMap(p, settings.afflictionRange))
                            feed(m);
            }
            else
                feed(p);
        }, 1ms);
    }

    // Malediction: Curse of Recklessness only lowers armor
    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!settings.enable || !settings.curses || !aura || aura->GetId() != SPELL_CURSE_OF_RECKLESSNESS || unit == aura->GetCaster())
            return;
        Player* caster = aura->GetCaster() ? aura->GetCaster()->ToPlayer() : nullptr;
        if (!caster || !HasMalediction(caster))
            return;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (AuraEffect* eff = aura->GetEffect(i))
                if (eff->GetAuraType() == SPELL_AURA_MOD_ATTACK_POWER && eff->GetAmount() > 0)
                    eff->ChangeAmount(0);
    }
};

class TemucraftClassReworksPlayer : public PlayerScript
{
public:
    TemucraftClassReworksPlayer() : PlayerScript("TemucraftClassReworksPlayer",
        { PLAYERHOOK_ON_SPELL_CAST }) { }

    void OnPlayerSpellCast(Player* p, Spell* spell, bool /*skipCheck*/) override
    {
        if (!settings.enable || !spell || spell->IsTriggered() || p->getClass() != CLASS_WARLOCK)
            return;
        if (SpellInfo const* info = spell->GetSpellInfo())
        {
            Unit* target = spell->m_targets.GetUnitTarget();
            if (target && target != p && !target->IsFriendlyTo(p))
                CursePrice(p, info);
        }
    }
};

void AddTemucraftClassReworksScripts()
{
    new TemucraftClassReworksWorld();
    new TemucraftClassReworksUnit();
    new TemucraftClassReworksPlayer();
}
