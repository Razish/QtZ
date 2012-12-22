
#ifdef XCVAR_PROTO
	#define XCVAR_DEF( name, defVal, flags ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
	#define XCVAR_DEF( name, defVal, flags ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
	#define XCVAR_DEF( name, defVal, flags ) { & name , #name , defVal , flags },
#endif

XCVAR_DEF( cg_smoothCamera,						"0",					CVAR_ARCHIVE )

XCVAR_DEF( cg_crosshairColour,					"1 1 1",				CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairFeedbackColour,			"1 0 0",				CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairFeedbackTime,			"200",					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairCooldownTics,			"16",					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairCooldownSpread,			"16",					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairCooldownWeapons,			"27",					CVAR_ARCHIVE )
XCVAR_DEF( cg_crosshairDisableZoomed,			"0",					CVAR_ARCHIVE )

XCVAR_DEF( cg_debugScoreboard,					"0",					CVAR_NONE )
XCVAR_DEF( cg_debugHUD,							"0",					CVAR_NONE )

XCVAR_DEF( cg_drawFlagCarrier,					"1",					CVAR_ARCHIVE )
XCVAR_DEF( cg_flagScale,						"0.5",					CVAR_ARCHIVE )
XCVAR_DEF( cg_itemPickupTime,					"3500",					CVAR_ARCHIVE )
XCVAR_DEF( cg_newObituary,						"1",					CVAR_ARCHIVE )
XCVAR_DEF( cg_obituaryTime,						"5000",					CVAR_ARCHIVE )

XCVAR_DEF( cg_drawPing,							"1",					CVAR_ARCHIVE )
XCVAR_DEF( cg_lagometerPos,						"592 432",				CVAR_ARCHIVE )

XCVAR_DEF( cg_brightModels,						"1",					CVAR_ARCHIVE )
XCVAR_DEF( cg_forceEnemyModel,					"none",					CVAR_ARCHIVE )
XCVAR_DEF( cg_forceAllyModel,					"none",					CVAR_ARCHIVE )
XCVAR_DEF( cg_forceEnemyModelColor,				"0.0 1.0 0.0 1.0",		CVAR_ARCHIVE )
XCVAR_DEF( cg_forceAllyModelColor,				"0.5 0.5 0.5 1.0",		CVAR_ARCHIVE )

XCVAR_DEF( cg_damageViewKick,					"0",					CVAR_ARCHIVE )
XCVAR_DEF( cg_zoomTime,							"0.55",					CVAR_ARCHIVE )

XCVAR_DEF( cg_gunAlign,							"0 0 0",				CVAR_ARCHIVE/*|CVAR_USERINFO*/ )
XCVAR_DEF( cg_gunZoomAlign,						"-2.0 4.0 2.35",		CVAR_ARCHIVE )
XCVAR_DEF( cg_gunBobEnable,						"1",					CVAR_ARCHIVE )
XCVAR_DEF( cg_gunBob,							"0.005 0.01 0.005",		CVAR_ARCHIVE )
XCVAR_DEF( cg_gunIdleDriftEnable,				"1",					CVAR_ARCHIVE )
XCVAR_DEF( cg_gunIdleDrift,						"0.01 0.01 0.01 0.004",	CVAR_ARCHIVE )
XCVAR_DEF( cg_viewBobEnable,					"0",					CVAR_ARCHIVE )
XCVAR_DEF( cg_viewBob,							"0.002 0.002 0.005 1",	CVAR_ARCHIVE )
XCVAR_DEF( cg_instantDuck,						"1",					CVAR_ARCHIVE )

//Physics - must match g_xcvar.h
XCVAR_DEF( qtz_phys_snapVec,					"0",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_overbounce,					"0",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_rampJumpEnable,				"1",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_wallJumpEnable,				"1",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_wallJumpDebounce,			"900",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_bunnyHopEnable,				"1",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_bunnyHopDebounce,			"250",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_doubleJumpEnable,			"1",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_doubleJumpPush,				"100",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_doubleJumpDebounce,			"300",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_accelerate,					"12.0",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airaccelerate,				"0.6",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_friction,					"10.0",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_jumpVelocity,				"250.0",				CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControlEnable,			"1",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControl,					"100.0",				CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControlStopAccelerate,	"2.5",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControlWishspeed,		"15",					CVAR_SYSTEMINFO )
XCVAR_DEF( qtz_phys_airControlStrafeAccelerate,	"100",					CVAR_SYSTEMINFO )

#undef XCVAR_DEF
