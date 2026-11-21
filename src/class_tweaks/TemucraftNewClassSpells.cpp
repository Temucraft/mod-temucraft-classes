/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// New class spells (rows from tools/build_new_spells.py), learned by level like trainer spells:
// - Mages: Overloaded Blast (ranks from 32 to 80). A spell of yours that is fully or partly resisted makes you
//   Overloaded for 5 sec; Overloaded Blast is only usable then, can't be resisted, and uses the Overloaded state up.
// - Night elf priests with a bow: Elune's Arrow (ranks from 20 to 78), weapon damage plus Fire.
// - Shamans with a shield: Stonewall Slam (20), a taunt.
// Switch: Temucraft.NewClassSpells.Enable (and one per spell).

#include "Config.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include <array>
#include <utility>

namespace
{
    // ids below are for the default Temucraft.NewClassSpells.SpellFirst (100110); Id() moves them with it
    constexpr uint32 DEFAULT_FIRST = 100110;
    uint32 spellFirst = DEFAULT_FIRST;
    uint32 Id(uint32 defaultId) { return defaultId - DEFAULT_FIRST + spellFirst; }
    constexpr uint32 SPELL_OVERLOADED = 100117, SPELL_OVERLOAD_SENSE = 100118, SPELL_STONEWALL_SLAM = 100126;
    std::array<std::pair<uint8, uint32>, 7> const BLAST = { { { 32, 100110 }, { 40, 100111 }, { 48, 100112 }, { 56, 100113 },
                                                              { 64, 100114 }, { 72, 100115 }, { 80, 100116 } } };
    std::array<std::pair<uint8, uint32>, 7> const ARROW = { { { 20, 100119 }, { 32, 100120 }, { 44, 100121 }, { 58, 100122 },
                                                              { 66, 100123 }, { 72, 100124 }, { 78, 100125 } } };

    struct Settings { bool enable = true, blast = true, arrow = true, slam = true; uint32 slamLevel = 20; } cfg;

    template<size_t N>
    void LearnRanks(Player* p, std::array<std::pair<uint8, uint32>, N> const& ranks)
    {
        for (auto const& [level, spell] : ranks)
            if (p->GetLevel() >= level && !p->HasSpell(Id(spell)) && sSpellMgr->GetSpellInfo(Id(spell)))
                p->learnSpell(Id(spell));
    }

    void Teach(Player* p)
    {
        if (!cfg.enable)
            return;
        switch (p->getClass())
        {
            case CLASS_MAGE:
                if (!cfg.blast)
                    break;
                if (p->GetLevel() >= BLAST[0].first && !p->HasSpell(Id(SPELL_OVERLOAD_SENSE)) && sSpellMgr->GetSpellInfo(Id(SPELL_OVERLOAD_SENSE)))
                    p->learnSpell(Id(SPELL_OVERLOAD_SENSE));
                LearnRanks(p, BLAST);
                break;
            case CLASS_PRIEST:
                if (cfg.arrow && p->getRace() == RACE_NIGHTELF)
                    LearnRanks(p, ARROW);
                break;
            case CLASS_SHAMAN:
                if (cfg.slam && p->GetLevel() >= cfg.slamLevel && !p->HasSpell(Id(SPELL_STONEWALL_SLAM)) && sSpellMgr->GetSpellInfo(Id(SPELL_STONEWALL_SLAM)))
                    p->learnSpell(Id(SPELL_STONEWALL_SLAM));
                break;
            default:
                break;
        }
    }

    bool IsBlast(uint32 id) { for (auto const& r : BLAST) if (Id(r.second) == id) return true; return false; }
}

class TemucraftNewClassSpells_World : public WorldScript
{
public:
    TemucraftNewClassSpells_World() : WorldScript("TemucraftNewClassSpells_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }
    void OnAfterConfigLoad(bool /*reload*/) override
    {
        cfg.enable = sConfigMgr->GetOption<bool>("Temucraft.NewClassSpells.Enable", true);
        cfg.blast = sConfigMgr->GetOption<bool>("Temucraft.NewClassSpells.OverloadedBlast", true);
        cfg.arrow = sConfigMgr->GetOption<bool>("Temucraft.NewClassSpells.ElunesArrow", true);
        cfg.slam = sConfigMgr->GetOption<bool>("Temucraft.NewClassSpells.StonewallSlam", true);
        spellFirst = sConfigMgr->GetOption<uint32>("Temucraft.NewClassSpells.SpellFirst", DEFAULT_FIRST);
        cfg.slamLevel = sConfigMgr->GetOption<uint32>("Temucraft.NewClassSpells.StonewallSlam.Level", 20);
    }
};

class TemucraftNewClassSpells_Player : public PlayerScript
{
public:
    TemucraftNewClassSpells_Player() : PlayerScript("TemucraftNewClassSpells_Player",
        { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LEVEL_CHANGED, PLAYERHOOK_ON_SPELL_CAST }) { }

    void OnPlayerLogin(Player* p) override { Teach(p); }
    void OnPlayerLevelChanged(Player* p, uint8 /*oldLevel*/) override { Teach(p); }

    // Overloaded Blast uses the Overloaded state up
    void OnPlayerSpellCast(Player* p, Spell* spell, bool /*skipCheck*/) override
    {
        if (spell && IsBlast(spell->GetSpellInfo()->Id))
            p->RemoveAurasDueToSpell(Id(SPELL_OVERLOADED));
    }
};

// 100118 - Overload Sense: only a fully or partly resisted spell overloads you
class spell_temucraft_overload_sense : public AuraScript
{
    PrepareAuraScript(spell_temucraft_overload_sense);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (eventInfo.GetHitMask() & PROC_HIT_FULL_RESIST)
            return true;
        DamageInfo* damage = eventInfo.GetDamageInfo();
        return damage && damage->GetResist() > 0;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_temucraft_overload_sense::CheckProc);
    }
};

void AddTemucraftNewClassSpellsScripts()
{
    new TemucraftNewClassSpells_World();
    new TemucraftNewClassSpells_Player();
    RegisterSpellScript(spell_temucraft_overload_sense);
}
