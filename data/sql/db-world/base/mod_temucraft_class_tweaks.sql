-- mod-temucraft-class-tweaks: world data. Re-runnable. data/sql/optional/mod_temucraft_class_tweaks_undo.sql puts
-- the stock values back.

-- Soul Shards stack to 99 (and there is no limit on how many you carry).
UPDATE `item_template` SET `stackable` = 99, `maxcount` = 0 WHERE `entry` = 6265;

-- Infernal Stone 10 silver, Demonic Figurine 20 silver; both fit in soul bags (and soul bags may keep them).
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250, `BagFamily` = 4 WHERE `entry` = 5565;
UPDATE `item_template` SET `BuyPrice` = 2000, `SellPrice` = 500, `BagFamily` = 4 WHERE `entry` = 16583;

-- Totemic Recall is trained from level 10, for 10 silver.
UPDATE `trainer_spell` SET `ReqLevel` = 10, `MoneyCost` = 1000 WHERE `SpellId` = 36936;

-- Ritual of Doom: the warlock and 2 helpers are enough, and nobody is sacrificed.
UPDATE `gameobject_template` SET `Data0` = 3, `Data4` = 0, `Data5` = 0 WHERE `entry` = 177193;

-- Weapon masters teach every weapon, Dual Wield and every armor proficiency. Who may learn which is up to the
-- config (AnyWeapon / AnyArmor): the others show in red. Added rows carry VerifiedBuild -1811 (for the undo file).
INSERT IGNORE INTO `trainer_spell` (`TrainerId`, `SpellId`, `MoneyCost`, `ReqSkillLine`, `ReqSkillRank`, `ReqAbility1`, `ReqAbility2`, `ReqAbility3`, `ReqLevel`, `VerifiedBuild`)
SELECT DISTINCT ts.`TrainerId`, p.`spell`, p.`cost`, 0, 0, 0, 0, 0, p.`lvl`, -1811
FROM `trainer_spell` ts
JOIN `trainer` t ON t.`Id` = ts.`TrainerId` AND t.`Type` = 2
JOIN (
    SELECT 196 AS `spell`, 1000 AS `cost`, 1 AS `lvl` UNION ALL SELECT 197, 1000, 1 UNION ALL SELECT 198, 1000, 1
    UNION ALL SELECT 199, 1000, 1 UNION ALL SELECT 200, 1000, 1 UNION ALL SELECT 201, 1000, 1 UNION ALL SELECT 202, 1000, 1
    UNION ALL SELECT 227, 1000, 1 UNION ALL SELECT 264, 1000, 1 UNION ALL SELECT 266, 1000, 1 UNION ALL SELECT 1180, 1000, 1
    UNION ALL SELECT 2567, 1000, 1 UNION ALL SELECT 5009, 1000, 1 UNION ALL SELECT 5011, 1000, 1 UNION ALL SELECT 15590, 1000, 1
    UNION ALL SELECT 674, 1000, 10
    UNION ALL SELECT 9078, 1000, 1 UNION ALL SELECT 9077, 1000, 1 UNION ALL SELECT 9116, 1000, 1
    UNION ALL SELECT 8737, 10000, 40 UNION ALL SELECT 750, 10000, 40
) p
WHERE ts.`SpellId` IN (196, 197, 198, 199, 200, 201, 202, 227, 264, 266, 1180, 2567, 5011, 15590);

-- Parachute: every class trainer teaches it from level 10, for 10 silver. Its cooldown shrinks with level (config).
INSERT IGNORE INTO `trainer_spell` (`TrainerId`, `SpellId`, `MoneyCost`, `ReqSkillLine`, `ReqSkillRank`, `ReqAbility1`, `ReqAbility2`, `ReqAbility3`, `ReqLevel`, `VerifiedBuild`)
SELECT `Id`, 55001, 1000, 0, 0, 0, 0, 0, 10, -1811 FROM `trainer` WHERE `Type` = 0;

-- Player-facing texts. Translations go in module_string_locale (same module and id, plus the locale).
DELETE FROM `module_string` WHERE `module` = 'mod-temucraft-class-tweaks';
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
('mod-temucraft-class-tweaks', 1, '|cff9482c9Your soul bag kept the {}.|r');

-- Tranquility (healing ticks, ranks 1-7): heals the most injured raid members in reach.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_temucraft_tranquility';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(44203, 'spell_temucraft_tranquility'), (44205, 'spell_temucraft_tranquility'), (44206, 'spell_temucraft_tranquility'),
(44207, 'spell_temucraft_tranquility'), (44208, 'spell_temucraft_tranquility'), (48444, 'spell_temucraft_tranquility'),
(48445, 'spell_temucraft_tranquility');
