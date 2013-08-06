#pragma once

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

// g_public.h -- game module information visible to server

#define	GAME_API_VERSION	1

// entity->svFlags
// the server does not know how to interpret most of the values
// in entityStates (level eType), so the game must explicitly flag
// special server behaviors
#define	SVF_NOCLIENT			0x00000001	// don't send entity to clients, even if it has effects

// TTimo
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=551
#define SVF_CLIENTMASK 0x00000002

#define SVF_BOT					0x00000008	// set if the entity is a bot
#define	SVF_BROADCAST			0x00000020	// send to all connected clients
#define	SVF_PORTAL				0x00000040	// merge a second pvs at origin2 into snapshots
#define	SVF_USE_CURRENT_ORIGIN	0x00000080	// entity->r.currentOrigin instead of entity->s.origin
											// for link position (missiles and movers)
#define SVF_SINGLECLIENT		0x00000100	// only send to a single client (entityShared_t->singleClient)
#define SVF_NOSERVERINFO		0x00000200	// don't send CS_SERVERINFO updates to this client
											// so that it can be updated for ping tools without
											// lagging clients
#define SVF_CAPSULE				0x00000400	// use capsule for collision detection instead of bbox
#define SVF_NOTSINGLECLIENT		0x00000800	// send entity to everyone but one client
											// (entityShared_t->singleClient)



//===============================================================


typedef struct {
	entityState_t	unused;			// apparently this field was put here accidentally
									//  (and is kept only for compatibility, as a struct pad)

	qboolean	linked;				// qfalse if not in any good cluster
	int			linkcount;

	int			svFlags;			// SVF_NOCLIENT, SVF_BROADCAST, etc

	// only send to this client when SVF_SINGLECLIENT is set	
	// if SVF_CLIENTMASK is set, use bitmask for clients to send to (maxclients must be <= 32, up to the mod to enforce this)
	int			singleClient;		

	qboolean	bmodel;				// if false, assume an explicit mins / maxs bounding box
									// only set by trap->SV_SetBrushModel
	vector3		mins, maxs;
	int			contents;			// CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc
									// a non-solid entity should set to 0

	vector3		absmin, absmax;		// derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vector3		currentOrigin;
	vector3		currentAngles;

	// when a trace call is made and passEntityNum != ENTITYNUM_NONE,
	// an ent will be excluded from testing if:
	// ent->s.number == passEntityNum	(don't interact with self)
	// ent->r.ownerNum == passEntityNum	(don't interact with your own missiles)
	// entity[ent->r.ownerNum].r.ownerNum == passEntityNum	(don't interact with other missiles from owner)
	int			ownerNum;
} entityShared_t;



// the server looks at a sharedEntity, which is the start of the game's gentity_t structure
typedef struct {
	entityState_t	s;				// communicated by server to clients
	entityShared_t	r;				// shared by both the server system and game
} sharedEntity_t;


//================================
//
// GAME API
//
//================================

#include "../botlib/botlib.h"

typedef struct gameImport_s {
	// common
	void			(*Print)						( const char *msg, ... );
	void			(*Error)						( int level, const char *error, ... );
	int				(*Milliseconds)					( void );
	// cvar
	void			(*Cvar_Register)				( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags, const char *description );
	void			(*Cvar_Update)					( vmCvar_t *vmCvar );
	void			(*Cvar_Set)						( const char *var_name, const char *value );
	int				(*Cvar_VariableIntegerValue)	( const char *var_name );
	void			(*Cvar_VariableStringBuffer)	( const char *var_name, char *buffer, int bufsize );

	// command
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

	// server
	void			(*SV_LocateGameData)			( sharedEntity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient );
	void			(*SV_GameDropClient)			( int clientNum, const char *reason );
	void			(*SV_GameSendServerCommand)		( int clientNum, const char *text );
	void			(*SV_SetConfigstring)			( int index, const char *val );
	void			(*SV_GetConfigstring)			( int index, char *buffer, int bufferSize );
	void			(*SV_GetUserinfo)				( int index, char *buffer, int bufferSize );
	void			(*SV_SetUserinfo)				( int index, const char *val );
	void			(*SV_GetServerinfo)				( char *buffer, int bufferSize );
	void			(*SV_SetBrushModel)				( sharedEntity_t *ent, const char *name );
	void			(*SV_Trace)						( trace_t *results, const vector3 *start, const vector3 *mins, const vector3 *maxs, const vector3 *end, int passEntityNum, int contentmask );
	qboolean		(*SV_EntityContact)				( vector3 *mins, vector3 *maxs, const sharedEntity_t *gEnt, int capsule );
	int				(*SV_PointContents)				( const vector3 *p, int passEntityNum );
	qboolean		(*SV_InPVS)						( const vector3 *p1, const vector3 *p2 );
	qboolean		(*SV_InPVSIgnorePortals)		( const vector3 *p1, const vector3 *p2 );
	void			(*SV_AdjustAreaPortalState)		( sharedEntity_t *ent, qboolean open );
	qboolean		(*CM_AreasConnected)			( int area1, int area2 );
	void			(*SV_LinkEntity)				( sharedEntity_t *gEnt );
	void			(*SV_UnlinkEntity)				( sharedEntity_t *gEnt );
	int				(*SV_AreaEntities)				( const vector3 *mins, const vector3 *maxs, int *entityList, int maxcount );
	int				(*SV_BotAllocateClient)			( void );
	void			(*SV_BotFreeClient)				( int clientNum );
	void			(*SV_GetUsercmd)				( int clientNum, usercmd_t *cmd );
	qboolean		(*SV_GetEntityToken)			( char *buffer, int bufferSize );

	int				(*DebugPolygonCreate)			( int color, int numPoints, vector3 *points );
	void			(*DebugPolygonDelete)			( int id );
	int				(*RealTime)						( qtime_t *qtime );

	int				(*SV_BotLibSetup)				( void );
	int				(*SV_BotLibShutdown)			( void );
	int				(*BotLibVarSet)					( char *var_name, char *value );
	int				(*BotLibVarGet)					( char *var_name, char *value, int size );
	int				(*PC_AddGlobalDefine)			( char *string );
	int				(*PC_LoadSourceHandle)			( const char *filename );
	int				(*PC_FreeSourceHandle)			( int handle );
	int				(*PC_ReadTokenHandle)			( int handle, pc_token_t *pc_token );
	int				(*PC_SourceFileAndLine)			( int handle, char *filename, int *line );

	int				(*BotLibStartFrame)				( float time );
	int				(*BotLibLoadMap)				( const char *mapname );
	int				(*BotLibUpdateEntity)			( int ent, bot_entitystate_t *state );
	int				(*BotLibTest)					( int parm0, char *parm1, vector3 *parm2, vector3 *parm3 );

	int				(*SV_BotGetSnapshotEntity)		( int client, int sequence );
	int				(*SV_BotGetConsoleMessage)		( int client, char *buf, int size );
	void			(*SV_ClientThink)				( int clientNum, usercmd_t *cmd );

	aas_export_t	*aas;	// area awareness system
	ea_export_t		*ea;	// elementary action
	ai_export_t		*ai;	// artificial intelligence
} gameImport_t;


//
// functions exported by the game subsystem
//
typedef struct gameExport_s {
	void		(*Init)						( int levelTime, int randomSeed, qboolean restart );
	void		(*Shutdown)					( qboolean restart );
	char *		(*ClientConnect)			( int clientNum, qboolean firstTime, qboolean isBot );
	void		(*ClientBegin)				( int clientNum );
	void		(*ClientUserinfoChanged)	( int clientNum );
	void		(*ClientDisconnect)			( int clientNum );
	void		(*ClientCommand)			( int clientNum );
	void		(*ClientThink)				( int clientNum );
	void		(*RunFrame)					( int levelTime );
	qboolean	(*ConsoleCommand)			( void );

	int			(*BotAIStartFrame)			( int time );
} gameExport_t;

//linking of game library
typedef gameExport_t* (QDECL *GetGameAPI_t)( int apiVersion, gameImport_t *import );
