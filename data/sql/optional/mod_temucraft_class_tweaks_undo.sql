-- mod-temucraft-class-tweaks: puts the stock values back (run it by hand after removing the module).
SET @ClassSpell := 96400;
SET @NewSpell := 100110;
SET @MendingSpell := 100172;
UPDATE `item_template` SET `stackable` = 1, `maxcount` = 32 WHERE `entry` = 6265;
DELETE FROM `trainer_spell` WHERE `VerifiedBuild` = -1811;
UPDATE `item_template` SET `BuyPrice` = 5000, `SellPrice` = 1250, `BagFamily` = 0 WHERE `entry` = 5565;
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500, `BagFamily` = 0 WHERE `entry` = 16583;
UPDATE `trainer_spell` SET `ReqLevel` = 30, `MoneyCost` = 7000 WHERE `SpellId` = 36936;
UPDATE `gameobject_template` SET `Data0` = 5, `Data4` = 20625, `Data5` = 1 WHERE `entry` = 177193;
DELETE FROM `module_string` WHERE `module` = 'mod-temucraft-class-tweaks';

-- TemucraftClassChanges.cpp
UPDATE `spell_script_names` SET `ScriptName` = 'spell_pri_lightwell' WHERE `spell_id` = 60123 AND `ScriptName` = 'spell_temucraft_lightwell';
UPDATE `spell_bonus_data` SET `dot_bonus` = 0.0529 WHERE `entry` IN (14914, 48135);
DELETE FROM `spell_dbc` WHERE `ID` BETWEEN @ClassSpell AND @ClassSpell+5;
DELETE FROM `spell_bonus_data` WHERE `entry` = @ClassSpell+3;

-- TemucraftNewClassSpells.cpp
DELETE FROM `spell_dbc` WHERE `ID` BETWEEN @NewSpell AND @NewSpell+16;
DELETE FROM `spell_ranks` WHERE `first_spell_id` IN (@NewSpell, @NewSpell+9);
DELETE FROM `spell_bonus_data` WHERE `entry` IN (@NewSpell, @NewSpell+9);
DELETE FROM `spell_proc` WHERE `SpellId` = @NewSpell+8;
DELETE FROM `spell_script_names` WHERE `spell_id` = @NewSpell+8;
DELETE FROM `spell_dbc` WHERE `ID` BETWEEN @MendingSpell AND @MendingSpell+15;
