/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Small class changes: other cooldowns for chosen spells, soul bags that sometimes keep a soul shard, infernal stone or
// demonic figurine, bows for night elf priests, any weapon (and armor) for every race and class, raid-wide auras and
// shouts, hour-long class buffs and hunter pets that are always happy.

#include "TemucraftLocked.h"
#include "Bag.h"
#include "Chat.h"
#include "Config.h"
#include "DBCStores.h"
#include "Pet.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "TemucraftModuleText.h"
#include <algorithm>
#include <list>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    constexpr char const* MODULE = "mod-temucraft-class-tweaks";

    // Reagents a soul bag holds (and may keep): Soul Shard, Infernal Stone, Demonic Figurine.
    constexpr uint32 SOUL_BAG_REAGENTS[] = { 6265, 5565, 16583 };

    // Night elf priests: Bows proficiency and the ranged Shoot.
    constexpr uint32 SPELL_BOWS  = 264;
    constexpr uint32 SPELL_SHOOT = 3018;

    struct Settings
    {
        bool enable = true;
        std::unordered_map<uint32, uint32> cooldowns;   // spell id -> new cooldown in ms
        bool soulBags = true;
        int32 soulBagMinChance = 5;
        int32 soulBagMaxChance = 25;
        bool nightElfPriestBows = true;
        bool anyWeapon = true;
        bool anyArmor = false;
        bool raidWideAuras = true;
        uint32 buffMinutes = 60;
        uint32 tranquilityTargets = 5;
        bool happyPets = true;
        std::unordered_set<uint32> noDaze;              // daze spells that do nothing
        bool smartTranquility = true;
        std::vector<std::pair<uint8, uint32>> parachute;   // from level -> cooldown in ms, by level
    } settings;

    // Parachute (taught by every class trainer from level 10): its cooldown shrinks as the player levels.
    constexpr uint32 SPELL_PARACHUTE = 55001;

    // The healing ticks of Tranquility, ranks 1-7.
    std::unordered_set<uint32> const TRANQUILITY_HEALS = { 44203, 44205, 44206, 44207, 44208, 48444, 48445 };

    // Weapon and armor skill lines (SkillLine.dbc categories 6 and 8, Defense and Unarmed left out).
    std::unordered_set<uint32> const WEAPON_SKILLS = { 43, 44, 45, 46, 54, 55, 118, 136, 160, 172, 173, 176, 226, 228, 229, 473 };
    std::unordered_set<uint32> const ARMOR_SKILLS  = { 293, 413, 414, 415, 433 };

    // What a newly learned proficiency also needs to be usable. Ranged weapons: the attack the server gives with the
    // skill (Shoot for warriors and rogues, or another module's), else Shoot; shields: Block.
    std::unordered_map<uint32, uint32> const PROFICIENCY_SKILL = {
        { 264, 45 }, { 266, 46 }, { 5011, 226 }, { 5009, 228 }, { 2567, 176 }, { 9116, 433 },
    };
    constexpr uint32 SPELL_BLOCK = 107;

    // The stock masks of the DBC rows the module opens up, to put them back when the setting is turned off.
    struct SavedMasks { uint32* race; uint32* cls; uint32 raceValue; uint32 classValue; };
    std::vector<SavedMasks> savedMasks;

    void OpenSkills()
    {
        for (SavedMasks const& m : savedMasks)
        {
            *m.race = m.raceValue;
            *m.cls = m.classValue;
        }
        savedMasks.clear();
        if (!settings.enable)
            return;

        // Who may have the skill (SkillRaceClassInfo): the weapon / armor skills that are opened, and Bows for night
        // elf priests (trainers still teach bows only to those the spell rows allow, unless AnyWeapon is on).
        auto mayHave = [](uint32 skill)
        {
            return (settings.anyWeapon && WEAPON_SKILLS.count(skill)) || (settings.anyArmor && ARMOR_SKILLS.count(skill))
                || (settings.nightElfPriestBows && skill == 45);
        };
        // Who may learn the proficiency itself (SkillLineAbility rows that come with the skill). Rows learned as the
        // skill rises (Shoot, Auto Shot, Throw: the attacks) keep their classes, so each class keeps its own attack.
        auto mayLearn = [](uint32 skill)
        {
            return (settings.anyWeapon && WEAPON_SKILLS.count(skill)) || (settings.anyArmor && ARMOR_SKILLS.count(skill));
        };
        auto open = [](uint32& race, uint32& cls)
        {
            savedMasks.push_back({ &race, &cls, race, cls });
            race = 0;
            cls = 0;
        };

        // The rows any race/class pair finds (the core looks them up per skill), each opened once.
        std::unordered_set<SkillRaceClassInfoEntry const*> rows;
        for (std::unordered_set<uint32> const* skills : { &WEAPON_SKILLS, &ARMOR_SKILLS })
            for (uint32 skill : *skills)
                if (mayHave(skill))
                    for (uint8 race = 1; race <= 32; ++race)
                        for (uint8 cls = 1; cls < MAX_CLASSES; ++cls)
                            if (SkillRaceClassInfoEntry const* e = GetSkillRaceClassInfo(skill, race, cls))
                                rows.insert(e);
        for (SkillRaceClassInfoEntry const* e : rows)
        {
            SkillRaceClassInfoEntry* row = const_cast<SkillRaceClassInfoEntry*>(e);
            open(row->RaceMask, row->ClassMask);
        }
        for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
            if (SkillLineAbilityEntry const* e = sSkillLineAbilityStore.LookupEntry(i))
                if (e->AcquireMethod == SKILL_LINE_ABILITY_LEARNED_ON_SKILL_LEARN && mayLearn(e->SkillLine))
                {
                    SkillLineAbilityEntry* row = const_cast<SkillLineAbilityEntry*>(e);
                    open(row->RaceMask, row->ClassMask);
                }
        if (!savedMasks.empty())
            LOG_INFO("server.loading", "{}: {} weapon/armor skill rows open to every race and class.", MODULE, savedMasks.size());
    }

    // true if the player knows an attack that comes with this skill (a row learned as the skill rises)
    bool HasSkillAttack(Player* player, uint32 skill)
    {
        for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
            if (SkillLineAbilityEntry const* e = sSkillLineAbilityStore.LookupEntry(i))
                if (e->SkillLine == skill && e->AcquireMethod == SKILL_LINE_ABILITY_LEARNED_ON_SKILL_VALUE && player->HasSpell(e->Spell))
                    return true;
        return false;
    }

    // A ranged weapon without an attack (or a shield without Block) is no use: give what's missing.
    void CompleteProficiency(Player* player, uint32 spellId)
    {
        auto itr = PROFICIENCY_SKILL.find(spellId);
        if (itr == PROFICIENCY_SKILL.end())
            return;
        uint32 const skill = itr->second;
        if (skill == 433)
        {
            if (!player->HasSpell(SPELL_BLOCK))
                player->learnSpell(SPELL_BLOCK);
            return;
        }
        if (skill == 228 || skill == 176)   // wands (Shoot) and thrown (Throw) come with their skill
            return;
        if (player->HasSpell(75) || player->HasSpell(SPELL_SHOOT) || HasSkillAttack(player, skill))
            return;
        player->learnSpell(SPELL_SHOOT);
    }

    bool IsClassFamily(uint32 family)
    {
        switch (family)
        {
            case SPELLFAMILY_MAGE: case SPELLFAMILY_WARRIOR: case SPELLFAMILY_WARLOCK: case SPELLFAMILY_PRIEST:
            case SPELLFAMILY_DRUID: case SPELLFAMILY_ROGUE: case SPELLFAMILY_HUNTER: case SPELLFAMILY_PALADIN:
            case SPELLFAMILY_SHAMAN: case SPELLFAMILY_DEATHKNIGHT:
                return true;
            default:
                return false;
        }
    }

    bool IsFriendlyTarget(uint32 target)
    {
        switch (target)
        {
            case 0: case TARGET_UNIT_CASTER: case TARGET_UNIT_PET: case TARGET_UNIT_TARGET_ALLY: case TARGET_UNIT_MASTER:
            case TARGET_UNIT_CASTER_AREA_PARTY: case TARGET_UNIT_SRC_AREA_ALLY: case TARGET_UNIT_SRC_AREA_PARTY:
            case TARGET_UNIT_TARGET_PARTY: case TARGET_UNIT_LASTTARGET_AREA_PARTY: case TARGET_UNIT_CASTER_AREA_RAID:
            case TARGET_UNIT_TARGET_RAID: case TARGET_UNIT_TARGET_AREA_RAID_CLASS:
                return true;
            default:
                return false;
        }
    }

    SpellDurationEntry const* DurationOf(int32 ms)
    {
        for (uint32 i = 0; i < sSpellDurationStore.GetNumRows(); ++i)
            if (SpellDurationEntry const* d = sSpellDurationStore.LookupEntry(i))
                if (d->Duration[0] == ms && d->Duration[2] == ms)
                    return d;
        return nullptr;
    }

    // Spell data changes, made once while the server loads its spells (a restart applies a changed setting).
    void TuneSpell(SpellInfo* spell)
    {
        if (!settings.enable)
            return;

        // No daze: the listed spells keep their name but do nothing (no slow, no dismount).
        if (settings.noDaze.count(spell->Id))
        {
            for (SpellEffectInfo& effect : spell->_GetEffects())
            {
                effect.Effect = SpellEffects(0);
                effect.ApplyAuraName = SPELL_AURA_NONE;
            }
            return;
        }

        // Tranquility heals the raid: the 5 most injured within reach every tick (see spell script below).
        if (settings.smartTranquility && TRANQUILITY_HEALS.count(spell->Id))
            for (SpellEffectInfo& effect : spell->_GetEffects())
                if (effect.Effect == SPELL_EFFECT_HEAL && effect.TargetA.GetTarget() == TARGET_UNIT_CASTER_AREA_PARTY)
                    effect.TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER_AREA_RAID);

        if (!IsClassFamily(spell->SpellFamilyName))
            return;

        // Party auras, shouts, totems and party-wide talent auras reach the whole raid.
        if (settings.raidWideAuras)
            for (SpellEffectInfo& effect : spell->_GetEffects())
            {
                if (effect.Effect == SPELL_EFFECT_APPLY_AREA_AURA_PARTY)
                    effect.Effect = SPELL_EFFECT_APPLY_AREA_AURA_RAID;
                // buffs cast on the caster's party (older shout ranks, Furious Howl...); heals stay party-wide
                else if (effect.Effect == SPELL_EFFECT_APPLY_AURA && effect.TargetA.GetTarget() == TARGET_UNIT_CASTER_AREA_PARTY)
                    effect.TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER_AREA_RAID);
            }

        // Friendly class buffs of 10 or 30 minutes last BuffMinutes.
        if (settings.buffMinutes && spell->DurationEntry && !spell->IsPassive())
        {
            int32 const d = spell->DurationEntry->Duration[0];
            if ((d == 10 * MINUTE * IN_MILLISECONDS || d == 30 * MINUTE * IN_MILLISECONDS) && spell->DurationEntry->Duration[2] == d)
            {
                bool buff = false, friendly = true;
                for (SpellEffectInfo const& effect : spell->GetEffects())
                {
                    if (!effect.Effect)
                        continue;
                    if (effect.ApplyAuraName != SPELL_AURA_NONE)
                        buff = true;
                    if (!IsFriendlyTarget(effect.TargetA.GetTarget()) || !IsFriendlyTarget(effect.TargetB.GetTarget()))
                        friendly = false;
                }
                static SpellDurationEntry const* longer = nullptr;
                static uint32 longerMinutes = 0;
                if (longerMinutes != settings.buffMinutes)
                {
                    longer = DurationOf(int32(settings.buffMinutes * MINUTE * IN_MILLISECONDS));
                    longerMinutes = settings.buffMinutes;
                    if (!longer)
                        LOG_ERROR("server.loading", "{}: SpellDuration.dbc has no {} minute duration; class buffs keep theirs.", MODULE, settings.buffMinutes);
                }
                if (buff && friendly && longer && longer->Duration[0] > d)
                    spell->DurationEntry = longer;
            }
        }
    }

    void LoadSettings()
    {
        settings.enable = sConfigMgr->GetOption<bool>("Temucraft.ClassTweaks.Enable", true);
        settings.soulBags = sConfigMgr->GetOption<bool>("Temucraft.ClassTweaks.SoulBags", true);
        settings.soulBagMinChance = std::clamp(sConfigMgr->GetOption<int32>("Temucraft.ClassTweaks.SoulBags.MinChance", 5), 0, 100);
        settings.soulBagMaxChance = std::clamp(sConfigMgr->GetOption<int32>("Temucraft.ClassTweaks.SoulBags.MaxChance", 25), settings.soulBagMinChance, 100);
        settings.nightElfPriestBows = sConfigMgr->GetOption<bool>("Temucraft.ClassTweaks.NightElfPriestBows", true);
        settings.anyWeapon = sConfigMgr->GetOption<bool>("Temucraft.ClassTweaks.AnyWeapon", true);
        settings.anyArmor = sConfigMgr->GetOption<bool>("Temucraft.ClassTweaks.AnyArmor", true);
        settings.raidWideAuras = sConfigMgr->GetOption<bool>("Temucraft.ClassTweaks.RaidWideAuras", true);
        settings.buffMinutes = sConfigMgr->GetOption<uint32>("Temucraft.ClassTweaks.BuffMinutes", 60);
        settings.happyPets = sConfigMgr->GetOption<bool>("Temucraft.ClassTweaks.HappyPets", true);
        settings.smartTranquility = sConfigMgr->GetOption<bool>("Temucraft.ClassTweaks.SmartTranquility", true);
        settings.tranquilityTargets = std::max<uint32>(1, sConfigMgr->GetOption<uint32>("Temucraft.ClassTweaks.SmartTranquility.Targets", 5));
        settings.noDaze.clear();
        {
            std::string daze = sConfigMgr->GetOption<std::string>("Temucraft.ClassTweaks.NoDaze", "1604,15571,35955,51372,135016,1201861");
            std::replace(daze.begin(), daze.end(), ',', ' ');
            std::istringstream in(daze);
            std::string id;
            while (in >> id)
                if (uint32 spellId = uint32(std::strtoul(id.c_str(), nullptr, 10)))
                    settings.noDaze.insert(spellId);
        }

        settings.parachute.clear();
        std::string steps = sConfigMgr->GetOption<std::string>("Temucraft.ClassTweaks.Parachute", "10:600 20:480 30:360 40:300 50:240 60:180 70:150 80:120");
        std::replace(steps.begin(), steps.end(), ',', ' ');
        std::istringstream stepIn(steps);
        std::string step;
        while (stepIn >> step)
        {
            std::size_t colon = step.find(':');
            if (colon == std::string::npos)
                continue;
            uint32 level = uint32(std::strtoul(step.substr(0, colon).c_str(), nullptr, 10));
            double seconds = std::strtod(step.substr(colon + 1).c_str(), nullptr);
            if (!level || level > 255 || seconds < 0.0)
                continue;
            settings.parachute.emplace_back(uint8(level), uint32(seconds * IN_MILLISECONDS));
        }
        std::sort(settings.parachute.begin(), settings.parachute.end());

        // "58984:60 12345:90": spell id and cooldown in seconds, separated by spaces or commas.
        settings.cooldowns.clear();
        std::string list = sConfigMgr->GetOption<std::string>("Temucraft.ClassTweaks.Cooldowns", "");
        std::replace(list.begin(), list.end(), ',', ' ');
        std::istringstream in(list);
        std::string pair;
        while (in >> pair)
        {
            std::size_t colon = pair.find(':');
            if (colon == std::string::npos)
                continue;
            uint32 spellId = uint32(std::strtoul(pair.substr(0, colon).c_str(), nullptr, 10));
            double seconds = std::strtod(pair.substr(colon + 1).c_str(), nullptr);
            if (!spellId || seconds < 0.0)
            {
                LOG_ERROR("server.loading", "{}: Temucraft.ClassTweaks.Cooldowns has a bad entry '{}', skipped.", MODULE, pair);
                continue;
            }
            settings.cooldowns[spellId] = uint32(seconds * IN_MILLISECONDS);
        }
    }

    // Chance a player's soul bags keep a reagent: the biggest soul bag counts, MinChance for 16 slots or fewer up to
    // MaxChance for 32 slots or more. 0 without a soul bag.
    int32 SoulBagChance(Player* player)
    {
        uint32 slots = 0;
        for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                if (Bag* bag = item->ToBag())
                    if (bag->GetTemplate()->BagFamily & BAG_FAMILY_MASK_SOUL_SHARDS)
                        slots = std::max(slots, bag->GetBagSize());
        if (!slots)
            return 0;
        int32 const extra = std::clamp(int32(slots) - 16, 0, 16);
        return settings.soulBagMinChance + (settings.soulBagMaxChance - settings.soulBagMinChance) * extra / 16;
    }

    void ApplyCooldown(Player* player, SpellInfo const* spellInfo)
    {
        uint32 wanted = 0;
        if (spellInfo->Id == SPELL_PARACHUTE && !settings.parachute.empty())
        {
            for (auto const& [level, ms] : settings.parachute)
                if (player->GetLevel() >= level)
                    wanted = ms;
            if (!wanted)
                wanted = settings.parachute.front().second;
        }
        else
        {
            auto itr = settings.cooldowns.find(spellInfo->Id);
            if (itr == settings.cooldowns.end())
                return;
            wanted = itr->second;
        }
        uint32 const stock = spellInfo->RecoveryTime ? spellInfo->RecoveryTime : spellInfo->CategoryRecoveryTime;
        int32 const delta = int32(wanted) - int32(stock);
        if (!stock || !delta)
            return;
        uint32 const spellId = spellInfo->Id;
        // The cooldown is set (and the client starts its own) during the cast; change both a moment later.
        player->m_Events.AddEventAtOffset([player, spellId, delta]()
        {
            if (player->HasSpellCooldown(spellId))
                player->ModifySpellCooldown(spellId, delta);
        }, 300ms);
    }

    void SoulBagKeep(Player* player, SpellInfo const* spellInfo)
    {
        for (uint8 i = 0; i < MAX_SPELL_REAGENTS; ++i)
        {
            if (spellInfo->Reagent[i] <= 0 || !spellInfo->ReagentCount[i])
                continue;
            uint32 const itemId = uint32(spellInfo->Reagent[i]);
            if (std::find(std::begin(SOUL_BAG_REAGENTS), std::end(SOUL_BAG_REAGENTS), itemId) == std::end(SOUL_BAG_REAGENTS))
                continue;
            int32 const chance = SoulBagChance(player);
            if (chance <= 0)
                return;
            uint32 const before = player->GetItemCount(itemId);
            uint32 const used = spellInfo->ReagentCount[i];
            // Reagents are taken after this hook; look again once the cast is done. Nothing taken (reagents off,
            // requirement turned off) means nothing to keep.
            player->m_Events.AddEventAtOffset([player, itemId, before, used, chance]()
            {
                uint32 const now = player->GetItemCount(itemId);
                if (now >= before || !roll_chance_i(chance))
                    return;
                uint32 const back = std::min(before - now, used);
                if (!player->AddItem(itemId, back))
                    return;
                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
                ChatHandler(player->GetSession()).PSendSysMessage(TemucraftModuleText(MODULE, player, 1,
                    "|cff9482c9Your soul bag kept the {}.|r"), proto ? proto->Name1 : std::string("reagent"));
            }, 100ms);
        }
    }
}

class TemucraftClassTweaksWorld : public WorldScript
{
public:
    TemucraftClassTweaksWorld() : WorldScript("TemucraftClassTweaksWorld", { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_STARTUP }) { }

    void OnAfterConfigLoad(bool reload) override
    {
        LoadSettings();
        if (reload)
            OpenSkills();   // at startup the DBCs aren't loaded yet: OnStartup does it
    }

    void OnStartup() override
    {
        OpenSkills();
    }
};

class TemucraftClassTweaksSpells : public GlobalScript
{
public:
    TemucraftClassTweaksSpells() : GlobalScript("TemucraftClassTweaksSpells", { GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR }) { }

    void OnLoadSpellCustomAttr(SpellInfo* spell) override
    {
        TuneSpell(spell);
    }
};

class TemucraftClassTweaksPlayer : public PlayerScript
{
public:
    TemucraftClassTweaksPlayer() : PlayerScript("TemucraftClassTweaksPlayer", { PLAYERHOOK_ON_SPELL_CAST, PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LEARN_SPELL, PLAYERHOOK_ON_UPDATE, PLAYERHOOK_ON_LOGOUT }) { }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!settings.enable || !player || !spell || spell->IsTriggered())
            return;
        SpellInfo const* spellInfo = spell->GetSpellInfo();
        if (!spellInfo)
            return;
        if (!spell->m_CastItem)
            ApplyCooldown(player, spellInfo);
        if (settings.soulBags)
            SoulBagKeep(player, spellInfo);
    }

    void OnPlayerLogin(Player* player) override
    {
        if (!settings.enable || !settings.nightElfPriestBows)
            return;
        if (player->getRace() != RACE_NIGHTELF || player->getClass() != CLASS_PRIEST)
            return;
        if (!player->HasSpell(SPELL_BOWS))
            player->learnSpell(SPELL_BOWS);
        CompleteProficiency(player, SPELL_BOWS);
    }

    // A proficiency outside the class: also teach what makes it usable.
    void OnPlayerLearnSpell(Player* player, uint32 spellId) override
    {
        if (settings.enable && (settings.anyWeapon || settings.anyArmor))
            CompleteProficiency(player, spellId);
    }

    // Hunter pets stay happy: no feeding needed.
    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        if (!settings.enable || !settings.happyPets || player->getClass() != CLASS_HUNTER)
            return;
        uint32& timer = petTimers[player->GetGUID().GetCounter()];
        timer += diff;
        if (timer < 5000)
            return;
        timer = 0;
        if (Pet* pet = player->GetPet())
            if (pet->getPetType() == HUNTER_PET && pet->GetPower(POWER_HAPPINESS) < pet->GetMaxPower(POWER_HAPPINESS))
                pet->SetPower(POWER_HAPPINESS, pet->GetMaxPower(POWER_HAPPINESS));
    }

    void OnPlayerLogout(Player* player) override
    {
        petTimers.erase(player->GetGUID().GetCounter());
    }

private:
    Temucraft::LockedMap<uint32, uint32> petTimers;
};

// Tranquility: of the raid members in reach, heal the most injured few (the party-wide spell made raid-wide).
class spell_temucraft_tranquility : public SpellScript
{
    PrepareSpellScript(spell_temucraft_tranquility);

    void PickTargets(std::list<WorldObject*>& targets)
    {
        if (!settings.enable || !settings.smartTranquility)
            return;
        targets.remove_if([](WorldObject* o) { Unit* u = o->ToUnit(); return !u || !u->IsAlive(); });
        if (targets.size() <= settings.tranquilityTargets)
            return;
        targets.sort([](WorldObject* a, WorldObject* b) { return a->ToUnit()->GetHealthPct() < b->ToUnit()->GetHealthPct(); });
        targets.resize(settings.tranquilityTargets);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_temucraft_tranquility::PickTargets, EFFECT_ALL, TARGET_UNIT_CASTER_AREA_RAID);
    }
};

void AddTemucraftClassTweaksScripts()
{
    RegisterSpellScript(spell_temucraft_tranquility);
    new TemucraftClassTweaksWorld();
    new TemucraftClassTweaksPlayer();
    new TemucraftClassTweaksSpells();
}
