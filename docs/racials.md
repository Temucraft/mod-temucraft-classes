# mod-temucraft-racials (part of mod-temucraft-classes)

Racials for Temucraft: every racial at its best and usable more often, and heritage (carry another race's racials, plus a second racial from the races you can't play).

Before: mod-temucraft-racials, mod-temucraft-heritage. Their tables, SQL file names, texts (`module_string` rows under their old names), settings and addon prefixes are unchanged, so an existing server only swaps the folders.

## From mod-temucraft-racials


Racials at their best: the strongest version each racial had over the years, with the nerfs left out, and usable
more often.

| Change | Before | After |
|---|---|---|
| Cooldowns of every racial | 3 min / 2 min / 105 sec | 2 min / 90 sec / ~80 sec (25% shorter, at most 2 min, not below 45 sec) |
| Shadowmeld, Escape Artist | 2 min, 105 sec | 1 min |
| Stoneform | poison, disease, bleed; +10% armor | also harmful curses and magic, and 10% less damage taken while it lasts |
| Gift of the Naaru | fixed heal | at least 20% of the target's maximum health |
| Hardiness / Da Voodoo Shuffle | 15% | 20% shorter stuns / movement impairing effects |
| The Human Spirit | +3% Spirit | +10% Spirit |
| Diplomacy | +10% reputation | also 5% off vendor prices with factions you're Friendly with |
| Expansive Mind | +5% mana | also +5% rage, energy and runic power |

The cooldown rule covers every spell ranked "Racial", so custom races' racials ([mod-worgoblin-high-elf](https://github.com/Temucraft/mod-worgoblin-high-elf)'s allied
races, the classes module's) get shorter cooldowns too. The server's and the client's cooldown are both corrected right
after the cast. Everything has a switch in the config.

Racials that later expansions split into several spells, and racials the 3.3.5 client has no spell for (Might of the
Mountain, Brawn, Arcane Acuity, Touch of Elune...), need new client spell data and aren't part of this module.

### Racial bonuses at their best (`Temucraft.Racials.BestBonuses`)

For each race, the strongest version of every passive racial bonus over the years (Retail, Classic Era, TBC/Wrath/
Cata/MoP Classic, WoW Forever), the good parts of different versions combined and nothing that was removed later:
Cultivation is +15 Herbalism (Cataclysm) and +25 Deftness (Dragonflight); Sword Specialization +5 weapon skill
(Classic) and +5 expertise (TBC). The server's own value counts as a version, so no racial gets weaker. The full
per-race table, with the versions each value comes from and what was left out: `_dev/reference/racial-bonuses-best-2026-11-26.md`.

| Race | Changed racials | New racials (95800-95834) |
|---|---|---|
| Human | Sword and Mace Specialization +5 expertise and +5 skill; Ruthless works without a Gun | No Place Like Home (Hearthstone -5 min) |
| Orc | Hardiness also 25% chance to resist stuns; Axe Specialization +5 skill | Savage (+1% crit with Axes) |
| Dwarf | Gun Specialization +5 skill; Frost Resistance = level, -1% damage; Artisan's Precision works without a Gun | Might of the Mountain (+2% crit damage) |
| Night Elf | Quickness +2% dodge; Nature Resistance; Elusiveness +2% speed, +5% while stealthed | Touch of Elune (haste at night, crit by day) |
| Undead | Underwater Breathing indefinite; Shadow Resistance | - |
| Tauren | Endurance +5% total health and +1% hit; Nature Resistance; Cultivation +25 Deftness | Brawn (+2% crit damage), Plainsrunning (up to +60% speed on foot) |
| Gnome | Expansive Mind +5% mana too; Arcane Resistance | Shortblade Specialization (+4 expertise), Nimble Fingers (+1% haste) |
| Troll | Regeneration 20%/20%; Beast Slaying +20% XP; Bow and Throwing +5 skill | Dead Eye (+1% hit with ranged weapons) |
| Goblin | Best Deals Anywhere works (20% everywhere) | - |
| Blood Elf | Magic Resistance +5 all resistances | Arcane Resistance, Arcane Acuity (+1% crit) |
| Draenei | Gemcutting +10; Heroic Presence +1% all stats; Shadow Resistance | - |
| Worgen | Flayer +25 Deftness; Aberration = level, -1% damage | - |
| Pandaren | Gourmand +15 Cooking; Inner Peace double rested XP; Bouncy half falling damage | Epicurean (Well Fed doubled) |
| Nightborne | Magical Affinity +1% magic damage; Ancient History +15 Inscription | - |
| Highmountain Tauren | Rugged Tenacity (hits smaller by 1.8% Stamina); Mountaineer +1% damage/healing; Pride of Ironhorn +15 Mining, +25 Deftness | Waste Not, Want Not |
| Void Elf | Entropic Embrace (in combat a 40% roll every 15 sec, sure on the 4th: +5% damage/healing for 12 sec) | Chill of Night (-2% Shadow damage), Preternatural Calm (no pushback) |
| Lightforged Draenei | Holy Resistance +1% healing; Forge of Light +10 Blacksmithing | Demonbane (+20% XP vs Demons) |
| Zandalari Troll | - | City of Gold (+2% gold) |
| Kul Tiran | Brush It Off (heal 2% of damage taken, +1% damage/healing); Child of the Sea +10% swim speed; Rime -1% Frost/Nature damage | Jack of All Trades (+5 all professions) |
| Vulpera | Nose for Trouble (first hit of each enemy smaller) | Fire Resistance |
| Dark Iron Dwarf | Dungeon Delver 4% (was 94%) | Forged in Flames, Mass Production (+5 Blacksmithing, 25% faster) |
| Mag'har Orc | Savage Blood -10% poison/disease/curse duration; Sympathetic Vigor +10% pet health | - |
| Mechagnome | Emergency Failsafe heals 15% below 20% health (2.5 min) | Combat Analysis (passive: +1% Str/Agi/Int every 5 sec in combat, up to 10%) |
| Dracthyr | - | Discerning Eye (+2 Perception) |
| Earthen | Titan-Forged +10% armor from items | Ingest Minerals (+60 Stamina and Spirit), Hyper Productive (+25 Finesse), Wide-Eyed Wonder (+200% exploration XP) |
| Haranir | - | Lash Out, One With Nature, Subterranean Predator |

How:

- `tools/build_racial_bonuses.py` holds every value and tooltip. It writes `mod_temucraft_racial_bonuses.sql` (the new
  spells, their rows on the racial skill lines 31950-32049, Jack of All Trades' hidden helpers) and
  `src/racials/TemucraftRacialBonusesData.h` (what changes in existing racial spells); with `--client OUTDIR` it writes the
  client's `Spell.dbc` and `SkillLineAbility.dbc` (patch-T's own files plus these rows, ranks and tooltips).
- The worldserver changes the existing spells at startup (`TemucraftRacialBonuses.cpp`), only with BestBonuses = 1.
  New racials sit on the racial skill lines, so they are learned like the others at creation and at every login, and
  heritage carries them. With BestBonuses = 0 they are never learned and are taken away at login.
- What auras can't do is scripted there too (XP by creature type, Hearthstone, vendor discount, gold, rested XP,
  falling damage, Well Fed, Plainsrunning, Touch of Elune, Combat Analysis, Emergency Failsafe, Nose for Trouble,
  Rugged Tenacity, Brush It Off, Entropic Embrace, Savage Blood, Mass Production, pet health, extra meat and fish).
- Deftness, Finesse and Perception come from mod-temucraft-professions (`mod_temucraft_profession_racial`,
  `Temucraft.Professions.RacialStats`): a character gets the stats of the racials it knows.
- Tooltips: the rank line reads "Racial - Category (Sub-group)", e.g. "Racial - Utility (Travel)", for every passive
  racial bonus (categories: Profession, Weapon Skill, Stats, Combat, Resistance, Utility). "Racial" ranks are matched by
  their start, so cooldowns and heritage keep working with the new ranks.
- Settings: `Temucraft.Racials.BestBonuses` (1), `.ResistancePerLevel` (1; 0 = flat 10), `.StunResistChance` (25),
  `.PlainsrunningMax` (60), `.EntropicEmbracePeriod` (15 sec), `.EntropicEmbraceChance` (40),
  `.EntropicEmbraceGuarantee` (4: the roll after 3 misses in a row always succeeds; 0 = never). Entropic Embrace rolls
  on the module's 1 sec player tick, only in combat; leaving combat resets the misses. The tooltip states the defaults.
  Spell values need a restart.

### Racial actives (`Temucraft.Racials.Actives`)

The racials of `_dev/references/wow-racials_v3.html` that the races did not have, or only had as a renamed copy of a
stock racial (Haymaker, Bull Rush, Quaking Palm and Wing Buffet were War Stomp; Arcane Pulse was Arcane Torrent;
Spatial Rift and Cantrips were Shadowmeld; Bag of Tricks and Soar were Escape Artist; Make Camp was Cannibalize;
Azerite Surge was Stoneform; Light's Reckoning was Gift of the Naaru; Ancestral Call was Blood Fury; Combat Analysis was
Berserking), each in its best version, adapted to 3.3.5 and balanced for level 80. Spells 95835-95876. The decisions
(what was picked, what was left out and why): `_dev/reference/racial-actives-2026-11-26.md`.

| Race | New (or replacing the copy) | Cooldown |
|---|---|---|
| Zandalari Troll | Regeneratin' (50% health over 6 sec, only direct damage stops it); Pterrordax Swoop (while falling: slow fall, +60% speed); Embrace of Bwonsamdi (passive: 15% chance on damage to drain the target, 20 sec) | 2 min, 2 min |
| Dark Iron Dwarf | Fireblood (cleanse poison, disease, curse, magic, bleed + highest primary stat, more per effect removed); Mole Machine (tunnel to a discovered Dark Iron place: menu) | 90 sec, 10 min |
| Mag'har Orc | Ancestral Call (one of four spirits for 15 sec: +5% crit, +5% haste, +5/+3 AP/SP per level, or +3% damage/healing and -3% damage taken) | 90 sec |
| Kul Tiran | Haymaker (single target: damage, 3 sec stun, knockback) | 2 min |
| Vulpera | Bag of Tricks (enemy: damage and 80% slow; ally: heal and +20% speed; 4 sec); Make Camp; Return to Camp | 75 sec, -, 10 min |
| Mechagnome | Hyper Organic Light Originator (two decoys taunt your attackers, 8 sec, not bosses); Skeleton Pinkie (pick locks up to 5 x level) | 2 min, - |
| Nightborne | Arcane Pulse (8 yd Arcane damage, 80% slow 8 sec); Cantrips (a mailbox for 90 sec) | 2 min, 2 min |
| Highmountain Tauren | Bull Rush (leap ~10 yd, knock down up to 5 enemies 1.5 sec) | 90 sec |
| Lightforged Draenei | Light's Judgment (ground target, Holy damage after 3 sec, 10 yd); Light's Reckoning (passive: on death, damage enemies and heal allies 8 yd); Forge of Light (forge and anvil for 3 min) | 2 min, -, 2 min |
| Void Elf | Spatial Rift (a rift travels forward, cast again to step through, up to 35 yd) | 2 min |
| Pandaren | Quaking Palm (single target, 4 sec incapacitate, damage breaks it) | 90 sec |
| Dracthyr | Wing Buffet (frontal knockback); Tail Swipe (knockback around you); Soar (leap and glide, outdoors, out of combat) | 75 sec, 75 sec, 15 sec |
| Earthen | Azerite Surge (1.5 sec cast: fire cone, +50% on the healthiest enemy, heals allies in it); Quiet Contemplation (sit: 5% health and mana a second, 20 sec) | 90 sec, - |
| Haranir | Thorn Bloom (ground target: damage + DoT on 8 enemies, heal + HoT on 8 allies); Rootwalking (to Nighthaven, Moonglade) | 2 min, 10 min |

How:

- `tools/build_racial_actives.py` holds every spell (ids, effects, tooltips, icons, cooldowns). It writes
  `mod_temucraft_racial_actives.sql` (spell_dbc 95835-95899, new skill line rows 32050-32149, the skill line rows of
  the replaced copies pointed at the new spell, spell_script_names, texts), `mod_temucraft_racial_camp.sql`
  (characters: the Vulpera camp) and `src/racials/TemucraftRacialActivesData.h`; with `--client OUTDIR` the client's
  `Spell.dbc` and `SkillLineAbility.dbc` on top of the PTR's (`--base`), with the mod-worgoblin-high-elf fixes below.
  Run `build_racial_bonuses.py` first when its lists change.
- `TemucraftRacialActives.cpp` scripts them. Amounts: a part per level plus a part of the higher of attack power and
  1.5 x spell power (melee and casters land close); the tooltips say the same.
- Cooldowns are the final ones: the cooldown cut above skips these spells. Combat ones follow the same rule (retail
  -25%, at most 2 min); the teleports (Mole Machine, Return to Camp, Rootwalking) are 10 min.
- At login a character loses the replaced copy and its action bar buttons move to the new spell. Old copies stay in the
  DBC (and keep their fixes in `mod_temucraft_racials_fixes.sql`), nobody learns them.
- Actives = 0: the new spells are never learned and are taken away at login, the copies come back.
- Stock 3.3.5 icons only.

Also fixed (mod-worgoblin-high-elf's racials): Rugged Tenacity, Rime of the Ancient Mariner and Desert Hardened used aura
186 (attacker's chance to hit) under a "damage taken" tooltip, now damage taken (aura 87, BestBonuses' MODS and the
client); Dungeon Delver and Sympathetic Vigor had retail icon file ids, now stock icons (client); Sympathetic Vigor's pet
health is mod-worgoblin-high-elf's now (it puts the aura on the pet), so this module no longer adds 95834.

### Works with other modules

mod-temucraft-heritage: swapped and second racials get the same treatment. [mod-temucraft-classes](https://github.com/Temucraft/mod-temucraft-classes) no longer sets
Shadowmeld's cooldown by default (its `Cooldowns` list is empty); if you set racial cooldowns there too, leave them out
of one of the two.

### Install

Copy the module into `modules/`, re-run CMake and build. No SQL.

### Fixes for custom race racials

`mod_temucraft_racials_fixes.sql` gives custom race racials that copy a stock racial what the stock one has: Make Camp
(Vulpera) gets Cannibalize's script, Spatial Rift (Void Elf) and Cantrips (Nightborne) get Shadowmeld's linked
Sanctuary and deselect, and Azerite Surge (Earthen) gets Stoneform's armor buff. The spell ids are those of
mod-worgoblin-high-elf (`modpaks/config.sql`).

## From mod-temucraft-heritage


Heritage: a player can carry the racial traits of another race instead of their own: a dwarf with Shadowmeld and
Quickness, an orc with Every Man for Himself. Going back to their own traits is always possible (and free).

- **Choosing**: the **Heritage** button beside the character window (or `/heritage`) lists the races with their
  traits; or `.temucraft heritage` (list), `.temucraft heritage choose <race name or id>`, `.temucraft heritage reset`.
- **Rules** (config): from level 10, 10 gold, then 24 hours before the next change; optionally only races of the
  player's own faction.
- **Second racial**: on top of the heritage, a character can carry racials of races that can't be played on this
  server (Pandaren, Vulpera, Dracthyr... whatever the client has that no playable race owns), one by default, 10 gold
  each, dropping is free: in the same menu, or `.temucraft heritage bonus` (list), `.temucraft heritage take <name or id>`,
  `.temucraft heritage drop <name or id>`. Kept per character (`mod_temucraft_heritage_bonus`).
- **What changes**: all racial traits of the race (actives and passives, weapon specializations, resistances...).
  Class versions follow the player's class (Blood Fury for casters, Arcane Torrent for energy users, Gift of the
  Naaru...). Languages, looks and reputations stay those of the player's own race.

A race's traits are worked out at startup the way the game hands them to a new character: the racial skill lines
the race is given at creation (`playercreateinfo_skills`) and their SkillLineAbility spells that fit the race and the
player's class. So custom races are offered too, also those sharing the stock racial lines (as
[mod-worgoblin-high-elf](https://github.com/Temucraft/mod-worgoblin-high-elf)'s races do) and races above 32; racials other modules add to those lines come along. A race
whose own traits can't be found can't change heritage (it would keep its own next to the new ones), and a heritage
that can't be carried any more (race removed, module turned off) gives the own traits back at login. The choice is kept per
character (`mod_temucraft_heritage`) and put back on every login (the game hands a character its own racials again
when it loads).

The Heritage button is part of the Temucraft interface built by [mod-temucraft-client-version](https://github.com/Temucraft/mod-temucraft-client-version) (addon messages,
prefix THR); without it the commands work on their own.

### Install

1. Put this folder in `modules/` of your AzerothCore source, re-run CMake and build.
2. The SQL in `data/sql/` is applied by the worldserver on its next start: `mod_temucraft_heritage` (characters) and
   the module's texts in `module_string`.
3. Copy `conf/mod-temucraft-heritage.conf.dist` to `configs/modules/` as `mod-temucraft-heritage.conf` to change the
   settings. `.reload config` applies them.

### Works with other modules

- Other modules' cooldown or tuning changes to a racial (for example [mod-temucraft-classes](https://github.com/Temucraft/mod-temucraft-classes)' Shadowmeld) follow
  the spell, so they apply to whoever carries it.
- Spellbook tooltips are the game's own; nothing on the client changes.

### Independence

This module doesn't need any other module, nor any other Temucraft module.

### Texts and translations

The 13 player-facing texts are in `module_string` under `mod-temucraft-heritage`. Translations go in
`module_string_locale`. If a row is missing the English text in the code is used.

Part of the Temucraft modules for AzerothCore 3.3.5a.
