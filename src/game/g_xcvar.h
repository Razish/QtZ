
#ifdef XCVAR_PROTO
	#define XCVAR_DEF( name, defVal, notify, flags ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
	#define XCVAR_DEF( name, defVal, notify, flags ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
	#define XCVAR_DEF( name, defVal, notify, flags ) { & name , #name , defVal , flags , notify },
#endif

XCVAR_DEF( qtz_version,							QTZ_VERSION,		qfalse,	CVAR_SERVERINFO )
XCVAR_DEF( gamename,							GAMEVERSION,		qfalse, CVAR_SERVERINFO|CVAR_ROM )
XCVAR_DEF( gamedate,							__DATE__,			qfalse,	CVAR_SERVERINFO|CVAR_ROM )
XCVAR_DEF( ui_singlePlayerActive,				"",					qfalse,	CVAR_ROM )

//RAZTODO: ClientConnect/ClientUserinfoChanged from modbase/JA++	
XCVAR_DEF( qtz_antiFakePlayer,					"1",				qfalse,	CVAR_ARCHIVE )
XCVAR_DEF( qtz_randFix,							"1",				qfalse,	CVAR_ARCHIVE )
XCVAR_DEF( qtz_delagHitscan,					"1",				qfalse,	CVAR_SERVERINFO )
XCVAR_DEF( g_friendlyFireScale,					"1.0",				qfalse,	CVAR_ARCHIVE )
XCVAR_DEF( g_enableHeadshots,					"1",				qfalse,	CVAR_ARCHIVE )
XCVAR_DEF( g_bounceMissileStyle,				"0",				qfalse,	CVAR_ARCHIVE )
XCVAR_DEF( qtz_cinfo,							"0",				qfalse,	CVAR_SERVERINFO )

//Physics - must match cg_xcvar.h
XCVAR_DEF( qtz_phys_snapVec,					"0",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_overbounce,					"0",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_rampJumpEnable,				"1",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_wallJumpEnable,				"1",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_wallJumpDebounce,			"900",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_bunnyHopEnable,				"1",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_bunnyHopDebounce,			"250",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_doubleJumpEnable,			"1",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_doubleJumpPush,				"100",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_doubleJumpDebounce,			"300",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_accelerate,					"12.0",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airaccelerate,				"0.6",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_friction,					"10.0",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_jumpVelocity,				"250.0",			qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControlEnable,			"1",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControl,					"100.0",			qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControlStopAccelerate,	"2.5",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControlWishspeed,		"15",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControlStrafeAccelerate,	"100",				qtrue,	CVAR_ARCHIVE|CVAR_SYSTEMINFO )

XCVAR_DEF( pmove_fixed,							"0",				qtrue,	CVAR_SYSTEMINFO )
XCVAR_DEF( pmove_msec,							"8",				qtrue,	CVAR_SYSTEMINFO )
XCVAR_DEF( g_speed,								"350",				qtrue,	CVAR_ARCHIVE )
XCVAR_DEF( g_gravity,							"800",				qtrue,	CVAR_ARCHIVE )
XCVAR_DEF( g_knockback,							"1200",				qtrue,	CVAR_ARCHIVE )
XCVAR_DEF( g_quadFactor,						"3",				qtrue,	CVAR_ARCHIVE )
XCVAR_DEF( g_weaponRespawn,						"15",				qtrue,	CVAR_ARCHIVE )
XCVAR_DEF( g_inactivity,						"0",				qtrue,	CVAR_ARCHIVE )
//RAZTODO: rewrite callvote akin to JA++
XCVAR_DEF( g_allowVote,							"1",				qtrue,	CVAR_ARCHIVE )
XCVAR_DEF( g_friendlyFire,						"2",				qtrue,	CVAR_SYSTEMINFO|CVAR_ARCHIVE )
XCVAR_DEF( g_teamAutoJoin,						"0",				qfalse,	CVAR_ARCHIVE )
XCVAR_DEF( g_teamForceBalance,					"0",				qfalse,	CVAR_ARCHIVE )
XCVAR_DEF( g_forceRespawn,						"20",				qtrue,	CVAR_ARCHIVE )
//RAZTODO: delay mega spawns like powerups
XCVAR_DEF( g_delayMega,							"0",				qtrue,	CVAR_ARCHIVE )

XCVAR_DEF( dedicated,							"0",				qfalse,	CVAR_INIT )
XCVAR_DEF( sv_cheats,							"0",				qtrue,	CVAR_ROM )
XCVAR_DEF( g_restarted,							"0",				qfalse,	CVAR_ROM )
XCVAR_DEF( g_motd,								"#QtZ @ QuakeNET!",	qfalse,	CVAR_ARCHIVE )
XCVAR_DEF( g_synchronousClients,				"0",				qfalse,	CVAR_SYSTEMINFO )
XCVAR_DEF( g_smoothClients,						"1",				qtrue,	CVAR_ARCHIVE )
XCVAR_DEF( g_debugDamage,						"0",				qtrue,	CVAR_ARCHIVE )
XCVAR_DEF( g_log,								"games.log",		qfalse,	CVAR_ARCHIVE )
XCVAR_DEF( g_logSync,							"0",				qfalse,	CVAR_ARCHIVE )

XCVAR_DEF( g_password,							"",					qfalse,	CVAR_ARCHIVE )
XCVAR_DEF( g_needpass,							"0",				qtrue,	CVAR_SYSTEMINFO|CVAR_ROM )
//RAZTODO: Rewrite banning. file > cvar?
XCVAR_DEF( g_banIPs,							"",					qfalse,	CVAR_ARCHIVE )
XCVAR_DEF( g_filterBan,							"1",				qfalse,	CVAR_ARCHIVE )

XCVAR_DEF( g_gametype,							"0",				qfalse,	CVAR_SERVERINFO|CVAR_LATCH )
XCVAR_DEF( sv_maxclients,						"12",				qfalse,	CVAR_SERVERINFO|CVAR_LATCH|CVAR_ARCHIVE )
XCVAR_DEF( g_maxGameClients,					"0",				qfalse,	CVAR_SERVERINFO|CVAR_LATCH|CVAR_ARCHIVE )
XCVAR_DEF( dmflags,								"0",				qfalse,	CVAR_SERVERINFO|CVAR_ARCHIVE )
XCVAR_DEF( fraglimit,							"0",				qtrue,	CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART )
XCVAR_DEF( timelimit,							"10",				qtrue,	CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART )
XCVAR_DEF( capturelimit,						"0",				qtrue,	CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NORESTART )
XCVAR_DEF( g_warmup,							"300",				qfalse,	CVAR_ARCHIVE )
XCVAR_DEF( g_doWarmup,							"1",				qtrue,	CVAR_SERVERINFO|CVAR_ARCHIVE )

XCVAR_DEF( g_redteam,							"Red",				qtrue,	CVAR_ARCHIVE|CVAR_SERVERINFO )
XCVAR_DEF( g_blueteam,							"Blue",				qtrue,	CVAR_ARCHIVE|CVAR_SERVERINFO )

#undef XCVAR_DEF
