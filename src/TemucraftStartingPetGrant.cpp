/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Characters made before the starting pets get theirs once, on their next login:
//  - Warlocks who don't know Summon Imp (688) learn it. New warlocks get it from playercreateinfo_cast_spell
//    (mod_temucraft_classes_starting_pets.sql).
//  - Hunters with no pet at all (no current pet and no row in character_pet, stable included) get the pet a new
//    hunter of their race gets from modpak F-128 in mod-worgoblin-high-elf (player_starting_pet: the race's row, else
//    its kin's, else its home race's), made the same way, plus Call Pet, Revive Pet, Dismiss Pet and Feed Pet.
//    New hunters get it at level 1, so every level qualifies.
// Each character is written to mod_temucraft_classes_pet_grant once it is handled, also when it already had a pet,
// so this never runs twice for it. New characters are marked on their first login without being touched.
// Bots are skipped (no open socket, the same test as RangedAutoAttack.cpp); they are not marked, so a bot character
// that a person logs in later is still handled.

#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "Player.h"
#include "RaceMgr.h"
#include "ScriptMgr.h"
#include "TemucraftModuleText.h"
#include "WorldSession.h"
#include <map>

namespace
{
    char const* const TEXT_MODULE = "mod-temucraft-classes";

    enum StartingPetGrantValues : uint32
    {
        SPELL_SUMMON_IMP          = 688,
        SPELL_TAME_BEAST_EFFECT   = 13481,
        SPELL_CALL_PET            = 883,
        SPELL_REVIVE_PET          = 982,
        SPELL_DISMISS_PET         = 2641,
        SPELL_FEED_PET            = 6991,
        FULL_HAPPINESS            = 1048000,

        TEXT_GOT_IMP              = 6,
        TEXT_GOT_PET              = 7
    };

    // Used only when player_starting_pet has nothing usable for the race, its kin, its home race or Human (F-128 off
    // or a broken row): stock tameable level 1 beasts (Diseased Young Wolf, Young Thistle Boar, Mottled Boar).
    uint32 const FALLBACK_BEASTS[] = { 299, 1984, 3098 };

    bool sEnable = true;

    bool IsBot(Player* player)
    {
        WorldSession* session = player->GetSession();
        return !session || session->IsSocketClosed();
    }

    bool IsUsableBeast(uint32 entry)
    {
        CreatureTemplate const* creatureTemplate = entry ? sObjectMgr->GetCreatureTemplate(entry) : nullptr;
        // Not exotic: Pet::LoadPetFromDB refuses those for hunters without Beast Mastery
        return creatureTemplate && creatureTemplate->IsTameable(false);
    }

    // The same choice as F-128's GetStartingPet (race, kin, home race), then Human's row, then a stock beast
    uint32 GetStartingPet(uint8 race)
    {
        std::map<uint8, uint32> pets;
        if (QueryResult result = WorldDatabase.Query("SELECT `race`, `creature_entry` FROM `player_starting_pet`"))
        {
            do
            {
                Field* fields = result->Fetch();
                pets[fields[0].Get<uint8>()] = fields[1].Get<uint32>();
            } while (result->NextRow());
        }

        for (uint8 candidate : { race, sRaceMgr->GetKinRace(race), sRaceMgr->GetHomeRace(race), uint8(RACE_HUMAN) })
        {
            auto const itr = pets.find(candidate);
            if (itr != pets.end() && IsUsableBeast(itr->second))
                return itr->second;
        }

        for (uint32 entry : FALLBACK_BEASTS)
            if (IsUsableBeast(entry))
            {
                LOG_WARN("module", "TemucraftStartingPetGrant: no usable starting pet for race {}, using creature {}.", race, entry);
                return entry;
            }

        LOG_ERROR("module", "TemucraftStartingPetGrant: no usable starting pet for race {}.", race);
        return 0;
    }

    bool HasAnyPet(Player* player)
    {
        if (player->GetPet() || player->IsExistPet())
            return true;
        if (PetStable const* stable = player->GetPetStable())
        {
            if (stable->CurrentPet || !stable->UnslottedPets.empty())
                return true;
            for (auto const& slot : stable->StabledPets)
                if (slot)
                    return true;
        }
        return CharacterDatabase.Query("SELECT 1 FROM `character_pet` WHERE `owner` = {} LIMIT 1", player->GetGUID().GetCounter()) != nullptr;
    }

    // Made as F-128 makes a new hunter's pet (from Gozzim's mod-starting-pet)
    Pet* GiveStartingPet(Player* player, uint32 entry)
    {
        Pet* pet = player->CreateTamedPetFrom(entry, SPELL_TAME_BEAST_EFFECT);
        if (!pet)
            return nullptr;

        pet->SetPower(POWER_HAPPINESS, FULL_HAPPINESS);
        pet->GetMap()->AddToMap(pet->ToCreature());
        player->SetMinion(pet, true);
        pet->InitTalentForLevel();
        pet->SavePetToDB(PET_SAVE_AS_CURRENT);
        player->PetSpellInitialize();
        return pet;
    }

    void MarkDone(Player* player)
    {
        CharacterDatabase.Execute("INSERT IGNORE INTO `mod_temucraft_classes_pet_grant` (`guid`) VALUES ({})", player->GetGUID().GetCounter());
    }
}

class TemucraftStartingPetGrant_World : public WorldScript
{
public:
    TemucraftStartingPetGrant_World() : WorldScript("TemucraftStartingPetGrant_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sEnable = sConfigMgr->GetOption<bool>("Temucraft.ClassChanges.StartingPetGrant.Enable", true);
    }
};

class TemucraftStartingPetGrant_Player : public PlayerScript
{
public:
    TemucraftStartingPetGrant_Player() : PlayerScript("TemucraftStartingPetGrant_Player", { PLAYERHOOK_ON_LOGIN }) { }

    void OnPlayerLogin(Player* player) override
    {
        if (!sEnable)
            return;

        uint8 const playerClass = player->getClass();
        if (playerClass != CLASS_WARLOCK && playerClass != CLASS_HUNTER)
            return;

        if (IsBot(player))
            return;

        uint32 const guid = player->GetGUID().GetCounter();
        if (CharacterDatabase.Query("SELECT 1 FROM `mod_temucraft_classes_pet_grant` WHERE `guid` = {}", guid))
            return;

        // A new character already got its pet when it was made (OnPlayerFirstLogin runs after this hook)
        if (player->HasAtLoginFlag(AT_LOGIN_FIRST))
        {
            MarkDone(player);
            return;
        }

        ChatHandler chat(player->GetSession());

        if (playerClass == CLASS_WARLOCK)
        {
            if (!player->HasSpell(SPELL_SUMMON_IMP))
            {
                player->learnSpell(SPELL_SUMMON_IMP);
                chat.SendSysMessage(TemucraftModuleText(TEXT_MODULE, player, TEXT_GOT_IMP,
                    "|cffffcc00Temucraft:|r you learned Summon Imp, the first demon every warlock now starts with."));
            }
            MarkDone(player);
            return;
        }

        // Hunters: only when new hunters get a pet too (F-128's switch); not marked, so turning it on later still works
        if (!sConfigMgr->GetOption<bool>("Worgoblin.StartingPet.Enable", true))
            return;

        if (HasAnyPet(player))
        {
            MarkDone(player);
            return;
        }

        // Try again next login rather than making a pet in a flight or on a dead hunter
        if (!player->IsInWorld() || !player->IsAlive() || player->IsInFlight())
            return;

        uint32 const entry = GetStartingPet(player->getRace(true));
        if (!entry)
            return;

        for (uint32 spell : { SPELL_CALL_PET, SPELL_REVIVE_PET, SPELL_DISMISS_PET, SPELL_FEED_PET })
            if (!player->HasSpell(spell))
                player->learnSpell(spell);

        Pet* pet = GiveStartingPet(player, entry);
        if (!pet)
        {
            LOG_ERROR("module", "TemucraftStartingPetGrant: could not make creature {} the pet of {} ({}).", entry, player->GetName(), guid);
            return;
        }

        MarkDone(player);
        chat.PSendSysMessage(TemucraftModuleText(TEXT_MODULE, player, TEXT_GOT_PET,
            "|cffffcc00Temucraft:|r every hunter now starts with a pet, so {} has joined you. Call Pet, Dismiss Pet, Revive Pet and Feed Pet are in your spellbook."),
            pet->GetName());
    }
};

void AddTemucraftStartingPetGrantScripts()
{
    new TemucraftStartingPetGrant_World();
    new TemucraftStartingPetGrant_Player();
}
