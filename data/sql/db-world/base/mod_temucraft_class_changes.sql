-- mod-temucraft-class-tweaks: data for TemucraftClassChanges.cpp.
-- Lightwell: our script (15 charges, one drink per player) instead of the core's.
UPDATE `spell_script_names` SET `ScriptName` = 'spell_temucraft_lightwell' WHERE `spell_id` = 60123 AND `ScriptName` = 'spell_pri_lightwell';
-- Holy Fire: the burn lasts 16 sec instead of 7 and deals 40% more in total, so each tick's spell power share is
-- 1.4 * 7 / 16 of the old one.
UPDATE `spell_bonus_data` SET `dot_bonus` = 0.0324 WHERE `entry` IN (14914, 48135);
