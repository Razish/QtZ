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
// cl_cgame.c  -- client system interaction with client game

#include "client.h"

#include "../sys/sys_local.h"
#include "../sys/sys_loadlib.h"
#include "../botlib/botlib.h"
#include "../cgame/cg_public.h"

#ifdef USE_MUMBLE
#include "libmumblelink.h"
#endif

extern botlib_export_t *botlib_export;

static void *cgameLib = NULL;
cgameExport_t *cgame = NULL;

extern qboolean loadCamera(const char *name);
extern void startCamera(int time);
extern qboolean getCameraInfo(int time, vector3 *origin, vector3 *angles);

/*
====================
CL_GetGameState
====================
*/
void CL_GetGameState( gameState_t *gs ) {
	*gs = cl.gameState;
}

/*
====================
CL_GetGlconfig
====================
*/
void CL_GetGlconfig( glconfig_t *glconfig ) {
	*glconfig = cls.glconfig;
}


/*
====================
CL_GetUserCmd
====================
*/
qboolean CL_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if ( cmdNumber > cl.cmdNumber ) {
		Com_Error( ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber );
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if ( cmdNumber <= cl.cmdNumber - CMD_BACKUP ) {
		return qfalse;
	}

	*ucmd = cl.cmds[ cmdNumber & CMD_MASK ];

	return qtrue;
}

int CL_GetCurrentCmdNumber( void ) {
	return cl.cmdNumber;
}


/*
====================
CL_GetParseEntityState
====================
*/
qboolean	CL_GetParseEntityState( int parseEntityNumber, entityState_t *state ) {
	// can't return anything that hasn't been parsed yet
	if ( parseEntityNumber >= cl.parseEntitiesNum ) {
		Com_Error( ERR_DROP, "CL_GetParseEntityState: %i >= %i",
			parseEntityNumber, cl.parseEntitiesNum );
	}

	// can't return anything that has been overwritten in the circular buffer
	if ( parseEntityNumber <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES ) {
		return qfalse;
	}

	*state = cl.parseEntities[ parseEntityNumber & ( MAX_PARSE_ENTITIES - 1 ) ];
	return qtrue;
}

/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
void	CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	*snapshotNumber = cl.snap.messageNum;
	*serverTime = cl.snap.serverTime;
}

/*
====================
CL_GetSnapshot
====================
*/
qboolean	CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	clSnapshot_t	*clSnap;
	int				i, count;

	if ( snapshotNumber > cl.snap.messageNum ) {
		Com_Error( ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum" );
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if ( cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP ) {
		return qfalse;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];
	if ( !clSnap->valid ) {
		return qfalse;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if ( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES ) {
		return qfalse;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
	snapshot->ps = clSnap->ps;
	count = clSnap->numEntities;
	if ( count > MAX_ENTITIES_IN_SNAPSHOT ) {
		Com_DPrintf( "CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;
	for ( i = 0 ; i < count ; i++ ) {
		snapshot->entities[i] = 
			cl.parseEntities[ ( clSnap->parseEntitiesNum + i ) & (MAX_PARSE_ENTITIES-1) ];
	}

	// FIXME: configstring changes and server commands!!!

	return qtrue;
}

/*
=====================
CL_SetUserCmdValue
=====================
*/
void CL_SetUserCmdValue( int userCmdValue, float sensitivityScale ) {
	cl.cgameUserCmdValue = userCmdValue;
	cl.cgameSensitivity = sensitivityScale;
}

/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified( void ) {
	char		*old, *s;
	int			i, index;
	char		*dup;
	gameState_t	oldGs;
	int			len;

	index = atoi( Cmd_Argv(1) );
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "CL_ConfigstringModified: bad index %i", index );
	}
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom(2);

	old = cl.gameState.stringData + cl.gameState.stringOffsets[ index ];
	if ( !strcmp( old, s ) ) {
		return;		// unchanged
	}

	// build the new gameState_t
	oldGs = cl.gameState;

	memset( &cl.gameState, 0, sizeof( cl.gameState ) );

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;
		
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( i == index ) {
			dup = s;
		} else {
			dup = oldGs.stringData + oldGs.stringOffsets[ i ];
		}
		if ( !dup[0] ) {
			continue;		// leave with the default empty string
		}

		len = strlen( dup );

		if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
			Com_Error( ERR_DROP, "MAX_GAMESTATE_CHARS exceeded" );
		}

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
		memcpy( cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1 );
		cl.gameState.dataCount += len + 1;
	}

	if ( index == CS_SYSTEMINFO ) {
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}

}


/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
qboolean CL_GetServerCommand( int serverCommandNumber ) {
	char	*s;
	char	*cmd;
	static char bigConfigString[BIG_INFO_STRING];
	int argc;

	// if we have irretrievably lost a reliable command, drop the connection
	if ( serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS ) {
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if ( clc.demoplaying )
			return qfalse;
		Com_Error( ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out" );
		return qfalse;
	}

	if ( serverCommandNumber > clc.serverCommandSequence ) {
		Com_Error( ERR_DROP, "CL_GetServerCommand: requested a command not received" );
		return qfalse;
	}

	s = clc.serverCommands[ serverCommandNumber & ( MAX_RELIABLE_COMMANDS - 1 ) ];
	clc.lastExecutedServerCommand = serverCommandNumber;

	Com_DPrintf( "serverCommand: %i : %s\n", serverCommandNumber, s );

rescan:
	Cmd_TokenizeString( s );
	cmd = Cmd_Argv(0);
	argc = Cmd_Argc();

	if ( !strcmp( cmd, "disconnect" ) ) {
		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=552
		// allow server to indicate why they were disconnected
		if ( argc >= 2 )
			Com_Error( ERR_SERVERDISCONNECT, "Server disconnected - %s", Cmd_Argv( 1 ) );
		else
			Com_Error( ERR_SERVERDISCONNECT, "Server disconnected" );
	}

	if ( !strcmp( cmd, "bcs0" ) ) {
		Com_sprintf( bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2) );
		return qfalse;
	}

	if ( !strcmp( cmd, "bcs1" ) ) {
		s = Cmd_Argv(2);
		if( strlen(bigConfigString) + strlen(s) >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		return qfalse;
	}

	if ( !strcmp( cmd, "bcs2" ) ) {
		s = Cmd_Argv(2);
		if( strlen(bigConfigString) + strlen(s) + 1 >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		strcat( bigConfigString, "\"" );
		s = bigConfigString;
		goto rescan;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		return qtrue;
	}

	if ( !strcmp( cmd, "map_restart" ) ) {
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		// reparse the string, because Con_ClearNotify() may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		memset( cl.cmds, 0, sizeof( cl.cmds ) );
		return qtrue;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return qtrue;
}


/*
====================
CL_CM_LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
void CL_CM_LoadMap( const char *mapname ) {
	int checksum;

	CM_LoadMap( mapname, qtrue, &checksum );
}

/*
====================
CL_ShutdownCGame

====================
*/
void CL_ShutdownCGame( void ) {
	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CGAME );
	cls.cgameStarted = qfalse;

	if ( !cgameLib )
		return;

	cgame->Shutdown();
	Sys_UnloadDll( cgameLib );
	cgameLib = NULL;
	cgame = NULL;
}

static int	FloatAsInt( float f ) {
	floatint_t fi;
	fi.f = f;
	return fi.i;
}

/*
====================
CL_InitCGame

Should only be called by CL_StartHunkUsers
====================
*/

void CL_InitCGame( void ) {
	const char			*info;
	const char			*mapname;
	int					t1, t2;
	static cgameImport_t cgameTrap;
	GetCGameAPI_t		GetModuleAPI;
	char				dllName[MAX_OSPATH] = "cgame"ARCH_STRING DLL_EXT;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SERVERINFO ];
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cl.mapname, sizeof( cl.mapname ), "maps/%s.bsp", mapname );

	// load the dll or bytecode
	if( !(cgameLib = Sys_LoadDll( va( "%s/%s", FS_GetCurrentGameDir(), dllName ), qfalse )) )
	{
		Com_Printf( "failed:\n\"%s\"\n", Sys_LibraryError() );
		Com_Error( ERR_FATAL, "Failed to load cgame" );
	}

	memset( &cgameTrap, 0, sizeof( cgameTrap ) );

	GetModuleAPI = (GetCGameAPI_t)Sys_LoadFunction( cgameLib, "GetModuleAPI");
	if( !GetModuleAPI )
		Com_Error( ERR_FATAL, "Can't load symbol GetModuleAPI: '%s'",  Sys_LibraryError() );

	// set up the cgame imports
	cgameTrap.Print							= Com_Printf;
	cgameTrap.Error							= Com_Error;
	cgameTrap.Milliseconds					= Sys_Milliseconds;
	cgameTrap.Cvar_Get						= Cvar_Get;
	cgameTrap.Cvar_Set						= Cvar_SetSafe;
	cgameTrap.Cvar_VariableStringBuffer		= Cvar_VariableStringBuffer;
	cgameTrap.Cmd_Argc						= Cmd_Argc;
	cgameTrap.Cmd_Argv						= Cmd_Argv;
	cgameTrap.Cmd_Args						= Cmd_ArgsBuffer;
	cgameTrap.SendConsoleCommand			= Cbuf_AddText;
	cgameTrap.AddCommand					= Cmd_AddCommand;
	cgameTrap.RemoveCommand					= Cmd_RemoveCommand;
	cgameTrap.SendClientCommand				= CL_AddReliableCommand2;
	cgameTrap.FS_Open						= FS_FOpenFileByMode;
	cgameTrap.FS_Read						= FS_Read2;
	cgameTrap.FS_Write						= FS_Write;
	cgameTrap.FS_Close						= FS_FCloseFile;
	cgameTrap.FS_Seek						= FS_Seek;
	cgameTrap.UpdateScreen					= SCR_UpdateScreen;
	cgameTrap.CM_LoadMap					= CL_CM_LoadMap;
	cgameTrap.CM_NumInlineModels			= CM_NumInlineModels;
	cgameTrap.CM_InlineModel				= CM_InlineModel;
	cgameTrap.CM_TempModel					= CM_TempBoxModel; // may not be a box
	cgameTrap.CM_Trace						= CM_BoxTrace; // may not be a box
	cgameTrap.CM_TransformedTrace			= CM_TransformedBoxTrace; // may not be a box
	cgameTrap.CM_PointContents				= CM_PointContents;
	cgameTrap.CM_TransformedPointContents	= CM_TransformedPointContents;
	cgameTrap.S_AddLoopingSound				= S_AddLoopingSound;
	cgameTrap.S_AddRealLoopingSound			= S_AddRealLoopingSound;
	cgameTrap.S_ClearLoopingSounds			= S_ClearLoopingSounds;
	cgameTrap.S_Respatialize				= S_Respatialize;
	cgameTrap.S_RegisterSound				= S_RegisterSound;
	cgameTrap.S_StartSound					= S_StartSound;
	cgameTrap.S_StartBackgroundTrack		= S_StartBackgroundTrack;
	cgameTrap.S_StartLocalSound				= S_StartLocalSound;
	cgameTrap.S_StopBackgroundTrack			= S_StopBackgroundTrack;
	cgameTrap.S_StopLoopingSound			= S_StopLoopingSound;
	cgameTrap.S_UpdateEntityPosition		= S_UpdateEntityPosition;
	cgameTrap.R_AddAdditiveLightToScene		= re->AddAdditiveLightToScene;
	cgameTrap.R_AddLightToScene				= re->AddLightToScene;
	cgameTrap.R_AddPolysToScene				= re->AddPolyToScene;
	cgameTrap.R_AddRefEntityToScene			= re->AddRefEntityToScene;
	cgameTrap.R_ClearScene					= re->ClearScene;
	cgameTrap.R_DrawStretchPic				= re->DrawStretchPic;
	cgameTrap.R_DrawRotatedPic				= re->DrawRotatedPic;
	cgameTrap.R_GetEntityToken				= re->GetEntityToken;
	cgameTrap.R_inPVS						= re->inPVS;
	cgameTrap.R_LerpTag						= re->LerpTag;
	cgameTrap.R_LightForPoint				= re->LightForPoint;
	cgameTrap.R_LoadWorld					= re->LoadWorld;
	cgameTrap.R_MarkFragments				= re->MarkFragments;
	cgameTrap.R_ModelBounds					= re->ModelBounds;
	cgameTrap.R_RegisterFont				= re->RegisterFont;
	cgameTrap.R_RegisterModel				= re->RegisterModel;
	cgameTrap.R_RegisterSkin				= re->RegisterSkin;
	cgameTrap.R_RegisterShader				= re->RegisterShader;
	cgameTrap.R_RegisterShaderNoMip			= re->RegisterShaderNoMip;
	cgameTrap.R_RemapShader					= re->RemapShader;
	cgameTrap.R_RenderScene					= re->RenderScene;
	cgameTrap.R_SetColor					= re->SetColor;
	cgameTrap.GetGLConfig					= CL_GetGlconfig;
	cgameTrap.GetGameState					= CL_GetGameState;
	cgameTrap.GetCurrentSnapshotNumber		= CL_GetCurrentSnapshotNumber;
	cgameTrap.GetSnapshot					= CL_GetSnapshot;
	cgameTrap.GetServerCommand				= CL_GetServerCommand;
	cgameTrap.GetCurrentCmdNumber			= CL_GetCurrentCmdNumber;
	cgameTrap.GetUserCmd					= CL_GetUserCmd;
	cgameTrap.SetUserCmdValue				= CL_SetUserCmdValue;
	cgameTrap.MemoryRemaining				= Hunk_MemoryRemaining;
	cgameTrap.RealTime						= Com_RealTime;
	cgameTrap.Key_IsDown					= Key_IsDown;
	cgameTrap.Key_GetCatcher				= Key_GetCatcher;
	cgameTrap.Key_SetCatcher				= Key_SetCatcher;
	cgameTrap.Key_GetKey					= Key_GetKey;
	cgameTrap.Field_CharEvent				= Field_CharEvent;
	cgameTrap.PC_AddGlobalDefine			= botlib_export->PC_AddGlobalDefine;
	cgameTrap.PC_LoadSourceHandle			= botlib_export->PC_LoadSourceHandle;
	cgameTrap.PC_FreeSourceHandle			= botlib_export->PC_FreeSourceHandle;
	cgameTrap.PC_ReadTokenHandle			= botlib_export->PC_ReadTokenHandle;
	cgameTrap.PC_SourceFileAndLine			= botlib_export->PC_SourceFileAndLine;
	cgameTrap.CIN_PlayCinematic				= CIN_PlayCinematic;
	cgameTrap.CIN_StopCinematic				= CIN_StopCinematic;
	cgameTrap.CIN_RunCinematic				= CIN_RunCinematic;
	cgameTrap.CIN_DrawCinematic				= CIN_DrawCinematic;
	cgameTrap.CIN_SetExtents				= CIN_SetExtents;

	// init the cgame module and grab the exports
	if ( !(cgame = GetModuleAPI( CGAME_API_VERSION, &cgameTrap )) ) {
		cls.cgameStarted = qfalse;
		Com_Error( ERR_FATAL, "Couldn't initialize cgame" );
		return;
	}

	clc.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	cgame->Init( clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum );

	// reset any CVAR_CHEAT cvars registered by cgame
	if ( !clc.demoplaying && !cl_connectedToCheatServer )
		Cvar_SetCheatState();

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	clc.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	Com_Printf( "CL_InitCGame: %5.2f seconds\n", (t2-t1)/1000.0 );

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re->EndRegistration();

	// make sure everything is paged in
	if ( !Sys_LowPhysicalMemory() )
		Com_TouchMemory();

	// clear anything that got printed
	Con_ClearNotify();
}

/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering( stereoFrame_t stereo ) {
	cgame->DrawActiveFrame( cl.serverTime, stereo, clc.demoplaying );
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define	RESET_TIME	500

void CL_AdjustTimeDelta( void ) {
	int		newDelta;
	int		deltaDelta;

	cl.newSnapshots = qfalse;

	// the delta never drifts when replaying a demo
	if ( clc.demoplaying ) {
		return;
	}

	newDelta = cl.snap.serverTime - cls.realtime;
	deltaDelta = abs( newDelta - cl.serverTimeDelta );

	if ( deltaDelta > RESET_TIME ) {
		cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.snap.serverTime;	// FIXME: is this a problem for cgame?
		cl.serverTime = cl.snap.serverTime;
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<RESET> " );
		}
	} else if ( deltaDelta > 100 ) {
		// fast adjust, cut the difference in half
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<FAST> " );
		}
		cl.serverTimeDelta = ( cl.serverTimeDelta + newDelta ) >> 1;
	} else {
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if ( com_timescale->value == 0 || com_timescale->value == 1 ) {
			if ( cl.extrapolatedSnapshot ) {
				cl.extrapolatedSnapshot = qfalse;
				cl.serverTimeDelta -= 2;
			} else {
				// otherwise, move our sense of time forward to minimize total latency
				cl.serverTimeDelta++;
			}
		}
	}

	if ( cl_showTimeDelta->integer ) {
		Com_Printf( "%i ", cl.serverTimeDelta );
	}
}


/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot( void ) {
	// ignore snapshots that don't have entities
	if ( cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE ) {
		return;
	}
	clc.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
	cl.oldServerTime = cl.snap.serverTime;

	clc.timeDemoBaseTime = cl.snap.serverTime;

	//QtZ: Raw mouse input
	#ifdef _WIN32
		Cbuf_AddText( "in_restart;" );
	#endif

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if ( cl_activeAction->string[0] ) {
		Cbuf_AddText( cl_activeAction->string );
		Cvar_Set( "activeAction", "" );
	}

#ifdef USE_MUMBLE
	if ((cl_useMumble->integer) && !mumble_islinked()) {
		int ret = mumble_link(CLIENT_WINDOW_TITLE);
		Com_Printf("Mumble: Linking to Mumble application %s\n", ret==0?"ok":"failed");
	}
#endif

#ifdef USE_VOIP
	if (!clc.speexInitialized) {
		int i;
		speex_bits_init(&clc.speexEncoderBits);
		speex_bits_reset(&clc.speexEncoderBits);

		clc.speexEncoder = speex_encoder_init(&speex_nb_mode);

		speex_encoder_ctl(clc.speexEncoder, SPEEX_GET_FRAME_SIZE,
		                  &clc.speexFrameSize);
		speex_encoder_ctl(clc.speexEncoder, SPEEX_GET_SAMPLING_RATE,
		                  &clc.speexSampleRate);

		clc.speexPreprocessor = speex_preprocess_state_init(clc.speexFrameSize,
		                                                  clc.speexSampleRate);

		i = 1;
		speex_preprocess_ctl(clc.speexPreprocessor,
		                     SPEEX_PREPROCESS_SET_DENOISE, &i);

		i = 1;
		speex_preprocess_ctl(clc.speexPreprocessor,
		                     SPEEX_PREPROCESS_SET_AGC, &i);

		for (i = 0; i < MAX_CLIENTS; i++) {
			speex_bits_init(&clc.speexDecoderBits[i]);
			speex_bits_reset(&clc.speexDecoderBits[i]);
			clc.speexDecoder[i] = speex_decoder_init(&speex_nb_mode);
			clc.voipIgnore[i] = qfalse;
			clc.voipGain[i] = 1.0f;
		}
		clc.speexInitialized = qtrue;
		clc.voipMuteAll = qfalse;
		Cmd_AddCommand ("voip", CL_Voip_f, NULL);
		Cvar_Set("cl_voipSendTarget", "spatial");
		memset(clc.voipTargets, ~0, sizeof(clc.voipTargets));
	}
#endif
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime( void ) {
	// getting a valid frame message ends the connection process
	if ( clc.state != CA_ACTIVE ) {
		if ( clc.state != CA_PRIMED ) {
			return;
		}
		if ( clc.demoplaying ) {
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if ( !clc.firstDemoFrameSkipped ) {
				clc.firstDemoFrameSkipped = qtrue;
				return;
			}
			CL_ReadDemoMessage();
		}
		if ( cl.newSnapshots ) {
			cl.newSnapshots = qfalse;
			CL_FirstSnapshot();
		}
		if ( clc.state != CA_ACTIVE ) {
			return;
		}
	}	

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if ( !cl.snap.valid ) {
		Com_Error( ERR_DROP, "CL_SetCGameTime: !cl.snap.valid" );
	}

	// allow pause in single player
	if ( sv_paused->integer && CL_CheckPaused() && com_sv_running->integer ) {
		// paused
		return;
	}

	if ( cl.snap.serverTime < cl.oldFrameServerTime ) {
		Com_Error( ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime" );
	}
	cl.oldFrameServerTime = cl.snap.serverTime;


	// get our current view of time

	if ( clc.demoplaying && cl_freezeDemo->integer ) {
		// cl_freezeDemo is used to lock a demo in place for single frame advances

	} else {
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better 
		// smoothness or better responsiveness.
		int tn;
		
		tn = cl_timeNudge->integer;
		if (tn<-30) {
			tn = -30;
		} else if (tn>30) {
			tn = 30;
		}

		cl.serverTime = cls.realtime + cl.serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if ( cl.serverTime < cl.oldServerTime ) {
			cl.serverTime = cl.oldServerTime;
		}
		cl.oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if ( cls.realtime + cl.serverTimeDelta >= cl.snap.serverTime - 5 ) {
			cl.extrapolatedSnapshot = qtrue;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if ( cl.newSnapshots ) {
		CL_AdjustTimeDelta();
	}

	if ( !clc.demoplaying ) {
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definately
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if ( cl_timedemo->integer ) {
		int now = Sys_Milliseconds( );
		int frameDuration;

		if (!clc.timeDemoStart) {
			clc.timeDemoStart = clc.timeDemoLastFrame = now;
			clc.timeDemoMinDuration = INT_MAX;
			clc.timeDemoMaxDuration = 0;
		}

		frameDuration = now - clc.timeDemoLastFrame;
		clc.timeDemoLastFrame = now;

		// Ignore the first measurement as it'll always be 0
		if( clc.timeDemoFrames > 0 )
		{
			if( frameDuration > clc.timeDemoMaxDuration )
				clc.timeDemoMaxDuration = frameDuration;

			if( frameDuration < clc.timeDemoMinDuration )
				clc.timeDemoMinDuration = frameDuration;

			// 255 ms = about 4fps
			if( frameDuration > UCHAR_MAX )
				frameDuration = UCHAR_MAX;

			clc.timeDemoDurations[ ( clc.timeDemoFrames - 1 ) %
				MAX_TIMEDEMO_DURATIONS ] = frameDuration;
		}

		clc.timeDemoFrames++;
		cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
	}

	while ( cl.serverTime >= cl.snap.serverTime ) {
		// feed another messag, which should change
		// the contents of cl.snap
		CL_ReadDemoMessage();
		if ( clc.state != CA_ACTIVE ) {
			return;		// end of demo
		}
	}

}



