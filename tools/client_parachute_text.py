#!/usr/bin/env python3
"""Puts the two-phase Parachute (55001, TemucraftClassTweaks.cpp) into a client Spell.dbc: its tooltip and buff text.

usage: client_parachute_text.py <Spell.dbc> [ready_minutes]

Spell.dbc is the client's (in the Temucraft release: patch-T.MPQ, PTR first); it is changed in place, pack it back
into the MPQ afterwards. Running it again is safe. ready_minutes defaults to the config default
(Temucraft.ClassTweaks.Parachute.ReadyMinutes = 5); the cooldown steps are those of Temucraft.ClassTweaks.Parachute.
The row keeps its 30 sec duration ($d) and 1 min RecoveryTime (the server sets the level-based cooldown)."""
import struct, sys

DESC, AURA = 170, 187                     # Spell.dbc enUS Description / AuraDescription
SPELL = 55001


def texts(minutes):
    desc = ('Readies a parachute for %d min. It opens by itself when you start to fall, slowing your fall for up to $d '
            'or until you land. Used while falling, it opens at once. The cooldown starts when it opens.\n\n'
            'Cooldown by level: 10 min at level 10, 8 min at 20, 6 min at 30, 4 min at 40, 2 min at 50, 1 min from '
            'level 60.' % minutes)
    aura = 'Ready: opens by itself when you start to fall. Once open, falling speed reduced.'
    return desc, aura


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    path = sys.argv[1]
    minutes = int(sys.argv[2]) if len(sys.argv) > 2 else 5
    desc, aura = texts(minutes)
    d = bytearray(open(path, 'rb').read())
    assert d[:4] == b'WDBC'
    n, f, rs, ss = struct.unpack_from('<4I', d, 4)
    strings = bytearray(d[20 + n * rs:])

    def add(text):
        raw = text.encode('utf-8') + b'\0'
        at = strings.find(b'\0' + raw)
        if at >= 0:
            return at + 1
        at = len(strings)
        strings.extend(raw)
        return at

    done = False
    for i in range(n):
        o = 20 + i * rs
        if struct.unpack_from('<I', d, o)[0] == SPELL:
            struct.pack_into('<I', d, o + DESC * 4, add(desc))
            struct.pack_into('<I', d, o + AURA * 4, add(aura))
            done = True
    out = d[:20 + n * rs] + strings
    struct.pack_into('<I', out, 16, len(strings))
    open(path, 'wb').write(out)
    print('set' if done else 'not in this Spell.dbc:', SPELL)


if __name__ == '__main__':
    main()
