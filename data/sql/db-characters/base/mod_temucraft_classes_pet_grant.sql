-- mod-temucraft-classes: characters already handled by TemucraftStartingPetGrant.cpp (older hunters and warlocks get
-- their starting pet once). A row means "done", also for characters that already had a pet. Re-runnable.
CREATE TABLE IF NOT EXISTS `mod_temucraft_classes_pet_grant` (
  `guid` INT UNSIGNED NOT NULL COMMENT 'characters.guid',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Temucraft: hunters and warlocks already given (or checked for) their starting pet';
