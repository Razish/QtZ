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
// cl_bot.c
// most of these are stubs in the client.
// should eventually remove this dependency

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../botlib/botlib.h"
#include "../sys/sys_local.h"
#include "../sys/sys_loadlib.h"
#include "server/sv_local.h"
#include "client.h"

botlib_export_t	*botlib_export;
void *botlibLib;

static __attribute__ ((format (printf, 2, 3))) void QDECL BotImport_Print(int type, char *fmt, ...)
{
	char str[2048];
	va_list ap;

	va_start(ap, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);

	switch(type) {
		case PRT_MESSAGE: {
			Com_Printf("%s", str);
			break;
		}
		case PRT_WARNING: {
			Com_Printf(S_COLOR_YELLOW "Warning: %s", str);
			break;
		}
		case PRT_ERROR: {
			Com_Printf(S_COLOR_RED "Error: %s", str);
			break;
		}
		case PRT_FATAL: {
			Com_Printf(S_COLOR_RED "Fatal: %s", str);
			break;
		}
		case PRT_EXIT: {
			Com_Error(ERR_DROP, S_COLOR_RED "Exit: %s", str);
			break;
		}
		default: {
			Com_Printf("unknown print type\n");
			break;
		}
	}
}

static void BotImport_Trace(bsp_trace_t *bsptrace, vector3 *start, vector3 *mins, vector3 *maxs, vector3 *end, int passent, int contentmask) {
}

static void BotImport_EntityTrace(bsp_trace_t *bsptrace, vector3 *start, vector3 *mins, vector3 *maxs, vector3 *end, int entnum, int contentmask) {
}

static int BotImport_PointContents(vector3 *point) {
	return 0;
}

static int BotImport_inPVS(vector3 *p1, vector3 *p2) {
	return 0;
}

static char *BotImport_BSPEntityData(void) {
	return CM_EntityString();
}

static void BotImport_BSPModelMinsMaxsOrigin(int modelnum, vector3 *angles, vector3 *outmins, vector3 *outmaxs, vector3 *origin) {
}

static void *BotImport_GetMemory(size_t size) {
	return Z_TagMalloc( size, TAG_BOTLIB );
}

static void BotImport_FreeMemory(void *ptr) {
	Z_Free(ptr);
}

static void *BotImport_HunkAlloc( size_t size ) {
	if( Hunk_CheckMark() ) {
		Com_Error( ERR_DROP, "CL_Bot_HunkAlloc: Alloc with marks already set" );
	}
	return Hunk_Alloc( size, PREF_HIGH );
}

static void BotImport_DebugPolygonShow(int id, int color, int numPoints, vector3 *points) {
}

static int BotImport_DebugLineCreate(void) {
	vector3 points[1];
	return BotImport_DebugPolygonCreate(0, 0, points);
}

static void BotImport_DebugLineDelete(int line) {
}

static void BotImport_DebugLineShow(int line, vector3 *start, vector3 *end, int color) {
}

static void BotClientCommand( int client, char *command ) {
	const char *arg = NULL;
	client_t *cl = &svs.clients[client];
	
	Cmd_TokenizeString( command );
	arg = Cmd_Argv( 0 );

	// pass unknown strings to the game
	if ( sv.state == SS_GAME && (cl->state == CS_ACTIVE || cl->state == CS_PRIMED) ) {
		Cmd_Args_Sanitize();
		game->ClientCommand( client );
	}
}

void CL_BotFrame( int time ) {
}

/*
===============
CL_BotLibSetup
===============
*/
int CL_BotLibSetup( void ) {
	if ( !botlib_export ) {
		Com_Printf( S_COLOR_RED "Error: CL_BotLibSetup without CL_BotInitBotLib\n" );
		return -1;
	}

	return botlib_export->BotLibSetup();
}

/*
===============
CL_ShutdownBotLib

Called when either the entire client is being killed, or
it is changing to a different game directory.
===============
*/
int CL_BotLibShutdown( void ) {

	if ( !botlib_export ) {
		return -1;
	}

	return botlib_export->BotLibShutdown();
}

/*
==================
CL_BotInitCvars
==================
*/
void CL_BotInitCvars(void) {

	Cvar_Get( "bot_enable",					"1",	CVAR_NONE,	NULL, NULL );	// enable the bot
	Cvar_Get( "bot_developer",				"0",	CVAR_CHEAT,	NULL, NULL );	// bot developer mode
	Cvar_Get( "bot_debug",					"0",	CVAR_CHEAT,	NULL, NULL );	// enable bot debugging
	Cvar_Get( "bot_maxdebugpolys",			"2",	CVAR_NONE,	NULL, NULL );	// maximum number of debug polys
	Cvar_Get( "bot_groundonly",				"1",	CVAR_NONE,	NULL, NULL );	// only show ground faces of areas
	Cvar_Get( "bot_reachability",			"0",	CVAR_NONE,	NULL, NULL );	// show all reachabilities to other areas
	Cvar_Get( "bot_visualizejumppads",		"0",	CVAR_CHEAT,	NULL, NULL );	// show jumppads
	Cvar_Get( "bot_forceclustering",		"0",	CVAR_NONE,	NULL, NULL );	// force cluster calculations
	Cvar_Get( "bot_forcereachability",		"0",	CVAR_NONE,	NULL, NULL );	// force reachability calculations
	Cvar_Get( "bot_forcewrite",				"0",	CVAR_NONE,	NULL, NULL );	// force writing aas file
	Cvar_Get( "bot_aasoptimize",			"0",	CVAR_NONE,	NULL, NULL );	// no aas file optimisation
	Cvar_Get( "bot_saveroutingcache",		"0",	CVAR_NONE,	NULL, NULL );	// save routing cache
	Cvar_Get( "bot_thinktime",				"100",	CVAR_CHEAT,	NULL, NULL );	// msec the bots thinks
	Cvar_Get( "bot_reloadcharacters",		"0",	CVAR_NONE,	NULL, NULL );	// reload the bot characters each time
	Cvar_Get( "bot_testichat",				"0",	CVAR_NONE,	NULL, NULL );	// test ichats
	Cvar_Get( "bot_testrchat",				"0",	CVAR_NONE,	NULL, NULL );	// test rchats
	Cvar_Get( "bot_testsolid",				"0",	CVAR_CHEAT,	NULL, NULL );	// test for solid areas
	Cvar_Get( "bot_testclusters",			"0",	CVAR_CHEAT,	NULL, NULL );	// test the AAS clusters
	Cvar_Get( "bot_fastchat",				"0",	CVAR_NONE,	NULL, NULL );	// fast chatting bots
	Cvar_Get( "bot_nochat",					"0",	CVAR_NONE,	NULL, NULL );	// disable chats
	Cvar_Get( "bot_pause",					"0",	CVAR_CHEAT,	NULL, NULL );	// pause the bots thinking
	Cvar_Get( "bot_report",					"0",	CVAR_CHEAT,	NULL, NULL );	// get a full report in ctf
	Cvar_Get( "bot_grapple",				"0",	CVAR_NONE,	NULL, NULL );	// enable grapple
	Cvar_Get( "bot_rocketjump",				"1",	CVAR_NONE,	NULL, NULL );	// enable rocket jumping
	Cvar_Get( "bot_challenge",				"0",	CVAR_NONE,	NULL, NULL );	// challenging bot
	Cvar_Get( "bot_minplayers",				"0",	CVAR_NONE,	NULL, NULL );	// minimum players in a team or the game
	Cvar_Get( "bot_interbreedchar",			"",		CVAR_CHEAT,	NULL, NULL );	// bot character used for interbreeding
	Cvar_Get( "bot_interbreedbots",			"10",	CVAR_CHEAT,	NULL, NULL );	// number of bots used for interbreeding
	Cvar_Get( "bot_interbreedcycle",		"20",	CVAR_CHEAT,	NULL, NULL );	// bot interbreeding cycle
	Cvar_Get( "bot_interbreedwrite",		"",		CVAR_CHEAT,	NULL, NULL );	// write interbreeded bots to this file
}

/*
==================
CL_BotInitBotLib
==================
*/
void CL_BotInitBotLib(void) {
	botlib_import_t	botlib_import;
	GetBotLibAPI_t GetBotLibAPI;
	char dllName[MAX_OSPATH] = "botlib_"ARCH_STRING DLL_EXT;

	if( !(botlibLib = Sys_LoadDll( dllName, qfalse )) )
	{
		Com_Printf( "failed:\n\"%s\"\n", Sys_LibraryError() );
		Com_Error( ERR_FATAL, "Failed to load botlib" );
	}

	GetBotLibAPI = (GetBotLibAPI_t)Sys_LoadFunction( botlibLib, "GetBotLibAPI" );
	if ( !GetBotLibAPI )
		Com_Error(ERR_FATAL, "Can't load symbol GetBotLibAPI: '%s'",  Sys_LibraryError());

	CL_BotInitCvars();

	botlib_import.Print = BotImport_Print;
	botlib_import.Error = Com_Error;
	botlib_import.Trace = BotImport_Trace;
	botlib_import.EntityTrace = BotImport_EntityTrace;
	botlib_import.PointContents = BotImport_PointContents;
	botlib_import.inPVS = BotImport_inPVS;
	botlib_import.BSPEntityData = BotImport_BSPEntityData;
	botlib_import.BSPModelMinsMaxsOrigin = BotImport_BSPModelMinsMaxsOrigin;
	botlib_import.BotClientCommand = BotClientCommand;

	//memory management
	botlib_import.GetMemory = BotImport_GetMemory;
	botlib_import.FreeMemory = BotImport_FreeMemory;
	botlib_import.AvailableMemory = Z_AvailableMemory;
	botlib_import.HunkAlloc = BotImport_HunkAlloc;

	// file system access
	botlib_import.FS_FOpenFile = FS_FOpenFileByMode;
	botlib_import.FS_Read = FS_Read2;
	botlib_import.FS_Write = FS_Write;
	botlib_import.FS_FCloseFile = FS_FCloseFile;
	botlib_import.FS_Seek = FS_Seek;

	//debug lines
	botlib_import.DebugLineCreate = BotImport_DebugLineCreate;
	botlib_import.DebugLineDelete = BotImport_DebugLineDelete;
	botlib_import.DebugLineShow = BotImport_DebugLineShow;

	//debug polygons
	botlib_import.DebugPolygonCreate = BotImport_DebugPolygonCreate;
	botlib_import.DebugPolygonDelete = BotImport_DebugPolygonDelete;

	botlib_export = (botlib_export_t *)GetBotLibAPI( BOTLIB_API_VERSION, &botlib_import );
	assert(botlib_export); 	// somehow we end up with a zero import.
}
