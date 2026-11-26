/*
 * mod-temucraft-classes: reagents as a choice.
 *
 * The class spells that need reagents (portals, Arcane Brilliance, the greater blessings, Gift of the Wild,
 * Prayer of Fortitude, warlock stones and summons, Rebirth, Reincarnation...) have them again in Spell.dbc. Each
 * player picks with .temucraft reagents on|off whether to carry them; Temucraft.Reagents.Default is the choice of players who
 * never picked, and Temucraft.Reagents.AllowChoice = 0 makes the default apply to everyone.
 *
 * "Off" works the way the game's own no-reagent effects (glyphs, some talents) do: the player's no-reagent mask
 * (PLAYER_NO_REAGENT_COST_1-3) gets the spell class flags of those spells, and the client and the server both skip
 * the reagent check for spells that match it. Create Healthstone shares its flags with the warlock stones, so it is
 * free with reagents off too. The mask is set again every few seconds, since the core rewrites it when such an
 * effect comes or goes.
 */

#include "Chat.h"
#include "TemucraftModuleText.h"
#include "ChatCommand.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "Tokenize.h"
#include "WorldSession.h"

#include <array>
#include <mutex>
#include <unordered_map>

using namespace Acore::ChatCommands;

namespace
{
    struct ReagentSettings
    {
        bool enable = true;
        bool defaultUse = false;
        bool allowChoice = true;
        std::array<uint32, 3> mask = { 0xB1150FAA, 0xB0801880, 0x00000449 };
    } reagents;

    struct State
    {
        bool chosen = false;
        bool use = false;
        uint32 timer = 0;
    };

    std::mutex sLock;
    std::unordered_map<ObjectGuid::LowType, State> sStates;

    bool UsesReagents(Player* player)
    {
        if (!reagents.allowChoice)
            return reagents.defaultUse;
        std::lock_guard lock(sLock);
        auto itr = sStates.find(player->GetGUID().GetCounter());
        return itr != sStates.end() && itr->second.chosen ? itr->second.use : reagents.defaultUse;
    }

    // The mask the core would set from the player's own no-reagent effects, plus ours when reagents are off
    void ApplyMask(Player* player)
    {
        std::array<uint32, 3> mask = { 0, 0, 0 };
        for (AuraEffect const* effect : player->GetAuraEffectsByType(SPELL_AURA_NO_REAGENT_USE))
        {
            flag96 const& classMask = effect->GetSpellInfo()->Effects[effect->GetEffIndex()].SpellClassMask;
            for (uint8 i = 0; i < 3; ++i)
                mask[i] |= classMask[i];
        }
        if (reagents.enable && !UsesReagents(player))
            for (uint8 i = 0; i < 3; ++i)
                mask[i] |= reagents.mask[i];
        for (uint8 i = 0; i < 3; ++i)
            if (player->GetUInt32Value(PLAYER_NO_REAGENT_COST_1 + i) != mask[i])
                player->SetUInt32Value(PLAYER_NO_REAGENT_COST_1 + i, mask[i]);
    }
}

class TemucraftReagents_Player : public PlayerScript
{
public:
    TemucraftReagents_Player() : PlayerScript("TemucraftReagents_Player", { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT, PLAYERHOOK_ON_UPDATE }) { }

    void OnPlayerLogin(Player* player) override
    {
        State state;
        if (QueryResult result = CharacterDatabase.Query("SELECT `value` FROM `mod_temucraft_character_setting` WHERE `guid` = {} AND `setting` = 'reagents'",
            player->GetGUID().GetCounter()))
        {
            state.chosen = true;
            state.use = result->Fetch()[0].Get<int32>() != 0;
        }
        {
            std::lock_guard lock(sLock);
            sStates[player->GetGUID().GetCounter()] = state;
        }
        ApplyMask(player);
        if (reagents.enable && reagents.allowChoice && !state.chosen)
            ChatHandler(player->GetSession()).PSendSysMessage(TemucraftModuleText("mod-temucraft-classes", player->GetSession(), 1, "|cffffcc00Reagents:|r your class spells {}. Type .temucraft reagents on or "
                ".temucraft reagents off to choose."), reagents.defaultUse ? "use their reagents" : "need no reagents");
    }

    void OnPlayerLogout(Player* player) override
    {
        std::lock_guard lock(sLock);
        sStates.erase(player->GetGUID().GetCounter());
    }

    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        {
            std::lock_guard lock(sLock);
            State& state = sStates[player->GetGUID().GetCounter()];
            if (state.timer > diff)
            {
                state.timer -= diff;
                return;
            }
            state.timer = 5000;
        }
        ApplyMask(player);
    }
};

class TemucraftReagents_Commands : public CommandScript
{
public:
    TemucraftReagents_Commands() : CommandScript("TemucraftReagents_Commands") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "temucraft reagents", HandleReagents, 102050, Console::No },
        };
        return commandTable;
    }

    static bool HandleReagents(ChatHandler* handler, Optional<std::string> choice)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!reagents.enable)
        {
            handler->SendSysMessage(TemucraftModuleText("mod-temucraft-classes", handler, 2, "Reagents can't be chosen on this server."));
            return true;
        }
        if (!choice || (*choice != "on" && *choice != "off"))
        {
            handler->PSendSysMessage(TemucraftModuleText("mod-temucraft-classes", handler, 3, "|cffffcc00Reagents:|r your class spells {}.{}"), UsesReagents(player) ? "use their reagents" :
                "need no reagents", reagents.allowChoice ? " .temucraft reagents on or .temucraft reagents off changes it." : "");
            return true;
        }
        if (!reagents.allowChoice)
        {
            handler->SendSysMessage(TemucraftModuleText("mod-temucraft-classes", handler, 4, "This server decides that for everyone."));
            return true;
        }
        bool use = *choice == "on";
        {
            std::lock_guard lock(sLock);
            State& state = sStates[player->GetGUID().GetCounter()];
            state.chosen = true;
            state.use = use;
        }
        CharacterDatabase.Execute("REPLACE INTO `mod_temucraft_character_setting` (`guid`, `setting`, `value`) VALUES ({}, 'reagents', {})",
            player->GetGUID().GetCounter(), use ? 1 : 0);
        ApplyMask(player);
        handler->PSendSysMessage(TemucraftModuleText("mod-temucraft-classes", handler, 5, "|cffffcc00Reagents:|r {}"), use ? "on. Your class spells use their reagents (portals, greater "
            "blessings, Arcane Brilliance, soul shards...)." : "off. Your class spells need no reagents.");
        return true;
    }
};

class TemucraftReagents_World : public WorldScript
{
public:
    TemucraftReagents_World() : WorldScript("TemucraftReagents_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        reagents.enable = sConfigMgr->GetOption<bool>("Temucraft.Reagents.Enable", true);
        reagents.defaultUse = sConfigMgr->GetOption<bool>("Temucraft.Reagents.Default", false);
        reagents.allowChoice = sConfigMgr->GetOption<bool>("Temucraft.Reagents.AllowChoice", true);
    }
};

// a deleted character's reagent choice (mod_temucraft_character_setting 'reagents') goes with it
// OnPlayerDeleteFromDB: only when the character is really removed (not a CharDelete.Method = 1 unlink, which can be
// restored), and in the same transaction as the core's own deletes.
class TemucraftReagents_Delete : public PlayerScript
{
public:
    TemucraftReagents_Delete() : PlayerScript("TemucraftReagents_Delete", { PLAYERHOOK_ON_DELETE_FROM_DB }) { }

    void OnPlayerDeleteFromDB(CharacterDatabaseTransaction trans, uint32 guid) override
    {
        trans->Append("DELETE FROM `mod_temucraft_character_setting` WHERE `guid` = {} AND `setting` = 'reagents'", guid);
    }
};

void AddTemucraftReagentsScripts()
{
    new TemucraftReagents_Player();
    new TemucraftReagents_Commands();
    new TemucraftReagents_World();
    new TemucraftReagents_Delete();
}
