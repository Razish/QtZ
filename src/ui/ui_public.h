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
#pragma once

#define UI_API_VERSION	1

typedef struct uiClientState_s {
	connState_t		connState;
	int				connectPacketCount;
	int				clientNum;
	char			servername[MAX_STRING_CHARS];
	char			updateInfoString[MAX_STRING_CHARS];
	char			messageString[MAX_STRING_CHARS];
} uiClientState_t;

typedef enum uiMenuCommand_e {
	UIMENU_NONE=0,
	UIMENU_MAIN,
	UIMENU_INGAME,
	UIMENU_TEAM,
	UIMENU_POSTGAME
} uiMenuCommand_t;

#define SORT_HOST			0
#define SORT_MAP			1
#define SORT_CLIENTS		2
#define SORT_GAME			3
#define SORT_PING			4
#define SORT_PUNKBUSTER		5


//================================
//
// UI API
//
//================================

typedef struct uiImport_s {
	// common
	void			(*Print)						( const char *msg, ... );
	void			(*Error)						( int level, const char *error, ... );
	int				(*Milliseconds)					( void );

	// cvar
	void			(*Cvar_InfoStringBuffer)		( int bit, char *buff, int buffsize );
	cvar_t *		(*Cvar_Get)						( const char *name, const char *value, int flags, const char *description, void (*update)( void ) );
	void			(*Cvar_Reset)					( const char *name );
	void			(*Cvar_Set)						( const char *name, const char *value );
	void			(*Cvar_SetValue)				( const char *name, float value );
	void			(*Cvar_VariableStringBuffer)	( const char *name, char *buffer, int bufsize );
	float			(*Cvar_VariableValue)			( const char *name );

	// cmd
	int				(*Cmd_Argc)						( void );
	void			(*Cmd_Argv)						( int arg, char *buffer, int bufferLength );
	void			(*Cbuf_ExecuteText)				( int exec_when, const char *text );

	// filesystem
	int				(*FS_Open)						( const char *qpath, fileHandle_t *f, fsMode_t mode );
	int				(*FS_Read)						( void *buffer, int len, fileHandle_t f );
	int				(*FS_Write)						( const void *buffer, int len, fileHandle_t f );
	void			(*FS_Close)						( fileHandle_t f );
	int				(*FS_GetFileList)				( const char *path, const char *extension, char *listbuf, int bufsize );
	int				(*FS_Seek)						( fileHandle_t f, long offset, int origin );

	// sound
	sfxHandle_t		(*S_RegisterSound)				( const char *sample, qboolean compressed );
	void			(*S_StartBackgroundTrack)		( const char *intro, const char *loop );
	void			(*S_StartLocalSound)			( sfxHandle_t sfx, int channelNum );
	void			(*S_StopBackgroundTrack)		( void );

	// renderer
	void			(*R_AddLightToScene)			( const vector3 *org, float intensity, float r, float g, float b );
	void			(*R_AddPolyToScene)				( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
	void			(*R_AddRefEntityToScene)		( const refEntity_t *re );
	void			(*R_ClearScene)					( void );
	void			(*R_DrawStretchPic)				( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
	int				(*R_LerpTag)					( orientation_t *tag,  qhandle_t model, int startFrame, int endFrame, float frac, const char *tagName );
	void			(*R_ModelBounds)				( qhandle_t model, vector3 *mins, vector3 *maxs );
	void			(*R_RegisterFont)				( const char *fontName, int pointSize, fontInfo_t *font );
	qhandle_t		(*R_RegisterModel)				( const char *name );
	qhandle_t		(*R_RegisterSkin)				( const char *name );
	qhandle_t		(*R_RegisterShader)				( const char *name );
	void			(*R_RemapShader)				( const char *oldShader, const char *newShader, const char *offsetTime );
	void			(*R_RenderScene)				( const refdef_t *fd );
	void			(*R_SetColor)					( const vector4 *rgba );

	// screen
	void			(*UpdateScreen)					( void );

	// preprocessor, imported from botlib
	int				(*PC_AddGlobalDefine)			( char *string );
	int				(*PC_LoadSourceHandle)			( const char *filename );
	int				(*PC_FreeSourceHandle)			( int handle );
	int				(*PC_ReadTokenHandle)			( int handle, pc_token_t *pc_token );
	int				(*PC_SourceFileAndLine)			( int handle, char *filename, int *line );

	// misc
	void			(*GetClipboardData)				( char *buf, int buflen );
	void			(*GetGLConfig)					( glconfig_t *config );
	void			(*GetClientState)				( uiClientState_t *state );
	int				(*GetConfigString)				( int index, char *buf, int size );
	int				(*MemoryRemaining)				( void );
	int				(*RealTime)						( qtime_t *qtime );

	// keys
	void			(*Key_ClearStates)				( void );
	void			(*Key_GetBindingBuf)			( int keynum, char *buf, int buflen );
	int				(*Key_GetCatcher)				( void );
	qboolean		(*Key_GetOverstrikeMode)		( void );
	qboolean		(*Key_IsDown)					( int keynum );
	void			(*Key_KeynumToStringBuf)		( int keynum, char *buf, int buflen );
	void			(*Key_SetBinding)				( int keynum, const char *binding );
	void			(*Key_SetCatcher)				( int catcher );
	void			(*Key_SetOverstrikeMode)		( qboolean state );

	// server browser
	int				(*LAN_AddServer)				( int source, const char *name, const char *address );
	void			(*LAN_ClearPing)				( int n );
	int				(*LAN_CompareServers)			( int source, int sortKey, int sortDir, int s1, int s2 );
	void			(*LAN_GetPing)					( int n, char *buf, int buflen, int *pingtime );
	void			(*LAN_GetPingInfo)				( int n, char *buf, int buflen );
	int				(*LAN_GetPingQueueCount)		( void );
	void			(*LAN_GetServerAddressString)	( int source, int n, char *buf, int buflen );
	int				(*LAN_GetServerCount)			( int source );
	void			(*LAN_GetServerInfo)			( int source, int n, char *buf, int buflen );
	int				(*LAN_GetServerPing)			( int source, int n );
	void			(*LAN_LoadCachedServers)		( void );
	void			(*LAN_MarkServerVisible)		( int source, int n, qboolean visible );
	void			(*LAN_RemoveServer)				( int source, const char *addr );
	void			(*LAN_ResetPings)				( int source );
	void			(*LAN_SaveServersToCache)		( void );
	int				(*LAN_ServerIsVisible)			( int source, int n );
	int				(*LAN_GetServerStatus)			( char *serverAddress, char *serverStatus, int maxLen );
	qboolean		(*LAN_UpdateVisiblePings)		( int source );

	// cinematic
	void			(*CIN_DrawCinematic)			( int handle );
	int				(*CIN_PlayCinematic)			( const char *arg0, int xpos, int ypos, int width, int height, int bits );
	cinState_t		(*CIN_RunCinematic)				( int handle );
	void			(*CIN_SetExtents)				( int handle, int x, int y, int w, int h );
	cinState_t		(*CIN_StopCinematic)			( int handle );
} uiImport_t;

typedef struct uiExport_s {
	void		(*Init)					( qboolean inGameLoad );
	void		(*Shutdown)				( void );
	void		(*KeyEvent)				( int key, qboolean down );
	void		(*MouseEvent)			( int dx, int dy );
	void		(*Refresh)				( int time );
	qboolean	(*IsFullscreen)			( void );
	void		(*SetActiveMenu)		( uiMenuCommand_t menu );
	qboolean	(*ConsoleCommand)		( int realTime );
	void		(*DrawConnectScreen)	( qboolean overlay );
} uiExport_t;

//linking of ui library
typedef uiExport_t* (QDECL *GetUIAPI_t)( int apiVersion, uiImport_t *import );
