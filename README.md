# mod-temucraft-classes

Class changes: new talents and their scripts, new racials, and reagents as a player's choice.

- **57 new talents** across all classes (Talent.dbc 2286-2342, spells 95100-95271), placed in free spots of the stock trees. Scripted ones include Bloom, Templar's Bulwark, Lone Wolf, Focused Fire, Predatory Discipline, Lingering Rage, Demonic Energies, Demonic Brand, Bane of Havoc, Unsealed Wrath, Light's Vigil, Arcane Wrath and Spearing Strike.
- **New racials** (learned with the racial skill): Ruthless, Big Game Hunter, Artisan's Precision, Shatter Curse, Rapid Regeneration, Elune's Light, Eureka!, Touch of the Grave.
- **Reagents as a choice:** 120 class spells have their reagents back; each player picks with `.temucraft reagents on|off` (server default in the config).

## Install

1. Put this folder in `modules/` of your AzerothCore source, re-run CMake and build.
2. The SQL in `data/sql/` is applied by the worldserver on its next start (AzerothCore's module updater). Every file deletes its own rows before inserting them, so running one again is safe.
   Copy `conf/mod-temucraft-classes.conf.dist` to your `configs/modules/` as `mod-temucraft-classes.conf` to change its settings.
3. **Client.** `client/patch-C.MPQ/` is a patch folder, not a packed MPQ: pack it yourself (for example with Ladik's MPQ Editor: new MPQ, add the folder contents keeping the `DBFilesClient\` path) and put the `.MPQ` in the game's `Data/` folder. Each DBC in it is the stock 3.3.5a (12340) file plus only this module's rows.
   **Using more than one Temucraft module, or other DBC changes?** Two patches that both carry `Spell.dbc` overwrite each other (the later letter wins). Use the all-in-one patch from the Temucraft configuration repo (`client/patch-5.MPQ/`, every Temucraft change merged), or merge the rows yourself. The server side never conflicts: it reads this module's rows from the `*_dbc` tables in `data/sql/`.

## What it adds, and where

| Database | Table | IDs |
|---|---|---|
| world | `spell_dbc` | 95041-95048, 95100-95271 (new), plus 120 stock class spells with reagents back |
| world | `talent_dbc` | 2286-2342 |
| world | `skilllineability_dbc` | 31798, 31800-31806 (racials) |
| world | `spell_ranks`, `spell_proc`, `spell_script_names` | for the spells above |
| world | `creature_template` (+`_model`) | 601990 (Summon Hawk) |
| characters | `mod_temucraft_character_setting` (setting `reagents`) | per character |
| client | `Spell.dbc`, `Talent.dbc`, `SkillLineAbility.dbc` | the same rows |

## Works with other modules

- Overrides 120 stock class spells (only their reagents change): another module changing the same spells clashes.
- Talents take free tree spots of the stock trees: another talent mod using the same spots or Talent IDs 2286-2342 clashes.
- Talent scripts hook stock spells: Rejuvenation, Regrowth, Ferocious Bite, Holy Shock, Life Tap.

## Independence

This module doesn't need any other module, nor any other Temucraft module. 

## Customising IDs

The entries above are a block reserved for this module. Spell and talent IDs are fixed: they are in the scripts and the client DBCs.

## Texts and translations

The 5 player-facing texts (chat messages, gossip options) are in the `module_string` table under the module name
`mod-temucraft-classes`, so they can be changed without rebuilding. Translations go in `module_string_locale` with the same
module and id plus the locale (`deDE`, `esES`, `frFR`, ...). If a row is missing the English text in the code is used.

Part of the Temucraft modules for AzerothCore 3.3.5a.

## Requirements (forges, anvils, tools, soul shards)

The client's spell data lost some stock requirements. The server puts them back per kind, as
`Temucraft.Requirements.Anvil / Forge / CookingFire / OtherFocus / Tools / SoulShards` says (1 = required, the
default; 0 = not required). The stock values are in `mod_temucraft_requirement` (1,427 rows); they apply at startup
and on `.reload config`.

## Also in this module

These were separate modules; they now live here, each with its own folder under `src/`, its own config file in
`conf/` (same file and key names as before) and its notes under `docs/`. Their SQL files kept their names, so nothing
is applied twice. Remove the old module folders from `modules/` before building.

| Was | What | Code | Notes |
|---|---|---|---|
| mod-temucraft-class-tweaks | class and race changes, reworks and new class spells | `src/class_tweaks` | [docs/class_tweaks.md](docs/class_tweaks.md) |
| mod-temucraft-new-spells | new spells (Power Word: Barrier and others) | `src/new_spells` | [docs/new_spells.md](docs/new_spells.md) |
| mod-temucraft-racials | racial changes and heritage (racial bonuses) | `src/racials` | [docs/racials.md](docs/racials.md) |
| mod-temucraft-pushback | spell pushback: off, Wrath or classic | `src/pushback` | [docs/pushback.md](docs/pushback.md) |
| mod-temucraft-ranged-autoattack | right-click Auto Shot for every class | `src/ranged_autoattack` | [docs/ranged_autoattack.md](docs/ranged_autoattack.md) |
