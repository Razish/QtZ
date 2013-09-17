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
// sv_game.c -- interface to the game dll

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../sys/sys_local.h"
#include "../sys/sys_loadlib.h"
#include "../botlib/botlib.h"
#include "../game/g_public.h"
#include "sv_local.h"

static void *gameLib = NULL;
gameExport_t *game = NULL;

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
int	SV_NumForGentity( sharedEntity_t *ent ) {
	int		num;

	num = ( (byte *)ent - (byte *)sv.gentities ) / sv.gentitySize;

	return num;
}

sharedEntity_t *SV_GentityNum( int num ) {
	sharedEntity_t *ent;

	ent = (sharedEntity_t *)((byte *)sv.gentities + sv.gentitySize*(num));

	return ent;
}

playerState_t *SV_GameClientNum( int num ) {
	playerState_t	*ps;

	ps = (playerState_t *)((byte *)sv.gameClients + sv.gameClientSize*(num));

	return ps;
}

svEntity_t	*SV_SvEntityForGentity( sharedEntity_t *gEnt ) {
	if ( !gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "SV_SvEntityForGentity: bad gEnt" );
	}
	return &sv.svEntities[ gEnt->s.number ];
}

sharedEntity_t *SV_GEntityForSvEntity( svEntity_t *svEnt ) {
	int		num;

	num = svEnt - sv.svEntities;
	return SV_GentityNum( num );
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand( int clientNum, const char *text ) {
	if ( clientNum == -1 ) {
		SV_SendServerCommand( NULL, "%s", text );
	} else {
		if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
			return;
		}
		SV_SendServerCommand( svs.clients + clientNum, "%s", text );	
	}
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient( int clientNum, const char *reason ) {
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		return;
	}
	SV_DropClient( svs.clients + clientNum, reason );	
}


/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel( sharedEntity_t *ent, const char *name ) {
	clipHandle_t	h;
	vector3			mins, maxs;

	if (!name) {
		Com_Error( ERR_DROP, "SV_SetBrushModel: NULL" );
	}

	if (name[0] != '*') {
		Com_Error( ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name );
	}


	ent->s.modelindex = atoi( name + 1 );

	h = CM_InlineModel( ent->s.modelindex );
	CM_ModelBounds( h, &mins, &maxs );
	VectorCopy (&mins, &ent->r.mins);
	VectorCopy (&maxs, &ent->r.maxs);
	ent->r.bmodel = qtrue;

	ent->r.contents = -1;		// we don't know exactly what is in the brushes

	SV_LinkEntity( ent );		// FIXME: remove
}



/*
=================
SV_InPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean SV_InPVS (const vector3 *p1, const vector3 *p2)
{
	int		leafnum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1 = CM_LeafArea (leafnum);
	mask = CM_ClusterPVS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2 = CM_LeafArea (leafnum);
	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return qfalse;
	if (!CM_AreasConnected (area1, area2))
		return qfalse;		// a door blocks sight
	return qtrue;
}


/*
=================
SV_InPVSIgnorePortals

Does NOT check portalareas
=================
*/
// currently unused...
static qboolean SV_InPVSIgnorePortals( const vector3 *p1, const vector3 *p2)
{
	int		leafnum;
	int		cluster;
	byte	*mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	mask = CM_ClusterPVS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);

	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return qfalse;

	return qtrue;
}


/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState( sharedEntity_t *ent, qboolean open ) {
	svEntity_t	*svEnt;

	svEnt = SV_SvEntityForGentity( ent );
	if ( svEnt->areanum2 == -1 ) {
		return;
	}
	CM_AdjustAreaPortalState( svEnt->areanum, svEnt->areanum2, open );
}


/*
==================
SV_EntityContact
==================
*/
qboolean	SV_EntityContact( vector3 *mins, vector3 *maxs, const sharedEntity_t *gEnt, int capsule ) {
	const vector3	*origin, *angles;
	clipHandle_t	ch;
	trace_t			trace;

	// check for exact collision
	origin = &gEnt->r.currentOrigin;
	angles = &gEnt->r.currentAngles;

	ch = SV_ClipHandleForEntity( gEnt );
	CM_TransformedBoxTrace ( &trace, &vec3_origin, &vec3_origin, mins, maxs, ch, -1, origin, angles, capsule );

	return trace.startsolid;
}


/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo( char *buffer, int bufferSize ) {
	if ( bufferSize < 1 )
		Com_Error( ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize );
	Q_strncpyz( buffer, Cvar_InfoString( CVAR_SERVERINFO ), bufferSize );
}

/*
===============
SV_LocateGameData

===============
*/
void SV_LocateGameData( sharedEntity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient ) {
	sv.gentities = gEnts;
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients = clients;
	sv.gameClientSize = sizeofGameClient;
}


/*
===============
SV_GetUsercmd

===============
*/
void SV_GetUsercmd( int clientNum, usercmd_t *cmd ) {
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum );
	}
	*cmd = svs.clients[clientNum].lastUsercmd;
}

/*
================
SV_GetEntityToken

================
*/
static qboolean SV_GetEntityToken( char *buffer, int bufferSize ) {
	const char	*s;

	s = COM_Parse( &sv.entityParsePoint );
	Q_strncpyz( buffer, s, bufferSize );
	if ( !sv.entityParsePoint && !s[0] )
		return qfalse;
	else
		return qtrue;
}

/*
================
SV_GameClientThink

================
*/
static void SV_GameClientThink( int clientNum, usercmd_t *cmd ) {
	SV_ClientThink( &svs.clients[clientNum], cmd );
}

/*
================
SV_GameTrace

================
*/
static void SV_GameTrace( trace_t *results, const vector3 *start, const vector3 *mins, const vector3 *maxs, const vector3 *end, int passEntityNum, int contentmask ) {
	SV_Trace( results, start, mins, maxs, end, passEntityNum, contentmask, qfalse );
}

//==============================================

static int	FloatAsInt( float f ) {
	floatint_t fi;
	fi.f = f;
	return fi.i;
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs( void ) {
	if ( !svs.gameStarted )
		return;

	game->Shutdown( qfalse );
	Sys_UnloadDll( gameLib );
	gameLib = NULL;
	game = NULL;
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM( qboolean restart ) {
	int		i;

	// start the entity parsing at the beginning
	sv.entityParsePoint = CM_EntityString();

	// clear all gentity pointers that might still be set from
	// a previous level
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=522
	//   now done before GAME_INIT call
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		svs.clients[i].gentity = NULL;
	}
	
	// use the current msec count for a random seed
	// init for this gamestate
	game->Init( sv.time, Com_Milliseconds(), restart );
}



/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs( void ) {
	if ( !gameLib )
		return;

	game->Shutdown( qtrue );

	//QTZTODO: OJK restart
	// do a restart instead of a free
//	if ( !gvm )
//		Com_Error( ERR_FATAL, "SV_RestartGameProgs failed" );

	SV_InitGameVM( qtrue );
}


/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs( void ) {
	cvar_t	*var;
	//FIXME these are temp while I make bots run in vm
	extern int	bot_enable;
	static gameImport_t gameTrap;
	GetGameAPI_t		GetModuleAPI;
	char				dllName[MAX_OSPATH] = "game"ARCH_STRING DLL_EXT;

	var = Cvar_Get( "bot_enable", "1", CVAR_LATCH, NULL, NULL );
	if ( var )	bot_enable = var->integer;
	else		bot_enable = 0;

	svs.gameStarted = qtrue;

	// load the dll or bytecode
	if ( !(gameLib = Sys_LoadDll( va( "%s/%s", FS_GetCurrentGameDir(), dllName ), qfalse )) ) {
		Com_Printf( "failed:\n\"%s\"\n", Sys_LibraryError() );
		svs.gameStarted = qfalse;
		Com_Error( ERR_FATAL, "Failed to load game" );
	}

	GetModuleAPI = (GetGameAPI_t)Sys_LoadFunction( gameLib, "GetModuleAPI");
	if ( !GetModuleAPI ) {
		svs.gameStarted = qfalse;
		Com_Error( ERR_FATAL, "Can't load symbol GetModuleAPI: '%s'",  Sys_LibraryError() );
	}

	// set up the game imports
	gameTrap.Print						= Com_Printf;
	gameTrap.Error						= Com_Error;
	gameTrap.Milliseconds				= Sys_Milliseconds;
	gameTrap.Cvar_Get					= Cvar_Get;
	gameTrap.Cvar_Set					= Cvar_SetSafe;
	gameTrap.Cvar_VariableIntegerValue	= Cvar_VariableIntegerValue;
	gameTrap.Cvar_VariableStringBuffer	= Cvar_VariableStringBuffer;
	gameTrap.Cmd_Argc					= Cmd_Argc;
	gameTrap.Cmd_Argv					= Cmd_ArgvBuffer;
	gameTrap.Cbuf_ExecuteText			= Cbuf_ExecuteText;
	gameTrap.FS_Open					= FS_FOpenFileByMode;
	gameTrap.FS_Read					= FS_Read2;
	gameTrap.FS_Write					= FS_Write;
	gameTrap.FS_Close					= FS_FCloseFile;
	gameTrap.FS_GetFileList				= FS_GetFileList;
	gameTrap.FS_Seek					= FS_Seek;
	gameTrap.SV_LocateGameData			= SV_LocateGameData;
	gameTrap.SV_GameDropClient			= SV_GameDropClient;
	gameTrap.SV_GameSendServerCommand	= SV_GameSendServerCommand;
	gameTrap.SV_SetConfigstring			= SV_SetConfigstring;
	gameTrap.SV_GetConfigstring			= SV_GetConfigstring;
	gameTrap.SV_GetUserinfo				= SV_GetUserinfo;
	gameTrap.SV_SetUserinfo				= SV_SetUserinfo;
	gameTrap.SV_GetServerinfo			= SV_GetServerinfo;
	gameTrap.SV_SetBrushModel			= SV_SetBrushModel;
	gameTrap.SV_Trace					= SV_GameTrace;
	gameTrap.SV_EntityContact			= SV_EntityContact;
	gameTrap.SV_PointContents			= SV_PointContents;
	gameTrap.SV_InPVS					= SV_InPVS;
	gameTrap.SV_InPVSIgnorePortals		= SV_InPVSIgnorePortals;
	gameTrap.SV_AdjustAreaPortalState	= SV_AdjustAreaPortalState;
	gameTrap.CM_AreasConnected			= CM_AreasConnected;
	gameTrap.SV_LinkEntity				= SV_LinkEntity;
	gameTrap.SV_UnlinkEntity			= SV_UnlinkEntity;
	gameTrap.SV_AreaEntities			= SV_AreaEntities;
	gameTrap.SV_BotAllocateClient		= SV_BotAllocateClient;
	gameTrap.SV_BotFreeClient			= SV_BotFreeClient;
	gameTrap.SV_GetUsercmd				= SV_GetUsercmd;
	gameTrap.SV_GetEntityToken			= SV_GetEntityToken;
	gameTrap.DebugPolygonCreate			= BotImport_DebugPolygonCreate;
	gameTrap.DebugPolygonDelete			= BotImport_DebugPolygonDelete;
	gameTrap.RealTime					= Com_RealTime;
	gameTrap.SV_BotLibSetup				= SV_BotLibSetup;
	gameTrap.SV_BotLibShutdown			= SV_BotLibShutdown;
	gameTrap.BotLibVarSet				= botlib_export->BotLibVarSet;
	gameTrap.BotLibVarGet				= botlib_export->BotLibVarGet;
	gameTrap.PC_AddGlobalDefine			= botlib_export->PC_AddGlobalDefine;
	gameTrap.PC_LoadSourceHandle		= botlib_export->PC_LoadSourceHandle;
	gameTrap.PC_FreeSourceHandle		= botlib_export->PC_FreeSourceHandle;
	gameTrap.PC_ReadTokenHandle			= botlib_export->PC_ReadTokenHandle;
	gameTrap.PC_SourceFileAndLine		= botlib_export->PC_SourceFileAndLine;
	gameTrap.BotLibStartFrame			= botlib_export->BotLibStartFrame;
	gameTrap.BotLibLoadMap				= botlib_export->BotLibLoadMap;
	gameTrap.BotLibUpdateEntity			= botlib_export->BotLibUpdateEntity;
	gameTrap.BotLibTest					= botlib_export->Test;
	gameTrap.SV_BotGetSnapshotEntity	= SV_BotGetSnapshotEntity;
	gameTrap.SV_BotGetConsoleMessage	= SV_BotGetConsoleMessage;
	gameTrap.SV_ClientThink				= SV_GameClientThink;
	gameTrap.aas						= &botlib_export->aas;
	gameTrap.ea							= &botlib_export->ea;
	gameTrap.ai							= &botlib_export->ai;

	// init the cgame module and grab the exports
	if ( !(game = GetModuleAPI( GAME_API_VERSION, &gameTrap )) ) {
		svs.gameStarted = qfalse;
		Com_Error( ERR_FATAL, "Couldn't initialize game" );
	}

	SV_InitGameVM( qfalse );
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
qboolean SV_GameCommand( void ) {
	if ( sv.state != SS_GAME )
		return qfalse;

	return game->ConsoleCommand();
}

