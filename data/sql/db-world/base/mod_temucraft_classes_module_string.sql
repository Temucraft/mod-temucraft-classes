-- mod-temucraft-classes: player-facing texts. Translations go in module_string_locale (same module and id, plus the locale).
DELETE FROM `module_string` WHERE `module` = 'mod-temucraft-classes';
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
('mod-temucraft-classes', 1, '|cffffcc00Reagents:|r your class spells {}. Type .temucraft reagents on or .temucraft reagents off to choose.'),
('mod-temucraft-classes', 2, 'Reagents can''t be chosen on this server.'),
('mod-temucraft-classes', 3, '|cffffcc00Reagents:|r your class spells {}.{}'),
('mod-temucraft-classes', 4, 'This server decides that for everyone.'),
('mod-temucraft-classes', 5, '|cffffcc00Reagents:|r {}'),
('mod-temucraft-classes', 6, '|cffffcc00Temucraft:|r you learned Summon Imp, the first demon every warlock now starts with.'),
('mod-temucraft-classes', 7, '|cffffcc00Temucraft:|r every hunter now starts with a pet, so {} has joined you. Call Pet, Dismiss Pet, Revive Pet and Feed Pet are in your spellbook.');
