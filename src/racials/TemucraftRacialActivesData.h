// Made by tools/build_racial_actives.py from its SPELLS list: change it there, not here.
// The racial actives (and missing racials) from wow-racials_v3, spells 95835-95899.
#ifndef TEMUCRAFT_RACIAL_ACTIVES_DATA_H
#define TEMUCRAFT_RACIAL_ACTIVES_DATA_H

#include "Define.h"

namespace TemucraftRacialActivesData
{
    constexpr uint32 FIRST_SPELL = 95835;
    constexpr uint32 LAST_SPELL = 95899;

    constexpr uint32 SPELL_REGENERATIN = 95835;   // Regeneratin'
    constexpr uint32 SPELL_PTERRORDAX_SWOOP = 95836;   // Pterrordax Swoop
    constexpr uint32 SPELL_EMBRACE_OF_BWONSAMDI = 95837;   // Embrace of Bwonsamdi
    constexpr uint32 SPELL_BWONSAMDI_DRAIN = 95838;   // Embrace of Bwonsamdi (helper)
    constexpr uint32 SPELL_FIREBLOOD = 95839;   // Fireblood
    constexpr uint32 SPELL_FIREBLOOD_BUFF = 95840;   // Fireblood (helper)
    constexpr uint32 SPELL_MOLE_MACHINE = 95841;   // Mole Machine
    constexpr uint32 SPELL_ANCESTRAL_CALL = 95842;   // Ancestral Call
    constexpr uint32 SPELL_FEROCITY_FROSTWOLF = 95843;   // Ferocity of the Frostwolf (helper)
    constexpr uint32 SPELL_MIGHT_BLACKROCK = 95844;   // Might of the Blackrock (helper)
    constexpr uint32 SPELL_ZEAL_BURNING_BLADE = 95845;   // Zeal of the Burning Blade (helper)
    constexpr uint32 SPELL_RICTUS_LAUGHING_SKULL = 95846;   // Rictus of the Laughing Skull (helper)
    constexpr uint32 SPELL_HAYMAKER = 95847;   // Haymaker
    constexpr uint32 SPELL_BAG_OF_TRICKS = 95848;   // Bag of Tricks
    constexpr uint32 SPELL_BAG_OF_TRICKS_ENEMY = 95849;   // Bag of Tricks (helper)
    constexpr uint32 SPELL_BAG_OF_TRICKS_ALLY = 95850;   // Bag of Tricks (helper)
    constexpr uint32 SPELL_MAKE_CAMP = 95851;   // Make Camp
    constexpr uint32 SPELL_RETURN_TO_CAMP = 95852;   // Return to Camp
    constexpr uint32 SPELL_HOLO = 95853;   // Hyper Organic Light Originator
    constexpr uint32 SPELL_SKELETON_PINKIE = 95854;   // Skeleton Pinkie
    constexpr uint32 SPELL_ARCANE_PULSE = 95855;   // Arcane Pulse
    constexpr uint32 SPELL_CANTRIPS = 95856;   // Cantrips
    constexpr uint32 SPELL_BULL_RUSH = 95857;   // Bull Rush
    constexpr uint32 SPELL_BULL_RUSH_STUN = 95858;   // Bull Rush (helper)
    constexpr uint32 SPELL_LIGHTS_JUDGMENT = 95859;   // Light's Judgment
    constexpr uint32 SPELL_LIGHTS_JUDGMENT_DAMAGE = 95860;   // Light's Judgment (helper)
    constexpr uint32 SPELL_LIGHTS_RECKONING = 95861;   // Light's Reckoning
    constexpr uint32 SPELL_LIGHTS_RECKONING_BURST = 95862;   // Light's Reckoning (helper)
    constexpr uint32 SPELL_FORGE_OF_LIGHT = 95863;   // Forge of Light
    constexpr uint32 SPELL_SPATIAL_RIFT = 95864;   // Spatial Rift
    constexpr uint32 SPELL_QUAKING_PALM = 95865;   // Quaking Palm
    constexpr uint32 SPELL_WING_BUFFET = 95866;   // Wing Buffet
    constexpr uint32 SPELL_TAIL_SWIPE = 95867;   // Tail Swipe
    constexpr uint32 SPELL_SOAR = 95868;   // Soar
    constexpr uint32 SPELL_SOAR_GLIDE = 95869;   // Soar (helper)
    constexpr uint32 SPELL_AZERITE_SURGE = 95870;   // Azerite Surge
    constexpr uint32 SPELL_QUIET_CONTEMPLATION = 95871;   // Quiet Contemplation
    constexpr uint32 SPELL_THORN_BLOOM = 95872;   // Thorn Bloom
    constexpr uint32 SPELL_THORN_BLOOM_DAMAGE = 95873;   // Thorn Bloom (helper)
    constexpr uint32 SPELL_THORN_BLOOM_HEAL = 95874;   // Thorn Bloom (helper)
    constexpr uint32 SPELL_ROOTWALKING = 95875;   // Rootwalking
    constexpr uint32 SPELL_SPATIAL_RIFT_OPEN = 95876;   // Spatial Rift (helper)

    // what a character learns (its racial skill line teaches them); their cooldowns are final (the
    // racials module's cooldown cut skips them)
    constexpr uint32 LEARNED[] = { 95835, 95836, 95837, 95839, 95841, 95842, 95847, 95848, 95851, 95852, 95853, 95854, 95855, 95856, 95857, 95859, 95861, 95863, 95864, 95865, 95866, 95867, 95868, 95870, 95871, 95872, 95875 };

    // the renamed stock copies they replace: { old, new, race mask of its skill line row } (taken away at
    // login, action bar buttons moved over)
    struct Replaced { uint32 oldSpell; uint32 newSpell; uint32 raceMask; };
    constexpr Replaced REPLACED[] =
    {
        { 81019, 95847, 0x80000 },
        { 81031, 95865, 0x2000 },
        { 81059, 95865, 0x1000 },
        { 81091, 95864, 0x10000 },
        { 81099, 95859, 0x20000 },
        { 81103, 95855, 0x4000 },
        { 81104, 95856, 0x4000 },
        { 81107, 95857, 0x8000 },
        { 81111, 95853, 0x800000 },
        { 81115, 95870, 0x4000000 },
        { 81119, 95870, 0x8000000 },
        { 81123, 95848, 0x100000 },
        { 81124, 95851, 0x100000 },
        { 81175, 95866, 0x1000000 },
        { 81176, 95868, 0x1000000 },
        { 81179, 95866, 0x2000000 },
        { 81180, 95868, 0x2000000 },
        { 274738, 95842, 0x400000 },
    };
}

#endif
