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

#include "client.h"

#include "../sys/sys_local.h"
#include "../sys/sys_loadlib.h"
#include "../botlib/botlib.h"
#include "../ui/ui_public.h"

extern	botlib_export_t	*botlib_export;

static void *uiLib = NULL;
uiExport_t *ui = NULL;

/*
====================
GetClientState
====================
*/
static void GetClientState( uiClientState_t *state ) {
	state->connectPacketCount = clc.connectPacketCount;
	state->connState = clc.state;
	Q_strncpyz( state->servername, clc.servername, sizeof( state->servername ) );
	Q_strncpyz( state->updateInfoString, cls.updateInfoString, sizeof( state->updateInfoString ) );
	Q_strncpyz( state->messageString, clc.serverMessage, sizeof( state->messageString ) );
	state->clientNum = cl.snap.ps.clientNum;
}

/*
====================
LAN_LoadCachedServers
====================
*/
void LAN_LoadCachedServers( void ) {
	int size;
	fileHandle_t fileIn;
	cls.numglobalservers = cls.numfavoriteservers = 0;
	cls.numGlobalServerAddresses = 0;
	if (FS_SV_FOpenFileRead("servercache.dat", &fileIn)) {
		FS_Read(&cls.numglobalservers, sizeof(int), fileIn);
		FS_Read(&cls.numfavoriteservers, sizeof(int), fileIn);
		FS_Read(&size, sizeof(int), fileIn);
		if (size == sizeof(cls.globalServers) + sizeof(cls.favoriteServers)) {
			FS_Read(&cls.globalServers, sizeof(cls.globalServers), fileIn);
			FS_Read(&cls.favoriteServers, sizeof(cls.favoriteServers), fileIn);
		} else {
			cls.numglobalservers = cls.numfavoriteservers = 0;
			cls.numGlobalServerAddresses = 0;
		}
		FS_FCloseFile(fileIn);
	}
}

/*
====================
LAN_SaveServersToCache
====================
*/
void LAN_SaveServersToCache( void ) {
	int size;
	fileHandle_t fileOut = FS_SV_FOpenFileWrite("servercache.dat");
	FS_Write(&cls.numglobalservers, sizeof(int), fileOut);
	FS_Write(&cls.numfavoriteservers, sizeof(int), fileOut);
	size = sizeof(cls.globalServers) + sizeof(cls.favoriteServers);
	FS_Write(&size, sizeof(int), fileOut);
	FS_Write(&cls.globalServers, sizeof(cls.globalServers), fileOut);
	FS_Write(&cls.favoriteServers, sizeof(cls.favoriteServers), fileOut);
	FS_FCloseFile(fileOut);
}


/*
====================
LAN_ResetPings
====================
*/
static void LAN_ResetPings(int source) {
	int count,i;
	serverInfo_t *servers = NULL;
	count = 0;

	switch (source) {
		case AS_LOCAL :
			servers = &cls.localServers[0];
			count = MAX_OTHER_SERVERS;
			break;
		case AS_MPLAYER:
		case AS_GLOBAL :
			servers = &cls.globalServers[0];
			count = MAX_GLOBAL_SERVERS;
			break;
		case AS_FAVORITES :
			servers = &cls.favoriteServers[0];
			count = MAX_OTHER_SERVERS;
			break;
	}
	if (servers) {
		for (i = 0; i < count; i++) {
			servers[i].ping = -1;
		}
	}
}

/*
====================
LAN_AddServer
====================
*/
static int LAN_AddServer(int source, const char *name, const char *address) {
	int max, *count, i;
	netadr_t adr;
	serverInfo_t *servers = NULL;
	max = MAX_OTHER_SERVERS;
	count = NULL;

	switch (source) {
		case AS_LOCAL :
			count = &cls.numlocalservers;
			servers = &cls.localServers[0];
			break;
		case AS_MPLAYER:
		case AS_GLOBAL :
			max = MAX_GLOBAL_SERVERS;
			count = &cls.numglobalservers;
			servers = &cls.globalServers[0];
			break;
		case AS_FAVORITES :
			count = &cls.numfavoriteservers;
			servers = &cls.favoriteServers[0];
			break;
	}
	if (servers && *count < max) {
		NET_StringToAdr( address, &adr, NA_UNSPEC );
		for ( i = 0; i < *count; i++ ) {
			if (NET_CompareAdr(servers[i].adr, adr)) {
				break;
			}
		}
		if (i >= *count) {
			servers[*count].adr = adr;
			Q_strncpyz(servers[*count].hostName, name, sizeof(servers[*count].hostName));
			servers[*count].visible = qtrue;
			(*count)++;
			return 1;
		}
		return 0;
	}
	return -1;
}

/*
====================
LAN_RemoveServer
====================
*/
static void LAN_RemoveServer(int source, const char *addr) {
	int *count, i;
	serverInfo_t *servers = NULL;
	count = NULL;
	switch (source) {
		case AS_LOCAL :
			count = &cls.numlocalservers;
			servers = &cls.localServers[0];
			break;
		case AS_MPLAYER:
		case AS_GLOBAL :
			count = &cls.numglobalservers;
			servers = &cls.globalServers[0];
			break;
		case AS_FAVORITES :
			count = &cls.numfavoriteservers;
			servers = &cls.favoriteServers[0];
			break;
	}
	if (servers) {
		netadr_t comp;
		NET_StringToAdr( addr, &comp, NA_UNSPEC );
		for (i = 0; i < *count; i++) {
			if (NET_CompareAdr( comp, servers[i].adr)) {
				int j = i;
				while (j < *count - 1) {
					memcpy(&servers[j], &servers[j+1], sizeof(servers[j]));
					j++;
				}
				(*count)--;
				break;
			}
		}
	}
}


/*
====================
LAN_GetServerCount
====================
*/
static int LAN_GetServerCount( int source ) {
	switch (source) {
		case AS_LOCAL :
			return cls.numlocalservers;
			break;
		case AS_MPLAYER:
		case AS_GLOBAL :
			return cls.numglobalservers;
			break;
		case AS_FAVORITES :
			return cls.numfavoriteservers;
			break;
	}
	return 0;
}

/*
====================
LAN_GetLocalServerAddressString
====================
*/
static void LAN_GetServerAddressString( int source, int n, char *buf, int buflen ) {
	switch (source) {
		case AS_LOCAL :
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				Q_strncpyz(buf, NET_AdrToStringwPort( cls.localServers[n].adr) , buflen );
				return;
			}
			break;
		case AS_MPLAYER:
		case AS_GLOBAL :
			if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
				Q_strncpyz(buf, NET_AdrToStringwPort( cls.globalServers[n].adr) , buflen );
				return;
			}
			break;
		case AS_FAVORITES :
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				Q_strncpyz(buf, NET_AdrToStringwPort( cls.favoriteServers[n].adr) , buflen );
				return;
			}
			break;
	}
	buf[0] = '\0';
}

/*
====================
LAN_GetServerInfo
====================
*/
static void LAN_GetServerInfo( int source, int n, char *buf, int buflen ) {
	char info[MAX_STRING_CHARS];
	serverInfo_t *server = NULL;
	info[0] = '\0';
	switch (source) {
		case AS_LOCAL :
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				server = &cls.localServers[n];
			}
			break;
		case AS_MPLAYER:
		case AS_GLOBAL :
			if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
				server = &cls.globalServers[n];
			}
			break;
		case AS_FAVORITES :
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				server = &cls.favoriteServers[n];
			}
			break;
	}
	if (server && buf) {
		buf[0] = '\0';
		Info_SetValueForKey( info, "hostname", server->hostName);
		Info_SetValueForKey( info, "mapname", server->mapName);
		Info_SetValueForKey( info, "clients", va("%i",server->clients));
		Info_SetValueForKey( info, "sv_maxclients", va("%i",server->maxClients));
		Info_SetValueForKey( info, "ping", va("%i",server->ping));
		Info_SetValueForKey( info, "minping", va("%i",server->minPing));
		Info_SetValueForKey( info, "maxping", va("%i",server->maxPing));
		Info_SetValueForKey( info, "game", server->game);
		Info_SetValueForKey( info, "gametype", va("%i",server->gameType));
		Info_SetValueForKey( info, "nettype", va("%i",server->netType));
		Info_SetValueForKey( info, "addr", NET_AdrToStringwPort(server->adr));
		Info_SetValueForKey( info, "punkbuster", va("%i", server->punkbuster));
		Info_SetValueForKey( info, "g_needpass", va("%i", server->g_needpass));
		Info_SetValueForKey( info, "g_humanplayers", va("%i", server->g_humanplayers));
		Q_strncpyz(buf, info, buflen);
	} else {
		if (buf) {
			buf[0] = '\0';
		}
	}
}

/*
====================
LAN_GetServerPing
====================
*/
static int LAN_GetServerPing( int source, int n ) {
	serverInfo_t *server = NULL;
	switch (source) {
		case AS_LOCAL :
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				server = &cls.localServers[n];
			}
			break;
		case AS_MPLAYER:
		case AS_GLOBAL :
			if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
				server = &cls.globalServers[n];
			}
			break;
		case AS_FAVORITES :
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				server = &cls.favoriteServers[n];
			}
			break;
	}
	if (server) {
		return server->ping;
	}
	return -1;
}

/*
====================
LAN_GetServerPtr
====================
*/
static serverInfo_t *LAN_GetServerPtr( int source, int n ) {
	switch (source) {
		case AS_LOCAL :
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				return &cls.localServers[n];
			}
			break;
		case AS_MPLAYER:
		case AS_GLOBAL :
			if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
				return &cls.globalServers[n];
			}
			break;
		case AS_FAVORITES :
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				return &cls.favoriteServers[n];
			}
			break;
	}
	return NULL;
}

/*
====================
LAN_CompareServers
====================
*/
static int LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 ) {
	int res;
	serverInfo_t *server1, *server2;

	server1 = LAN_GetServerPtr(source, s1);
	server2 = LAN_GetServerPtr(source, s2);
	if (!server1 || !server2) {
		return 0;
	}

	res = 0;
	switch( sortKey ) {
		case SORT_HOST:
			res = Q_stricmp( server1->hostName, server2->hostName );
			break;

		case SORT_MAP:
			res = Q_stricmp( server1->mapName, server2->mapName );
			break;
		case SORT_CLIENTS:
			if (server1->clients < server2->clients) {
				res = -1;
			}
			else if (server1->clients > server2->clients) {
				res = 1;
			}
			else {
				res = 0;
			}
			break;
		case SORT_GAME:
			if (server1->gameType < server2->gameType) {
				res = -1;
			}
			else if (server1->gameType > server2->gameType) {
				res = 1;
			}
			else {
				res = 0;
			}
			break;
		case SORT_PING:
			if (server1->ping < server2->ping) {
				res = -1;
			}
			else if (server1->ping > server2->ping) {
				res = 1;
			}
			else {
				res = 0;
			}
			break;
	}

	if (sortDir) {
		if (res < 0)
			return 1;
		if (res > 0)
			return -1;
		return 0;
	}
	return res;
}

/*
====================
LAN_GetPingQueueCount
====================
*/
static int LAN_GetPingQueueCount( void ) {
	return (CL_GetPingQueueCount());
}

/*
====================
LAN_ClearPing
====================
*/
static void LAN_ClearPing( int n ) {
	CL_ClearPing( n );
}

/*
====================
LAN_GetPing
====================
*/
static void LAN_GetPing( int n, char *buf, int buflen, int *pingtime ) {
	CL_GetPing( n, buf, buflen, pingtime );
}

/*
====================
LAN_GetPingInfo
====================
*/
static void LAN_GetPingInfo( int n, char *buf, int buflen ) {
	CL_GetPingInfo( n, buf, buflen );
}

/*
====================
LAN_MarkServerVisible
====================
*/
static void LAN_MarkServerVisible(int source, int n, qboolean visible ) {
	if (n == -1) {
		int count = MAX_OTHER_SERVERS;
		serverInfo_t *server = NULL;
		switch (source) {
			case AS_LOCAL :
				server = &cls.localServers[0];
				break;
			case AS_MPLAYER:
			case AS_GLOBAL :
				server = &cls.globalServers[0];
				count = MAX_GLOBAL_SERVERS;
				break;
			case AS_FAVORITES :
				server = &cls.favoriteServers[0];
				break;
		}
		if (server) {
			for (n = 0; n < count; n++) {
				server[n].visible = visible;
			}
		}

	} else {
		switch (source) {
			case AS_LOCAL :
				if (n >= 0 && n < MAX_OTHER_SERVERS) {
					cls.localServers[n].visible = visible;
				}
				break;
			case AS_MPLAYER:
			case AS_GLOBAL :
				if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
					cls.globalServers[n].visible = visible;
				}
				break;
			case AS_FAVORITES :
				if (n >= 0 && n < MAX_OTHER_SERVERS) {
					cls.favoriteServers[n].visible = visible;
				}
				break;
		}
	}
}


/*
=======================
LAN_ServerIsVisible
=======================
*/
static int LAN_ServerIsVisible(int source, int n ) {
	switch (source) {
		case AS_LOCAL :
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				return cls.localServers[n].visible;
			}
			break;
		case AS_MPLAYER:
		case AS_GLOBAL :
			if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
				return cls.globalServers[n].visible;
			}
			break;
		case AS_FAVORITES :
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				return cls.favoriteServers[n].visible;
			}
			break;
	}
	return qfalse;
}

/*
=======================
LAN_UpdateVisiblePings
=======================
*/
qboolean LAN_UpdateVisiblePings(int source ) {
	return CL_UpdateVisiblePings_f(source);
}

/*
====================
LAN_GetServerStatus
====================
*/
int LAN_GetServerStatus( char *serverAddress, char *serverStatus, int maxLen ) {
	return CL_ServerStatus( serverAddress, serverStatus, maxLen );
}

/*
====================
CL_GetGlConfig
====================
*/
static void CL_GetGlconfig( glconfig_t *config ) {
	*config = cls.glconfig;
}

/*
====================
CL_GetClipboardData
====================
*/
static void CL_GetClipboardData( char *buf, int buflen ) {
	char	*cbd;

	cbd = Sys_GetClipboardData();

	if ( !cbd ) {
		*buf = 0;
		return;
	}

	Q_strncpyz( buf, cbd, buflen );

	Z_Free( cbd );
}

/*
====================
Key_KeynumToStringBuf
====================
*/
static void Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	Q_strncpyz( buf, Key_KeynumToString( keynum ), buflen );
}

/*
====================
Key_GetBindingBuf
====================
*/
static void Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	char	*value;

	value = Key_GetBinding( keynum );
	if ( value ) {
		Q_strncpyz( buf, value, buflen );
	}
	else {
		*buf = 0;
	}
}

/*
====================
GetConfigString
====================
*/
static int GetConfigString(int index, char *buf, int size)
{
	int		offset;

	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		return qfalse;

	offset = cl.gameState.stringOffsets[index];
	if (!offset) {
		if( size ) {
			buf[0] = 0;
		}
		return qfalse;
	}

	Q_strncpyz( buf, cl.gameState.stringData+offset, size);
 
	return qtrue;
}

/*
====================
FloatAsInt
====================
*/
static int FloatAsInt( float f ) {
	floatint_t fi;
	fi.f = f;
	return fi.i;
}

/*
====================
CL_ShutdownUI
====================
*/
void CL_ShutdownUI( void ) {
	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_UI );
	cls.uiStarted = qfalse;

	if ( !uiLib )
		return;

	ui->Shutdown();
	Sys_UnloadDll( uiLib );
	uiLib = NULL;
}

/*
====================
CL_InitUI
====================
*/

void CL_InitUI( void ) {
	static uiImport_t uiTrap;
	GetUIAPI_t		GetModuleAPI;
	char			dllName[MAX_OSPATH] = "ui"ARCH_STRING DLL_EXT;

	// load the dll or bytecode
	if( !(uiLib = Sys_LoadDll( va( "%s/%s", FS_GetCurrentGameDir(), dllName ), qfalse )) )
	{
		Com_Printf( "failed:\n\"%s\"\n", Sys_LibraryError() );
		Com_Error( ERR_FATAL, "Failed to load ui" );
	}

	memset( &uiTrap, 0, sizeof( uiTrap ) );

	GetModuleAPI = (GetUIAPI_t)Sys_LoadFunction( uiLib, "GetModuleAPI");
	if( !GetModuleAPI )
		Com_Error( ERR_FATAL, "Can't load symbol GetModuleAPI: '%s'",  Sys_LibraryError() );

	// set up the ui imports
	uiTrap.Print						= Com_Printf;
	uiTrap.Error						= Com_Error;
	uiTrap.Milliseconds					= Sys_Milliseconds;
	uiTrap.Cvar_InfoStringBuffer		= Cvar_InfoStringBuffer;
	uiTrap.Cvar_Get						= Cvar_Get;
	uiTrap.Cvar_Reset					= Cvar_Reset;
	uiTrap.Cvar_Set						= Cvar_SetSafe;
	uiTrap.Cvar_SetValue				= Cvar_SetValueSafe;
	uiTrap.Cvar_VariableStringBuffer	= Cvar_VariableStringBuffer;
	uiTrap.Cvar_VariableValue			= Cvar_VariableValue;
	uiTrap.Cmd_Argc						= Cmd_Argc;
	uiTrap.Cmd_Argv						= Cmd_ArgvBuffer;
	uiTrap.Cbuf_ExecuteText				= Cbuf_ExecuteText;
	uiTrap.FS_Open						= FS_FOpenFileByMode;
	uiTrap.FS_Read						= FS_Read2;
	uiTrap.FS_Write						= FS_Write;
	uiTrap.FS_Close						= FS_FCloseFile;
	uiTrap.FS_GetFileList				= FS_GetFileList;
	uiTrap.FS_Seek						= FS_Seek;
	uiTrap.S_RegisterSound				= S_RegisterSound;
	uiTrap.S_StartBackgroundTrack		= S_StartBackgroundTrack;
	uiTrap.S_StartLocalSound			= S_StartLocalSound;
	uiTrap.S_StopBackgroundTrack		= S_StopBackgroundTrack;
	uiTrap.R_AddLightToScene			= re->AddLightToScene;
	uiTrap.R_AddPolyToScene				= re->AddPolyToScene;
	uiTrap.R_AddRefEntityToScene		= re->AddRefEntityToScene;
	uiTrap.R_ClearScene					= re->ClearScene;
	uiTrap.R_DrawStretchPic				= re->DrawStretchPic;
	uiTrap.R_LerpTag					= re->LerpTag;
	uiTrap.R_ModelBounds				= re->ModelBounds;
	uiTrap.R_RegisterFont				= re->RegisterFont;
	uiTrap.R_RegisterModel				= re->RegisterModel;
	uiTrap.R_RegisterSkin				= re->RegisterSkin;
	uiTrap.R_RegisterShader				= re->RegisterShader;
	uiTrap.R_RemapShader				= re->RemapShader;
	uiTrap.R_RenderScene				= re->RenderScene;
	uiTrap.R_SetColor					= re->SetColor;
	uiTrap.UpdateScreen					= SCR_UpdateScreen;
	uiTrap.PC_AddGlobalDefine			= botlib_export->PC_AddGlobalDefine;
	uiTrap.PC_LoadSourceHandle			= botlib_export->PC_LoadSourceHandle;
	uiTrap.PC_FreeSourceHandle			= botlib_export->PC_FreeSourceHandle;
	uiTrap.PC_ReadTokenHandle			= botlib_export->PC_ReadTokenHandle;
	uiTrap.PC_SourceFileAndLine			= botlib_export->PC_SourceFileAndLine;
	uiTrap.GetClipboardData				= CL_GetClipboardData;
	uiTrap.GetGLConfig					= CL_GetGlconfig;
	uiTrap.GetClientState				= GetClientState;
	uiTrap.GetConfigString				= GetConfigString;
	uiTrap.MemoryRemaining				= Hunk_MemoryRemaining;
	uiTrap.RealTime						= Com_RealTime;
	uiTrap.Key_ClearStates				= Key_ClearStates;
	uiTrap.Key_GetBindingBuf			= Key_GetBindingBuf;
	uiTrap.Key_GetCatcher				= Key_GetCatcher;
	uiTrap.Key_GetOverstrikeMode		= Key_GetOverstrikeMode;
	uiTrap.Key_IsDown					= Key_IsDown;
	uiTrap.Key_KeynumToStringBuf		= Key_KeynumToStringBuf;
	uiTrap.Key_SetBinding				= Key_SetBinding;
	uiTrap.Key_SetCatcher				= Key_SetCatcher;
	uiTrap.Key_SetOverstrikeMode		= Key_SetOverstrikeMode;
	uiTrap.LAN_AddServer				= LAN_AddServer;
	uiTrap.LAN_ClearPing				= LAN_ClearPing;
	uiTrap.LAN_CompareServers			= LAN_CompareServers;
	uiTrap.LAN_GetPing					= LAN_GetPing;
	uiTrap.LAN_GetPingInfo				= LAN_GetPingInfo;
	uiTrap.LAN_GetPingQueueCount		= LAN_GetPingQueueCount;
	uiTrap.LAN_GetServerAddressString	= LAN_GetServerAddressString;
	uiTrap.LAN_GetServerCount			= LAN_GetServerCount;
	uiTrap.LAN_GetServerInfo			= LAN_GetServerInfo;
	uiTrap.LAN_GetServerPing			= LAN_GetServerPing;
	uiTrap.LAN_LoadCachedServers		= LAN_LoadCachedServers;
	uiTrap.LAN_MarkServerVisible		= LAN_MarkServerVisible;
	uiTrap.LAN_RemoveServer				= LAN_RemoveServer;
	uiTrap.LAN_ResetPings				= LAN_ResetPings;
	uiTrap.LAN_SaveServersToCache		= LAN_SaveServersToCache;
	uiTrap.LAN_ServerIsVisible			= LAN_ServerIsVisible;
	uiTrap.LAN_GetServerStatus			= LAN_GetServerStatus;
	uiTrap.LAN_UpdateVisiblePings		= LAN_UpdateVisiblePings;
	uiTrap.CIN_DrawCinematic			= CIN_DrawCinematic;
	uiTrap.CIN_PlayCinematic			= CIN_PlayCinematic;
	uiTrap.CIN_RunCinematic				= CIN_RunCinematic;
	uiTrap.CIN_SetExtents				= CIN_SetExtents;
	uiTrap.CIN_StopCinematic			= CIN_StopCinematic;

	// init the ui module and grab the exports
	if ( !(ui = GetModuleAPI( UI_API_VERSION, &uiTrap )) ) {
		cls.uiStarted = qfalse;
		Com_Error( ERR_FATAL, "Couldn't initialize ui" );
		return;
	}

	// init for this gamestate
	ui->Init( (clc.state >= CA_AUTHORIZING && clc.state < CA_ACTIVE) );
}

/*
====================
UI_GameCommand

See if the current console command is claimed by the ui
====================
*/
qboolean UI_GameCommand( void ) {
	if ( !cls.uiStarted )
		return qfalse;

	return ui->ConsoleCommand( cls.realtime );
}
