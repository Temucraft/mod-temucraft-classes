/*
 * mod-temucraft-classes: crafting and spell requirements, one switch per kind.
 *
 * The client's Spell.dbc lost a number of stock requirements: the anvil, forge or cooking fire a recipe must be
 * cast near, other spell foci (alchemy lab, moonwell, altar of shadows), the tools some recipes need (Jeweler's
 * Kit, Simple Grinder) and the soul shard of the healthstones, Inferno and Soulshatter. The stock values are in
 * mod_temucraft_requirement. At startup (and on .reload config) the server puts each kind back on its spells, or
 * leaves it off, as Temucraft.Requirements.* says. The client lets the cast start either way; the server answers
 * with the usual "Requires Anvil" or "Missing reagent" error when the kind is on.
 */
#include "Config.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "WorldScript.h"
#include <array>
#include <vector>

namespace
{
    enum Kind : uint8 { KIND_ANVIL = 1, KIND_FORGE, KIND_COOKING_FIRE, KIND_OTHER_FOCUS, KIND_TOOL, KIND_SOUL_SHARD, KIND_MAX };

    char const* const KindOption[KIND_MAX] = { "", "Temucraft.Requirements.Anvil", "Temucraft.Requirements.Forge",
        "Temucraft.Requirements.CookingFire", "Temucraft.Requirements.OtherFocus", "Temucraft.Requirements.Tools",
        "Temucraft.Requirements.SoulShards" };

    struct Row { uint32 spell; uint8 kind; uint32 value; uint32 count; };
    std::vector<Row> sRows;
    bool sLoaded = false;

    void Load()
    {
        sRows.clear();
        if (QueryResult r = WorldDatabase.Query("SELECT `SpellID`, `Kind`, `Value`, `Count` FROM `mod_temucraft_requirement`"))
            do
            {
                Field* f = r->Fetch();
                Row row { f[0].Get<uint32>(), f[1].Get<uint8>(), f[2].Get<uint32>(), f[3].Get<uint32>() };
                if (row.kind > 0 && row.kind < KIND_MAX)
                    sRows.push_back(row);
            } while (r->NextRow());
        sLoaded = true;
    }

    void Apply()
    {
        if (!sLoaded)
            return;
        std::array<bool, KIND_MAX> on = { };
        for (uint8 k = 1; k < KIND_MAX; ++k)
            on[k] = sConfigMgr->GetOption<bool>(KindOption[k], true);

        std::array<uint32, KIND_MAX> counts = { };
        for (Row const& row : sRows)
        {
            SpellInfo* info = const_cast<SpellInfo*>(sSpellMgr->GetSpellInfo(row.spell));
            if (!info)
                continue;
            bool use = on[row.kind];
            switch (row.kind)
            {
                case KIND_TOOL:
                    info->Totem[0] = use ? row.value : 0;
                    break;
                case KIND_SOUL_SHARD:
                    info->Reagent[0] = use ? int32(row.value) : 0;
                    info->ReagentCount[0] = use ? row.count : 0;
                    break;
                default:
                    info->RequiresSpellFocus = use ? row.value : 0;
                    break;
            }
            if (use)
                ++counts[row.kind];
        }
        LOG_INFO("module", "mod-temucraft-classes: requirements on: anvil {}, forge {}, cooking fire {}, other focus {}, tools {}, soul shards {}",
            counts[KIND_ANVIL], counts[KIND_FORGE], counts[KIND_COOKING_FIRE], counts[KIND_OTHER_FOCUS], counts[KIND_TOOL], counts[KIND_SOUL_SHARD]);
    }
}

class TemucraftRequirements_World : public WorldScript
{
public:
    TemucraftRequirements_World() : WorldScript("TemucraftRequirements_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_STARTUP }) { }

    void OnAfterConfigLoad(bool reload) override
    {
        if (reload)
            Apply();
    }

    void OnStartup() override
    {
        Load();
        Apply();
    }
};

void AddTemucraftRequirementsScripts()
{
    new TemucraftRequirements_World();
}
