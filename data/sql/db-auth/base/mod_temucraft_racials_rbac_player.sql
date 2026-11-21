-- mod-temucraft-racials: its commands (all under .temucraft) as RBAC permissions 102850-102899. Every command is admin only by
-- default: linked to the administrator roles 192 and 1014. To give one to players, link it to 195 and 1010 here (or
-- grant it per account with .rbac account grant <account> <id>). "Temucraft: use ..." permissions are not
-- commands: they stay linked to the player roles so the items keep working.
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
(192, 102850),
(1014, 102850),
(192, 102851),
(1014, 102851),
(192, 102852),
(1014, 102852),
(192, 102853),
(1014, 102853),
(192, 102854),
(1014, 102854),
(192, 102855),
(1014, 102855);
