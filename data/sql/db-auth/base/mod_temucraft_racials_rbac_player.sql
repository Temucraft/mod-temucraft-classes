-- mod-temucraft-racials: its player commands (all under .temucraft) as RBAC permissions 102850-102899. Linked to the stock player
-- role 195 and the Temucraft player role 1010, so every account has them by default. To take one away, remove its
-- link here (or deny it per account with .rbac account deny <account> <id>); to give it to staff only, link it to
-- 1011-1014 instead.
DELETE FROM `rbac_linked_permissions` WHERE `linkedId` BETWEEN 102850 AND 102899;
DELETE FROM `rbac_permissions` WHERE `id` BETWEEN 102850 AND 102899;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(102850, 'Command: temucraft heritage choose'),
(102851, 'Command: temucraft heritage reset'),
(102852, 'Command: temucraft heritage bonus'),
(102853, 'Command: temucraft heritage take'),
(102854, 'Command: temucraft heritage drop'),
(102855, 'Command: temucraft heritage');
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(195, 102850),
(1010, 102850),
(195, 102851),
(1010, 102851),
(195, 102852),
(1010, 102852),
(195, 102853),
(1010, 102853),
(195, 102854),
(1010, 102854),
(195, 102855),
(1010, 102855);
