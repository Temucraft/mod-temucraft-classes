// Made by tools/build_racial_bonuses.py from its NEW and MODS lists: change it there, not here.
// What Temucraft.Racials.BestBonuses changes in existing racial spells, and the spells it adds.
#ifndef TEMUCRAFT_RACIAL_BONUSES_DATA_H
#define TEMUCRAFT_RACIAL_BONUSES_DATA_H

#include "Define.h"

namespace TemucraftRacialBonusesData
{
    // value: the effect's amount; perLevel: the amount is the character's level instead (resistances)
    struct EffectEdit { uint32 spell; uint8 index; uint32 aura; int32 value; int32 misc; bool perLevel; uint32 family; uint32 familyMask; };

    constexpr uint32 FIRST_SPELL = 95800;
    constexpr uint32 LAST_SPELL = 95834;

    // the new racials a character learns (the others in the range are hidden helpers and a buff)
    constexpr uint32 LEARNED[] = { 95800, 95801, 95802, 95803, 95804, 95805, 95806, 95807, 95808, 95809, 95810, 95811, 95812, 95814, 95815, 95816, 95817, 95822, 95823, 95824, 95825, 95826, 95827, 95828, 95829, 95830, 95831, 95832, 95833 };

    constexpr EffectEdit EFFECTS[] =
    {
        { 20597, 0, 240, 5, 0, false, 0, 0 },
        { 20597, 1, 30, 5, 43, false, 0, 0 },
        { 20597, 2, 30, 5, 55, false, 0, 0 },
        { 20864, 0, 240, 5, 0, false, 0, 0 },
        { 20864, 1, 30, 5, 54, false, 0, 0 },
        { 20864, 2, 30, 5, 160, false, 0, 0 },
        { 20595, 1, 30, 5, 46, false, 0, 0 },
        { 20596, 0, 186, -2, 16, false, 0, 0 },
        { 20596, 1, 22, 0, 16, true, 0, 0 },
        { 20596, 2, 87, -1, 16, false, 0, 0 },
        { 20582, 2, 49, 2, 0, false, 0, 0 },
        { 20583, 0, 186, -2, 8, false, 0, 0 },
        { 20583, 1, 22, 0, 8, true, 0, 0 },
        { 20583, 2, 87, -1, 8, false, 0, 0 },
        { 21009, 1, 129, 2, 0, false, 0, 0 },
        { 21009, 2, 129, 0, 0, false, 0, 0 },
        { 5227, 0, 82, 0, 0, false, 0, 0 },
        { 20579, 0, 186, -2, 32, false, 0, 0 },
        { 20579, 1, 22, 0, 32, true, 0, 0 },
        { 20579, 2, 87, -1, 32, false, 0, 0 },
        { 20550, 0, 133, 5, 0, false, 0, 0 },
        { 20550, 1, 54, 1, 0, false, 0, 0 },
        { 20550, 2, 55, 1, 127, false, 0, 0 },
        { 20551, 0, 186, -2, 8, false, 0, 0 },
        { 20551, 1, 22, 0, 8, true, 0, 0 },
        { 20551, 2, 87, -1, 8, false, 0, 0 },
        { 20591, 1, 132, 5, 0, false, 0, 0 },
        { 20592, 0, 186, -2, 64, false, 0, 0 },
        { 20592, 1, 22, 0, 64, true, 0, 0 },
        { 20592, 2, 87, -1, 64, false, 0, 0 },
        { 20555, 0, 88, 20, 0, false, 0, 0 },
        { 20555, 1, 116, 20, 0, false, 0, 0 },
        { 20558, 1, 30, 5, 176, false, 0, 0 },
        { 26290, 1, 30, 5, 45, false, 0, 0 },
        { 822, 1, 22, 5, 126, false, 0, 0 },
        { 28875, 0, 98, 10, 755, false, 0, 0 },
        { 6562, 2, 137, 1, -1, false, 0, 0 },
        { 28878, 2, 137, 1, -1, false, 0, 0 },
        { 59221, 0, 186, -2, 32, false, 0, 0 },
        { 59221, 1, 22, 0, 32, true, 0, 0 },
        { 59221, 2, 87, -1, 32, false, 0, 0 },
        { 59535, 0, 186, -2, 32, false, 0, 0 },
        { 59535, 1, 22, 0, 32, true, 0, 0 },
        { 59535, 2, 87, -1, 32, false, 0, 0 },
        { 59536, 0, 186, -2, 32, false, 0, 0 },
        { 59536, 1, 22, 0, 32, true, 0, 0 },
        { 59536, 2, 87, -1, 32, false, 0, 0 },
        { 59538, 0, 186, -2, 32, false, 0, 0 },
        { 59538, 1, 22, 0, 32, true, 0, 0 },
        { 59538, 2, 87, -1, 32, false, 0, 0 },
        { 59539, 0, 186, -2, 32, false, 0, 0 },
        { 59539, 1, 22, 0, 32, true, 0, 0 },
        { 59539, 2, 87, -1, 32, false, 0, 0 },
        { 59540, 0, 186, -2, 32, false, 0, 0 },
        { 59540, 1, 22, 0, 32, true, 0, 0 },
        { 59540, 2, 87, -1, 32, false, 0, 0 },
        { 59541, 0, 186, -2, 32, false, 0, 0 },
        { 59541, 1, 22, 0, 32, true, 0, 0 },
        { 59541, 2, 87, -1, 32, false, 0, 0 },
        { 80872, 0, 186, -2, 32, false, 0, 0 },
        { 80872, 1, 22, 0, 32, true, 0, 0 },
        { 80872, 2, 87, -1, 32, false, 0, 0 },
        { 80873, 0, 186, -2, 32, false, 0, 0 },
        { 80873, 1, 22, 0, 32, true, 0, 0 },
        { 80873, 2, 87, -1, 32, false, 0, 0 },
        { 80874, 0, 186, -2, 32, false, 0, 0 },
        { 80874, 1, 22, 0, 32, true, 0, 0 },
        { 80874, 2, 87, -1, 32, false, 0, 0 },
        { 68976, 0, 22, 0, 40, true, 0, 0 },
        { 68976, 1, 87, -1, 40, false, 0, 0 },
        { 20573, 1, 117, 25, 12, false, 0, 0 },
        { 20574, 1, 30, 5, 44, false, 0, 0 },
        { 20574, 2, 30, 5, 172, false, 0, 0 },
        { 81034, 1, 98, 15, 185, false, 0, 0 },
        { 81062, 1, 98, 15, 185, false, 0, 0 },
        { 81105, 1, 98, 15, 773, false, 0, 0 },
        { 81106, 1, 79, 1, 126, false, 0, 0 },
        { 81108, 1, 79, 1, 127, false, 0, 0 },
        { 81108, 2, 136, 1, 0, false, 0, 0 },
        { 81109, 1, 98, 15, 186, false, 0, 0 },
        { 81110, 0, 87, -2, 8, false, 0, 0 },
        { 81093, 0, 4, 0, 0, false, 0, 0 },
        { 81100, 1, 98, 10, 164, false, 0, 0 },
        { 81101, 1, 136, 1, 0, false, 0, 0 },
        { 81020, 1, 79, 1, 127, false, 0, 0 },
        { 81020, 2, 136, 1, 0, false, 0, 0 },
        { 81021, 1, 58, 10, 0, false, 0, 0 },
        { 81022, 0, 87, -2, 16, false, 0, 0 },
        { 81022, 1, 87, -1, 24, false, 0, 0 },
        { 81126, 0, 87, -2, 8, false, 0, 0 },
        { 100001, 0, 31, 4, 0, false, 0, 0 },
        { 81116, 1, 142, 10, 1, false, 0, 0 },
        { 81120, 1, 142, 10, 1, false, 0, 0 },
    };

    // item requirement taken away (the tooltip always said "all attacks and spells")
    constexpr uint32 NO_ITEM[] = { 95041, 95044 };
}

#endif
