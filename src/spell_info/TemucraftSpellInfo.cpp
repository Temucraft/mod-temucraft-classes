/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Spell tooltip details: what changes one of your spells right now. The client (TemucraftSpellInfo.lua in the
// mod-temucraft-client-version patch) asks when a spell tooltip is shown and adds the answer as extra lines:
//   Your bonuses: +35% damage, -10% cooldown, -5% cost, -0.5 sec cast
//   Damage: 236 to 265 -> 1031 to 1237          (base -> what you really get; green when better, red when worse)
//   Cost 335 -> 300 mana, Cast 2 -> 1.5 sec
//   Talents:
//     Improved Wrath (5/5): -0.5 sec cast time
//     Moonfury (3/3): +10% damage
//   Other effects: ... (set bonuses, expedition powers, buffs...), Glyphs: ...
//   7% chance to crit for 200% damage
//   Changed by Temucraft: Damage 236 to 265 -> 260 to 290
// Addon messages, prefix "TSI": the client sends "Q~<spell id>", the server answers, in this order:
//   "I~<spell id>~<damage %>~<cooldown %>~<cost %>~<cast ms>~<crit %>~<crit damage %>~<talents>~<others>"
//       (names separated by ','; the 1.4.0 client reads only this one and ignores the rest)
//   "V~<spell id>~<values>"   base and final values, "key:base:final" separated by ',' (only those that differ)
//   "C~<spell id>~<values>"   the same for stock 3.3.5 -> this server (our class changes), if any
//   "T~<spell id>~<kind>~<name>~<rank>~<max rank>~<mods>" one per talent (kind t), glyph (g) or other effect (o);
//       mods are "<SpellModOp>:<f|p>:<value>" separated by ',' (f = flat, p = percent), rank 0 when not a talent
//   "E~<spell id>"            the end of the answer
// Value keys: d direct damage, o damage over time (total), h heal, g heal over time (total) as "min-max", c cost
// (third part: power type), t cast ms, k cooldown ms, r range yd, u duration ms.
// Nothing is sent for spells the player doesn't know.

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
#include <map>
#include <set>
#include <vector>

namespace
{
    constexpr char const* PREFIX = "TSI";
    constexpr size_t MAX_EFFECT_LINES = 16;
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

    // one talent, glyph or other effect and what it does to the spell
    struct Effect
    {
        char kind = 'o';
        std::string name;
        uint32 rank = 0, maxRank = 0;
        std::map<std::pair<int32, char>, int32> mods;   // (SpellModOp, 'f'|'p') -> amount
    };

    // the amount of an effect before any roll, at the player's level (same as SpellEffectInfo::CalcValue)
    int32 LevelPoints(SpellInfo const* info, Player* player, int32 bp, float perLevel)
    {
        if (perLevel != 0.0f)
        {
            int32 level = int32(player->GetLevel());
            if (level > int32(info->MaxLevel) && info->MaxLevel > 0)
                level = int32(info->MaxLevel);
            else if (level < int32(info->BaseLevel))
                level = int32(info->BaseLevel);
            level -= int32(std::max(info->BaseLevel, info->SpellLevel));
            bp += int32(level * perLevel);
        }
        return bp;
    }

    std::pair<int32, int32> Roll(int32 bp, int32 dieSides)
    {
        if (dieSides <= 1)
            return { bp + dieSides, bp + dieSides };
        return { bp + 1, bp + dieSides };
    }

    std::string Range(std::pair<int32, int32> r) { return Acore::StringFormat("{}-{}", std::max(r.first, 0), std::max(r.second, 0)); }

    // "d:236-265:1031-1237": direct damage (d), damage over time (o), heal (h) and heal over time (g)
    void AmountValues(Player* player, SpellInfo const* info, int32 finalDuration, std::vector<std::string>& out)
    {
        std::set<char> done;
        for (SpellEffectInfo const& e : info->GetEffects())
        {
            char key = 0;
            if (e.Effect == SPELL_EFFECT_SCHOOL_DAMAGE) key = 'd';
            else if (e.Effect == SPELL_EFFECT_HEAL) key = 'h';
            else if (e.Effect == SPELL_EFFECT_APPLY_AURA && e.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE) key = 'o';
            else if (e.Effect == SPELL_EFFECT_APPLY_AURA && e.ApplyAuraName == SPELL_AURA_PERIODIC_HEAL) key = 'g';
            if (!key || done.count(key))
                continue;
            done.insert(key);

            bool periodic = key == 'o' || key == 'g';
            int32 baseTicks = 1, finalTicks = 1;
            if (periodic)
            {
                if (!e.Amplitude || info->GetMaxDuration() <= 0)
                    continue;
                baseTicks = std::max<int32>(1, info->GetMaxDuration() / int32(e.Amplitude));
                finalTicks = std::max<int32>(1, finalDuration / int32(e.Amplitude));
            }
            std::pair<int32, int32> base = Roll(LevelPoints(info, player, e.BasePoints, e.RealPointsPerLevel), e.DieSides);
            auto final = [&](int32 v) -> int32
            {
                uint32 amount = uint32(std::max(0.0f, player->ApplyEffectModifiers(info, uint8(e.EffectIndex), float(v))));
                DamageEffectType type = periodic ? DOT : (key == 'h' ? HEAL : SPELL_DIRECT_DAMAGE);
                if (key == 'd' || key == 'o')
                    return int32(player->SpellDamageBonusDone(player, info, amount, type, uint8(e.EffectIndex)));
                return int32(player->SpellHealingBonusDone(player, info, amount, type, uint8(e.EffectIndex)));
            };
            std::pair<int32, int32> now = { final(base.first) * finalTicks, final(base.second) * finalTicks };
            base = { base.first * baseTicks, base.second * baseTicks };
            if (base != now)
                out.push_back(Acore::StringFormat("{}:{}:{}", key, Range(base), Range(now)));
        }
    }

    int32 BaseCost(Player* player, SpellInfo const* info, uint32 manaCost)
    {
        int32 cost = int32(manaCost);
        if (info->ManaCostPercentage)
            cost += int32(CalculatePct(info->PowerType == POWER_MANA ? player->GetCreateMana() :
                player->GetMaxPower(Powers(info->PowerType)), info->ManaCostPercentage));
        return cost;
    }

    // "Changed by Temucraft": stock Spell.dbc against the spell as this server loaded it (class changes)
    std::string StockValues(Player* player, SpellInfo const* info)
    {
        SpellEntry const* stock = sSpellStore.LookupEntry(info->Id);
        if (!stock)
            return {};
        std::vector<std::string> out;
        std::set<char> done;
        int32 stockDuration = info->GetMaxDuration();
        if (SpellDurationEntry const* d = sSpellDurationStore.LookupEntry(stock->DurationIndex))
            stockDuration = d->Duration[2];
        for (SpellEffectInfo const& e : info->GetEffects())
        {
            char key = 0;
            if (e.Effect == SPELL_EFFECT_SCHOOL_DAMAGE) key = 'd';
            else if (e.Effect == SPELL_EFFECT_HEAL) key = 'h';
            else if (e.Effect == SPELL_EFFECT_APPLY_AURA && e.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE) key = 'o';
            else if (e.Effect == SPELL_EFFECT_APPLY_AURA && e.ApplyAuraName == SPELL_AURA_PERIODIC_HEAL) key = 'g';
            if (!key || done.count(key) || stock->Effect[e.EffectIndex] != e.Effect)
                continue;
            done.insert(key);
            int32 oldTicks = 1, newTicks = 1;
            if ((key == 'o' || key == 'g') && e.Amplitude)
            {
                oldTicks = std::max<int32>(1, stockDuration / int32(e.Amplitude));
                newTicks = std::max<int32>(1, info->GetMaxDuration() / int32(e.Amplitude));
            }
            auto oldR = Roll(LevelPoints(info, player, stock->EffectBasePoints[e.EffectIndex], e.RealPointsPerLevel), stock->EffectDieSides[e.EffectIndex]);
            auto newR = Roll(LevelPoints(info, player, e.BasePoints, e.RealPointsPerLevel), e.DieSides);
            oldR = { oldR.first * oldTicks, oldR.second * oldTicks };
            newR = { newR.first * newTicks, newR.second * newTicks };
            if (oldR != newR)
                out.push_back(Acore::StringFormat("{}:{}:{}", key, Range(oldR), Range(newR)));
        }
        if (stock->ManaCost != info->ManaCost)
            out.push_back(Acore::StringFormat("c:{}:{}:{}", BaseCost(player, info, stock->ManaCost), BaseCost(player, info, info->ManaCost), info->PowerType));
        if (SpellCastTimesEntry const* c = sSpellCastTimesStore.LookupEntry(stock->CastingTimeIndex))
            if (info->CastTimeEntry && c->CastTime != info->CastTimeEntry->CastTime)
                out.push_back(Acore::StringFormat("t:{}:{}", c->CastTime, info->CastTimeEntry->CastTime));
        uint32 oldCd = std::max(stock->RecoveryTime, stock->CategoryRecoveryTime), newCd = std::max(info->RecoveryTime, info->CategoryRecoveryTime);
        if (oldCd != newCd)
            out.push_back(Acore::StringFormat("k:{}:{}", oldCd, newCd));
        if (SpellRangeEntry const* r = sSpellRangeStore.LookupEntry(stock->RangeIndex))
            if (info->RangeEntry && r->RangeMax[0] != info->RangeEntry->RangeMax[0])
                out.push_back(Acore::StringFormat("r:{:.0f}:{:.0f}", r->RangeMax[0], info->RangeEntry->RangeMax[0]));
        if (stockDuration != info->GetMaxDuration() && !done.count('o') && !done.count('g'))
            out.push_back(Acore::StringFormat("u:{}:{}", stockDuration, info->GetMaxDuration()));
        std::string s;
        for (std::string const& v : out)
            s += (s.empty() ? "" : ",") + v;
        return s;
    }

    void Answer(Player* player, uint32 spellId)
    {
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
        if (!info || !player->HasSpell(spellId))
            return;

        // spell mods (talents, buffs, set bonuses...) on damage, cooldown, cost and cast time
        float damage = 100.0f;
        player->ApplySpellMod(spellId, SPELLMOD_DAMAGE, damage);
        damage *= player->GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, info->GetSchoolMask());

        std::vector<std::string> values;
        float cooldown = 0.0f;
        uint32 baseCd = std::max(info->RecoveryTime, info->CategoryRecoveryTime);
        if (baseCd)
        {
            int32 cd = int32(baseCd);
            player->ApplySpellMod(spellId, SPELLMOD_COOLDOWN, cd);
            cooldown = PctChange(float(baseCd), float(std::max(cd, 0)));
            if (std::max(cd, 0) != int32(baseCd))
                values.push_back(Acore::StringFormat("k:{}:{}", baseCd, std::max(cd, 0)));
        }

        float cost = 0.0f;
        if (info->ManaCost || info->ManaCostPercentage)
        {
            float c = 100.0f;
            player->ApplySpellMod(spellId, SPELLMOD_COST, c);
            cost = c - 100.0f;
            int32 baseCost = BaseCost(player, info, info->ManaCost);
            int32 nowCost = std::max(0, info->CalcPowerCost(player, info->GetSchoolMask()));
            if (baseCost != nowCost)
                values.push_back(Acore::StringFormat("c:{}:{}:{}", baseCost, nowCost, info->PowerType));
        }

        int32 castDelta = 0;
        if (int32 baseCast = info->CalcCastTime())
        {
            int32 cast = baseCast;
            player->ApplySpellMod(spellId, SPELLMOD_CASTING_TIME, cast);
            castDelta = cast - baseCast;
            int32 now = int32(info->CalcCastTime(player));   // with haste too
            if (now != baseCast)
                values.push_back(Acore::StringFormat("t:{}:{}", baseCast, now));
        }

        if (info->RangeEntry)
        {
            bool friendly = info->IsPositive();
            float base = info->RangeEntry->RangeMax[friendly ? 1 : 0];
            float now = info->GetMaxRange(friendly, player);
            if (base > 5.0f && int32(base + 0.5f) != int32(now + 0.5f))
                values.push_back(Acore::StringFormat("r:{:.0f}:{:.0f}", base, now));
        }

        int32 baseDuration = info->GetMaxDuration(), nowDuration = baseDuration;
        if (baseDuration > 0)
        {
            player->ApplySpellMod(spellId, SPELLMOD_DURATION, nowDuration);
            if (nowDuration != baseDuration)
                values.push_back(Acore::StringFormat("u:{}:{}", baseDuration, nowDuration));
        }
        AmountValues(player, info, nowDuration, values);

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

        // which auras change it, and how: talents apart from the rest
        std::set<std::string> talents, others;
        std::map<uint32, Effect> effects;          // aura spell id -> effect
        std::vector<uint32> order;
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
                bool talent = GetTalentSpellPos(from->GetFirstRankSpell()->Id) != nullptr;
                bool listed = talent || !from->HasAttribute(SPELL_ATTR0_DO_NOT_DISPLAY) || from->IsPassive();
                if (!listed)
                    continue;
                (talent ? talents : others).insert(name);

                auto [it, added] = effects.try_emplace(from->Id);
                Effect& e = it->second;
                if (added)
                {
                    order.push_back(from->Id);
                    e.name = TemucraftAddonClean(name);
                    for (char& c : e.name) if (c == ',' || c == ':') c = ' ';
                    if (talent)
                    {
                        e.kind = 't';
                        if (TalentSpellPos const* pos = GetTalentSpellPos(from->Id))
                        {
                            e.rank = pos->rank + 1;
                            if (TalentEntry const* t = sTalentStore.LookupEntry(pos->talent_id))
                                for (uint32 r : t->RankID)
                                    if (r) ++e.maxRank;
                        }
                    }
                    else if (e.name.rfind("Glyph", 0) == 0)
                        e.kind = 'g';
                }
                e.mods[{ eff->GetMiscValue(), type == SPELL_AURA_ADD_PCT_MODIFIER ? 'p' : 'f' }] += eff->GetAmount();
            }

        TemucraftAddonSend(player, PREFIX, Acore::StringFormat("I~{}~{:.0f}~{:.0f}~{:.0f}~{}~{:.1f}~{:.0f}~{}~{}",
            spellId, damage - 100.0f, cooldown, cost, castDelta, crit, critDamage, Join(talents), Join(others)));

        std::string v;
        for (std::string const& s : values)
            v += (v.empty() ? "" : ",") + s;
        if (!v.empty())
            TemucraftAddonSend(player, PREFIX, Acore::StringFormat("V~{}~{}", spellId, v));
        std::string changed = StockValues(player, info);
        if (!changed.empty())
            TemucraftAddonSend(player, PREFIX, Acore::StringFormat("C~{}~{}", spellId, changed));

        // talents first (by name), then glyphs, then the rest
        std::stable_sort(order.begin(), order.end(), [&](uint32 a, uint32 b)
        {
            auto rankOf = [](char k) { return k == 't' ? 0 : k == 'g' ? 1 : 2; };
            Effect const& x = effects[a];
            Effect const& y = effects[b];
            if (rankOf(x.kind) != rankOf(y.kind))
                return rankOf(x.kind) < rankOf(y.kind);
            return x.name < y.name;
        });
        size_t sent = 0;
        for (uint32 id : order)
        {
            Effect const& e = effects[id];
            std::string mods;
            for (auto const& [key, amount] : e.mods)
                if (amount)
                    mods += Acore::StringFormat("{}{}:{}:{}", mods.empty() ? "" : ",", key.first, key.second, amount);
            if (mods.empty())
                continue;
            if (++sent > MAX_EFFECT_LINES)
                break;
            TemucraftAddonSend(player, PREFIX, Acore::StringFormat("T~{}~{}~{}~{}~{}~{}", spellId, e.kind, e.name, e.rank, e.maxRank, mods));
        }
        TemucraftAddonSend(player, PREFIX, Acore::StringFormat("E~{}", spellId));
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
