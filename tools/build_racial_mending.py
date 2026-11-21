#!/usr/bin/env python3
"""Builds the racial heals (TemucraftClassChanges.cpp teaches them): every race gets its own take on Cannibalize - a
10 sec channel healing 7% of maximum health every 2 sec, 2 min cooldown - with its own name, icon and animation.
Undead and skeletons keep Cannibalize itself. Spells 100172-100187, cloned from Cannibalize's heal (20578).
Writes data/sql/db-world/base/mod_temucraft_racial_mending.sql; --client also writes the client Spell.dbc."""
import os, sys
sys.path.insert(0, os.path.expanduser('~/tc'))
import dbctool as T

HERE = os.path.dirname(os.path.abspath(__file__))
MOD = os.path.dirname(HERE)
FIRST = 100172
# name, description, visual from spell, icon from spell
THEMES = [
    ('Second Wind',          'Catch your breath over a quick ration.',                      433, 433),     # 100172 human
    ('Blood Rite',           'Draw on the strength of your blood.',                         20572, 20572), # 100173 orc
    ('Hearty Brew',          'A long pull from your trusty flask.',                         430, 20594),   # 100174 dwarf
    ('Moonlit Prayer',       'A quiet prayer to Elune.',                                    64901, 58984), # 100175 night elf
    ('Earthmother\'s Embrace', 'Let the Earthmother mend your wounds.',                     740, 20549),   # 100176 tauren
    ('Tinker\'s Patch',      'Patch yourself up with whatever is in your pockets.',         746, 746),     # 100177 gnome, goblin
    ('Voodoo Mending',       'Old spirits knit your flesh back together.',                  15286, 26297), # 100178 troll
    ('Mana Siphon',          'Draw on stored arcane power to restore yourself.',            28734, 28730), # 100179 blood elf
    ('Crystal Communion',    'Commune with your crystal to restore yourself.',              64843, 28880), # 100180 draenei
    ('Lick Wounds',          'Tend to your wounds like the beast within.',                  136, 136),     # 100181 worgen
    ('Brewmaster\'s Tea',    'A calming cup of tea.',                                       430, 430),     # 100182 pandaren
    ('Campfire Meal',        'A warm meal by the fire.',                                    433, 433),     # 100183 vulpera, tuskarr
    ('Draconic Renewal',     'Your draconic blood renews you.',                             10060, 10060), # 100184 dracthyr
    ('Stone Mending',        'Your stone body seals its cracks.',                           20594, 20594), # 100185 earthen
    ('Root Mending',         'Draw life from the roots beneath you.',                       774, 774),     # 100186 haranir
    ('Tidal Mending',        'The tides wash your wounds away.',                            331, 331),     # 100187 naga
]
LAST = FIRST + len(THEMES) - 1


def build(dbc):
    C = {name: i for i, (name, kind) in enumerate(T.table_columns('spell_dbc'))}
    cannibalize = dbc.rows[20577]
    rows = []
    for n, (name, desc, visual, icon) in enumerate(THEMES):
        s = dbc.clone(20578, FIRST + n)
        s[C['Attributes']] = cannibalize[C['Attributes']]
        s[C['RecoveryTime']] = 120000
        s[C['SpellVisualID_1']] = dbc.rows[visual][C['SpellVisualID_1']]
        s[C['SpellIconID']] = dbc.rows[icon][C['SpellIconID']]
        s[C['Name_Lang_enUS']] = dbc.add(name)
        s[C['NameSubtext_Lang_enUS']] = dbc.add('Racial')
        s[C['Description_Lang_enUS']] = dbc.add(desc + ' Heals 7% of your maximum health every 2 sec for 10 sec.')
        s[C['AuraDescription_Lang_enUS']] = dbc.add('Healing 7% of maximum health every 2 sec.')
        s[C['SpellDescriptionVariableID']] = 0
        rows.append(s)
    for s in rows:
        dbc.put(s)
    return rows


base_dbc = T.DBC(T.SERVER_DBC + '/Spell.dbc')
for i in range(FIRST, LAST + 1):
    assert i not in base_dbc.rows, i
rows = build(base_dbc)
out = os.path.join(MOD, 'data', 'sql', 'db-world', 'base', 'mod_temucraft_racial_mending.sql')
open(out, 'w').write(
    '-- mod-temucraft-class-tweaks: racial heals, a themed Cannibalize for every race (the client has the same rows).\n'
    '-- Made by tools/build_racial_mending.py.\n' +
    'DELETE FROM `spell_dbc` WHERE `ID` BETWEEN %d AND %d;\n' % (FIRST, LAST) +
    T.sql_insert('spell_dbc', rows, base_dbc) + '\n')
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
