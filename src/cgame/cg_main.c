/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

#include "../ui/ui_shared.h"
// display context for new ui stuff
displayContextDef_t cgDC;

void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void CG_Shutdown( void );

cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];
weaponInfo_t		cg_weapons[MAX_WEAPONS];
itemInfo_t			cg_items[MAX_ITEMS];


// ********************************
// VMCVAR callback functions

static void CG_ForceModelUpdate( void ) {
	int i=0;

	for ( i=0; i<MAX_CLIENTS; i++ ) {
		const char *clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0] )
			continue;

		CG_NewClientInfo( i );
	}
}

static void CG_ForceColorUpdate( void ) {
	int *color = NULL;
	int i=0;
	
	// enemy color
	color = cg.forceModel.enemyColor;
	if ( sscanf( cg_forceEnemyModelColor.string, "%i %i %i", &color[0], &color[1], &color[2] ) != 3 ) {
		MAKERGB( color, 0, 255, 0 );
	}
	for ( i=0; i<3; i++ )
		color[i] &= 0xFF;

	// ally color
	color = cg.forceModel.allyColor;
	if ( sscanf( cg_forceAllyModelColor.string, "%i %i %i", &color[0], &color[1], &color[2] ) != 3 ) {
		MAKERGB( color, 127, 127, 127 );
	}
	for ( i=0; i<3; i++ )
		color[i] &= 0xFF;
}

static void CG_XHairColorUpdate( void ) {
	int *color = NULL;
	int i=0;
	
	// base color
	color = cg.crosshair.baseColor;
	if ( sscanf( cg_crosshairColour.string, "%i %i %i", &color[0], &color[1], &color[2] ) != 3 ) {
		MAKERGB( color, 0, 255, 0 );
	}
	for ( i=0; i<3; i++ )
		color[i] &= 0xFF;

	// feedback color
	color = cg.crosshair.feedbackColor;
	if ( sscanf( cg_crosshairFeedbackColour.string, "%i %i %i", &color[0], &color[1], &color[2] ) != 3 ) {
		MAKERGB( color, 127, 127, 127 );
	}
	for ( i=0; i<3; i++ )
		color[i] &= 0xFF;
}

static void CG_GunAlignUpdate( void ) {
	vector3 *v = NULL;

	// base position (x, y, z)
	v = &cg.gunAlign.basePos;
	if ( sscanf( cg_gunAlign.string, "%f %f %f", &v->x, &v->y, &v->z ) != 3 ) {
		v->x = v->y = v->z = 0.0f;
	}

	// zoom position (x, y, z)
	v = &cg.gunAlign.zoomPos;
	if ( sscanf( cg_gunZoomAlign.string, "%f %f %f", &v->x, &v->y, &v->z ) != 3 ) {
		v->x = -2.0f;
		v->y = 4.0f;
		v->z = 2.35f;
	}
}

static void CG_ViewVarsUpdate( void ) {
	angle3 *a = NULL;
	number *n = NULL;
	qboolean *b = NULL;

	// viewmodel bobbing (pitch, yaw, roll)
	a = &cg.gunBob;
	if ( sscanf( cg_gunBob.string, "%f %f %f", &a->pitch, &a->yaw, &a->roll ) != 3 ) {
		a->pitch = 0.005f;
		a->yaw = 0.01f;
		a->roll = 0.005f;
	}

	// viewmodel drifting (pitch, yaw, roll, speed)
	a = &cg.gunIdleDrift;
	n = &cg.gunIdleDriftSpeed;
	if ( sscanf( cg_gunIdleDrift.string, "%f %f %f %f", &a->pitch, &a->yaw, &a->roll, n ) != 4 ) {
		a->pitch = 0.01f;
		a->yaw = 0.01f;
		a->roll = 0.01f;
		*n = 0.004f;
	}

	// view bobbing (pitch, roll, up, fall)
	n = &cg.viewBob[0];
	b = &cg.viewBobFall;
	if ( sscanf( cg_viewBob.string, "%f %f %f %i", &n[0], &n[1], &n[2], b ) != 4 ) {
		n[0] = 0.002f;
		n[1] = 0.002f;
		n[2] = 0.005f;
		*b = qtrue;
	}
}

// If team overlay is on, ask for updates from the server.
// If it's off, let the server know so we don't receive it
static void CG_TeamOverlayUpdate( void ) {
	if ( cg_drawTeamOverlay.boolean )
		cgi.Cvar_Set( "teamoverlay", "1" );
	else
		cgi.Cvar_Set( "teamoverlay", "0" );
}


#define XCVAR_DECL
	#include "cg_xcvar.h"
#undef XCVAR_DECL

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	void		(*update)( void );
	int			cvarFlags;
	char		*description;
} cvarTable_t;

static cvarTable_t cvarTable[] = {
#define XCVAR_LIST
	#include "cg_xcvar.h"
#undef XCVAR_LIST
};

static const int cvarTableSize = ARRAY_LEN( cvarTable );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	char		var[MAX_TOKEN_CHARS];

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		cgi.Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags, cv->description );
		if ( cv->update )
			cv->update();
	}

	// see if we are also running the server on this machine
	cgi.Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	cgi.Cvar_Register( NULL, "model",			DEFAULT_MODEL,		CVAR_USERINFO|CVAR_ARCHIVE,	NULL );
	cgi.Cvar_Register( NULL, "team_model",		DEFAULT_TEAM_MODEL,	CVAR_USERINFO|CVAR_ARCHIVE,	NULL );
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		int modCount = cv->vmCvar->modificationCount;
		cgi.Cvar_Update( cv->vmCvar );
		if ( cv->vmCvar->modificationCount > modCount ) {
			if ( cv->update )
				cv->update();
		}
	}
}

int CG_CrosshairPlayer( void ) {
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) ) {
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
	if ( !cg.attackerTime ) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	cgi.Print( text );
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	cgi.Error( ERR_DROP, text );
}

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	cgi.Error( level, text );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	cgi.Print( text );
}

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	cgi.Cmd_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}

const char *CG_Cvar_VariableString( const char *var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	cgi.Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	gitem_t			*item;
	char			data[MAX_QPATH];
	char			*s, *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if( item->pickup_sound ) {
		cgi.S_RegisterSound( item->pickup_sound, qfalse );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string", 
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "wav" )) {
			cgi.S_RegisterSound( data, qfalse );
		}
	}
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int		i;
	char	items[MAX_ITEMS+1];
	char	name[MAX_QPATH];
	const char	*soundName;

	// voice commands
	CG_LoadVoiceChats();

	cgs.media.oneMinuteSound = cgi.S_RegisterSound( "sound/feedback/1_minute.wav", qtrue );
	cgs.media.fiveMinuteSound = cgi.S_RegisterSound( "sound/feedback/5_minute.wav", qtrue );
	cgs.media.suddenDeathSound = cgi.S_RegisterSound( "sound/feedback/sudden_death.wav", qtrue );
	cgs.media.oneFragSound = cgi.S_RegisterSound( "sound/feedback/1_frag.wav", qtrue );
	cgs.media.twoFragSound = cgi.S_RegisterSound( "sound/feedback/2_frags.wav", qtrue );
	cgs.media.threeFragSound = cgi.S_RegisterSound( "sound/feedback/3_frags.wav", qtrue );
	cgs.media.count3Sound = cgi.S_RegisterSound( "sound/feedback/three.wav", qtrue );
	cgs.media.count2Sound = cgi.S_RegisterSound( "sound/feedback/two.wav", qtrue );
	cgs.media.count1Sound = cgi.S_RegisterSound( "sound/feedback/one.wav", qtrue );
	cgs.media.countFightSound = cgi.S_RegisterSound( "sound/feedback/fight.wav", qtrue );
	cgs.media.countPrepareSound = cgi.S_RegisterSound( "sound/feedback/prepare.wav", qtrue );
	cgs.media.countPrepareTeamSound = cgi.S_RegisterSound( "sound/feedback/prepare_team.wav", qtrue );

	if ( cgs.gametype >= GT_TEAM ) {

		cgs.media.captureAwardSound = cgi.S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
		cgs.media.redLeadsSound = cgi.S_RegisterSound( "sound/feedback/redleads.wav", qtrue );
		cgs.media.blueLeadsSound = cgi.S_RegisterSound( "sound/feedback/blueleads.wav", qtrue );
		cgs.media.teamsTiedSound = cgi.S_RegisterSound( "sound/feedback/teamstied.wav", qtrue );
		cgs.media.hitTeamSound = cgi.S_RegisterSound( "sound/feedback/hit_teammate.wav", qtrue );

		cgs.media.redScoredSound = cgi.S_RegisterSound( "sound/teamplay/voc_red_scores.wav", qtrue );
		cgs.media.blueScoredSound = cgi.S_RegisterSound( "sound/teamplay/voc_blue_scores.wav", qtrue );

		cgs.media.captureYourTeamSound = cgi.S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
		cgs.media.captureOpponentSound = cgi.S_RegisterSound( "sound/teamplay/flagcapture_opponent.wav", qtrue );

		cgs.media.returnYourTeamSound = cgi.S_RegisterSound( "sound/teamplay/flagreturn_yourteam.wav", qtrue );
		cgs.media.returnOpponentSound = cgi.S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );

		cgs.media.takenYourTeamSound = cgi.S_RegisterSound( "sound/teamplay/flagtaken_yourteam.wav", qtrue );
		cgs.media.takenOpponentSound = cgi.S_RegisterSound( "sound/teamplay/flagtaken_opponent.wav", qtrue );

		if ( cgs.gametype == GT_CTF ) {
			cgs.media.redFlagReturnedSound = cgi.S_RegisterSound( "sound/teamplay/voc_red_returned.wav", qtrue );
			cgs.media.blueFlagReturnedSound = cgi.S_RegisterSound( "sound/teamplay/voc_blue_returned.wav", qtrue );
			cgs.media.enemyTookYourFlagSound = cgi.S_RegisterSound( "sound/teamplay/voc_enemy_flag.wav", qtrue );
			cgs.media.yourTeamTookEnemyFlagSound = cgi.S_RegisterSound( "sound/teamplay/voc_team_flag.wav", qtrue );
		}

#ifdef QTZRELIC
		cgs.media.youHaveFlagSound = cgi.S_RegisterSound( "sound/teamplay/voc_you_flag.wav", qtrue );
		cgs.media.holyShitSound = cgi.S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
#else
		if ( cgs.gametype == GT_1FCTF ) {
			// FIXME: get a replacement for this sound ?
			cgs.media.neutralFlagReturnedSound = cgi.S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );
			cgs.media.yourTeamTookTheFlagSound = cgi.S_RegisterSound( "sound/teamplay/voc_team_1flag.wav", qtrue );
			cgs.media.enemyTookTheFlagSound = cgi.S_RegisterSound( "sound/teamplay/voc_enemy_1flag.wav", qtrue );
		}

		if ( cgs.gametype == GT_1FCTF || cgs.gametype == GT_CTF ) {
			cgs.media.youHaveFlagSound = cgi.S_RegisterSound( "sound/teamplay/voc_you_flag.wav", qtrue );
			cgs.media.holyShitSound = cgi.S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
		}
#endif // QTZRELIC
	}

	cgs.media.tracerSound = cgi.S_RegisterSound( "sound/weapons/machinegun/buletby1.wav", qfalse );
	cgs.media.selectSound = cgi.S_RegisterSound( "sound/weapons/change.wav", qfalse );
	cgs.media.wearOffSound = cgi.S_RegisterSound( "sound/items/wearoff.wav", qfalse );
	cgs.media.useNothingSound = cgi.S_RegisterSound( "sound/items/use_nothing.wav", qfalse );
	cgs.media.gibSound = cgi.S_RegisterSound( "sound/player/gibsplt1.wav", qfalse );
	cgs.media.gibBounce1Sound = cgi.S_RegisterSound( "sound/player/gibimp1.wav", qfalse );
	cgs.media.gibBounce2Sound = cgi.S_RegisterSound( "sound/player/gibimp2.wav", qfalse );
	cgs.media.gibBounce3Sound = cgi.S_RegisterSound( "sound/player/gibimp3.wav", qfalse );

	cgs.media.guardSound = cgi.S_RegisterSound("sound/items/cl_guard.wav", qfalse);

	cgs.media.teleInSound = cgi.S_RegisterSound( "sound/world/telein.wav", qfalse );
	cgs.media.teleOutSound = cgi.S_RegisterSound( "sound/world/teleout.wav", qfalse );
	cgs.media.respawnSound = cgi.S_RegisterSound( "sound/items/respawn1.wav", qfalse );

	cgs.media.noAmmoSound = cgi.S_RegisterSound( "sound/weapons/noammo.wav", qfalse );

	cgs.media.talkSound = cgi.S_RegisterSound( "sound/player/talk.wav", qfalse );
	cgs.media.landSound = cgi.S_RegisterSound( "sound/player/land1.wav", qfalse);

	cgs.media.hitSound = cgi.S_RegisterSound( "sound/feedback/hit.wav", qfalse );
	cgs.media.hitSoundHighArmor = cgi.S_RegisterSound( "sound/feedback/hithi.wav", qfalse );
	cgs.media.hitSoundLowArmor = cgi.S_RegisterSound( "sound/feedback/hitlo.wav", qfalse );

	cgs.media.impressiveSound = cgi.S_RegisterSound( "sound/feedback/impressive.wav", qtrue );
	cgs.media.excellentSound = cgi.S_RegisterSound( "sound/feedback/excellent.wav", qtrue );
	cgs.media.deniedSound = cgi.S_RegisterSound( "sound/feedback/denied.wav", qtrue );
	cgs.media.humiliationSound = cgi.S_RegisterSound( "sound/feedback/humiliation.wav", qtrue );
	cgs.media.assistSound = cgi.S_RegisterSound( "sound/feedback/assist.wav", qtrue );
	cgs.media.defendSound = cgi.S_RegisterSound( "sound/feedback/defense.wav", qtrue );

	cgs.media.firstImpressiveSound = cgi.S_RegisterSound( "sound/feedback/first_impressive.wav", qtrue );
	cgs.media.firstExcellentSound = cgi.S_RegisterSound( "sound/feedback/first_excellent.wav", qtrue );
	cgs.media.firstHumiliationSound = cgi.S_RegisterSound( "sound/feedback/first_gauntlet.wav", qtrue );

	cgs.media.takenLeadSound = cgi.S_RegisterSound( "sound/feedback/takenlead.wav", qtrue);
	cgs.media.tiedLeadSound = cgi.S_RegisterSound( "sound/feedback/tiedlead.wav", qtrue);
	cgs.media.lostLeadSound = cgi.S_RegisterSound( "sound/feedback/lostlead.wav", qtrue);

	cgs.media.voteNow = cgi.S_RegisterSound( "sound/feedback/vote_now.wav", qtrue);
	cgs.media.votePassed = cgi.S_RegisterSound( "sound/feedback/vote_passed.wav", qtrue);
	cgs.media.voteFailed = cgi.S_RegisterSound( "sound/feedback/vote_failed.wav", qtrue);

	cgs.media.watrInSound = cgi.S_RegisterSound( "sound/player/watr_in.wav", qfalse);
	cgs.media.watrOutSound = cgi.S_RegisterSound( "sound/player/watr_out.wav", qfalse);
	cgs.media.watrUnSound = cgi.S_RegisterSound( "sound/player/watr_un.wav", qfalse);

	cgs.media.jumpPadSound = cgi.S_RegisterSound ("sound/world/jumppad.wav", qfalse );

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = cgi.S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = cgi.S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = cgi.S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/mech%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_MECH][i] = cgi.S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/energy%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = cgi.S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/splash%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = cgi.S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/clank%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = cgi.S_RegisterSound (name, qfalse);
	}

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
//		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_RegisterItemSounds( i );
//		}
	}

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS+i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' ) {
			continue;	// custom sound
		}
		cgs.gameSounds[i] = cgi.S_RegisterSound( soundName, qfalse );
	}

	// FIXME: only needed with item
	cgs.media.flightSound = cgi.S_RegisterSound( "sound/items/flight.wav", qfalse );
	cgs.media.medkitSound = cgi.S_RegisterSound ("sound/items/use_medkit.wav", qfalse);
	cgs.media.quadSound = cgi.S_RegisterSound("sound/items/damage3.wav", qfalse);
	cgs.media.weaponHoverSound = cgi.S_RegisterSound( "sound/weapons/weapon_hover.wav", qfalse );
	cgs.media.winnerSound = cgi.S_RegisterSound( "sound/feedback/voc_youwin.wav", qfalse );
	cgs.media.loserSound = cgi.S_RegisterSound( "sound/feedback/voc_youlose.wav", qfalse );

	cgs.media.wstbimplSound = cgi.S_RegisterSound("sound/weapons/proxmine/wstbimpl.wav", qfalse);
	cgs.media.wstbimpmSound = cgi.S_RegisterSound("sound/weapons/proxmine/wstbimpm.wav", qfalse);
	cgs.media.wstbimpdSound = cgi.S_RegisterSound("sound/weapons/proxmine/wstbimpd.wav", qfalse);
	cgs.media.wstbactvSound = cgi.S_RegisterSound("sound/weapons/proxmine/wstbactv.wav", qfalse);

	cgs.media.regenSound = cgi.S_RegisterSound("sound/items/regen.wav", qfalse);
	cgs.media.protectSound = cgi.S_RegisterSound("sound/items/protect3.wav", qfalse);
	cgs.media.n_healthSound = cgi.S_RegisterSound("sound/items/n_health.wav", qfalse );

	cgi.S_RegisterSound("sound/player/"DEFAULT_MODEL"/death1.wav", qfalse );
	cgi.S_RegisterSound("sound/player/"DEFAULT_MODEL"/death2.wav", qfalse );
	cgi.S_RegisterSound("sound/player/"DEFAULT_MODEL"/death3.wav", qfalse );
	cgi.S_RegisterSound("sound/player/"DEFAULT_MODEL"/jump1.wav", qfalse );
	cgi.S_RegisterSound("sound/player/"DEFAULT_MODEL"/pain25_1.wav", qfalse );
	cgi.S_RegisterSound("sound/player/"DEFAULT_MODEL"/pain75_1.wav", qfalse );
	cgi.S_RegisterSound("sound/player/"DEFAULT_MODEL"/pain100_1.wav", qfalse );
	cgi.S_RegisterSound("sound/player/"DEFAULT_MODEL"/falling1.wav", qfalse );
	cgi.S_RegisterSound("sound/player/"DEFAULT_MODEL"/gasp.wav", qfalse );
	cgi.S_RegisterSound("sound/player/"DEFAULT_MODEL"/drown.wav", qfalse );
	cgi.S_RegisterSound("sound/player/"DEFAULT_MODEL"/fall1.wav", qfalse );
	cgi.S_RegisterSound("sound/player/"DEFAULT_MODEL"/taunt.wav", qfalse );
}


//===================================================================================


/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	int			i;
	char		items[MAX_ITEMS+1];
	static char		*sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	cgi.R_ClearScene();

	CG_LoadingString( cgs.mapname );

	cgi.R_LoadWorld( cgs.mapname );

	// precache status bar pics
	CG_LoadingString( "game media" );

	for ( i=0 ; i<11 ; i++) {
		cgs.media.numberShaders[i] = cgi.R_RegisterShader( sb_nums[i] );
	}

	cgs.media.botSkillShaders[0] = cgi.R_RegisterShader( "menu/art/skill1.tga" );
	cgs.media.botSkillShaders[1] = cgi.R_RegisterShader( "menu/art/skill2.tga" );
	cgs.media.botSkillShaders[2] = cgi.R_RegisterShader( "menu/art/skill3.tga" );
	cgs.media.botSkillShaders[3] = cgi.R_RegisterShader( "menu/art/skill4.tga" );
	cgs.media.botSkillShaders[4] = cgi.R_RegisterShader( "menu/art/skill5.tga" );

	cgs.media.viewBloodShader = cgi.R_RegisterShader( "viewBloodBlend" );

	cgs.media.deferShader = cgi.R_RegisterShaderNoMip( "gfx/2d/defer.tga" );

	cgs.media.scoreboardName = cgi.R_RegisterShaderNoMip( "menu/tab/name.tga" );
	cgs.media.scoreboardPing = cgi.R_RegisterShaderNoMip( "menu/tab/ping.tga" );
	cgs.media.scoreboardScore = cgi.R_RegisterShaderNoMip( "menu/tab/score.tga" );
	cgs.media.scoreboardTime = cgi.R_RegisterShaderNoMip( "menu/tab/time.tga" );

	cgs.media.bloodTrailShader = cgi.R_RegisterShader( "bloodTrail" );
	cgs.media.lagometerShader = cgi.R_RegisterShader("lagometer" );
	cgs.media.connectionShader = cgi.R_RegisterShader( "disconnected" );

	cgs.media.tracerShader = cgi.R_RegisterShader( "gfx/misc/tracer" );
	cgs.media.selectShader = cgi.R_RegisterShader( "gfx/2d/select" );

	cgs.media.backTileShader = cgi.R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.noammoShader = cgi.R_RegisterShader( "icons/noammo" );

	// powerup shaders
	cgs.media.quadShader = cgi.R_RegisterShader("powerups/quad" );
	cgs.media.quadWeaponShader = cgi.R_RegisterShader("powerups/quadWeapon" );
	cgs.media.regenShader = cgi.R_RegisterShader("powerups/regen" );

	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF ) {
		cgs.media.redFlagModel = cgi.R_RegisterModel( "models/flags/r_flag.md3" );
		cgs.media.blueFlagModel = cgi.R_RegisterModel( "models/flags/b_flag.md3" );
		cgs.media.redFlagShader[0] = cgi.R_RegisterShaderNoMip( "icons/iconf_red1" );
		cgs.media.redFlagShader[1] = cgi.R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.redFlagShader[2] = cgi.R_RegisterShaderNoMip( "icons/iconf_red3" );
		cgs.media.blueFlagShader[0] = cgi.R_RegisterShaderNoMip( "icons/iconf_blu1" );
		cgs.media.blueFlagShader[1] = cgi.R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.blueFlagShader[2] = cgi.R_RegisterShaderNoMip( "icons/iconf_blu3" );

		cgs.media.flagPoleModel = cgi.R_RegisterModel( "models/flag2/flagpole.md3" );
		cgs.media.flagFlapModel = cgi.R_RegisterModel( "models/flag2/flagflap3.md3" );

		cgs.media.redFlagFlapSkin = cgi.R_RegisterSkin( "models/flag2/red.skin" );
		cgs.media.blueFlagFlapSkin = cgi.R_RegisterSkin( "models/flag2/blue.skin" );
		cgs.media.neutralFlagFlapSkin = cgi.R_RegisterSkin( "models/flag2/white.skin" );

		cgs.media.redFlagBaseModel = cgi.R_RegisterModel( "models/mapobjects/flagbase/red_base.md3" );
		cgs.media.blueFlagBaseModel = cgi.R_RegisterModel( "models/mapobjects/flagbase/blue_base.md3" );
		cgs.media.neutralFlagBaseModel = cgi.R_RegisterModel( "models/mapobjects/flagbase/ntrl_base.md3" );
	}

	if ( cgs.gametype == GT_1FCTF ) {
		cgs.media.neutralFlagModel = cgi.R_RegisterModel( "models/flags/n_flag.md3" );
		cgs.media.flagShader[0] = cgi.R_RegisterShaderNoMip( "icons/iconf_neutral1" );
		cgs.media.flagShader[1] = cgi.R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.flagShader[2] = cgi.R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.flagShader[3] = cgi.R_RegisterShaderNoMip( "icons/iconf_neutral3" );
	}

	cgs.media.dustPuffShader = cgi.R_RegisterShader("hasteSmokePuff" );

	if ( cgs.gametype >= GT_TEAM ) {
		cgs.media.friendShader = cgi.R_RegisterShader( "sprites/foe" );
		cgs.media.redQuadShader = cgi.R_RegisterShader("powerups/blueflag" );
		cgs.media.teamStatusBar = cgi.R_RegisterShader( "gfx/2d/colorbar.tga" );
	}

	cgs.media.armorModel = cgi.R_RegisterModel( "models/powerups/armor/armor_yel.md3" );
	cgs.media.armorIcon  = cgi.R_RegisterShaderNoMip( "icons/iconr_yellow" );

	cgs.media.gibAbdomen = cgi.R_RegisterModel( "models/gibs/abdomen.md3" );
	cgs.media.gibArm = cgi.R_RegisterModel( "models/gibs/arm.md3" );
	cgs.media.gibChest = cgi.R_RegisterModel( "models/gibs/chest.md3" );
	cgs.media.gibFist = cgi.R_RegisterModel( "models/gibs/fist.md3" );
	cgs.media.gibFoot = cgi.R_RegisterModel( "models/gibs/foot.md3" );
	cgs.media.gibForearm = cgi.R_RegisterModel( "models/gibs/forearm.md3" );
	cgs.media.gibIntestine = cgi.R_RegisterModel( "models/gibs/intestine.md3" );
	cgs.media.gibLeg = cgi.R_RegisterModel( "models/gibs/leg.md3" );
	cgs.media.gibSkull = cgi.R_RegisterModel( "models/gibs/skull.md3" );
	cgs.media.gibBrain = cgi.R_RegisterModel( "models/gibs/brain.md3" );

	cgs.media.smoke2 = cgi.R_RegisterModel( "models/weapons2/shells/s_shell.md3" );

	cgs.media.balloonShader = cgi.R_RegisterShader( "sprites/balloon3" );

	cgs.media.bloodExplosionShader = cgi.R_RegisterShader( "bloodExplosion" );

#ifdef QTZRELIC
	cgs.media.teleportEffectModel = cgi.R_RegisterModel( "models/misc/telep.md3" );
	cgs.media.teleportEffectShader = cgi.R_RegisterShader( "teleportEffect" );
#else
	cgs.media.teleportEffectModel = cgi.R_RegisterModel( "models/powerups/pop.md3" );
#endif // QTZRELIC

	cgs.media.guardPowerupModel = cgi.R_RegisterModel( "models/powerups/guard_player.md3" );
	cgs.media.medkitUsageModel = cgi.R_RegisterModel( "models/powerups/regen.md3" );
	cgs.media.heartShader = cgi.R_RegisterShaderNoMip( "ui/assets/statusbar/selectedhealth.tga" );

	cgs.media.medalImpressive = cgi.R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent = cgi.R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalGauntlet = cgi.R_RegisterShaderNoMip( "medal_gauntlet" );
	cgs.media.medalDefend = cgi.R_RegisterShaderNoMip( "medal_defend" );
	cgs.media.medalAssist = cgi.R_RegisterShaderNoMip( "medal_assist" );
	cgs.media.medalCapture = cgi.R_RegisterShaderNoMip( "medal_capture" );


	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i=1; i<bg_numItems; i++ ) {
		if ( items[i] == '1' ) {
			CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
		}
	}

	// wall marks
	cgs.media.bulletMarkShader = cgi.R_RegisterShader( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader = cgi.R_RegisterShader( "gfx/damage/burn_med_mrk" );
	cgs.media.holeMarkShader = cgi.R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.energyMarkShader = cgi.R_RegisterShader( "gfx/damage/plasma_mrk" );
	cgs.media.shadowMarkShader = cgi.R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader = cgi.R_RegisterShader( "wake" );
	cgs.media.bloodMarkShader = cgi.R_RegisterShader( "bloodMark" );

	// register the inline models
	cgs.numInlineModels = cgi.CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = cgi.R_RegisterModel( name );
		cgi.R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_MODELS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.gameModels[i] = cgi.R_RegisterModel( modelName );
	}

	// new stuff
	cgs.media.patrolShader = cgi.R_RegisterShaderNoMip("ui/assets/statusbar/patrol.tga");
	cgs.media.assaultShader = cgi.R_RegisterShaderNoMip("ui/assets/statusbar/assault.tga");
	cgs.media.campShader = cgi.R_RegisterShaderNoMip("ui/assets/statusbar/camp.tga");
	cgs.media.followShader = cgi.R_RegisterShaderNoMip("ui/assets/statusbar/follow.tga");
	cgs.media.defendShader = cgi.R_RegisterShaderNoMip("ui/assets/statusbar/defend.tga");
	cgs.media.teamLeaderShader = cgi.R_RegisterShaderNoMip("ui/assets/statusbar/team_leader.tga");
	cgs.media.retrieveShader = cgi.R_RegisterShaderNoMip("ui/assets/statusbar/retrieve.tga");
	cgs.media.escortShader = cgi.R_RegisterShaderNoMip("ui/assets/statusbar/escort.tga");
	cgs.media.cursor = cgi.R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor = cgi.R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	cgs.media.selectCursor = cgi.R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	cgs.media.flagShaders[0] = cgi.R_RegisterShaderNoMip("ui/assets/statusbar/flag_in_base.tga");
	cgs.media.flagShaders[1] = cgi.R_RegisterShaderNoMip("ui/assets/statusbar/flag_capture.tga");
	cgs.media.flagShaders[2] = cgi.R_RegisterShaderNoMip("ui/assets/statusbar/flag_missing.tga");

	cgi.R_RegisterModel( "models/players/"DEFAULT_MODEL"/lower.md3" );
	cgi.R_RegisterModel( "models/players/"DEFAULT_MODEL"/upper.md3" );
	//cgi.R_RegisterModel( "models/players/heads/"DEFAULT_MODEL"/"DEFAULT_MODEL".md3" );

	CG_ClearParticles ();
/*
	for (i=1; i<MAX_PARTICLES_AREAS; i++)
	{
		{
			int rval;

			rval = CG_NewParticleArea ( CS_PARTICLES + i);
			if (!rval)
				break;
		}
	}
*/

	cgs.media.brightModel				= cgi.R_RegisterShader( "brightModel" );

	//QtZ: New composite crosshair style
	for ( i=0; i<NUM_CROSSHAIRS; i++ )
		cgs.media.crosshair.images[i] = cgi.R_RegisterShaderNoMip( va("gfx/hud/crosshair_bit%i", (1<<i)) );
	cgs.media.crosshair.cooldownTic		= cgi.R_RegisterShader( "cooldownTic" );
}



/*																																			
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString(void) {
	int i;
	cg.spectatorList[0] = 0;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}
	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen) {
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
}


/*																																			
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients( void ) {
	int		i;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum);

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		if (cg.clientNum == i) {
			continue;
		}

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0]) {
			continue;
		}
		CG_LoadingClient( i );
		CG_NewClientInfo( i );
	}
	CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void ) {
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	cgi.S_StartBackgroundTrack( parm1, parm2 );
}

char *CG_GetMenuBuffer(const char *filename) {
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUFILE];

	len = cgi.FS_Open( filename, &f, FS_READ );
	if ( !f ) {
		cgi.Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return NULL;
	}
	if ( len >= MAX_MENUFILE ) {
		cgi.Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", filename, len, MAX_MENUFILE ) );
		cgi.FS_Close( f );
		return NULL;
	}

	cgi.FS_Read( buf, len, f );
	buf[len] = 0;
	cgi.FS_Close( f );

	return buf;
}

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
qboolean CG_Asset_Parse(int handle) {
	pc_token_t token;
	const char *tempStr;

	if (!cgi.PC_ReadTokenHandle(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}

	while ( 1 ) {
		if (!cgi.PC_ReadTokenHandle(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.textFont);
			continue;
		}

		// smallFont
		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.smallFont);
			continue;
		}

		// font
		if (Q_stricmp(token.string, "bigfont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.bigFont);
			continue;
		}

		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.gradientBar = cgi.R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuEnterSound = cgi.S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuExitSound = cgi.S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.itemFocusSound = cgi.S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuBuzzSound = cgi.S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) {
			if (!PC_String_Parse(handle, &cgDC.Assets.cursorStr)) {
				return qfalse;
			}
			cgDC.Assets.cursor = cgi.R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &cgDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &cgDC.Assets.shadowColor)) {
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			continue;
		}
	}
	return qfalse;
}

void CG_ParseMenu(const char *menuFile) {
	pc_token_t token;
	int handle;

	handle = cgi.PC_LoadSourceHandle(menuFile);
	if (!handle)
		handle = cgi.PC_LoadSourceHandle("ui/testhud.menu");
	if (!handle)
		return;

	while ( 1 ) {
		if (!cgi.PC_ReadTokenHandle( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (CG_Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}


		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle);
		}
	}
	cgi.PC_FreeSourceHandle(handle);
}

qboolean CG_Load_Menu(char **p) {
	char *token;

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		CG_ParseMenu(token); 
	}
	return qfalse;
}



void CG_LoadMenus(const char *menuFile) {
	char	*token;
	char *p;
	int	len, start;
	fileHandle_t	f;
	static char buf[MAX_MENUDEFFILE];

	start = cgi.Milliseconds();

	len = cgi.FS_Open( menuFile, &f, FS_READ );
	if ( !f ) {
		Com_Printf( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );
		len = cgi.FS_Open( "ui/hud.txt", &f, FS_READ );
		if (!f) {
			CG_Error( S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!" );
		}
	}

	if ( len >= MAX_MENUDEFFILE ) {
		cgi.FS_Close( f );
		CG_Error( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", menuFile, len, MAX_MENUDEFFILE );
		return;
	}

	cgi.FS_Read( buf, len, f );
	buf[len] = 0;
	cgi.FS_Close( f );

	COM_Compress(buf);

	Menu_Reset();

	p = buf;

	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}') {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( Q_stricmp( token, "}" ) == 0 ) {
			break;
		}

		if (Q_stricmp(token, "loadmenu") == 0) {
			if (CG_Load_Menu(&p)) {
				continue;
			} else {
				break;
			}
		}
	}

	Com_Printf("UI menu load time = %d milli seconds\n", cgi.Milliseconds() - start);

}



static qboolean CG_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
	return qfalse;
}


static int CG_FeederCount(float feederID) {
	int i, count;
	count = 0;
	if (feederID == FEEDER_REDTEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_RED) {
				count++;
			}
		}
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_BLUE) {
				count++;
			}
		}
	} else if (feederID == FEEDER_SCOREBOARD) {
		return cg.numScores;
	}
	return count;
}


void CG_SetScoreSelection(void *p) {
	menuDef_t *menu = (menuDef_t*)p;
	playerState_t *ps = &cg.snap->ps;
	int i, red, blue;
	red = blue = 0;
	for (i = 0; i < cg.numScores; i++) {
		if (cg.scores[i].team == TEAM_RED) {
			red++;
		} else if (cg.scores[i].team == TEAM_BLUE) {
			blue++;
		}
		if (ps->clientNum == cg.scores[i].client) {
			cg.selectedScore = i;
		}
	}

	if (menu == NULL) {
		// just interested in setting the selected score
		return;
	}

	if ( cgs.gametype >= GT_TEAM ) {
		int feeder = FEEDER_REDTEAM_LIST;
		i = red;
		if (cg.scores[cg.selectedScore].team == TEAM_BLUE) {
			feeder = FEEDER_BLUETEAM_LIST;
			i = blue;
		}
		Menu_SetFeederSelection(menu, feeder, i, NULL);
	} else {
		Menu_SetFeederSelection(menu, FEEDER_SCOREBOARD, cg.selectedScore, NULL);
	}
}

// FIXME: might need to cache this info
static clientInfo_t * CG_InfoFromScoreIndex(int index, int team, int *scoreIndex) {
	int i, count;
	if ( cgs.gametype >= GT_TEAM ) {
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (count == index) {
					*scoreIndex = i;
					return &cgs.clientinfo[cg.scores[i].client];
				}
				count++;
			}
		}
	}
	*scoreIndex = index;
	return &cgs.clientinfo[ cg.scores[index].client ];
}

static const char *CG_FeederItemText(float feederID, int index, int column, qhandle_t *handle) {
	gitem_t *item;
	int scoreIndex = 0;
	clientInfo_t *info = NULL;
	int team = -1;
	score_t *sp = NULL;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			if ( info->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
				item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
				*handle = cg_items[ ITEM_INDEX(item) ].icon;
			} else if ( info->powerups & ( 1 << PW_REDFLAG ) ) {
				item = BG_FindItemForPowerup( PW_REDFLAG );
				*handle = cg_items[ ITEM_INDEX(item) ].icon;
			} else if ( info->powerups & ( 1 << PW_BLUEFLAG ) ) {
				item = BG_FindItemForPowerup( PW_BLUEFLAG );
				*handle = cg_items[ ITEM_INDEX(item) ].icon;
			} else {
				if ( info->botSkill > 0 && info->botSkill <= 5 ) {
					*handle = cgs.media.botSkillShaders[ info->botSkill - 1 ];
				} else if ( info->handicap < 100 ) {
					return va("%i", info->handicap );
				}
			}
			break;
		case 1:
			if (team == -1) {
				return "";
			} else {
				*handle = CG_StatusHandle(info->teamTask);
			}
			break;
		case 2:
			if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
				return "Ready";
			}
			if (team == -1) {
				if (cgs.gametype == GT_TOURNAMENT) {
					return va("%i/%i", info->wins, info->losses);
				} else if (info->infoValid && info->team == TEAM_SPECTATOR ) {
					return "Spectator";
				} else {
					return "";
				}
			} else {
				if (info->teamLeader) {
					return "Leader";
				}
			}
			break;
		case 3:
			return info->name;
			break;
		case 4:
			return va("%i", info->score);
			break;
		case 5:
			return va("%4i", sp->time);
			break;
		case 6:
			if ( sp->ping == -1 ) {
				return "connecting";
			} 
			return va("%4i", sp->ping);
			break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage(float feederID, int index) {
	return 0;
}

static void CG_FeederSelection(float feederID, int index) {
	if ( cgs.gametype >= GT_TEAM ) {
		int i, count;
		int team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_RED : TEAM_BLUE;
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (index == count) {
					cg.selectedScore = i;
				}
				count++;
			}
		}
	} else {
		cg.selectedScore = index;
	}
}

static float CG_Cvar_Get(const char *cvar) {
	char buff[128];
	memset(buff, 0, sizeof(buff));
	cgi.Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return atof(buff);
}

void CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style) {
	CG_Text_Paint(x, y, scale, color, text, 0, limit, style);
}

static int CG_OwnerDrawWidth(int ownerDraw, float scale) {
	switch (ownerDraw) {
	case CG_GAME_TYPE:
		return CG_Text_Width(gametypeNames[cgs.gametype], scale, 0);
	case CG_GAME_STATUS:
		return CG_Text_Width(CG_GetGameStatusText(), scale, 0);
		break;
	case CG_KILLER:
		return CG_Text_Width(CG_GetKillerText(), scale, 0);
		break;
	//QTZTODO: Team names
		/*
	case CG_RED_NAME:
		return CG_Text_Width(cg_redTeamName.string, scale, 0);
		break;
	case CG_BLUE_NAME:
		return CG_Text_Width(cg_blueTeamName.string, scale, 0);
		break;
		*/


	}
	return 0;
}

static int CG_PlayCinematic(const char *name, float x, float y, float w, float h) {
	return cgi.CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

static void CG_StopCinematic(int handle) {
	cgi.CIN_StopCinematic(handle);
}

static void CG_DrawCinematic(int handle, float x, float y, float w, float h) {
	cgi.CIN_SetExtents(handle, x, y, w, h);
	cgi.CIN_DrawCinematic(handle);
}

static void CG_RunCinematicFrame(int handle) {
	cgi.CIN_RunCinematic(handle);
}

/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu( void ) {
	char buff[1024];
	const char *hudSet;

	cgDC.registerShaderNoMip = cgi.R_RegisterShaderNoMip;
	cgDC.setColor = cgi.R_SetColor;
	cgDC.drawHandlePic = &CG_DrawPic;
	cgDC.drawStretchPic = cgi.R_DrawStretchPic;
	cgDC.drawText = &CG_Text_Paint;
	cgDC.textWidth = &CG_Text_Width;
	cgDC.textHeight = &CG_Text_Height;
	cgDC.registerModel = cgi.R_RegisterModel;
	cgDC.modelBounds = cgi.R_ModelBounds;
	cgDC.fillRect = &CG_FillRect;
	cgDC.drawRect = &CG_DrawRect;   
	cgDC.drawSides = &CG_DrawSides;
	cgDC.drawTopBottom = &CG_DrawTopBottom;
	cgDC.clearScene = cgi.R_ClearScene;
	cgDC.addRefEntityToScene = cgi.R_AddRefEntityToScene;
	cgDC.renderScene = cgi.R_RenderScene;
	cgDC.registerFont = cgi.R_RegisterFont;
	cgDC.ownerDrawItem = &CG_OwnerDraw;
	cgDC.getValue = &CG_GetValue;
	cgDC.ownerDrawVisible = &CG_OwnerDrawVisible;
	cgDC.runScript = &CG_RunMenuScript;
	cgDC.deferScript = &CG_DeferMenuScript;
	cgDC.getTeamColor = &CG_GetTeamColor;
	cgDC.setCVar = cgi.Cvar_Set;
	cgDC.getCVarString = cgi.Cvar_VariableStringBuffer;
	cgDC.getCVarValue = CG_Cvar_Get;
	cgDC.drawTextWithCursor = &CG_Text_PaintWithCursor;
	//cgDC.setOverstrikeMode = &cgi.Key_SetOverstrikeMode;
	//cgDC.getOverstrikeMode = &cgi.Key_GetOverstrikeMode;
	cgDC.startLocalSound = cgi.S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount = &CG_FeederCount;
	cgDC.feederItemImage = &CG_FeederItemImage;
	cgDC.feederItemText = &CG_FeederItemText;
	cgDC.feederSelection = &CG_FeederSelection;
	//cgDC.setBinding = &cgi.Key_SetBinding;
	//cgDC.getBindingBuf = &cgi.Key_GetBindingBuf;
	//cgDC.keynumToStringBuf = &cgi.Key_KeynumToStringBuf;
	//cgDC.executeText = &cgi.Cmd_ExecuteText;
	cgDC.Error = &Com_Error; 
	cgDC.Print = &Com_Printf; 
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	//cgDC.Pause = &CG_Pause;
	cgDC.registerSound = cgi.S_RegisterSound;
	cgDC.startBackgroundTrack = cgi.S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack = cgi.S_StopBackgroundTrack;
	cgDC.playCinematic = &CG_PlayCinematic;
	cgDC.stopCinematic = &CG_StopCinematic;
	cgDC.drawCinematic = &CG_DrawCinematic;
	cgDC.runCinematicFrame = &CG_RunCinematicFrame;
	
	Init_Display(&cgDC);

	Menu_Reset();
	
	cgi.Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
	hudSet = buff;
	if (hudSet[0] == '\0') {
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus(hudSet);
}

void CG_AssetCache( void ) {
	//if (Assets.textFont == NULL) {
	//  cgi.R_RegisterFont("fonts/arial.ttf", 72, &Assets.textFont);
	//}
	//Assets.background = cgi.R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	cgDC.Assets.gradientBar = cgi.R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	cgDC.Assets.fxBasePic = cgi.R_RegisterShaderNoMip( ART_FX_BASE );
	cgDC.Assets.fxPic[0] = cgi.R_RegisterShaderNoMip( ART_FX_RED );
	cgDC.Assets.fxPic[1] = cgi.R_RegisterShaderNoMip( ART_FX_YELLOW );
	cgDC.Assets.fxPic[2] = cgi.R_RegisterShaderNoMip( ART_FX_GREEN );
	cgDC.Assets.fxPic[3] = cgi.R_RegisterShaderNoMip( ART_FX_TEAL );
	cgDC.Assets.fxPic[4] = cgi.R_RegisterShaderNoMip( ART_FX_BLUE );
	cgDC.Assets.fxPic[5] = cgi.R_RegisterShaderNoMip( ART_FX_CYAN );
	cgDC.Assets.fxPic[6] = cgi.R_RegisterShaderNoMip( ART_FX_WHITE );
	cgDC.Assets.scrollBar = cgi.R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	cgDC.Assets.scrollBarArrowDown = cgi.R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	cgDC.Assets.scrollBarArrowUp = cgi.R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	cgDC.Assets.scrollBarArrowLeft = cgi.R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	cgDC.Assets.scrollBarArrowRight = cgi.R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	cgDC.Assets.scrollBarThumb = cgi.R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	cgDC.Assets.sliderBar = cgi.R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	cgDC.Assets.sliderThumb = cgi.R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
}

/*
=================
CG_Init

Called when the level loads or when the renderer is restarted.
All media should be registered at this time
CGame will display loading status by calling SCR_Update, which will call CG_DrawInformation during the loading process
reliableCommandSequence will be 0 on fresh loads, but higher for demos, tourney restarts, or vid_restarts
=================
*/
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum ) {
	const char	*s;

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof(cg_entities) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );
	memset( cg_items, 0, sizeof(cg_items) );

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader		= cgi.R_RegisterShader( "gfx/2d/bigchars" );
	cgs.media.whiteShader		= cgi.R_RegisterShader( "white" );
	cgs.media.charsetProp		= cgi.R_RegisterShaderNoMip( "menu/art/font1_prop.tga" );
	cgs.media.charsetPropGlow	= cgi.R_RegisterShaderNoMip( "menu/art/font1_prop_glo.tga" );
	cgs.media.charsetPropB		= cgi.R_RegisterShaderNoMip( "menu/art/font2_prop.tga" );

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	cg.weaponSelect = WP_QUANTIZER;

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	// get the rendering configuration from the client system
	cgi.GetGLConfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / SCREEN_WIDTH;
	cgs.screenYScale = cgs.glconfig.vidHeight / SCREEN_HEIGHT;

	// get the gamestate from the client system
	cgi.GetGameState( &cgs.gameState );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if ( strcmp( s, GAME_VERSION ) ) {
		CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
	}

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_ParseServerinfo();

	// load the new map
	CG_LoadingString( "collision map" );

	cgi.CM_LoadMap( cgs.mapname );

	String_Init();

	cg.loading = qtrue;		// force players to load instead of defer

	CG_LoadingString( "sounds" );

	CG_RegisterSounds();

	CG_LoadingString( "graphics" );

	CG_RegisterGraphics();

	CG_LoadingString( "clients" );

	CG_RegisterClients();		// if low on memory, some clients will be deferred

	CG_AssetCache();
	CG_LoadHudMenu();      // load new hud stuff

	cg.loading = qfalse;	// future players will be deferred

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString( "" );

	CG_InitTeamChat();

	CG_ShaderStateChanged();

	cgi.S_ClearLoopingSounds( qtrue );
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}


/*
==================
CG_EventHandling
==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/

/*
============
GetCGameAPI
============
*/

cgameImport_t cgi;

Q_EXPORT cgameExport_t* QDECL GetCGameAPI( int apiVersion, cgameImport_t *import )
{
	static cgameExport_t cge = {0};
	
	assert( import );
	cgi = *import;

	memset( &cge, 0, sizeof( cge ) );

	if ( apiVersion != CGAME_API_VERSION ) {
		cgi.Print( "Mismatched CGAME_API_VERSION: expected %i, got %i\n", CGAME_API_VERSION, apiVersion );
		return NULL;
	}

	cge.Init			= CG_Init;
	cge.Shutdown		= CG_Shutdown;
	cge.ConsoleCommand	= CG_ConsoleCommand;
	cge.DrawActiveFrame	= CG_DrawActiveFrame;
	cge.CrosshairPlayer	= CG_CrosshairPlayer;
	cge.LastAttacker	= CG_LastAttacker;
	cge.KeyEvent		= CG_KeyEvent;
	cge.MouseEvent		= CG_MouseEvent;
	cge.EventHandling	= CG_EventHandling;

	return &cge;
}
