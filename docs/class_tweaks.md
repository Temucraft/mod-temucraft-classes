# mod-temucraft-class-tweaks (part of mod-temucraft-classes)

Class changes that need no client data: tweaks to spells and items that stayed as they were in 3.3.5a, and reworks that let specs do new things (Affliction feeding the group, the price of curses, bleeds keeping Slice and Dice up).

Before: mod-temucraft-class-tweaks, mod-temucraft-class-reworks. Their tables, SQL file names, texts (`module_string` rows under their old names), settings and addon prefixes are unchanged, so an existing server only swaps the folders.

## From mod-temucraft-class-tweaks


Class changes that make the classes easier and more fun to play: a few from later class reworks, a few for spells and
items that stayed as they were in 3.3.5a. Everything happens on the server: no client patch is needed.

| Change | Who | How |
|---|---|---|
| Spell cooldowns of your choice (none by default; Shadowmeld and other racials are in [mod-temucraft-racials](https://github.com/Temucraft/mod-temucraft-racials)) | Everyone | `Cooldowns` (any spell can be added) |
| **Soul bags keep reagents**: an equipped soul bag has a chance not to use up a Soul Shard, Infernal Stone or Demonic Figurine (5% for a 16-slot bag up to 25% for a 32-slot one) | Warlocks | `SoulBags` |
| **Soul Shards stack to 99**, with no limit on how many you carry | Warlocks | SQL |
| **Infernal Stone** costs 10 silver, **Demonic Figurine** 20 silver; both fit in soul bags | Warlocks | SQL |
| **Ritual of Doom** needs the warlock and 2 helpers (not 4) and nobody is sacrificed | Warlocks | SQL |
| **Totemic Recall** is trained at level 10 (not 30) for 10 silver | Shamans | SQL |
| **Bows for night elf priests**: they learn Bows and Shoot, like the priestesses of the moon | Night elf priests | `NightElfPriestBows` |
| **Any weapon for anyone**: weapon masters teach every weapon and Dual Wield to every race and class (also Dual Wield without the shaman talent); a ranged weapon comes with the attack your class gets with it (Shoot, Throw, or another module's Auto Shot), else Shoot | Everyone | `AnyWeapon` (+ SQL) |
| **Any armor for anyone**: cloth, leather, mail (40), plate (40) and shields (with Block) | Everyone | `AnyArmor` (+ SQL) |
| **Raid-wide auras**: class auras, shouts, totems and party auras from talents reach the whole raid (heals stay party-wide) | Everyone | `RaidWideAuras` |
| **Hour-long buffs**: friendly class buffs of 10 or 30 minutes last an hour | Everyone | `BuffMinutes` |
| **Happy pets**: hunter pets never get unhappy, no feeding needed | Hunters | `HappyPets` |
| **Parachute**: every class trainer teaches it from level 10; its cooldown drops from 10 minutes at 10 to 2 minutes at 80 | Everyone | `Parachute` (+ SQL) |

Tooltips still show the stock texts (Shadowmeld's "2 min cooldown", Parachute's "1 min", Ritual of Doom's "4
additional party members", "party" on raid-wide auras, 30 minute buff durations). The cooldown bar and buff timers do
show the new values. Players see the new stack sizes and prices once the client's
item cache is refreshed (delete the client's `Cache` folder after the update).

- **No daze**: being hit from behind by creatures no longer dazes or dismounts, and Aspect of the Cheetah / Pack
  no longer daze when struck (`NoDaze`).
- **Tranquility** heals the 5 most injured raid members in reach every tick, not just the party
  (`SmartTranquility`).

### Install

1. Put this folder in `modules/` of your AzerothCore source, re-run CMake and build.
2. The SQL in `data/sql/db-world/base/` is applied by the worldserver on its next start. It only changes stock rows
   (`item_template` 6265, 5565, 16583; `trainer_spell` for spell 36936; `gameobject_template` 177193), adds every
   weapon and armor proficiency and Dual Wield to the weapon masters and Parachute to every class trainer
   (`trainer_spell`, marked VerifiedBuild -1811), and adds its texts to `module_string`. `data/sql/optional/mod_temucraft_class_tweaks_undo.sql` puts the stock values back.
3. Copy `conf/mod-temucraft-class-tweaks.conf.dist` to `configs/modules/` as `mod-temucraft-class-tweaks.conf` to
   change the settings. `.reload config` applies them.

### Works with other modules

- [mod-temucraft-ranged-autoattack](https://github.com/Temucraft/mod-temucraft-ranged-autoattack): its Auto Shot / Auto Throw (learned with the bow, gun, crossbow and thrown skills) is what
  other classes get with a new ranged weapon; Shoot is only taught when no attack came with the skill.
- [mod-temucraft-threat](https://github.com/Temucraft/mod-temucraft-threat)'s pet tank happiness floor and HappyPets agree (this one just keeps pets full).
- [mod-world-randomizer](https://github.com/TheSCREWEDSoftware/mod-world-randomizer) gives rolled proficiencies on every login; opened skills simply stay.
- With AnyWeapon off, NightElfPriestBows still lets night elf priests have the Bows skill (only Bows is opened, and
  only the module teaches it to them).

- Raid-wide auras and hour-long buffs are worked out from the spell data when the server loads its spells: other
  modules' new class spells are included.
- [mod-temucraft-auto-learn](https://github.com/Temucraft/mod-temucraft-auto-learn) learns Parachute and the new Totemic Recall level like any other trainer spell.

- With [mod-temucraft-classes](https://github.com/Temucraft/mod-temucraft-classes)' reagent choice, a spell cast without reagents uses nothing, so a soul bag has nothing
  to keep.
- Another module changing the same rows (soul shard stacks, those prices, Totemic Recall's training, the Doom
  Portal) clashes with the SQL here: the one applied last wins.

### Independence

This module doesn't need any other module, nor any other Temucraft module.

### Texts and translations

The one player-facing text ("Your soul bag kept the ...") is in `module_string` under `mod-temucraft-class-tweaks`, id
1. Translations go in `module_string_locale`. If the row is missing the English text in the code is used.

Part of the Temucraft modules for AzerothCore 3.3.5a.

## From mod-temucraft-class-reworks


Specs doing things they never could.

Tank mode and healer threat now live in **[mod-temucraft-threat](https://github.com/Temucraft/mod-temucraft-threat)**.

### Affliction feeds the group

Damage over time from an Affliction warlock's spells heals nearby group members (40 yards) for 2% of the damage and
gives them 1% as mana. Alone, it feeds the warlock.

### The price of curses

A warlock in a group who curses an enemy carries a copy of that curse at 25% strength for up to 30 seconds: Curse of
Weakness, Curse of Recklessness, Curse of Tongues (so longer casts), Curse of the Elements, Curse of Exhaustion. Alone
there is no price. The **Malediction** talent removes the price and turns Curse of Recklessness into an armor-only
curse (the target gains no attack power).

### Rogues: bleeds feed Slice and Dice

Every tick of a rogue's bleeds adds 1 second to their Slice and Dice, up to its full length.

### Install

Copy the module into `modules/`, re-run CMake and build. No tables (the world SQL only clears this module's old texts).

## Class and race changes (TemucraftClassChanges.cpp, `Temucraft.ClassChanges.*`)
- Weapon masteries: racial Sword/Mace/Axe Specialization give +1% crit and 1% less dodged/parried with those weapons.
- Racial heals: every race gets its own take on Cannibalize (10 sec channel, 7% health every 2 sec, 2 min cooldown,
  shortened by the racial cooldown cut), with its own name, icon and animation. Undead and skeletons keep Cannibalize.
  Spells 100172-100187 (`tools/build_racial_mending.py`).
- Warriors learn Victory Rush at 10.
- Ghost Wolf walks on water (with Water Walking known); Sentry Totem reveals stealthed and invisible enemies;
  Rolling Thunder (Lightning Bolt / Chain Lightning with Lightning Shield: mana and an extra charge).
- Wildkin Frenzy (hit in Moonkin Form), Tiger's Fury 18 sec with Tiger's Bloodlust stacks from bleeds.
- Judgement of Light at full health stores Light's Reserve; Overwhelming Light for Holy paladins.
- Holy Fire stronger and longer; Lightwell 15 charges, one drink per player.
- Banes: Agony, Doom, Tongues, Exhaustion can sit next to one other curse.
- Demonic Empowerment also works on an Infernal, a Doomguard and an enslaved demon.
Spells 96400-96405 (`tools/build_class_change_spells.py`, server `spell_dbc` and client Spell.dbc in patch-T).

## New class spells (TemucraftNewClassSpells.cpp, `Temucraft.NewClassSpells.*`)
- Mages: **Overloaded Blast** (ranks 32-80): instant Arcane damage, 8 sec cooldown, only while Overloaded (5 sec after
  one of your spells is fully or partly resisted), can't be resisted.
- Night elf priests with a bow: **Elune's Arrow** (ranks 20-78): weapon damage plus Fire, 10 sec cooldown.
- Shamans with a shield: **Stonewall Slam** (20): taunt, 5 yd, 10 sec cooldown.
Spells 100110-100126 (`tools/build_new_spells.py`, server `spell_dbc` and client Spell.dbc in patch-T). Learned
automatically at the level of each rank.

## Client DBCs (weapon and armor skills for everyone)
The server opens the skills in memory. The client and AoWoW read the DBC files, so they need the same change, or
they keep showing the stock class lists:

    python tools/build_open_skills.py            # writes client/patch-T.MPQ/DBFilesClient/ (SkillRaceClassInfo, SkillLineAbility)
    python tools/build_open_skills.py --client   # also puts them into the client's patch-T.MPQ (game closed)

It starts from patch-5's two files, keeps the rows mod-temucraft-classes adds (its racials), and opens the same skills
as the module (weapons, Dual Wield, armor, shields); attacks like Shoot keep their classes. Run it again after patch-5
or mod-temucraft-classes change, and with `--weapons-only` if AnyArmor is turned off. Then copy patch-T to the
release `patches\` and re-import AoWoW's DBC data.
