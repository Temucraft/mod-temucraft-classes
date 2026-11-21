#!/usr/bin/env python3
"""Builds the spell rows of TemucraftClassChanges.cpp from the server's Spell.dbc:
- 96400 Wildkin Frenzy (Moonkin Form hit): +5% spell damage, +10% attack power, no spell pushback, 8 sec
- 96401 Tiger's Bloodlust (bleeds during Tiger's Fury): +1% physical damage per stack, 10 stacks
- 96402 Light's Reserve (Judgement of Light at full health): +5% Judgement of Light healing per stack, 20 stacks
- 96403 Overwhelming Light: the heal on nearby allies (amount given by the script)
- 96404 Empowered Infernal: immune to all schools, 6 sec (Demonic Empowerment)
- 96405 Empowered Thrall: 30% attack and casting speed, 15 sec (Demonic Empowerment on an enslaved demon)
Writes data/sql/db-world/base/mod_temucraft_class_changes_spell_dbc.sql. With --client the rows also go into the client's
Spell.dbc (patch-T.MPQ, game closed)."""
import os, sys
sys.path.insert(0, os.path.expanduser('~/tc'))
import dbctool as T

HERE = os.path.dirname(os.path.abspath(__file__))
MOD = os.path.dirname(HERE)
FIRST, LAST = 96400, 96405
ATTR3_IGNORE_CASTER_MODIFIERS = 0x20000000

# id: name, description, aura text, icon from, duration index, stacks
SPELLS = {
    96400: ('Wildkin Frenzy', 'Taking a hit in Moonkin Form drives you into a frenzy.',
            'Spell damage increased by 5%, attack power by 10%. Casting can\'t be pushed back.', 24858, 31, 0),
    96401: ('Tiger\'s Bloodlust', 'Bleeds during Tiger\'s Fury feed your hunger.',
            'Physical damage increased by $s1%.', 5217, 85, 10),
    96402: ('Light\'s Reserve', 'Judgement of Light that found you at full health is saved for later.',
            'Healing from Judgement of Light increased by $s1%.', 20185, 4, 20),
    96403: ('Overwhelming Light', 'Light overflowing from a full heal.', '', 635, 0, 0),
    96404: ('Empowered Infernal', 'Immune to all attacks and spells.', 'Immune to all attacks and spells.', 47193, 32, 0),
    96405: ('Empowered Thrall', 'Attack and casting speed increased.', 'Attack and casting speed increased by 30%.', 47193, 8, 0),
}


def build(dbc):
    C = {name: i for i, (name, kind) in enumerate(T.table_columns('spell_dbc'))}
    mask = dbc.rows[633]

    def base(spell_id):
        name, desc, aura, icon, duration, stacks = SPELLS[spell_id]
        s = [0] * dbc.fc
        s[0] = spell_id
        s[C['CastingTimeIndex']] = 1
        s[C['DurationIndex']] = duration
        s[C['RangeIndex']] = 1
        s[C['BaseLevel']] = s[C['SpellLevel']] = 1
        s[C['SpellIconID']] = dbc.rows[icon][C['SpellIconID']]
        s[C['SchoolMask']] = 1
        s[C['CumulativeAura']] = stacks
        for k in ('Name_Lang_Mask', 'NameSubtext_Lang_Mask', 'Description_Lang_Mask', 'AuraDescription_Lang_Mask'):
            s[C[k]] = mask[C[k]]
        s[C['Name_Lang_enUS']] = dbc.add(name)
        s[C['Description_Lang_enUS']] = dbc.add(desc)
        if aura:
            s[C['AuraDescription_Lang_enUS']] = dbc.add(aura)
        return s

    def effect(s, n, kind, aura, amount, misc=0, target=1):
        s[C['Effect_%d' % n]] = kind
        s[C['EffectAura_%d' % n]] = aura
        s[C['EffectBasePoints_%d' % n]] = (amount - 1) & 0xFFFFFFFF
        s[C['EffectDieSides_%d' % n]] = 1
        s[C['EffectMiscValue_%d' % n]] = misc
        s[C['ImplicitTargetA_%d' % n]] = target
        s[C['EffectMultipleValue_%d' % n]] = T.f2u(1.0)
        s[C['EffectBonusMultiplier_%d' % n]] = T.f2u(1.0)
        s[C['EffectChainAmplitude_%d' % n]] = T.f2u(1.0)

    APPLY, HEAL = 6, 10
    rows = []
    s = base(96400)
    effect(s, 1, APPLY, 79, 5, 126)        # spell schools
    effect(s, 2, APPLY, 166, 10)           # attack power %
    effect(s, 3, APPLY, 149, 100)          # pushback
    rows.append(s)
    s = base(96401); effect(s, 1, APPLY, 79, 1, 1); rows.append(s)
    s = base(96402); effect(s, 1, APPLY, 4, 5); rows.append(s)
    s = base(96403)
    s[C['SchoolMask']] = 2
    s[C['RangeIndex']] = 6
    s[C['AttributesEx3']] = ATTR3_IGNORE_CASTER_MODIFIERS
    effect(s, 1, HEAL, 0, 1, 0, 21)        # target ally
    rows.append(s)
    s = base(96404); effect(s, 1, APPLY, 39, 1, 127); rows.append(s)
    s = base(96405); effect(s, 1, APPLY, 138, 30); effect(s, 2, APPLY, 65, 30); rows.append(s)
    for s in rows:
        dbc.put(s)
    return rows


base_dbc = T.DBC(T.SERVER_DBC + '/Spell.dbc')
for i in range(FIRST, LAST + 1):
    assert i not in base_dbc.rows, i
rows = build(base_dbc)
out = os.path.join(MOD, 'data', 'sql', 'db-world', 'base', 'mod_temucraft_class_changes_spell_dbc.sql')
open(out, 'w').write(
    '-- mod-temucraft-class-tweaks: spells of the class changes (the client has the same rows in Spell.dbc).\n' +
    '-- Made by tools/build_class_change_spells.py.\n' +
    'DELETE FROM `spell_dbc` WHERE `ID` BETWEEN %d AND %d;\n' % (FIRST, LAST) +
    T.sql_insert('spell_dbc', rows, base_dbc) + '\n' +
    'DELETE FROM `spell_bonus_data` WHERE `entry` = 96403;\n' +
    'INSERT INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES\n' +
    "(96403, 0, 0, 0, 0, 'Temucraft - Overwhelming Light (amount from the script)');\n")
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
