#!/usr/bin/env python3
"""Opens the weapon and armor skills to every race and class in the client's DBCs, the same way the server does in
memory with AnyWeapon / AnyArmor (TemucraftClassTweaks.cpp): the client then shows the skills and proficiencies as
anyone's (trainers, skill window, tooltips) and AoWoW lists them for every class.

- SkillRaceClassInfo.dbc: rows of the weapon and armor skills get race and class masks 0 (anyone).
- SkillLineAbility.dbc: the proficiency rows that come with those skills (AcquireMethod 2, learned with the skill)
  get race and class masks 0. Rows learned as the skill rises (AcquireMethod 1: Shoot, Auto Shot, Throw, the attacks)
  keep their classes, so each class keeps its own attack, like on the server.

Reads the two files patch-5 holds (default: the client's Patch-5-add\\DBFilesClient folder, or --from <folder>), adds
the SkillLineAbility rows mod-temucraft-classes brings (its racials), and writes the changed copies to client/patch-T.MPQ/DBFilesClient/ in this module. --client also puts them into the
client's patch-T.MPQ (loaded after patch-5, so these copies win; game closed). --weapons-only leaves armor as it is.
"""
import os, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
MOD = os.path.dirname(HERE)
OUT = os.path.join(MOD, 'client', 'patch-T.MPQ', 'DBFilesClient')

# the same lists as the module (TemucraftClassTweaks.cpp)
WEAPON_SKILLS = {43, 44, 45, 46, 54, 55, 118, 136, 160, 172, 173, 176, 226, 228, 229, 473}
ARMOR_SKILLS = {293, 413, 414, 415, 433}
LEARNED_ON_SKILL_LEARN = 2


def read(path):
    d = open(path, 'rb').read()
    magic, n, fc, rs, ss = struct.unpack('<4s4I', d[:20])
    assert magic == b'WDBC', path
    rows = [list(struct.unpack_from('<%dI' % fc, d, 20 + i * rs)) for i in range(n)]
    return rows, fc, rs, d[20 + n * rs:]


def write(path, rows, fc, rs, strings):
    out = bytearray(struct.pack('<4s4I', b'WDBC', len(rows), fc, rs, len(strings)))
    for r in rows:
        out += struct.pack('<%dI' % fc, *r)
    out += strings
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, 'wb').write(out)


def main():
    args = sys.argv[1:]
    src = args[args.index('--from') + 1] if '--from' in args else \
        os.path.join(MOD, '..', '..', '..', 'Modified 335a Client', 'Patch-5-add', 'DBFilesClient')
    skills = WEAPON_SKILLS | (set() if '--weapons-only' in args else ARMOR_SKILLS)

    # SkillRaceClassInfo: ID, SkillID, RaceMask, ClassMask, Flags, MinLevel, SkillTierID, SkillCostIndex
    rows, fc, rs, st = read(os.path.join(src, 'SkillRaceClassInfo.dbc'))
    opened = 0
    for r in rows:
        if r[1] in skills and (r[2] or r[3]):
            r[2] = r[3] = 0
            opened += 1
    write(os.path.join(OUT, 'SkillRaceClassInfo.dbc'), rows, fc, rs, st)
    print('SkillRaceClassInfo: %d rows open to every race and class' % opened)

    # SkillLineAbility: ID, SkillLine, Spell, RaceMask, ClassMask, ExcludeRace, ExcludeClass, MinSkillLineRank,
    # SupercededBySpell, AcquireMethod, ...
    rows, fc, rs, st = read(os.path.join(src, 'SkillLineAbility.dbc'))
    # this copy wins over every other one, so it also carries the rows other modules add (mod-temucraft-classes'
    # racials): rows of their client SkillLineAbility.dbc whose ID patch-5's file doesn't have
    have = {r[0] for r in rows}
    for extra in (os.path.join(MOD, 'client', 'patch-C.MPQ', 'DBFilesClient', 'SkillLineAbility.dbc'),):
        if os.path.exists(extra):
            more, efc, _, _ = read(extra)
            added = [r for r in more if r[0] not in have and efc == fc]
            rows += added
            have |= {r[0] for r in added}
            print('SkillLineAbility: %d rows from %s' % (len(added), os.path.relpath(extra, MOD)))
    rows.sort(key=lambda r: r[0])
    opened = 0
    for r in rows:
        if r[1] in skills and r[9] == LEARNED_ON_SKILL_LEARN and (r[3] or r[4] or r[5] or r[6]):
            r[3] = r[4] = r[5] = r[6] = 0
            opened += 1
    write(os.path.join(OUT, 'SkillLineAbility.dbc'), rows, fc, rs, st)
    print('SkillLineAbility: %d proficiency rows open to every race and class' % opened)
    print('wrote', OUT)

    if '--client' in args:
        sys.path.insert(0, os.path.expanduser('~/tc'))
        import dbctool as T
        mpq = args[args.index('--mpq') + 1] if '--mpq' in args else T.CLIENT_PATCH
        T.client_put({'DBFilesClient\\SkillRaceClassInfo.dbc': os.path.join(OUT, 'SkillRaceClassInfo.dbc'),
                      'DBFilesClient\\SkillLineAbility.dbc': os.path.join(OUT, 'SkillLineAbility.dbc')}, mpq)
        print('put into', mpq)


if __name__ == '__main__':
    main()
