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

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#define PRODUCT_NAME				"QtZ"
#define PRODUCT_VERSION				"0.4"
#define QTZ_VERSION					PRODUCT_NAME" "PRODUCT_VERSION
#define BASEGAME					"qtz"
#define CLIENT_WINDOW_TITLE     	"QuantiZe ["PRODUCT_VERSION"] "ARCH_STRING
#define CLIENT_WINDOW_MIN_TITLE 	PRODUCT_NAME
#define HOMEPATH_NAME_UNIX			".qtz"
#define HOMEPATH_NAME_WIN			"QtZ"
#define HOMEPATH_NAME_MACOSX		HOMEPATH_NAME_WIN
#define GAMENAME_FOR_MASTER			"QuantiZe"	// must NOT contain whitespace

// Heartbeat for dpmaster protocol. You shouldn't change this unless you know what you're doing
#define HEARTBEAT_FOR_MASTER		"DarkPlaces"

// When com_gamename is LEGACY_MASTER_GAMENAME, use quake3 master protocol.
// You shouldn't change this unless you know what you're doing.
#define LEGACY_MASTER_GAMENAME		"Quake3Arena"
#define LEGACY_HEARTBEAT_FOR_MASTER	"QuakeArena-1"

#define MAX_TEAMNAME		32
#define MAX_MASTER_SERVERS      5	// number of supported master servers

#define DEMO_DIRECTORY	"demos/"
#define DEMO_EXTENSION	"dm_"			// standard demo extension
#define MAX_DEMOLIST	(MAX_DEMOS * MAX_QPATH)

// QTZFIXME: disablewarnings.h
#ifdef _MSC_VER
	#pragma warning( disable: 4018 )    // signed/unsigned mismatch
	#pragma warning( disable: 4100 )	// unreferenced formal parameter
	#pragma warning( disable: 4127 )	// conditional expression is constant
	#pragma warning( disable: 4131 )	// 'x' : uses old-style declarator
	#pragma warning( disable: 4152 )	// nonstandard extension, function/data pointer conversion in expression
	#pragma warning( disable: 4706 )	// assignment within conditional expression
	#pragma warning( disable: 4820 )	// 'n' bytes padding added after data member 'x'
	#pragma warning( disable: 4996 )	// deprecated function

//	#pragma warning( error: 4255 )		// no function prototype given: converting '()' to '(void)'
	#pragma warning( error: 4505 ) 		// unreferenced local function has been removed
	#pragma warning( error: 4013 )		// 'x' undefined; assuming extern returning int
	#pragma warning( error: 4028 )		// formal parameter n different from declaration
	#pragma warning( error: 4090 )		// '=' : different 'const' qualifiers
//	#pragma intrinsic( memset, memcpy )
#endif

//QtZ: Add common headers here
#include "../qcommon/linkedlist.h"

//Ignore __attribute__ on non-gcc platforms
#if !defined(__GNUC__) && !defined(__attribute__)
	#define __attribute__(x)
#endif

#if defined(__GNUC__)
	#define UNUSED_VAR __attribute__((unused))
#else
	#define UNUSED_VAR
#endif

#if defined(_MSC_VER)
	#define Q_EXPORT __declspec(dllexport)
#elif defined(__SUNPRO_C)
	#define Q_EXPORT __global
#elif __GNUC__ >= 3 && !__EMX__ && !sun
	#define Q_EXPORT __attribute__(( visibility( "default" ) ))
#else
	#define Q_EXPORT
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#ifdef _MSC_VER
  #include <io.h>

  typedef __int64 int64_t;
  typedef __int32 int32_t;
  typedef __int16 int16_t;
  typedef __int8 int8_t;
  typedef unsigned __int64 uint64_t;
  typedef unsigned __int32 uint32_t;
  typedef unsigned __int16 uint16_t;
  typedef unsigned __int8 uint8_t;

  // vsnprintf is ISO/IEC 9899:1999
  // abstracting this to make it portable
  size_t Q_vsnprintf( char *str, size_t size, const char *format, va_list ap );
#else
  #include <stdint.h>

  #define Q_vsnprintf vsnprintf
#endif

#include "q_platform.h"
#include "q_asm.h"

//=============================================================

typedef unsigned char byte;

typedef enum qboolean_e { qfalse=0, qtrue } qboolean;

typedef union floatint_u {
	float f;
	int i;
	unsigned int ui;
} floatint_t;

typedef int		qhandle_t;
typedef int		sfxHandle_t;
typedef int		fileHandle_t;
typedef int		clipHandle_t;

#define PAD(base, alignment)	(((base)+(alignment)-1) & ~((alignment)-1))
#define PADLEN(base, alignment)	(PAD((base), (alignment)) - (base))

#define PADP(base, alignment)	((void *) PAD((intptr_t) (base), (alignment)))

#ifdef __GNUC__
	#define QALIGN(x) __attribute__((aligned(x)))
#else
	#define QALIGN(x)
#endif

#ifndef NULL
	#define NULL ((void *)0)
#endif

#define STRING(s)			#s
// expand constants before stringifying them
#define XSTRING(s)			STRING(s)

#define	MAX_QINT			0x7fffffff
#define	MIN_QINT			(-MAX_QINT-1)

#define VALIDSTRING( s )	((s) && *(s))
#define VALIDENT( e )		((e) && ((e)->inuse))

#define ARRAY_LEN(x)		(sizeof(x) / sizeof(*(x)))
#define STRARRAY_LEN(x)		(ARRAY_LEN(x) - 1)

// angle indexes
/*
#define PITCH	0 // up / down
#define YAW		1 // left / right
#define ROLL	2 // fall over
*/

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	1024	// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024

#define	BIG_INFO_STRING		8192  // used for system info key only
#define	BIG_INFO_KEY		8192
#define	BIG_INFO_VALUE		8192


#define	MAX_QPATH			64		// max length of a quake game pathname
#ifdef PATH_MAX
#define MAX_OSPATH			PATH_MAX
#else
#define	MAX_OSPATH			256		// max length of a filesystem pathname
#endif

#define	MAX_NETNAME			36		// max length of a client name

#define	MAX_SAY_TEXT		150

#define DEFAULT_NAME			"Unnamed"
#define	DEFAULT_MODEL			"nooblet"

// paramters for command buffer stuffing
typedef enum cbufExec_e {
	EXEC_NOW,			// don't return until completed, a VM should NEVER use this,
						// because some commands might cause the VM to be unloaded...
	EXEC_INSERT,		// insert at current position, but don't run yet
	EXEC_APPEND			// add to end of the command buffer (normal case)
} cbufExec_t;


//
// these aren't needed by any of the VMs.  put in another header?
//
#define	MAX_MAP_AREA_BYTES		32		// bit vector of area visibility


// print levels from renderer (FIXME: set up for game / cgame?)
typedef enum printParm_e {
	PRINT_ALL,
	PRINT_DEVELOPER,		// only print when "developer 1"
	PRINT_WARNING,
	PRINT_ERROR
} printParm_t;


#ifdef ERR_FATAL
	#undef ERR_FATAL			// this is be defined in malloc.h
#endif

// parameters to the main Error routine
typedef enum errorParm_e {
	ERR_FATAL,					// exit the entire game with a popup window
	ERR_DROP,					// print to console and disconnect from game
	ERR_SERVERDISCONNECT		// don't kill server
} errorParm_t;


// font rendering values used by ui and cgame

#define PROP_GAP_WIDTH			3
#define PROP_SPACE_WIDTH		8
#define PROP_HEIGHT				27
#define PROP_SMALL_SIZE_SCALE	0.75f

#define BLINK_DIVISOR			200
#define PULSE_DIVISOR			(75.0f)

#define UI_LEFT			0x00000000	// default
#define UI_CENTER		0x00000001
#define UI_RIGHT		0x00000002
#define UI_FORMATMASK	0x00000007
#define UI_SMALLFONT	0x00000010
#define UI_BIGFONT		0x00000020	// default
#define UI_GIANTFONT	0x00000040
#define UI_DROPSHADOW	0x00000800
#define UI_BLINK		0x00001000
#define UI_INVERSE		0x00002000
#define UI_PULSE		0x00004000

#if !defined(NDEBUG) && !defined(BSPC)
	#define HUNK_DEBUG
#endif

typedef enum hunkallocPref_e {
	PREF_HIGH=0,
	PREF_LOW,
	PREF_DONTCARE
} hunkallocPref_t;

#ifdef HUNK_DEBUG
#define Hunk_Alloc( size, preference ) Hunk_AllocDebug(size, preference, #size, __FILE__, __LINE__)
void *Hunk_AllocDebug( size_t size, hunkallocPref_t preference, char *label, char *file, int line );
#else
void *Hunk_Alloc( size_t size, hunkallocPref_t preference );
#endif

#define CIN_system	0x01
#define CIN_loop	0x02
#define	CIN_hold	0x04
#define CIN_silent	0x08
#define CIN_shader	0x10

/*
==============================================================

MATHLIB

==============================================================
*/

typedef float number;
typedef int integer;

#ifdef _MSC_VER
	#pragma warning( push )
	#pragma warning( disable : 4201 )
#endif // _MSC_VER

typedef union {
	struct { number x, y; };
	struct { number w, h; };
	// s, t?
	number data[2];
} vector2;
typedef union {
	struct { integer x, y; };
	struct { integer w, h; };
	// s, t?
	integer data[2];
} ivector2;

typedef union {
	struct { number x, y, z; };
	struct { number r, g, b; }; // red, green, blue?
	struct { number pitch, yaw, roll; };
	number data[3];
} vector3;
typedef union {
	struct { integer x, y, z; };
	struct { integer r, g, b; }; // red, green, blue?
	struct { integer pitch, yaw, roll; };
	integer data[3];
} ivector3;

typedef union {
	struct { number x, y, z, w; };
	struct { number r, g, b, a; };
	// red, green, blue, alpha?
	number data[4];
} vector4;
typedef union {
	struct { integer x, y, z, w; };
	struct { integer r, g, b, a; };
	// red, green, blue, alpha?
	integer data[4];
} ivector4;

#ifdef _MSC_VER
	#pragma warning( pop )
#endif // _MSC_VER

typedef	int	fixed4_t, fixed8_t, fixed16_t;

#ifndef M_PI
	#define M_PI 3.14159265358979323846f	// matches value in gcc v2 math.h
#endif

#define NUMVERTEXNORMALS 162
extern vector3 bytedirs[NUMVERTEXNORMALS];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define	SCREEN_WIDTH		(640.0f)
#define	SCREEN_HEIGHT		(480.0f)

#define Q_COLOR_ESCAPE	'^'
#define Q_COLOR_BITS 0xF

#define COLOR_BLACK		'0'
#define COLOR_GREY		'1'
#define COLOR_WHITE		'2'
#define COLOR_RED		'3'
#define COLOR_ORANGE	'4'
#define COLOR_YELLOW	'5'
#define COLOR_LIME		'6'
#define COLOR_GREEN		'7'
#define COLOR_AQUA		'8'
#define COLOR_CYAN		'9'
#define COLOR_LIGHTBLUE	'a'
#define COLOR_BLUE		'b'
#define COLOR_VIOLET	'c'
#define COLOR_PURPLE	'd'
#define COLOR_PINK		'e'

#define S_COLOR_BLACK		"^0"
#define S_COLOR_GREY		"^1"
#define S_COLOR_WHITE		"^2"
#define S_COLOR_RED			"^3"
#define S_COLOR_ORANGE		"^4"
#define S_COLOR_YELLOW		"^5"
#define S_COLOR_LIME		"^6"
#define S_COLOR_GREEN		"^7"
#define S_COLOR_AQUA		"^8"
#define S_COLOR_CYAN		"^9"
#define S_COLOR_LIGHTBLUE	"^a"
#define S_COLOR_BLUE		"^b"
#define S_COLOR_VIOLET		"^c"
#define S_COLOR_PURPLE		"^d"
#define S_COLOR_PINK		"^e"

qboolean Q_IsColorString( const char *p );
int ColorIndex( char c );
extern const vector4 g_color_table[Q_COLOR_BITS+1];

#define	MAKERGB( v, r, g, b ) v[0]=r;v[1]=g;v[2]=b
#define	MAKERGBA( v, r, g, b, a ) v[0]=r;v[1]=g;v[2]=b;v[3]=a

#define DEG2RAD( a ) ( ( (a) * M_PI ) / 180.0F )
#define RAD2DEG( a ) ( ( (a) * 180.0f ) / M_PI )

struct cplane_s;

extern	vector3	vec3_origin;
extern	vector3	axisDefault[3];

#define	nanmask (255<<23)

#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

int Q_isnan(float x);

#ifdef idx64
  extern long qftolsse(float f);
  extern int qvmftolsse(void);

  #define Q_ftol qftolsse

  extern int (*Q_VMftol)(void);
#elif defined(id386)
  extern long QDECL qftolx87(float f);
  extern long QDECL qftolsse(float f);
  extern int QDECL qvmftolx87(void);
  extern int QDECL qvmftolsse(void);

  extern long (QDECL *Q_ftol)(float f);
  extern int (QDECL *Q_VMftol)(void);
#else
  // Q_ftol must expand to a function name so the pluggable renderer can take
  // its address
  #define Q_ftol lrintf
#endif
/*
// if your system does not have lrintf() and round() you can try this block. Please also open a bug report at bugzilla.icculus.org
// or write a mail to the ioq3 mailing list.
#else
  #define Q_ftol(v) ((long) (v))
  #define Q_round(v) do { if((v) < 0) (v) -= 0.5f; else (v) += 0.5f; (v) = Q_ftol((v)); } while(0)
#endif
*/

#ifdef idppc

static QINLINE float Q_rsqrt( float number ) {
		float x = 0.5f * number;
                float y;
#ifdef __GNUC__            
                asm("frsqrte %0,%1" : "=f" (y) : "f" (number));
#else
		y = __frsqrte( number );
#endif
		return y * (1.5f - (x * y * y));
	}

#ifdef __GNUC__            
static QINLINE float Q_fabs(float x) {
    float abs_x;
    
    asm("fabs %0,%1" : "=f" (abs_x) : "f" (x));
    return abs_x;
}
#else
#define Q_fabs __fabsf
#endif

#else
float Q_fabs( float f );
float Q_rsqrt( float f );		// reciprocal square root
#endif

#define SQRTFAST( x ) ( (x) * Q_rsqrt( x ) )

signed char ClampChar( int i );
signed short ClampShort( int i );

// this isn't a real cheap function to call!
int DirToByte( vector3 *dir );
void ByteToDir( int b, vector3 *dir );

void		VectorAdd( const vector3 *vec1, const vector3 *vec2, vector3 *vecOut );
void		VectorSubtract( const vector3 *vec1, const vector3 *vec2, vector3 *vecOut );
void		VectorNegate( const vector3 *vecIn, vector3 *vecOut );
void		VectorScale( const vector3 *vecIn, number scale, vector3 *vecOut );
void		VectorScale4( const vector4 *vecIn, number scale, vector4 *vecOut );
void		VectorScaleVector( const vector3 *vecIn, const vector3 *vecScale, vector3 *vecOut );
void		VectorMA( const vector3 *vec1, number scale, const vector3 *vec2, vector3 *vecOut );
void		VectorLerp( const vector3 *vec1, number frac, const vector3 *vec2, vector3 *vecOut );
void		VectorLerp4( const vector4 *vec1, number frac, const vector4 *vec2, vector4 *vecOut );
number		VectorLength( const vector3 *vec );
number		VectorLengthSquared( const vector3 *vec );
number		Distance( const vector3 *p1, const vector3 *p2 );
number		DistanceSquared( const vector3 *p1, const vector3 *p2 );
void		VectorNormalizeFast( vector3 *vec );
number		VectorNormalize( vector3 *vec );
number		VectorNormalize2( const vector3 *vec, vector3 *vecOut );
void		VectorCopy( const vector3 *vecIn, vector3 *vecOut );
void		IVectorCopy( const ivector3 *vecIn, ivector3 *vecOut );
void		VectorCopy4( const vector4 *vecIn, vector4 *vecOut );
void		VectorSet( vector3 *vec, number x, number y, number z );
void		VectorSet4( vector4 *vec, number x, number y, number z, number w );
void		VectorClear( vector3 *vec );
void		VectorClear4( vector4 *vec );
void		VectorInc( vector3 *vec );
void		VectorDec( vector3 *vec );
void		VectorRotate( vector3 *in, vector3 matrix[3], vector3 *out );
void		VectorInverse( vector3 *vec );
void		CrossProduct( const vector3 *vec1, const vector3 *vec2, vector3 *vecOut );
number		DotProduct( const vector3 *vec1, const vector3 *vec2 );
qboolean	VectorCompare( const vector3 *vec1, const vector3 *vec2 );
void		VectorSnap( vector3 *v );
void		VectorSnapTowards( vector3 *v, vector3 *to );

// TODO
#define VectorInverseScaleVector(a,b,c)	((c)[0]=(a)[0]/(b)[0],(c)[1]=(a)[1]/(b)[1],(c)[2]=(a)[2]/(b)[2])
#define VectorScaleVectorAdd(c,a,b,o)	((o)[0]=(c)[0]+((a)[0]*(b)[0]),(o)[1]=(c)[1]+((a)[1]*(b)[1]),(o)[2]=(c)[2]+((a)[2]*(b)[2]))
#define VectorAdvance(a,s,b,c)			(((c)[0]=(a)[0] + s * ((b)[0] - (a)[0])),((c)[1]=(a)[1] + s * ((b)[1] - (a)[1])),((c)[2]=(a)[2] + s * ((b)[2] - (a)[2])))
#define VectorAverage(a,b,c)			(((c)[0]=((a)[0]+(b)[0])*0.5f),((c)[1]=((a)[1]+(b)[1])*0.5f),((c)[2]=((a)[2]+(b)[2])*0.5f))

unsigned int ColorBytes3( float r, float g, float b );
unsigned int ColorBytes4( float r, float g, float b, float a );

float NormalizeColor( const vector3 *in, vector3 *out );

float RadiusFromBounds( const vector3 *mins, const vector3 *maxs );
void ClearBounds( vector3 *mins, vector3 *maxs );
void AddPointToBounds( const vector3 *v, vector3 *mins, vector3 *maxs );

int Q_log2(int val);

float Q_acos(float c);

int		Q_rand( int *seed );
float	Q_random( int *seed );
float	Q_crandom( int *seed );

void Rand_Init(int seed);
float flrand(float min, float max);
float Q_flrand(float min, float max);
int irand(int min, int max);
int Q_irand(int value1, int value2);
float Q_powf ( float x, int y );

void HSL2RGB( float h, float s, float l, float *r, float *g, float *b );
void HSV2RGB( float h, float s, float v, float *r, float *g, float *b );

#define random()	((rand () & 0x7fff) / ((float)0x7fff))
#define crandom()	(2.0f * (random() - 0.5f))

void vectoangles( const vector3 *value1, vector3 *angles);
void AnglesToAxis( const vector3 *angles, vector3 axis[3] );

void AxisClear( vector3 axis[3] );
void AxisCopy( vector3 in[3], vector3 out[3] );

void SetPlaneSignbits( struct cplane_s *out );
int BoxOnPlaneSide (vector3 *emins, vector3 *emaxs, struct cplane_s *plane);

qboolean BoundsIntersect(const vector3 *mins, const vector3 *maxs, const vector3 *mins2, const vector3 *maxs2);
qboolean BoundsIntersectSphere(const vector3 *mins, const vector3 *maxs, const vector3 *origin, number radius);
qboolean BoundsIntersectPoint(const vector3 *mins, const vector3 *maxs, const vector3 *origin);

float	AngleMod(float a);
float	LerpAngle (float from, float to, float frac);
float	AngleSubtract( float a1, float a2 );
void	AnglesSubtract( vector3 *v1, vector3 *v2, vector3 *v3 );

float AngleNormalize360 ( float angle );
float AngleNormalize180 ( float angle );
float AngleDelta ( float angle1, float angle2 );

qboolean PlaneFromPoints( vector4 *plane, const vector3 *a, const vector3 *b, const vector3 *c );
void ProjectPointOnPlane( vector3 *dst, const vector3 *p, const vector3 *normal );
void RotatePointAroundVector( vector3 *dst, const vector3 *dir, const vector3 *point, float degrees );
void RotateAroundDirection( vector3 axis[3], float yaw );
void MakeNormalVectors( const vector3 *forward, vector3 *right, vector3 *up );
// perpendicular vector could be replaced by this

//int	PlaneTypeForNormal (vector3 *normal);

void MatrixMultiply( vector3 in1[3], vector3 in2[3], vector3 out[3] );
void MatrixTranspose( vector3 matrix[3], vector3 transpose[3] );
void AngleVectors( const vector3 *angles, vector3 *forward, vector3 *right, vector3 *up);
void PerpendicularVector( vector3 *dst, const vector3 *src );

#ifndef MAX
	#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

#ifndef MIN
	#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

//=============================================

float Q_clamp( float min, float value, float max );
int Q_clampi( int min, int value, int max );
float Q_cap( float value, float max );
int Q_capi( int value, int max );
float Q_bump( float min, float value );
int Q_bumpi( int min, int value );

char	*COM_SkipPath( char *pathname );
const char	*COM_GetExtension( const char *name );
void	COM_StripExtension(const char *in, char *out, size_t destsize);
qboolean COM_CompareExtension(const char *in, const char *ext);
void	COM_DefaultExtension( char *path, size_t maxSize, const char *extension );

void	COM_BeginParseSession( const char *name );
int		COM_GetCurrentParseLine( void );
char	*COM_Parse( char **data_p );
char	*COM_ParseExt( const char **data_p, qboolean allowLineBreaks );
ptrdiff_t COM_Compress( char *data_p );
void	COM_ParseError( char *format, ... ) __attribute__ ((format (printf, 1, 2)));
void	COM_ParseWarning( char *format, ... ) __attribute__ ((format (printf, 1, 2)));
qboolean COM_ParseString( const char **data, const char **s );
qboolean COM_ParseInt( const char **data, int *i );
qboolean COM_ParseFloat( const char **data, float *f );
qboolean COM_ParseVec4( const char **buffer, vector4 *c);
//int		COM_ParseInfos( char *buf, int max, char infos[][MAX_INFO_STRING] );

#define MAX_TOKENLENGTH		1024

#ifndef TT_STRING
//token types
#define TT_STRING					1			// string
#define TT_LITERAL					2			// literal
#define TT_NUMBER					3			// number
#define TT_NAME						4			// name
#define TT_PUNCTUATION				5			// punctuation
#endif

typedef struct pc_token_s {
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
} pc_token_t;

// data is an in/out parm, returns a parsed out token

void	COM_MatchToken( char**buf_p, char *match );

qboolean SkipBracedSection( char **program, int depth );
void SkipRestOfLine ( char **data );

void Parse1DMatrix (char **buf_p, int x, float *m);
void Parse2DMatrix (char **buf_p, int y, int x, float *m);
void Parse3DMatrix (char **buf_p, int z, int y, int x, float *m);
int Com_HexStrToInt( const char *str );

size_t QDECL Com_sprintf (char *dest, size_t size, const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

char *Com_SkipTokens( char *s, int numTokens, char *sep );
char *Com_SkipCharset( char *s, char *sep );

void Com_RandomBytes( byte *string, size_t len );

// mode parm for FS_FOpenFile
typedef enum fsMode_e {
	FS_READ,
	FS_WRITE,
	FS_APPEND,
	FS_APPEND_SYNC
} fsMode_t;

typedef enum fsOrigin_e {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

//=============================================

int Q_isprint( int c );
int Q_islower( int c );
int Q_isupper( int c );
int Q_isalpha( int c );
qboolean Q_isanumber( const char *s );
qboolean Q_isintegral( float f );

// portable case insensitive compare
int		Q_stricmp (const char *s1, const char *s2);
int		Q_strncmp (const char *s1, const char *s2, size_t n);
int		Q_stricmpn (const char *s1, const char *s2, size_t n);
char	*Q_strlwr( char *s1 );
char	*Q_strupr( char *s1 );
const char	*Q_stristr( const char *s, const char *find);

// buffer size safe library replacements
void	Q_strncpyz( char *dest, const char *src, size_t destsize );
void	Q_strcat( char *dest, size_t size, const char *src );

void Q_strstrip( char *string, const char *strip, const char *repl );
const char *Q_strchrs( const char *string, const char *search );
char *Q_strrep( const char *string, const char *substr, const char *replacement );
void Q_strrev( char *string );
char *Q_CleanColorStr( char *string );

// strlen that discounts Quake color sequences
int Q_PrintStrlen( const char *string );
// removes color sequences from string
char *Q_CleanStr( char *string );
// Count the number of char tocount encountered in string
int Q_CountChar(const char *string, char tocount);

//=============================================

// 64-bit integers for global rankings interface
// implemented as a struct for qvm compatibility
typedef struct qint64_s {
	byte	b0;
	byte	b1;
	byte	b2;
	byte	b3;
	byte	b4;
	byte	b5;
	byte	b6;
	byte	b7;
} qint64_t;

//=============================================
/*
short	BigShort(short l);
short	LittleShort(short l);
int		BigLong (int l);
int		LittleLong (int l);
qint64_t  BigLong64 (qint64_t l);
qint64_t  LittleLong64 (qint64_t l);
float	BigFloat (const float *l);
float	LittleFloat (const float *l);

void	Swap_Init (void);
*/
char	* QDECL va(char *format, ...) __attribute__ ((format (printf, 1, 2)));

#define TRUNCATE_LENGTH	64
void Com_TruncateLongString( char *buffer, const char *s );

//=============================================

//
// key / value info strings
//
char *Info_ValueForKey( const char *s, const char *key );
void Info_RemoveKey( char *s, const char *key );
void Info_RemoveKey_Big( char *s, const char *key );
void Info_SetValueForKey( char *s, const char *key, const char *value );
void Info_SetValueForKey_Big( char *s, const char *key, const char *value );
qboolean Info_Validate( const char *s );
void Info_NextPair( const char **s, char *key, char *value );

// this is only here so the functions in q_shared.c and bg_*.c can link
#if defined( PROJECT_GAME ) || defined( PROJECT_CGAME ) || defined( PROJECT_UI )
	void (*Com_Error)	( int level, const char *error, ... ) __attribute__ ((noreturn, format(printf, 2, 3)));
	void (*Com_Printf)	( const char *msg, ... ) __attribute__ ((format (printf, 1, 2)));
#else
	void	QDECL Com_Error( int level, const char *error, ... ) __attribute__ ((noreturn, format(printf, 2, 3)));
	void	QDECL Com_Printf( const char *msg, ... ) __attribute__ ((format (printf, 1, 2)));
#endif

vector3 *tv( float x, float y, float z );
char *vtos( const vector3 *v );

/*
==========================================================

CVARS (console variables)

Many variables can be used for cheating purposes, so when
cheats is zero, force all unspecified variables to their
default values.
==========================================================
*/

#define CVAR_NONE			0x00000000
#define	CVAR_ARCHIVE		0x00000001	// set to cause it to be saved to vars.rc - used for system variables, not for player specific configurations
#define	CVAR_USERINFO		0x00000002	// sent to server on connect or change
#define	CVAR_SERVERINFO		0x00000004	// sent in response to front end requests
#define	CVAR_SYSTEMINFO		0x00000008	// these cvars will be duplicated on all clients
#define	CVAR_INIT			0x00000010	// don't allow change from console at all, but can be set from the command line
#define	CVAR_LATCH			0x00000020	// will only change when C code next does a Cvar_Get(), so it can't be changed without proper initialization. modified will be set, even though the value hasn't changed yet
#define	CVAR_ROM			0x00000040	// display only, cannot be set by user at all
#define	CVAR_USER_CREATED	0x00000080	// created by a set command
#define	CVAR_TEMP			0x00000100	// can be set even when cheats are disabled, but is not archived
#define CVAR_CHEAT			0x00000200	// can not be changed if cheats are disabled
#define CVAR_NORESTART		0x00000400	// do not clear when a cvar_restart is issued
#define CVAR_SERVER_CREATED	0x00000800	// cvar was created by a server the client connected to.
#define CVAR_PROTECTED		0x00001000	// prevent modifying this var from VMs or the server
// These flags are only returned by the Cvar_Flags() function
#define CVAR_MODIFIED		0x40000000	// Cvar was modified
#define CVAR_NONEXISTENT	0x80000000	// Cvar doesn't exist.

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s {
	char			*name;
	char			*string;
	char			*resetString;		// cvar_restart will reset to this value
	char			*latchedString;		// for CVAR_LATCH vars
	int				flags;
	char			*description;
	void			(*update)( void );

	// attributes
	qboolean		modified;			// set each time the cvar is changed
	int				modificationCount;	// incremented each time the cvar is changed
	qboolean		validate;
	qboolean		integral;
	float			min, max;

	// alternate values
	float			value;				// atof( string )
	int				integer;			// atoi( string )
	qboolean		boolean;			// !!atoi( string )

	// internal storage data
	struct cvar_s	*next;
	struct cvar_s	*prev;
	struct cvar_s	*hashNext;
	struct cvar_s	*hashPrev;
	int				hashIndex;
} cvar_t;

#define	MAX_CVAR_VALUE_STRING	256

typedef int	cvarHandle_t;

/*
==============================================================

VoIP

==============================================================
*/

// if you change the count of flags be sure to also change VOIP_FLAGNUM
#define VOIP_SPATIAL		0x01		// spatialized voip message
#define VOIP_DIRECT		0x02		// non-spatialized voip message

// number of flags voip knows. You will have to bump protocol version number if you
// change this.
#define VOIP_FLAGCNT		2

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

#include "surfaceflags.h"			// shared with the q3map utility

// plane types are used to speed some tests
// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2
#define	PLANE_NON_AXIAL	3

extern QINLINE int PlaneTypeForNormal( vector3 *normal );

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s {
	vector3	normal;
	float	dist;
	byte	type;			// for fast side tests: 0,1,2 = axial, 3 = nonaxial
	byte	signbits;		// signx + (signy<<1) + (signz<<2), used as lookup during collision
	byte	pad[2];
} cplane_t;


// a trace is returned when a box is swept through the world
typedef struct trace_s {
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	float		fraction;	// time completed, 1.0 = didn't hit anything
	vector3		endpos;		// final position
	cplane_t	plane;		// surface normal at impact, transformed to world space
	int			surfaceFlags;	// surface hit
	int			contents;	// contents on other side of surface hit
	int			entityNum;	// entity the contacted sirface is a part of
} trace_t;

// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD


// markfragments are returned by R_MarkFragments()
typedef struct markFragment_s {
	int		firstPoint;
	int		numPoints;
} markFragment_t;



typedef struct orientation_s {
	vector3		origin;
	vector3		axis[3];
} orientation_t;

//=====================================================================


// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
#define KEYCATCH_CONSOLE		0x0001
#define	KEYCATCH_UI				0x0002
#define	KEYCATCH_CGAME			0x0008


// sound channels
// channel 0 never willingly overrides
// other channels will allways override a playing sound on that channel
typedef enum soundChannel_e {
	CHAN_AUTO,
	CHAN_LOCAL,		// menu sounds, etc
	CHAN_WEAPON,
	CHAN_VOICE,
	CHAN_ITEM,
	CHAN_BODY,
	CHAN_LOCAL_SOUND,	// chat messages, etc
	CHAN_ANNOUNCER		// announcer voices, etc
} soundChannel_t;


/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/

#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0f/65536))

#define	SNAPFLAG_RATE_DELAYED	1
#define	SNAPFLAG_NOT_ACTIVE		2	// snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT	4	// toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define	MAX_CLIENTS			16		// absolute limit
#define MAX_LOCATIONS		64

#define	GENTITYNUM_BITS		10		// don't need to send any more
#define	MAX_GENTITIES		(1<<GENTITYNUM_BITS)

// entitynums are communicated with GENTITY_BITS, so any reserved
// values that are going to be communcated over the net need to
// also be in this range
#define	ENTITYNUM_NONE		(MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD		(MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES-2)


#define	MAX_MODELS			256		// these are sent over the net as 8 bits
#define	MAX_SOUNDS			256		// so they cannot be blindly increased


#define	MAX_CONFIGSTRINGS	1024

// these two are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define	CS_SERVERINFO		0		// an info string with all the serverinfo cvars
#define	CS_SYSTEMINFO		1		// an info string for server system to client system configuration (timescale, etc)

#define	RESERVED_CONFIGSTRINGS	2	// game can't modify below this, only the system can

#define	MAX_GAMESTATE_CHARS	16000
typedef struct gameState_s {
	int			stringOffsets[MAX_CONFIGSTRINGS];
	char		stringData[MAX_GAMESTATE_CHARS];
	int			dataCount;
} gameState_t;


// gametypes

#define DEFAULT_GAMETYPE "duel"

typedef enum gametype_e {
	GT_BLOODBATH=0,
	GT_DUEL,
	// team games
	GT_TEAMBLOOD,
	GT_FLAGS,
	GT_TROJAN,
	GT_NUM_GAMETYPES
} gametype_t;

// gametype bits
#define GTB_NONE			0x00 // invalid
#define GTB_BLOOD			0x01 // bloodbath
#define GTB_DUEL			0x02 // duel
#define GTB_NOTTEAM			0x03 // **SPECIAL: All of the above gametypes, i.e. not team-based
#define GTB_TEAMBLOOD		0x04 // team bloodbath
#define GTB_FLAGS			0x08 // flags
#define GTB_TROJAN			0x10 // trojan
#define GTB_ALL				0x1F // **SPECIAL: All

extern const char *gametypeStringShort[GT_NUM_GAMETYPES];
const char *GametypeStringForID( int gametype );
int GametypeIDForString( const char *gametype );

//=========================================================

// bit field limits
#define	MAX_STATS				16
#define	MAX_PERSISTANT			16
#define	MAX_POWERUPS			16
#define	MAX_WEAPONS				16		

#define	MAX_PS_EVENTS			2

#define PS_PMOVEFRAMECOUNTBITS	6

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
typedef struct playerState_s {
	int			commandTime;	// cmd->serverTime of last executed command
	int			pm_type;
	int			bobCycle;		// for view bobbing and footstep generation
	int			pm_flags;		// ducked, jump_held, etc
	int			pm_time;

	vector3		origin;
	vector3		velocity;
	int			weaponTime;
	ivector3	delta_angles;	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters

	int			groundEntityNum;// ENTITYNUM_NONE = in air

	// don't change low priority animations until this runs out
	int			legsTimer, torsoTimer;
	// mask off ANIM_TOGGLEBIT
	int			legsAnim, torsoAnim;

	int			movementDir;	// a number 0 to 7 that represents the relative angle
								// of movement to the view angle (axial and diagonals)
								// when at rest, the value will remain unchanged
								// used to twist the legs during strafing

	int			eFlags;			// copied to entityState_t->eFlags

	int			eventSequence;	// pmove generated events
	int			events[MAX_PS_EVENTS];
	int			eventParms[MAX_PS_EVENTS];

	int			externalEvent;	// events set on player from another source
	int			externalEventParm;
	int			externalEventTime;

	int			clientNum;		// ranges from 0 to MAX_CLIENTS-1
	int			weapon;			// copied to entityState_t->weapon
	int			weaponstate;

	vector3		viewangles;		// for fixed views
	int			viewheight;

	// damage feedback
	int			damageEvent;	// when it changes, latch the other parms
	int			damageYaw;
	int			damagePitch;
	int			damageCount;

	int			stats[MAX_STATS];
	int			persistant[MAX_PERSISTANT];	// stats that aren't cleared on death
	int			powerups[MAX_POWERUPS];	// level.time that the powerup runs out
	int			ammo[MAX_WEAPONS];

	int			generic1;
	int			loopSound;
	int			jumppad_ent;	// jumppad entity hit this frame

	// not communicated over the net at all
	int			ping;			// server to game info for scoreboard
	int			pmove_framecount;	// FIXME: don't transmit over the network
	int			jumppad_frame;
	int			entityEventSequence;

	int			bunnyHopTime;
	int			doubleJumpTime;
	int			wallJumpTime;
	int			weaponCooldown[MAX_WEAPONS];
} playerState_t;


//====================================================================


//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define	BUTTON_ATTACK		0x01
#define BUTTON_ALT_ATTACK	0x02
#define	BUTTON_TALK			0x04
#define	BUTTON_USE_HOLDABLE	0x08
#define	BUTTON_GESTURE		0x10
#define	BUTTON_WALKING		0x20
#define	BUTTON_ANY			0x40 // any key at all

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s {
	int				serverTime;
	short			angles[3];
	byte 			buttons;
	byte			weapon;           // weapon 
	signed char		forwardmove, rightmove, upmove;
} usercmd_t;

//===================================================================

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define	SOLID_BMODEL	0xffffff

typedef enum trType_e {
	TR_STATIONARY=0,
	TR_INTERPOLATE,				// non-parametric, but interpolate between snapshots
	TR_LINEAR,
	TR_LINEAR_STOP,
	TR_SINE,					// value = base + sin( time / duration ) * delta
	TR_GRAVITY
} trType_t;

typedef struct trajectory_s {
	trType_t	trType;
	int			trTime;
	int			trDuration;			// if non 0, trTime + trDuration = stop time
	vector3		trBase;
	vector3		trDelta;			// velocity, etc
} trajectory_t;

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large

typedef struct entityState_s {
	int				number;			// entity index
	int				eType;			// entityType_t
	int				eFlags;

	trajectory_t	pos, apos;		// for calculating angles

	int				time, time2;

	vector3			origin, origin2;
	vector3			angles, angles2;

	int				otherEntityNum, otherEntityNum2;

	int				groundEntityNum;// ENTITYNUM_NONE = in air

	int				constantLight;	// r + (g<<8) + (b<<16) + (intensity<<24)
	int				loopSound;		// constantly loop this sound

	int				modelindex, modelindex2;
	int				clientNum;		// 0 to (MAX_CLIENTS - 1), for players and corpses
	int				frame;

	int				solid;			// for client side prediction, trap->SV_LinkEntity sets this properly

	int				event;			// impulse events -- muzzle flashes, footsteps, etc
	int				eventParm;

	// for players
	int				powerups;		// bit flags
	int				weapon;			// determines weapon and flash model, etc
	int				legsAnim, torsoAnim; // mask off ANIM_TOGGLEBIT

	int				generic1;
} entityState_t;

typedef enum connState_e {
	CA_UNINITIALIZED=0,
	CA_DISCONNECTED, 	// not talking to a server
	CA_AUTHORIZING,		// not used any more, was checking cd key 
	CA_CONNECTING,		// sending request packets to the server
	CA_CHALLENGING,		// sending challenge packets to the server
	CA_CONNECTED,		// netchan_t established, getting gamestate
	CA_LOADING,			// only during cgame initialization, never during main loop
	CA_PRIMED,			// got gamestate, waiting for first frame
	CA_ACTIVE,			// game views should be displayed
	CA_CINEMATIC		// playing a cinematic or a static pic, not connected to a server
} connState_t;

// font support 

#define GLYPH_START 0
#define GLYPH_END 255
#define GLYPH_CHARSTART 32
#define GLYPH_CHAREND 127
#define GLYPHS_PER_FONT GLYPH_END - GLYPH_START + 1
typedef struct glyphInfo_s {
	int			height;       // number of scan lines
	int			top;          // top of glyph in buffer
	int			bottom;       // bottom of glyph in buffer
	int			pitch;        // width for copying
	int			xSkip;        // x adjustment
	int			imageWidth;   // width of actual image
	int			imageHeight;  // height of actual image
	float		s;          // x offset in image where glyph starts
	float		t;          // y offset in image where glyph starts
	float		s2;
	float		t2;
	qhandle_t	glyph;  // handle to the shader with the glyph
	char		shaderName[32];
} glyphInfo_t;

typedef struct fontInfo_s {
	glyphInfo_t	glyphs[GLYPHS_PER_FONT];
	float		glyphScale;
	char		name[MAX_QPATH];
} fontInfo_t;

#define Square(x) ((x)*(x))

// real time
//=============================================


typedef struct qtime_s {
	int tm_sec;     /* seconds after the minute - [0,59] */
	int tm_min;     /* minutes after the hour - [0,59] */
	int tm_hour;    /* hours since midnight - [0,23] */
	int tm_mday;    /* day of the month - [1,31] */
	int tm_mon;     /* months since January - [0,11] */
	int tm_year;    /* years since 1900 */
	int tm_wday;    /* days since Sunday - [0,6] */
	int tm_yday;    /* days since January 1 - [0,365] */
	int tm_isdst;   /* daylight savings time flag */
} qtime_t;

// server browser sources
// TTimo: AS_MPLAYER is no longer used
typedef enum serverBrowserSource_e {
	AS_LOCAL=0,
	AS_MPLAYER,
	AS_GLOBAL,
	AS_FAVORITES
} serverBrowserSource_t;

// cinematic states
typedef enum cinState_e {
	FMV_IDLE,
	FMV_PLAY,		// play
	FMV_EOF,		// all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
} cinState_t;

typedef enum _flag_status {
	FLAG_ATBASE = 0,
	FLAG_TAKEN,			// Flags
	FLAG_TAKEN_RED,		// Trojan
	FLAG_TAKEN_BLUE,	// Trojan
	FLAG_DROPPED
} flagStatus_t;

#define	MAX_GLOBAL_SERVERS				4096
#define	MAX_OTHER_SERVERS					128
#define MAX_PINGREQUESTS					32
#define MAX_SERVERSTATUSREQUESTS	16

#define SAY_ALL		0
#define SAY_TEAM	1
#define SAY_TELL	2

#define LERP( a, b, w ) ( ( a ) * ( 1.0f - ( w ) ) + ( b ) * ( w ) )
#define LERP2( a, b, w ) ( (a) + (w)*((b) - (a)) )
#define LUMA( red, green, blue ) ( 0.2126f * ( red ) + 0.7152f * ( green ) + 0.0722f * ( blue ) )

//QtZ: Generic string<->ID struct, used for cleaning up shader loading atm
typedef struct stringToEnum_s {
	const char *s;
	unsigned int e;
} stringToEnum_t;

#define	MAX_EDIT_LINE	256
typedef struct field_s {
	int		cursor;
	int		scroll;
	int		widthInChars;
	char	buffer[MAX_EDIT_LINE];
} field_t;

void Field_Clear( field_t *edit );
