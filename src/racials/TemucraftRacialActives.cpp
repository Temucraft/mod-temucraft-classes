/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Racial actives (Temucraft.Racials.Actives): the racials of _dev/references/wow-racials_v3.html the races did not
// have, or only had as a renamed copy of a stock racial, adapted to 3.3.5 and balanced for level 80.
//
// - Spells 95835-95876 (data/sql mod_temucraft_racial_actives.sql, made by tools/build_racial_actives.py with
//   TemucraftRacialActivesData.h). They sit on the racial skill lines, so they are learned like the other racials;
//   the skill line rows of the copies they replace now teach the new spell.
// - At login the replaced copies are taken away and the action bar buttons moved to the new spell.
// - Amounts: a part per level plus a part of the higher of attack power and 1.5 x spell power (so a caster and a
//   melee of the same gear land close). The tooltips give the same numbers.
// - Their cooldowns are final: the racials module's cooldown cut skips them (TemucraftRacials.cpp).
// - Actives = 0: the new spells are never learned (and are taken away at login), the replaced copies come back.

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "GameTime.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "TemporarySummon.h"
#include "TemucraftModuleText.h"
#include "TemucraftRacialActivesData.h"
#include "WorldPacket.h"
#include <algorithm>
#include <array>
#include <mutex>
#include <unordered_map>

namespace
{
    namespace D = TemucraftRacialActivesData;

    constexpr char const* MODULE = "mod-temucraft-racial-actives";
    constexpr uint32 MOLE_MENU = 0x4D4F4C45;       // our own gossip menu id ("MOLE")
    constexpr uint32 NPC_DECOY = 12426;            // Masterwork Target Dummy: stays put, can be attacked
    constexpr uint32 SPELL_CLONE_ME = 45204;       // Mirror Image's look copy
    constexpr uint32 SPELL_TAUNT = 355;
    constexpr uint32 GO_MAILBOX = 32349;

    bool sEnable = true;

    char const* Say(Player* player, uint32 id, char const* fallback) { return TemucraftModuleText(MODULE, player, id, fallback); }
    void Tell(Player* player, uint32 id, char const* fallback) { ChatHandler(player->GetSession()).SendSysMessage(Say(player, id, fallback)); }

    bool IsLearned(uint32 spell)
    {
        return std::find(std::begin(D::LEARNED), std::end(D::LEARNED), spell) != std::end(D::LEARNED);
    }

    // the higher of attack power (melee or ranged) and 1.5 x spell power (damage or healing)
    float Power(Unit* unit)
    {
        float const ap = std::max(unit->GetTotalAttackPowerValue(BASE_ATTACK), unit->GetTotalAttackPowerValue(RANGED_ATTACK));
        float const sp = float(std::max(unit->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ALL),
            unit->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_ALL)));
        return std::max(ap, sp * 1.5f);
    }

    int32 Amount(Unit* unit, float perLevel, float powerPct)
    {
        return std::max(1, int32(unit->GetLevel() * perLevel + Power(unit) * powerPct / 100.0f));
    }

    // a spell's own cooldown, started by hand (the cast's cooldown was taken back) and shown on the client
    void StartCooldown(Player* player, uint32 spellId)
    {
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
        if (!info || !info->RecoveryTime)
            return;
        player->AddSpellCooldown(spellId, 0, info->RecoveryTime);
        WorldPacket data(SMSG_SPELL_COOLDOWN, 8 + 1 + 4 + 4);
        data << player->GetGUID();
        data << uint8(0);
        data << uint32(spellId);
        data << uint32(info->RecoveryTime);
        player->SendDirectMessage(&data);
    }

    void ClearCooldownSoon(Player* player, uint32 spellId)
    {
        player->m_Events.AddEventAtOffset([player, spellId]() { player->RemoveSpellCooldown(spellId, true); }, 100ms);
    }

    // ------------------------------------------------------------------------------------------------ state
    struct Rift { Position start; uint32 startMs = 0; };
    struct State
    {
        time_t embraceReady = 0;
        bool hasCamp = false;
        WorldLocation camp;
        Rift rift;
    };

    std::mutex sLock;
    std::unordered_map<ObjectGuid::LowType, State> sStates;

    template <typename F>
    auto WithState(Player* player, F&& f)
    {
        std::lock_guard lock(sLock);
        return f(sStates[player->GetGUID().GetCounter()]);
    }

    // ------------------------------------------------------------------------------------------------ Mole Machine
    struct Place { uint32 text; char const* name; uint32 zone; uint32 map; float x, y, z, o; };
    std::array<Place, 7> const MOLE_PLACES =
    { {
        { 10, "Ironforge",                       1537, 0,   -4918.88f,  -940.406f, 501.564f, 5.42347f },
        { 11, "Thelsamar, Loch Modan",             38, 0,   -5352.54f, -2948.53f,  323.78f,  5.34258f },
        { 12, "Blackrock Mountain",                25, 0,   -7494.94f, -1123.49f,  265.547f, 3.3092f  },
        { 13, "Un'Goro Crater",                   490, 1,   -7943.22f, -2119.09f, -218.343f, 6.0727f  },
        { 14, "Honor Hold, Hellfire Peninsula",  3483, 530,  -748.211f, 2681.52f,  100.35f,  5.7479f  },
        { 15, "Blade's Edge Mountains",          3522, 530,  3037.67f,  5962.86f,  130.774f, 1.27253f },
        { 16, "Valiance Keep, Borean Tundra",    3537, 571,  2213.95f,  5273.15f,  11.2565f, 5.89294f },
    } };

    bool Explored(Player* player, uint32 zone)
    {
        AreaTableEntry const* area = sAreaTableStore.LookupEntry(zone);
        if (!area)
            return false;
        uint32 const offset = area->exploreFlag / 32;
        if (offset >= PLAYER_EXPLORED_ZONES_SIZE)
            return false;
        return player->GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset) & (1u << (area->exploreFlag % 32));
    }

    // ------------------------------------------------------------------------------------------------ Vulpera camp
    void SaveCamp(Player* player, WorldLocation const& camp)
    {
        CharacterDatabase.Execute("REPLACE INTO `mod_temucraft_racial_camp` (`guid`, `map`, `x`, `y`, `z`, `o`) VALUES ({}, {}, {}, {}, {}, {})",
            player->GetGUID().GetCounter(), camp.GetMapId(), camp.GetPositionX(), camp.GetPositionY(), camp.GetPositionZ(),
            camp.GetOrientation());
    }

    void LoadCamp(Player* player)
    {
        if (!player->HasSpell(D::SPELL_RETURN_TO_CAMP))
            return;
        QueryResult result = CharacterDatabase.Query("SELECT `map`, `x`, `y`, `z`, `o` FROM `mod_temucraft_racial_camp` WHERE `guid` = {}",
            player->GetGUID().GetCounter());
        if (!result)
            return;
        Field* f = result->Fetch();
        WorldLocation const camp(f[0].Get<uint16>(), f[1].Get<float>(), f[2].Get<float>(), f[3].Get<float>(), f[4].Get<float>());
        WithState(player, [&](State& st) { st.hasCamp = true; st.camp = camp; });
    }
}

// ---------------------------------------------------------------------------------------------------- spells
// Pterrordax Swoop: only while falling
class spell_temucraft_pterrordax_swoop : public SpellScript
{
    PrepareSpellScript(spell_temucraft_pterrordax_swoop);

    SpellCastResult CheckFalling()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (player && !player->IsFalling())
        {
            Tell(player, 5, "You must be falling to call the Pterrordax.");
            return SPELL_FAILED_DONT_REPORT;
        }
        return SPELL_CAST_OK;
    }

    void Register() override { OnCheckCast += SpellCheckCastFn(spell_temucraft_pterrordax_swoop::CheckFalling); }
};

// Fireblood: harmful poisons, diseases, curses, magic and bleeds go; the highest primary stat grows
class spell_temucraft_fireblood : public SpellScript
{
    PrepareSpellScript(spell_temucraft_fireblood);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;
        uint32 removed = 0;
        Unit::AuraApplicationMap& auras = caster->GetAppliedAuras();
        for (auto itr = auras.begin(); itr != auras.end();)
        {
            AuraApplication const* app = itr->second;
            SpellInfo const* info = app->GetBase()->GetSpellInfo();
            bool const bleed = info->GetAllEffectsMechanicMask() & (uint64(1) << MECHANIC_BLEED);
            bool const type = info->Dispel == DISPEL_POISON || info->Dispel == DISPEL_DISEASE || info->Dispel == DISPEL_CURSE ||
                info->Dispel == DISPEL_MAGIC;
            if (!app->IsPositive() && !info->IsPassive() && (type || bleed))
            {
                caster->RemoveAura(itr, AURA_REMOVE_BY_ENEMY_SPELL);
                ++removed;
            }
            else
                ++itr;
        }

        float const level = float(caster->GetLevel());
        int32 const amount = int32(level * 1.5f + std::min<uint32>(removed, 3) * level * 0.5f);
        std::array<int32, 3> bp = { 0, 0, 0 };   // Strength, Agility, Intellect
        std::array<float, 3> const stats = { caster->GetStat(STAT_STRENGTH), caster->GetStat(STAT_AGILITY), caster->GetStat(STAT_INTELLECT) };
        bp[std::distance(stats.begin(), std::max_element(stats.begin(), stats.end()))] = amount;
        caster->CastCustomSpell(caster, D::SPELL_FIREBLOOD_BUFF, &bp[0], &bp[1], &bp[2], true);
    }

    void Register() override { OnEffectHitTarget += SpellEffectFn(spell_temucraft_fireblood::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY); }
};

// Mole Machine: a menu of the places you have discovered; the cooldown starts when you go
class spell_temucraft_mole_machine : public SpellScript
{
    PrepareSpellScript(spell_temucraft_mole_machine);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;
        ClearCooldownSoon(player, D::SPELL_MOLE_MACHINE);
        ClearGossipMenuFor(player);
        uint32 shown = 0;
        for (uint32 i = 0; i < MOLE_PLACES.size(); ++i)
            if (Explored(player, MOLE_PLACES[i].zone))
            {
                AddGossipItemFor(player, GOSSIP_ICON_TAXI, Say(player, MOLE_PLACES[i].text, MOLE_PLACES[i].name), GOSSIP_SENDER_MAIN, i);
                ++shown;
            }
        if (!shown)
        {
            Tell(player, 1, "The Mole Machine knows no place you have discovered yet.");
            return;
        }
        player->PlayerTalkClass->GetGossipMenu().SetMenuId(MOLE_MENU);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, player->GetGUID());
    }

    void Register() override { OnEffectHitTarget += SpellEffectFn(spell_temucraft_mole_machine::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY); }
};

// Ancestral Call: one of the four spirits
class spell_temucraft_ancestral_call : public SpellScript
{
    PrepareSpellScript(spell_temucraft_ancestral_call);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        static constexpr std::array<uint32, 4> BUFFS = { D::SPELL_FEROCITY_FROSTWOLF, D::SPELL_MIGHT_BLACKROCK,
            D::SPELL_ZEAL_BURNING_BLADE, D::SPELL_RICTUS_LAUGHING_SKULL };
        if (Unit* caster = GetCaster())
            caster->CastSpell(caster, BUFFS[urand(0, BUFFS.size() - 1)], true);
    }

    void Register() override { OnEffectHitTarget += SpellEffectFn(spell_temucraft_ancestral_call::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY); }
};

// Damage of the first effect: level part + power part (Haymaker, Arcane Pulse, Wing Buffet, Tail Swipe)
class spell_temucraft_racial_damage : public SpellScript
{
    PrepareSpellScript(spell_temucraft_racial_damage);

public:
    spell_temucraft_racial_damage(float perLevel, float powerPct) : _perLevel(perLevel), _powerPct(powerPct) { }

private:
    float _perLevel, _powerPct;

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
            SetEffectValue(Amount(caster, _perLevel, _powerPct));
    }

    void Register() override { OnEffectLaunchTarget += SpellEffectFn(spell_temucraft_racial_damage::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE); }
};

// Bag of Tricks: an enemy takes damage and is slowed, an ally is healed and sped up
class spell_temucraft_bag_of_tricks : public SpellScript
{
    PrepareSpellScript(spell_temucraft_bag_of_tricks);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;
        int32 const bp = Amount(caster, 8.0f, 50.0f);
        if (caster->IsValidAttackTarget(target))
            caster->CastCustomSpell(target, D::SPELL_BAG_OF_TRICKS_ENEMY, &bp, nullptr, nullptr, true);
        else if (caster->IsFriendlyTo(target))
            caster->CastCustomSpell(target, D::SPELL_BAG_OF_TRICKS_ALLY, &bp, nullptr, nullptr, true);
    }

    void Register() override { OnEffectHitTarget += SpellEffectFn(spell_temucraft_bag_of_tricks::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY); }
};

// Make Camp / Return to Camp
class spell_temucraft_make_camp : public SpellScript
{
    PrepareSpellScript(spell_temucraft_make_camp);

    SpellCastResult CheckPlace()
    {
        Unit* caster = GetCaster();
        if (caster && (caster->GetMap()->Instanceable() || caster->GetTransport()))
            return SPELL_FAILED_NOT_HERE;
        return SPELL_CAST_OK;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;
        WorldLocation const camp(player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(),
            player->GetOrientation());
        WithState(player, [&](State& st) { st.hasCamp = true; st.camp = camp; });
        SaveCamp(player, camp);
        Tell(player, 3, "Camp made. Return to Camp brings you back here.");
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_temucraft_make_camp::CheckPlace);
        OnEffectHitTarget += SpellEffectFn(spell_temucraft_make_camp::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_temucraft_return_to_camp : public SpellScript
{
    PrepareSpellScript(spell_temucraft_return_to_camp);

    SpellCastResult CheckCamp()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return SPELL_CAST_OK;
        if (player->InBattleground() || player->InArena())
            return SPELL_FAILED_NOT_HERE;
        if (!WithState(player, [](State& st) { return st.hasCamp; }))
        {
            Tell(player, 2, "You have no camp. Use Make Camp first.");
            return SPELL_FAILED_DONT_REPORT;
        }
        return SPELL_CAST_OK;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;
        WorldLocation const camp = WithState(player, [](State& st) { return st.camp; });
        player->TeleportTo(camp.GetMapId(), camp.GetPositionX(), camp.GetPositionY(), camp.GetPositionZ(), camp.GetOrientation());
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_temucraft_return_to_camp::CheckCamp);
        OnEffectHitTarget += SpellEffectFn(spell_temucraft_return_to_camp::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Hyper Organic Light Originator: two decoys that look like you and pull your attackers off you
class spell_temucraft_holo : public SpellScript
{
    PrepareSpellScript(spell_temucraft_holo);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;
        std::vector<ObjectGuid> enemies;
        for (Unit* attacker : player->getAttackers())
            if (Creature* creature = attacker->ToCreature())
                if (!creature->IsDungeonBoss() && !creature->isWorldBoss() && creature->IsWithinDistInMap(player, 15.0f))
                    enemies.push_back(creature->GetGUID());

        for (uint8 i = 0; i < 2; ++i)
        {
            Position const pos = player->GetNearPosition(2.5f, i ? float(M_PI / 2) : float(-M_PI / 2));
            TempSummon* decoy = player->SummonCreature(NPC_DECOY, pos, TEMPSUMMON_TIMED_DESPAWN, 8000);
            if (!decoy)
                continue;
            decoy->SetFaction(player->GetFaction());
            decoy->SetLevel(player->GetLevel());
            decoy->SetMaxHealth(std::max<uint32>(1, player->GetMaxHealth() / 5));
            decoy->SetFullHealth();
            player->CastSpell(decoy, SPELL_CLONE_ME, true);
            // the first decoy taunts now, the second one 4 sec later
            ObjectGuid const decoyGuid = decoy->GetGUID();
            player->m_Events.AddEventAtOffset([player, decoyGuid, enemies]()
            {
                Creature* d = ObjectAccessor::GetCreature(*player, decoyGuid);
                if (!d || !d->IsAlive())
                    return;
                for (ObjectGuid const& guid : enemies)
                    if (Creature* enemy = ObjectAccessor::GetCreature(*player, guid))
                        if (enemy->IsAlive() && enemy->IsWithinDistInMap(d, 30.0f))
                            d->CastSpell(enemy, SPELL_TAUNT, TRIGGERED_FULL_MASK);
            }, i ? 4000ms : 100ms);
        }
    }

    void Register() override { OnEffectHitTarget += SpellEffectFn(spell_temucraft_holo::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY); }
};

// Bull Rush: the leap, then the knock-down where you land
class spell_temucraft_bull_rush : public SpellScript
{
    PrepareSpellScript(spell_temucraft_bull_rush);

    SpellCastResult CheckRooted()
    {
        if (Unit* caster = GetCaster())
            if (caster->HasUnitState(UNIT_STATE_ROOT))
                return SPELL_FAILED_ROOTED;
        return SPELL_CAST_OK;
    }

    void AfterLeap()
    {
        if (Unit* caster = GetCaster())
            caster->m_Events.AddEventAtOffset([caster]() { caster->CastSpell(caster, D::SPELL_BULL_RUSH_STUN, true); }, 550ms);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_temucraft_bull_rush::CheckRooted);
        AfterCast += SpellCastFn(spell_temucraft_bull_rush::AfterLeap);
    }
};

// Light's Judgment: the strike lands 3 sec after the cast
class spell_temucraft_lights_judgment : public SpellScript
{
    PrepareSpellScript(spell_temucraft_lights_judgment);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        WorldLocation const* dest = GetExplTargetDest();
        if (!caster || !dest)
            return;
        Position const pos = dest->GetPosition();
        int32 const bp = Amount(caster, 15.0f, 100.0f);
        caster->m_Events.AddEventAtOffset([caster, pos, bp]()
        {
            SpellInfo const* info = sSpellMgr->GetSpellInfo(D::SPELL_LIGHTS_JUDGMENT_DAMAGE);
            if (!info || !caster->IsInWorld())
                return;
            SpellCastTargets targets;
            targets.SetDst(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
            CustomSpellValues values;
            values.AddSpellMod(SPELLVALUE_BASE_POINT0, bp);
            caster->CastSpell(targets, info, &values, TRIGGERED_FULL_MASK);
        }, 3000ms);
    }

    void Register() override { OnEffectHit += SpellEffectFn(spell_temucraft_lights_judgment::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY); }
};

// Light's Judgment's strike: beyond 8 targets the damage is shared
class spell_temucraft_lights_judgment_damage : public SpellScript
{
    PrepareSpellScript(spell_temucraft_lights_judgment_damage);

    std::size_t _targets = 0;

    void CountTargets(std::list<WorldObject*>& targets) { _targets = targets.size(); }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        if (_targets > 8)
            SetHitDamage(int32(GetHitDamage() * 8 / _targets));
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_temucraft_lights_judgment_damage::CountTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
        OnEffectHitTarget += SpellEffectFn(spell_temucraft_lights_judgment_damage::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// Spatial Rift: the first cast opens the rift (no cooldown yet), the second steps through it
class spell_temucraft_spatial_rift : public SpellScript
{
    PrepareSpellScript(spell_temucraft_spatial_rift);

    SpellCastResult CheckRooted()
    {
        if (Unit* caster = GetCaster())
            if (caster->HasUnitState(UNIT_STATE_ROOT) && caster->HasAura(D::SPELL_SPATIAL_RIFT_OPEN))
                return SPELL_FAILED_ROOTED;
        return SPELL_CAST_OK;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;
        uint32 const now = GameTime::GetGameTimeMS().count();
        if (player->HasAura(D::SPELL_SPATIAL_RIFT_OPEN))
        {
            Rift const rift = WithState(player, [](State& st) { return st.rift; });
            float const dist = std::min(35.0f, 15.0f * float(now - rift.startMs) / 1000.0f);
            Position pos = rift.start;
            player->MovePositionToFirstCollision(pos, dist, rift.start.GetOrientation() - player->GetOrientation());
            player->RemoveAurasDueToSpell(D::SPELL_SPATIAL_RIFT_OPEN);
            player->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), rift.start.GetOrientation());
            return;   // this cast's cooldown stays
        }
        WithState(player, [&](State& st) { st.rift.start = player->GetPosition(); st.rift.startMs = now; });
        player->CastSpell(player, D::SPELL_SPATIAL_RIFT_OPEN, true);
        ClearCooldownSoon(player, D::SPELL_SPATIAL_RIFT);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_temucraft_spatial_rift::CheckRooted);
        OnEffectHitTarget += SpellEffectFn(spell_temucraft_spatial_rift::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// the rift closing unused: now the cooldown starts
class spell_temucraft_spatial_rift_open : public AuraScript
{
    PrepareAuraScript(spell_temucraft_spatial_rift_open);

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Player* player = GetTarget()->ToPlayer())
            if (!player->HasSpellCooldown(D::SPELL_SPATIAL_RIFT))
                player->m_Events.AddEventAtOffset([player]()
                {
                    if (!player->HasSpellCooldown(D::SPELL_SPATIAL_RIFT))
                        StartCooldown(player, D::SPELL_SPATIAL_RIFT);
                }, 200ms);
    }

    void Register() override { AfterEffectRemove += AuraEffectRemoveFn(spell_temucraft_spatial_rift_open::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL); }
};

// Soar: the leap, then the glide
class spell_temucraft_soar : public SpellScript
{
    PrepareSpellScript(spell_temucraft_soar);

    void AfterLeap()
    {
        if (Unit* caster = GetCaster())
            caster->m_Events.AddEventAtOffset([caster]()
            {
                if (caster->IsAlive() && !caster->HasAura(D::SPELL_SOAR_GLIDE))
                    caster->CastSpell(caster, D::SPELL_SOAR_GLIDE, true);
            }, 1100ms);
    }

    void Register() override { AfterCast += SpellCastFn(spell_temucraft_soar::AfterLeap); }
};

// Azerite Surge: 50% more to the healthiest enemy; the heal is the same amount
class spell_temucraft_azerite_surge : public SpellScript
{
    PrepareSpellScript(spell_temucraft_azerite_surge);

    ObjectGuid _healthiest;

    void FindHealthiest(std::list<WorldObject*>& targets)
    {
        uint32 most = 0;
        for (WorldObject* obj : targets)
            if (Unit* unit = obj->ToUnit())
                if (unit->GetHealth() > most)
                {
                    most = unit->GetHealth();
                    _healthiest = unit->GetGUID();
                }
    }

    void HandleAmount(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
            SetEffectValue(Amount(caster, 10.0f, 60.0f));
    }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        if (GetHitUnit() && GetHitUnit()->GetGUID() == _healthiest)
            SetHitDamage(GetHitDamage() * 3 / 2);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_temucraft_azerite_surge::FindHealthiest, EFFECT_0, TARGET_UNIT_CONE_ENEMY_104);
        OnEffectLaunchTarget += SpellEffectFn(spell_temucraft_azerite_surge::HandleAmount, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        OnEffectLaunchTarget += SpellEffectFn(spell_temucraft_azerite_surge::HandleAmount, EFFECT_1, SPELL_EFFECT_HEAL);
        OnEffectHitTarget += SpellEffectFn(spell_temucraft_azerite_surge::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// Quiet Contemplation: you sit down
class spell_temucraft_quiet_contemplation : public AuraScript
{
    PrepareAuraScript(spell_temucraft_quiet_contemplation);

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->SetStandState(UNIT_STAND_STATE_SIT);
    }

    void Register() override { AfterEffectApply += AuraEffectApplyFn(spell_temucraft_quiet_contemplation::HandleApply, EFFECT_0, SPELL_AURA_OBS_MOD_HEALTH, AURA_EFFECT_HANDLE_REAL); }
};

// Thorn Bloom: damage and a DoT on enemies, a heal and a HoT on allies at the spot
class spell_temucraft_thorn_bloom : public SpellScript
{
    PrepareSpellScript(spell_temucraft_thorn_bloom);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        WorldLocation const* dest = GetExplTargetDest();
        if (!caster || !dest)
            return;
        int32 const amount = Amount(caster, 8.0f, 40.0f);
        for (uint32 spellId : { D::SPELL_THORN_BLOOM_DAMAGE, D::SPELL_THORN_BLOOM_HEAL })
            if (SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId))
            {
                SpellCastTargets targets;
                targets.SetDst(dest->GetPositionX(), dest->GetPositionY(), dest->GetPositionZ(), dest->GetOrientation());
                CustomSpellValues values;
                values.AddSpellMod(SPELLVALUE_BASE_POINT0, amount);
                values.AddSpellMod(SPELLVALUE_BASE_POINT1, std::max(1, amount / 4));   // 4 ticks over 12 sec
                caster->CastSpell(targets, info, &values, TRIGGERED_FULL_MASK);
            }
    }

    void Register() override { OnEffectHit += SpellEffectFn(spell_temucraft_thorn_bloom::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY); }
};

// Rootwalking: to Nighthaven, Moonglade
class spell_temucraft_rootwalking : public SpellScript
{
    PrepareSpellScript(spell_temucraft_rootwalking);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr)
            player->TeleportTo(1, 7654.3f, -2232.87f, 462.107f, 5.96786f);
    }

    void Register() override { OnEffectHitTarget += SpellEffectFn(spell_temucraft_rootwalking::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY); }
};

// ---------------------------------------------------------------------------------------------------- hooks
class TemucraftRacialActivesWorld : public WorldScript
{
public:
    TemucraftRacialActivesWorld() : WorldScript("TemucraftRacialActivesWorld", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }
    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sEnable = sConfigMgr->GetOption<bool>("Temucraft.Racials.Enable", true) &&
            sConfigMgr->GetOption<bool>("Temucraft.Racials.Actives", true);
    }
};

class TemucraftRacialActivesPlayer : public PlayerScript
{
public:
    TemucraftRacialActivesPlayer() : PlayerScript("TemucraftRacialActivesPlayer",
        { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT, PLAYERHOOK_CAN_LEARN_SPELL, PLAYERHOOK_ON_GOSSIP_SELECT,
          PLAYERHOOK_ON_PLAYER_JUST_DIED }) { }

    static bool MoveButtons(Player* player, uint32 from, uint32 to)
    {
        bool moved = false;
        for (uint8 i = 0; i < MAX_ACTION_BUTTONS; ++i)
            if (ActionButton const* button = player->GetActionButton(i))
                if (button->GetType() == ACTION_BUTTON_SPELL && button->GetAction() == from)
                {
                    player->addActionButton(i, to, ACTION_BUTTON_SPELL);
                    moved = true;
                }
        return moved;
    }

    void OnPlayerLogin(Player* player) override
    {
        bool moved = false;
        if (!sEnable)
        {
            // switched off: the new racials go, the copies they replaced come back
            for (D::Replaced const& r : D::REPLACED)
                if (player->HasSpell(r.newSpell) && (r.raceMask & player->getRaceMask()))
                {
                    player->learnSpell(r.oldSpell);
                    moved |= MoveButtons(player, r.newSpell, r.oldSpell);
                }
            for (uint32 spell : D::LEARNED)
                if (player->HasSpell(spell))
                    player->removeSpell(spell, SPEC_MASK_ALL, false);
        }
        else
        {
            // (any race: a heritage may have taught another race's copy)
            for (D::Replaced const& r : D::REPLACED)
                if (player->HasSpell(r.oldSpell))
                {
                    if (!player->HasSpell(r.newSpell))
                        player->learnSpell(r.newSpell);
                    moved |= MoveButtons(player, r.oldSpell, r.newSpell);
                    player->removeSpell(r.oldSpell, SPEC_MASK_ALL, false);
                }
            LoadCamp(player);
        }
        if (moved)
            player->SendInitialActionButtons();
    }

    void OnPlayerLogout(Player* player) override
    {
        std::lock_guard lock(sLock);
        sStates.erase(player->GetGUID().GetCounter());
    }

    bool OnPlayerCanLearnSpell(Player* /*player*/, uint32 spellId) override
    {
        return sEnable || !IsLearned(spellId);
    }

    void OnPlayerGossipSelect(Player* player, uint32 menu, uint32 /*sender*/, uint32 action) override
    {
        if (menu != MOLE_MENU)
            return;
        CloseGossipMenuFor(player);
        if (action >= MOLE_PLACES.size() || !player->HasSpell(D::SPELL_MOLE_MACHINE) || player->HasSpellCooldown(D::SPELL_MOLE_MACHINE) ||
            player->IsInCombat() || !Explored(player, MOLE_PLACES[action].zone))
            return;
        Place const& place = MOLE_PLACES[action];
        if (player->TeleportTo(place.map, place.x, place.y, place.z, place.o))
            StartCooldown(player, D::SPELL_MOLE_MACHINE);
    }

    // Light's Reckoning: the Light erupts from the fallen
    void OnPlayerJustDied(Player* player) override
    {
        if (!sEnable || !player->HasSpell(D::SPELL_LIGHTS_RECKONING))
            return;
        int32 const bp = Amount(player, 15.0f, 50.0f);
        player->CastCustomSpell(player, D::SPELL_LIGHTS_RECKONING_BURST, &bp, &bp, nullptr, true);
    }
};

// Embrace of Bwonsamdi: a chance on damage to drain the target
class TemucraftRacialActivesUnit : public UnitScript
{
public:
    TemucraftRacialActivesUnit() : UnitScript("TemucraftRacialActivesUnit", true, { UNITHOOK_ON_DAMAGE }) { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!sEnable || !damage || !attacker || !victim || attacker == victim || !victim->IsAlive())
            return;
        Player* player = attacker->ToPlayer();
        if (!player || !player->HasSpell(D::SPELL_EMBRACE_OF_BWONSAMDI))
            return;
        time_t const now = GameTime::GetGameTime().count();
        bool const proc = WithState(player, [now](State& st)
        {
            if (now < st.embraceReady || !roll_chance_i(15))
                return false;
            st.embraceReady = now + 20;
            return true;
        });
        if (!proc)
            return;
        ObjectGuid const target = victim->GetGUID();
        player->m_Events.AddEventAtOffset([player, target]()
        {
            Unit* unit = ObjectAccessor::GetUnit(*player, target);
            if (!unit || !unit->IsAlive() || !player->IsAlive())
                return;
            int32 const bp = Amount(player, 6.0f, 25.0f);
            player->CastCustomSpell(unit, D::SPELL_BWONSAMDI_DRAIN, &bp, &bp, nullptr, true);
        }, 1ms);
    }
};

void AddTemucraftRacialActivesScripts()
{
    new TemucraftRacialActivesWorld();
    new TemucraftRacialActivesPlayer();
    new TemucraftRacialActivesUnit();
    RegisterSpellScript(spell_temucraft_pterrordax_swoop);
    RegisterSpellScript(spell_temucraft_fireblood);
    RegisterSpellScript(spell_temucraft_mole_machine);
    RegisterSpellScript(spell_temucraft_ancestral_call);
    RegisterSpellScriptWithArgs(spell_temucraft_racial_damage, "spell_temucraft_haymaker", 10.0f, 50.0f);
    RegisterSpellScriptWithArgs(spell_temucraft_racial_damage, "spell_temucraft_arcane_pulse", 10.0f, 75.0f);
    RegisterSpellScriptWithArgs(spell_temucraft_racial_damage, "spell_temucraft_wing_buffet", 5.0f, 20.0f);
    RegisterSpellScript(spell_temucraft_bag_of_tricks);
    RegisterSpellScript(spell_temucraft_make_camp);
    RegisterSpellScript(spell_temucraft_return_to_camp);
    RegisterSpellScript(spell_temucraft_holo);
    RegisterSpellScript(spell_temucraft_bull_rush);
    RegisterSpellScript(spell_temucraft_lights_judgment);
    RegisterSpellScript(spell_temucraft_lights_judgment_damage);
    RegisterSpellScript(spell_temucraft_spatial_rift);
    RegisterSpellScript(spell_temucraft_spatial_rift_open);
    RegisterSpellScript(spell_temucraft_soar);
    RegisterSpellScript(spell_temucraft_azerite_surge);
    RegisterSpellScript(spell_temucraft_quiet_contemplation);
    RegisterSpellScript(spell_temucraft_thorn_bloom);
    RegisterSpellScript(spell_temucraft_rootwalking);
}
