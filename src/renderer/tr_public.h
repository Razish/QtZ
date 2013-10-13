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
#ifndef __TR_PUBLIC_H
#define __TR_PUBLIC_H

#include "tr_types.h"
#ifdef USE_LOCAL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

//QtZ: Raw mouse input from https://github.com/runaos/iodfe/commit/977f61eca82e9c06869b647d752bf9bf2684b877
#ifdef _WIN32
	#ifdef USE_LOCAL_HEADERS
		#include "SDL_syswm.h"
	#else
		#include <SDL_syswm.h>
	#endif
#endif

#define	REF_API_VERSION		8

//
// these are the functions exported by the refresh module
//
typedef struct refexport_s {
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = qfalse,
	// which will keep the screen from flashing to the desktop.
	void	(*Shutdown)( qboolean destroyWindow );

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// BeginRegistration makes any existing media pointers invalid
	// and returns the current gl configuration, including screen width
	// and height, which can be used by the client to intelligently
	// size display elements
	void	(*BeginRegistration)( glconfig_t *config );
	qhandle_t (*RegisterModel)( const char *name );
	qhandle_t (*RegisterSkin)( const char *name );
	qhandle_t (*RegisterShader)( const char *name );
	qhandle_t (*RegisterShaderNoMip)( const char *name );
	void	(*LoadWorld)( const char *name );

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void	(*SetWorldVisData)( const byte *vis );

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void	(*EndRegistration)( void );

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void	(*ClearScene)( void );
	void	(*AddRefEntityToScene)( const refEntity_t *re );
	void	(*AddPolyToScene)( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
	int		(*LightForPoint)( vector3 *point, vector3 *ambientLight, vector3 *directedLight, vector3 *lightDir );
	void	(*AddLightToScene)( const vector3 *org, float intensity, float r, float g, float b );
	void	(*AddAdditiveLightToScene)( const vector3 *org, float intensity, float r, float g, float b );
	void	(*RenderScene)( const refdef_t *fd );

	void	(*SetColor)( const vector4 *rgba );	// NULL = 1,1,1,1
	void	(*DrawStretchPic) ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );	// 0 = white
	//QtZ: Added from JA/EF
	void	(*DrawRotatedPic) ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, float a, qboolean centered, qhandle_t hShader );  // 0 = white
	//~QtZ

	void	(*BeginFrame)( stereoFrame_t stereoFrame );

	// if the pointers are not NULL, timing info will be returned
	void	(*EndFrame)( int *frontEndMsec, int *backEndMsec );


	int		(*MarkFragments)( int numPoints, const vector3 *points, const vector3 *projection, int maxPoints, vector3 *pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

	int		(*LerpTag)( orientation_t *tag,  qhandle_t model, int startFrame, int endFrame, float frac, const char *tagName );
	void	(*ModelBounds)( qhandle_t model, vector3 *mins, vector3 *maxs );

#ifdef __USEA3D
	void    (*A3D_RenderGeometry) (void *pVoidA3D, void *pVoidGeom, void *pVoidMat, void *pVoidGeomStatus);
#endif
	void	(*RegisterFont)(const char *fontName, int pointSize, fontInfo_t *font);
	qboolean (*GetEntityToken)( char *buffer, size_t size );
	qboolean (*inPVS)( const vector3 *p1, const vector3 *p2 );

	void (*TakeVideoFrame)( int h, int w, byte* captureBuffer, byte *encodeBuffer, qboolean motionJpeg );

} refexport_t;

//
// these are the functions imported by the refresh module
//
typedef struct refimport_s {
	// print message on the local console
	void	(QDECL *Printf)( int printLevel, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

	// abort the game
	void	(QDECL *Error)( int errorLevel, const char *fmt, ...) __attribute__ ((noreturn, format (printf, 2, 3)));

	// milliseconds should only be used for profiling, never
	// for anything game related.  Get time from the refdef
	int		(*Milliseconds)( void );

	// stack based memory allocation for per-level things that
	// won't be freed
#ifdef HUNK_DEBUG
	void	*(*Hunk_AllocDebug)( size_t size, hunkallocPref_t pref, char *label, char *file, int line );
#else
	void	*(*Hunk_Alloc)( size_t size, hunkallocPref_t pref );
#endif
	void	*(*Hunk_AllocateTempMemory)( size_t size );
	void	(*Hunk_FreeTempMemory)( void *block );

	// dynamic memory allocator for things that need to be freed
	void	*(*Malloc)( int bytes );
	void	(*Free)( void *buf );

	cvar_t	*(*Cvar_Get)( const char *name, const char *value, int flags, const char *description, void (*update)( void ) );
	void	(*Cvar_Set)( const char *name, const char *value );
	void	(*Cvar_SetValue) (const char *name, float value);
	void	(*Cvar_CheckRange)( cvar_t *cv, float minVal, float maxVal, qboolean shouldBeIntegral );

	int		(*Cvar_VariableIntegerValue) (const char *name);

	void	(*Cmd_AddCommand)( const char *name, xcommand_t cmd, completionFunc_t complete );
	void	(*Cmd_RemoveCommand)( const char *name );

	int		(*Cmd_Argc)( void );
	const char *(*Cmd_Argv) (int i);

	void	(*Cmd_ExecuteText) (int exec_when, const char *text);

	byte	*(*CM_ClusterPVS)(int cluster);

	// visualization for debugging collision detection
	void	(*CM_DrawDebugSurface)( void (*drawPoly)(int color, int numPoints, vector3 *points) );

	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int		(*FS_FileIsInPAK)( const char *name, int *pCheckSum );
	long		(*FS_ReadFile)( const char *name, void **buf );
	void	(*FS_FreeFile)( void *buf );
	char **	(*FS_ListFiles)( const char *name, const char *extension, size_t *numfilesfound );
	void	(*FS_FreeFileList)( char **filelist );
	void	(*FS_WriteFile)( const char *qpath, const void *buffer, int size );
	int		(*FS_Write)( const void *buffer, int len, fileHandle_t f );
	fileHandle_t (*FS_FOpenFileWrite)( const char *qpath );
	void	(*FS_FCloseFile)( fileHandle_t f );
	qboolean (*FS_FileExists)( const char *file );

	void	(*CL_WriteAVIVideoFrame)( const byte *buffer, int size );

	// input event handling
	void	(*IN_Init)( void );
	void	(*IN_Shutdown)( void );
	void	(*IN_Restart)( void );

	// math
	long    (*ftol)(float f);

	// system stuff
	void	(*Sys_SetEnv)( const char *name, const char *value );
	void	(*Sys_GLimpSafeInit)( void );
	void	(*Sys_GLimpInit)( void );
	qboolean (*Sys_LowPhysicalMemory)( void );
} refimport_t;


// this is the only function actually exported at the linker level
// If the module can't init to a valid rendering state, NULL will be
// returned.
typedef	refexport_t* (QDECL *GetRefAPI_t) (int apiVersion, refimport_t * rimp);

#endif	// __TR_PUBLIC_H
