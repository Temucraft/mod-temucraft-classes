-- mod-temucraft-ranged-autoattack: its player commands (all under .temucraft) as RBAC permissions 102950-102999. Linked to the stock player
-- role 195 and the Temucraft player role 1010, so every account has them by default. To take one away, remove its
-- link here (or deny it per account with .rbac account deny <account> <id>); to give it to staff only, link it to
-- 1011-1014 instead.
DELETE FROM `rbac_linked_permissions` WHERE `linkedId` BETWEEN 102950 AND 102999;
DELETE FROM `rbac_permissions` WHERE `id` BETWEEN 102950 AND 102999;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(102950, 'Command: temucraft rangedauto rightclick'),
(102951, 'Command: temucraft rangedauto melee'),
(102952, 'Command: temucraft rangedauto');
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(195, 102950),
(1010, 102950),
(195, 102951),
(1010, 102951),
(195, 102952),
(1010, 102952);
