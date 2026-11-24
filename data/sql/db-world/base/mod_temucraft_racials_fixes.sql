-- mod-temucraft-classes (racials): server-side fixes for custom race racials that copy a stock racial but miss the
-- stock racial's script or linked spell. The spells come from mod-worgoblin-high-elf (tools/races/coa_racials.py,
-- modpaks/config.sql @CoA...Racial ids). If those ids move, change them here too. Re-runnable.

-- Make Camp (Vulpera, 81124) is a copy of Cannibalize: its dummy effect does nothing without Cannibalize's script.
DELETE FROM `spell_script_names` WHERE `spell_id` = 81124 AND `ScriptName` = 'spell_gen_cannibalize';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(81124, 'spell_gen_cannibalize');

-- Spatial Rift (Void Elf, 81091) and Cantrips (Nightborne, 81104) are copies of Shadowmeld: like Shadowmeld they need
-- the linked Sanctuary (drops combat and threat) and the force deselect, or they are only a plain stealth.
-- Azerite Surge (Earthen, 81115 and 81119) is a copy of Stoneform: like Stoneform it needs the linked armor buff
-- (65116) that its tooltip names.
DELETE FROM `spell_linked_spell` WHERE `spell_trigger` IN (81091, 81104, 81115, 81119);
INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `type`, `comment`) VALUES
(81091, 59646, 0, 'Spatial Rift: Sanctuary (as Shadowmeld)'),
(81091, 62196, 0, 'Spatial Rift: Force deselect (as Shadowmeld)'),
(81104, 59646, 0, 'Cantrips: Sanctuary (as Shadowmeld)'),
(81104, 62196, 0, 'Cantrips: Force deselect (as Shadowmeld)'),
(81115, 65116, 0, 'Azerite Surge: armor (as Stoneform)'),
(81119, 65116, 0, 'Azerite Surge: armor (as Stoneform)');
