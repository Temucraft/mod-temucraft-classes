/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Spell pushback, as one of three rules:
//  0 Off:     damage never pushes a cast or a channel back (as in today's game).
//  1 Wrath:   the 3.3.5 rule, unchanged: 0.5 seconds a hit for casts, 25% of the duration for channels, twice at most.
//  2 Classic: every hit pushes back, with no limit, and a cast loses Classic.CastDelay ms a hit (1 second by default).
// Works from the damage hook, which runs before the core applies its pushback, and from the player update,
// which adds the extra classic delay after the core has decided that a hit pushes the cast back.
// A player can pick his own rule in mod-temucraft-player-settings (.temucraft settings); otherwise the server's applies.

#include "Config.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "TemucraftPlayerSettings.h"
#include "WorldPacket.h"

namespace
{
    struct Settings
    {
        uint32 mode = 1;
        int32 castDelay = 1000;
    } cfg;

    // Reads protected members of Spell through member pointers taken in a derived class (no core change needed).
    struct SpellAccess : Spell
    {
        static uint8 Spell::* Count() { return &SpellAccess::m_delayAtDamageCount; }
        static int32 Spell::* Timer() { return &SpellAccess::m_timer; }
        static int32 Spell::* CastTime() { return &SpellAccess::m_casttime; }
    };

    // Pushback that the 3.3.5 rule does not give: how much of `delay` survives the caster's pushback reductions.
    int32 Reduced(Player* player, Spell* spell, int32 delay)
    {
        int32 reduce = 100;
        player->ApplySpellMod(spell->m_spellInfo->Id, SPELLMOD_NOT_LOSE_CASTING_TIME, reduce, spell);
        reduce += player->GetTotalAuraModifier(SPELL_AURA_REDUCE_PUSHBACK) - 100;
        if (reduce >= 100)
            return 0;
        AddPct(delay, -reduce);
        return delay;
    }

    // Classic only: adds the part of the cast delay over the core's 0.5 seconds, once for each core pushback.
    void ApplyClassicExtra(Player* player, Spell* cast)
    {
        uint8& count = cast->*SpellAccess::Count();
        if (!count)
            return;
        count = 0;
        if (cast->getState() != SPELL_STATE_PREPARING || cfg.castDelay <= 500)
            return;
        if (!(cast->m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_PUSH_BACK) || cast->m_spellInfo->HasAttribute(SPELL_ATTR6_NO_PUSHBACK))
            return;
        int32 extra = Reduced(player, cast, cfg.castDelay - 500);
        int32& timer = cast->*SpellAccess::Timer();
        int32 castTime = cast->*SpellAccess::CastTime();
        if (extra <= 0 || timer >= castTime)
            return;
        extra = std::min(extra, castTime - timer);
        timer += extra;
        WorldPacket data(SMSG_SPELL_DELAYED, 8 + 4);
        data << player->GetPackGUID();
        data << uint32(extra);
        player->SendMessageToSet(&data, true);
    }
}

class TemucraftPushback_Unit : public UnitScript
{
public:
    TemucraftPushback_Unit() : UnitScript("TemucraftPushback_Unit") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!victim || victim == attacker || !damage)
            return;
        if (!victim->IsPlayer())
            return;   // the core only pushes players back
        int32 mode = TemucraftPlayerPushbackMode(victim->ToPlayer(), int32(cfg.mode));
        if (mode == 1)
            return;
        Spell* cast = victim->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        Spell* channel = victim->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        if (mode == 0)
        {
            // the core skips pushback once a spell has been pushed back twice
            if (cast) cast->*SpellAccess::Count() = 2;
            if (channel) channel->*SpellAccess::Count() = 2;
            return;
        }
        // classic: a cast whose count is 1 here was pushed back by the core since the last check, so it gets its extra delay now
        if (cast)
            ApplyClassicExtra(victim->ToPlayer(), cast);
        // the core stops after two pushbacks, so the count goes back to zero before every hit
        if (cast) cast->*SpellAccess::Count() = 0;
        if (channel) channel->*SpellAccess::Count() = 0;
    }
};

class TemucraftPushback_Player : public PlayerScript
{
public:
    TemucraftPushback_Player() : PlayerScript("TemucraftPushback_Player", { PLAYERHOOK_ON_UPDATE }) { }

    // The core pushback runs after the damage hook and only for some hits (not for damage over time, for example).
    // It raises the count from 0 to 1 when it runs, so the extra classic delay is added here, after the core decided.
    void OnPlayerUpdate(Player* player, uint32 /*diff*/) override
    {
        if (TemucraftPlayerPushbackMode(player, int32(cfg.mode)) != 2)
            return;
        if (Spell* cast = player->GetCurrentSpell(CURRENT_GENERIC_SPELL))
            ApplyClassicExtra(player, cast);
    }
};

class TemucraftPushback_World : public WorldScript
{
public:
    TemucraftPushback_World() : WorldScript("TemucraftPushback_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        cfg.mode = std::min<uint32>(sConfigMgr->GetOption<uint32>("Temucraft.Pushback.Mode", 1), 2);
        cfg.castDelay = sConfigMgr->GetOption<int32>("Temucraft.Pushback.Classic.CastDelay", 1000);
    }
};

void AddTemucraftPushbackScripts()
{
    new TemucraftPushback_Unit();
    new TemucraftPushback_Player();
    new TemucraftPushback_World();
}
