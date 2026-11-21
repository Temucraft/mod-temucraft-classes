#!/usr/bin/env python3
"""Builds three new class spells (TemucraftNewClassSpells.cpp teaches them):
- 100110-100116 Overloaded Blast (mage, ranks at 32/40/48/56/64/72/80): instant Arcane damage, 8 sec cooldown, only
  while Overloaded (a spell of yours was fully or partly resisted), can't be resisted.
- 100117 Overloaded (5 sec, shows when Overloaded Blast can be used), 100118 Overload Sense (hidden passive that procs it)
- 100119-100125 Elune's Arrow (night elf priests with a bow, ranks at 20/32/44/58/66/72/78): weapon damage plus Fire.
- 100126 Stonewall Slam (shaman with a shield, level 20): taunts the target, 5 yd, 10 sec cooldown.
Rows are cloned from stock spells for their visuals (Arcane Barrage, Aimed Shot, Taunt). Writes
data/sql/db-world/base/mod_temucraft_new_class_spells.sql; --client also writes the client Spell.dbc (game closed)."""
import os, sys
sys.path.insert(0, os.path.expanduser('~/tc'))
import dbctool as T

HERE = os.path.dirname(os.path.abspath(__file__))
MOD = os.path.dirname(HERE)
FIRST, LAST = 100110, 100126
BLAST = list(range(100110, 100117)); OVERLOADED, SENSE = 100117, 100118
ARROW = list(range(100119, 100126)); SLAM = 100126
BLAST_RANKS = [(32, 202, 244), (40, 290, 349), (48, 398, 474), (56, 517, 612), (64, 660, 780), (72, 820, 970), (80, 1000, 1180)]
ARROW_RANKS = [(20, 14, 18), (32, 25, 29), (44, 37, 42), (58, 51, 57), (66, 68, 74), (72, 86, 94), (78, 105, 115)]


def build(dbc):
    C = {name: i for i, (name, kind) in enumerate(T.table_columns('spell_dbc'))}
    mask = dbc.rows[633]

    def texts(s, name, rank, desc, aura=''):
        for k in ('Name_Lang_Mask', 'NameSubtext_Lang_Mask', 'Description_Lang_Mask', 'AuraDescription_Lang_Mask'):
            s[C[k]] = mask[C[k]]
        s[C['Name_Lang_enUS']] = dbc.add(name)
        s[C['NameSubtext_Lang_enUS']] = dbc.add(rank) if rank else 0
        s[C['Description_Lang_enUS']] = dbc.add(desc)
        s[C['AuraDescription_Lang_enUS']] = dbc.add(aura) if aura else 0
        s[C['SpellDescriptionVariableID']] = 0

    def clear_effect(s, n):
        for k in ('Effect', 'EffectDieSides', 'EffectBasePoints', 'EffectAura', 'EffectAuraPeriod', 'EffectMiscValue',
                  'EffectMiscValueB', 'EffectTriggerSpell', 'ImplicitTargetA', 'ImplicitTargetB', 'EffectRadiusIndex',
                  'EffectSpellClassMaskA', 'EffectSpellClassMaskB', 'EffectSpellClassMaskC', 'EffectMechanic',
                  'EffectChainTargets', 'EffectItemType'):
            s[C['%s_%d' % (k, n)]] = 0
        for k in ('EffectRealPointsPerLevel', 'EffectPointsPerCombo'):
            s[C['%s_%d' % (k, n)]] = T.f2u(0.0)

    def damage(s, n, lo, hi):
        s[C['EffectBasePoints_%d' % n]] = (lo - 1) & 0xFFFFFFFF
        s[C['EffectDieSides_%d' % n]] = hi - lo + 1
        s[C['EffectRealPointsPerLevel_%d' % n]] = T.f2u(0.0)

    def classless(s, family):
        s[C['SpellClassSet']] = family
        for i in (1, 2, 3):
            s[C['SpellClassMask_%d' % i]] = 0

    rows = []
    for r, (spell_id, (lvl, lo, hi)) in enumerate(zip(BLAST, BLAST_RANKS), 1):
        s = dbc.clone(44425, spell_id)              # Arcane Barrage
        texts(s, 'Overloaded Blast', 'Rank %d' % r,
              'Blast the target with overloaded mana for $s1 Arcane damage. Only usable after one of your spells is '
              'fully or partly resisted. Can\'t be resisted.')
        classless(s, 3)
        damage(s, 1, lo, hi)
        s[C['BaseLevel']] = s[C['SpellLevel']] = lvl
        s[C['MaxLevel']] = 0
        s[C['RecoveryTime']] = 8000
        s[C['CategoryRecoveryTime']] = 0
        s[C['Category']] = 0
        s[C['ManaCost']] = 0
        s[C['ManaCostPct']] = 6
        s[C['CasterAuraSpell']] = OVERLOADED
        s[C['AttributesEx4']] |= 0x1                # can't be resisted
        s[C['AttributesEx5']] |= 0x00800000         # no partial resists
        rows.append(s)

    s = dbc.clone(44425, OVERLOADED)
    texts(s, 'Overloaded', '', 'Overloaded Blast can be used.', 'Overloaded Blast can be used.')
    classless(s, 3)
    for n in (1, 2, 3):
        clear_effect(s, n)
    s[C['Effect_1']] = 6; s[C['EffectAura_1']] = 4; s[C['ImplicitTargetA_1']] = 1
    s[C['EffectMultipleValue_1']] = T.f2u(1.0); s[C['EffectBonusMultiplier_1']] = T.f2u(1.0)
    s[C['RangeIndex']] = 1; s[C['RecoveryTime']] = 0; s[C['ManaCost']] = 0; s[C['ManaCostPct']] = 0
    s[C['StartRecoveryCategory']] = 0; s[C['StartRecoveryTime']] = 0; s[C['Speed']] = T.f2u(0.0)
    s[C['SpellVisualID_1']] = 0; s[C['DefenseType']] = 0; s[C['BaseLevel']] = s[C['SpellLevel']] = 1
    rows.append(s)

    s = list(s); s[0] = SENSE
    texts(s, 'Overload Sense', '', 'A resisted spell overloads you.')
    s[C['Attributes']] = 0x40 | 0x80          # passive, hidden
    s[C['DurationIndex']] = 21
    s[C['EffectAura_1']] = 42; s[C['EffectTriggerSpell_1']] = OVERLOADED   # proc: trigger spell
    s[C['ProcTypeMask']] = 0x10000            # done magic spell damage
    s[C['ProcChance']] = 101
    rows.append(s)

    for r, (spell_id, (lvl, lo, hi)) in enumerate(zip(ARROW, ARROW_RANKS), 1):
        s = dbc.clone(19434, spell_id)              # Aimed Shot
        texts(s, 'Elune\'s Arrow', 'Rank %d' % r, 'Fire a moonlit arrow that deals weapon damage plus $s1 Fire damage.')
        classless(s, 6)
        clear_effect(s, 2); clear_effect(s, 3)
        damage(s, 1, lo, hi)
        s[C['SchoolMask']] = 4
        s[C['EquippedItemSubclass']] = 4           # bows
        s[C['BaseLevel']] = s[C['SpellLevel']] = lvl
        s[C['MaxLevel']] = 0
        s[C['CastingTimeIndex']] = 1
        s[C['RecoveryTime']] = 10000
        s[C['CategoryRecoveryTime']] = 0; s[C['Category']] = 0
        s[C['PowerType']] = 0; s[C['ManaCost']] = 0; s[C['ManaCostPct']] = 5
        rows.append(s)

    s = dbc.clone(355, SLAM)                        # Taunt
    texts(s, 'Stonewall Slam', '', 'Slam the target with your shield, taunting it. No effect if it is already attacking you.')
    classless(s, 11)
    s[C['RangeIndex']] = 2                          # melee
    s[C['EquippedItemClass']] = 4; s[C['EquippedItemSubclass']] = 64; s[C['EquippedItemInvTypes']] = 1 << 14
    s[C['RecoveryTime']] = 10000; s[C['CategoryRecoveryTime']] = 0; s[C['Category']] = 0
    s[C['PowerType']] = 0; s[C['ManaCost']] = 0; s[C['ManaCostPct']] = 3
    s[C['BaseLevel']] = s[C['SpellLevel']] = 20
    s[C['Stances'] if 'Stances' in C else C['ShapeshiftMask']] = 0
    rows.append(s)
    # Overloaded lasts 5 sec
    d5 = [i for i, r in DUR.rows.items() if r[1] == 5000 and r[3] == 5000][0]
    rows[len(BLAST)][C['DurationIndex']] = d5
    for s in rows:
        dbc.put(s)
    return rows


DUR = T.DBC(T.SERVER_DBC + '/SpellDuration.dbc')
base_dbc = T.DBC(T.SERVER_DBC + '/Spell.dbc')
for i in range(FIRST, LAST + 1):
    assert i not in base_dbc.rows, i
rows = build(base_dbc)
ranks = []
for chain in (BLAST, ARROW):
    for r, sid in enumerate(chain, 1):
        ranks.append('(%d, %d, %d)' % (chain[0], sid, r))
out = os.path.join(MOD, 'data', 'sql', 'db-world', 'base', 'mod_temucraft_new_class_spells.sql')
open(out, 'w').write(
    '-- mod-temucraft-class-tweaks: Overloaded Blast (mage), Elune\'s Arrow (night elf priest), Stonewall Slam (shaman).\n'
    '-- The client has the same rows in Spell.dbc. Made by tools/build_new_spells.py.\n' +
    'DELETE FROM `spell_dbc` WHERE `ID` BETWEEN %d AND %d;\n' % (FIRST, LAST) +
    T.sql_insert('spell_dbc', rows, base_dbc) + '\n' +
    'DELETE FROM `spell_ranks` WHERE `first_spell_id` IN (%d, %d);\n' % (BLAST[0], ARROW[0]) +
    'INSERT INTO `spell_ranks` (`first_spell_id`, `spell_id`, `rank`) VALUES\n' + ',\n'.join(ranks) + ';\n' +
    'DELETE FROM `spell_bonus_data` WHERE `entry` IN (%d, %d);\n' % (BLAST[0], ARROW[0]) +
    'INSERT INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES\n'
    "(%d, 0.65, 0, 0, 0, 'Temucraft - Overloaded Blast'),\n(%d, 0.33, 0, 0, 0, 'Temucraft - Elune''s Arrow');\n" % (BLAST[0], ARROW[0]) +
    'DELETE FROM `spell_proc` WHERE `SpellId` = %d;\n' % SENSE +
    'INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `Chance`) VALUES\n'
    '(%d, 0, 3, 0x10000, 7, 2, 0xB, 100);\n' % SENSE +
    'DELETE FROM `spell_script_names` WHERE `spell_id` = %d;\n' % SENSE +
    "INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (%d, 'spell_temucraft_overload_sense');\n" % SENSE)
print('wrote', out)

if '--client' in sys.argv:
    tmp = '/tmp/Spell_client.dbc'
    assert T.client_get('DBFilesClient\\Spell.dbc', tmp)
    client = T.DBC(tmp)
    for i in range(FIRST, LAST + 1):
        assert i not in client.rows, i
    build(client)
    client.save(tmp)
    T.client_put({'DBFilesClient\\Spell.dbc': tmp})
    os.remove(tmp)
    print('client Spell.dbc updated')
