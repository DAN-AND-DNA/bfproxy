drop table if exists bf_user_1;

create table bf_user_1 (
    `openid` VARCHAR(45) NOT NULL,
    `userid` bigint(20) unsigned NOT NULL,
    `updatetime` INT UNSIGNED DEFAULT 0,
    `player` blob,
    UNIQUE KEY `openid_index` (`openid`),
    PRIMARY KEY(`userid`)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

