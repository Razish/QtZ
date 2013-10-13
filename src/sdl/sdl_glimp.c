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

#ifdef USE_LOCAL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "tr_local.h"
#include "../client/client.h"
#include "../sys/sys_local.h"
#include "sdl_icon.h"

/* Just hack it for now. */
#ifdef MACOS_X
#include <OpenGL/OpenGL.h>
typedef CGLContextObj QGLContext;
#define GLimp_GetCurrentContext() CGLGetCurrentContext()
#define GLimp_SetCurrentContext(ctx) CGLSetCurrentContext(ctx)
#else
typedef void *QGLContext;
#define GLimp_GetCurrentContext() (NULL)
#define GLimp_SetCurrentContext(ctx)
#endif

static QGLContext opengl_context;

typedef enum rserr_e {
	RSERR_OK=0,
	RSERR_INVALID_MODE,
	RSERR_UNKNOWN
} rserr_t;

static SDL_Surface *screen = NULL;
static const SDL_VideoInfo *videoInfo = NULL;

cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
cvar_t *vid_centerWindow;
cvar_t *r_sdlDriver;

//QtZ: in_disableLockKeys, vid_xpos, vid_ypos from iodfe
cvar_t *vid_width, *vid_height;
cvar_t *vid_xpos, *vid_ypos;
cvar_t *in_disableLockKeys = NULL;
//~QtZ

void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);

void (APIENTRYP qglLockArraysEXT) (GLint first, GLsizei count);
void (APIENTRYP qglUnlockArraysEXT)( void );

void GLimp_Shutdown( void )
{
	ri->IN_Shutdown();

	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	screen = NULL;

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );
}

// Minimize the game so that user is back at the desktop
void GLimp_Minimize( void )
{
	SDL_WM_IconifyWindow();
}


void GLimp_LogComment( const char *comment )
{
}

static int GLimp_CompareModes( const void *a, const void *b )
{
	const float ASPECT_EPSILON = 0.001f;
	SDL_Rect *modeA = *(SDL_Rect **)a;
	SDL_Rect *modeB = *(SDL_Rect **)b;
	float aspectA = (float)modeA->w / (float)modeA->h;
	float aspectB = (float)modeB->w / (float)modeB->h;
	int areaA = modeA->w * modeA->h;
	int areaB = modeB->w * modeB->h;
	float aspectDiffA = fabsf( aspectA - displayAspect );
	float aspectDiffB = fabsf( aspectB - displayAspect );
	float aspectDiffsDiff = aspectDiffA - aspectDiffB;

	if( aspectDiffsDiff > ASPECT_EPSILON )
		return 1;
	else if( aspectDiffsDiff < -ASPECT_EPSILON )
		return -1;
	else
		return areaA - areaB;
}

static void GLimp_DetectAvailableModes( void )
{
	char buf[ MAX_STRING_CHARS ] = { 0 };
	SDL_Rect **modes;
	int numModes;
	int i;

	modes = SDL_ListModes( videoInfo->vfmt, SDL_OPENGL | SDL_FULLSCREEN );

	if( !modes )
	{
		ri->Printf( PRINT_WARNING, "Can't get list of available modes\n" );
		return;
	}

	if( modes == ((SDL_Rect **)-1) )
	{
		ri->Printf( PRINT_ALL, "Display supports any resolution\n" );
		return; // can set any resolution
	}

	for( numModes = 0; modes[ numModes ]; numModes++ );

	if( numModes > 1 )
		qsort( modes, numModes, sizeof( SDL_Rect* ), GLimp_CompareModes );

	for( i = 0; i < numModes; i++ )
	{
		const char *newModeString = va( "%ux%u ", modes[ i ]->w, modes[ i ]->h );

		if( strlen( newModeString ) < (int)sizeof( buf ) - strlen( buf ) )
			Q_strcat( buf, sizeof( buf ), newModeString );
		else
			ri->Printf( PRINT_WARNING, "Skipping mode %ux%x, buffer too small\n", modes[i]->w, modes[i]->h );
	}

	if( *buf )
	{
		buf[ strlen( buf ) - 1 ] = 0;
		ri->Printf( PRINT_ALL, "Available modes: '%s'\n", buf );
		ri->Cvar_Set( "r_availableModes", buf );
	}
}

static int GLimp_SetMode( qboolean fullscreen, qboolean noborder ) {
	const char *glstring;
	int colorbits, depthbits, stencilbits;
	int tcolorbits, tdepthbits, tstencilbits;
	int sdlcolorbits, samples, i=0;
	SDL_Surface *vidscreen = NULL;
	Uint32 flags = SDL_OPENGL|SDL_RESIZABLE;

	ri->Printf( PRINT_ALL, "Initializing OpenGL display\n");

	if ( videoInfo == NULL ) {
		static SDL_VideoInfo sVideoInfo;
		static SDL_PixelFormat sPixelFormat;

		videoInfo = SDL_GetVideoInfo();

		// Take a copy of the videoInfo
		memcpy( &sPixelFormat, videoInfo->vfmt, sizeof( SDL_PixelFormat ) );
		sPixelFormat.palette = NULL; // Should already be the case
		memcpy( &sVideoInfo, videoInfo, sizeof( SDL_VideoInfo ) );
		sVideoInfo.vfmt = &sPixelFormat;
		videoInfo = &sVideoInfo;

		if ( videoInfo->current_h > 0 ) {
			// Guess the display aspect ratio through the desktop resolution by assuming (relatively safely)
			//	that it is set at or close to the display's native aspect ratio
			displayAspect = (float)videoInfo->current_w / (float)videoInfo->current_h;

			ri->Printf( PRINT_ALL, "Estimated display aspect: %.3f\n", displayAspect );
		}
		else
			ri->Printf( PRINT_ALL, "Cannot estimate display aspect, assuming 1.333\n" );
	}

	if ( vid_width->integer == 0 && vid_height->integer == 0 ) {
		// use desktop video resolution
		if ( videoInfo->current_h > 0 ) {
			glConfig.vidWidth = videoInfo->current_w;
			glConfig.vidHeight = videoInfo->current_h;
		}
		else {
			glConfig.vidWidth = 640;
			glConfig.vidHeight = 480;
			ri->Printf( PRINT_ALL, "Cannot determine display resolution, assuming 640x480\n" );
		}

		glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
	}
	else {
		glConfig.vidWidth = vid_width->integer;
		glConfig.vidHeight = vid_height->integer;
		glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
	}

	ri->Printf( PRINT_ALL, "setting resolution: %d %d\n", glConfig.vidWidth, glConfig.vidHeight);

	if ( fullscreen ) {
		flags |= SDL_FULLSCREEN;
		glConfig.isFullscreen = qtrue;
	}
	else {
		if ( noborder )
			flags |= SDL_NOFRAME;

		glConfig.isFullscreen = qfalse;
	}

	colorbits = r_colorbits->integer;
	if ( (!colorbits) || (colorbits >= 32) )
		colorbits = 24;

	if ( !r_depthbits->integer )
		depthbits = 24;
	else
		depthbits = r_depthbits->integer;
	stencilbits = r_stencilbits->integer;
	samples = r_ext_multisample->integer;

	for ( i = 0; i < 16; i++ )
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if ((i % 4) == 0 && i)
		{
			// one pass, reduce
			switch (i / 4)
			{
				case 2 :
					if (colorbits == 24)
						colorbits = 16;
					break;
				case 1 :
					if (depthbits == 24)
						depthbits = 16;
					else if (depthbits == 16)
						depthbits = 8;
				case 3 :
					if (stencilbits == 24)
						stencilbits = 16;
					else if (stencilbits == 16)
						stencilbits = 8;
			}
		}

		tcolorbits = colorbits;
		tdepthbits = depthbits;
		tstencilbits = stencilbits;

		if ((i % 4) == 3)
		{ // reduce colorbits
			if (tcolorbits == 24)
				tcolorbits = 16;
		}

		if ((i % 4) == 2)
		{ // reduce depthbits
			if (tdepthbits == 24)
				tdepthbits = 16;
			else if (tdepthbits == 16)
				tdepthbits = 8;
		}

		if ((i % 4) == 1)
		{ // reduce stencilbits
			if (tstencilbits == 24)
				tstencilbits = 16;
			else if (tstencilbits == 16)
				tstencilbits = 8;
			else
				tstencilbits = 0;
		}

		sdlcolorbits = 4;
		if (tcolorbits == 24)
			sdlcolorbits = 8;

#ifdef __sgi /* Fix for SGIs grabbing too many bits of color */
		if (sdlcolorbits == 4)
			sdlcolorbits = 0; /* Use minimum size for 16-bit color */

		/* Need alpha or else SGIs choose 36+ bit RGB mode */
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 1);
#endif

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, sdlcolorbits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, sdlcolorbits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, sdlcolorbits );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, tdepthbits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, tstencilbits );

		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, samples );

		if(r_stereoEnabled->integer)
		{
			glConfig.stereoEnabled = qtrue;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 1);
		}
		else
		{
			glConfig.stereoEnabled = qfalse;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
		}
		
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

#if 0 // See http://bugzilla.icculus.org/show_bug.cgi?id=3526
		// If not allowing software GL, demand accelerated
		if( !r_allowSoftwareGL->integer )
		{
			if( SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 ) < 0 )
			{
				ri->Printf( PRINT_ALL, "Unable to guarantee accelerated visual with libSDL < 1.2.10\n" );
			}
		}
#endif

		if ( SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, r_swapInterval->integer ) < 0 )
			ri->Printf( PRINT_ALL, "r_swapInterval requires libSDL >= 1.2.10\n" );

#ifdef USE_ICON
		{
			SDL_Surface *icon = SDL_CreateRGBSurfaceFrom(
					(void *)CLIENT_WINDOW_ICON.pixel_data,
					CLIENT_WINDOW_ICON.width,
					CLIENT_WINDOW_ICON.height,
					CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
					CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
					0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
					0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
					);

			SDL_WM_SetIcon( icon, NULL );
			SDL_FreeSurface( icon );
		}
#endif

		SDL_WM_SetCaption(CLIENT_WINDOW_TITLE, CLIENT_WINDOW_MIN_TITLE);
		SDL_ShowCursor(0);

		if (!(vidscreen = SDL_SetVideoMode(glConfig.vidWidth, glConfig.vidHeight, colorbits, flags)))
		{
			ri->Printf( PRINT_DEVELOPER, "SDL_SetVideoMode failed: %s\n", SDL_GetError( ) );
			continue;
		}

		opengl_context = GLimp_GetCurrentContext();

		ri->Printf( PRINT_ALL, "Using %d/%d/%d Color bits, %d depth, %d stencil display.\n",
				sdlcolorbits, sdlcolorbits, sdlcolorbits, tdepthbits, tstencilbits);

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		break;
	}

	GLimp_DetectAvailableModes();

	if ( !vidscreen ) {
		ri->Printf( PRINT_ALL, "Couldn't get a visual\n" );
		return RSERR_INVALID_MODE;
	}

	screen = vidscreen;

	glstring = (char *) qglGetString (GL_RENDERER);
	ri->Printf( PRINT_ALL, "GL_RENDERER: %s\n", glstring );

	return RSERR_OK;
}

static qboolean GLimp_StartDriverAndSetMode( qboolean fullscreen, qboolean noborder ) {
	rserr_t err;

	if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		char driverName[64];
		int ret = SDL_Init( SDL_INIT_VIDEO );

		if ( ret < 0 ) {
			ri->Printf( PRINT_ALL, "SDL_Init( SDL_INIT_VIDEO ) FAILED (%d: %s)\n", ret, SDL_GetError() );
			return qfalse;
		}

		SDL_VideoDriverName( driverName, sizeof( driverName ) - 1 );
		ri->Printf( PRINT_ALL, "SDL using driver \"%s\"\n", driverName );
		ri->Cvar_Set( "r_sdlDriver", driverName );
	}

	if ( fullscreen && ri->Cvar_VariableIntegerValue( "in_nograb" ) )
	{
		ri->Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n" );
		ri->Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}
	
	err = (rserr_t)GLimp_SetMode( fullscreen, noborder );

	switch ( err ) {
		case RSERR_INVALID_MODE:
			ri->Printf( PRINT_ALL, "...WARNING: could not set the given mode\n" );
			return qfalse;
		default:
			break;
	}

	return qtrue;
}

static qboolean GLimp_HaveExtension( const char *ext ) {
	const char *ptr = Q_stristr( glConfig.extensions_string, ext );
	if ( ptr == NULL )
		return qfalse;
	ptr += strlen( ext );
	return (qboolean)( (*ptr == ' ') || (*ptr == '\0') );  // verify it's complete string.
}


static void GLimp_InitExtensions( void )
{
	if ( !r_allowExtensions->integer ) {
		ri->Printf( PRINT_ALL, "* IGNORING OPENGL EXTENSIONS *\n" );
		return;
	}

	ri->Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	glConfig.textureCompression = TC_NONE;

	// GL_EXT_texture_compression_s3tc
	if ( GLimp_HaveExtension( "GL_ARB_texture_compression" ) &&
	     GLimp_HaveExtension( "GL_EXT_texture_compression_s3tc" ) )
	{
		if ( r_ext_compressed_textures->value ) {
			glConfig.textureCompression = TC_S3TC_ARB;
			ri->Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
		}
		else
			ri->Printf( PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc\n" );
	}
	else
		ri->Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc not found\n" );

	// GL_S3_s3tc ... legacy extension before GL_EXT_texture_compression_s3tc.
	if (glConfig.textureCompression == TC_NONE)
	{
		if ( GLimp_HaveExtension( "GL_S3_s3tc" ) ) {
			if ( r_ext_compressed_textures->value ) {
				glConfig.textureCompression = TC_S3TC;
				ri->Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
			}
			else
				ri->Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
		}
		else
			ri->Printf( PRINT_ALL, "...GL_S3_s3tc not found\n" );
	}


	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( GLimp_HaveExtension( "EXT_texture_env_add" ) ) {
		if ( r_ext_texture_env_add->integer ) {
			glConfig.textureEnvAddAvailable = qtrue;
			ri->Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
		}
		else {
			glConfig.textureEnvAddAvailable = qfalse;
			ri->Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
		}
	}
	else
		ri->Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( GLimp_HaveExtension( "GL_ARB_multitexture" ) ) {
		if ( r_ext_multitexture->value ) {
			qglMultiTexCoord2fARB = SDL_GL_GetProcAddress( "glMultiTexCoord2fARB" );
			qglActiveTextureARB = SDL_GL_GetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = SDL_GL_GetProcAddress( "glClientActiveTextureARB" );

			if ( qglActiveTextureARB ) {
				GLint glint = 0;
				qglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glint );
				glConfig.numTextureUnits = (int) glint;
				if ( glConfig.numTextureUnits > 1 )
					ri->Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
				else {
					qglMultiTexCoord2fARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					ri->Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
				}
			}
		}
		else
			ri->Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
	}
	else
		ri->Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );

	// GL_EXT_compiled_vertex_array
	if ( GLimp_HaveExtension( "GL_EXT_compiled_vertex_array" ) ) {
		if ( r_ext_compiled_vertex_array->value ) {
			ri->Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
			qglLockArraysEXT = SDL_GL_GetProcAddress( "glLockArraysEXT" );
			qglUnlockArraysEXT = SDL_GL_GetProcAddress( "glUnlockArraysEXT" );
			if (!qglLockArraysEXT || !qglUnlockArraysEXT)
				ri->Error (ERR_FATAL, "bad getprocaddress");
		}
		else
			ri->Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
	}
	else
		ri->Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );

	textureFilterAnisotropic = qfalse;
	if ( GLimp_HaveExtension( "GL_EXT_texture_filter_anisotropic" ) ) {
		if ( r_ext_texture_filter_anisotropic->integer ) {
			qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLfloat *)&glConfig.maxTextureFilterAnisotropy );
			if ( glConfig.maxTextureFilterAnisotropy <= 0 ) {
				ri->Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not properly supported!\n" );
				glConfig.maxTextureFilterAnisotropy = 0;
			}
			else {
				ri->Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic (max: %.2f)\n", glConfig.maxTextureFilterAnisotropy );
				textureFilterAnisotropic = qtrue;
			}
		}
		else
			ri->Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
	}
	else
		ri->Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
}

// This routine is responsible for initializing the OS specific portions of OpenGL
void GLimp_Init( void )
{
	r_allowSoftwareGL	= ri->Cvar_Get( "r_allowSoftwareGL",	"0",	CVAR_LATCH,		NULL, NULL );
	r_sdlDriver			= ri->Cvar_Get( "r_sdlDriver",			"",		CVAR_ROM,		NULL, NULL );
	vid_centerWindow	= ri->Cvar_Get( "vid_centerWindow",		"1",	CVAR_ARCHIVE,	NULL, NULL );

	//QtZ: in_disableLockKeys, vid_xpos, vid_ypos from iodfe
	vid_width			= ri->Cvar_Get( "vid_width",			"0",	CVAR_ARCHIVE,	NULL, NULL );
	vid_height			= ri->Cvar_Get( "vid_height",			"0",	CVAR_ARCHIVE,	NULL, NULL );
	vid_xpos			= ri->Cvar_Get( "vid_xpos",				"100",	CVAR_ARCHIVE,	NULL, NULL );
	vid_ypos			= ri->Cvar_Get( "vid_ypos",				"100",	CVAR_ARCHIVE,	NULL, NULL );
	in_disableLockKeys	= ri->Cvar_Get( "in_disableLockKeys",	"0",	CVAR_ARCHIVE,	NULL, NULL );
	//~QtZ

	if ( ri->Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		ri->Cvar_Set( "vid_width", "0" );
		ri->Cvar_Set( "vid_height", "0" );
		ri->Cvar_Set( "r_fullscreen", "0" );
		ri->Cvar_Set( "vid_centerWindow", "1" );
		ri->Cvar_Set( "com_abnormalExit", "0" );
		//QtZ: in_disableLockKeys, r_xpos, r_ypos from iodfe
		ri->Cvar_Set( "vid_xpos", "100" );
		ri->Cvar_Set( "vid_ypos", "100" );
		//~QtZ
	}

	ri->Sys_SetEnv( "SDL_VIDEO_CENTERED", vid_centerWindow->integer ? "1" : "" );

	//QtZ: in_disableLockKeys, r_xpos, r_ypos from iodfe
	ri->Sys_SetEnv( "SDL_VIDEO_WINDOW_POS", va( "%d,%d", vid_xpos->integer, vid_ypos->integer ) );
	ri->Sys_SetEnv( "SDL_DISABLE_LOCK_KEYS", in_disableLockKeys->integer ? "1" : "" ); //putting this here because it needs vid_restart anyway
	//~QtZ

	ri->Sys_GLimpInit( );

	// Create the window and set up the context
	if ( GLimp_StartDriverAndSetMode( r_fullscreen->boolean, r_noborder->boolean ) )
		goto success;

	// Try again, this time in a platform specific "safe mode"
	ri->Sys_GLimpSafeInit();

	if ( GLimp_StartDriverAndSetMode( r_fullscreen->boolean, qfalse ) )
		goto success;

	// Nothing worked, give up
	ri->Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem" );

success:
	// This values force the UI to disable driver selection
	glConfig.deviceSupportsGamma = SDL_SetGamma( 1.0f, 1.0f, 1.0f ) >= 0;

	// Mysteriously, if you use an NVidia graphics card and multiple monitors,
	// SDL_SetGamma will incorrectly return false... the first time; ask
	// again and you get the correct answer. This is a suspected driver bug, see
	// http://bugzilla.icculus.org/show_bug.cgi?id=4316
	glConfig.deviceSupportsGamma = SDL_SetGamma( 1.0f, 1.0f, 1.0f ) >= 0;

	if ( -1 == r_ignorehwgamma->integer)
		glConfig.deviceSupportsGamma = 1;

	if ( 1 == r_ignorehwgamma->integer)
		glConfig.deviceSupportsGamma = 0;

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, (char *) qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (char *) qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	Q_strncpyz( glConfig.version_string, (char *) qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, (char *) qglGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );

	// initialize extensions
	GLimp_InitExtensions( );

	ri->Cvar_Get( "r_availableModes", "", CVAR_ROM, NULL, NULL );

	// This depends on SDL_INIT_VIDEO, hence having it here
	ri->IN_Init( );
}


// Responsible for doing a swapbuffers
void GLimp_EndFrame( void )
{
	// don't flip if drawing to front buffer
	if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		SDL_GL_SwapBuffers();
	}

	if( r_fullscreen->modified )
	{
		qboolean    fullscreen;
		qboolean    needToToggle = qtrue;
		qboolean    sdlToggled = qfalse;
		SDL_Surface *s = SDL_GetVideoSurface( );

		if( s )
		{
			// Find out the current state
			fullscreen = !!( s->flags & SDL_FULLSCREEN );
				
			if( r_fullscreen->integer && ri->Cvar_VariableIntegerValue( "in_nograb" ) )
			{
				ri->Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n");
				ri->Cvar_Set( "r_fullscreen", "0" );
				r_fullscreen->modified = qfalse;
			}

			// Is the state we want different from the current state?
			needToToggle = !!r_fullscreen->integer != fullscreen;

			if( needToToggle )
				sdlToggled = SDL_WM_ToggleFullScreen( s );
		}

		if( needToToggle )
		{
			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			if( !sdlToggled )
				ri->Cmd_ExecuteText(EXEC_APPEND, "vid_restart\n");

			ri->IN_Restart( );
		}

		r_fullscreen->modified = qfalse;
	}
}

// No SMP - stubs
void GLimp_RenderThreadWrapper( void *arg ) {
}

qboolean GLimp_SpawnRenderThread( void (*function)( void ) ) {
	ri->Printf( PRINT_WARNING, "ERROR: SMP support was disabled at compile time\n");
	return qfalse;
}

void *GLimp_RendererSleep( void ) {
	return NULL;
}

void GLimp_FrontEndSleep( void ) {
}

void GLimp_WakeRenderer( void *data ) {
}
