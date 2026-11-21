/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Spell tooltip details: what changes one of your spells right now. The client (TemucraftSpellInfo.lua in the
// mod-temucraft-client-version patch) asks when a spell tooltip is shown and adds the answer as extra lines:
//   Your bonuses: +35% damage, -10% cooldown, -5% cost, -0.5 sec cast
//   Talents: Improved Wrath, Moonfury
//   Other effects: Moonkin Form, (set bonuses, expedition powers, buffs...)
//   7% chance to crit for 200% damage
// Addon messages, prefix "TSI": the client sends "Q~<spell id>", the server answers
//   "I~<spell id>~<damage %>~<cooldown %>~<cost %>~<cast ms>~<crit %>~<crit damage %>~<talents>~<others>"
// (names separated by ','). Nothing is sent for spells the player doesn't know.

#include "Config.h"
#include "DBCStores.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "StringConvert.h"
#include "StringFormat.h"
#include "TemucraftAddonMessage.h"
#include <set>

namespace
{
    constexpr char const* PREFIX = "TSI";
    bool enabled = true;

    std::string Join(std::set<std::string> const& names)
    {
        std::string out;
        for (std::string const& n : names)
        {
            if (out.size() + n.size() > 70)
                break;
            if (!out.empty())
                out += ',';
            out += TemucraftAddonClean(n);
        }
        for (char& c : out) if (c == '~') c = ' ';
        return out;
    }

    float PctChange(float base, float now) { return base > 0.0f ? (now / base - 1.0f) * 100.0f : 0.0f; }

    void Answer(Player* player, uint32 spellId)
    {
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
        if (!info || !player->HasSpell(spellId))
            return;

        // spell mods (talents, buffs, set bonuses...) on damage, cooldown, cost and cast time
        float damage = 100.0f;
        player->ApplySpellMod(spellId, SPELLMOD_DAMAGE, damage);
        damage *= player->GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, info->GetSchoolMask());

        float cooldown = 0.0f;
        uint32 baseCd = std::max(info->RecoveryTime, info->CategoryRecoveryTime);
        if (baseCd)
        {
            int32 cd = int32(baseCd);
            player->ApplySpellMod(spellId, SPELLMOD_COOLDOWN, cd);
            cooldown = PctChange(float(baseCd), float(std::max(cd, 0)));
        }

        float cost = 0.0f;
        if (info->ManaCost || info->ManaCostPercentage)
        {
            float c = 100.0f;
            player->ApplySpellMod(spellId, SPELLMOD_COST, c);
            cost = c - 100.0f;
        }

        int32 castDelta = 0;
        if (int32 baseCast = info->CalcCastTime())
        {
            int32 cast = baseCast;
            player->ApplySpellMod(spellId, SPELLMOD_CASTING_TIME, cast);
            castDelta = cast - baseCast;
        }

        // crit chance and crit damage, for damage or healing spells that can crit
        float crit = 0.0f, critDamage = 0.0f;
        if (!info->HasAttribute(SPELL_ATTR2_CANT_CRIT) && (info->HasEffect(SPELL_EFFECT_SCHOOL_DAMAGE) ||
            info->HasEffect(SPELL_EFFECT_HEAL) || info->HasAura(SPELL_AURA_PERIODIC_DAMAGE) ||
            info->HasEffect(SPELL_EFFECT_WEAPON_PERCENT_DAMAGE) || info->HasEffect(SPELL_EFFECT_NORMALIZED_WEAPON_DMG) ||
            info->HasEffect(SPELL_EFFECT_WEAPON_DAMAGE)))
        {
            WeaponAttackType att = info->DmgClass == SPELL_DAMAGE_CLASS_RANGED ? RANGED_ATTACK : BASE_ATTACK;
            crit = player->SpellDoneCritChance(nullptr, info, info->GetSchoolMask(), att, true);
            critDamage = float(Unit::SpellCriticalDamageBonus(player, info, 1000, nullptr)) / 10.0f;
        }

        // which auras change it: talents apart from the rest
        std::set<std::string> talents, others;
        int locale = player->GetSession()->GetSessionDbcLocale();
        for (AuraType type : { SPELL_AURA_ADD_FLAT_MODIFIER, SPELL_AURA_ADD_PCT_MODIFIER })
            for (AuraEffect const* eff : player->GetAuraEffectsByType(type))
            {
                if (!eff->IsAffectedOnSpell(info) || !eff->GetAmount())
                    continue;
                SpellInfo const* from = eff->GetSpellInfo();
                if (!from || from->Id == spellId)
                    continue;
                char const* name = from->SpellName[locale] && *from->SpellName[locale] ? from->SpellName[locale] : from->SpellName[0];
                if (!name || !*name)
                    continue;
                if (GetTalentSpellPos(from->GetFirstRankSpell()->Id))
                    talents.insert(name);
                else if (!from->HasAttribute(SPELL_ATTR0_DO_NOT_DISPLAY) || from->IsPassive())
                    others.insert(name);
            }

        TemucraftAddonSend(player, PREFIX, Acore::StringFormat("I~{}~{:.0f}~{:.0f}~{:.0f}~{}~{:.1f}~{:.0f}~{}~{}",
            spellId, damage - 100.0f, cooldown, cost, castDelta, crit, critDamage, Join(talents), Join(others)));
    }
}

class TemucraftSpellInfoWorld : public WorldScript
{
public:
    TemucraftSpellInfoWorld() : WorldScript("TemucraftSpellInfoWorld", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }
    void OnAfterConfigLoad(bool /*reload*/) override
    {
        enabled = sConfigMgr->GetOption<bool>("Temucraft.SpellInfo.Enable", true);
    }
};

class TemucraftSpellInfoPlayer : public PlayerScript
{
public:
    TemucraftSpellInfoPlayer() : PlayerScript("TemucraftSpellInfoPlayer", { PLAYERHOOK_CAN_PLAYER_USE_PRIVATE_CHAT }) { }

    using PlayerScript::OnPlayerCanUseChat;
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Player* receiver) override
    {
        std::string body;
        if (!TemucraftAddonParse(player, type, language, msg, receiver, PREFIX, body))
            return true;
        if (enabled && body.size() > 2 && body[0] == 'Q' && body[1] == '~')
            if (Optional<uint32> id = Acore::StringTo<uint32>(body.substr(2)))
                Answer(player, *id);
        return false;
    }
};

void AddTemucraftSpellInfoScripts()
{
    new TemucraftSpellInfoWorld();
    new TemucraftSpellInfoPlayer();
}
