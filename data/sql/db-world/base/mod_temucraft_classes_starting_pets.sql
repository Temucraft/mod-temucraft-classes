-- mod-temucraft-classes: new warlocks start with their Imp out (as in later expansions). Re-runnable.
-- Hunters get their starting pet from modpak F-128 in mod-worgoblin-high-elf (player_starting_pet).
DELETE FROM `playercreateinfo_cast_spell` WHERE `classMask` = 256 AND `spell` = 688;
INSERT INTO `playercreateinfo_cast_spell` (`raceMask`, `classMask`, `spell`, `note`) VALUES (0, 256, 688, 'Warlock - Summon Imp');
