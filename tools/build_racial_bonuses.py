#!/usr/bin/env python3
"""Racial bonuses at their best (Temucraft.Racials.BestBonuses): one source for everything.

For each race, the strongest value of each passive racial bonus over every version of the game (Retail, Classic Era,
TBC/Wrath/Cata/MoP Classic, WoW Forever), the good parts of different versions combined, nothing that was removed.
The decisions: F:\\TemuCraft Server\\_dev\\reference\\racial-bonuses-best-2026-11-26.md.

Writes
  data/sql/db-world/base/mod_temucraft_racial_bonuses.sql   new spells (spell_dbc), their racial skill lines
                                                             (skilllineability_dbc) and linked helpers
  src/racials/TemucraftRacialBonusesData.h                   what the server changes in existing racial spells
                                                             (applied at startup, only with BestBonuses = 1)
With --client OUTDIR: OUTDIR/DBFilesClient/Spell.dbc and SkillLineAbility.dbc = the client's own files (from
5) Release/patches/patch-T.MPQ) plus the new rows and the new tooltips, ranks and values of the changed spells.
Pack them into the PTR copy of patch-T (never the Live one).

    python3 build_racial_bonuses.py [--client OUTDIR]      (paths: ~/mnt/TemuCraft Server, as in Ryan's VM)
"""
import os, struct, subprocess, sys

HOME = os.path.expanduser('~/mnt/TemuCraft Server')
HERE = os.path.dirname(os.path.abspath(__file__))
MOD = os.path.dirname(HERE)
SERVER_DBC = HOME + '/_dev/server-dbc/dbc'
CORE_SPELL_SQL = HOME + '/1) Source/data/sql/base/db_world/spell_dbc.sql'
PATCH_T = HOME + '/5) Release/patches/patch-T.MPQ'
MPQTOOL = HOME + '/_tools/mpqtool3.py'

# ------------------------------------------------------------------------------------------------------------ data
# auras (SpellAuraDefines.h)
DUMMY, MOD_RESISTANCE, MOD_STAT, MOD_SKILL, MOD_DODGE, HIT, SPELL_HIT = 4, 22, 29, 30, 49, 54, 55
INCREASE_SPEED, SWIM_SPEED, DMG_DONE_PCT, WATER_BREATHING, DMG_TAKEN_PCT, HP_REGEN_PCT = 31, 58, 79, 82, 87, 88
SKILL_TALENT, PCT_MOD, REGEN_IN_COMBAT, MECHANIC_RESIST, SPEED_ALWAYS = 98, 108, 116, 117, 129
POWER_PCT, HEALTH_PCT, HEAL_DONE_PCT, STAT_PCT, ARMOR_ITEMS_PCT, REDUCE_PUSHBACK = 132, 133, 136, 137, 142, 149
CRIT_DAMAGE, DMG_VERSUS, SPELL_HIT_TAKEN, MELEE_HASTE, SPELL_HASTE, EXPERTISE, CRIT_ALL = 163, 168, 186, 192, 216, 240, 290
PHYS, HOLY, FIRE, NATURE, FROST, SHADOW, ARCANE, MAGIC, ALL = 1, 2, 4, 8, 16, 32, 64, 126, 127
LEVEL = 'level'          # amount equal to the character's level (resistances)


def races(*ids):
    return sum(1 << (i - 1) for i in ids)


# racial skill lines (SkillLine.dbc) and the race masks of their rows. The stock lines' rows carry every race (who
# knows the line decides, as with the stock rows); the allied races' lines carry their own races.
ALL_RACES = races(*range(1, 31))
LINE = {'human': [(754, ALL_RACES)], 'orc': [(125, ALL_RACES)], 'dwarf': [(101, ALL_RACES)],
        'nightelf': [(126, ALL_RACES)], 'undead': [(220, ALL_RACES)], 'tauren': [(124, ALL_RACES)],
        'gnome': [(753, ALL_RACES)], 'troll': [(733, ALL_RACES)], 'goblin': [(790, races(9))],
        'bloodelf': [(756, ALL_RACES)], 'draenei': [(760, ALL_RACES)], 'worgen': [(789, races(12))],
        'pandaren': [(797, races(13, 14))], 'nightborne': [(812, races(15))], 'highmountain': [(801, races(16))],
        'voidelf': [(804, races(17))], 'lightforged': [(805, races(18))], 'zandalari': [(795, races(19))],
        'kultiran': [(799, races(20))], 'vulpera': [(796, races(21))], 'darkiron': [(794, races(22))],
        'maghar': [(792, races(23))], 'mechagnome': [(808, races(24))],
        'dracthyr': [(840, races(25)), (841, races(26))], 'earthen': [(807, races(27, 28))],
        'haranir': [(844, races(29, 30)), (845, races(29, 30))]}


def eff(aura, value=0, misc=0, family=None):
    return dict(aura=aura, value=value, misc=misc, family=family)


def rank(cat, sub):
    return 'Racial - %s (%s)' % (cat, sub)


def res_text(school):
    return ('Increases your %s resistance by an amount equal to your level, reduces %s damage taken by 1%%, and '
            'reduces the chance you will be hit by %s spells by 2%%.' % (school, school, school))


def res_effects(mask):
    return {0: eff(SPELL_HIT_TAKEN, -2, mask), 1: eff(MOD_RESISTANCE, LEVEL, mask), 2: eff(DMG_TAKEN_PCT, -1, mask)}


FIRST_SPELL, LAST_SPELL = 95800, 95834
FIRST_SLA, LAST_SLA = 31950, 32049

# New spells: id, race (racial line key; None = not learned: a helper or a buff), name, rank, tooltip, effects,
# icon taken from this spell, extra
NEW = [
    (95800, 'human', 'No Place Like Home', rank('Utility', 'Travel'),
     'The cooldown of your Hearthstone is 5 min shorter.', [eff(DUMMY)], 8690, {}),
    (95801, 'dwarf', 'Might of the Mountain', rank('Combat', 'Offense'),
     'Your critical strikes with attacks and spells deal 2% more damage.', [eff(CRIT_DAMAGE, 2, ALL)], 20594, {}),
    (95802, 'gnome', 'Shortblade Specialization', rank('Weapon Skill', 'Melee'),
     'Expertise with Daggers and One-Handed Swords increased by 4.', [eff(EXPERTISE, 4)], 20597,
     {'item': (2, (1 << 15) | (1 << 7))}),
    (95803, 'gnome', 'Nimble Fingers', rank('Combat', 'Offense'),
     'Attack and casting speed increased by 1%.', [eff(MELEE_HASTE, 1), eff(SPELL_HASTE, 1)], 20589, {}),
    (95804, 'nightelf', 'Touch of Elune', rank('Combat', 'Offense'),
     'Attack and casting speed increased by 1% at night, critical strike chance increased by 1% during the day.',
     [eff(MELEE_HASTE, 1), eff(SPELL_HASTE, 1), eff(CRIT_ALL, 1)], 58984, {}),
    (95805, 'tauren', 'Brawn', rank('Combat', 'Offense'),
     'Your critical strikes with attacks and spells deal 2% more damage.', [eff(CRIT_DAMAGE, 2, ALL)], 20549, {}),
    (95806, 'tauren', 'Plainsrunning', rank('Utility', 'Travel'),
     'While you keep moving on foot, your movement speed increases by 2% every second, up to 60%. Stopping, '
     'mounting or flying starts it over.', [eff(SPEED_ALWAYS, 0)], 20550, {}),
    (95807, 'troll', 'Dead Eye', rank('Weapon Skill', 'Ranged'),
     'Chance to hit increased by 1% while you wield a Gun, Bow or Crossbow.', [eff(HIT, 1)], 26290,
     {'item': (2, (1 << 2) | (1 << 3) | (1 << 18))}),
    (95808, 'bloodelf', 'Arcane Resistance', rank('Resistance', 'Arcane'),
     'Increases your Arcane resistance by an amount equal to your level and reduces Arcane damage taken by 1%.',
     [eff(MOD_RESISTANCE, LEVEL, ARCANE), eff(DMG_TAKEN_PCT, -1, ARCANE)], 20592, {}),
    (95809, 'bloodelf', 'Arcane Acuity', rank('Combat', 'Offense'),
     'Critical strike chance with all attacks and spells increased by 1%.', [eff(CRIT_ALL, 1)], 28730, {}),
    (95810, 'orc', 'Savage', rank('Combat', 'Offense'),
     'While you wield an Axe, your critical strike chance with all attacks and spells is increased by 1%.',
     [eff(CRIT_ALL, 1)], 20574, {'item': (2, (1 << 0) | (1 << 1))}),
    (95811, 'voidelf', 'Chill of Night', rank('Resistance', 'Shadow'),
     'Shadow damage taken reduced by 2%.', [eff(DMG_TAKEN_PCT, -2, SHADOW)], 81093, {}),
    (95812, 'voidelf', 'Preternatural Calm', rank('Combat', 'Defense'),
     'Taking damage no longer delays your spell casts.', [eff(REDUCE_PUSHBACK, 100)], 81091, {}),
    (95813, None, 'Entropic Embrace', '',
     'Damage and healing done increased by 5% for 12 sec.', [eff(DMG_DONE_PCT, 5, ALL), eff(HEAL_DONE_PCT, 5)], 81093,
     {'buff': 'Damage and healing done increased by 5%.'}),
    (95814, 'lightforged', 'Demonbane', rank('Utility', 'Experience'),
     'Experience gained from killing Demons increased by 20%.', [eff(DUMMY)], 81099, {}),
    (95815, 'darkiron', 'Forged in Flames', rank('Resistance', 'Physical'),
     'Physical damage taken reduced by 1%.', [eff(DMG_TAKEN_PCT, -1, PHYS)], 20594, {}),
    (95816, 'darkiron', 'Mass Production', rank('Profession', 'Crafting'),
     'Blacksmithing skill increased by 5, and you make Blacksmithing items 25% faster.',
     [eff(SKILL_TALENT, 5, 164), eff(PCT_MOD, -25, 10, family=(12, 1))], 20593, {}),
    (95817, 'kultiran', 'Jack of All Trades', rank('Profession', 'All'),
     'Skill in every profession increased by 5.',
     [eff(SKILL_TALENT, 5, 171), eff(SKILL_TALENT, 5, 164), eff(SKILL_TALENT, 5, 333)], 20593,
     {'links': [95818, 95819, 95820, 95821]}),
    (95818, None, 'Jack of All Trades', '', '',
     [eff(SKILL_TALENT, 5, 202), eff(SKILL_TALENT, 5, 182), eff(SKILL_TALENT, 5, 773)], 20593, {'hidden': True}),
    (95819, None, 'Jack of All Trades', '', '',
     [eff(SKILL_TALENT, 5, 755), eff(SKILL_TALENT, 5, 165), eff(SKILL_TALENT, 5, 186)], 20593, {'hidden': True}),
    (95820, None, 'Jack of All Trades', '', '',
     [eff(SKILL_TALENT, 5, 393), eff(SKILL_TALENT, 5, 197), eff(SKILL_TALENT, 5, 185)], 20593, {'hidden': True}),
    (95821, None, 'Jack of All Trades', '', '',
     [eff(SKILL_TALENT, 5, 129), eff(SKILL_TALENT, 5, 356)], 20593, {'hidden': True}),
    (95822, 'pandaren', 'Epicurean', rank('Stats', 'Primary'),
     'The stats you get from Well Fed effects are doubled.', [eff(DUMMY)], 81032, {}),
    (95823, 'earthen', 'Hyper Productive', rank('Profession', 'Gathering'),
     'Finesse increased by 25 when gathering: more bonus materials from herbs, ore, skins and fish.',
     [eff(DUMMY)], 81115, {}),
    (95824, 'earthen', 'Wide-Eyed Wonder', rank('Utility', 'Experience'),
     'Experience gained from discovering new areas increased by 200%.', [eff(DUMMY)], 81115, {}),
    (95825, 'earthen', 'Ingest Minerals', rank('Stats', 'Primary'),
     'You are always Well Fed: Stamina and Spirit increased by 40.', [eff(MOD_STAT, 40, 2), eff(MOD_STAT, 40, 4)],
     81115, {}),
    (95826, 'haranir', 'Lash Out', rank('Combat', 'Offense'),
     'Your critical strikes with attacks and spells deal 1% more damage.', [eff(CRIT_DAMAGE, 1, ALL)], 20583, {}),
    (95827, 'haranir', 'One With Nature', rank('Profession', 'Gathering'),
     'Herbalism skill increased by 5, and Finesse increased by 10 when gathering herbs.',
     [eff(SKILL_TALENT, 5, 182)], 20552, {}),
    (95828, 'haranir', 'Subterranean Predator', rank('Combat', 'Offense'),
     'Damage dealt to Beasts and Elementals increased by 1%, and experience gained from killing them increased by 10%.',
     [eff(DMG_VERSUS, 1, (1 << 0) | (1 << 3))], 20557, {}),
    (95829, 'dracthyr', 'Discerning Eye', rank('Profession', 'Gathering'),
     'Perception increased by 2 when gathering: a better chance to find rare reagents.', [eff(DUMMY)], 81177, {}),
    (95830, 'vulpera', 'Fire Resistance', rank('Resistance', 'Fire'),
     'Fire damage taken reduced by 1%.', [eff(DMG_TAKEN_PCT, -1, FIRE)], 81126, {}),
    (95831, 'zandalari', 'City of Gold', rank('Utility', 'Economy'),
     'Gold looted from creatures increased by 2%.', [eff(DUMMY)], 69044, {}),
    (95832, 'mechagnome', 'Combat Analysis', rank('Stats', 'Primary'),
     'Every 5 sec in combat your Strength, Agility and Intellect increase by 0.5%, stacking up to 10 times (5%). '
     'Out of combat one stack fades every 5 sec.',
     [eff(STAT_PCT, 0, 0), eff(STAT_PCT, 0, 1), eff(STAT_PCT, 0, 3)], 81111, {}),
    (95833, 'highmountain', 'Waste Not, Want Not', rank('Utility', 'Other'),
     'Gives a 25% chance to loot an additional piece of meat or fish.', [eff(DUMMY)], 81107, {}),
    (95834, None, 'Sympathetic Vigor', '', 'Health increased by 10%.', [eff(HEALTH_PCT, 10)], 273217, {'hidden': True}),
]

# Existing racial spells: ids, rank (None = keep), tooltip (None = keep; APPEND: adds to it), effect changes
# {index: eff}, extra
MODS = [
    # Human
    ([20597], rank('Weapon Skill', 'Melee'), 'Expertise and weapon skill with Swords and Two-Handed Swords increased by 5.',
     {0: eff(EXPERTISE, 5), 1: eff(MOD_SKILL, 5, 43), 2: eff(MOD_SKILL, 5, 55)}, {}),
    ([20864], rank('Weapon Skill', 'Melee'), 'Expertise and weapon skill with Maces and Two-Handed Maces increased by 5.',
     {0: eff(EXPERTISE, 5), 1: eff(MOD_SKILL, 5, 54), 2: eff(MOD_SKILL, 5, 160)}, {}),
    ([20598], rank('Stats', 'Primary'), 'Spirit increased by 10%.', {}, {}),
    ([20599], rank('Utility', 'Reputation'), 'Reputation gains increased by 10%. Vendors of factions you are Friendly '
     'with or better sell to you for 5% less, 25% less at Exalted.', {}, {}),
    ([58985], rank('Utility', 'Other'), None, {}, {}),
    ([95041], rank('Combat', 'Offense'), None, {}, {'noitem': True}),
    # Dwarf
    ([20595], rank('Weapon Skill', 'Ranged'), 'Weapon skill with Guns increased by 5 and your chance to critically hit '
     'with Guns increased by 1%.', {1: eff(MOD_SKILL, 5, 46)}, {}),
    ([20596], rank('Resistance', 'Frost'), res_text('Frost'), res_effects(FROST), {}),
    ([59224], rank('Weapon Skill', 'Melee'), None, {}, {}),
    ([95043], rank('Combat', 'Offense'), None, {}, {}),
    ([95044], rank('Combat', 'Offense'), None, {}, {'noitem': True}),
    # Night Elf
    ([20582], rank('Combat', 'Defense'), 'Dodge chance increased by 2% and the chance to be hit by melee and ranged '
     'attacks reduced by 2%.', {2: eff(MOD_DODGE, 2)}, {}),
    ([20583], rank('Resistance', 'Nature'), res_text('Nature'), res_effects(NATURE), {}),
    ([20585], rank('Utility', 'Travel'), None, {}, {}),
    ([21009], rank('Utility', 'Travel'), 'Movement speed increased by 2%, and by a further 5% while Stealthed or '
     'Prowling. Enemies have less chance to detect you while Shadowmelded or Stealthed.',
     {1: eff(SPEED_ALWAYS, 2), 2: eff(SPEED_ALWAYS, 0)}, {}),
    # Undead
    ([5227], rank('Utility', 'Survival'), 'You can breathe underwater indefinitely.', {0: eff(WATER_BREATHING)}, {}),
    ([20579], rank('Resistance', 'Shadow'), res_text('Shadow'), res_effects(SHADOW), {}),
    ([95148], rank('Combat', 'Offense'), None, {}, {}),
    # Tauren
    ([20550], rank('Stats', 'Health and Mana'), 'Total health increased by 5% and chance to hit with all attacks and '
     'spells increased by 1%.', {0: eff(HEALTH_PCT, 5), 1: eff(HIT, 1), 2: eff(SPELL_HIT, 1, ALL)}, {}),
    ([20551], rank('Resistance', 'Nature'), res_text('Nature'), res_effects(NATURE), {}),
    ([20552], rank('Profession', 'Gathering'), 'Herbalism skill increased by 15 and Deftness increased by 25: you '
     'gather herbs faster.', {}, {}),
    # Gnome
    ([20591], rank('Stats', 'Health and Mana'), 'Intellect and maximum mana increased by 5%. Maximum rage, energy and '
     'runic power also increased by 5%.', {1: eff(POWER_PCT, 5, 0)}, {}),
    ([20592], rank('Resistance', 'Arcane'), res_text('Arcane'), res_effects(ARCANE), {}),
    ([20593], rank('Profession', 'Crafting'), None, {}, {}),
    # Troll
    ([20555], rank('Stats', 'Health and Mana'), 'Health regeneration rate increased by 20%. 20% of total health '
     'regeneration may continue during combat.', {0: eff(HP_REGEN_PCT, 20), 1: eff(REGEN_IN_COMBAT, 20)}, {}),
    ([20557], rank('Combat', 'Offense'), 'Damage dealt versus Beasts increased by 5% and experience gained from killing '
     'Beasts increased by 20%.', {}, {}),
    ([20558], rank('Weapon Skill', 'Ranged'), 'Weapon skill with Thrown weapons increased by 5 and your chance to '
     'critically hit with them increased by 1%.', {1: eff(MOD_SKILL, 5, 176)}, {}),
    ([26290], rank('Weapon Skill', 'Ranged'), 'Weapon skill with Bows increased by 5 and your chance to critically hit '
     'with Bows increased by 1%.', {1: eff(MOD_SKILL, 5, 45)}, {}),
    ([58943], rank('Combat', 'Defense'), 'Duration of movement impairing effects reduced by 20%.', {}, {}),
    # Goblin
    ([69042], rank('Combat', 'Offense'), None, {}, {}),
    ([69044], rank('Utility', 'Economy'), 'Always receive the best possible discount from vendors (20%, as at '
     'Exalted), whatever your standing with their faction.', {}, {}),
    ([69045], rank('Profession', 'Crafting'), None, {}, {}),
    # Blood Elf
    ([822], rank('Resistance', 'All'), 'All resistances increased by 5 and the chance you will be hit by spells '
     'reduced by 2%.', {1: eff(MOD_RESISTANCE, 5, MAGIC)}, {}),
    ([28877], rank('Profession', 'Crafting'), None, {}, {}),
    # Draenei
    ([28875], rank('Profession', 'Crafting'), 'Jewelcrafting skill increased by 10.', {0: eff(SKILL_TALENT, 10, 755)}, {}),
    ([6562, 28878], rank('Combat', 'Offense'), 'Chance to hit with all spells and attacks increased by 1% for you and '
     'all party members within 30 yards. All your stats are increased by 1%.', {2: eff(STAT_PCT, 1, -1)}, {}),
    ([59221, 59535, 59536, 59538, 59539, 59540, 59541, 80872, 80873, 80874], rank('Resistance', 'Shadow'),
     res_text('Shadow'), res_effects(SHADOW), {}),
    # Worgen
    ([68975], rank('Combat', 'Offense'), None, {}, {}),
    ([68976], rank('Resistance', 'Nature, Shadow'), 'Increases your Nature and Shadow resistance by an amount equal to '
     'your level and reduces Nature and Shadow damage taken by 1%.',
     {0: eff(MOD_RESISTANCE, LEVEL, NATURE | SHADOW), 1: eff(DMG_TAKEN_PCT, -1, NATURE | SHADOW)}, {}),
    ([68978], rank('Profession', 'Gathering'), 'Skinning skill increased by 15 and Deftness increased by 25: you skin '
     'faster.', {}, {}),
    # Orc
    ([20573], rank('Combat', 'Defense'), 'Duration of Stun effects reduced by 20%, and a 25% chance to resist Stun '
     'effects.', {1: eff(MECHANIC_RESIST, 25, 12)}, {}),
    ([20574], rank('Weapon Skill', 'Melee'), 'Expertise with Axes, Two-Handed Axes and Fist Weapons increased by 5, and '
     'weapon skill with Axes and Two-Handed Axes increased by 5.',
     {1: eff(MOD_SKILL, 5, 44), 2: eff(MOD_SKILL, 5, 172)}, {}),
    ([20575, 20576, 21563, 54562, 65222], rank('Combat', 'Offense'), None, {}, {}),
    # Pandaren
    ([81034, 81062], rank('Profession', 'Secondary'), 'Cooking skill increased by 15 and base health increased by 5%.',
     {1: eff(SKILL_TALENT, 15, 185)}, {}),
    ([81032, 81060], rank('Utility', 'Experience'), 'You earn twice as much rested experience. Spirit increased by 3%.',
     {}, {}),
    ([81033, 81061], rank('Utility', 'Survival'), 'Falling damage taken reduced by 50%. Dodge chance increased by 2%.',
     {}, {}),
    # Nightborne
    ([81105], rank('Profession', 'Crafting'), 'Inscription skill increased by 15 and Intellect increased by 5%.',
     {1: eff(SKILL_TALENT, 15, 773)}, {}),
    ([81106], rank('Combat', 'Offense'), 'Magical damage dealt increased by 1% and magical damage taken reduced by 1%.',
     {1: eff(DMG_DONE_PCT, 1, MAGIC)}, {}),
    # Highmountain Tauren
    ([81108], rank('Combat', 'Offense'), 'Damage and healing done increased by 1% and base health increased by 5%.',
     {1: eff(DMG_DONE_PCT, 1, ALL), 2: eff(HEAL_DONE_PCT, 1)}, {}),
    ([81109], rank('Profession', 'Gathering'), 'Mining skill increased by 15 and Deftness increased by 25: you mine '
     'faster. Duration of Stun effects reduced by 10%.', {1: eff(SKILL_TALENT, 15, 186)}, {}),
    ([81110], rank('Combat', 'Defense'), 'Damage taken from each hit reduced by 1.8% of your Stamina. Chance to be hit '
     'by Nature spells reduced by 2%.', {}, {}),
    # Void Elf
    ([81093], rank('Combat', 'Offense'), 'Your attacks, spells and heals have a chance to grant Entropic Embrace, '
     'increasing damage and healing done by 5% for 12 sec.', {0: eff(DUMMY)}, {}),
    ([81094], rank('Stats', 'Primary'), None, {}, {}),
    # Lightforged Draenei
    ([81100], rank('Profession', 'Crafting'), 'Blacksmithing skill increased by 10 and Spirit increased by 3%.',
     {1: eff(SKILL_TALENT, 10, 164)}, {}),
    ([81101], rank('Resistance', 'All'), 'Magical damage taken reduced by 1% and healing done increased by 1%.',
     {1: eff(HEAL_DONE_PCT, 1)}, {}),
    # Kul Tiran
    ([81020], rank('Combat', 'Defense'), 'When you take damage, you heal for 2% of it over 4 sec. Damage and healing '
     'done increased by 1% and base health increased by 5%.',
     {1: eff(DMG_DONE_PCT, 1, ALL), 2: eff(HEAL_DONE_PCT, 1)}, {}),
    ([81021], rank('Utility', 'Survival'), 'You can hold your breath 233% longer and swim 10% faster.',
     {1: eff(SWIM_SPEED, 10)}, {}),
    ([81022], rank('Resistance', 'Frost, Nature'), 'Frost and Nature damage taken reduced by 1%, and the chance to be '
     'hit by Frost spells reduced by 2%.', {1: eff(DMG_TAKEN_PCT, -1, FROST | NATURE)}, {}),
    # Vulpera
    ([81125], rank('Combat', 'Defense'), 'The first time each enemy damages you, that damage is reduced by 5% of your '
     'maximum health. Dodge chance increased by 2%.', {}, {}),
    # Dark Iron Dwarf: Dungeon Delver's indoor buff (100001) gave 94% instead of 4%
    ([265223], rank('Utility', 'Travel'), 'While indoors, movement speed increased by 4%.', {}, {}),
    ([100001], None, 'While indoors, movement speed increased by 4%.', {0: eff(INCREASE_SPEED, 4)}, {}),
    # Mag'har Orc
    ([273216], rank('Utility', 'Travel'), None, {}, {}),
    ([273217], rank('Stats', 'Health and Mana'), 'Your pet\'s health is increased by 10%, and your base health by 10%.',
     {}, {}),
    ([273220], rank('Combat', 'Defense'), 'Duration of Poison, Disease and Curse effects on you reduced by 10%, and a '
     '15% chance to resist them.', {}, {}),
    # Mechagnome: Emergency Failsafe stays an active racial (rank "Racial"); its passive part comes on top
    ([81112], None, 'APPEND:\n\nPassive: when you fall below 20% health, you heal for 15% of your maximum health. '
     'This can only happen once every 2.5 min.', {}, {}),
    # Earthen
    ([81116, 81120], rank('Stats', 'Armor'), 'Armor from items increased by 10%. Duration of Stun effects reduced by 10%.',
     {1: eff(ARMOR_ITEMS_PCT, 10, 1)}, {}),
]

# ------------------------------------------------------------------------------------------------------------ DBC
class DBC:
    def __init__(self, data):
        _, n, self.f, self.rs, _ = struct.unpack('<4s4I', data[:20])
        self.rows, self.order = {}, []
        for i in range(n):
            r = list(struct.unpack_from('<%dI' % self.f, data, 20 + i * self.rs))
            self.rows[r[0]] = r
            self.order.append(r[0])
        self.strings = bytearray(data[20 + n * self.rs:])
        self.index = {}

    def s(self, o):
        return self.strings[o:self.strings.index(b'\0', o)].decode('utf-8') if o < len(self.strings) else ''

    def add(self, text):
        if not text:
            return 0
        b = text.encode('utf-8')
        if b not in self.index:
            self.index[b] = len(self.strings)
            self.strings += b + b'\0'
        return self.index[b]

    def put(self, row):
        if row[0] not in self.rows:
            self.order.append(row[0])
        self.rows[row[0]] = row

    def save(self, path):
        out = bytearray(struct.pack('<4s4I', b'WDBC', len(self.order), self.f, self.rs, len(self.strings)))
        for i in self.order:
            out += struct.pack('<%dI' % self.f, *self.rows[i])
        out += self.strings
        open(path, 'wb').write(out)


def u(v): return v & 0xFFFFFFFF
def fbits(x): return struct.unpack('<I', struct.pack('<f', x))[0]
def ffrom(b): return struct.unpack('<f', struct.pack('<I', b))[0]
def i32(v): return struct.unpack('<i', struct.pack('<I', v & 0xFFFFFFFF))[0]


# Spell.dbc columns (3.3.5a, 234 fields)
ATTR, ATTR_EX, CAST, DURATION, RANGE, ITEM_CLASS, ITEM_SUB = 4, 5, 28, 40, 46, 68, 69
EFFECT, DIE, PPL, BP, MECH, TA, TB, RADIUS, AURA, PERIOD, MULT = 71, 74, 77, 80, 83, 86, 89, 92, 95, 98, 101
CHAIN, ITEMTYPE, MISC, MISCB, TRIGGER, PPC, MASKA, MASKB, MASKC = 104, 107, 110, 113, 116, 119, 122, 125, 128
ICON, NAME, RANK, DESC, AURADESC, FAMILY, DESCVAR, BONUS = 133, 136, 153, 170, 187, 208, 232, 229
LANG_MASK = 16712190


def set_effect(row, i, e):
    value, ppl = (0, 1.0) if e['value'] == LEVEL else (e['value'], 0.0)
    row[EFFECT + i] = 6                     # apply aura
    row[DIE + i] = 1
    row[PPL + i] = fbits(ppl)
    row[BP + i] = u(value - 1)              # amount = base points + 1 die
    for col in (MECH, TB, RADIUS, PERIOD, CHAIN, ITEMTYPE, MISCB, TRIGGER, MASKB, MASKC):
        row[col + i] = 0
    row[TA + i] = 1                         # caster
    row[AURA + i] = e['aura']
    row[MULT + i] = fbits(0.0)
    row[MISC + i] = u(e['misc'])
    row[PPC + i] = fbits(0.0)
    row[MASKA + i] = e['family'][1] if e['family'] else 0
    row[BONUS + i] = fbits(1.0)
    if e['family']:
        row[FAMILY] = e['family'][0]


def set_text(dbc, row, col, text):
    row[col] = dbc.add(text)
    for k in range(1, 16):
        row[col + k] = 0
    row[col + 16] = LANG_MASK if text else 0


def new_row(dbc, spell):
    sid, line, name, rk, desc, effects, icon, extra = spell
    row = list(dbc.rows[20550])             # Endurance: a plain racial passive
    row[0] = sid
    for i in range(3):
        for col in (EFFECT, DIE, BP, MECH, TA, TB, RADIUS, AURA, PERIOD, CHAIN, ITEMTYPE, MISC, MISCB, TRIGGER,
                    MASKA, MASKB, MASKC):
            row[col + i] = 0
        for col in (PPL, MULT, PPC, BONUS):
            row[col + i] = fbits(0.0)
    for i, e in enumerate(effects):
        set_effect(row, i, e)
    row[ICON] = dbc.rows[icon][ICON] if icon in dbc.rows else row[ICON]
    row[DURATION] = 21                      # infinite
    if extra.get('hidden'):
        row[ATTR] = 0x40 | 0x80 | 0x100     # passive, not in the spellbook, not in the aura bar
    if extra.get('buff'):
        row[ATTR] = 0
        row[ATTR_EX] = 0
        row[DURATION] = 29                  # 12 sec
    if extra.get('item'):
        row[ITEM_CLASS], row[ITEM_SUB] = u(extra['item'][0]), extra['item'][1]
    else:
        row[ITEM_CLASS], row[ITEM_SUB] = u(-1), 0
    row[DESCVAR] = 0
    set_text(dbc, row, NAME, name)
    set_text(dbc, row, RANK, rk)
    set_text(dbc, row, DESC, desc)
    set_text(dbc, row, AURADESC, extra.get('buff', ''))
    return row


def apply_mods(dbc):
    for ids, rk, desc, effects, extra in MODS:
        for sid in ids:
            row = list(dbc.rows[sid])
            for i, e in effects.items():
                set_effect(row, i, e)
            if extra.get('noitem'):
                row[ITEM_CLASS], row[ITEM_SUB] = u(-1), 0
            if rk:
                set_text(dbc, row, RANK, rk)
            if desc:
                if desc.startswith('APPEND:'):
                    desc = dbc.s(row[DESC]) + desc[len('APPEND:'):]
                set_text(dbc, row, DESC, desc)
                row[DESCVAR] = 0
            dbc.put(row)


def sla_rows():
    rows, n = [], FIRST_SLA
    for sid, line, *_ in NEW:
        for skill, mask in (LINE[line] if line else []):
            # ID, SkillLine, Spell, RaceMask, ClassMask, ExcludeRace, ExcludeClass, MinSkillLineRank,
            # SupercededBySpell, AcquireMethod (2 = learned with the skill line), TrivialHigh, TrivialLow, CharPoints 1-2
            rows.append([n, skill, sid, mask, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0])
            n += 1
    assert n <= LAST_SLA + 1
    return rows

# ------------------------------------------------------------------------------------------------------------ SQL
def spell_columns():
    cols, inside = [], False
    for line in open(CORE_SPELL_SQL, encoding='utf-8'):
        if line.startswith('CREATE TABLE'):
            inside = True
            continue
        if inside:
            line = line.strip()
            if not line.startswith('`'):
                break
            name = line.split('`')[1]
            decl = line.split('`')[2]         # the type part only (a column name like "NameSubtext" holds "text")
            kind = 'f' if ' float' in decl else 's' if 'char' in decl or 'text' in decl else 'u' if 'unsigned' in decl else 'i'
            cols.append((name, kind))
    assert len(cols) == 234, len(cols)
    return cols


def sql_value(dbc, v, kind):
    if kind == 'f':
        return repr(round(ffrom(v), 6))
    if kind == 's':
        return "'" + dbc.s(v).replace('\\', '\\\\').replace("'", "''").replace('\n', '\\n') + "'"
    return str(v) if kind == 'u' else str(i32(v))


def write_sql(dbc, rows):
    cols = spell_columns()
    links = [(sid, l) for sid, _, _, _, _, _, _, extra in NEW for l in extra.get('links', [])]
    sla = sla_rows()
    out = ['-- mod-temucraft-classes (racials): Temucraft.Racials.BestBonuses, the strongest version of every passive racial',
           '-- bonus. New racial spells %d-%d, on the racial skill lines (skill line rows %d-%d) so new and existing'
           % (FIRST_SPELL, LAST_SPELL, FIRST_SLA, LAST_SLA),
           '-- characters learn them with their racials, and heritage carries them like the others. The client needs the',
           '-- same rows (tools/build_racial_bonuses.py --client). The changes to existing racial spells are made by the',
           '-- worldserver at startup (TemucraftRacialBonuses.cpp), only with BestBonuses = 1; with 0 the new spells are',
           '-- never learned and are taken away at login. Made by tools/build_racial_bonuses.py. Re-runnable.',
           'DELETE FROM `spell_dbc` WHERE `ID` BETWEEN %d AND %d;' % (FIRST_SPELL, LAST_SPELL),
           'INSERT INTO `spell_dbc` (%s) VALUES' % ', '.join('`%s`' % c for c, _ in cols),
           ',\n'.join('(' + ', '.join(sql_value(dbc, r[i], k) for i, (c, k) in enumerate(cols)) + ')' for r in rows) + ';',
           '',
           'DELETE FROM `skilllineability_dbc` WHERE `ID` BETWEEN %d AND %d;' % (FIRST_SLA, LAST_SLA),
           'INSERT INTO `skilllineability_dbc` (`ID`, `SkillLine`, `Spell`, `RaceMask`, `ClassMask`, `ExcludeRace`, '
           '`ExcludeClass`, `MinSkillLineRank`, `SupercededBySpell`, `AcquireMethod`, `TrivialSkillLineRankHigh`, '
           '`TrivialSkillLineRankLow`, `CharacterPoints_1`, `CharacterPoints_2`) VALUES',
           ',\n'.join('(' + ', '.join(str(i32(x)) for x in r) + ')' for r in sla) + ';',
           '',
           '-- Jack of All Trades: three effects per spell, so the other professions ride on hidden auras that come and go',
           '-- with it (type 2: aura)',
           'DELETE FROM `spell_linked_spell` WHERE `spell_trigger` IN (%s);' % ', '.join(sorted({str(s) for s, _ in links})),
           'INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `type`, `comment`) VALUES',
           ',\n'.join("(%d, %d, 2, 'Jack of All Trades: more professions')" % (s, l) for s, l in links) + ';']
    path = os.path.join(MOD, 'data', 'sql', 'db-world', 'base', 'mod_temucraft_racial_bonuses.sql')
    open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
    print('wrote', path, '-', len(rows), 'spells,', len(sla), 'skill line rows')

# ------------------------------------------------------------------------------------------------------------ C++ data
def write_header():
    lines = ['// Made by tools/build_racial_bonuses.py from its NEW and MODS lists: change it there, not here.',
             '// What Temucraft.Racials.BestBonuses changes in existing racial spells, and the spells it adds.',
             '#ifndef TEMUCRAFT_RACIAL_BONUSES_DATA_H', '#define TEMUCRAFT_RACIAL_BONUSES_DATA_H', '',
             '#include "Define.h"', '',
             'namespace TemucraftRacialBonusesData', '{',
             '    // value: the effect\'s amount; perLevel: the amount is the character\'s level instead (resistances)',
             '    struct EffectEdit { uint32 spell; uint8 index; uint32 aura; int32 value; int32 misc; bool perLevel; '
             'uint32 family; uint32 familyMask; };', '',
             '    constexpr uint32 FIRST_SPELL = %d;' % FIRST_SPELL, '    constexpr uint32 LAST_SPELL = %d;' % LAST_SPELL, '',
             '    // the new racials a character learns (the others in the range are hidden helpers and a buff)',
             '    constexpr uint32 LEARNED[] = { %s };' % ', '.join(str(s[0]) for s in NEW if s[1]), '',
             '    constexpr EffectEdit EFFECTS[] =', '    {']
    for ids, rk, desc, effects, extra in MODS:
        for sid in ids:
            for i, e in sorted(effects.items()):
                lvl = e['value'] == LEVEL
                fam = e['family'] or (0, 0)
                lines.append('        { %d, %d, %d, %d, %d, %s, %d, %d },' % (sid, i, e['aura'], 0 if lvl else e['value'],
                             e['misc'], 'true' if lvl else 'false', fam[0], fam[1]))
    lines += ['    };', '', '    // item requirement taken away (the tooltip always said "all attacks and spells")',
              '    constexpr uint32 NO_ITEM[] = { %s };' % ', '.join(str(sid) for ids, _, _, _, x in MODS if x.get('noitem')
                                                              for sid in ids),
              '}', '', '#endif', '']
    path = os.path.join(MOD, 'src', 'racials', 'TemucraftRacialBonusesData.h')
    open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines))
    print('wrote', path)

# ------------------------------------------------------------------------------------------------------------ main
def client_file(name):
    data = subprocess.run([sys.executable, MPQTOOL, '--get', PATCH_T, 'DBFilesClient\\' + name], capture_output=True).stdout
    assert data[:4] == b'WDBC', name
    return data


def build_spells(dbc):
    names = {s[0]: s[2] for s in NEW}
    for sid in range(FIRST_SPELL, LAST_SPELL + 1):
        assert sid not in dbc.rows or dbc.s(dbc.rows[sid][NAME]) == names.get(sid), 'spell %d is taken' % sid
    rows = [new_row(dbc, s) for s in NEW]
    for r in rows:
        dbc.put(r)
    return rows


if __name__ == '__main__':
    server = DBC(open(SERVER_DBC + '/Spell.dbc', 'rb').read())
    write_sql(server, build_spells(server))
    write_header()
    if '--client' in sys.argv:
        out = os.path.join(sys.argv[sys.argv.index('--client') + 1], 'DBFilesClient')
        os.makedirs(out, exist_ok=True)
        spell = DBC(client_file('Spell.dbc'))
        build_spells(spell)
        apply_mods(spell)
        spell.save(os.path.join(out, 'Spell.dbc'))
        sla = DBC(client_file('SkillLineAbility.dbc'))
        for r in sla_rows():
            assert r[0] not in sla.rows or sla.rows[r[0]][2] == r[2], 'skill line row %d is taken' % r[0]
            sla.put([u(x) for x in r])
        sla.save(os.path.join(out, 'SkillLineAbility.dbc'))
        print('wrote', out, '(Spell.dbc, SkillLineAbility.dbc): pack them into the PTR copy of patch-T.MPQ')
