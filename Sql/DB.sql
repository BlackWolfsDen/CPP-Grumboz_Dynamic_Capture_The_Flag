CREATE TABLE `grumboz_ctf` (
	`guid` BIGINT(20) UNSIGNED NOT NULL COMMENT 'guid of player toon',
	`acct_id` MEDIUMINT(8) UNSIGNED NOT NULL,
	`name` VARCHAR(255) NOT NULL,
	`captures` MEDIUMINT(8) UNSIGNED NOT NULL,
	UNIQUE INDEX `guid` (`guid`)
)
COLLATE='utf8_general_ci'
ENGINE=InnoDB;