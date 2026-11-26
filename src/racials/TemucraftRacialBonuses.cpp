/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Racial bonuses at their best (Temucraft.Racials.BestBonuses): for each race the strongest version of every passive
// racial bonus over the years (Retail, Classic Era, TBC/Wrath/Cata/MoP Classic, WoW Forever), the good parts of
// different versions combined, nothing that was later removed.
//
// - Spell data: the changes to existing racial spells (TemucraftRacialBonusesData.h, made by
//   tools/build_racial_bonuses.py) are made at startup: weapon skill and expertise, resistances that grow with the
//   level, more hit, dodge, stun resistance... A restart applies a changed setting.
// - New racial spells 95800-95834 (data/sql mod_temucraft_racial_bonuses.sql) sit on the racial skill lines, so they
//   are learned like the other racials, at creation and at every login, and heritage carries them.
// - What auras can't do is done here: XP against some creature types, Hearthstone cooldown, best vendor discount,
//   more gold, rested XP, falling damage, Well Fed, Plainsrunning, Touch of Elune, Combat Analysis, Emergency
//   Failsafe, Nose for Trouble, Rugged Tenacity, Brush It Off, Entropic Embrace, Savage Blood, Mass Production,
//   Waste Not Want Not, the Elusiveness speed while stealthed. (Sympathetic Vigor's pet half is mod-worgoblin-high-elf's
//   now: it puts 273217 itself on the pet.)
// - BestBonuses = 0: nothing changes, the new spells are never learned and are taken away at login.
// Gathering stats (Deftness, Finesse, Perception) of racials are in mod-temucraft-professions
// (mod_temucraft_profession_racial).

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "ItemTemplate.h"
#include "LootMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "TemucraftRacialBonusesData.h"
#include "Timer.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace
{
    namespace D = TemucraftRacialBonusesData;

    // existing racials
    constexpr uint32 SPELL_HEARTHSTONE = 8690;
    constexpr uint32 SPELL_ELUSIVENESS = 21009;
    constexpr uint32 SPELL_BEAST_SLAYING = 20557;
    constexpr uint32 SPELL_HARDINESS = 20573;
    constexpr uint32 SPELL_BEST_DEALS = 69044;
    constexpr uint32 SPELL_INNER_PEACE_H = 81032, SPELL_INNER_PEACE_A = 81060;
    constexpr uint32 SPELL_BOUNCY_H = 81033, SPELL_BOUNCY_A = 81061;
    constexpr uint32 SPELL_BRUSH_IT_OFF = 81020;
    constexpr uint32 SPELL_ENTROPIC_EMBRACE = 81093;
    constexpr uint32 SPELL_RUGGED_TENACITY = 81110;
    constexpr uint32 SPELL_EMERGENCY_FAILSAFE = 81112;
    constexpr uint32 SPELL_NOSE_FOR_TROUBLE = 81125;
    constexpr uint32 SPELL_SAVAGE_BLOOD = 273220;
    // new ones
    constexpr uint32 SPELL_NO_PLACE_LIKE_HOME = 95800;
    constexpr uint32 SPELL_TOUCH_OF_ELUNE = 95804;
    constexpr uint32 SPELL_PLAINSRUNNING = 95806;
    constexpr uint32 SPELL_ENTROPIC_BUFF = 95813;
    constexpr uint32 SPELL_DEMONBANE = 95814;
    constexpr uint32 SPELL_MASS_PRODUCTION = 95816;
    constexpr uint32 SPELL_EPICUREAN = 95822;
    constexpr uint32 SPELL_WIDE_EYED_WONDER = 95824;
    constexpr uint32 SPELL_SUBTERRANEAN_PREDATOR = 95828;
    constexpr uint32 SPELL_CITY_OF_GOLD = 95831;
    constexpr uint32 SPELL_COMBAT_ANALYSIS = 95832;
    constexpr uint32 SPELL_WASTE_NOT = 95833;
    constexpr uint32 SKILL_BLACKSMITHING_ID = 164;

    struct Settings
    {
        bool enable = true;
        bool resistancePerLevel = true;
        int32 stunResist = 25;
        int32 plainsMax = 60;
    } settings;

    void LoadSettings()
    {
        settings.enable = sConfigMgr->GetOption<bool>("Temucraft.Racials.Enable", true) &&
            sConfigMgr->GetOption<bool>("Temucraft.Racials.BestBonuses", true);
        settings.resistancePerLevel = sConfigMgr->GetOption<bool>("Temucraft.Racials.BestBonuses.ResistancePerLevel", true);
        settings.stunResist = std::clamp(sConfigMgr->GetOption<int32>("Temucraft.Racials.BestBonuses.StunResistChance", 25), 0, 100);
        settings.plainsMax = std::clamp(sConfigMgr->GetOption<int32>("Temucraft.Racials.BestBonuses.PlainsrunningMax", 60), 0, 200);
    }

    bool IsNewLearned(uint32 spell)
    {
        return std::find(std::begin(D::LEARNED), std::end(D::LEARNED), spell) != std::end(D::LEARNED);
    }

    // ------------------------------------------------------------------------------------------------ spell data
    std::unordered_set<uint32> sPerLevelSpells;   // resistances that follow the level: refreshed on level up

    void TuneSpell(SpellInfo* spell)
    {
        static bool loaded = false;
        if (!loaded)
        {
            LoadSettings();   // spell data loads before the world scripts' config hook runs on some setups
            loaded = true;
        }
        if (!settings.enable)
            return;

        for (D::EffectEdit const& edit : D::EFFECTS)
        {
            if (edit.spell != spell->Id || edit.index >= MAX_SPELL_EFFECTS)
                continue;
            int32 value = edit.value;
            bool perLevel = edit.perLevel;
            if (perLevel && !settings.resistancePerLevel)
            {
                perLevel = false;
                value = 10;   // the flat resistance of the first versions
            }
            if (edit.spell == SPELL_HARDINESS && edit.aura == SPELL_AURA_MOD_MECHANIC_RESISTANCE)
            {
                if (!settings.stunResist)
                    continue;
                value = settings.stunResist;
            }

            SpellEffectInfo& e = spell->_GetEffects()[edit.index];
            e.Effect = SPELL_EFFECT_APPLY_AURA;
            e.ApplyAuraName = AuraType(edit.aura);
            e.Amplitude = 0;
            e.DieSides = 1;
            e.BasePoints = perLevel ? -1 : value - 1;   // stored one below the value
            e.RealPointsPerLevel = perLevel ? 1.0f : 0.0f;
            e.PointsPerComboPoint = 0.0f;
            e.ValueMultiplier = 0.0f;
            e.MiscValue = edit.misc;
            e.MiscValueB = 0;
            e.Mechanic = MECHANIC_NONE;
            e.TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
            e.TargetB = SpellImplicitTargetInfo();
            e.RadiusEntry = nullptr;
            e.ChainTarget = 0;
            e.ItemType = 0;
            e.TriggerSpell = 0;
            e.SpellClassMask = flag96(edit.familyMask, 0, 0);
            if (edit.family)
                spell->SpellFamilyName = edit.family;
            // a bonus: never a harmful effect
            spell->AttributesCu &= ~(uint32(SPELL_ATTR0_CU_NEGATIVE_EFF0) << edit.index);
            if (perLevel)
                sPerLevelSpells.insert(spell->Id);
        }

        if (std::find(std::begin(D::NO_ITEM), std::end(D::NO_ITEM), spell->Id) != std::end(D::NO_ITEM))
        {
            spell->EquippedItemClass = -1;
            spell->EquippedItemSubClassMask = 0;
            spell->EquippedItemInventoryTypeMask = 0;
        }
    }

    bool IsBlacksmithingCraft(SpellInfo const* spell)
    {
        if (!spell->HasEffect(SPELL_EFFECT_CREATE_ITEM) && !spell->HasEffect(SPELL_EFFECT_CREATE_ITEM_2))
            return false;
        SkillLineAbilityMapBounds bounds = sSpellMgr->GetSkillLineAbilityMapBounds(spell->Id);
        for (auto itr = bounds.first; itr != bounds.second; ++itr)
            if (itr->second->SkillLine == SKILL_BLACKSMITHING_ID)
                return true;
        return false;
    }

    // ------------------------------------------------------------------------------------------------ per player
    struct State
    {
        uint32 timer = 0;            // ms to the next 1 sec tick
        uint8 ticks = 0;             // 1 sec ticks, every 5th one does the slow work
        uint8 plains = 0;            // Plainsrunning stacks (2% each)
        uint8 analysis = 0;          // Combat Analysis stacks
        float lastRest = -1.0f;      // Inner Peace: rested XP seen at the last tick
        std::unordered_set<ObjectGuid> trouble;   // Nose for Trouble: enemies that already hit
        std::array<float, 4> heal{};              // Brush It Off: healing due in the next 4 seconds
        uint8 healPos = 0;
        time_t failsafeReady = 0;    // Emergency Failsafe
        time_t embraceReady = 0;     // Entropic Embrace
    };

    std::mutex sLock;
    std::unordered_map<ObjectGuid::LowType, State> sStates;

    template <typename F>
    void WithState(Player* player, F&& f)
    {
        std::lock_guard lock(sLock);
        f(sStates[player->GetGUID().GetCounter()]);
    }

    bool HasInnerPeace(Player* player) { return player->HasSpell(SPELL_INNER_PEACE_H) || player->HasSpell(SPELL_INNER_PEACE_A); }
    bool IsResting(Player* player)
    {
        return player->HasRestFlag(REST_FLAG_IN_TAVERN) || player->HasRestFlag(REST_FLAG_IN_CITY) ||
            player->HasRestFlag(REST_FLAG_IN_FACTION_AREA);
    }

    void SetAmount(Player* player, uint32 spell, uint8 index, int32 amount)
    {
        if (AuraEffect* eff = player->GetAuraEffect(spell, index))
            if (eff->GetAmount() != amount)
                eff->ChangeAmount(amount);
    }

    // Elusiveness: 5% more speed while stealthed (its third effect)
    void UpdateElusiveness(Player* player)
    {
        if (player->HasSpell(SPELL_ELUSIVENESS))
            SetAmount(player, SPELL_ELUSIVENESS, EFFECT_2, player->HasStealthAura() ? 5 : 0);
    }

    void Heal(Player* player, uint32 amount, uint32 spellId)
    {
        if (!amount || !player->IsAlive())
            return;
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
        HealInfo heal(player, player, amount, info, info ? info->GetSchoolMask() : SPELL_SCHOOL_MASK_NORMAL);
        player->HealBySpell(heal);
    }

    // every second
    void Tick(Player* player, State& st)
    {
        // Brush It Off: the healing due this second
        float due = st.heal[st.healPos];
        st.heal[st.healPos] = 0.0f;
        st.healPos = (st.healPos + 1) % st.heal.size();
        if (due >= 1.0f && player->HasSpell(SPELL_BRUSH_IT_OFF))
        {
            uint32 const amount = uint32(due);
            player->m_Events.AddEventAtOffset([player, amount]() { Heal(player, amount, SPELL_BRUSH_IT_OFF); }, 1ms);
        }

        // Plainsrunning: 2% more speed for each second on the move, on foot
        if (player->HasSpell(SPELL_PLAINSRUNNING))
        {
            bool const onFoot = player->isMoving() && !player->IsMounted() && !player->IsInFlight() && !player->IsFlying() &&
                !player->GetVehicle() && !player->IsInWater();
            uint8 const most = uint8(settings.plainsMax / 2);
            st.plains = onFoot ? std::min<uint8>(most, st.plains + 1) : 0;
            SetAmount(player, SPELL_PLAINSRUNNING, EFFECT_0, st.plains * 2);
        }

        if (++st.ticks < 5)
            return;
        st.ticks = 0;

        // Touch of Elune: haste at night, crit during the day (server time)
        if (player->HasSpell(SPELL_TOUCH_OF_ELUNE))
        {
            std::tm const now = Acore::Time::TimeBreakdown(GameTime::GetGameTime().count());
            bool const night = now.tm_hour < 6 || now.tm_hour >= 18;
            SetAmount(player, SPELL_TOUCH_OF_ELUNE, EFFECT_0, night ? 1 : 0);
            SetAmount(player, SPELL_TOUCH_OF_ELUNE, EFFECT_1, night ? 1 : 0);
            SetAmount(player, SPELL_TOUCH_OF_ELUNE, EFFECT_2, night ? 0 : 1);
        }

        // Combat Analysis: a stack every 5 sec in combat, one fades every 5 sec out of it; 0.5% each (whole percents)
        if (player->HasSpell(SPELL_COMBAT_ANALYSIS))
        {
            if (player->IsInCombat())
                st.analysis = std::min<uint8>(10, st.analysis + 1);
            else if (st.analysis)
                --st.analysis;
            for (uint8 i = 0; i < 3; ++i)
                SetAmount(player, SPELL_COMBAT_ANALYSIS, i, st.analysis / 2);
        }

        // Nose for Trouble: a new fight, new enemies
        if (!player->IsInCombat())
            st.trouble.clear();

        UpdateElusiveness(player);

        // Inner Peace: rested XP earned in game counts twice
        if (HasInnerPeace(player))
        {
            float const now = player->GetRestBonus();
            if (st.lastRest >= 0.0f && now > st.lastRest && IsResting(player))
                player->SetRestBonus(now + (now - st.lastRest));
            st.lastRest = player->GetRestBonus();
        }
    }
}

// ---------------------------------------------------------------------------------------------------- scripts
class TemucraftRacialBonusesWorld : public WorldScript
{
public:
    TemucraftRacialBonusesWorld() : WorldScript("TemucraftRacialBonusesWorld", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }
    void OnAfterConfigLoad(bool reload) override
    {
        // the spell data part keeps the values it started with (a restart applies a change)
        bool const enable = settings.enable;
        LoadSettings();
        if (reload)
            settings.enable = enable;
    }
};

class TemucraftRacialBonusesGlobal : public GlobalScript
{
public:
    TemucraftRacialBonusesGlobal() : GlobalScript("TemucraftRacialBonusesGlobal",
        { GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR, GLOBALHOOK_ON_IS_AFFECTED_BY_SPELL_MOD_CHECK }) { }

    void OnLoadSpellCustomAttr(SpellInfo* spell) override { TuneSpell(spell); }

    // Mass Production's cast time modifier only touches Blacksmithing crafts
    bool OnIsAffectedBySpellModCheck(SpellInfo const* affectSpell, SpellInfo const* checkSpell, SpellModifier const* /*mod*/) override
    {
        if (settings.enable && affectSpell && affectSpell->Id == SPELL_MASS_PRODUCTION && checkSpell && IsBlacksmithingCraft(checkSpell))
            return false;   // false = affected
        return true;
    }
};

class TemucraftRacialBonusesPlayer : public PlayerScript
{
public:
    TemucraftRacialBonusesPlayer() : PlayerScript("TemucraftRacialBonusesPlayer",
        { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT, PLAYERHOOK_ON_LEVEL_CHANGED, PLAYERHOOK_CAN_LEARN_SPELL,
          PLAYERHOOK_ON_UPDATE, PLAYERHOOK_ON_GIVE_EXP, PLAYERHOOK_ON_BEFORE_LOOT_MONEY, PLAYERHOOK_ON_SPELL_CAST,
          PLAYERHOOK_ON_GET_REPUTATION_PRICE_DISCOUNT }) { }

    void OnPlayerLogin(Player* player) override
    {
        if (!settings.enable)
        {
            // switched off: the new racials go (they come back with the setting)
            for (uint32 spell : D::LEARNED)
                if (player->HasSpell(spell))
                    player->removeSpell(spell, SPEC_MASK_ALL, false);
            for (uint32 spell = D::FIRST_SPELL; spell <= D::LAST_SPELL; ++spell)
                player->RemoveAurasDueToSpell(spell);
            return;
        }

        // Inner Peace: the rested XP earned while logged out counts twice (the database still has the old value)
        if (HasInnerPeace(player))
            if (QueryResult result = CharacterDatabase.Query("SELECT `rest_bonus` FROM `characters` WHERE `guid` = {}", player->GetGUID().GetCounter()))
            {
                float const before = result->Fetch()[0].Get<float>();
                float const now = player->GetRestBonus();
                if (now > before)
                    player->SetRestBonus(now + (now - before));
            }
        WithState(player, [player](State& st) { st.lastRest = player->GetRestBonus(); });
        UpdateElusiveness(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        std::lock_guard lock(sLock);
        sStates.erase(player->GetGUID().GetCounter());
    }

    bool OnPlayerCanLearnSpell(Player* /*player*/, uint32 spellId) override
    {
        return settings.enable || !IsNewLearned(spellId);
    }

    // resistances equal to the level: worked out again
    void OnPlayerLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if (!settings.enable)
            return;
        for (uint32 spell : sPerLevelSpells)
            if (player->HasAura(spell))
            {
                player->RemoveAurasDueToSpell(spell);
                player->AddAura(spell, player);
            }
    }

    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        if (!settings.enable)
            return;
        std::lock_guard lock(sLock);
        State& st = sStates[player->GetGUID().GetCounter()];
        if (st.timer > diff)
        {
            st.timer -= diff;
            return;
        }
        st.timer = 1000;
        Tick(player, st);
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        if (!settings.enable || !amount)
            return;
        float pct = 0.0f;
        if (xpSource == XPSOURCE_EXPLORE && player->HasSpell(SPELL_WIDE_EYED_WONDER))
            pct += 200.0f;
        if (xpSource == XPSOURCE_KILL && victim)
        {
            uint32 const type = victim->GetCreatureType();
            if (type == CREATURE_TYPE_BEAST && player->HasSpell(SPELL_BEAST_SLAYING))
                pct += 20.0f;
            if (type == CREATURE_TYPE_DEMON && player->HasSpell(SPELL_DEMONBANE))
                pct += 20.0f;
            if ((type == CREATURE_TYPE_BEAST || type == CREATURE_TYPE_ELEMENTAL) && player->HasSpell(SPELL_SUBTERRANEAN_PREDATOR))
                pct += 10.0f;
        }
        if (pct > 0.0f)
            amount = uint32(amount * (100.0f + pct) / 100.0f);
    }

    // City of Gold: 2% more gold from creatures
    void OnPlayerBeforeLootMoney(Player* player, Loot* loot) override
    {
        if (settings.enable && loot && loot->gold && player->GetLootGUID().IsCreature() && player->HasSpell(SPELL_CITY_OF_GOLD))
            loot->gold += std::max<uint32>(1, loot->gold * 2 / 100);
    }

    // No Place Like Home: Hearthstone 5 min shorter
    void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!settings.enable || !spell || spell->GetSpellInfo()->Id != SPELL_HEARTHSTONE || !player->HasSpell(SPELL_NO_PLACE_LIKE_HOME))
            return;
        player->m_Events.AddEventAtOffset([player]()
        {
            if (player->HasSpellCooldown(SPELL_HEARTHSTONE))
                player->ModifySpellCooldown(SPELL_HEARTHSTONE, -int32(5 * MINUTE * IN_MILLISECONDS));
        }, 300ms);
    }

    // Best Deals Anywhere: the Exalted discount (20%) from every vendor
    void OnPlayerGetReputationPriceDiscount(Player const* player, FactionTemplateEntry const* /*faction*/, float& discount) override
    {
        if (settings.enable && player->HasSpell(SPELL_BEST_DEALS))
            discount = std::min(discount, 0.8f);
    }
};

class TemucraftRacialBonusesUnit : public UnitScript
{
public:
    TemucraftRacialBonusesUnit() : UnitScript("TemucraftRacialBonusesUnit", true,
        { UNITHOOK_ON_DAMAGE, UNITHOOK_ON_HEAL, UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_AURA_REMOVE }) { }

    static void Embrace(Player* player)
    {
        if (!player->HasSpell(SPELL_ENTROPIC_EMBRACE))
            return;
        time_t const now = GameTime::GetGameTime().count();
        bool proc = false;
        WithState(player, [&](State& st)
        {
            if (now >= st.embraceReady && roll_chance_i(10))
            {
                st.embraceReady = now + 30;
                proc = true;
            }
        });
        if (proc)
            player->m_Events.AddEventAtOffset([player]() { player->CastSpell(player, SPELL_ENTROPIC_BUFF, true); }, 1ms);
    }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!settings.enable || !damage || !victim)
            return;

        if (Player* player = victim->ToPlayer())
        {
            if (attacker == victim)
            {
                // Bouncy: falling damage halved (fall damage is self damage dealt while the fall flag is still on)
                if ((player->HasSpell(SPELL_BOUNCY_H) || player->HasSpell(SPELL_BOUNCY_A)) &&
                    player->HasUnitMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR))
                    damage /= 2;
            }
            else if (attacker)
            {
                if (player->HasSpell(SPELL_RUGGED_TENACITY))
                {
                    uint32 const cut = uint32(player->GetStat(STAT_STAMINA) * 0.018f);
                    damage = damage > cut ? damage - cut : 0;
                }
                if (damage && player->HasSpell(SPELL_NOSE_FOR_TROUBLE))
                {
                    bool first = false;
                    ObjectGuid const enemy = attacker->GetCharmerOrOwnerOrSelf()->GetGUID();
                    WithState(player, [&](State& st) { first = st.trouble.insert(enemy).second; });
                    if (first)
                    {
                        uint32 const cut = player->CountPctFromMaxHealth(5);
                        damage = damage > cut ? damage - cut : 0;
                    }
                }
                if (damage && player->HasSpell(SPELL_BRUSH_IT_OFF))
                {
                    float const part = damage * 0.02f / 4.0f;
                    WithState(player, [&](State& st)
                    {
                        for (uint8 i = 0; i < st.heal.size(); ++i)
                            st.heal[(st.healPos + i) % st.heal.size()] += part;
                    });
                }
                if (damage && player->HasSpell(SPELL_EMERGENCY_FAILSAFE))
                {
                    uint32 const health = player->GetHealth();
                    if (health > damage && health - damage < player->CountPctFromMaxHealth(20))
                    {
                        time_t const now = GameTime::GetGameTime().count();
                        bool proc = false;
                        WithState(player, [&](State& st)
                        {
                            if (now >= st.failsafeReady)
                            {
                                st.failsafeReady = now + 150;
                                proc = true;
                            }
                        });
                        if (proc)
                            player->m_Events.AddEventAtOffset([player]()
                            {
                                Heal(player, player->CountPctFromMaxHealth(15), SPELL_EMERGENCY_FAILSAFE);
                            }, 1ms);
                    }
                }
            }
        }

        if (attacker && attacker != victim)
            if (Player* player = attacker->ToPlayer())
                Embrace(player);
    }

    void OnHeal(Unit* healer, Unit* /*receiver*/, uint32& gain) override
    {
        if (settings.enable && gain && healer)
            if (Player* player = healer->ToPlayer())
                Embrace(player);
    }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!settings.enable || !unit || !aura)
            return;
        Player* player = unit->ToPlayer();
        if (!player)
            return;
        SpellInfo const* info = aura->GetSpellInfo();
        if (info->HasAura(SPELL_AURA_MOD_STEALTH))
            player->m_Events.AddEventAtOffset([player]() { UpdateElusiveness(player); }, 1ms);

        // Epicurean: Well Fed doubled
        char const* name = info->SpellName[LOCALE_enUS];
        if (name && !std::strcmp(name, "Well Fed") && player->HasSpell(SPELL_EPICUREAN))
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (AuraEffect* eff = aura->GetEffect(i))
                    if (eff->GetAmount() > 0 && eff->GetAuraType() != SPELL_AURA_PERIODIC_TRIGGER_SPELL)
                        eff->ChangeAmount(eff->GetAmount() * 2);
    }

    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode /*mode*/) override
    {
        if (!settings.enable || !unit || !aurApp)
            return;
        if (Player* player = unit->ToPlayer())
            if (aurApp->GetBase()->GetSpellInfo()->HasAura(SPELL_AURA_MOD_STEALTH))
                player->m_Events.AddEventAtOffset([player]() { UpdateElusiveness(player); }, 1ms);
    }
};

// Savage Blood: poisons, diseases and curses last 10% shorter
class TemucraftRacialBonusesSpells : public AllSpellScript
{
public:
    TemucraftRacialBonusesSpells() : AllSpellScript("TemucraftRacialBonusesSpells", { ALLSPELLHOOK_ON_CALC_MAX_DURATION }) { }

    void OnCalcMaxDuration(Aura const* aura, int32& maxDuration) override
    {
        if (!settings.enable || maxDuration <= 0 || !aura || aura->GetType() != UNIT_AURA_TYPE)
            return;
        SpellInfo const* info = aura->GetSpellInfo();
        if (info->IsPositive() || (info->Dispel != DISPEL_POISON && info->Dispel != DISPEL_DISEASE && info->Dispel != DISPEL_CURSE))
            return;
        if (Player* player = aura->GetUnitOwner()->ToPlayer())
            if (player->HasSpell(SPELL_SAVAGE_BLOOD))
                maxDuration = maxDuration * 9 / 10;
    }
};

// Waste Not, Want Not: a chance of one more piece of meat or fish
class TemucraftRacialBonusesLoot : public MiscScript
{
public:
    TemucraftRacialBonusesLoot() : MiscScript("TemucraftRacialBonusesLoot", { MISCHOOK_ON_AFTER_LOOT_TEMPLATE_PROCESS }) { }

    void OnAfterLootTemplateProcess(Loot* loot, LootTemplate const* /*tab*/, LootStore const& store, Player* owner, bool /*personal*/,
        bool /*noEmptyError*/, uint16 /*lootMode*/) override
    {
        if (!settings.enable || !loot || !owner || !owner->HasSpell(SPELL_WASTE_NOT))
            return;
        if (&store != &LootTemplates_Creature && &store != &LootTemplates_Fishing && &store != &LootTemplates_Skinning)
            return;
        for (LootItem& item : loot->items)
            if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(item.itemid))
                if (proto->Class == ITEM_CLASS_TRADE_GOODS && proto->SubClass == ITEM_SUBCLASS_MEAT &&
                    item.count < proto->GetMaxStackSize() && roll_chance_i(25))
                    ++item.count;
    }
};

void AddTemucraftRacialBonusesScripts()
{
    new TemucraftRacialBonusesWorld();
    new TemucraftRacialBonusesGlobal();
    new TemucraftRacialBonusesPlayer();
    new TemucraftRacialBonusesUnit();
    new TemucraftRacialBonusesSpells();
    new TemucraftRacialBonusesLoot();
}
