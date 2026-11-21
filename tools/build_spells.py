#!/usr/bin/env python3
"""Builds mod-temucraft-new-spells' spell rows from the server's Spell.dbc:
- steady copies of crowd control that breaks on damage (same spell, same cooldown and level, damage doesn't break it)
- Slap
Writes data/sql/db-world/base/mod_temucraft_new_spells_dbc.sql (spell_dbc, server) and src/TemucraftNewSpellsIds.h
(original -> copy). With --client the rows also go into the client's Spell.dbc (patch-T.MPQ, game closed)."""
import os, sys
sys.path.insert(0, os.path.expanduser('~/tc'))
import dbctool as T

HERE = os.path.dirname(os.path.abspath(__file__))
MOD = os.path.dirname(HERE)

ORIGINALS = [118, 12824, 12825, 12826, 28272, 28271, 61305, 61721, 61780,          # Polymorph
             339, 1062, 5195, 5196, 9852, 9853, 26989, 53308,                      # Entangling Roots
             2637, 18657, 18658,                                                   # Hibernate
             6770, 2070, 11297, 51724,                                             # Sap
             1776, 2094, 20066, 51514,                                             # Gouge, Blind, Repentance, Hex
             9484, 9485, 10955,                                                    # Shackle Undead
             19503]                                                                # Scatter Shot
SLAP = None   # first free id after the copies
HEROIC_LEAP, REDIRECT, BARRIER = 6544, 73981, 81782
F_AURA_INTERRUPT, F_PROC_MASK, F_PROC_CHANCE, F_PROC_CHARGES = 32, 34, 35, 36
F_NAME, F_RANK, F_DESC = 136, 153, 170
TAKE_DAMAGE = 0x2


def build(dbc, base):
    """base: the server Spell.dbc (for picking free ids the same way on both sides)"""
    free = [i for i in range(96000, 96199) if i not in base.rows]
    pairs, rows = [], []
    for n, orig in enumerate(ORIGINALS):
        new_id = free[n]
        r = dbc.clone(orig, new_id)
        r[F_AURA_INTERRUPT] &= ~TAKE_DAMAGE
        r[F_PROC_MASK] = r[F_PROC_CHANCE] = r[F_PROC_CHARGES] = 0
        rank = dbc.s(r[F_RANK])
        r[F_RANK] = dbc.add((rank + ' ' if rank else '') + '(Steady)')
        desc = dbc.s(r[F_DESC])
        r[F_DESC] = dbc.add(desc + '\n\nSteady: damage does not break it. Shares its cooldown with the usual version.')
        dbc.put(r)
        rows.append(r)
        pairs.append((orig, new_id))

    slap = free[len(ORIGINALS)] if SLAP is None else SLAP
    C = {name: i for i, (name, kind) in enumerate(T.table_columns('spell_dbc'))}
    src = dbc.rows[633]                          # Lay on Hands: for the hand icon; everything else set here
    s = [0] * dbc.fc
    s[0] = slap
    s[C['CastingTimeIndex']] = 1                 # instant
    s[C['RecoveryTime']] = 30000                 # 30 sec cooldown
    s[C['BaseLevel']] = s[C['SpellLevel']] = 1
    s[C['PowerType']] = 0                        # mana
    s[C['ManaCostPct']] = 5                      # 5% of base mana
    s[C['RangeIndex']] = 2                       # melee range
    s[C['Effect_1']] = 3                         # dummy: the script wakes the target and attacks enemies
    s[C['ImplicitTargetA_1']] = 25               # any unit (friend or foe)
    s[C['SpellIconID']] = src[C['SpellIconID']]
    s[C['Name_Lang_enUS']] = dbc.add('Slap')
    s[C['Name_Lang_Mask']] = src[C['Name_Lang_Mask']]
    s[C['NameSubtext_Lang_Mask']] = src[C['NameSubtext_Lang_Mask']]
    s[C['Description_Lang_enUS']] = dbc.add('Slap a sleeping player awake, friend or foe. An enemy also takes your basic attack.')
    s[C['Description_Lang_Mask']] = src[C['Description_Lang_Mask']]
    s[C['AuraDescription_Lang_Mask']] = src[C['AuraDescription_Lang_Mask']]
    s[C['StartRecoveryCategory']] = 133          # global cooldown
    s[C['StartRecoveryTime']] = 1500
    s[C['DefenseType']] = 2                      # melee
    s[C['SchoolMask']] = 1                       # physical
    s[C['EffectMultipleValue_1']] = T.f2u(1.0)
    s[C['EffectBonusMultiplier_1']] = T.f2u(1.0)
    s[C['EffectChainAmplitude_1']] = T.f2u(1.0)
    dbc.put(s)
    rows.append(s)

    # Rune of Power (mages): a rune at your feet for 30 sec; standing in it (4 yards) gives 10% more spell damage
    rune = list(dbc.rows[61975])                 # the Ulduar rune: persistent area aura with its ground visual
    rune[0] = free[len(ORIGINALS) + 1]
    rune[C['Attributes']] = 0
    rune[C['AttributesEx']] = 0
    rune[C['RecoveryTime']] = 45000
    rune[C['DurationIndex']] = 9                 # 30 sec
    rune[C['EffectRadiusIndex_1']] = 26          # 4 yards
    rune[C['EffectBasePoints_1']] = 9            # 10%
    rune[C['EffectMiscValue_1']] = 126           # magic schools
    rune[C['PowerType']] = 0
    rune[C['ManaCostPct']] = 5
    rune[C['BaseLevel']] = rune[C['SpellLevel']] = 50
    rune[C['StartRecoveryCategory']] = 133
    rune[C['StartRecoveryTime']] = 1500
    rune[C['Name_Lang_enUS']] = dbc.add('Rune of Power')
    rune[C['Description_Lang_enUS']] = dbc.add('Places a Rune of Power on the ground for 30 sec. You and allies standing within 4 yards of it deal 10% more spell damage.')
    rune[C['AuraDescription_Lang_enUS']] = dbc.add('Spell damage increased by 10%.')
    for k in ('Name_Lang_Mask', 'Description_Lang_Mask', 'AuraDescription_Lang_Mask'):
        rune[C[k]] = src[C[k]]
    dbc.put(rune)
    rows.append(rune)

    # Call of the Pack (hunters): party and raid within 25 yards run 40% faster for 8 sec, no daze, 5 min
    pack = list(dbc.rows[77761])                 # Stampeding Roar: the raid speed boost
    pack[0] = free[len(ORIGINALS) + 2]
    pack[C['RecoveryTime']] = 300000
    pack[C['EffectBasePoints_1']] = 39           # 40%
    pack[C['EffectRadiusIndex_1']] = 20          # 25 yards
    pack[C['SpellIconID']] = dbc.rows[13159][C['SpellIconID']]
    pack[C['PowerType']] = 0
    pack[C['ManaCostPct']] = 3
    pack[C['BaseLevel']] = pack[C['SpellLevel']] = 40
    pack[C['StartRecoveryCategory']] = 133
    pack[C['StartRecoveryTime']] = 1500
    pack[C['Name_Lang_enUS']] = dbc.add('Call of the Pack')
    pack[C['Description_Lang_enUS']] = dbc.add('Party and raid members within 25 yards run 40% faster for 8 sec.')
    pack[C['AuraDescription_Lang_enUS']] = dbc.add('Movement speed increased by 40%.')
    for k in ('Name_Lang_Mask', 'Description_Lang_Mask', 'AuraDescription_Lang_Mask'):
        pack[C[k]] = src[C[k]]
    dbc.put(pack)
    rows.append(pack)

    # Talents our changes emptied. Tactical Mastery kept rage on stance changes, but warriors no longer need stances
    # (every ability works in any stance): it now shortens Bloodrage and Berserker Rage (threat part unchanged).
    for rank_id, secs in ((12295, 5), (12676, 10), (12677, 15)):
        t = list(dbc.rows[rank_id])
        t[C['EffectAura_1']] = 107                   # flat spell modifier
        t[C['EffectMiscValue_1']] = 11               # cooldown
        t[C['EffectBasePoints_1']] = (-secs * 1000 - 1) & 0xFFFFFFFF
        t[C['EffectDieSides_1']] = 1
        t[C['EffectSpellClassMaskA_1']] = 0x10000100  # Bloodrage, Berserker Rage
        t[C['EffectSpellClassMaskB_1']] = 0
        t[C['EffectSpellClassMaskC_1']] = 0
        old = dbc.s(t[C['Description_Lang_enUS']])
        tail = old[old.index('Also'):] if 'Also' in old else ''
        t[C['Description_Lang_enUS']] = dbc.add('Reduces the cooldown of your Bloodrage and Berserker Rage abilities by $/1000;s1 sec.  ' + tail)
        dbc.put(t)
        rows.append(t)

    # existing spells made learnable: Heroic Leap (warriors), Redirect (rogues), Power Word: Barrier (discipline priests)
    leap = list(dbc.rows[HEROIC_LEAP])
    leap[C['RecoveryTime']] = 45000
    dbc.put(leap)
    rows.append(leap)

    shield = dbc.rows[17]                        # Power Word: Shield, for the look
    b = list(dbc.rows[BARRIER])
    b[C['Attributes']] = 0x00010000              # not usable while dead is not set; plain castable spell
    b[C['AttributesEx']] = 0
    b[C['AuraInterruptFlags']] = 0
    b[C['CastingTimeIndex']] = 1
    b[C['RecoveryTime']] = 180000
    b[C['DurationIndex']] = 1                    # 10 sec
    b[C['RangeIndex']] = 1                       # self
    b[C['PowerType']] = 0
    b[C['ManaCostPct']] = 15
    b[C['StartRecoveryCategory']] = 133
    b[C['StartRecoveryTime']] = 1500
    b[C['BaseLevel']] = b[C['SpellLevel']] = 60
    for n in (1, 2):
        b[C['ImplicitTargetA_%d' % n]] = 56      # caster's raid around them
        b[C['ImplicitTargetB_%d' % n]] = 0
        b[C['EffectRadiusIndex_%d' % n]] = 13    # 10 yards
    b[C['SpellVisualID_1']] = shield[C['SpellVisualID_1']]
    b[C['Description_Lang_enUS']] = dbc.add('Shields all party and raid members within 10 yards, reducing damage taken by 25% and the spell pushback they suffer, for 10 sec.')
    b[C['Description_Lang_Mask']] = shield[C['Description_Lang_Mask']]
    b[C['AuraDescription_Lang_enUS']] = dbc.add('Damage taken reduced by 25%.')
    b[C['AuraDescription_Lang_Mask']] = shield[C['AuraDescription_Lang_Mask']]
    dbc.put(b)
    rows.append(b)
    return pairs, rows


base = T.DBC(T.SERVER_DBC + '/Spell.dbc')
server = T.DBC(T.SERVER_DBC + '/Spell.dbc')
pairs, rows = build(server, base)
open(MOD + '/data/sql/db-world/base/mod_temucraft_new_spells_dbc.sql', 'w', encoding='utf-8').write(
    '-- mod-temucraft-new-spells: the new spells for the server (the client has the same rows in Spell.dbc).\n' +
    '-- Built by tools/build_spells.py from the server Spell.dbc.\n' + T.sql_insert('spell_dbc', rows, server))
with open(MOD + '/src/TemucraftNewSpellsIds.h', 'w') as f:
    f.write('// Built by tools/build_spells.py: crowd control spell -> its steady copy.\n#pragma once\n#include <utility>\n')
    f.write('constexpr unsigned SPELL_SLAP = %d, SPELL_RUNE_OF_POWER = %d, SPELL_CALL_OF_THE_PACK = %d;\n' % tuple([r[0] for r in rows if r[0] >= 96000][-3:]))
    f.write('constexpr unsigned SPELL_HEROIC_LEAP = %d, SPELL_REDIRECT = %d, SPELL_BARRIER = %d;\n' % (HEROIC_LEAP, REDIRECT, BARRIER))
    f.write('constexpr std::pair<unsigned, unsigned> STEADY_COPIES[] = {\n')
    for o, c in pairs:
        f.write('    { %d, %d },\n' % (o, c))
    f.write('};\n')
if '--client' in sys.argv:
    tmp = '/tmp/Spell_client.dbc'
    assert T.client_get('DBFilesClient\\Spell.dbc', tmp)
    client = T.DBC(tmp)
    build(client, base)
    client.save(tmp)
    T.client_put({'DBFilesClient\\Spell.dbc': tmp})
    print('client Spell.dbc updated')
print(len(pairs), 'steady copies + Slap')
