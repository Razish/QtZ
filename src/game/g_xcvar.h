
#ifdef XCVAR_PROTO
	#define XCVAR_DEF( name, defVal, notify, update, flags, desc ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
	#define XCVAR_DEF( name, defVal, notify, update, flags, desc ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
	#define XCVAR_DEF( name, defVal, notify, update, flags, desc ) { & name , #name , defVal , update , flags , desc , notify },
#endif

//			name							default value		announce	update		flags											description
XCVAR_DEF( gamename,						GAMEVERSION,		qfalse,		NULL,		CVAR_SERVERINFO|CVAR_ROM,						NULL )
XCVAR_DEF( gamedate,						__DATE__,			qfalse,		NULL,		CVAR_SERVERINFO|CVAR_ROM,						"Build date of QuantiZe" )
XCVAR_DEF( sv_frametime,					"25",				qtrue,		NULL,		CVAR_NONE,										"Server update rate" )
XCVAR_DEF( sv_gametype,						"dm",				qfalse,		NULL,		0,												"Gametype" )
XCVAR_DEF( g_version,						QTZ_VERSION,		qfalse,		NULL,		CVAR_SERVERINFO|CVAR_ROM,						"Version of QuantiZe" )

XCVAR_DEF( capturelimit,					"0",				qtrue,		NULL,		CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART,	"In Flag gametypes, the game will end when either team reaches this number of flag captures" )
XCVAR_DEF( dedicated,						"0",				qfalse,		NULL,		CVAR_INIT,										"Must set on command line" )
XCVAR_DEF( dmflags,							"0",				qfalse,		NULL,		CVAR_SERVERINFO|CVAR_ARCHIVE,					NULL )
XCVAR_DEF( fraglimit,						"0",				qtrue,		NULL,		CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART,	"In Bloodbath gametypes, the game will end when a player or team reaches this many frags" )
XCVAR_DEF( g_allowFlagDrop,					"1",				qtrue,		NULL,		CVAR_ARCHIVE,									"Allow dropping flags in CTF gametypes" )
XCVAR_DEF( g_allowVote,						"1",				qtrue,		NULL,		CVAR_ARCHIVE,									"Allow certain voting options" )
XCVAR_DEF( g_allowWeaponDrop,				"1",				qtrue,		NULL,		CVAR_ARCHIVE,									"Allow dropping weapons" )
XCVAR_DEF( g_ammoRespawnTime,				"20",				qtrue,		NULL,		CVAR_ARCHIVE,									"How many seconds before ammo respawns after it is picked up" )
XCVAR_DEF( g_banIPs,						"",					qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_blueteam,						"Blue",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SERVERINFO,					NULL )
XCVAR_DEF( g_bounceMissileStyle,			"0",				qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_debugDamage,					"0",				qtrue,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_delagHistory,					"1000",				qtrue,		NULL,		CVAR_ARCHIVE,									"How far back to store client positions for ping compensation" )
XCVAR_DEF( g_delagHitscan,					"1",				qtrue,		NULL,		CVAR_SERVERINFO,								NULL )
XCVAR_DEF( g_delayMajorItems,				"0",				qtrue,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_difficulty,					"3",				qtrue,		NULL,		CVAR_ARCHIVE,									"Difficulty level for bots [1-3]" )
XCVAR_DEF( g_doWarmup,						"1",				qtrue,		NULL,		CVAR_SERVERINFO|CVAR_ARCHIVE,					NULL )
XCVAR_DEF( g_enableHeadshots,				"1",				qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_filterBan,						"1",				qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_forceRespawn,					"20",				qtrue,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_friendlyFire,					"2",				qtrue,		NULL,		CVAR_SYSTEMINFO|CVAR_ARCHIVE,					NULL )
XCVAR_DEF( g_friendlyFireScale,				"1.0",				qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_holdableRespawnTime,			"60",				qtrue,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_inactivity,					"0",				qtrue,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_knockback,						"1200",				qtrue,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_log,							"games.log",		qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_logSync,						"0",				qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_maxGameClients,				"0",				qfalse,		NULL,		CVAR_SERVERINFO|CVAR_LATCH|CVAR_ARCHIVE,		NULL )
XCVAR_DEF( g_motd,							"#QtZ @ QuakeNET!",	qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_needpass,						"0",				qtrue,		NULL,		CVAR_SYSTEMINFO|CVAR_ROM,						NULL )
XCVAR_DEF( g_password,						"",					qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_pauseTime,						"120",				qtrue,		NULL,		CVAR_ARCHIVE,									"How long a match will be paused for" )
XCVAR_DEF( g_powerupRespawnTime,			"120",				qtrue,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_quadFactor,					"2",				qtrue,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_randFix,						"1",				qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_redTeam,						"Red",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SERVERINFO,					NULL )
XCVAR_DEF( g_restarted,						"0",				qfalse,		NULL,		CVAR_ROM,										NULL )
XCVAR_DEF( g_smoothClients,					"1",				qtrue,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_synchronousClients,			"0",				qfalse,		NULL,		CVAR_SYSTEMINFO,								NULL )
XCVAR_DEF( g_teamAutoJoin,					"0",				qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_teamForceBalance,				"0",				qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_unpauseTime,					"5",				qtrue,		NULL,		CVAR_ARCHIVE,									"How long a match takes to unpause" )
XCVAR_DEF( g_voteDelay,						"3000",				qfalse,		NULL,		CVAR_NONE,										NULL )
XCVAR_DEF( g_voteDisable,					"0",				qfalse,		NULL,		CVAR_NONE,										NULL )
XCVAR_DEF( g_warmup,						"120",				qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_warmupPrintDelay,				"15",				qfalse,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_weaponRespawnTime,				"15",				qtrue,		NULL,		CVAR_ARCHIVE,									NULL )
XCVAR_DEF( pm_acceleration,					"16.0",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_bunnyHopDebounce,				"125",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_bunnyHopEnable,				"1",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_doubleJumpDebounce,			"300",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_doubleJumpEnable,				"1",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_doubleJumpPush,				"100",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_frametime,					"20",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_friction,						"8.0",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_gravity,						"1100",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_jumpVelocity,					"300.0",			qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_overbounce,					"0",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_rampJumpEnable,				"1",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_float,						"1",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_speed,						"320",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_wallJumpEnable,				"1",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_wallJumpDebounce,				"900",				qtrue,		NULL,		CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( sv_cheats,						"0",				qtrue,		NULL,		0,												NULL )
XCVAR_DEF( sv_maxclients,					"8",				qfalse,		NULL,		0,												NULL )
XCVAR_DEF( timelimit,						"10",				qtrue,		NULL,		CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART,	NULL )

#undef XCVAR_DEF
