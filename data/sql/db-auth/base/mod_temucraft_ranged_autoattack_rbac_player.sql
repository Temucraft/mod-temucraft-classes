-- mod-temucraft-ranged-autoattack: its commands (all under .temucraft) as RBAC permissions 102950-102999. Every command is admin only by
-- default: linked to the administrator roles 192 and 1014. To give one to players, link it to 195 and 1010 here (or
-- grant it per account with .rbac account grant <account> <id>). "Temucraft: use ..." permissions are not
-- commands: they stay linked to the player roles so the items keep working.
DELETE FROM `rbac_linked_permissions` WHERE `linkedId` BETWEEN 102950 AND 102999;
DELETE FROM `rbac_permissions` WHERE `id` BETWEEN 102950 AND 102999;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(102950, 'Command: temucraft rangedauto rightclick'),
(102951, 'Command: temucraft rangedauto melee'),
(102952, 'Command: temucraft rangedauto');
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(192, 102950),
(1014, 102950),
(192, 102951),
(1014, 102951),
(192, 102952),
(1014, 102952);
