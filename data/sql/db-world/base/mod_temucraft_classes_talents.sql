-- mod-temucraft-classes: new talents (Talent.dbc 2286-2305, spells 95100-95154) and Touch of the Grave.
-- Rank chains (a higher rank replaces the lower), procs and scripts. Re-runnable.
SET @ClassSpell := 95041;
SET @TalentCreature := 601990;
DELETE FROM `spell_ranks` WHERE `first_spell_id` IN (@ClassSpell+59, @ClassSpell+61, @ClassSpell+66, @ClassSpell+71, @ClassSpell+75, @ClassSpell+78, @ClassSpell+83, @ClassSpell+90, @ClassSpell+92, @ClassSpell+95, @ClassSpell+111);
INSERT INTO `spell_ranks` (`first_spell_id`, `spell_id`, `rank`) VALUES
(@ClassSpell+59, @ClassSpell+59, 1), (@ClassSpell+59, @ClassSpell+60, 2),
(@ClassSpell+61, @ClassSpell+61, 1), (@ClassSpell+61, @ClassSpell+62, 2), (@ClassSpell+61, @ClassSpell+63, 3), (@ClassSpell+61, @ClassSpell+64, 4), (@ClassSpell+61, @ClassSpell+65, 5),
(@ClassSpell+66, @ClassSpell+66, 1), (@ClassSpell+66, @ClassSpell+67, 2), (@ClassSpell+66, @ClassSpell+68, 3), (@ClassSpell+66, @ClassSpell+69, 4), (@ClassSpell+66, @ClassSpell+70, 5),
(@ClassSpell+71, @ClassSpell+71, 1), (@ClassSpell+71, @ClassSpell+72, 2), (@ClassSpell+71, @ClassSpell+73, 3),
(@ClassSpell+75, @ClassSpell+75, 1), (@ClassSpell+75, @ClassSpell+76, 2), (@ClassSpell+75, @ClassSpell+77, 3),
(@ClassSpell+78, @ClassSpell+78, 1), (@ClassSpell+78, @ClassSpell+79, 2), (@ClassSpell+78, @ClassSpell+80, 3), (@ClassSpell+78, @ClassSpell+81, 4), (@ClassSpell+78, @ClassSpell+82, 5),
(@ClassSpell+83, @ClassSpell+83, 1), (@ClassSpell+83, @ClassSpell+84, 2),
(@ClassSpell+90, @ClassSpell+90, 1), (@ClassSpell+90, @ClassSpell+91, 2),
(@ClassSpell+92, @ClassSpell+92, 1), (@ClassSpell+92, @ClassSpell+93, 2), (@ClassSpell+92, @ClassSpell+94, 3),
(@ClassSpell+95, @ClassSpell+95, 1), (@ClassSpell+95, @ClassSpell+96, 2), (@ClassSpell+95, @ClassSpell+97, 3),
(@ClassSpell+111, @ClassSpell+111, 1), (@ClassSpell+111, @ClassSpell+112, 2);

-- Heating Up: only critical hits of Fireball, Fire Blast, Scorch and Frostfire Bolt. Touch of the Grave: 20%, once per 15 sec.
DELETE FROM `spell_proc` WHERE `SpellId` IN (@ClassSpell+102, @ClassSpell+107);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`,
    `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`,
    `Cooldown`, `Charges`) VALUES
(@ClassSpell+102, 0, 3, 0x13, 0x1000, 0, 0x10000, 1, 2, 2, 0, 0, 0, 100, 0, 0),
(@ClassSpell+107, 0, 0, 0, 0, 0, 0x10114, 1, 2, 0, 0, 0, 0, 20, 15000, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_temucraft_bloom', 'spell_temucraft_templars_bulwark');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-774, 'spell_temucraft_bloom'), (-8936, 'spell_temucraft_bloom'), (@ClassSpell+106, 'spell_temucraft_templars_bulwark');

-- Third batch of talents (Talent.dbc 2306+, spells 95160-95237)
DELETE FROM `spell_ranks` WHERE `first_spell_id` IN (@ClassSpell+119, @ClassSpell+121, @ClassSpell+126, @ClassSpell+128, @ClassSpell+130, @ClassSpell+133, @ClassSpell+135, @ClassSpell+137, @ClassSpell+148, @ClassSpell+152, @ClassSpell+155, @ClassSpell+158, @ClassSpell+161, @ClassSpell+165, @ClassSpell+167, @ClassSpell+171, @ClassSpell+177);
INSERT INTO `spell_ranks` (`first_spell_id`, `spell_id`, `rank`) VALUES
(@ClassSpell+119, @ClassSpell+119, 1),
(@ClassSpell+119, @ClassSpell+120, 2),
(@ClassSpell+121, @ClassSpell+121, 1),
(@ClassSpell+121, @ClassSpell+122, 2),
(@ClassSpell+121, @ClassSpell+123, 3),
(@ClassSpell+121, @ClassSpell+124, 4),
(@ClassSpell+121, @ClassSpell+125, 5),
(@ClassSpell+126, @ClassSpell+126, 1),
(@ClassSpell+126, @ClassSpell+127, 2),
(@ClassSpell+128, @ClassSpell+128, 1),
(@ClassSpell+128, @ClassSpell+129, 2),
(@ClassSpell+130, @ClassSpell+130, 1),
(@ClassSpell+130, @ClassSpell+131, 2),
(@ClassSpell+130, @ClassSpell+132, 3),
(@ClassSpell+133, @ClassSpell+133, 1),
(@ClassSpell+133, @ClassSpell+134, 2),
(@ClassSpell+135, @ClassSpell+135, 1),
(@ClassSpell+135, @ClassSpell+136, 2),
(@ClassSpell+137, @ClassSpell+137, 1),
(@ClassSpell+137, @ClassSpell+139, 2),
(@ClassSpell+137, @ClassSpell+141, 3),
(@ClassSpell+137, @ClassSpell+143, 4),
(@ClassSpell+137, @ClassSpell+145, 5),
(@ClassSpell+148, @ClassSpell+148, 1),
(@ClassSpell+148, @ClassSpell+150, 2),
(@ClassSpell+152, @ClassSpell+152, 1),
(@ClassSpell+152, @ClassSpell+153, 2),
(@ClassSpell+155, @ClassSpell+155, 1),
(@ClassSpell+155, @ClassSpell+156, 2),
(@ClassSpell+158, @ClassSpell+158, 1),
(@ClassSpell+158, @ClassSpell+159, 2),
(@ClassSpell+161, @ClassSpell+161, 1),
(@ClassSpell+161, @ClassSpell+162, 2),
(@ClassSpell+161, @ClassSpell+163, 3),
(@ClassSpell+165, @ClassSpell+165, 1),
(@ClassSpell+165, @ClassSpell+166, 2),
(@ClassSpell+167, @ClassSpell+167, 1),
(@ClassSpell+167, @ClassSpell+169, 2),
(@ClassSpell+171, @ClassSpell+171, 1),
(@ClassSpell+171, @ClassSpell+173, 2),
(@ClassSpell+177, @ClassSpell+177, 1),
(@ClassSpell+177, @ClassSpell+179, 2),
(@ClassSpell+177, @ClassSpell+181, 3),
(@ClassSpell+177, @ClassSpell+183, 4),
(@ClassSpell+177, @ClassSpell+185, 5);
DELETE FROM `spell_proc` WHERE `SpellId` BETWEEN @ClassSpell+119 AND @ClassSpell+196;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(@ClassSpell+137, 0, 6, 8192, 0, 1088, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+139, 0, 6, 8192, 0, 1088, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+141, 0, 6, 8192, 0, 1088, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+143, 0, 6, 8192, 0, 1088, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+145, 0, 6, 8192, 0, 1088, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+148, 0, 4, 269484032, 1024, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+150, 0, 4, 269484032, 1024, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+152, 0, 0, 0, 0, 0, 40, 0, 2, 48, 0, 0, 0, 25, 0, 0),
(@ClassSpell+153, 0, 0, 0, 0, 0, 40, 0, 2, 48, 0, 0, 0, 50, 0, 0),
(@ClassSpell+155, 0, 0, 0, 0, 0, 4, 0, 2, 0, 0, 0, 0, 5, 0, 0),
(@ClassSpell+156, 0, 0, 0, 0, 0, 4, 0, 2, 0, 0, 0, 0, 10, 0, 0),
(@ClassSpell+158, 0, 0, 0, 0, 0, 340, 0, 2, 0, 0, 0, 0, 3, 0, 0),
(@ClassSpell+159, 0, 0, 0, 0, 0, 340, 0, 2, 0, 0, 0, 0, 6, 0, 0),
(@ClassSpell+161, 0, 10, 8388608, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 33, 0, 0),
(@ClassSpell+162, 0, 10, 8388608, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 66, 0, 0),
(@ClassSpell+163, 0, 10, 8388608, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+165, 0, 6, 269312, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 50, 0, 0),
(@ClassSpell+166, 0, 6, 269312, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+167, 0, 9, 16384, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+169, 0, 9, 16384, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+171, 0, 11, 0, 16777232, 0, 87376, 0, 1, 0, 0, 0, 0, 25, 0, 0),
(@ClassSpell+173, 0, 11, 0, 16777232, 0, 87376, 0, 1, 0, 0, 0, 0, 50, 0, 0),
(@ClassSpell+176, 0, 9, 2, 0, 0, 16, 0, 2, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+177, 0, 5, 0, 8388608, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+179, 0, 5, 0, 8388608, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+181, 0, 5, 0, 8388608, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+183, 0, 5, 0, 8388608, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+185, 0, 5, 0, 8388608, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+188, 0, 6, 2048, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0);
-- Summon Hawk's hawk: a guardian that fights for 18 sec (owl model)
DELETE FROM `creature_template` WHERE `entry` = @TalentCreature;
DELETE FROM `creature_template_model` WHERE `CreatureID` = @TalentCreature;
INSERT INTO `creature_template` (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `unit_class`, `unit_flags`, `type`, `AIName`, `MovementType`, `HealthModifier`, `DamageModifier`, `RegenHealth`, `flags_extra`, `ScriptName`, `VerifiedBuild`) VALUES
(@TalentCreature, 'Hawk', '', 1, 80, 35, 0, 1, 1.42857, 1, 0, 1, '', 0, 0.5, 0.5, 1, 0, '', 0);
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
SELECT @TalentCreature, 0, `CreatureDisplayID`, 1, 1, 0 FROM `creature_template_model` WHERE `CreatureID` = 1995 AND `Idx` = 0;

-- Batch 4: script talents (Predatory Discipline, Lingering Rage, Demonic Energies, Quick Getaway, Earthbound,
-- Solar Momentum, Swift Judgement, Unsealed Wrath, Demonic Brand, Bane of Havoc) and Light's Vigil rework
DELETE FROM `spell_ranks` WHERE `first_spell_id` IN (@ClassSpell+200, @ClassSpell+205, @ClassSpell+207, @ClassSpell+212, @ClassSpell+221);
INSERT INTO `spell_ranks` (`first_spell_id`, `spell_id`, `rank`) VALUES
(@ClassSpell+200, @ClassSpell+200, 1),
(@ClassSpell+200, @ClassSpell+201, 2),
(@ClassSpell+200, @ClassSpell+202, 3),
(@ClassSpell+200, @ClassSpell+203, 4),
(@ClassSpell+200, @ClassSpell+204, 5),
(@ClassSpell+205, @ClassSpell+205, 1),
(@ClassSpell+205, @ClassSpell+206, 2),
(@ClassSpell+207, @ClassSpell+207, 1),
(@ClassSpell+207, @ClassSpell+209, 2),
(@ClassSpell+212, @ClassSpell+212, 1),
(@ClassSpell+212, @ClassSpell+214, 2),
(@ClassSpell+212, @ClassSpell+216, 3),
(@ClassSpell+221, @ClassSpell+221, 1),
(@ClassSpell+221, @ClassSpell+223, 2),
(@ClassSpell+221, @ClassSpell+225, 3);
DELETE FROM `spell_proc` WHERE `SpellId` BETWEEN @ClassSpell+199 AND @ClassSpell+230;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(@ClassSpell+207, 0, 8, 128, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+209, 0, 8, 128, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+211, 0, 11, 536870912, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+212, 0, 7, 1, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+214, 0, 7, 1, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+216, 0, 7, 1, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+220, 0, 0, 0, 0, 0, 20, 0, 2, 0, 0, 0, 0, 10, 0, 0),
(@ClassSpell+221, 0, 5, 256, 0, 0, 87376, 0, 2, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+223, 0, 5, 256, 0, 0, 87376, 0, 2, 0, 0, 0, 0, 100, 0, 0),
(@ClassSpell+225, 0, 5, 256, 0, 0, 87376, 0, 2, 0, 0, 0, 0, 100, 0, 0);
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_temucraft_predatory_discipline', 'spell_temucraft_spearing_strike', 'spell_temucraft_arcane_wrath', 'spell_temucraft_lights_vigil', 'spell_temucraft_unsealed_wrath', 'spell_temucraft_bane_of_havoc', 'spell_temucraft_demonic_energies_tap') OR `spell_id` IN (@ClassSpell+218);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-22568, 'spell_temucraft_predatory_discipline'),
(@ClassSpell+192, 'spell_temucraft_spearing_strike'),
(@ClassSpell+190, 'spell_temucraft_arcane_wrath'),
(-20473, 'spell_temucraft_lights_vigil'),
(@ClassSpell+219, 'spell_temucraft_unsealed_wrath'),
(@ClassSpell+218, 'spell_pal_judgement_of_light'),
(@ClassSpell+227, 'spell_temucraft_bane_of_havoc'),
(-1454, 'spell_temucraft_demonic_energies_tap');
