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
// tr_init.c -- functions that are not called every frame

#include "tr_local.h"
#include "tr_ext_public.h"

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

glconfig_t  glConfig;
qboolean    textureFilterAnisotropic = qfalse;
int         maxAnisotropy = 0;
float       displayAspect = 0.0f;

glstate_t	glState;

static void GfxInfo_f( void );

cvar_t  *com_altivec;

cvar_t	*r_flareSize;
cvar_t	*r_flareFade;
cvar_t	*r_flareCoeff;
cvar_t	*r_flareWidth;
cvar_t	*r_flareDlights;

cvar_t	*r_ignoreFastPath;

cvar_t	*r_verbose;
cvar_t	*r_ignore;

cvar_t	*r_detailTextures;

cvar_t	*r_znear;
cvar_t	*r_zproj;
cvar_t	*r_stereoSeparation;

cvar_t	*r_skipBackEnd;

cvar_t	*r_stereoEnabled;
cvar_t	*r_anaglyphMode;

cvar_t	*r_greyscale;

cvar_t	*r_ignorehwgamma;
cvar_t	*r_measureOverdraw;

cvar_t	*r_inGameVideo;
cvar_t	*r_fastsky;
cvar_t	*r_drawSun;
cvar_t	*r_dynamiclight;
cvar_t	*r_dlightBacks;

cvar_t	*r_lodbias;
cvar_t	*r_lodscale;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_facePlaneCull;
cvar_t	*r_showcluster;
cvar_t	*r_nocurves;

cvar_t	*r_allowExtensions;

cvar_t	*r_ext_compressed_textures;
cvar_t	*r_ext_multitexture;
cvar_t	*r_ext_compiled_vertex_array;
cvar_t	*r_ext_texture_env_add;
cvar_t	*r_ext_texture_filter_anisotropic;
cvar_t	*r_ext_max_anisotropy;

cvar_t	*r_ignoreGLErrors;
cvar_t	*r_logFile;

cvar_t	*r_stencilbits;
cvar_t	*r_depthbits;
cvar_t	*r_colorbits;
cvar_t	*r_primitives;
cvar_t	*r_texturebits;
cvar_t  *r_ext_multisample;

cvar_t	*r_drawBuffer;
cvar_t	*r_lightmap;
cvar_t	*r_vertexLight;
cvar_t	*r_uiFullScreen;
cvar_t	*r_shadows;
cvar_t	*r_flares;
cvar_t	*r_nobind;
cvar_t	*r_singleShader;
cvar_t	*r_roundImagesDown;
cvar_t	*r_colorMipLevels;
cvar_t	*r_picmip;
cvar_t	*r_showtris;
cvar_t	*r_showsky;
cvar_t	*r_shownormals;
cvar_t	*r_finish;
cvar_t	*r_clear;
cvar_t	*r_swapInterval;
cvar_t	*r_textureMode;
cvar_t	*r_offsetFactor;
cvar_t	*r_offsetUnits;
cvar_t	*r_gamma;
cvar_t	*r_intensity;
cvar_t	*r_lockpvs;
cvar_t	*r_noportals;
cvar_t	*r_portalOnly;

cvar_t	*r_subdivisions;
cvar_t	*r_lodCurveError;

cvar_t	*r_fullscreen;
cvar_t  *r_noborder;

cvar_t	*r_customPixelAspect;

cvar_t	*r_overBrightBits;
cvar_t	*r_mapOverBrightBits;

cvar_t	*r_debugSurface;
cvar_t	*r_simpleMipMaps;

cvar_t	*r_showImages;

cvar_t	*r_ambientScale;
cvar_t	*r_directedScale;
cvar_t	*r_debugLight;
cvar_t	*r_debugSort;
cvar_t	*r_printShaders;
cvar_t	*r_saveFontData;

cvar_t	*r_marksOnTriangleMeshes;

cvar_t	*r_aviMotionJpegQuality;
cvar_t	*r_screenshotJpegQuality;

cvar_t	*r_maxpolys;
int		max_polys;
cvar_t	*r_maxpolyverts;
int		max_polyverts;

//QtZ: Post processing
cvar_t *r_postprocess_enable;
cvar_t *r_postprocess_glow;
cvar_t *r_postprocess_glowMulti;
cvar_t *r_postprocess_glowSat;
cvar_t *r_postprocess_glowSize;
cvar_t *r_postprocess_glowBlurStyle;
cvar_t *r_postprocess_hdr;
cvar_t *r_postprocess_hdrExposureBias;
cvar_t *r_postprocess_brightPassThres;
cvar_t *r_postprocess_brightPassStyle;
cvar_t *r_postprocess_bloom;
cvar_t *r_postprocess_bloomMulti;
cvar_t *r_postprocess_bloomSat;
cvar_t *r_postprocess_bloomSceneMulti;
cvar_t *r_postprocess_bloomSceneSat;
cvar_t *r_postprocess_bloomSize;
cvar_t *r_postprocess_bloomSteps;
cvar_t *r_postprocess_bloomBlurStyle;
cvar_t *r_postprocess_finalPass;
cvar_t *r_postprocess_userVec1;
cvar_t *r_postprocess_debugFBOs;

cvar_t *r_environmentMapping;
//~QtZ


/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL( void )
{
	char renderer_buffer[1024];

	//
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_(color|depth|stencil)bits
	//		- r_ignorehwgamma
	//		- r_gamma
	//
	
	if ( glConfig.vidWidth == 0 )
	{
		GLint		temp;
		
		GLimp_Init();

		Q_strncpyz( renderer_buffer, glConfig.renderer_string, sizeof( renderer_buffer ) );
		Q_strlwr( renderer_buffer );

		// OpenGL driver constants
		qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
		glConfig.maxTextureSize = temp;

		// stubbed or broken drivers may have reported 0...
		if ( glConfig.maxTextureSize <= 0 ) 
		{
			glConfig.maxTextureSize = 0;
		}
	}

	// set default state
	GL_SetDefaultState();
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors( void ) {
	int		err;
	char	s[64];

	err = qglGetError();
	if ( err == GL_NO_ERROR ) {
		return;
	}
	if ( r_ignoreGLErrors->integer ) {
		return;
	}
	switch( err ) {
		case GL_INVALID_ENUM:
			Q_strncpyz( s, "GL_INVALID_ENUM", sizeof( s ) );
			break;
		case GL_INVALID_VALUE:
			Q_strncpyz( s, "GL_INVALID_VALUE", sizeof( s ) );
			break;
		case GL_INVALID_OPERATION:
			Q_strncpyz( s, "GL_INVALID_OPERATION", sizeof( s ) );
			break;
		case GL_STACK_OVERFLOW:
			Q_strncpyz( s, "GL_STACK_OVERFLOW", sizeof( s ) );
			break;
		case GL_STACK_UNDERFLOW:
			Q_strncpyz( s, "GL_STACK_UNDERFLOW", sizeof( s ) );
			break;
		case GL_OUT_OF_MEMORY:
			Q_strncpyz( s, "GL_OUT_OF_MEMORY", sizeof( s ) );
			break;
		default:
			Com_sprintf( s, sizeof(s), "%i", err);
			break;
	}

	ri->Error( ERR_FATAL, "GL_CheckErrors: %s", s );
}

/* 
============================================================================== 
 
						SCREEN SHOTS 

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

============================================================================== 
*/ 

/* 
================== 
RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.

Reads a minimum offset for where the RGB data starts in the image from
integer stored at pointer offset. When the function has returned the actual
offset was written back to address offset. This address will always have an
alignment of packAlign to ensure efficient copying.

Stores the length of padding after a line of pixels to address padlen

Return value must be freed with ri->Hunk_FreeTempMemory()
================== 
*/  

byte *RB_ReadPixels(int x, int y, int width, int height, size_t *offset, int *padlen)
{
	byte *buffer, *bufstart;
	int padwidth, linelen;
	GLint packAlign;
	
	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);
	
	linelen = width * 3;
	padwidth = PAD(linelen, packAlign);
	
	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = ri->Hunk_AllocateTempMemory(padwidth * height + *offset + packAlign - 1);
	
	bufstart = PADP((intptr_t) buffer + *offset, packAlign);
	qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);
	
	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;
	
	return buffer;
}

void R_TakeScreenshotTGA( int x, int y, int width, int height, char *fileName ) {
	byte *allbuf, *buffer;
	byte *srcptr, *destptr;
	byte *endline, *endmem;
	byte temp;

	int linelen, padlen;
	size_t offset = 18, memcount;

	allbuf = RB_ReadPixels( x, y, width, height, &offset, &padlen );
	buffer = allbuf + offset - 18;

	memset( buffer, 0, 18 );
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	// swap rgb to bgr and remove padding from line endings
	linelen = width * 3;

	srcptr = destptr = allbuf + offset;
	endmem = srcptr + (linelen + padlen) * height;

	while ( srcptr < endmem )
	{
		endline = srcptr + linelen;

		while ( srcptr < endline ) {
			temp = srcptr[0];
			*destptr++ = srcptr[2];
			*destptr++ = srcptr[1];
			*destptr++ = temp;

			srcptr += 3;
		}

		// Skip the pad
		srcptr += padlen;
	}

	memcount = linelen * height;

	// gamma correct
	if ( glConfig.deviceSupportsGamma )
		R_GammaCorrect( allbuf + offset, memcount );

	ri->FS_WriteFile( fileName, buffer, memcount + 18 );

	ri->Hunk_FreeTempMemory( allbuf );
}

void R_TakeScreenshotPNG( int x, int y, int width, int height, char *fileName ) {
	byte *buffer=NULL;
	size_t offset=0;
	int padlen=0;

	buffer = RB_ReadPixels( x, y, width, height, &offset, &padlen );
	RE_SavePNG( fileName, buffer, width, height, 3 );
	ri->Hunk_FreeTempMemory( buffer );
}

void R_TakeScreenshotJPEG( int x, int y, int width, int height, char *fileName ) {
	byte *buffer;
	size_t offset = 0, memcount;
	int padlen;

	buffer = RB_ReadPixels( x, y, width, height, &offset, &padlen );
	memcount = (width * 3 + padlen) * height;

	// gamma correct
	if ( glConfig.deviceSupportsGamma )
		R_GammaCorrect( buffer + offset, memcount );

	RE_SaveJPG( fileName, r_screenshotJpegQuality->integer, width, height, buffer + offset, padlen );
	ri->Hunk_FreeTempMemory( buffer );
}

void R_ScreenshotFilename( char *buf, int bufSize, const char *ext ) {
	time_t rawtime;
	char timeStr[32] = {0}; // should really only reach ~19 chars

	time( &rawtime );
	strftime( timeStr, sizeof( timeStr ), "%Y-%m-%d_%H-%M-%S", localtime( &rawtime ) ); // or gmtime

	Com_sprintf( buf, bufSize, "screenshots/shot%s%s", timeStr, ext );
}

void R_ScreenShotTGA_f( void ) {
	char checkname[MAX_OSPATH] = {0};
	qboolean silent = qfalse;

	if ( !strcmp( ri->Cmd_Argv( 1 ), "silent" ) )
		silent = qtrue;

	if ( ri->Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, sizeof( checkname ), "screenshots/%s.tga", ri->Cmd_Argv( 1 ) );
	}
	else {
		// timestamp the file
		R_ScreenshotFilename( checkname, sizeof( checkname ), ".tga" );

		if ( ri->FS_FileExists( checkname ) ) {
			Com_Printf( "ScreenShot: Couldn't create a file\n"); 
			return;
 		}
	}

	R_TakeScreenshotTGA( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );

	if ( !silent )
		Com_Printf( "Wrote %s\n", checkname );
}

void R_ScreenShotPNG_f( void ) {
	char checkname[MAX_OSPATH] = {0};
	qboolean silent = qfalse;

	if ( !strcmp( ri->Cmd_Argv( 1 ), "silent" ) )
		silent = qtrue;

	if ( ri->Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, sizeof( checkname ), "screenshots/%s.png", ri->Cmd_Argv( 1 ) );
	}
	else {
		// timestamp the file
		R_ScreenshotFilename( checkname, sizeof( checkname ), ".png" );

		if ( ri->FS_FileExists( checkname ) ) {
			Com_Printf( "ScreenShot: Couldn't create a file\n"); 
			return;
 		}
	}

	R_TakeScreenshotPNG( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );

	if ( !silent )
		Com_Printf( "Wrote %s\n", checkname );
} 

void R_ScreenShotJPEG_f( void ) {
	char checkname[MAX_OSPATH] = {0};
	qboolean silent = qfalse;

	if ( !strcmp( ri->Cmd_Argv( 1 ), "silent" ) )
		silent = qtrue;

	if ( ri->Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, sizeof( checkname ), "screenshots/%s.jpg", ri->Cmd_Argv( 1 ) );
	}
	else {
		// timestamp the file
		R_ScreenshotFilename( checkname, sizeof( checkname ), ".jpg" );

		if ( ri->FS_FileExists( checkname ) ) {
			Com_Printf( "ScreenShot: Couldn't create a file\n" );
			return;
 		}
	}

	R_TakeScreenshotJPEG( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );

	if ( !silent )
		Com_Printf( "Wrote %s\n", checkname );
}

//QtZ: Post processing
/*
==================
RB_PostProcessCmd
==================
*/
const void *RB_PostProcessCmd( const void *data ) {
	const postProcessCommand_t*cmd;
	
	cmd = (const postProcessCommand_t *)data;
	
	RB_PostProcess();
	
	return (const void *)(cmd + 1);	
}
//~QtZ

//============================================================================

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void *RB_TakeVideoFrameCmd( const void *data )
{
	const videoFrameCommand_t	*cmd;
	byte				*cBuf;
	size_t				memcount, linelen;
	int				padwidth, avipadwidth, padlen, avipadlen;
	GLint packAlign;
	
	cmd = (const videoFrameCommand_t *)data;
	
	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen = cmd->width * 3;

	// Alignment stuff for glReadPixels
	padwidth = PAD(linelen, packAlign);
	padlen = padwidth - linelen;
	// AVI line padding
	avipadwidth = PAD(linelen, AVI_LINE_PADDING);
	avipadlen = avipadwidth - linelen;

	cBuf = PADP(cmd->captureBuffer, packAlign);
		
	qglReadPixels(0, 0, cmd->width, cmd->height, GL_RGB,
		GL_UNSIGNED_BYTE, cBuf);

	memcount = padwidth * cmd->height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(cBuf, memcount);

	if(cmd->motionJpeg)
	{
		memcount = RE_SaveJPGToBuffer(cmd->encodeBuffer, linelen * cmd->height,
			r_aviMotionJpegQuality->integer,
			cmd->width, cmd->height, cBuf, padlen);
		ri->CL_WriteAVIVideoFrame(cmd->encodeBuffer, memcount);
	}
	else
	{
		byte *lineend, *memend;
		byte *srcptr, *destptr;
	
		srcptr = cBuf;
		destptr = cmd->encodeBuffer;
		memend = srcptr + memcount;
		
		// swap R and B and remove line paddings
		while(srcptr < memend)
		{
			lineend = srcptr + linelen;
			while(srcptr < lineend)
			{
				*destptr++ = srcptr[2];
				*destptr++ = srcptr[1];
				*destptr++ = srcptr[0];
				srcptr += 3;
			}
			
			memset(destptr, '\0', avipadlen);
			destptr += avipadlen;
			
			srcptr += padlen;
		}
		
		ri->CL_WriteAVIVideoFrame(cmd->encodeBuffer, avipadwidth * cmd->height);
	}

	return (const void *)(cmd + 1);	
}

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	qglClearDepth( 1.0f );

	qglCullFace(GL_FRONT);

	qglColor4f (1,1,1,1);

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if ( qglActiveTextureARB ) {
		GL_SelectTexture( 1 );
		GL_TextureMode( r_textureMode->string );
		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		GL_SelectTexture( 0 );
	}

	qglEnable(GL_TEXTURE_2D);
	GL_TextureMode( r_textureMode->string );
	GL_TexEnv( GL_MODULATE );

	qglShadeModel( GL_SMOOTH );
	qglDepthFunc( GL_LEQUAL );

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState (GL_VERTEX_ARRAY);

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask( GL_TRUE );
	qglDisable( GL_DEPTH_TEST );
	qglEnable( GL_SCISSOR_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );
}

/*
================
R_PrintLongString

Workaround for ri->Printf's 1024 characters buffer limit.
================
*/
void R_PrintLongString(const char *string) {
	char buffer[1024];
	const char *p;
	int size = strlen(string);

	p = string;
	while(size > 0)
	{
		Q_strncpyz(buffer, p, sizeof (buffer) );
		ri->Printf( PRINT_ALL, "%s", buffer );
		p += 1023;
		size -= 1023;
	}
}

/*
================
GfxInfo_f
================
*/
void GfxInfo_f( void ) {
	const char *enablestrings[] = {
		"disabled",
		"enabled"
	};
	const char *fsstrings[] = {
		"windowed",
		"fullscreen"
	};

	ri->Printf( PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	ri->Printf( PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	ri->Printf( PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
	ri->Printf( PRINT_ALL, "GL_EXTENSIONS: " ); R_PrintLongString( glConfig.extensions_string );
	ri->Printf( PRINT_ALL, "\n" );
	ri->Printf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	ri->Printf( PRINT_ALL, "GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.numTextureUnits );
	ri->Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	ri->Printf( PRINT_ALL, "DIMENSIONS: %d x %d %s hz:", glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1] );

	if ( glConfig.displayFrequency )
		ri->Printf( PRINT_ALL, "FREQUENCY: %d\n", glConfig.displayFrequency );
	else
		ri->Printf( PRINT_ALL, "FREQUENCY: N/A\n" );

	if ( glConfig.deviceSupportsGamma )
		ri->Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	else
		ri->Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );

	// rendering primitives
	{
		int		primitives;

		// default is to use triangles if compiled vertex arrays are present
		ri->Printf( PRINT_ALL, "rendering primitives: " );
		primitives = r_primitives->integer;
		if ( primitives == 0 ) {
			if ( qglLockArraysEXT )		primitives = 2;
			else						primitives = 1;
		}
			 if ( primitives == -1 )	ri->Printf( PRINT_ALL, "none\n" );
		else if ( primitives == 2 )		ri->Printf( PRINT_ALL, "single glDrawElements\n" );
		else if ( primitives == 1 )		ri->Printf( PRINT_ALL, "multiple glArrayElement\n" );
		else if ( primitives == 3 )		ri->Printf( PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
	}

	ri->Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
	ri->Printf( PRINT_ALL, "picmip: %d\n", r_picmip->integer );
	ri->Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );
	ri->Printf( PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != 0] );
	ri->Printf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
	ri->Printf( PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
	ri->Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression!=TC_NONE] );
	if ( r_vertexLight->integer )
		ri->Printf( PRINT_ALL, "HACK: using vertex lightmap approximation\n" );
	if ( r_finish->integer )
		ri->Printf( PRINT_ALL, "Forcing glFinish\n" );
}

/*
===============
R_Register
===============
*/
void R_Register( void ) 
{
	com_altivec							= ri->Cvar_Get( "com_altivec",						"1",								CVAR_ARCHIVE,				NULL, NULL );

	// latched and archived variables
	r_allowExtensions					= ri->Cvar_Get( "r_allowExtensions",				"1",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_ext_compressed_textures			= ri->Cvar_Get( "r_ext_compressed_textures",		"0",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_ext_multitexture					= ri->Cvar_Get( "r_ext_multitexture",				"1",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_ext_compiled_vertex_array			= ri->Cvar_Get( "r_ext_compiled_vertex_array",		"1",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_ext_texture_env_add				= ri->Cvar_Get( "r_ext_texture_env_add",			"1",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_ext_texture_filter_anisotropic	= ri->Cvar_Get( "r_ext_texture_filter_anisotropic",	"16",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_ext_max_anisotropy				= ri->Cvar_Get( "r_ext_max_anisotropy",				"16",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_picmip							= ri->Cvar_Get( "r_picmip",							"0",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_roundImagesDown					= ri->Cvar_Get( "r_roundImagesDown",				"1",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_colorMipLevels					= ri->Cvar_Get( "r_colorMipLevels",					"0",								CVAR_LATCH,					NULL, NULL );
	ri->Cvar_CheckRange( r_picmip, 0, 16, qtrue );
	r_detailTextures					= ri->Cvar_Get( "r_detailtextures",					"1",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_texturebits						= ri->Cvar_Get( "r_texturebits",					"32",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_colorbits							= ri->Cvar_Get( "r_colorbits",						"32",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_stencilbits						= ri->Cvar_Get( "r_stencilbits",					"8",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_depthbits							= ri->Cvar_Get( "r_depthbits",						"24",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_ext_multisample					= ri->Cvar_Get( "r_ext_multisample",				"0",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	ri->Cvar_CheckRange( r_ext_multisample, 0, 4, qtrue );
	r_overBrightBits					= ri->Cvar_Get( "r_overBrightBits",					"0",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_mapOverBrightBits					= ri->Cvar_Get( "r_mapOverBrightBits",				"2",								CVAR_LATCH,					NULL, NULL );
	r_ignorehwgamma						= ri->Cvar_Get( "r_ignorehwgamma",					"0",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_fullscreen						= ri->Cvar_Get( "r_fullscreen",						"0",								CVAR_ARCHIVE,				NULL, NULL );
	r_noborder							= ri->Cvar_Get( "r_noborder",						"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_customPixelAspect					= ri->Cvar_Get( "r_customPixelAspect",				"1",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_simpleMipMaps						= ri->Cvar_Get( "r_simpleMipMaps",					"0",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_vertexLight						= ri->Cvar_Get( "r_vertexLight",					"0",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_uiFullScreen						= ri->Cvar_Get( "r_uifullscreen",					"1",								CVAR_NONE,					NULL, NULL );
	r_subdivisions						= ri->Cvar_Get( "r_subdivisions",					"4",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_stereoEnabled						= ri->Cvar_Get( "r_stereoEnabled",					"0",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_ignoreFastPath					= ri->Cvar_Get( "r_ignoreFastPath",					"1",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_greyscale							= ri->Cvar_Get( "r_greyscale",						"0",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );

	r_fullbright						= ri->Cvar_Get( "r_fullbright",						"0",								CVAR_LATCH,					NULL, NULL );
	r_intensity							= ri->Cvar_Get( "r_intensity",						"1",								CVAR_LATCH,					NULL, NULL );
	r_singleShader						= ri->Cvar_Get( "r_singleShader",					"0",								CVAR_CHEAT|CVAR_LATCH,		NULL, NULL );

	// archived variables that can change at any time
	r_lodCurveError						= ri->Cvar_Get( "r_lodCurveError",					"250",								CVAR_ARCHIVE|CVAR_CHEAT,	NULL, NULL );
	r_lodbias							= ri->Cvar_Get( "r_lodbias",						"0",								CVAR_ARCHIVE,				NULL, NULL );
	r_flares							= ri->Cvar_Get( "r_flares",							"0",								CVAR_ARCHIVE,				NULL, NULL );
	r_znear								= ri->Cvar_Get( "r_znear",							"4",								CVAR_CHEAT,					NULL, NULL );
	ri->Cvar_CheckRange( r_znear, 0.001f, 200, qfalse );
	r_zproj								= ri->Cvar_Get( "r_zproj",							"64",								CVAR_ARCHIVE,				NULL, NULL );
	r_stereoSeparation					= ri->Cvar_Get( "r_stereoSeparation",				"64",								CVAR_ARCHIVE,				NULL, NULL );
	r_ignoreGLErrors					= ri->Cvar_Get( "r_ignoreGLErrors",					"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_fastsky							= ri->Cvar_Get( "r_fastsky",						"0",								CVAR_ARCHIVE,				NULL, NULL );
	r_inGameVideo						= ri->Cvar_Get( "r_inGameVideo",					"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_drawSun							= ri->Cvar_Get( "r_drawSun",						"0",								CVAR_ARCHIVE,				NULL, NULL );
	r_dynamiclight						= ri->Cvar_Get( "r_dynamiclight",					"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_dlightBacks						= ri->Cvar_Get( "r_dlightBacks",					"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_finish							= ri->Cvar_Get( "r_finish",							"0",								CVAR_ARCHIVE,				NULL, NULL );
	r_textureMode						= ri->Cvar_Get( "r_textureMode",					"GL_LINEAR_MIPMAP_LINEAR",			CVAR_ARCHIVE,				NULL, NULL );
	r_swapInterval						= ri->Cvar_Get( "r_swapInterval",					"0",								CVAR_ARCHIVE|CVAR_LATCH,	NULL, NULL );
	r_gamma								= ri->Cvar_Get( "r_gamma",							"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_facePlaneCull						= ri->Cvar_Get( "r_facePlaneCull",					"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_primitives						= ri->Cvar_Get( "r_primitives",						"0",								CVAR_ARCHIVE,				NULL, NULL );
	r_ambientScale						= ri->Cvar_Get( "r_ambientScale",					"0.333",							CVAR_ARCHIVE,				NULL, NULL );
	r_directedScale						= ri->Cvar_Get( "r_directedScale",					"0.666",							CVAR_ARCHIVE,				NULL, NULL );
	r_anaglyphMode						= ri->Cvar_Get( "r_anaglyphMode",					"0",								CVAR_ARCHIVE,				NULL, NULL );

	// temporary variables that can change at any time
	r_showImages						= ri->Cvar_Get( "r_showImages",						"0",								CVAR_TEMP,					NULL, NULL );
	r_debugLight						= ri->Cvar_Get( "r_debuglight",						"0",								CVAR_TEMP,					NULL, NULL );
	r_debugSort							= ri->Cvar_Get( "r_debugSort",						"0",								CVAR_CHEAT,					NULL, NULL );
	r_printShaders						= ri->Cvar_Get( "r_printShaders",					"0",								CVAR_NONE,					NULL, NULL );
	r_saveFontData						= ri->Cvar_Get( "r_saveFontData",					"0",								CVAR_NONE,					NULL, NULL );
	r_nocurves							= ri->Cvar_Get( "r_nocurves",						"0",								CVAR_CHEAT,					NULL, NULL );
	r_drawworld							= ri->Cvar_Get( "r_drawworld",						"1",								CVAR_CHEAT,					NULL, NULL );
	r_lightmap							= ri->Cvar_Get( "r_lightmap",						"0",								CVAR_NONE,					NULL, NULL );
	r_portalOnly						= ri->Cvar_Get( "r_portalOnly",						"0",								CVAR_CHEAT,					NULL, NULL );
	r_flareSize							= ri->Cvar_Get( "r_flareSize",						"40",								CVAR_CHEAT,					NULL, NULL );
	r_flareFade							= ri->Cvar_Get( "r_flareFade",						"7",								CVAR_CHEAT,					NULL, NULL );
	r_flareCoeff						= ri->Cvar_Get( "r_flareCoeff",						FLARE_STDCOEFF,						CVAR_CHEAT,					NULL, NULL );
	r_flareWidth						= ri->Cvar_Get( "r_flareWidth",						"2.25",								CVAR_ARCHIVE,				NULL, NULL );
	r_flareDlights						= ri->Cvar_Get( "r_flareDlights",					"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_skipBackEnd						= ri->Cvar_Get( "r_skipBackEnd",					"0",								CVAR_CHEAT,					NULL, NULL );
	r_measureOverdraw					= ri->Cvar_Get( "r_measureOverdraw",				"0",								CVAR_CHEAT,					NULL, NULL );
	r_lodscale							= ri->Cvar_Get( "r_lodscale",						"5",								CVAR_ARCHIVE,				NULL, NULL );
	r_norefresh							= ri->Cvar_Get( "r_norefresh",						"0",								CVAR_CHEAT,					NULL, NULL );
	r_drawentities						= ri->Cvar_Get( "r_drawentities",					"1",								CVAR_CHEAT,					NULL, NULL );
	r_ignore							= ri->Cvar_Get( "r_ignore",							"1",								CVAR_CHEAT,					NULL, NULL );
	r_nocull							= ri->Cvar_Get( "r_nocull",							"0",								CVAR_CHEAT,					NULL, NULL );
	r_novis								= ri->Cvar_Get( "r_novis",							"0",								CVAR_CHEAT,					NULL, NULL );
	r_showcluster						= ri->Cvar_Get( "r_showcluster",					"0",								CVAR_CHEAT,					NULL, NULL );
	r_speeds							= ri->Cvar_Get( "r_speeds",							"0",								CVAR_CHEAT,					NULL, NULL );
	r_verbose							= ri->Cvar_Get( "r_verbose",						"0",								CVAR_CHEAT,					NULL, NULL );
	r_logFile							= ri->Cvar_Get( "r_logFile",						"0",								CVAR_CHEAT,					NULL, NULL );
	r_debugSurface						= ri->Cvar_Get( "r_debugSurface",					"0",								CVAR_CHEAT,					NULL, NULL );
	r_nobind							= ri->Cvar_Get( "r_nobind",							"0",								CVAR_CHEAT,					NULL, NULL );
	r_showtris							= ri->Cvar_Get( "r_showtris",						"0",								CVAR_CHEAT,					NULL, NULL );
	r_showsky							= ri->Cvar_Get( "r_showsky",						"0",								CVAR_CHEAT,					NULL, NULL );
	r_shownormals						= ri->Cvar_Get( "r_shownormals",					"0",								CVAR_CHEAT,					NULL, NULL );
	r_clear								= ri->Cvar_Get( "r_clear",							"0",								CVAR_ARCHIVE,				NULL, NULL );
	r_offsetFactor						= ri->Cvar_Get( "r_offsetfactor",					"-1",								CVAR_CHEAT,					NULL, NULL );
	r_offsetUnits						= ri->Cvar_Get( "r_offsetunits",					"-2",								CVAR_CHEAT,					NULL, NULL );
	r_drawBuffer						= ri->Cvar_Get( "r_drawBuffer",						"GL_BACK",							CVAR_CHEAT,					NULL, NULL );
	r_lockpvs							= ri->Cvar_Get( "r_lockpvs",						"0",								CVAR_CHEAT,					NULL, NULL );
	r_noportals							= ri->Cvar_Get( "r_noportals",						"0",								CVAR_CHEAT,					NULL, NULL );
	r_shadows							= ri->Cvar_Get( "cg_shadows",						"1",								CVAR_NONE,					NULL, NULL );
	r_marksOnTriangleMeshes				= ri->Cvar_Get( "r_marksOnTriangleMeshes",			"0",								CVAR_ARCHIVE,				NULL, NULL );
	r_aviMotionJpegQuality				= ri->Cvar_Get( "r_aviMotionJpegQuality",			"90",								CVAR_ARCHIVE,				NULL, NULL );
	r_screenshotJpegQuality				= ri->Cvar_Get( "r_screenshotJpegQuality",			"90",								CVAR_ARCHIVE,				NULL, NULL );
	r_maxpolys							= ri->Cvar_Get( "r_maxpolys",						va( "%d", MAX_POLYS ),				CVAR_ARCHIVE,				NULL, NULL );
	r_maxpolyverts						= ri->Cvar_Get( "r_maxpolyverts",					va( "%d", MAX_POLYVERTS ),			CVAR_ARCHIVE,				NULL, NULL );

	//QtZ: Post processing
	r_postprocess_enable				= ri->Cvar_Get( "r_postprocess_enable",				"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_glow					= ri->Cvar_Get( "r_postprocess_glow",				"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_glowMulti				= ri->Cvar_Get( "r_postprocess_glowMulti",			"1.25",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_glowSat				= ri->Cvar_Get( "r_postprocess_glowSat",			"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_glowSize				= ri->Cvar_Get( "r_postprocess_glowSize",			"1024",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_glowBlurStyle			= ri->Cvar_Get( "r_postprocess_glowBlurStyle",		"2",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_hdr					= ri->Cvar_Get( "r_postprocess_hdr",				"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_hdrExposureBias		= ri->Cvar_Get( "r_postprocess_hdrExposureBias",	"1.0",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_brightPassThres		= ri->Cvar_Get( "r_postprocess_brightPassThres",	"0.75",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_brightPassStyle		= ri->Cvar_Get( "r_postprocess_brightPassStyle",	"3",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_bloom					= ri->Cvar_Get( "r_postprocess_bloom",				"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_bloomMulti			= ri->Cvar_Get( "r_postprocess_bloomMulti",			"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_bloomSat				= ri->Cvar_Get( "r_postprocess_bloomSat",			"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_bloomSceneMulti		= ri->Cvar_Get( "r_postprocess_bloomSceneMulti",	"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_bloomSceneSat			= ri->Cvar_Get( "r_postprocess_bloomSceneSat",		"1",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_bloomSize				= ri->Cvar_Get( "r_postprocess_bloomSize",			"1024",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_bloomSteps			= ri->Cvar_Get( "r_postprocess_bloomSteps",			"3",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_bloomBlurStyle		= ri->Cvar_Get( "r_postprocess_bloomBlurStyle",		"2",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_finalPass				= ri->Cvar_Get( "r_postprocess_finalPass",			"0",								CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_userVec1				= ri->Cvar_Get( "r_postprocess_userVec1",			"0.243137 0.298039 0.452941 0.18",	CVAR_ARCHIVE,				NULL, NULL );
	r_postprocess_debugFBOs				= ri->Cvar_Get( "r_postprocess_debugFBOs",			"0",								CVAR_ARCHIVE,				NULL, NULL );

	r_environmentMapping				= ri->Cvar_Get( "r_environmentMapping",				"1",								CVAR_ARCHIVE,				"Enable environment mapping effects", NULL );
	//~QtZ

	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri->Cmd_AddCommand( "imagelist", R_ImageList_f, NULL );
	ri->Cmd_AddCommand( "shaderlist", R_ShaderList_f, NULL );
	ri->Cmd_AddCommand( "skinlist", R_SkinList_f, NULL );
	ri->Cmd_AddCommand( "modellist", R_Modellist_f, NULL );
	ri->Cmd_AddCommand( "screenshot", R_ScreenShotJPEG_f, NULL );
	ri->Cmd_AddCommand( "screenshot_png", R_ScreenShotPNG_f, NULL );
	ri->Cmd_AddCommand( "screenshot_tga", R_ScreenShotTGA_f, NULL );
	ri->Cmd_AddCommand( "gfxinfo", GfxInfo_f, NULL );
	ri->Cmd_AddCommand( "minimize", GLimp_Minimize, NULL );
	ri->Cmd_AddCommand( "postprocess_reload", R_PostProcessReload_f, NULL );
}

/*
===============
R_Init
===============
*/
void R_Init( void ) {	
	int	err;
	int i;
	byte *ptr;

	ri->Printf( PRINT_ALL, "----- R_Init -----\n" );

	// clear all our internal state
	memset( &tr, 0, sizeof( tr ) );
	memset( &backEnd, 0, sizeof( backEnd ) );
	memset( &tess, 0, sizeof( tess ) );

	if(sizeof(glconfig_t) != 11324)
		ri->Error( ERR_FATAL, "Mod ABI incompatible: sizeof(glconfig_t) == %u != 11324", (unsigned int) sizeof(glconfig_t));

//	Swap_Init();

	if ( (intptr_t)tess.xyz & 15 ) {
		ri->Printf( PRINT_WARNING, "tess.xyz not 16 byte aligned\n" );
	}
	memset( tess.constantColor255, 255, sizeof( tess.constantColor255 ) );

	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[i]		= sinf( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if ( i < FUNCTABLE_SIZE / 2 )
		{
			if ( i < FUNCTABLE_SIZE / 4 )
			{
				tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
		}
	}

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	max_polys = r_maxpolys->integer;
	if (max_polys < MAX_POLYS)
		max_polys = MAX_POLYS;

	max_polyverts = r_maxpolyverts->integer;
	if (max_polyverts < MAX_POLYVERTS)
		max_polyverts = MAX_POLYVERTS;

	ptr = (byte *)ri->Hunk_Alloc( sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
	backEndData = (backEndData_t *) ptr;
	backEndData->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData ));
	backEndData->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys);
	R_ToggleSmpFrame();

	InitOpenGL();

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();

	//QtZ: Post processing
	R_EXT_Init();
	//~QtZ

	err = qglGetError();
	if ( err != GL_NO_ERROR )
		ri->Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);

	// print info
	//QtZ: Only print GFX info if running developer mode
	if ( ri->Cvar_VariableIntegerValue( "com_developer" ) )
		GfxInfo_f();
	ri->Printf( PRINT_ALL, "----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow ) {	

	ri->Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );

	ri->Cmd_RemoveCommand( "modellist" );
	ri->Cmd_RemoveCommand( "screenshot_tga" );
	ri->Cmd_RemoveCommand( "screenshot_png" );
	ri->Cmd_RemoveCommand( "screenshot" );
	ri->Cmd_RemoveCommand( "imagelist" );
	ri->Cmd_RemoveCommand( "shaderlist" );
	ri->Cmd_RemoveCommand( "skinlist" );
	ri->Cmd_RemoveCommand( "gfxinfo" );
	ri->Cmd_RemoveCommand( "minimize" );
	ri->Cmd_RemoveCommand( "modelist" );
	ri->Cmd_RemoveCommand( "shaderstate" );
	ri->Cmd_RemoveCommand( "postprocess_reload" );


	if ( tr.registered ) {
		R_SyncRenderThread();
		//QtZ: Post processing
		R_EXT_Cleanup();
		//~QtZ
		R_DeleteTextures();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if ( destroyWindow ) {
		GLimp_Shutdown();
	}

	tr.registered = qfalse;
}


/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void ) {
	R_SyncRenderThread();
	if (!ri->Sys_LowPhysicalMemory()) {
		RB_ShowImages();
	}
}


/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
Q_EXPORT refexport_t* QDECL GetRefAPI ( int apiVersion, refimport_t *rimp ) {
	static refexport_t re;

	ri = rimp;

	memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		ri->Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel = RE_RegisterModel;
	re.RegisterSkin = RE_RegisterSkin;
	re.RegisterShader = RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.LoadWorld = RE_LoadWorldMap;
	re.SetWorldVisData = RE_SetWorldVisData;
	re.EndRegistration = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame = RE_EndFrame;

	re.MarkFragments = R_MarkFragments;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	re.ClearScene = RE_ClearScene;
	re.AddRefEntityToScene = RE_AddRefEntityToScene;
	re.AddPolyToScene = RE_AddPolyToScene;
	re.LightForPoint = R_LightForPoint;
	re.AddLightToScene = RE_AddLightToScene;
	re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;
	re.RenderScene = RE_RenderScene;

	re.SetColor = RE_SetColor;
	re.DrawStretchPic = RE_StretchPic;
	//QtZ: Added from JA/EF
	re.DrawRotatedPic = RE_RotatedPic;
	//~QtZ
	re.DrawStretchRaw = RE_StretchRaw;
	re.UploadCinematic = RE_UploadCinematic;

	re.RegisterFont = RE_RegisterFont;
	re.RemapShader = R_RemapShader;
	re.GetEntityToken = R_GetEntityToken;
	re.inPVS = R_inPVS;

	re.TakeVideoFrame = RE_TakeVideoFrame;

	return &re;
}
