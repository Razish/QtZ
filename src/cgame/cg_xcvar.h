
#ifdef XCVAR_PROTO
	#define XCVAR_DEF( name, defVal, flags ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
	#define XCVAR_DEF( name, defVal, flags ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
	#define XCVAR_DEF( name, defVal, flags ) { & name , #name , defVal , flags },
#endif

XCVAR_DEF( qtz_version,							QTZ_VERSION,	CVAR_SERVERINFO			)
//XCVAR_DEF( com_maxFPS,							"125",			CVAR_ARCHIVE			)

XCVAR_DEF( cg_smoothCamera,						"0",			CVAR_ARCHIVE			)

XCVAR_DEF( cg_crosshairColour,					"0 1 1",		CVAR_ARCHIVE			)
XCVAR_DEF( cg_crosshairFeedbackColour,			"180 1 .5",		CVAR_ARCHIVE			)
XCVAR_DEF( cg_crosshairFeedbackTime,			"1000",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_crosshairCooldownTics,			"16",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_crosshairCooldownSpread,			"16",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_crosshairCooldownWeapons,			"8",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_crosshairDisableZoomed,			"0",			CVAR_ARCHIVE			)

XCVAR_DEF( cg_debugScoreboard,					"0",			CVAR_NONE				)
XCVAR_DEF( cg_debugHUD,							"0",			CVAR_NONE				)

XCVAR_DEF( cg_drawFlagCarrier,					"1",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_flagScale,						"0.5",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_itemPickupTime,					"3500",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_newObituary,						"1",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_obituaryTime,						"5000",			CVAR_ARCHIVE			)

XCVAR_DEF( cg_drawPing,							"1",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_lagometerX,						"592",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_lagometerY,						"432",			CVAR_ARCHIVE			)

XCVAR_DEF( cg_brightModels,						"0",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_forceEnemyModel,					"none",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_forceEnemyModelHue,				"120",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_forceEnemyModelSat,				"1.0",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_forceEnemyModelLum,				"0.5",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_forceEnemyModelAlpha,				"1.0",			CVAR_ARCHIVE			)

XCVAR_DEF( cg_forceTeamModel,					"none",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_forceTeamModelHue,				"140",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_forceTeamModelSat,				"1.0",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_forceTeamModelLum,				"0.5",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_forceTeamModelAlpha,				"1.0",			CVAR_ARCHIVE			)

XCVAR_DEF( cg_damageViewKick,					"1",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_zoomTime,							"0.55",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_gunZoomX,							"-2.0",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_gunZoomY,							"4.0",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_gunZoomZ,							"2.35",			CVAR_ARCHIVE			)

XCVAR_DEF( cg_gunIdleDrift,						"1",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_gunIdleDriftSpeed,				"0.001",		CVAR_ARCHIVE			)
XCVAR_DEF( cg_gunIdleDriftPitch,				"0.01",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_gunIdleDriftYaw,					"0.01",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_gunIdleDriftRoll,					"0.01",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_gunBob,							"1",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_gunBobPitch,						"0.005",		CVAR_ARCHIVE			)
XCVAR_DEF( cg_gunBobYaw,						"0.01",			CVAR_ARCHIVE			)
XCVAR_DEF( cg_gunBobRoll,						"0.005",		CVAR_ARCHIVE			)

//Physics - must match g_xcvar.h
XCVAR_DEF( qtz_phys_wallJumpEnable,				"1",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_wallJumpDebounce,			"1250",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_bunnyHopEnable,				"1",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_bunnyHopDebounce,			"400",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_doubleJumpEnable,			"1",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_doubleJumpPush,				"100",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_doubleJumpDebounce,			"400",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_accelerate,					"15.0",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_airaccelerate,				"1.0",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_friction,					"8.0",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_jumpVelocity,				"270.0",		CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_airControlEnable,			"1",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_airControl,					"150.0",		CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_airControlStopAccelerate,	"2.5",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_airControlWishspeed,		"30",			CVAR_SYSTEMINFO			)
XCVAR_DEF( qtz_phys_airControlStrafeAccelerate,	"70",			CVAR_SYSTEMINFO			)

#undef XCVAR_DEF
