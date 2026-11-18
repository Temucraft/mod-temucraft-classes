-- mod-temucraft-classes: characters tables
CREATE TABLE IF NOT EXISTS `mod_temucraft_reagent_choice` (
    `guid`         INT UNSIGNED NOT NULL,
    `use_reagents` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
