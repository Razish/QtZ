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
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"

char	*cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"*death1.wav",
	"*death2.wav",
	"*death3.wav",
	"*jump1.wav",
	"*pain25_1.wav",
	"*pain50_1.wav",
	"*pain75_1.wav",
	"*pain100_1.wav",
	"*falling1.wav",
	"*gasp.wav",
	"*drown.wav",
	"*fall1.wav",
	"*taunt.wav"
};

sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName ) {
	clientInfo_t *ci;
	int			i;

	if ( soundName[0] != '*' ) {
		return trap->S_RegisterSound( soundName, qfalse );
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i] ; i++ ) {
		if ( !strcmp( soundName, cg_customSoundNames[i] ) ) {
			return ci->sounds[i];
		}
	}

	trap->Error( ERR_DROP, "Unknown custom sound: %s", soundName );
	return 0;
}



/*

	CLIENT INFO

*/

// Read a configuration file containing animation counts and rates
//	models/players/visor/animation.cfg, etc
static qboolean	CG_ParseAnimationFile( const char *filename, clientInfo_t *ci ) {
	char		*text_p, *prev;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			skip;
	char		text[20000];
	fileHandle_t	f;
	animation_t *animations;

	animations = ci->animations;

	// load the file
	len = trap->FS_Open( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		trap->Print( "File %s too long\n", filename );
		trap->FS_Close( f );
		return qfalse;
	}
	trap->FS_Read( text, len, f );
	text[len] = 0;
	trap->FS_Close( f );

	// parse the text
	text_p = text;
	skip = 0;

	ci->footsteps = FOOTSTEP_NORMAL;
	VectorClear( &ci->headOffset );
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;

	// read optional parameters
	while ( 1 ) {
		prev = text_p;	// so we can unget
		token = COM_Parse( &text_p );
		if ( !token ) {
			break;
		}
		if ( !Q_stricmp( token, "footsteps" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}
			if ( !Q_stricmp( token, "default" ) || !Q_stricmp( token, "normal" ) ) {
				ci->footsteps = FOOTSTEP_NORMAL;
			} else if ( !Q_stricmp( token, "boot" ) ) {
				ci->footsteps = FOOTSTEP_BOOT;
			} else if ( !Q_stricmp( token, "flesh" ) ) {
				ci->footsteps = FOOTSTEP_FLESH;
			} else if ( !Q_stricmp( token, "mech" ) ) {
				ci->footsteps = FOOTSTEP_MECH;
			} else if ( !Q_stricmp( token, "energy" ) ) {
				ci->footsteps = FOOTSTEP_ENERGY;
			} else {
				trap->Print( "Bad footsteps parm in %s: %s\n", filename, token );
			}
			continue;
		} else if ( !Q_stricmp( token, "headoffset" ) ) {
			for ( i = 0 ; i < 3 ; i++ ) {
				token = COM_Parse( &text_p );
				if ( !token ) {
					break;
				}
				ci->headOffset.data[i] = (float)atof( token );
			}
			continue;
		} else if ( !Q_stricmp( token, "fixedlegs" ) ) {
			ci->fixedlegs = qtrue;
			continue;
		} else if ( !Q_stricmp( token, "fixedtorso" ) ) {
			ci->fixedtorso = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if ( token[0] >= '0' && token[0] <= '9' ) {
			text_p = prev;	// unget the token
			break;
		}
		Com_Printf( "unknown token '%s' in %s\n", token, filename );
	}

	// read information for each frame
	for ( i = 0 ; i < MAX_ANIMATIONS ; i++ ) {

		token = COM_Parse( &text_p );
		if ( !*token ) {
			if( i >= TORSO_GETFLAG && i <= TORSO_NEGATIVE ) {
				animations[i].firstFrame = animations[TORSO_GESTURE].firstFrame;
				animations[i].frameLerp = animations[TORSO_GESTURE].frameLerp;
				animations[i].initialLerp = animations[TORSO_GESTURE].initialLerp;
				animations[i].loopFrames = animations[TORSO_GESTURE].loopFrames;
				animations[i].numFrames = animations[TORSO_GESTURE].numFrames;
				animations[i].reversed = qfalse;
				animations[i].flipflop = qfalse;
				continue;
			}
			break;
		}
		animations[i].firstFrame = atoi( token );
		// leg only frames are adjusted to not count the upper body only frames
		if ( i == LEGS_WALKCR ) {
			skip = animations[LEGS_WALKCR].firstFrame - animations[TORSO_GESTURE].firstFrame;
		}
		if ( i >= LEGS_WALKCR && i<TORSO_GETFLAG) {
			animations[i].firstFrame -= skip;
		}

		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		animations[i].numFrames = atoi( token );

		animations[i].reversed = qfalse;
		animations[i].flipflop = qfalse;
		// if numFrames is negative the animation is reversed
		if (animations[i].numFrames < 0) {
			animations[i].numFrames = -animations[i].numFrames;
			animations[i].reversed = qtrue;
		}

		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		animations[i].loopFrames = atoi( token );

		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		fps = (float)atof( token );
		if ( fps == 0 ) {
			fps = 1;
		}
		animations[i].frameLerp = (int)(1000 / fps);
		animations[i].initialLerp = (int)(1000 / fps);
	}

	if ( i != MAX_ANIMATIONS ) {
		trap->Print( "Error parsing animation file: %s\n", filename );
		return qfalse;
	}

	// crouch backward animation
	memcpy(&animations[LEGS_BACKCR], &animations[LEGS_WALKCR], sizeof(animation_t));
	animations[LEGS_BACKCR].reversed = qtrue;
	// walk backward animation
	memcpy(&animations[LEGS_BACKWALK], &animations[LEGS_WALK], sizeof(animation_t));
	animations[LEGS_BACKWALK].reversed = qtrue;
	// flag moving fast
	animations[FLAG_RUN].firstFrame = 0;
	animations[FLAG_RUN].numFrames = 16;
	animations[FLAG_RUN].loopFrames = 16;
	animations[FLAG_RUN].frameLerp = 1000 / 15;
	animations[FLAG_RUN].initialLerp = 1000 / 15;
	animations[FLAG_RUN].reversed = qfalse;
	// flag not moving or moving slowly
	animations[FLAG_STAND].firstFrame = 16;
	animations[FLAG_STAND].numFrames = 5;
	animations[FLAG_STAND].loopFrames = 0;
	animations[FLAG_STAND].frameLerp = 1000 / 20;
	animations[FLAG_STAND].initialLerp = 1000 / 20;
	animations[FLAG_STAND].reversed = qfalse;
	// flag speeding up
	animations[FLAG_STAND2RUN].firstFrame = 16;
	animations[FLAG_STAND2RUN].numFrames = 5;
	animations[FLAG_STAND2RUN].loopFrames = 1;
	animations[FLAG_STAND2RUN].frameLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].initialLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].reversed = qtrue;
	//
	// new anims changes
	//
//	animations[TORSO_GETFLAG].flipflop = qtrue;
//	animations[TORSO_GUARDBASE].flipflop = qtrue;
//	animations[TORSO_PATROL].flipflop = qtrue;
//	animations[TORSO_AFFIRMATIVE].flipflop = qtrue;
//	animations[TORSO_NEGATIVE].flipflop = qtrue;
	//
	return qtrue;
}

static qboolean	CG_FileExists(const char *filename) {
	int len;

	len = trap->FS_Open( filename, NULL, FS_READ );
	if (len>0) {
		return qtrue;
	}
	return qfalse;
}

static qboolean	CG_FindClientModelFile( char *filename, int length, const char *modelName, const char *base, const char *ext ) {
	//"models/players/mymodel/lower.skin"
	Com_sprintf( filename, length, "models/players/%s/%s.%s", modelName, base, ext );

	if ( CG_FileExists( filename ) )
		return qtrue;

	return qfalse;
}

static qboolean	CG_RegisterClientSkin( clientInfo_t *ci, const char *modelName ) {
	char filename[MAX_QPATH]={0};

	//lower
	if ( CG_FindClientModelFile( filename, sizeof( filename ), modelName, "lower", "skin" ) )
		ci->legsSkin = trap->R_RegisterSkin( filename );
	if ( !ci->legsSkin )
		Com_Printf( "Leg skin load failure: %s\n", filename );

	//torso
	if ( CG_FindClientModelFile( filename, sizeof( filename ), modelName, "upper", "skin" ) )
		ci->torsoSkin = trap->R_RegisterSkin( filename );
	if ( !ci->torsoSkin )
		Com_Printf( "Torso skin load failure: %s\n", filename );

	//head
	if ( CG_FindClientModelFile( filename, sizeof( filename ), modelName, "head", "skin" ) )
		ci->headSkin = trap->R_RegisterSkin( filename );
	if ( !ci->headSkin )
		Com_Printf( "Head skin load failure: %s\n", filename );

	// if any skins failed to load
	if ( !ci->legsSkin || !ci->torsoSkin || !ci->headSkin )
		return qfalse;

	return qtrue;
}

static qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName ) {
	char filename[MAX_QPATH]={0};

	//legs
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", modelName );
	ci->legsModel = trap->R_RegisterModel( filename );
	if ( !ci->legsModel )
	{
		Com_Printf( "Failed to load model file %s\n", filename );
		return qfalse;
	}

	//torso
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", modelName );
	ci->torsoModel = trap->R_RegisterModel( filename );
	if ( !ci->torsoModel )
	{
		Com_Printf( "Failed to load model file %s\n", filename );
		return qfalse;
	}

	//head
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/head.md3", modelName );
	ci->headModel = trap->R_RegisterModel( filename );
	if ( !ci->headModel )
	{
		Com_Printf( "Failed to load model file %s\n", filename );
		return qfalse;
	}

	// if any skins failed to load, return failure
	if ( !CG_RegisterClientSkin( ci, modelName ) )
	{
		Com_Printf( "Failed to load skin file: %s\n", modelName );
		return qfalse;
	}

	// load the animations
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation.cfg", modelName );
	if ( !CG_ParseAnimationFile( filename, ci ) ) {
		Com_Printf( "Failed to load animation file %s\n", filename );
		return qfalse;
	}

	if ( CG_FindClientModelFile( filename, sizeof(filename), modelName, "icon", "tga" ) )
		ci->modelIcon = trap->R_RegisterShaderNoMip( filename );

	if ( !ci->modelIcon )
		return qfalse;

	return qtrue;
}

static void CG_ColorFromString( int c, vector3 *color ) {
	VectorCopy( (vector3*)&g_color_table[Q_clampi( 0, c, Q_COLOR_BITS )], color );
}

// Load it now, taking the disk hits.
//	This will usually be deferred to a safe time
static void CG_LoadClientInfo( int clientNum, clientInfo_t *ci ) {
	const char	*dir, *fallback;
	int			i, modelloaded;
	const char	*s;
	modelloaded = qtrue;

	if ( !CG_RegisterClientModelname( ci, ci->modelName ) )
	{
		if ( !CG_RegisterClientModelname( ci, DEFAULT_MODEL ) )
			trap->Error( ERR_DROP, "DEFAULT_MODEL ("DEFAULT_MODEL") failed to register" );

		modelloaded = qfalse;
	}

	ci->newAnims = qfalse;
	if ( ci->torsoModel ) {
		orientation_t tag;
		// if the torso model has the "tag_flag"
		if ( trap->R_LerpTag( &tag, ci->torsoModel, 0, 0, 1, "tag_flag" ) ) {
			ci->newAnims = qtrue;
		}
	}

	// sounds
	dir = ci->modelName;
	fallback = DEFAULT_MODEL;

	for ( i=0; i<MAX_CUSTOM_SOUNDS; i++ )
	{
		s = cg_customSoundNames[i];
		if ( !s )
			break;

		ci->sounds[i] = 0;
		// if the model didn't load use the sounds of the default model
		if (modelloaded)
			ci->sounds[i] = trap->S_RegisterSound( va("sound/player/%s/%s", dir, s + 1), qfalse );
		if ( !ci->sounds[i] )
			ci->sounds[i] = trap->S_RegisterSound( va("sound/player/%s/%s", fallback, s + 1), qfalse );
	}

	ci->deferred = qfalse;

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	for ( i=0; i<MAX_GENTITIES; i++ )
	{
		if ( cg_entities[i].currentState.clientNum == clientNum && cg_entities[i].currentState.eType == ET_PLAYER )
			CG_ResetPlayerEntity( &cg_entities[i] );
	}
}

static void CG_CopyClientInfoModel( clientInfo_t *from, clientInfo_t *to ) {
	VectorCopy( &from->headOffset, &to->headOffset );
	to->footsteps = from->footsteps;

	to->legsModel = from->legsModel;
	to->legsSkin = from->legsSkin;
	to->torsoModel = from->torsoModel;
	to->torsoSkin = from->torsoSkin;
	to->headModel = from->headModel;
	to->headSkin = from->headSkin;
	to->modelIcon = from->modelIcon;

	to->newAnims = from->newAnims;

	memcpy( to->animations, from->animations, sizeof( to->animations ) );
	memcpy( to->sounds, from->sounds, sizeof( to->sounds ) );
}

static qboolean CG_ScanForExistingClientInfo( clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}
		if ( match->deferred ) {
			continue;
		}
		if ( !Q_stricmp( ci->modelName, match->modelName ) &&
			 (cgs.gametype < GT_TEAMBLOOD || ci->team == match->team) )
		{// this clientinfo is identical, so use its handles
			ci->deferred = qfalse;
			CG_CopyClientInfoModel( match, ci );

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

// We aren't going to load it now, so grab some other client's info to use until we have some spare time.
static void CG_SetDeferredClientInfo( int clientNum, clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	// if someone else is already the same models and skins we
	// can just load the client info
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid || match->deferred ) {
			continue;
		}
		if ( Q_stricmp( ci->modelName, match->modelName ) ||
//			 Q_stricmp( ci->headModelName, match->headModelName ) ||
			 (cgs.gametype >= GT_TEAMBLOOD && ci->team != match->team) ) {
			continue;
		}
		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo( clientNum, ci );
		return;
	}

	// if we are in teamplay, only grab a model if the skin is correct
	if ( cgs.gametype >= GT_TEAMBLOOD ) {
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			match = &cgs.clientinfo[ i ];
			if ( !match->infoValid || match->deferred ) {
				continue;
			}
			ci->deferred = qtrue;
			CG_CopyClientInfoModel( match, ci );
			return;
		}
		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo( clientNum, ci );
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}

		ci->deferred = qtrue;
		CG_CopyClientInfoModel( match, ci );
		return;
	}

	// we should never get here...
	trap->Print( "CG_SetDeferredClientInfo: no valid clients!\n" );

	CG_LoadClientInfo( clientNum, ci );
}

void CG_NewClientInfo( int clientNum ) {
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char	*configstring;
	const char	*v;

	ci = &cgs.clientinfo[clientNum];

	configstring = CG_ConfigString( clientNum + CS_PLAYERS );
	if ( !configstring[0] ) {
		memset( ci, 0, sizeof( *ci ) );
		return;		// player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz( newInfo.name, v, sizeof( newInfo.name ) );

	// team
	v = Info_ValueForKey( configstring, "t" );
	newInfo.team = atoi( v );

	// model
	v = Info_ValueForKey( configstring, "m" );
	if ( newInfo.team != cg.predictedPlayerState.persistent[PERS_TEAM] && Q_stricmp( cg_forceEnemyModel->string, "none" ) )
		Q_strncpyz( newInfo.modelName, cg_forceEnemyModel->string, sizeof( newInfo.modelName ) );
	else if ( newInfo.team == cg.predictedPlayerState.persistent[PERS_TEAM] && Q_stricmp( cg_forceAllyModel->string, "none" ) )
		Q_strncpyz( newInfo.modelName, cg_forceAllyModel->string, sizeof( newInfo.modelName ) );
	else
		Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );

	// colors
	v = Info_ValueForKey( configstring, "c" );
	CG_ColorFromString( ColorIndex(v[0]), &newInfo.color1 );

	newInfo.c1RGBA[0] = (byte)(255 * newInfo.color1.r);
	newInfo.c1RGBA[1] = (byte)(255 * newInfo.color1.g);
	newInfo.c1RGBA[2] = (byte)(255 * newInfo.color1.b);
	newInfo.c1RGBA[3] = 255;

	CG_ColorFromString( ColorIndex(v[1]), &newInfo.color2 );
	newInfo.c2RGBA[0] = (byte)(255 * newInfo.color2.r);
	newInfo.c2RGBA[1] = (byte)(255 * newInfo.color2.g);
	newInfo.c2RGBA[2] = (byte)(255 * newInfo.color2.b);
	newInfo.c2RGBA[3] = 255;

	// bot skill
	v = Info_ValueForKey( configstring, "s" );
	if ( !v[0] )
		newInfo.botSkill = -1; // client
	else
		newInfo.botSkill = atoi( v );

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if ( !CG_ScanForExistingClientInfo( &newInfo ) )
	{
		qboolean forceDefer = (qboolean)!!(trap->MemoryRemaining() < 4000000);

		// if we are defering loads, just have it pick the first valid
		if ( forceDefer || (cg_deferPlayers->integer && !cg.loading ) )
		{// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo( clientNum, &newInfo );
			
			if ( forceDefer )
			{// if we are low on memory, leave them with this model
				trap->Print( "Memory is low. Using deferred model.\n" );
				newInfo.deferred = qfalse;
			}
		}
		else
			CG_LoadClientInfo( clientNum, &newInfo );
	}

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci = newInfo;
}

// Called each frame when a player is dead and the scoreboard is up so deferred players can be loaded
void CG_LoadDeferredPlayers( void ) {
	int		i;
	clientInfo_t	*ci;

	// scan for a deferred player to load
	for ( i = 0, ci = cgs.clientinfo ; i < cgs.maxclients ; i++, ci++ ) {
		if ( ci->infoValid && ci->deferred ) {
			// if we are low on memory, leave it deferred
			if ( trap->MemoryRemaining() < 0x400000 ) { // < 4 Mb
				trap->Print( "Memory is low. Using deferred model.\n" );
				ci->deferred = qfalse;
				continue;
			}
			CG_LoadClientInfo( i, ci );
//			break;
		}
	}
}

/*

	PLAYER ANIMATION

*/


// may include ANIM_TOGGLEBIT
static void CG_SetLerpFrameAnimation( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	animation_t	*anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS ) {
		trap->Error( ERR_DROP, "Bad animation number: %i", newAnimation );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;
}

// Sets cg.snap, cg.oldFrame, and cg.backlerp
//	cg.time should be between oldFrameTime and frameTime after exit
static void CG_RunLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale ) {
	int			f, numFrames;
	animation_t	*anim;

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation ) {
		CG_SetLerpFrameAnimation( ci, lf, newAnimation );
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim->frameLerp ) {
			return;		// shouldn't happen
		}
		if ( cg.time < lf->animationTime ) {
			lf->frameTime = lf->animationTime;		// initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		f = (int)(f * speedScale);		// adjust for haste, etc

		numFrames = anim->numFrames;
		if (anim->flipflop) {
			numFrames *= 2;
		}
		if ( f >= numFrames ) {
			f -= numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		if ( anim->reversed ) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		}
		else if (anim->flipflop && f>=anim->numFrames) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - (f%anim->numFrames);
		}
		else {
			lf->frame = anim->firstFrame + f;
		}
		if ( cg.time > lf->frameTime ) {
			lf->frameTime = cg.time;
		}
	}

	if ( lf->frameTime > cg.time + 200 ) {
		lf->frameTime = cg.time;
	}

	if ( lf->oldFrameTime > cg.time ) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0f - (float)( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}

static void CG_ClearLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int animationNumber ) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation( ci, lf, animationNumber );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}

static void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
						int *torsoOld, int *torso, float *torsoBackLerp ) {
	clientInfo_t	*ci;
	int				clientNum;
	float			speedScale;

	clientNum = cent->currentState.clientNum;

	speedScale = 1;

	ci = &cgs.clientinfo[ clientNum ];

	// do the shuffle turn frames locally
	if ( cent->pe.legs.yawing && ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLE ) {
		CG_RunLerpFrame( ci, &cent->pe.legs, LEGS_TURN, speedScale );
	} else {
		CG_RunLerpFrame( ci, &cent->pe.legs, cent->currentState.legsAnim, speedScale );
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	CG_RunLerpFrame( ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale );

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}

/*

	PLAYER ANGLES

*/

static void CG_SwingAngles( float destination, float swingTolerance, float clampTolerance,
					float speed, float *angle, qboolean *swinging ) {
	float	swing;
	float	move;
	float	scale;

	if ( !*swinging ) {
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );
		if ( swing > swingTolerance || swing < -swingTolerance ) {
			*swinging = qtrue;
		}
	}

	if ( !*swinging ) {
		return;
	}
	
	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract( destination, *angle );
	scale = fabsf( swing );
	if ( scale < swingTolerance * 0.5f ) {
		scale = 0.5f;
	} else if ( scale < swingTolerance ) {
		scale = 1.0f;
	} else {
		scale = 2.0f;
	}

	// swing towards the destination angle
	if ( swing >= 0 ) {
		move = cg.frametime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	} else if ( swing < 0 ) {
		move = cg.frametime * scale * -speed;
		if ( move <= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );
	if ( swing > clampTolerance ) {
		*angle = AngleMod( destination - (clampTolerance - 1) );
	} else if ( swing < -clampTolerance ) {
		*angle = AngleMod( destination + (clampTolerance - 1) );
	}
}

static void CG_AddPainTwitch( centity_t *cent, vector3 *torsoAngles ) {
	int		t;
	float	f;

	t = cg.time - cent->pe.painTime;
	if ( t >= PAIN_TWITCH_TIME ) {
		return;
	}

	f = 1.0f - (float)t / PAIN_TWITCH_TIME;

	if ( cent->pe.painDirection )
		torsoAngles->roll += 20 * f;
	else
		torsoAngles->roll -= 20 * f;
}


// Handles seperate torso motion
//	legs pivot based on direction of movement
//	head always looks exactly at cent->lerpAngles
//	if motion < 20 degrees, show in head only
//	if < 45 degrees, also show in torso
static void CG_PlayerAngles( centity_t *cent, matrix3 legs, matrix3 torso, matrix3 head ) {
	vector3		legsAngles, torsoAngles, headAngles;
	float		dest;
	static	int	movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 };
	vector3		velocity;
	float		speed;
	int			dir, clientNum;
	clientInfo_t	*ci;

	VectorCopy( &cent->lerpAngles, &headAngles );
	headAngles.yaw = AngleMod( headAngles.yaw );
	VectorClear( &legsAngles );
	VectorClear( &torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != LEGS_IDLE 
		|| ((cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND 
		&& (cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND2)) {
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue;	// always center
		cent->pe.torso.pitching = qtrue;	// always center
		cent->pe.legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	if ( cent->currentState.eFlags & EF_DEAD ) {
		// don't let dead bodies twitch
		dir = 0;
	} else {
		dir = (int)cent->currentState.angles2.yaw;
		if ( dir < 0 || dir > 7 ) {
			trap->Error( ERR_DROP, "Bad player movement angle" );
		}
	}
	legsAngles.yaw = headAngles.yaw + movementOffsets[ dir ];
	torsoAngles.yaw = headAngles.yaw + 0.25f * movementOffsets[ dir ];

	// torso
	CG_SwingAngles( torsoAngles.yaw, 25, 90, cg_swingSpeed->value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
	CG_SwingAngles( legsAngles.yaw, 40, 90, cg_swingSpeed->value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );

	torsoAngles.yaw = cent->pe.torso.yawAngle;
	legsAngles.yaw = cent->pe.legs.yawAngle;


	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if ( headAngles.pitch > 180 ) {
		dest = (-360 + headAngles.pitch) * 0.75f;
	} else {
		dest = headAngles.pitch * 0.75f;
	}
	CG_SwingAngles( dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
	torsoAngles.pitch = cent->pe.torso.pitchAngle;

	//
	clientNum = cent->currentState.clientNum;
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		ci = &cgs.clientinfo[ clientNum ];
		if ( ci->fixedtorso ) {
			torsoAngles.pitch = 0.0f;
		}
	}

	// --------- roll -------------


	// lean towards the direction of travel
	VectorCopy( &cent->currentState.pos.trDelta, &velocity );
	speed = VectorNormalize( &velocity );
	if ( speed ) {
		matrix3	axis;
		float	side;

		speed *= 0.05f;

		AnglesToAxis( &legsAngles, axis );
		side = speed * DotProduct( &velocity, &axis[1] );
		legsAngles.roll -= side;

		side = speed * DotProduct( &velocity, &axis[0] );
		legsAngles.pitch += side;
	}

	//
	clientNum = cent->currentState.clientNum;
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		ci = &cgs.clientinfo[ clientNum ];
		if ( ci->fixedlegs ) {
			legsAngles.yaw = torsoAngles.yaw;
			legsAngles.pitch = 0.0f;
			legsAngles.roll = 0.0f;
		}
	}

	// pain twitch
	CG_AddPainTwitch( cent, &torsoAngles );

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( &headAngles, &torsoAngles, &headAngles );
	AnglesSubtract( &torsoAngles, &legsAngles, &torsoAngles );
	AnglesToAxis( &legsAngles, legs );
	AnglesToAxis( &torsoAngles, torso );
	AnglesToAxis( &headAngles, head );
}

static void CG_BreathPuffs( centity_t *cent, refEntity_t *head) {
	clientInfo_t *ci;
	vector3 up, origin;
	int contents;

	ci = &cgs.clientinfo[ cent->currentState.number ];

	if ( cent->currentState.number == cg.snap->ps.clientNum ) {
		return;
	}
	if ( cent->currentState.eFlags & EF_DEAD ) {
		return;
	}
	contents = CG_PointContents( &head->origin, 0 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}
	if ( ci->breathPuffTime > cg.time ) {
		return;
	}

	VectorSet( &up, 0, 0, 8 );
	VectorMA(&head->origin, 8, &head->axis[0], &origin);
	VectorMA(&origin, -4, &head->axis[2], &origin);
	CG_SmokePuff( &origin, &up, 16, 1, 1, 1, 0.66f, 1500, cg.time, cg.time + 400, LEF_PUFF_DONT_SCALE, 0/*cgs.media.shotgunSmokePuffShader*/ );
	ci->breathPuffTime = cg.time + 2000;
}

static void CG_DustTrail( centity_t *cent ) {
	int				anim;
	vector3 end, vel;
	trace_t tr;

	if ( cent->dustTrailTime > cg.time ) {
		return;
	}

	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
	if ( anim != LEGS_LANDB && anim != LEGS_LAND ) {
		return;
	}

	cent->dustTrailTime += 40;
	if ( cent->dustTrailTime < cg.time ) {
		cent->dustTrailTime = cg.time;
	}

	VectorCopy(&cent->currentState.pos.trBase, &end);
	end.z -= 64;
	CG_Trace( &tr, &cent->currentState.pos.trBase, NULL, NULL, &end, cent->currentState.number, MASK_PLAYERSOLID );

	if ( !(tr.surfaceFlags & SURF_DUST) )
		return;

	VectorCopy( &cent->currentState.pos.trBase, &end );
	end.z -= 16;

	VectorSet(&vel, 0, 0, -30);
	CG_SmokePuff( &end, &vel,
				  24,
				  .8f, .8f, 0.7f, 0.33f,
				  500,
				  cg.time,
				  0,
				  0,
				  cgs.media.dustPuffShader );
}

static void CG_TrailItem( centity_t *cent, qhandle_t hModel ) {
	refEntity_t		ent;
	vector3			angles;
	matrix3			axis;

	VectorCopy( &cent->lerpAngles, &angles );
	angles.pitch = 0;
	angles.roll = 0;
	AnglesToAxis( &angles, axis );

	memset( &ent, 0, sizeof( ent ) );
	VectorMA( &cent->lerpOrigin, -16, &axis[0], &ent.origin );
	ent.origin.z += 16;
	angles.yaw += 90;
	AnglesToAxis( &angles, ent.axis );

	ent.hModel = hModel;
	trap->R_AddRefEntityToScene( &ent );
}

static void CG_PlayerFlag( centity_t *cent, qhandle_t hSkin, refEntity_t *torso ) {
	clientInfo_t	*ci;
	refEntity_t	pole;
	refEntity_t	flag;
	vector3		angles, dir;
	int			legsAnim, flagAnim, updateangles;
	float		angle, d;
	float		flagScale = cg_flagScale->value;

	// show the flag pole model
	memset( &pole, 0, sizeof(pole) );
	pole.hModel = cgs.media.flagPoleModel;
	VectorCopy( &torso->lightingOrigin, &pole.lightingOrigin );
	pole.shadowPlane = torso->shadowPlane;
	pole.renderfx = torso->renderfx;
	CG_PositionEntityOnTag( &pole, torso, torso->hModel, "tag_flag" );

	//QtZ: Scale flag models on players
	VectorScale( &pole.axis[0], flagScale, &pole.axis[0] );
	VectorScale( &pole.axis[1], flagScale, &pole.axis[1] );
	VectorScale( &pole.axis[2], flagScale, &pole.axis[2] );
	pole.nonNormalizedAxes = qtrue;
	//!QtZ

	trap->R_AddRefEntityToScene( &pole );

	// show the flag model
	memset( &flag, 0, sizeof(flag) );
	flag.hModel = cgs.media.flagFlapModel;
	flag.customSkin = hSkin;
	VectorCopy( &torso->lightingOrigin, &flag.lightingOrigin );
	flag.shadowPlane = torso->shadowPlane;
	flag.renderfx = torso->renderfx;

	VectorClear(&angles);

	updateangles = qfalse;
	legsAnim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if( legsAnim == LEGS_IDLE || legsAnim == LEGS_IDLECR ) {
		flagAnim = FLAG_STAND;
	} else if ( legsAnim == LEGS_WALK || legsAnim == LEGS_WALKCR ) {
		flagAnim = FLAG_STAND;
		updateangles = qtrue;
	} else {
		flagAnim = FLAG_RUN;
		updateangles = qtrue;
	}

	if ( updateangles ) {

		VectorCopy( &cent->currentState.pos.trDelta, &dir );
		// add gravity
		dir.z += 100;
		VectorNormalize( &dir );
		d = DotProduct(&pole.axis[2], &dir);
		// if there is enough movement orthogonal to the flag pole
		if (fabs(d) < 0.9f) {
			//
			d = DotProduct(&pole.axis[0], &dir);
			if (d > 1.0f) {
				d = 1.0f;
			}
			else if (d < -1.0f) {
				d = -1.0f;
			}
			angle = acosf(d);

			d = DotProduct(&pole.axis[1], &dir);
			if (d < 0) {
				angles.yaw = 360 - angle * 180 / M_PI;
			}
			else {
				angles.yaw = angle * 180 / M_PI;
			}
			if (angles.yaw < 0)
				angles.yaw += 360;
			if (angles.yaw > 360)
				angles.yaw -= 360;

			//vectoangles( cent->currentState.pos.trDelta, tmpangles );
			//angles[YAW] = tmpangles[YAW] + 45 - cent->pe.torso.yawAngle;
			// change the yaw angle
			CG_SwingAngles( angles.yaw, 25, 90, 0.15f, &cent->pe.flag.yawAngle, &cent->pe.flag.yawing );
		}

		/*
		d = DotProduct(pole.axis[2], dir);
		angle = Q_acos(d);

		d = DotProduct(pole.axis[1], dir);
		if (d < 0) {
			angle = 360 - angle * 180 / M_PI;
		}
		else {
			angle = angle * 180 / M_PI;
		}
		if (angle > 340 && angle < 20) {
			flagAnim = FLAG_RUNUP;
		}
		if (angle > 160 && angle < 200) {
			flagAnim = FLAG_RUNDOWN;
		}
		*/
	}

	// set the yaw angle
	angles.yaw = cent->pe.flag.yawAngle;
	// lerp the flag animation frames
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	CG_RunLerpFrame( ci, &cent->pe.flag, flagAnim, 1 );
	flag.oldframe = cent->pe.flag.oldFrame;
	flag.frame = cent->pe.flag.frame;
	flag.backlerp = cent->pe.flag.backlerp;

	AnglesToAxis( &angles, flag.axis );
	CG_PositionRotatedEntityOnTag( &flag, &pole, pole.hModel, "tag_flag" );

	//QtZ: Scale flag models on players
	VectorScale( &pole.axis[0], flagScale, &pole.axis[0] );
	VectorScale( &pole.axis[1], flagScale, &pole.axis[1] );
	VectorScale( &pole.axis[2], flagScale, &pole.axis[2] );
	pole.nonNormalizedAxes = qtrue;
	//!QtZ

	trap->R_AddRefEntityToScene( &flag );
}

static void CG_PlayerPowerups( centity_t *cent, refEntity_t *torso ) {
	int		powerups;
	clientInfo_t	*ci;

	powerups = cent->currentState.powerups;
	if ( !powerups ) {
		return;
	}

	// quad gives a dlight
	if ( powerups & ( 1 << PW_QUAD ) ) {
		trap->R_AddLightToScene( &cent->lerpOrigin, (float)(200 + (rand()&31)), 0.2f, 0.2f, 1.0f );
	}

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	// redflag
	if ( powerups & ( 1 << PW_REDFLAG ) ) {
		if (ci->newAnims) {
			CG_PlayerFlag( cent, cgs.media.redFlagFlapSkin, torso );
		}
		else {
			CG_TrailItem( cent, cgs.media.redFlagModel );
		}
		trap->R_AddLightToScene( &cent->lerpOrigin, (float)(200 + (rand()&31)), 1.0f, 0.2f, 0.2f );
	}

	// blueflag
	if ( powerups & ( 1 << PW_BLUEFLAG ) ) {
		if (ci->newAnims){
			CG_PlayerFlag( cent, cgs.media.blueFlagFlapSkin, torso );
		}
		else {
			CG_TrailItem( cent, cgs.media.blueFlagModel );
		}
		trap->R_AddLightToScene( &cent->lerpOrigin, (float)(200 + (rand()&31)), 0.2f, 0.2f, 1.0f );
	}

	// neutralflag
	if ( powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		if (ci->newAnims) {
			CG_PlayerFlag( cent, cgs.media.neutralFlagFlapSkin, torso );
		}
		else {
			CG_TrailItem( cent, cgs.media.neutralFlagModel );
		}
		trap->R_AddLightToScene( &cent->lerpOrigin, (float)(200 + (rand()&31)), 1.0f, 1.0f, 1.0f );
	}
}

// Float a sprite over the player's head
static void CG_PlayerFloatSprite( centity_t *cent, qhandle_t shader ) {
	int				rf;
	refEntity_t		ent;

	if ( cent->currentState.number == cg.snap->ps.clientNum ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( &cent->lerpOrigin, &ent.origin );
	ent.origin.z += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap->R_AddRefEntityToScene( &ent );
}

// Float sprites over the player's head
static void CG_PlayerSprites( centity_t *cent ) {
	int		team;

	if ( cent->currentState.eFlags & EF_CONNECTION ) {
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader );
		return;
	}

	if ( cent->currentState.eFlags & EF_TALK ) {
		CG_PlayerFloatSprite( cent, cgs.media.balloonShader );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_IMPRESSIVE ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalImpressive );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_EXCELLENT ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalExcellent );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_DEFEND ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalDefend );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_ASSIST ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalAssist );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_CAP ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalCapture );
		return;
	}

	team = cgs.clientinfo[ cent->currentState.clientNum ].team;
	if ( !(cent->currentState.eFlags & EF_DEAD) && 
		cg.snap->ps.persistent[PERS_TEAM] == team &&
		cgs.gametype >= GT_TEAMBLOOD) {
		CG_PlayerFloatSprite( cent, cgs.media.friendShader );
		return;
	}
}

#define	SHADOW_DISTANCE		128

// Returns the Z component of the surface being shadowed
//	FIXME: should it return a full plane instead of a Z?
static qboolean CG_PlayerShadow( centity_t *cent, float *shadowPlane ) {
	vector3		end, mins = {MINS_X, MINS_Y, 0}, maxs = {MAXS_X, MAXS_Y, 2};
	trace_t		trace;
	float		alpha;

	*shadowPlane = 0;

	if ( cg_shadows->integer == 0 ) {
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy( &cent->lerpOrigin, &end );
	end.z -= SHADOW_DISTANCE;

	trap->CM_Trace( &trace, &cent->lerpOrigin, &end, &mins, &maxs, 0, MASK_PLAYERSOLID, qfalse );

	// no shadow if too high
	if ( trace.fraction == 1.0f || trace.startsolid || trace.allsolid ) {
		return qfalse;
	}

	*shadowPlane = trace.endpos.z + 1;

	if ( cg_shadows->integer != 1 ) {	// no mark for stencil or projection shadows
		return qtrue;
	}

	// fade the shadow out with height
	alpha = 1.0f - trace.fraction;

	// hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f ) 

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark( cgs.media.shadowMarkShader, &trace.endpos, &trace.plane.normal, 
		cent->pe.legs.yawAngle, alpha,alpha,alpha,1, qfalse, 24, qtrue );

	return qtrue;
}

// Draw a mark at the water surface
static void CG_PlayerSplash( centity_t *cent ) {
	vector3		start, end;
	trace_t		trace;
	int			contents;
	polyVert_t	verts[4];

	if ( !cg_shadows->integer ) {
		return;
	}

	VectorCopy( &cent->lerpOrigin, &end );
	end.z -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = CG_PointContents( &end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		return;
	}

	VectorCopy( &cent->lerpOrigin, &start );
	start.z += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = CG_PointContents( &start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}

	// trace down to find the surface
	trap->CM_Trace( &trace, &start, &end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ), qfalse );

	if ( trace.fraction == 1.0f ) {
		return;
	}

	// create a mark polygon
	VectorCopy( &trace.endpos, &verts[0].xyz );
	verts[0].xyz.x -= 32;
	verts[0].xyz.y -= 32;
	verts[0].st.x = 0;
	verts[0].st.y = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy( &trace.endpos, &verts[1].xyz );
	verts[1].xyz.x -= 32;
	verts[1].xyz.y += 32;
	verts[1].st.x = 0;
	verts[1].st.y = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy( &trace.endpos, &verts[2].xyz );
	verts[2].xyz.x += 32;
	verts[2].xyz.y += 32;
	verts[2].st.x = 1;
	verts[2].st.y = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy( &trace.endpos, &verts[3].xyz );
	verts[3].xyz.x += 32;
	verts[3].xyz.y -= 32;
	verts[3].st.x = 1;
	verts[3].st.y = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap->R_AddPolysToScene( cgs.media.wakeMarkShader, 4, verts, 1 );
}

// Adds a piece with modifications or duplications for powerups
//	Also called by CG_Missile for quad rockets, but nobody can tell...
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team ) {
	clientInfo_t *ci = &cgs.clientinfo[state->clientNum];
	ivector3 *color = NULL;

	if ( (cgs.gametype < GT_TEAMBLOOD && state->number!=cg.snap->ps.clientNum) || (cgs.gametype >= GT_TEAMBLOOD && ci->team != cg.predictedPlayerState.persistent[PERS_TEAM]) )
		color = &cg.forceModel.enemyColor;
	else
		color = &cg.forceModel.allyColor;

	if ( cg_brightModels->boolean ) {
		MAKERGB( ent->shaderRGBA, (byte)color->r, (byte)color->g, (byte)color->b );
	}

	trap->R_AddRefEntityToScene( ent );

	if ( state->powerups & ( 1 << PW_QUAD ) )
	{
		if (team == TEAM_RED)
			ent->customShader = cgs.media.redQuadShader;
		else
			ent->customShader = cgs.media.quadShader;
		trap->R_AddRefEntityToScene( ent );
	}
	if ( state->powerups & ( 1 << PW_REGEN ) ) {
		if ( ((cg.time/100) % 10) == 1 ) {
			ent->customShader = cgs.media.regenShader;
			trap->R_AddRefEntityToScene( ent );
		}
	}
}

int CG_LightVerts( vector3 *normal, int numVerts, polyVert_t *verts ) {
	int i, j;
	float incoming;
	vector3 ambientLight, lightDir, directedLight;

	trap->R_LightForPoint( &verts[0].xyz, &ambientLight, &directedLight, &lightDir );

	for (i = 0; i < numVerts; i++) {
		incoming = DotProduct (normal, &lightDir);
		if ( incoming <= 0 ) {
			verts[i].modulate[0] = (byte)ambientLight.r;
			verts[i].modulate[1] = (byte)ambientLight.g;
			verts[i].modulate[2] = (byte)ambientLight.b;
			verts[i].modulate[3] = 255;
			continue;
		} 
		j = (int)( ambientLight.r + incoming * directedLight.r );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[0] = (byte)j;

		j = (int)( ambientLight.g + incoming * directedLight.g );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[1] = (byte)j;

		j = (int)( ambientLight.b + incoming * directedLight.b );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[2] = (byte)j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

void CG_Player( centity_t *cent ) {
	clientInfo_t	*ci;
	refEntity_t		legs;
	refEntity_t		torso;
	refEntity_t		head;
	int				clientNum;
	int				renderfx;
	qboolean		shadow;
	float			shadowPlane;
	refEntity_t		powerup;
	int				t;
	float			c;
	vector3			angles;

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		trap->Error( ERR_DROP, "Bad clientNum on player entity");
	}
	ci = &cgs.clientinfo[ clientNum ];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !ci->infoValid ) {
		return;
	}

	// get the player model information
	renderfx = 0;
	if ( cent->currentState.number == cg.snap->ps.clientNum) {
		renderfx = RF_THIRD_PERSON;			// only draw in mirrors
	}


	memset( &legs, 0, sizeof(legs) );
	memset( &torso, 0, sizeof(torso) );
	memset( &head, 0, sizeof(head) );

	// get the rotation information
	CG_PlayerAngles( cent, legs.axis, torso.axis, head.axis );
	
	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
		 &torso.oldframe, &torso.frame, &torso.backlerp );

	// add the talk baloon or disconnect icon
	CG_PlayerSprites( cent );

	// add the shadow
	shadow = CG_PlayerShadow( cent, &shadowPlane );

	// add a water splash if partially in and out of water
	CG_PlayerSplash( cent );

	if ( cg_shadows->integer == 3 && shadow ) {
		renderfx |= RF_SHADOW_PLANE;
	}

	renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all

	//
	// add the legs
	//
	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;

	VectorCopy( &cent->lerpOrigin, &legs.origin );

	VectorCopy( &cent->lerpOrigin, &legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy (&legs.origin, &legs.oldorigin);	// don't positionally lerp at all

	CG_AddRefEntityWithPowerups( &legs, &cent->currentState, ci->team );

	// if the model failed, allow the default nullmodel to be displayed
	if (!legs.hModel) {
		return;
	}

	//
	// add the torso
	//
	torso.hModel = ci->torsoModel;
	if (!torso.hModel) {
		return;
	}

	torso.customSkin = ci->torsoSkin;

	VectorCopy( &cent->lerpOrigin, &torso.lightingOrigin );

	CG_PositionRotatedEntityOnTag( &torso, &legs, ci->legsModel, "tag_torso");

	torso.shadowPlane = shadowPlane;
	torso.renderfx = renderfx;

	CG_AddRefEntityWithPowerups( &torso, &cent->currentState, ci->team );

	if ( cent->currentState.powerups & ( 1 << PW_GUARD ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.guardPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap->R_AddRefEntityToScene( &powerup );
	}

	t = cg.time - ci->medkitUsageTime;
	if ( ci->medkitUsageTime && t < 500 ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.medkitUsageModel;
		powerup.customSkin = 0;
		// always draw
		powerup.renderfx &= ~RF_THIRD_PERSON;
		VectorClear(&angles);
		AnglesToAxis(&angles, powerup.axis);
		VectorCopy(&cent->lerpOrigin, &powerup.origin);
		powerup.origin.z += -24 + (float) t * 80 / 500;
		if ( t > 400 ) {
			c = (float) (t - 1000) * 0xff / 100;
			powerup.shaderRGBA[0] = (byte)(0xff - c);
			powerup.shaderRGBA[1] = (byte)(0xff - c);
			powerup.shaderRGBA[2] = (byte)(0xff - c);
			powerup.shaderRGBA[3] = (byte)(0xff - c);
		}
		else {
			powerup.shaderRGBA[0] = 0xff;
			powerup.shaderRGBA[1] = 0xff;
			powerup.shaderRGBA[2] = 0xff;
			powerup.shaderRGBA[3] = 0xff;
		}
		trap->R_AddRefEntityToScene( &powerup );
	}

	//
	// add the head
	//
	head.hModel = ci->headModel;
	if (!head.hModel) {
		return;
	}
	head.customSkin = ci->headSkin;

	VectorCopy( &cent->lerpOrigin, &head.lightingOrigin );

	CG_PositionRotatedEntityOnTag( &head, &torso, ci->torsoModel, "tag_head");

	head.shadowPlane = shadowPlane;
	head.renderfx = renderfx;

	CG_AddRefEntityWithPowerups( &head, &cent->currentState, ci->team );

	CG_BreathPuffs(cent, &head);

	CG_DustTrail(cent);

	//
	// add the gun / barrel / flash
	//
	CG_AddPlayerWeapon( &torso, NULL, cent, ci->team );

	// add powerups floating behind the player
	CG_PlayerPowerups( cent, &torso );
}

// A player just came into view or teleported, so reset all animation info
void CG_ResetPlayerEntity( centity_t *cent ) {
	cent->errorTime = -99999;		// guarantee no error decay added
	cent->extrapolated = qfalse;	

	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, cent->currentState.legsAnim );
	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, cent->currentState.torsoAnim );

	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, &cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, &cent->lerpAngles );

	VectorCopy( &cent->lerpOrigin, &cent->rawOrigin );
	VectorCopy( &cent->lerpAngles, &cent->rawAngles );

	memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
	cent->pe.legs.yawAngle = cent->rawAngles.yaw;
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset( &cent->pe.torso, 0, sizeof( cent->pe.torso ) );
	cent->pe.torso.yawAngle = cent->rawAngles.yaw;
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles.pitch;
	cent->pe.torso.pitching = qfalse;
}
