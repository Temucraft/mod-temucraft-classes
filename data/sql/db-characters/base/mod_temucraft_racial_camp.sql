-- mod-temucraft-classes (racials): the Vulpera camp (Make Camp / Return to Camp). Re-runnable.
CREATE TABLE IF NOT EXISTS `mod_temucraft_racial_camp` (
  `guid` INT UNSIGNED NOT NULL,
  `map` SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `x` FLOAT NOT NULL DEFAULT 0,
  `y` FLOAT NOT NULL DEFAULT 0,
  `z` FLOAT NOT NULL DEFAULT 0,
  `o` FLOAT NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
