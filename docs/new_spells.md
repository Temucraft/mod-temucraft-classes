# mod-temucraft-new-spells (part of mod-temucraft-classes)

New spells that need new spell data (rows in `spell_dbc` for the server and in the client's Spell.dbc).

## Steady crowd control

A copy of each crowd control spell that breaks on damage, that damage doesn't break: Polymorph (sheep, pig, turtle,
black cat, rabbit, turkey), Entangling Roots, Hibernate, Sap, Gouge, Blind, Repentance, Hex, Shackle Undead and
Scatter Shot. You know the copy of the highest rank of the usual spell you know (so it is learned at the same level),
it is named the same with "(Steady)" after its rank, and it shares the usual spell's cooldown and diminishing returns.
The copies sit in the General tab of the spellbook.

## Slap

Slap a sleeping player awake, friend or foe: sleep effects (Hibernate, Wyvern Sting...) end, and someone lying down
asleep stands up. An enemy also takes your basic attack. 5% of base mana, 30 second cooldown, melee range, the Lay
on Hands hand icon. Every class that uses mana knows it.

## Later spells made learnable

- **Heroic Leap** (warriors, level 40 at warrior trainers): leap to a spot and slam nearby enemies, stunning them;
  45 second cooldown.
- **Redirect** (rogues, level 42 at rogue trainers): moves your combo points to your new target; 1 minute cooldown.
- **Power Word: Barrier** (discipline priests from level 60, learned with the spec): everyone in your party or raid
  within 10 yards takes 25% less damage and suffers less spell pushback for 10 seconds; 3 minute cooldown, 15% base
  mana.

New ones from the same idea:

- **Rune of Power** (mages, level 50 at mage trainers): places a rune on the ground for 30 seconds; you and allies
  standing within 4 yards deal 10% more spell damage. 45 second cooldown.
- **Call of the Pack** (hunters, level 40 at hunter trainers): party and raid members within 25 yards run 40% faster
  for 8 seconds (no daze). 5 minute cooldown.

Heroic Leap, Redirect and Power Word: Barrier are spells the client already has; their rows are changed in `spell_dbc` and the client's Spell.dbc (cooldowns,
targets, texts).

## Talents our changes emptied

Talents that lost their point because of other Temucraft changes get a new one (spell data only):

- **Tactical Mastery** (warrior, Arms): kept rage when changing stances, but warriors no longer need stances (every
  ability works in any stance). Now it shortens Bloodrage and Berserker Rage by 5 / 10 / 15 sec; its threat bonus for
  Bloodthirst and Mortal Strike stays.

Earlier ones (core custom SQL, 2026-11-16): Natural Shapeshifter (shapeshift global cooldown instead of mana),
Improved Ghost Wolf, Improved Revive Pet, King of the Jungle.

Checked and still worth their points: Warbringer (still lets Charge, Intercept and Intervene work in combat), the
hunter pet talents Guard Dog, Bloodthirsty and Carrion Feeder (pets are always happy now, but their threat and healing
parts stay), the talents that daze enemies (Piercing Howl, Blade Twisting, Improved Concussive Shot, Concussive
Barrage, Aftermath: no-daze only covers dazes put on players), Improved Reincarnation, Improved Tranquility,
Malediction (now also takes away the curse price).

## Building the spell data

`tools/build_spells.py` builds the rows from the server's Spell.dbc into
`data/sql/db-world/base/mod_temucraft_new_spells_dbc.sql` and `src/TemucraftNewSpellsIds.h`; with `--client` it also
writes them into the client's Spell.dbc (patch-T.MPQ, with the game closed). Spell ids: free ids from 96000 up.

## Install

Copy the module into `modules/`, re-run CMake and build. World SQL: the spell rows and Slap's script. Players need
the client Spell.dbc with the same rows.

Part of the Temucraft modules for AzerothCore 3.3.5a.
