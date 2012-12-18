
#ifdef XCVAR_PROTO
	#define XCVAR_DEF( name, defVal, flags ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
	#define XCVAR_DEF( name, defVal, flags ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
	#define XCVAR_DEF( name, defVal, flags ) { & name , #name , defVal , flags },
#endif

XCVAR_DEF( qtz_version,							QTZ_VERSION,	CVAR_SERVERINFO )

XCVAR_DEF( qtz_antiFakePlayer,					"1",			CVAR_ARCHIVE )
XCVAR_DEF( qtz_randFix,							"1",			CVAR_ARCHIVE )
XCVAR_DEF( qtz_showHeartbeats,					"0",			CVAR_ARCHIVE )
XCVAR_DEF( qtz_delagHitscan,					"1",			CVAR_SERVERINFO )
XCVAR_DEF( g_friendlyFireScale,					"1.0",			CVAR_ARCHIVE )
XCVAR_DEF( g_enableHeadshots,					"1",			CVAR_ARCHIVE )
XCVAR_DEF( g_bounceMissileStyle,				"0",			CVAR_ARCHIVE )
XCVAR_DEF( qtz_cinfo,							"0",			CVAR_SERVERINFO )

//Physics - must match cg_xcvar.h
XCVAR_DEF( qtz_phys_wallJumpEnable,				"1",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_wallJumpDebounce,			"1250",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_bunnyHopEnable,				"1",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_bunnyHopDebounce,			"400",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_doubleJumpEnable,			"1",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_doubleJumpPush,				"100",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_doubleJumpDebounce,			"400",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_accelerate,					"15.0",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airaccelerate,				"1.0",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_friction,					"8.0",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_jumpVelocity,				"270.0",		CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControlEnable,			"1",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControl,					"150.0",		CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControlStopAccelerate,	"2.5",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControlWishspeed,		"30",			CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControlStrafeAccelerate,	"70",			CVAR_SYSTEMINFO )

#undef XCVAR_DEF
