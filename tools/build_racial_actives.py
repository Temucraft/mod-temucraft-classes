#!/usr/bin/env python3
"""Racial actives (and the missing racial passives) from wow-racials_v3: one source for everything.

The best version of every racial in _dev/references/wow-racials_v3.html that the races did not have yet, or only had
as a renamed copy of a stock racial (Haymaker / Bull Rush / Quaking Palm / Wing Buffet = War Stomp, Arcane Pulse =
Arcane Torrent, Spatial Rift / Cantrips = Shadowmeld, Bag of Tricks / Soar = Escape Artist, Make Camp = Cannibalize,
Azerite Surge = Stoneform, Light's Reckoning = Gift of the Naaru, Ancestral Call = Blood Fury, Combat Analysis =
Berserking), adapted to 3.3.5 and balanced for level 80. The decisions: _dev/reference/racial-actives-2026-11-26.md.

Writes
  data/sql/db-world/base/mod_temucraft_racial_actives.sql     new spells (spell_dbc), their racial skill lines
                                                               (skilllineability_dbc: new rows, and the rows of the
                                                               replaced copies pointed at the new spell), the
                                                               spell_script_names
  data/sql/db-characters/base/mod_temucraft_racial_camp.sql    Vulpera camp table
  src/racials/TemucraftRacialActivesData.h                     ids, replaced spells, cooldowns (for the C++)
With --client OUTDIR: OUTDIR/DBFilesClient/Spell.dbc and SkillLineAbility.dbc = the PTR's files (--base, default the
PTR patch-T build folder, which already has build_racial_bonuses.py's rows) plus the rows here and the fixes to
mod-worgoblin-high-elf racials (icons, aura 186). Pack them into the PTR copy of patch-T (never the Live one).

    python3 build_racial_actives.py [--client OUTDIR] [--base DIR]     (paths: ~/mnt/TemuCraft Server, as in Ryan's VM)
Run build_racial_bonuses.py first when its lists change (this tool uses its DBC helpers and its MODS for the fixes).
"""
import os, struct, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import build_racial_bonuses as B   # noqa: E402

MOD = B.MOD
PTR_PATCH_T = B.HOME + '/5) Release/ptr/patches/patch-T.MPQ'
PTR_BASE = B.HOME + '/_tools/ptr-patch-T-build-2026-11-26/DBFilesClient'

FIRST_SPELL, LAST_SPELL = 95835, 95899
FIRST_SLA, LAST_SLA = 32050, 32149

# ------------------------------------------------------------------------------------------------------------ enums
# effects
E_SCHOOL_DAMAGE, E_DUMMY, E_TELEPORT, E_HEAL, E_APPLY_AURA, E_SUMMON_OBJECT_WILD = 2, 3, 5, 10, 6, 76
E_OPEN_LOCK, E_KNOCK_BACK, E_LEAP_BACK = 33, 98, 138
# auras
A_DUMMY, A_PERIODIC_DAMAGE, A_PERIODIC_HEAL, A_STUN, A_MOD_STAT, A_OBS_MOD_HEALTH, A_OBS_MOD_POWER = 4, 3, 8, 12, 29, 20, 21
A_INCREASE_SPEED, A_DECREASE_SPEED, A_DMG_DONE, A_DMG_DONE_PCT, A_DMG_TAKEN_PCT, A_AP, A_RANGED_AP = 31, 33, 13, 79, 87, 99, 124
A_FEATHER_FALL, A_HEAL_DONE_PCT, A_HEAL_DONE, A_MELEE_HASTE, A_SPELL_HASTE, A_RANGED_HASTE, A_CRIT_PCT = 105, 136, 135, 192, 216, 218, 290
# targets
T_CASTER, T_ENEMY, T_SRC_AREA_ENEMY, T_DEST_AREA_ENEMY, T_ALLY, T_SRC_CASTER, T_ANY = 1, 6, 15, 16, 21, 22, 25
T_GO_ITEM, T_SRC_AREA_ALLY, T_DEST_AREA_ALLY, T_FRONT_RIGHT, T_FRONT_LEFT, T_FRONT, T_CONE_ALLY = 26, 30, 31, 41, 44, 47, 59
T_DEST_DEST, T_CONE_ENEMY = 87, 104
# attributes
A0_ABILITY, A0_PASSIVE, A0_HIDDEN, A0_DO_NOT_LOG, A0_OUTDOORS = 0x10, 0x40, 0x80, 0x100, 0x8000
A0_NOT_SHAPESHIFTED, A0_NO_SHEATH, A0_WHILE_DEAD, A0_WHILE_SITTING, A0_PEACEFUL = 0x10000, 0x40000, 0x800000, 0x8000000, 0x10000000
A1_NO_REDIRECTION, A1_NO_REFLECTION, A1_NO_THREAT, A1_NO_AURA_ICON = 0x8, 0x80, 0x400, 0x10000000
# aura interrupt flags
I_TAKE_DAMAGE, I_MOVE, I_NOT_SEATED, I_NOT_ABOVEWATER, I_CHANGE_MAP, I_DIRECT_DAMAGE, I_LANDING = \
    0x2, 0x8, 0x40000, 0x80, 0x80000, 0x01000000, 0x02000000
# mechanics
M_STUN, M_KNOCKOUT, M_SNARE = 12, 14, 11
# schools
PHYS, HOLY, FIRE, NATURE, FROST, SHADOW, ARCANE = 1, 2, 4, 8, 16, 32, 64
# damage class / prevention
DC_NONE, DC_MAGIC, DC_MELEE = 0, 1, 2
# stock 3.3.5 visuals taken from these spells
VIS = {'war_stomp': 20549, 'concussion': 12809, 'gouge': 1776, 'arcane_explosion': 42921, 'holy_wrath': 48817,
       'slow_fall': 130, 'hearthstone': 8690, 'cannibalize': 20578, 'blink': 1953, 'pick_lock': 1804,
       'thunderstorm': 51490, 'food': 45548, 'flamestrike': 2120}

# ------------------------------------------------------------------------------------------------------------ data
def eff(effect, aura=0, value=0, misc=0, ta=T_CASTER, tb=0, radius=None, amp=0, ppl=0.0, mech=0, miscb=0, trigger=0):
    return dict(effect=effect, aura=aura, value=value, misc=misc, ta=ta, tb=tb, radius=radius, amp=amp, ppl=ppl,
                mech=mech, miscb=miscb, trigger=trigger)


def aura(a, value=0, misc=0, ta=T_CASTER, **kw):
    return eff(E_APPLY_AURA, a, value, misc, ta, **kw)


def rank(cat, sub):
    return B.rank(cat, sub)


DUMMY_SELF = [eff(E_DUMMY)]

# key, id, race (racial line key, None = helper), name, rank, icon names (first found wins), description, fields
# fields: cast (ms), cd (ms; the final cooldown: these skip the racials module's cut), dur (ms, -1 = until cancelled),
# range (yd; 0 self, 'melee'), gcd (False = off the global cooldown), a0..a3 (attributes), aif (aura interrupt flags),
# targets (TargetFlags), facing, school, dmgclass, maxtargets, visual (VIS key), effects, buff (aura text), script
# (spell_script_names), replaces (old spell ids whose skill line rows now teach this one), passive, hidden
SPELLS = [
    # ---------------------------------------------------------------- Zandalari Troll (19)
    dict(key='REGENERATIN', id=95835, race='zandalari', name="Regeneratin'", rank=rank('Combat', 'Healing'),
         icon=['Spell_Nature_Regenerate', 'Spell_Nature_Rejuvenation'],
         desc="Regenerate 50% of your maximum health over 6 sec. Only direct damage interrupts it.",
         buff="Regenerating 10% of maximum health every 1.2 sec.", cd=120000, dur=6000, aif=I_DIRECT_DAMAGE,
         visual='cannibalize', effects=[aura(A_OBS_MOD_HEALTH, 10, amp=1200)]),
    dict(key='PTERRORDAX_SWOOP', id=95836, race='zandalari', name='Pterrordax Swoop', rank=rank('Utility', 'Travel'),
         icon=['Ability_Hunter_Pet_WindSerpent', 'Spell_Magic_FeatherFall'],
         desc="While falling, whistle up a Pterrordax that glides you to the ground: you fall slowly and move 60% "
              "faster until you land (up to 30 sec).",
         buff="Gliding to the ground. Movement speed increased by 60%.", cd=120000, dur=30000, gcd=False,
         aif=I_LANDING | I_CHANGE_MAP, a0=A0_ABILITY | A0_OUTDOORS, visual='slow_fall',
         effects=[aura(A_FEATHER_FALL), aura(A_INCREASE_SPEED, 60)], script='spell_temucraft_pterrordax_swoop'),
    dict(key='EMBRACE_OF_BWONSAMDI', id=95837, race='zandalari', name='Embrace of Bwonsamdi', passive=True,
         rank=rank('Combat', 'Offense'), icon=['Spell_Shadow_LifeDrain02', 'Spell_Shadow_LifeDrain'],
         desc="Bwonsamdi walks with you: your damaging attacks and spells have a 15% chance to drain your target, "
              "dealing Shadow damage and healing you for the same amount (about 6 x your level, plus 25% of your "
              "attack power or 37% of your spell power). Can happen once every 20 sec.",
         effects=[aura(A_DUMMY)]),
    dict(key='BWONSAMDI_DRAIN', id=95838, race=None, name='Embrace of Bwonsamdi', rank='',
         icon=['Spell_Shadow_LifeDrain02', 'Spell_Shadow_LifeDrain'], desc='', range=100, gcd=False, school=SHADOW,
         dmgclass=DC_MAGIC, a1=A1_NO_REFLECTION | A1_NO_REDIRECTION, visual='cannibalize',
         effects=[eff(E_SCHOOL_DAMAGE, ta=T_ENEMY, value=1), eff(E_HEAL, value=1)]),
    # ---------------------------------------------------------------- Dark Iron Dwarf (22)
    dict(key='FIREBLOOD', id=95839, race='darkiron', name='Fireblood', rank=rank('Combat', 'Defense'),
         icon=['Spell_Fire_LavaSpawn', 'Spell_Fire_Volcano', 'Spell_Fire_Fire'],
         desc="Removes all harmful Poison, Disease, Curse, Magic and Bleed effects, and increases your highest "
              "primary stat (Strength, Agility or Intellect) by 1.5 x your level for 8 sec, and by another 0.5 x "
              "your level for each effect removed (up to 3).", cd=90000, gcd=False, script='spell_temucraft_fireblood',
         visual='holy_wrath', effects=DUMMY_SELF),
    dict(key='FIREBLOOD_BUFF', id=95840, race=None, name='Fireblood', rank='',
         icon=['Spell_Fire_LavaSpawn', 'Spell_Fire_Volcano', 'Spell_Fire_Fire'], desc='', dur=8000, gcd=False,
         buff="Primary stat increased.", effects=[aura(A_MOD_STAT, 0, 0), aura(A_MOD_STAT, 0, 1), aura(A_MOD_STAT, 0, 3)]),
    dict(key='MOLE_MACHINE', id=95841, race='darkiron', name='Mole Machine', rank=rank('Utility', 'Travel'),
         icon=['Trade_Mining', 'INV_Pick_02'],
         desc="Summon a Mole Machine and tunnel to a Dark Iron place you have discovered: Ironforge, Thelsamar, "
              "Blackrock Mountain, Un'Goro Crater, Honor Hold, the Blade's Edge Mountains or Valiance Keep. Outdoors "
              "only. The cooldown starts when you choose where to go.", cast=3000, cd=600000,
         a0=A0_ABILITY | A0_OUTDOORS | A0_PEACEFUL, visual='hearthstone', script='spell_temucraft_mole_machine',
         effects=DUMMY_SELF),
    # ---------------------------------------------------------------- Mag'har Orc (23)
    dict(key='ANCESTRAL_CALL', id=95842, race='maghar', name='Ancestral Call', rank=rank('Combat', 'Offense'),
         icon=['Spell_Nature_SpiritWolf', 'Spell_Shaman_FeralSpirit'],
         desc="Invoke the spirits of your ancestors, granting one of these for 15 sec: Ferocity of the Frostwolf "
              "(critical strike chance +5%), Might of the Blackrock (attack power +5 and spell power +3 per level), "
              "Zeal of the Burning Blade (haste +5%) or Rictus of the Laughing Skull (damage and healing done +3%, "
              "damage taken -3%).", replaces=[274738], cd=90000, gcd=False, script='spell_temucraft_ancestral_call',
         effects=DUMMY_SELF),
    dict(key='FEROCITY_FROSTWOLF', id=95843, race=None, name='Ferocity of the Frostwolf', rank='',
         icon=['Ability_Hunter_Pet_Wolf', 'Spell_Nature_SpiritWolf'], desc='', dur=15000, gcd=False,
         buff="Critical strike chance increased by 5%.", effects=[aura(A_CRIT_PCT, 5)]),
    dict(key='MIGHT_BLACKROCK', id=95844, race=None, name='Might of the Blackrock', rank='',
         icon=['Ability_Warrior_BattleShout'], desc='', dur=15000, gcd=False,
         buff="Attack power and spell power increased.",
         effects=[aura(A_AP, 0, ppl=5.0), aura(A_RANGED_AP, 0, ppl=5.0), aura(A_DMG_DONE, 0, 126, ppl=3.0)]),
    dict(key='ZEAL_BURNING_BLADE', id=95845, race=None, name='Zeal of the Burning Blade', rank='',
         icon=['Spell_Nature_BloodLust'], desc='', dur=15000, gcd=False, buff="Haste increased by 5%.",
         effects=[aura(A_MELEE_HASTE, 5), aura(A_SPELL_HASTE, 5), aura(A_RANGED_HASTE, 5)]),
    dict(key='RICTUS_LAUGHING_SKULL', id=95846, race=None, name='Rictus of the Laughing Skull', rank='',
         icon=['Spell_Shadow_DeathScream'], desc='', dur=15000, gcd=False,
         buff="Damage and healing done increased by 3%, damage taken reduced by 3%.",
         effects=[aura(A_DMG_DONE_PCT, 3, 127), aura(A_HEAL_DONE_PCT, 3), aura(A_DMG_TAKEN_PCT, -3, 127)]),
    # ---------------------------------------------------------------- Kul Tiran (20)
    dict(key='HAYMAKER', id=95847, race='kultiran', name='Haymaker', rank=rank('Combat', 'Offense'),
         icon=['INV_Gauntlets_04', 'Ability_Warrior_PunishingBlow'],
         desc="Haul off and punch your target for Physical damage (10 x your level plus 50% of your attack power or "
              "75% of your spell power), stunning it for 3 sec and knocking it back.", replaces=[81019],
         cd=120000, dur=3000, range='melee', facing=1, school=PHYS, dmgclass=DC_MELEE, visual='concussion',
         script='spell_temucraft_haymaker',
         # the knockback before the stun (a rooted target can't be knocked back)
         effects=[eff(E_SCHOOL_DAMAGE, ta=T_ENEMY, value=1), eff(E_KNOCK_BACK, ta=T_ENEMY, value=40, misc=150),
                  aura(A_STUN, ta=T_ENEMY, mech=M_STUN)]),
    # ---------------------------------------------------------------- Vulpera (21)
    dict(key='BAG_OF_TRICKS', id=95848, race='vulpera', name='Bag of Tricks', rank=rank('Combat', 'Offense'),
         icon=['INV_Misc_Bag_10', 'INV_Misc_Bag_07'],
         desc="Pull something from your bag and throw it. On an enemy: Shadow damage (8 x your level plus 50% of "
              "your attack power or 75% of your spell power) and 80% slower for 4 sec. On an ally: heals the same "
              "amount and 20% faster for 4 sec.", replaces=[81123], cd=75000, range=40,
         script='spell_temucraft_bag_of_tricks', effects=[eff(E_DUMMY, ta=T_ANY)]),
    dict(key='BAG_OF_TRICKS_ENEMY', id=95849, race=None, name='Bag of Tricks', rank='',
         icon=['INV_Misc_Bag_10', 'INV_Misc_Bag_07'], desc='', dur=4000, range=100, gcd=False, school=SHADOW,
         dmgclass=DC_MAGIC, buff="Movement speed reduced by 80%.", visual='cannibalize',
         effects=[eff(E_SCHOOL_DAMAGE, ta=T_ENEMY, value=1), aura(A_DECREASE_SPEED, -80, ta=T_ENEMY, mech=M_SNARE)]),
    dict(key='BAG_OF_TRICKS_ALLY', id=95850, race=None, name='Bag of Tricks', rank='',
         icon=['INV_Misc_Bag_10', 'INV_Misc_Bag_07'], desc='', dur=4000, range=100, gcd=False, school=NATURE,
         dmgclass=DC_MAGIC, buff="Movement speed increased by 20%.", visual='cannibalize',
         effects=[eff(E_HEAL, ta=T_ALLY, value=1), aura(A_INCREASE_SPEED, 20, ta=T_ALLY)]),
    dict(key='MAKE_CAMP', id=95851, race='vulpera', name='Make Camp', rank=rank('Utility', 'Travel'),
         icon=['Spell_Fire_Fire', 'INV_Misc_Bag_11'],
         desc="Set up camp here. Return to Camp takes you back to it. Outdoors only.", replaces=[81124], cast=3000,
         a0=A0_ABILITY | A0_OUTDOORS | A0_PEACEFUL, script='spell_temucraft_make_camp', effects=DUMMY_SELF),
    dict(key='RETURN_TO_CAMP', id=95852, race='vulpera', name='Return to Camp', rank=rank('Utility', 'Travel'),
         icon=['INV_Misc_Rune_01'], desc="Return to the camp you made with Make Camp.", cast=5000, cd=600000,
         a0=A0_ABILITY | A0_PEACEFUL, visual='hearthstone', script='spell_temucraft_return_to_camp',
         effects=DUMMY_SELF),
    # ---------------------------------------------------------------- Mechagnome (24)
    dict(key='HOLO', id=95853, race='mechagnome', name='Hyper Organic Light Originator',
         rank=rank('Combat', 'Defense'), icon=['Spell_Magic_LesserInvisibilty'],
         desc="Project two holographic decoys of yourself for 8 sec. Enemies within 15 yd that are fighting you turn "
              "on the decoys (not dungeon and raid bosses).", replaces=[81111], cd=120000,
         script='spell_temucraft_holo', effects=DUMMY_SELF),
    dict(key='SKELETON_PINKIE', id=95854, race='mechagnome', name='Skeleton Pinkie', rank=rank('Utility', 'Other'),
         icon=['INV_Misc_Key_03', 'INV_Misc_Key_14'],
         desc="Your mechanical pinkie picks locks on chests, lockboxes and doors, up to lockpicking 5 x your level.",
         cast=1500, range='melee', a0=A0_ABILITY | A0_NOT_SHAPESHIFTED, a1=0x20000, iflags=0x1f, visual='pick_lock',
         effects=[eff(E_OPEN_LOCK, value=0, misc=1, ta=T_GO_ITEM, ppl=5.0)]),
    # ---------------------------------------------------------------- Nightborne (15)
    dict(key='ARCANE_PULSE', id=95855, race='nightborne', name='Arcane Pulse', rank=rank('Combat', 'Offense'),
         icon=['Spell_Nature_WispSplode', 'Spell_Arcane_Blast'],
         desc="Release a pulse of arcane energy, dealing Arcane damage (10 x your level plus 75% of your attack "
              "power or 112% of your spell power) to enemies within 8 yd and slowing them by 80% for 8 sec.",
         replaces=[81103], cd=120000, dur=8000, school=ARCANE, dmgclass=DC_MAGIC, a1=A1_NO_REFLECTION,
         visual='arcane_explosion', script='spell_temucraft_arcane_pulse',
         effects=[eff(E_SCHOOL_DAMAGE, ta=T_SRC_CASTER, tb=T_SRC_AREA_ENEMY, radius=8, value=1),
                  aura(A_DECREASE_SPEED, -80, ta=T_SRC_CASTER, tb=T_SRC_AREA_ENEMY, radius=8, mech=M_SNARE)]),
    dict(key='CANTRIPS', id=95856, race='nightborne', name='Cantrips', rank=rank('Utility', 'Other'),
         icon=['INV_Misc_Book_06', 'INV_Misc_Book_09'],
         desc="Conjure an Eldritch Grimoire that works as a mailbox for 1 min 30 sec.", replaces=[81104],
         cast=3000, cd=120000, dur=90000, a0=A0_ABILITY | A0_PEACEFUL,
         effects=[eff(E_SUMMON_OBJECT_WILD, misc=32349, ta=T_FRONT, radius=2)]),
    # ---------------------------------------------------------------- Highmountain Tauren (16)
    dict(key='BULL_RUSH', id=95857, race='highmountain', name='Bull Rush', rank=rank('Combat', 'Offense'),
         icon=['Ability_Warrior_Charge', 'Ability_Druid_Charge'],
         desc="Charge forward about 10 yd, knocking down up to 5 enemies within 5 yd of where you land for "
              "1.5 sec.", replaces=[81107], cd=90000, a0=0x20040010, a1=0x400, a2=0x4000000, a3=0x40010000,
         script='spell_temucraft_bull_rush', effects=[eff(E_LEAP_BACK, value=50, misc=200)]),
    dict(key='BULL_RUSH_STUN', id=95858, race=None, name='Bull Rush', rank='', icon=['Ability_Warrior_Charge'],
         desc='', dur=1500, gcd=False, maxtargets=5, dmgclass=DC_MAGIC, a1=A1_NO_REFLECTION, visual='war_stomp',
         buff="Knocked down.",
         effects=[aura(A_STUN, ta=T_SRC_CASTER, tb=T_SRC_AREA_ENEMY, radius=5, mech=M_STUN)]),
    # ---------------------------------------------------------------- Lightforged Draenei (18)
    dict(key='LIGHTS_JUDGMENT', id=95859, race='lightforged', name="Light's Judgment", rank=rank('Combat', 'Offense'),
         icon=['Spell_Holy_SearingLightPriest', 'Spell_Holy_SearingLight', 'Spell_Holy_Excorcism_02'],
         desc="Call down a strike of Holy light at the target spot. After 3 sec it deals Holy damage (15 x your level "
              "plus 100% of your attack power or 150% of your spell power) to enemies within 10 yd; beyond 8 targets "
              "the damage is shared.", replaces=[81099], cd=120000, range=40, targets=0x40, school=HOLY,
         dmgclass=DC_MAGIC, script='spell_temucraft_lights_judgment',
         effects=[eff(E_DUMMY, ta=T_DEST_DEST, radius=10)]),
    dict(key='LIGHTS_JUDGMENT_DAMAGE', id=95860, race=None, name="Light's Judgment", rank='',
         icon=['Spell_Holy_SearingLightPriest', 'Spell_Holy_SearingLight', 'Spell_Holy_Excorcism_02'], desc='',
         range=100, gcd=False, school=HOLY, dmgclass=DC_MAGIC, a1=A1_NO_REFLECTION, visual='holy_wrath',
         script='spell_temucraft_lights_judgment_damage',
         effects=[eff(E_SCHOOL_DAMAGE, ta=T_DEST_DEST, tb=T_DEST_AREA_ENEMY, radius=10, value=1)]),
    dict(key='LIGHTS_RECKONING', id=95861, race='lightforged', name="Light's Reckoning", passive=True,
         rank=rank('Combat', 'Healing'), icon=['Spell_Holy_Excorcism', 'Spell_Holy_Restoration'],
         desc="When you die, the Light erupts from you, dealing Holy damage to enemies and healing allies within 8 yd "
              "(15 x your level plus 50% of your attack power or 75% of your spell power).", effects=[aura(A_DUMMY)]),
    dict(key='LIGHTS_RECKONING_BURST', id=95862, race=None, name="Light's Reckoning", rank='',
         icon=['Spell_Holy_Excorcism', 'Spell_Holy_Restoration'], desc='', gcd=False, school=HOLY,
         dmgclass=DC_MAGIC, a0=A0_WHILE_DEAD, a1=A1_NO_REFLECTION, visual='holy_wrath',
         effects=[eff(E_SCHOOL_DAMAGE, ta=T_SRC_CASTER, tb=T_SRC_AREA_ENEMY, radius=8, value=1),
                  eff(E_HEAL, ta=T_SRC_CASTER, tb=T_SRC_AREA_ALLY, radius=8, value=1)]),
    dict(key='FORGE_OF_LIGHT', id=95863, race='lightforged', name='Forge of Light', rank=rank('Profession', 'Crafting'),
         icon=['Trade_BlackSmithing'],
         desc="Call down a forge and an anvil of Light for 3 min, so you can do Blacksmithing anywhere.", cast=3000,
         cd=120000, dur=180000, a0=A0_ABILITY | A0_PEACEFUL,
         effects=[eff(E_SUMMON_OBJECT_WILD, misc=1744, ta=T_FRONT_LEFT, radius=3),
                  eff(E_SUMMON_OBJECT_WILD, misc=1685, ta=T_FRONT_RIGHT, radius=3)]),
    # ---------------------------------------------------------------- Void Elf (17)
    dict(key='SPATIAL_RIFT', id=95864, race='voidelf', name='Spatial Rift', rank=rank('Utility', 'Travel'),
         icon=['Spell_Shadow_DemonicCircleTeleport', 'Spell_Arcane_Blink'],
         desc="Tear a rift in space that travels forward at 15 yd per sec, up to 35 yd. Use Spatial Rift again "
              "within 8 sec to step through it to where it is. The cooldown starts when you step through or the "
              "rift closes.", replaces=[81091], cd=120000, gcd=False, script='spell_temucraft_spatial_rift',
         effects=DUMMY_SELF),
    dict(key='SPATIAL_RIFT_OPEN', id=95876, race=None, name='Spatial Rift', rank='',
         icon=['Spell_Shadow_DemonicCircleTeleport', 'Spell_Arcane_Blink'], desc='', dur=8000, gcd=False,
         buff="A rift is open. Use Spatial Rift again to step through it.", script='spell_temucraft_spatial_rift_open',
         effects=[aura(A_DUMMY)]),
    # ---------------------------------------------------------------- Pandaren (13, 14)
    dict(key='QUAKING_PALM', id=95865, race='pandaren', name='Quaking Palm', rank=rank('Combat', 'Offense'),
         icon=['Ability_Gouge', 'Ability_Rogue_KidneyShot'],
         desc="Strike the target's nerve points, incapacitating it for 4 sec. Damage breaks the effect.",
         replaces=[81031, 81059], cd=90000, dur=4000, range='melee', facing=1, dmgclass=DC_MELEE, aif=I_TAKE_DAMAGE,
         visual='gouge', buff="Incapacitated.", effects=[aura(A_STUN, ta=T_ENEMY, mech=M_KNOCKOUT)]),
    # ---------------------------------------------------------------- Dracthyr (25, 26)
    dict(key='WING_BUFFET', id=95866, race='dracthyr', name='Wing Buffet', rank=rank('Combat', 'Defense'),
         icon=['Ability_Druid_Typhoon', 'Spell_Nature_Cyclone'],
         desc="Beat your wings, knocking back enemies in front of you within 10 yd and dealing Physical damage (5 x "
              "your level plus 20% of your attack power or 30% of your spell power).", replaces=[81175, 81179],
         cd=75000, school=PHYS, dmgclass=DC_MAGIC, a1=A1_NO_REFLECTION, visual='thunderstorm',
         script='spell_temucraft_wing_buffet',
         effects=[eff(E_SCHOOL_DAMAGE, ta=T_CONE_ENEMY, radius=10, value=1),
                  eff(E_KNOCK_BACK, ta=T_CONE_ENEMY, radius=10, value=70, misc=200)]),
    dict(key='TAIL_SWIPE', id=95867, race='dracthyr', name='Tail Swipe', rank=rank('Combat', 'Defense'),
         icon=['INV_Misc_MonsterTail_03', 'INV_Misc_MonsterTail_01', 'Ability_Druid_Swipe'],
         desc="Swipe your tail around you, knocking enemies within 8 yd up and away and dealing Physical damage (5 x "
              "your level plus 20% of your attack power or 30% of your spell power).", cd=75000, school=PHYS,
         dmgclass=DC_MAGIC, a1=A1_NO_REFLECTION, visual='war_stomp', script='spell_temucraft_wing_buffet',
         effects=[eff(E_SCHOOL_DAMAGE, ta=T_SRC_CASTER, tb=T_SRC_AREA_ENEMY, radius=8, value=1),
                  eff(E_KNOCK_BACK, ta=T_SRC_CASTER, tb=T_SRC_AREA_ENEMY, radius=8, value=90, misc=120)]),
    dict(key='SOAR', id=95868, race='dracthyr', name='Soar', rank=rank('Utility', 'Travel'),
         icon=['Ability_Druid_FlightForm'],
         desc="Leap into the air and glide: you fall slowly and move 100% faster until you land (up to 20 sec). "
              "Outdoors and out of combat only.", replaces=[81176, 81180], cast=1500, cd=15000,
         a0=A0_ABILITY | A0_OUTDOORS | A0_PEACEFUL, script='spell_temucraft_soar',
         effects=[eff(E_LEAP_BACK, value=250, misc=120)]),
    dict(key='SOAR_GLIDE', id=95869, race=None, name='Soar', rank='', icon=['Ability_Druid_FlightForm'], desc='',
         dur=20000, gcd=False, aif=I_LANDING | I_CHANGE_MAP, visual='slow_fall',
         buff="Gliding. Movement speed increased by 100%.", effects=[aura(A_FEATHER_FALL), aura(A_INCREASE_SPEED, 100)]),
    # ---------------------------------------------------------------- Earthen (27, 28)
    dict(key='AZERITE_SURGE', id=95870, race='earthen', name='Azerite Surge', rank=rank('Combat', 'Offense'),
         icon=['Spell_Fire_Volcano', 'Spell_Fire_SelfDestruct'],
         desc="Release a surge of Azerite in front of you: Fire damage (10 x your level plus 60% of your attack power "
              "or 90% of your spell power) to enemies within 10 yd, 50% more to the healthiest of them, and heals "
              "allies there for the same amount.", replaces=[81115, 81119], cast=1500, cd=90000, school=FIRE,
         dmgclass=DC_MAGIC, a1=A1_NO_REFLECTION, visual='flamestrike', script='spell_temucraft_azerite_surge',
         effects=[eff(E_SCHOOL_DAMAGE, ta=T_CONE_ENEMY, radius=10, value=1),
                  eff(E_HEAL, ta=T_CONE_ALLY, radius=10, value=1)]),
    dict(key='QUIET_CONTEMPLATION', id=95871, race='earthen', name='Quiet Contemplation',
         rank=rank('Utility', 'Survival'), icon=['Spell_Nature_Sleep'],
         desc="Sit in quiet contemplation, restoring 5% of your health and mana every second for 20 sec. Out of "
              "combat only.", buff="Restoring 5% of health and mana every second.", dur=20000,
         a0=A0_ABILITY | A0_PEACEFUL | A0_WHILE_SITTING, aif=I_NOT_SEATED | I_NOT_ABOVEWATER | I_TAKE_DAMAGE,
         visual='food', script='spell_temucraft_quiet_contemplation',
         effects=[aura(A_OBS_MOD_HEALTH, 5, amp=1000), aura(A_OBS_MOD_POWER, 5, 0, amp=1000)]),
    # ---------------------------------------------------------------- Haranir (29, 30)
    dict(key='THORN_BLOOM', id=95872, race='haranir', name='Thorn Bloom', rank=rank('Combat', 'Offense'),
         icon=['Spell_Nature_Thorns'],
         desc="Make thorns bloom at the target spot: up to 8 enemies within 10 yd take Nature damage and more over "
              "12 sec, and up to 8 allies there are healed and healed more over 12 sec (8 x your level plus 40% of "
              "your attack power or 60% of your spell power each, the same again over time).", cd=120000, range=40,
         targets=0x40, school=NATURE, dmgclass=DC_MAGIC, script='spell_temucraft_thorn_bloom',
         effects=[eff(E_DUMMY, ta=T_DEST_DEST, radius=10)]),
    dict(key='THORN_BLOOM_DAMAGE', id=95873, race=None, name='Thorn Bloom', rank='', icon=['Spell_Nature_Thorns'],
         desc='', dur=12000, range=100, gcd=False, school=NATURE, dmgclass=DC_MAGIC, maxtargets=8,
         a1=A1_NO_REFLECTION, buff="Taking Nature damage every 3 sec.", visual='thunderstorm',
         effects=[eff(E_SCHOOL_DAMAGE, ta=T_DEST_DEST, tb=T_DEST_AREA_ENEMY, radius=10, value=1),
                  aura(A_PERIODIC_DAMAGE, 1, ta=T_DEST_DEST, tb=T_DEST_AREA_ENEMY, radius=10, amp=3000)]),
    dict(key='THORN_BLOOM_HEAL', id=95874, race=None, name='Thorn Bloom', rank='',
         icon=['Spell_Nature_Rejuvenation', 'Spell_Nature_Thorns'], desc='', dur=12000, range=100, gcd=False,
         school=NATURE, dmgclass=DC_MAGIC, maxtargets=8, buff="Healing every 3 sec.",
         effects=[eff(E_HEAL, ta=T_DEST_DEST, tb=T_DEST_AREA_ALLY, radius=10, value=1),
                  aura(A_PERIODIC_HEAL, 1, ta=T_DEST_DEST, tb=T_DEST_AREA_ALLY, radius=10, amp=3000)]),
    dict(key='ROOTWALKING', id=95875, race='haranir', name='Rootwalking', rank=rank('Utility', 'Travel'),
         icon=['Spell_Arcane_TeleportMoonglade'],
         desc="A ritual dance along the roots of the world takes you to Nighthaven in Moonglade.", cast=10000,
         cd=600000, a0=A0_ABILITY | A0_PEACEFUL, visual='hearthstone', script='spell_temucraft_rootwalking',
         effects=DUMMY_SELF),
]

# player-facing texts of TemucraftRacialActives.cpp (module_string; the code has the same English as its fallback)
TEXT_MODULE = 'mod-temucraft-racial-actives'
TEXTS = {1: 'The Mole Machine knows no place you have discovered yet.',
         2: 'You have no camp. Use Make Camp first.',
         3: 'Camp made. Return to Camp brings you back here.',
         5: 'You must be falling to call the Pterrordax.',
         10: 'Ironforge', 11: 'Thelsamar, Loch Modan', 12: 'Blackrock Mountain', 13: "Un'Goro Crater",
         14: 'Honor Hold, Hellfire Peninsula', 15: "Blade's Edge Mountains", 16: 'Valiance Keep, Borean Tundra'}

# fixes to mod-worgoblin-high-elf racials, client side (the server side is in that repo and in
# build_racial_bonuses.py's MODS): spell -> icon name (stock 3.3.5)
ICON_FIX = {265223: ['Ability_Rogue_Sprint'], 100001: ['Ability_Rogue_Sprint'],
            273217: ['Ability_Hunter_BeastTraining', 'Ability_Hunter_Pet_Wolf']}
# aura 186 (chance to be hit) where the tooltip says "damage taken": build_racial_bonuses.py MODS has the new values
AURA186_FIX = [81110, 81022, 81126]

# ------------------------------------------------------------------------------------------------------------ DBC
# Spell.dbc columns beyond build_racial_bonuses.py's
CATEGORY, DISPEL, MECHANIC = 1, 2, 3
STANCES, TARGETS, TARGET_CREATURE, SPELL_FOCUS, FACING = 12, 16, 17, 18, 19
RECOVERY, CAT_RECOVERY, INTERRUPT, AURA_INTERRUPT, CHANNEL_INTERRUPT = 29, 30, 31, 32, 33
PROC_FLAGS, PROC_CHANCE, PROC_CHARGES, MAX_LEVEL, BASE_LEVEL, SPELL_LEVEL = 34, 35, 36, 37, 38, 39
POWER, SPEED, VISUAL, ACTIVE_ICON, START_REC_CAT, START_REC_TIME = 41, 47, 131, 134, 205, 206
MAX_TARGETS, DMG_CLASS, PREVENTION, DMG_MULT, SCHOOL = 212, 213, 214, 216, 225


def lookup_tables():
    def load(name):
        d = open(B.SERVER_DBC + '/' + name, 'rb').read()
        _, n, f, rs, _ = struct.unpack('<4s4I', d[:20])
        return [struct.unpack_from('<%dI' % f, d, 20 + i * rs) for i in range(n)]
    first = lambda pairs: {v: k for k, v in sorted(pairs, reverse=True)}   # lowest id per value
    cast = first((r[0], B.i32(r[1])) for r in load('SpellCastTimes.dbc') if not r[2])
    dur = first((r[0], B.i32(r[1])) for r in load('SpellDuration.dbc') if not r[2] and B.i32(r[3]) == B.i32(r[1]))
    radius = first((r[0], round(B.ffrom(r[1]), 2)) for r in load('SpellRadius.dbc'))
    rng = first((r[0], (round(B.ffrom(r[1]), 2), round(B.ffrom(r[3]), 2))) for r in load('SpellRange.dbc')
                if B.ffrom(r[1]) == B.ffrom(r[2]) and B.ffrom(r[3]) == B.ffrom(r[4]))
    return cast, dur, radius, rng


def icon_table(path=None):
    data = open(path, 'rb').read() if path else subprocess.run(
        [sys.executable, B.MPQTOOL, '--get', PTR_PATCH_T, 'DBFilesClient\\SpellIcon.dbc'], capture_output=True).stdout
    dbc = B.DBC(data)
    names = {}
    for i in sorted(dbc.rows):
        if i < 4400:   # the stock 3.3.5 rows (custom ones start at 4400, retail file ids are far above)
            names.setdefault(dbc.s(dbc.rows[i][1]).split('\\')[-1].lower(), i)
    return names


def pick_icon(icons, names):
    for n in names:
        if n.lower() in icons:
            return icons[n.lower()]
    raise SystemExit('no stock icon among %s' % names)


def build_row(dbc, s, tables, icons):
    cast, dur, radius, rng = tables
    row = [0] * 234
    row[0] = s['id']
    a0 = s.get('a0', A0_ABILITY | A0_NO_SHEATH)
    if s.get('passive'):
        a0 = A0_PASSIVE | A0_ABILITY
    if s['race'] is None:
        a0 |= A0_DO_NOT_LOG
        if not s.get('buff'):
            a0 |= A0_HIDDEN
    row[B.ATTR] = a0
    row[B.ATTR_EX] = s.get('a1', 0)
    row[6], row[7] = s.get('a2', 0), s.get('a3', 0)
    row[TARGETS] = s.get('targets', 0)
    row[FACING] = s.get('facing', 0)
    row[B.CAST] = cast[s.get('cast', 0)]
    row[RECOVERY] = s.get('cd', 0)
    row[INTERRUPT] = s.get('iflags', 0xf if s.get('cast') else 0)
    row[AURA_INTERRUPT] = s.get('aif', 0)
    row[PROC_CHANCE] = 101
    d = s.get('dur', -1 if s.get('passive') else 0)
    row[B.DURATION] = dur[d] if d else 0
    r = s.get('range', 0)
    row[B.RANGE] = 1 if not r else 2 if r == 'melee' else rng[(0.0, float(r))]
    row[B.ITEM_CLASS] = B.u(-1)
    if s.get('visual'):
        row[VISUAL] = dbc.rows[VIS[s['visual']]][VISUAL]
    row[B.ICON] = pick_icon(icons, s['icon'])
    gcd = s.get('gcd', True) and s['race'] is not None and not s.get('passive')
    row[START_REC_CAT], row[START_REC_TIME] = (133, 1500) if gcd else (0, 0)
    row[MAX_TARGETS] = s.get('maxtargets', 0)
    row[DMG_CLASS] = s.get('dmgclass', DC_NONE)
    row[PREVENTION] = {DC_NONE: 0, DC_MAGIC: 1, DC_MELEE: 2}[row[DMG_CLASS]]
    for k in range(3):
        row[DMG_MULT + k] = B.fbits(1.0)
    row[SCHOOL] = s.get('school', PHYS)
    for i, e in enumerate(s['effects']):
        row[B.EFFECT + i] = e['effect']
        row[B.DIE + i] = 1
        row[B.PPL + i] = B.fbits(e['ppl'])
        row[B.BP + i] = B.u(e['value'] - 1)
        row[B.MECH + i] = e['mech']
        row[B.TA + i] = e['ta']
        row[B.TB + i] = e['tb']
        row[B.RADIUS + i] = radius[float(e['radius'])] if e['radius'] else 0
        row[B.AURA + i] = e['aura']
        row[B.PERIOD + i] = e['amp']
        row[B.MISC + i] = B.u(e['misc'])
        row[B.MISCB + i] = B.u(e['miscb'])
        row[B.TRIGGER + i] = e['trigger']
        row[B.BONUS + i] = B.fbits(0.0)   # no automatic spell power part: the scripts work the amount out
    B.set_text(dbc, row, B.NAME, s['name'])
    B.set_text(dbc, row, B.RANK, s['rank'])
    B.set_text(dbc, row, B.DESC, s['desc'])
    B.set_text(dbc, row, B.AURADESC, s.get('buff', ''))
    return row


def build_spells(dbc, tables, icons):
    names = {s['id']: s['name'] for s in SPELLS}
    assert len(names) == len(SPELLS)
    for sid in range(FIRST_SPELL, LAST_SPELL + 1):
        assert sid not in dbc.rows or dbc.s(dbc.rows[sid][B.NAME]) == names.get(sid), 'spell %d is taken' % sid
    rows = [build_row(dbc, s, tables, icons) for s in sorted(SPELLS, key=lambda s: s['id'])]
    for r in rows:
        dbc.put(r)
    return rows


def sla_rows(sla):
    """New skill line rows (from FIRST_SLA) and the rows of the replaced copies, now teaching the new spell."""
    new, repointed, replaced, n = [], [], [], FIRST_SLA
    for s in sorted(SPELLS, key=lambda s: s['id']):
        if not s['race']:
            continue
        if s.get('replaces'):
            found = [list(r) for r in sla.rows.values() if r[2] in s['replaces']]
            assert found, 'no skill line row teaches %s' % s['replaces']
            for r in found:
                replaced.append((r[2], s['id'], r[3]))
                r[2] = s['id']
                repointed.append(r)
            continue
        for skill, mask in B.LINE[s['race']]:
            new.append([n, skill, s['id'], mask, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0])
            n += 1
    assert n <= LAST_SLA + 1
    return new, repointed, sorted(replaced)


# ------------------------------------------------------------------------------------------------------------ SQL
def write_sql(dbc, rows, new, repointed):
    cols = B.spell_columns()
    scripts = [(s['id'], s['script']) for s in SPELLS if s.get('script')]
    sla_cols = ('`ID`, `SkillLine`, `Spell`, `RaceMask`, `ClassMask`, `ExcludeRace`, `ExcludeClass`, `MinSkillLineRank`, '
                '`SupercededBySpell`, `AcquireMethod`, `TrivialSkillLineRankHigh`, `TrivialSkillLineRankLow`, '
                '`CharacterPoints_1`, `CharacterPoints_2`')
    old_ids = sorted(r[0] for r in repointed)
    out = ['-- mod-temucraft-classes (racials): racial actives and missing racials from wow-racials_v3, adapted to 3.3.5.',
           '-- New spells %d-%d; new racial skill line rows %d-%d; the skill line rows of the renamed stock copies they'
           % (FIRST_SPELL, LAST_SPELL, FIRST_SLA, LAST_SLA),
           '-- replace (%s) now teach the new spell (the core loads these rows over the DBC ones).'
           % ', '.join(map(str, old_ids)),
           '-- The client needs the same rows (tools/build_racial_actives.py --client). The scripts are in',
           '-- src/racials/TemucraftRacialActives.cpp (Temucraft.Racials.Actives). Made by tools/build_racial_actives.py.',
           '-- Re-runnable.',
           'DELETE FROM `spell_dbc` WHERE `ID` BETWEEN %d AND %d;' % (FIRST_SPELL, LAST_SPELL),
           'INSERT INTO `spell_dbc` (%s) VALUES' % ', '.join('`%s`' % c for c, _ in cols),
           ',\n'.join('(' + ', '.join(B.sql_value(dbc, r[i], k) for i, (c, k) in enumerate(cols)) + ')' for r in rows) + ';',
           '',
           'DELETE FROM `skilllineability_dbc` WHERE `ID` BETWEEN %d AND %d OR `ID` IN (%s);'
           % (FIRST_SLA, LAST_SLA, ', '.join(map(str, old_ids))),
           'INSERT INTO `skilllineability_dbc` (%s) VALUES' % sla_cols,
           ',\n'.join('(' + ', '.join(str(B.i32(x)) for x in r) + ')' for r in sorted(new + repointed)) + ';',
           '',
           'DELETE FROM `spell_script_names` WHERE `spell_id` BETWEEN %d AND %d;' % (FIRST_SPELL, LAST_SPELL),
           'INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES',
           ',\n'.join("(%d, '%s')" % x for x in scripts) + ';',
           '',
           '-- player-facing texts (TemucraftRacialActives.cpp; translations go in module_string_locale)',
           "DELETE FROM `module_string` WHERE `module` = '%s';" % TEXT_MODULE,
           'INSERT INTO `module_string` (`module`, `id`, `string`) VALUES',
           ',\n'.join("('%s', %d, '%s')" % (TEXT_MODULE, i, t.replace("'", "''")) for i, t in sorted(TEXTS.items())) + ';']
    path = os.path.join(MOD, 'data', 'sql', 'db-world', 'base', 'mod_temucraft_racial_actives.sql')
    open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
    print('wrote', path, '-', len(rows), 'spells,', len(new), 'new and', len(repointed), 'repointed skill line rows,',
          len(scripts), 'scripts')
    camp = ['-- mod-temucraft-classes (racials): the Vulpera camp (Make Camp / Return to Camp). Re-runnable.',
            'CREATE TABLE IF NOT EXISTS `mod_temucraft_racial_camp` (',
            '  `guid` INT UNSIGNED NOT NULL,',
            '  `map` SMALLINT UNSIGNED NOT NULL DEFAULT 0,',
            '  `x` FLOAT NOT NULL DEFAULT 0,',
            '  `y` FLOAT NOT NULL DEFAULT 0,',
            '  `z` FLOAT NOT NULL DEFAULT 0,',
            '  `o` FLOAT NOT NULL DEFAULT 0,',
            '  PRIMARY KEY (`guid`)',
            ') ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;']
    path = os.path.join(MOD, 'data', 'sql', 'db-characters', 'base', 'mod_temucraft_racial_camp.sql')
    open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(camp) + '\n')
    print('wrote', path)


def write_header(replaced):
    learned = sorted(s['id'] for s in SPELLS if s['race'])
    lines = ['// Made by tools/build_racial_actives.py from its SPELLS list: change it there, not here.',
             '// The racial actives (and missing racials) from wow-racials_v3, spells %d-%d.' % (FIRST_SPELL, LAST_SPELL),
             '#ifndef TEMUCRAFT_RACIAL_ACTIVES_DATA_H', '#define TEMUCRAFT_RACIAL_ACTIVES_DATA_H', '',
             '#include "Define.h"', '', 'namespace TemucraftRacialActivesData', '{',
             '    constexpr uint32 FIRST_SPELL = %d;' % FIRST_SPELL, '    constexpr uint32 LAST_SPELL = %d;' % LAST_SPELL, '']
    for s in sorted(SPELLS, key=lambda s: s['id']):
        lines.append('    constexpr uint32 SPELL_%s = %d;   // %s%s' % (s['key'], s['id'], s['name'],
                                                                   '' if s['race'] else ' (helper)'))
    lines += ['', '    // what a character learns (its racial skill line teaches them); their cooldowns are final (the',
              '    // racials module\'s cooldown cut skips them)',
              '    constexpr uint32 LEARNED[] = { %s };' % ', '.join(map(str, learned)), '',
              '    // the renamed stock copies they replace: { old, new, race mask of its skill line row } (taken away at',
              '    // login, action bar buttons moved over)',
              '    struct Replaced { uint32 oldSpell; uint32 newSpell; uint32 raceMask; };',
              '    constexpr Replaced REPLACED[] =', '    {']
    lines += ['        { %d, %d, 0x%X },' % x for x in replaced]
    lines += ['    };', '}', '', '#endif', '']
    path = os.path.join(MOD, 'src', 'racials', 'TemucraftRacialActivesData.h')
    open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines))
    print('wrote', path)


# ------------------------------------------------------------------------------------------------------------ main
def client_fixes(spell, icons):
    for sid, names in ICON_FIX.items():
        spell.rows[sid][B.ICON] = pick_icon(icons, names)
    for ids, rk, desc, effects, extra in B.MODS:
        if any(i in AURA186_FIX for i in ids):
            for sid in ids:
                row = list(spell.rows[sid])
                for i, e in effects.items():
                    B.set_effect(row, i, e)
                if rk:
                    B.set_text(spell, row, B.RANK, rk)
                if desc:
                    B.set_text(spell, row, B.DESC, desc)
                    row[B.DESCVAR] = 0
                spell.put(row)


if __name__ == '__main__':
    tables = lookup_tables()
    icons = icon_table(sys.argv[sys.argv.index('--icons') + 1] if '--icons' in sys.argv else None)
    server = B.DBC(open(B.SERVER_DBC + '/Spell.dbc', 'rb').read())
    rows = build_spells(server, tables, icons)
    new, repointed, replaced = sla_rows(B.DBC(open(B.SERVER_DBC + '/SkillLineAbility.dbc', 'rb').read()))
    write_sql(server, rows, new, repointed)
    write_header(replaced)
    del server
    if '--client' in sys.argv:
        base = sys.argv[sys.argv.index('--base') + 1] if '--base' in sys.argv else PTR_BASE
        out = os.path.join(sys.argv[sys.argv.index('--client') + 1], 'DBFilesClient')
        os.makedirs(out, exist_ok=True)
        spell = B.DBC(open(os.path.join(base, 'Spell.dbc'), 'rb').read())
        build_spells(spell, tables, icons)
        client_fixes(spell, icons)
        spell.save(os.path.join(out, 'Spell.dbc'))
        del spell
        sla = B.DBC(open(os.path.join(base, 'SkillLineAbility.dbc'), 'rb').read())
        new, repointed, _ = sla_rows(sla)
        for r in new:
            assert r[0] not in sla.rows or sla.rows[r[0]][2] == r[2], 'skill line row %d is taken' % r[0]
        for r in new + repointed:
            sla.put([B.u(x) for x in r])
        sla.save(os.path.join(out, 'SkillLineAbility.dbc'))
        print('wrote', out, '(Spell.dbc, SkillLineAbility.dbc): pack them into the PTR copy of patch-T.MPQ')
