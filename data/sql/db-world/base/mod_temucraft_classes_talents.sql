-- mod-temucraft-classes: new talents (Talent.dbc 2286-2305, spells 95100-95154) and Touch of the Grave.
-- Rank chains (a higher rank replaces the lower), procs and scripts. Re-runnable.
DELETE FROM `spell_ranks` WHERE `first_spell_id` IN (95100, 95102, 95107, 95112, 95116, 95119, 95124, 95131, 95133, 95136, 95152);
INSERT INTO `spell_ranks` (`first_spell_id`, `spell_id`, `rank`) VALUES
(95100, 95100, 1), (95100, 95101, 2),
(95102, 95102, 1), (95102, 95103, 2), (95102, 95104, 3), (95102, 95105, 4), (95102, 95106, 5),
(95107, 95107, 1), (95107, 95108, 2), (95107, 95109, 3), (95107, 95110, 4), (95107, 95111, 5),
(95112, 95112, 1), (95112, 95113, 2), (95112, 95114, 3),
(95116, 95116, 1), (95116, 95117, 2), (95116, 95118, 3),
(95119, 95119, 1), (95119, 95120, 2), (95119, 95121, 3), (95119, 95122, 4), (95119, 95123, 5),
(95124, 95124, 1), (95124, 95125, 2),
(95131, 95131, 1), (95131, 95132, 2),
(95133, 95133, 1), (95133, 95134, 2), (95133, 95135, 3),
(95136, 95136, 1), (95136, 95137, 2), (95136, 95138, 3),
(95152, 95152, 1), (95152, 95153, 2);

-- Heating Up: only critical hits of Fireball, Fire Blast, Scorch and Frostfire Bolt. Touch of the Grave: 20%, once per 15 sec.
DELETE FROM `spell_proc` WHERE `SpellId` IN (95143, 95148);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`,
    `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`,
    `Cooldown`, `Charges`) VALUES
(95143, 0, 3, 0x13, 0x1000, 0, 0x10000, 1, 2, 2, 0, 0, 0, 100, 0, 0),
(95148, 0, 0, 0, 0, 0, 0x10114, 1, 2, 0, 0, 0, 0, 20, 15000, 0);

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_temucraft_bloom', 'spell_temucraft_templars_bulwark');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-774, 'spell_temucraft_bloom'), (-8936, 'spell_temucraft_bloom'), (95147, 'spell_temucraft_templars_bulwark');

-- Third batch of talents (Talent.dbc 2306+, spells 95160-95237)
DELETE FROM `spell_ranks` WHERE `first_spell_id` IN (95160, 95162, 95167, 95169, 95171, 95174, 95176, 95178, 95189, 95193, 95196, 95199, 95202, 95206, 95208, 95212, 95218);
INSERT INTO `spell_ranks` (`first_spell_id`, `spell_id`, `rank`) VALUES
(95160, 95160, 1),
(95160, 95161, 2),
(95162, 95162, 1),
(95162, 95163, 2),
(95162, 95164, 3),
(95162, 95165, 4),
(95162, 95166, 5),
(95167, 95167, 1),
(95167, 95168, 2),
(95169, 95169, 1),
(95169, 95170, 2),
(95171, 95171, 1),
(95171, 95172, 2),
(95171, 95173, 3),
(95174, 95174, 1),
(95174, 95175, 2),
(95176, 95176, 1),
(95176, 95177, 2),
(95178, 95178, 1),
(95178, 95180, 2),
(95178, 95182, 3),
(95178, 95184, 4),
(95178, 95186, 5),
(95189, 95189, 1),
(95189, 95191, 2),
(95193, 95193, 1),
(95193, 95194, 2),
(95196, 95196, 1),
(95196, 95197, 2),
(95199, 95199, 1),
(95199, 95200, 2),
(95202, 95202, 1),
(95202, 95203, 2),
(95202, 95204, 3),
(95206, 95206, 1),
(95206, 95207, 2),
(95208, 95208, 1),
(95208, 95210, 2),
(95212, 95212, 1),
(95212, 95214, 2),
(95218, 95218, 1),
(95218, 95220, 2),
(95218, 95222, 3),
(95218, 95224, 4),
(95218, 95226, 5);
DELETE FROM `spell_proc` WHERE `SpellId` BETWEEN 95160 AND 95237;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(95178, 0, 6, 8192, 0, 1088, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95180, 0, 6, 8192, 0, 1088, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95182, 0, 6, 8192, 0, 1088, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95184, 0, 6, 8192, 0, 1088, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95186, 0, 6, 8192, 0, 1088, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95189, 0, 4, 269484032, 1024, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95191, 0, 4, 269484032, 1024, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95193, 0, 0, 0, 0, 0, 40, 0, 2, 48, 0, 0, 0, 25, 0, 0),
(95194, 0, 0, 0, 0, 0, 40, 0, 2, 48, 0, 0, 0, 50, 0, 0),
(95196, 0, 0, 0, 0, 0, 4, 0, 2, 0, 0, 0, 0, 5, 0, 0),
(95197, 0, 0, 0, 0, 0, 4, 0, 2, 0, 0, 0, 0, 10, 0, 0),
(95199, 0, 0, 0, 0, 0, 340, 0, 2, 0, 0, 0, 0, 3, 0, 0),
(95200, 0, 0, 0, 0, 0, 340, 0, 2, 0, 0, 0, 0, 6, 0, 0),
(95202, 0, 10, 8388608, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 33, 0, 0),
(95203, 0, 10, 8388608, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 66, 0, 0),
(95204, 0, 10, 8388608, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95206, 0, 6, 269312, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 50, 0, 0),
(95207, 0, 6, 269312, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95208, 0, 9, 16384, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95210, 0, 9, 16384, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95212, 0, 11, 0, 16777232, 0, 87376, 0, 1, 0, 0, 0, 0, 25, 0, 0),
(95214, 0, 11, 0, 16777232, 0, 87376, 0, 1, 0, 0, 0, 0, 50, 0, 0),
(95217, 0, 9, 2, 0, 0, 16, 0, 2, 0, 0, 0, 0, 100, 0, 0),
(95218, 0, 5, 0, 8388608, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95220, 0, 5, 0, 8388608, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95222, 0, 5, 0, 8388608, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95224, 0, 5, 0, 8388608, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95226, 0, 5, 0, 8388608, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95229, 0, 6, 2048, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0);
-- Summon Hawk's hawk: a guardian that fights for 18 sec (owl model)
DELETE FROM `creature_template` WHERE `entry` = 601990;
DELETE FROM `creature_template_model` WHERE `CreatureID` = 601990;
INSERT INTO `creature_template` (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `unit_class`, `unit_flags`, `type`, `AIName`, `MovementType`, `HealthModifier`, `DamageModifier`, `RegenHealth`, `flags_extra`, `ScriptName`, `VerifiedBuild`) VALUES
(601990, 'Hawk', '', 1, 80, 35, 0, 1, 1.42857, 1, 0, 1, '', 0, 0.5, 0.5, 1, 0, '', 0);
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
SELECT 601990, 0, `CreatureDisplayID`, 1, 1, 0 FROM `creature_template_model` WHERE `CreatureID` = 1995 AND `Idx` = 0;

-- Batch 4: script talents (Predatory Discipline, Lingering Rage, Demonic Energies, Quick Getaway, Earthbound,
-- Solar Momentum, Swift Judgement, Unsealed Wrath, Demonic Brand, Bane of Havoc) and Light's Vigil rework
DELETE FROM `spell_ranks` WHERE `first_spell_id` IN (95241, 95246, 95248, 95253, 95262);
INSERT INTO `spell_ranks` (`first_spell_id`, `spell_id`, `rank`) VALUES
(95241, 95241, 1),
(95241, 95242, 2),
(95241, 95243, 3),
(95241, 95244, 4),
(95241, 95245, 5),
(95246, 95246, 1),
(95246, 95247, 2),
(95248, 95248, 1),
(95248, 95250, 2),
(95253, 95253, 1),
(95253, 95255, 2),
(95253, 95257, 3),
(95262, 95262, 1),
(95262, 95264, 2),
(95262, 95266, 3);
DELETE FROM `spell_proc` WHERE `SpellId` BETWEEN 95240 AND 95271;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(95248, 0, 8, 128, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95250, 0, 8, 128, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95252, 0, 11, 536870912, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95253, 0, 7, 1, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95255, 0, 7, 1, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95257, 0, 7, 1, 0, 0, 87376, 0, 1, 0, 0, 0, 0, 100, 0, 0),
(95261, 0, 0, 0, 0, 0, 20, 0, 2, 0, 0, 0, 0, 10, 0, 0),
(95262, 0, 5, 256, 0, 0, 87376, 0, 2, 0, 0, 0, 0, 100, 0, 0),
(95264, 0, 5, 256, 0, 0, 87376, 0, 2, 0, 0, 0, 0, 100, 0, 0),
(95266, 0, 5, 256, 0, 0, 87376, 0, 2, 0, 0, 0, 0, 100, 0, 0);
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_temucraft_predatory_discipline', 'spell_temucraft_spearing_strike', 'spell_temucraft_arcane_wrath', 'spell_temucraft_lights_vigil', 'spell_temucraft_unsealed_wrath', 'spell_temucraft_bane_of_havoc', 'spell_temucraft_demonic_energies_tap') OR `spell_id` IN (95259);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-22568, 'spell_temucraft_predatory_discipline'),
(95233, 'spell_temucraft_spearing_strike'),
(95231, 'spell_temucraft_arcane_wrath'),
(-20473, 'spell_temucraft_lights_vigil'),
(95260, 'spell_temucraft_unsealed_wrath'),
(95259, 'spell_pal_judgement_of_light'),
(95268, 'spell_temucraft_bane_of_havoc'),
(-1454, 'spell_temucraft_demonic_energies_tap');
