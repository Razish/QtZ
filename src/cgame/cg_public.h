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

#define	CGAME_API_VERSION		1

#define	CMD_BACKUP			64	
#define	CMD_MASK			(CMD_BACKUP - 1)
// allow a lot of command backups for very fast systems
// multiple commands may be combined into a single packet, so this
// needs to be larger than PACKET_BACKUP


#define	MAX_ENTITIES_IN_SNAPSHOT	256

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
typedef struct {
	int				snapFlags;			// SNAPFLAG_RATE_DELAYED, etc
	int				ping;

	int				serverTime;		// server time the message is valid for (in msec)

	byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	playerState_t	ps;						// complete information about the current player at this time

	int				numEntities;			// all of the entities that need to be presented
	entityState_t	entities[MAX_ENTITIES_IN_SNAPSHOT];	// at the time of this snapshot

	int				numServerCommands;		// text based server commands to execute when this
	int				serverCommandSequence;	// snapshot becomes current
} snapshot_t;

enum {
  CGAME_EVENT_NONE,
  CGAME_EVENT_TEAMMENU,
  CGAME_EVENT_SCOREBOARD,
  CGAME_EVENT_EDITHUD
};


//================================
//
// CGAME API
//
//================================

typedef struct cgameImport_s {
	// common
	void			(*Print)						( const char *msg, ... );
	void			(*Error)						( int level, const char *error, ... );
	int				(*Milliseconds)					( void );

	// cvar
	void			(*Cvar_Register)				( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags, const char *description );
	void			(*Cvar_Update)					( vmCvar_t *vmCvar );
	void			(*Cvar_Set)						( const char *var_name, const char *value );
	void			(*Cvar_VariableStringBuffer)	( const char *var_name, char *buffer, int bufsize );

	// command
	int				(*Cmd_Argc)						( void );
	void			(*Cmd_Argv)						( int arg, char *buffer, int bufferLength );
	void			(*Cmd_Args)						( char *buffer, int bufferLength );
	void			(*SendConsoleCommand)			( const char *text );
	void			(*AddCommand)					( const char *cmd_name, void (*function)(void) );
	void			(*RemoveCommand)				( const char *cmd_name );
	void			(*SendClientCommand)			( const char *cmd );

	// filesystem
	int				(*FS_Open)						( const char *qpath, fileHandle_t *f, fsMode_t mode );
	int				(*FS_Read)						( void *buffer, int len, fileHandle_t f );
	int				(*FS_Write)						( const void *buffer, int len, fileHandle_t f );
	void			(*FS_Close)						( fileHandle_t f );
	int				(*FS_Seek)						( fileHandle_t f, long offset, int origin );

	// screen
	void			(*UpdateScreen)					( void );

	// clip model
	void			(*CM_LoadMap)					( const char *mapname );
	int				(*CM_NumInlineModels)			( void );
	clipHandle_t	(*CM_InlineModel)				( int index );
	clipHandle_t	(*CM_TempModel)					( const vector3 *mins, const vector3 *maxs, int capsule );
	void			(*CM_Trace)						( trace_t *results, const vector3 *start, const vector3 *end, const vector3 *mins, const vector3 *maxs, clipHandle_t model, int brushmask, int capsule );
	void			(*CM_TransformedTrace)			( trace_t *results, const vector3 *start, const vector3 *end, const vector3 *mins, const vector3 *maxs, clipHandle_t model, int brushmask, const vector3 *origin, const vector3 *angles, int capsule );
	int				(*CM_PointContents)				( const vector3 *p, clipHandle_t model );
	int				(*CM_TransformedPointContents)	( const vector3 *p, clipHandle_t model, const vector3 *origin, const vector3 *angles );
	
	// sound
	void			(*S_AddLoopingSound)			( int entityNum, const vector3 *origin, const vector3 *velocity, sfxHandle_t sfx );
	void			(*S_AddRealLoopingSound)		( int entityNum, const vector3 *origin, const vector3 *velocity, sfxHandle_t sfx );
	void			(*S_ClearLoopingSounds)			( qboolean killall );
	void			(*S_Respatialize)				( int entityNum, const vector3 *origin, vector3 axis[3], int inwater );
	sfxHandle_t		(*S_RegisterSound)				( const char *sample, qboolean compressed );
	void			(*S_StartSound)					( vector3 *origin, int entnum, int entchannel, sfxHandle_t sfx );
	void			(*S_StartBackgroundTrack)		( const char *intro, const char *loop );
	void			(*S_StartLocalSound)			( sfxHandle_t sfx, int channelNum );
	void			(*S_StopBackgroundTrack)		( void );
	void			(*S_StopLoopingSound)			( int entityNum );
	void			(*S_UpdateEntityPosition)		( int entityNum, const vector3 *origin );

	// renderer
	void			(*R_AddAdditiveLightToScene)	( const vector3 *org, float intensity, float r, float g, float b );
	void			(*R_AddLightToScene)			( const vector3 *org, float intensity, float r, float g, float b );
	void			(*R_AddPolysToScene)			( qhandle_t hShader, int numVerts, const polyVert_t *verts, int num );
	void			(*R_AddRefEntityToScene)		( const refEntity_t *re );
	void			(*R_ClearScene)					( void );
	void			(*R_DrawStretchPic)				( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
	void			(*R_DrawRotatedPic)				( float x, float y, float w, float h, float s1, float t1, float s2, float t2, float a, qboolean centered, qhandle_t hShader );
	qboolean		(*R_GetEntityToken)				( char *buffer, int size );
	qboolean		(*R_inPVS)						( const vector3 *p1, const vector3 *p2 );
	int				(*R_LerpTag)					( orientation_t *tag,  qhandle_t model, int startFrame, int endFrame, float frac, const char *tagName );
	int				(*R_LightForPoint)				( vector3 *point, vector3 *ambientLight, vector3 *directedLight, vector3 *lightDir );
	void			(*R_LoadWorld)					( const char *name );
	int				(*R_MarkFragments)				( int numPoints, const vector3 *points, const vector3 *projection, int maxPoints, vector3 *pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );
	void			(*R_ModelBounds)				( qhandle_t model, vector3 *mins, vector3 *maxs );
	void			(*R_RegisterFont)				( const char *fontName, int pointSize, fontInfo_t *font );
	qhandle_t		(*R_RegisterModel)				( const char *name );
	qhandle_t		(*R_RegisterSkin)				( const char *name );
	qhandle_t		(*R_RegisterShader)				( const char *name );
	qhandle_t		(*R_RegisterShaderNoMip)		( const char *name );
	void			(*R_RemapShader)				( const char *oldShader, const char *newShader, const char *offsetTime );
	void			(*R_RenderScene)				( const refdef_t *fd );
	void			(*R_SetColor)					( const vector4 *rgba );

	// misc
	void			(*GetGLConfig)					( glconfig_t *glconfig );
	void			(*GetGameState)					( gameState_t *gs );
	void			(*GetCurrentSnapshotNumber)		( int *snapshotNumber, int *serverTime );
	qboolean		(*GetSnapshot)					( int snapshotNumber, snapshot_t *snapshot );
	qboolean		(*GetServerCommand)				( int serverCommandNumber );
	int				(*GetCurrentCmdNumber)			( void );
	qboolean		(*GetUserCmd)					( int cmdNumber, usercmd_t *ucmd );
	void			(*SetUserCmdValue)				( int userCmdValue, float sensitivityScale );
	int				(*MemoryRemaining)				( void );
	int				(*RealTime)						( qtime_t *qtime );

	// keys
	qboolean		(*Key_IsDown)					( int keynum );
	int				(*Key_GetCatcher)				( void );
	void			(*Key_SetCatcher)				( int catcher );
	int				(*Key_GetKey)					( const char *binding );

	// preprocessor, imported from botlib
	int				(*PC_AddGlobalDefine)			( char *string );
	int				(*PC_LoadSourceHandle)			( const char *filename );
	int				(*PC_FreeSourceHandle)			( int handle );
	int				(*PC_ReadTokenHandle)			( int handle, pc_token_t *pc_token );
	int				(*PC_SourceFileAndLine)			( int handle, char *filename, int *line );

	// cinematic
	int				(*CIN_PlayCinematic)			( const char *arg0, int xpos, int ypos, int width, int height, int bits );
	e_status		(*CIN_StopCinematic)			( int handle );
	e_status		(*CIN_RunCinematic)				( int handle );
	void			(*CIN_DrawCinematic)			( int handle );
	void			(*CIN_SetExtents)				( int handle, int x, int y, int w, int h );
} cgameImport_t;

typedef struct cgameExport_s {
	void		(*Init)				( int serverMessageNum, int serverCommandSequence, int clientNum );
	void		(*Shutdown)			( void );
	qboolean	(*ConsoleCommand)	( void );
	void		(*DrawActiveFrame)	( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
	int			(*CrosshairPlayer)	( void );
	int			(*LastAttacker)		( void );
	void		(*KeyEvent)			( int key, qboolean down ); 
	void		(*MouseEvent)		( int dx, int dy );
	void		(*EventHandling)	( int type );
} cgameExport_t;

//linking of cgame library
typedef cgameExport_t* (QDECL *GetCGameAPI_t)( int apiVersion, cgameImport_t *import );
