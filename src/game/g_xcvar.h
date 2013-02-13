
#ifdef XCVAR_PROTO
	#define XCVAR_DEF( name, defVal, notify, flags, desc ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
	#define XCVAR_DEF( name, defVal, notify, flags, desc ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
	#define XCVAR_DEF( name, defVal, notify, flags, desc ) { & name , #name , defVal , flags , desc , notify },
#endif

XCVAR_DEF( qtz_version,						QTZ_VERSION,		qfalse,	CVAR_SERVERINFO,								NULL )
XCVAR_DEF( gamename,						GAMEVERSION,		qfalse, CVAR_SERVERINFO|CVAR_ROM,						NULL )
XCVAR_DEF( gamedate,						__DATE__,			qfalse,	CVAR_SERVERINFO|CVAR_ROM,						NULL )
XCVAR_DEF( sv_fps,							"40",				qtrue,	CVAR_ARCHIVE,									NULL )

//RAZTODO: ClientConnect/ClientUserinfoChanged from modbase/JA++	
XCVAR_DEF( g_antiFakePlayer,				"1",				qfalse,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_randFix,						"1",				qfalse,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_delagHitscan,					"1",				qtrue,	CVAR_SERVERINFO,								NULL )
XCVAR_DEF( g_delagHistory,					"1000",				qtrue,	CVAR_ARCHIVE,									"How far back to store client positions for ping compensation" )
XCVAR_DEF( g_friendlyFireScale,				"1.0",				qfalse,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_enableHeadshots,				"1",				qfalse,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_bounceMissileStyle,			"0",				qfalse,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( qtz_cinfo,						"0",				qfalse,	CVAR_SERVERINFO,								NULL )

//Physics - must match cg_xcvar.h
XCVAR_DEF( pm_fixed,						"1",				qtrue,	CVAR_SYSTEMINFO,								NULL )
XCVAR_DEF( pm_frametime,					"8",				qtrue,	CVAR_SYSTEMINFO,								NULL )
XCVAR_DEF( pm_snapVec,						"0",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_overbounce,					"0",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_landAccelerateEnable,			"1",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_rampJumpEnable,				"1",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_wallJumpEnable,				"1",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_wallJumpDebounce,				"900",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_bunnyHopEnable,				"1",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_bunnyHopDebounce,				"250",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_doubleJumpEnable,				"1",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_doubleJumpPush,				"100",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_doubleJumpDebounce,			"300",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_accelerate,					"15.0",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_airaccelerate,				"1.0",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_friction,						"10.0",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_jumpVelocity,					"250.0",			qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_airControlStyle,				"2",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_airControl,					"100.0",			qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_airControlStopAccelerate,		"2.5",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_airControlWishspeed,			"15",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )
XCVAR_DEF( pm_airControlStrafeAccelerate,	"100.0",			qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO,					NULL )

XCVAR_DEF( g_speed,							"350",				qtrue,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_gravity,						"800",				qtrue,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_knockback,						"1200",				qtrue,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_quadFactor,					"3",				qtrue,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_weaponRespawn,					"15",				qtrue,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_inactivity,					"0",				qtrue,	CVAR_ARCHIVE,									NULL )
//RAZTODO: rewrite callvote akin to JA++
XCVAR_DEF( g_allowVote,						"1",				qtrue,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_friendlyFire,					"2",				qtrue,	CVAR_SYSTEMINFO|CVAR_ARCHIVE,					NULL )
XCVAR_DEF( g_teamAutoJoin,					"0",				qfalse,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_teamForceBalance,				"0",				qfalse,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_forceRespawn,					"20",				qtrue,	CVAR_ARCHIVE,									NULL )
//RAZTODO: delay mega spawns like powerups
XCVAR_DEF( g_delayMega,						"0",				qtrue,	CVAR_ARCHIVE,									NULL )

XCVAR_DEF( dedicated,						"0",				qfalse,	CVAR_INIT,										NULL )
XCVAR_DEF( sv_cheats,						"0",				qtrue,	CVAR_ROM,										NULL )
XCVAR_DEF( g_restarted,						"0",				qfalse,	CVAR_ROM,										NULL )
XCVAR_DEF( g_motd,							"#QtZ @ QuakeNET!",	qfalse,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_synchronousClients,			"0",				qfalse,	CVAR_SYSTEMINFO,								NULL )
XCVAR_DEF( g_smoothClients,					"1",				qtrue,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_debugDamage,					"0",				qtrue,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_log,							"games.log",		qfalse,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_logSync,						"0",				qfalse,	CVAR_ARCHIVE,									NULL )

XCVAR_DEF( g_password,						"",					qfalse,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_needpass,						"0",				qtrue,	CVAR_SYSTEMINFO|CVAR_ROM,						NULL )
//RAZTODO: Rewrite banning. file > cvar?
XCVAR_DEF( g_banIPs,						"",					qfalse,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_filterBan,						"1",				qfalse,	CVAR_ARCHIVE,									NULL )

XCVAR_DEF( g_gametype,						"0",				qfalse,	CVAR_SERVERINFO|CVAR_LATCH,						NULL )
XCVAR_DEF( sv_maxclients,					"12",				qfalse,	CVAR_SERVERINFO|CVAR_LATCH|CVAR_ARCHIVE,		NULL )
XCVAR_DEF( g_maxGameClients,				"0",				qfalse,	CVAR_SERVERINFO|CVAR_LATCH|CVAR_ARCHIVE,		NULL )
XCVAR_DEF( dmflags,							"0",				qfalse,	CVAR_SERVERINFO|CVAR_ARCHIVE,					NULL )
XCVAR_DEF( fraglimit,						"0",				qtrue,	CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART,	NULL )
XCVAR_DEF( timelimit,						"10",				qtrue,	CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART,	NULL )
XCVAR_DEF( capturelimit,					"0",				qtrue,	CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART,	NULL )
XCVAR_DEF( g_warmup,						"300",				qfalse,	CVAR_ARCHIVE,									NULL )
XCVAR_DEF( g_doWarmup,						"1",				qtrue,	CVAR_SERVERINFO|CVAR_ARCHIVE,					NULL )
XCVAR_DEF( g_pauseTime,						"120",				qtrue,	CVAR_ARCHIVE,									"How long a match will be paused for" )
XCVAR_DEF( g_unpauseTime,					"5",				qtrue,	CVAR_ARCHIVE,									"How long a match takes to unpause" )

XCVAR_DEF( g_redteam,						"Red",				qtrue,	CVAR_ARCHIVE|CVAR_SERVERINFO,					NULL )
XCVAR_DEF( g_blueteam,						"Blue",				qtrue,	CVAR_ARCHIVE|CVAR_SERVERINFO,					NULL )

#undef XCVAR_DEF
