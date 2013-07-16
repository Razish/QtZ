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
// sv_public.h

#define	SERVER_API_VERSION	1

typedef struct serverImport_s {
	void			(*Error)							( int code, const char *fmt, ... );
	void			(*Print)							( const char *fmt, ... );
	void			(*Cbuf_AddText)						( const char *text );
	void			(*Cbuf_ExecuteText)					( int exec_when, const char *text );
	void			(*CL_Disconnect)					( qboolean showMainMenu );
	void			(*CL_MapLoading)					( void );
	void			(*CL_StartHunkUsers)				( qboolean rendererOnly );
	void			(*CL_ShutdownAll)					( qboolean shutdownRef );
	void			(*CM_AdjustAreaPortalState)			( int area1, int area2, qboolean open );
	qboolean		(*CM_AreasConnected)				( int area1, int area2 );
	int				(*CM_BoxLeafnums)					( const vector3 *mins, const vector3 *maxs, int *list, int listsize, int *lastLeaf );
	void			(*CM_BoxTrace)						( trace_t *results, const vector3 *start, const vector3 *end, const vector3 *mins, const vector3 *maxs, clipHandle_t model, int brushmask, int capsule );
	void			(*CM_ClearMap)						( void );
	byte *			(*CM_ClusterPVS)					( int cluster );
	char *			(*CM_EntityString)					( void );
	clipHandle_t	(*CM_InlineModel)					( int index );
	int				(*CM_LeafArea)						( int leafnum );
	int				(*CM_LeafCluster)					( int leafnum );
	void			(*CM_LoadMap)						( const char *name, qboolean clientload, int *checksum );
	void			(*CM_ModelBounds)					( clipHandle_t model, vector3 *mins, vector3 *maxs );
	int				(*CM_PointContents)					( const vector3 *p, clipHandle_t model );
	int				(*CM_PointLeafnum)					( const vector3 *p );
	clipHandle_t	(*CM_TempBoxModel)					( const vector3 *mins, const vector3 *maxs, int capsule );
	void			(*CM_TransformedBoxTrace)			( trace_t *results, const vector3 *start, const vector3 *end, vector3 *mins, vector3 *maxs, clipHandle_t model, int brushmask, const vector3 *origin, const vector3 *angles, int capsule );
	int				(*CM_TransformedPointContents)		( const vector3 *p, clipHandle_t model, const vector3 *origin, const vector3 *angles );
	int				(*CM_WriteAreaBits)					( byte *buffer, int area );
	void			(*Cmd_AddCommand)					( const char *cmd_name, xcommand_t function );
	int				(*Cmd_Argc)							( void );
	char *			(*Cmd_Args)							( void );
	char *			(*Cmd_ArgsFrom)						( int arg );
	void			(*Cmd_Args_Sanitize)				( void );
	char *			(*Cmd_Argv)							( int arg );
	void			(*Cmd_ArgvBuffer)					( int arg, char *buffer, int bufferLength );
	char *			(*Cmd_Cmd)							( void );
	void			(*Cmd_ExecuteString)				( const char *text );
	void			(*Cmd_SetCommandCompletionFunc)		( const char *command, completionFunc_t complete );
	void			(*Cmd_TokenizeString)				( const char *text );
	void			(*Cmd_TokenizeStringIgnoreQuotes)	( const char *text_in );
	void			(*Com_BeginRedirect)				( char *buffer, int buffersize, void (*flush)(char *) );
	void			(*Com_EndRedirect)					( void );
	int				(*Com_Milliseconds)					( void );
	int				(*Com_RealTime)						( qtime_t *qtime );
	char *			(*CopyString)						( const char *in );
	void			(*Cvar_CheckRange)					( cvar_t *var, float min, float max, qboolean integral );
	cvar_t *		(*Cvar_Get)							( const char *name, const char *value, int flags, const char *description );
	char *			(*Cvar_InfoString)					( int bit );
	char *			(*Cvar_InfoString_Big)				( int bit );
	void			(*Cvar_Register)					( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags, const char *description );
	void			(*Cvar_Set)							( const char *var_name, const char *value );
	void			(*Cvar_SetSafe)						( const char *var_name, const char *value );
	void			(*Cvar_Update)						( vmCvar_t *vmCvar );
	int				(*Cvar_VariableIntegerValue)		( const char *var_name );
	char *			(*Cvar_VariableString)				( const char *var_name );
	void			(*Cvar_VariableStringBuffer)		( const char *var_name, char *buffer, int bufsize );
	float			(*Cvar_VariableValue)				( const char *var_name );
	void			(*Field_CompleteFilename)			( const char *dir, const char *filename, const char *ext, qboolean stripExt, qboolean allowNonPureFilesOnDisk );
	void			(*FS_ClearPakReferences)			( int flags );
	void			(*FS_FCloseFile)					( fileHandle_t f );
	int				(*FS_FileIsInPAK)					( const char *filename, int *pChecksum );
	qboolean		(*FS_FilenameCompare)				( const char *s1, const char *s2 );
	int				(*FS_FOpenFileByMode)				( const char *qpath, fileHandle_t *f, fsMode_t mode );
	long			(*FS_FOpenFileRead)					( const char *qpath, fileHandle_t *file, qboolean uniqueFILE );
	const char *	(*FS_GetCurrentGameDir)				( void );
	int				(*FS_GetFileList)					( const char *path, const char *extension, char *listbuf, int bufsize );
	const char *	(*FS_LoadedPakChecksums)			( void );
	const char *	(*FS_LoadedPakNames)				( void );
	const char *	(*FS_LoadedPakPureChecksums)		( void );
	int				(*FS_Read)							( void *buffer, int len, fileHandle_t f );
	int				(*FS_Read2)							( void *buffer, int len, fileHandle_t f );
	long			(*FS_ReadFile)						( const char *qpath, void **buffer );
	const char *	(*FS_ReferencedPakChecksums)		( void );
	const char *	(*FS_ReferencedPakNames)			( void );
	void			(*FS_Restart)						( int checksumFeed );
	int				(*FS_Seek)							( fileHandle_t f, long offset, int origin );
	long			(*FS_SV_FOpenFileRead)				( const char *filename, fileHandle_t *fp );
	fileHandle_t	(*FS_SV_FOpenFileWrite)				( const char *filename );
	int				(*FS_Write)							( const void *buffer, int len, fileHandle_t f );
	void			(*Huff_Decompress)					( msg_t *buf, int offset );
	void *			(*Hunk_AllocateTempMemory)			( int size );
#ifdef HUNK_DEBUG
	void *			(*Hunk_AllocDebug)					( int size, ha_pref pref, char *label, char *file, int line );
#else
	void *			(*Hunk_Alloc)						( int size, ha_pref preference );
#endif
	qboolean		(*Hunk_CheckMark)					( void );
	void			(*Hunk_Clear)						( void );
	void			(*Hunk_FreeTempMemory)				( void *buf );
	void			(*Hunk_SetMark)						( void );
	void			(*Info_Print)						( const char *s );
	void			(*Info_SetValueForKey)				( char *s, const char *key, const char *value );
	char *			(*Info_ValueForKey)					( const char *s, const char *key );
	void			(*MSG_Init)							( msg_t *buf, byte *data, int length );
	void			(*MSG_BeginReadingOOB)				( msg_t *sb );
	void			(*MSG_Bitstream)					( msg_t *buf );
	void			(*MSG_Clear)						( msg_t *buf );
	void			(*MSG_Copy)							( msg_t *buf, byte *data, int length, msg_t *src );
	int				(*MSG_HashKey)						( const char *string, int maxlen );
	int				(*MSG_ReadByte)						( msg_t *sb );
	void			(*MSG_ReadDeltaUsercmdKey)			( msg_t *msg, int key, usercmd_t *from, usercmd_t *to );
	int				(*MSG_ReadLong)						( msg_t *sb );
	int				(*MSG_ReadShort)					( msg_t *sb );
	char *			(*MSG_ReadString)					( msg_t *sb );
	char *			(*MSG_ReadStringLine)				( msg_t *sb );
	void			(*MSG_WriteBigString)				( msg_t *sb, const char *s );
	void			(*MSG_WriteBits)					( msg_t *msg, int value, int bits );
	void			(*MSG_WriteByte)					( msg_t *sb, int c );
	void			(*MSG_WriteData)					( msg_t *buf, const void *data, int length );
	void			(*MSG_WriteDeltaEntity)				( msg_t *msg, struct entityState_s *from, struct entityState_s *to, qboolean force );
	void			(*MSG_WriteDeltaPlayerstate)		( msg_t *msg, struct playerState_s *from, struct playerState_s *to );
	void			(*MSG_WriteLong)					( msg_t *sb, int c );
	void			(*MSG_WriteShort)					( msg_t *sb, int c );
	void			(*MSG_WriteString)					( msg_t *sb, const char *s );
	qboolean		(*Netchan_Process)					( netchan_t *chan, msg_t *msg );
	void			(*Netchan_Setup)					( netsrc_t sock, netchan_t *chan, netadr_t adr, int qport, int challenge, qboolean compat );
	const char *	(*NET_AdrToString)					( netadr_t a );
	const char *	(*NET_AdrToStringwPort)				( netadr_t a );
	qboolean		(*NET_CompareAdr)					( netadr_t a, netadr_t b );
	qboolean		(*NET_CompareBaseAdr)				( netadr_t a, netadr_t b );
	qboolean		(*NET_CompareBaseAdrMask)			( netadr_t a, netadr_t b, int netmask );
	void			(*NET_Init)							( void );
	qboolean		(*NET_IsLocalAddress)				( netadr_t adr );
	void			(*NET_JoinMulticast6)				( void );
	void			(*NET_LeaveMulticast6)				( void );
	void			(*NET_OutOfBandPrint)				( netsrc_t net_socket, netadr_t adr, const char *format, ... );
	int				(*NET_StringToAdr)					( const char *s, netadr_t *a, netadrtype_t family );
	void			(*Netchan_Transmit)					( netchan_t *chan, int length, const byte *data );
	void			(*Netchan_TransmitNextFragment)		( netchan_t *chan );
	qboolean		(*Sys_IsLANAddress)					( netadr_t adr );
	int				(*Sys_Milliseconds)					( void );
	void *			(*Sys_LoadDll)						( const char *name, qboolean useSystemLib );
	void			(*Sys_UnloadDll)					( void *dllHandle );
	void			(*Sys_Sleep)						( int msec );
	int				(*Z_AvailableMemory)				( void );
	void			(*Z_Free)							( void *ptr );
#ifdef ZONE_DEBUG
	void *			(*Z_MallocDebug)					( int size, char *label, char *file, int line );
	void *			(*Z_TagMallocDebug)					( int size, int tag, char *label, char *file, int line );
#else
	void *			(*Z_Malloc)							( int size );
	void *			(*Z_TagMalloc)						( int size, int tag );
#endif

	int *			cvar_modifiedFlags;
	int *			time_game;
	qboolean *		com_errorEntered;
	int *			com_frameTime;
} serverImport_t;

typedef struct serverExport_s {
	void		(*Shutdown)				( char *finalmsg );
	void		(*Frame)				( int msec );
	void		(*PacketEvent)			( netadr_t from, msg_t *msg );
	int			(*FrameMsec)			( void );
	qboolean	(*GameCommand)			( void );
	int			(*SendQueuedPackets)	( void );
	void		(*BotDrawDebugPolygons)	( void (*drawPoly)(int color, int numPoints, vector3 *points), int value );
	void		(*ShutdownGameProgs)	( void );
} serverExport_t;
extern serverExport_t sve;

typedef serverExport_t* (QDECL *GetServerAPI_t)( int apiVersion, serverImport_t *import );
