/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Talent points from an earlier level: the first point at Temucraft.Talents.FirstLevel (2) and one more every level
// after it, instead of the first at level 10. The core then multiplies by Rate.Talent (1.5 on Temucraft: roughly two
// full trees at level 80). Death knights in the Ebon Hold keep the core's quest-based points until they leave.
// The client shows the talent window from level 1 (SHOW_TALENT_LEVEL in patch-A's Constants.lua).

#include "Config.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace
{
    uint32 sFirstLevel = 2;
}

class TemucraftTalentPoints_Player : public PlayerScript
{
public:
    TemucraftTalentPoints_Player() : PlayerScript("TemucraftTalentPoints_Player", { PLAYERHOOK_ON_CALCULATE_TALENTS_POINTS }) { }

    void OnPlayerCalculateTalentsPoints(Player const* player, uint32& talentPointsForLevel) override
    {
        if (!sFirstLevel || sFirstLevel >= 10)
            return;   // off, or not earlier than the core
        if (player->IsClass(CLASS_DEATH_KNIGHT, CLASS_CONTEXT_TALENT_POINT_CALC) && player->GetMapId() == 609)
            return;   // Ebon Hold: the core counts the starting quests
        uint32 level = player->GetLevel();
        uint32 points = level >= sFirstLevel ? level - sFirstLevel + 1 : 0;
        talentPointsForLevel = points + const_cast<Player*>(player)->GetBonusTalentCount();
    }
};

class TemucraftTalentPoints_World : public WorldScript
{
public:
    TemucraftTalentPoints_World() : WorldScript("TemucraftTalentPoints_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sFirstLevel = sConfigMgr->GetOption<uint32>("Temucraft.Talents.FirstLevel", 2);
    }
};

void AddTemucraftTalentPointsScripts()
{
    new TemucraftTalentPoints_Player();
    new TemucraftTalentPoints_World();
}
