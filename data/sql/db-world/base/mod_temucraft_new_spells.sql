-- mod-temucraft-new-spells: Slap's script.
SET @NewSpell := 96016;
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_temucraft_slap';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (@NewSpell+56, 'spell_temucraft_slap');
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_temucraft_redirect';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (73981, 'spell_temucraft_redirect');

-- Heroic Leap at warrior trainers (those teaching Heroic Strike rank 2), level 40; Redirect at rogue trainers (those
-- teaching Sinister Strike rank 2), level 42.
DELETE FROM `trainer_spell` WHERE `SpellId` IN (6544, 73981) AND `VerifiedBuild` = -1811;
INSERT IGNORE INTO `trainer_spell` (`TrainerId`, `SpellId`, `MoneyCost`, `ReqSkillLine`, `ReqSkillRank`, `ReqAbility1`, `ReqAbility2`, `ReqAbility3`, `ReqLevel`, `VerifiedBuild`)
SELECT DISTINCT `TrainerId`, 6544, 50000, 0, 0, 0, 0, 0, 40, -1811 FROM `trainer_spell` WHERE `SpellId` = 284;
INSERT IGNORE INTO `trainer_spell` (`TrainerId`, `SpellId`, `MoneyCost`, `ReqSkillLine`, `ReqSkillRank`, `ReqAbility1`, `ReqAbility2`, `ReqAbility3`, `ReqLevel`, `VerifiedBuild`)
SELECT DISTINCT `TrainerId`, 73981, 60000, 0, 0, 0, 0, 0, 42, -1811 FROM `trainer_spell` WHERE `SpellId` = 1757;

-- Rune of Power (96073) at mage trainers (those teaching Fireball rank 2), level 50; Call of the Pack (96074) at
-- hunter trainers (those teaching Serpent Sting rank 1), level 40.
DELETE FROM `trainer_spell` WHERE `SpellId` IN (@NewSpell+57, @NewSpell+58) AND `VerifiedBuild` = -1811;
INSERT IGNORE INTO `trainer_spell` (`TrainerId`, `SpellId`, `MoneyCost`, `ReqSkillLine`, `ReqSkillRank`, `ReqAbility1`, `ReqAbility2`, `ReqAbility3`, `ReqLevel`, `VerifiedBuild`)
SELECT DISTINCT `TrainerId`, @NewSpell+57, 60000, 0, 0, 0, 0, 0, 50, -1811 FROM `trainer_spell` WHERE `SpellId` = 143;
INSERT IGNORE INTO `trainer_spell` (`TrainerId`, `SpellId`, `MoneyCost`, `ReqSkillLine`, `ReqSkillRank`, `ReqAbility1`, `ReqAbility2`, `ReqAbility3`, `ReqLevel`, `VerifiedBuild`)
SELECT DISTINCT `TrainerId`, @NewSpell+58, 40000, 0, 0, 0, 0, 0, 40, -1811 FROM `trainer_spell` WHERE `SpellId` = 1978;
