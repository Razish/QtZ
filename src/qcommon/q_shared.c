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
// q_shared.c -- stateless support routines that are included in each code dll
#include "q_shared.h"

/*
 1	0	black
 2	1	grey
 3	2	white
 4	3	red
 5	4	orange
 6	5	yellow
 7	6	lime
 8	7	green
 9	8	aqua
10	9	cyan
11	a	light blue
12	b	blue
13	c	violet
14	d	purple
15	e	pink
16	f	*white, to be changed
*/

const vector4 g_color_table[Q_COLOR_BITS+1] = {
	{ 0.0f, 0.0f, 0.0f, 1.0f }, // black
	{ 0.5f, 0.5f, 0.5f, 1.0f }, // grey
	{ 1.0f, 1.0f, 1.0f, 1.0f }, // white
	{ 1.0f, 0.0f, 0.0f, 1.0f }, // red
	{ 1.0f, 0.5f, 0.0f, 1.0f }, // orange
	{ 1.0f, 1.0f, 0.0f, 1.0f }, // yellow
	{ 0.5f, 1.0f, 0.0f, 1.0f }, // lime
	{ 0.0f, 1.0f, 0.0f, 1.0f }, // green
	{ 0.0f, 1.0f, 0.5f, 1.0f }, // aqua
	{ 0.0f, 1.0f, 1.0f, 1.0f }, // cyan
	{ 0.0f, 0.5f, 1.0f, 1.0f }, // light blue
	{ 0.0f, 0.0f, 1.0f, 1.0f }, // blue
	{ 0.5f, 0.0f, 1.0f, 1.0f }, // violet
	{ 1.0f, 0.0f, 1.0f, 1.0f }, // purple
	{ 1.0f, 0.0f, 0.5f, 1.0f }, // pink
	{ 1.0f, 1.0f, 1.0f, 1.0f }, // *white
};

qboolean Q_IsColorString( const char *p ) {
	if ( p && p[0] && p[0] == Q_COLOR_ESCAPE && p[1] ) {
		int c = tolower( p[1] );
		if ( isdigit( p[1] ) || (c >= 'a' && c <= 'e') )
			return qtrue;
	}

	return qfalse;
}

char ColorIndex( char c ) {
	int lowc = tolower(c);

	if ( c >= '0' && c <= '9' )
		return (c-'0') & Q_COLOR_BITS;
	else if ( lowc >= 'a' && lowc <= 'e' )
		return (lowc-'a'+'9') & Q_COLOR_BITS;

	return ColorIndex( COLOR_WHITE );
}

char *Q_CleanColorStr( char *string ) {
	char *d, *s;
	int c;
	ptrdiff_t len = strlen( string );

	s = d = string;
	while ( len > s-string && (c = *s) != 0 )
	{
		if ( Q_IsColorString( s ) )
			s += 2;
		
		*d = *s;

		d++;
		s++;
	}

	*d = '\0';

	return string;
}

float Q_clamp( float min, float value, float max ) {
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

int Q_clampi( int min, int value, int max ) {
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

// min
float Q_cap( float value, float max ) {
	return (value > max) ? max : value;
}

int Q_capi( int value, int max ) {
	return (value > max) ? max : value;
}

// max
float Q_bump( float min, float value ) {
	return (value < min) ? min : value;
}

int Q_bumpi( int min, int value ) {
	return (value < min) ? min : value;
}

char *COM_SkipPath (char *pathname) {
	char	*last;
	
	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return last;
}

const char *COM_GetExtension( const char *name ) {
	const char *dot = strrchr(name, '.'), *slash;
	if (dot && (!(slash = strrchr(name, '/')) || slash < dot))
		return dot + 1;
	else
		return "";
}

void COM_StripExtension( const char *in, char *out, size_t destsize ) {
	const char *dot = strrchr(in, '.'), *slash;
	if (dot && (!(slash = strrchr(in, '/')) || slash < dot))
		Q_strncpyz(out, in, (destsize < dot-in+1 ? destsize : dot-in+1));
	else
		Q_strncpyz(out, in, destsize);
}

// string compare the end of the strings and return qtrue if strings match
qboolean COM_CompareExtension(const char *in, const char *ext) {
	size_t inlen, extlen;
	
	inlen = strlen(in);
	extlen = strlen(ext);
	
	if(extlen <= inlen)
	{
		in += inlen - extlen;
		
		if(!Q_stricmp(in, ext))
			return qtrue;
	}
	
	return qfalse;
}

// if path doesn't have an extension, then append the specified one (which should include the .)
void COM_DefaultExtension( char *path, size_t maxSize, const char *extension ) {
	const char *dot = strrchr(path, '.'), *slash;
	if (dot && (!(slash = strrchr(path, '/')) || slash < dot))
		return;
	else
		Q_strcat(path, maxSize, extension);
}


QINLINE int PlaneTypeForNormal( vector3 *normal ) {
		 if ( normal->x == 1.0f )	return PLANE_X;
	else if ( normal->y == 1.0f )	return PLANE_Y;
	else if ( normal->z == 1.0f )	return PLANE_Z;
	else							return PLANE_NON_AXIAL;
}

void CopyShortSwap(void *dest, void *src) {
	byte *to = dest, *from = src;

	to[0] = from[1];
	to[1] = from[0];
}

void CopyLongSwap(void *dest, void *src) {
	byte *to = dest, *from = src;

	to[0] = from[3];
	to[1] = from[2];
	to[2] = from[1];
	to[3] = from[0];
}

short   ShortSwap (short l) {
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short	ShortNoSwap (short l) {
	return l;
}

int    LongSwap (int l) {
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int	LongNoSwap (int l) {
	return l;
}

qint64_t Long64Swap (qint64_t ll) {
	qint64_t	result;

	result.b0 = ll.b7;
	result.b1 = ll.b6;
	result.b2 = ll.b5;
	result.b3 = ll.b4;
	result.b4 = ll.b3;
	result.b5 = ll.b2;
	result.b6 = ll.b1;
	result.b7 = ll.b0;

	return result;
}

qint64_t Long64NoSwap (qint64_t ll) {
	return ll;
}

float FloatSwap (const float *f) {
	floatint_t out;

	out.f = *f;
	out.ui = LongSwap(out.ui);

	return out.f;
}

float FloatNoSwap (const float *f) {
	return *f;
}

/*

	PARSING

*/

static	char	com_token[MAX_TOKEN_CHARS];
static	char	com_parsename[MAX_TOKEN_CHARS];
static	int		com_lines;
static	int		com_tokenline;

void COM_BeginParseSession( const char *name ) {
	com_lines = 1;
	com_tokenline = 0;
	Com_sprintf(com_parsename, sizeof(com_parsename), "%s", name);
}

int COM_GetCurrentParseLine( void ) {
	if ( com_tokenline )
		return com_tokenline;
	return com_lines;
}

char *COM_Parse( const char **data_p )
{
	return COM_ParseExt( data_p, qtrue );
}

void COM_ParseError( char *format, ... ) {
	va_list argptr;
	static char string[4096];

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof(string), format, argptr);
	va_end (argptr);

	Com_Printf("ERROR: %s, line %d: %s\n", com_parsename, COM_GetCurrentParseLine(), string);
}

void COM_ParseWarning( char *format, ... ) {
	va_list argptr;
	static char string[4096];

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof(string), format, argptr);
	va_end (argptr);

	Com_Printf("WARNING: %s, line %d: %s\n", com_parsename, COM_GetCurrentParseLine(), string);
}

// Parse a token out of a string
//	Will never return NULL, just empty strings
//	If "allowLineBreaks" is qtrue then an empty string will be returned if the next token is a newline.
const char *SkipWhitespace( const char *data, qboolean *hasNewLines ) {
	int c;

	while( (c = *data) <= ' ') {
		if( !c ) {
			return NULL;
		}
		if( c == '\n' ) {
			com_lines++;
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}

ptrdiff_t COM_Compress( char *data_p ) {
	char *in, *out;
	int c;
	qboolean newline = qfalse, whitespace = qfalse;

	assert( data_p );

	in = out = data_p;
	if (in) {
		while ((c = *in) != 0) {
			// skip double slash comments
			if ( c == '/' && in[1] == '/' ) {
				while (*in && *in != '\n') {
					in++;
				}
			// skip /* */ comments
			} else if ( c == '/' && in[1] == '*' ) {
				while ( *in && ( *in != '*' || in[1] != '/' ) ) 
					in++;
				if ( *in ) 
					in += 2;
				// record when we hit a newline
			} else if ( c == '\n' || c == '\r' ) {
				newline = qtrue;
				in++;
				// record when we hit whitespace
			} else if ( c == ' ' || c == '\t') {
				whitespace = qtrue;
				in++;
				// an actual token
			} else {
				// if we have a pending newline, emit it (and it counts as whitespace)
				if (newline) {
					*out++ = '\n';
					newline = qfalse;
					whitespace = qfalse;
				} if (whitespace) {
					*out++ = ' ';
					whitespace = qfalse;
				}

				// copy quoted strings unmolested
				if (c == '"') {
					*out++ = (char)c;
					in++;
					while (1) {
						c = *in;
						if (c && c != '"') {
							*out++ = (char)c;
							in++;
						} else {
							break;
						}
					}
					if (c == '"') {
						*out++ = (char)c;
						in++;
					}
				} else {
					*out = (char)c;
					out++;
					in++;
				}
			}
		}

		*out = 0;
	}
	return out - data_p;
}

char *COM_ParseExt( const char **data_p, qboolean allowLineBreaks ) {
	int c = 0, len;
	qboolean hasNewLines = qfalse;
	const char *data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;
	com_tokenline = 0;

	// make sure incoming data is valid
	if ( !data )
	{
		*data_p = NULL;
		return com_token;
	}

	while ( 1 )
	{
		// skip whitespace
		data = SkipWhitespace( data, &hasNewLines );
		if ( !data )
		{
			*data_p = NULL;
			return com_token;
		}
		if ( hasNewLines && !allowLineBreaks )
		{
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			data += 2;
			while (*data && *data != '\n') {
				data++;
			}
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			data += 2;
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				if ( *data == '\n' )
					com_lines++;
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	// token starts on this line
	com_tokenline = com_lines;

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if ( c == '\n' )
				com_lines++;
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_token[len] = (char)c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS - 1)
		{
			com_token[len] = (char)c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32);

	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

qboolean COM_ParseString( const char **data, const char **s )  {
//	*s = COM_ParseExt( data, qtrue );
	*s = COM_ParseExt( data, qfalse );
	if ( s[0] == 0 ) 
	{
		Com_Printf("unexpected EOF\n");
		return qtrue;
	}
	return qfalse;
}

qboolean COM_ParseInt( const char **data, int *i )  {
	const char	*token;

	token = COM_ParseExt( data, qfalse );
	if ( token[0] == 0 ) 
	{
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	*i = atoi( token );
	return qfalse;
}

qboolean COM_ParseFloat( const char **data, float *f )  {
	const char	*token;

	token = COM_ParseExt( data, qfalse );
	if ( token[0] == 0 ) 
	{
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	*f = (float)atof( token );
	return qfalse;
}

qboolean COM_ParseVec4( const char **buffer, vector4 *c)  {
	int i;
	float f;

	for ( i=0; i<4; i++ ) {
		if (COM_ParseFloat(buffer, &f)) 
		{
			return qtrue;
		}
		c->data[i] = f;
	}
	return qfalse;
}

void COM_MatchToken( const char **buf_p, const char *match ) {
	char	*token;

	token = COM_Parse( buf_p );
	if ( strcmp( token, match ) ) {
		Com_Error( ERR_DROP, "MatchToken: %s != %s", token, match );
	}
}


// The next token should be an open brace or set depth to 1 if already parsed it.
//	Skips until a matching close brace is found.
//	Internal brace depths are properly skipped.
qboolean SkipBracedSection( const char **program, int depth ) {
	char *token;

	do {
		token = COM_ParseExt( program, qtrue );
		if ( token[1] == '\0' ) {
				 if ( token[0] == '{' )
				depth++;
			else if ( token[0] == '}' )
				depth--;
		}
	} while ( depth && *program );

	return (depth == 0);
}

void SkipRestOfLine ( char **data ) {
	char	*p;
	int		c;

	p = *data;
	while ( (c = *p++) != 0 ) {
		if ( c == '\n' ) {
			com_lines++;
			break;
		}
	}

	*data = p;
}


void Parse1DMatrix( const char **buf_p, int x, float *m ) {
	char	*token;
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < x ; i++) {
		token = COM_Parse(buf_p);
		m[i] = (float)atof(token);
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse2DMatrix (const char **buf_p, int y, int x, float *m) {
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < y ; i++) {
		Parse1DMatrix (buf_p, x, m + i * x);
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse3DMatrix (const char **buf_p, int z, int y, int x, float *m) {
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < z ; i++) {
		Parse2DMatrix (buf_p, y, x, m + i * x*y);
	}

	COM_MatchToken( buf_p, ")" );
}

int Com_HexStrToInt( const char *str ) {
	if ( !str || !str[ 0 ] )
		return -1;

	// check for hex code
	if ( str[ 0 ] == '0' && str[ 1 ] == 'x' ) {
		int n = 0;
		unsigned int i;

		for ( i=2; i<strlen( str ); i++ ) {
			char digit;

			n *= 16;

			digit = (char)tolower( str[ i ] );

			if( digit >= '0' && digit <= '9' )
				digit -= '0';
			else if( digit >= 'a' && digit <= 'f' )
				digit = digit - 'a' + 10;
			else
				return -1;

			n += digit;
		}

		return n;
	}

	return -1;
}

/*

	LIBRARY REPLACEMENT FUNCTIONS

*/

int Q_isprint( int c ) {
	if ( c >= 0x20 && c <= 0x7E )
		return ( 1 );
	return ( 0 );
}

int Q_islower( int c ) {
	if (c >= 'a' && c <= 'z')
		return ( 1 );
	return ( 0 );
}

int Q_isupper( int c ) {
	if (c >= 'A' && c <= 'Z')
		return ( 1 );
	return ( 0 );
}

int Q_isalpha( int c ) {
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return ( 1 );
	return ( 0 );
}

qboolean Q_isanumber( const char *s ) {
	char *p;
	double UNUSED_VAR d;

	if( *s == '\0' )
		return qfalse;

	d = strtod( s, &p );

	return *p == '\0';
}

qboolean Q_isintegral( float f ) {
	return (int)f == f;
}

#ifdef _MSC_VER
// Special wrapper function for Microsoft's broken _vsnprintf() function.
//	MinGW comes with its own snprintf() which is not broken.
size_t Q_vsnprintf(char *str, size_t size, const char *format, va_list ap) {
	int retval;
	
	retval = _vsnprintf(str, size, format, ap);

	if(retval < 0 || retval == (signed)size)
	{
		// Microsoft doesn't adhere to the C99 standard of vsnprintf,
		// which states that the return value must be the number of
		// bytes written if the output string had sufficient length.
		//
		// Obviously we cannot determine that value from Microsoft's
		// implementation, so we have no choice but to return size.
		
		str[size - 1] = '\0';
		return size;
	}
	
	return (unsigned)retval;
}
#endif

// Safe strncpy that ensures a trailing zero
void Q_strncpyz( char *dest, const char *src, size_t destsize ) {
  if ( !dest ) {
    Com_Error( ERR_FATAL, "Q_strncpyz: NULL dest" );
  }
	if ( !src ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
	}
	if ( destsize < 1 ) {
		Com_Error(ERR_FATAL,"Q_strncpyz: destsize < 1" ); 
	}

	strncpy( dest, src, destsize-1 );
  dest[destsize-1] = 0;
}
                 
int Q_stricmpn (const char *s1, const char *s2, size_t n) {
	int		c1, c2;

        if ( s1 == NULL ) {
           if ( s2 == NULL )
             return 0;
           else
             return -1;
        }
        else if ( s2==NULL )
          return 1;


	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}
		
		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);
	
	return 0;		// strings are equal
}

int Q_strncmp (const char *s1, const char *s2, size_t n) {
	int		c1, c2;
	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}
		
		if (c1 != c2) {
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);
	
	return 0;		// strings are equal
}

int Q_stricmp (const char *s1, const char *s2) {
	return (s1 && s2) ? Q_stricmpn (s1, s2, 99999) : -1;
}


char *Q_strlwr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = (char)tolower(*s);
		s++;
	}
    return s1;
}

char *Q_strupr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = (char)toupper(*s);
		s++;
	}
    return s1;
}


// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, size_t size, const char *src ) {
	size_t l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
	}
	Q_strncpyz( dest + l1, src, size - l1 );
}

/*
* Find the first occurrence of find in s.
*/
const char *Q_stristr( const char *s, const char *find ) {
	char c, sc;
	size_t len;
	
	if ( (c = *find++) != 0 ) {
		if (c >= 'a' && c <= 'z')
			c -= ('a' - 'A');

		len = strlen( find );
		do {
			do {
				if ( !(sc = *s++) )
					return NULL;
				if (sc >= 'a' && sc <= 'z')
					sc -= ('a' - 'A');
			} while ( sc != c );
		} while ( Q_stricmpn( s, find, len ) );
		s--;
	}
	return s;
}


int Q_PrintStrlen( const char *string ) {
	int			len;
	const char	*p;

	if( !string ) {
		return 0;
	}

	len = 0;
	p = string;
	while( *p ) {
		if( Q_IsColorString( p ) ) {
			p += 2;
			continue;
		}
		// Cgg - repeated escape codes (^^) were broken
		if ( *p == Q_COLOR_ESCAPE && *(p+1) == Q_COLOR_ESCAPE )
			p++;
		// !Cgg
		p++;
		len++;
	}

	return len;
}


char *Q_CleanStr( char *string ) {
	char*	d;
	char*	s;

	s = string;
	d = string;
	// Cgg - repeated escape codes (^^) were broken
	while (*s) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
			continue;
		}

		if (*s == Q_COLOR_ESCAPE && *(s+1) == Q_COLOR_ESCAPE)
			s++;

		if (*s >= 0x20 && *s <= 0x7E)
			*d++ = *s;
		s++;
	}
	*d = '\0';
	// !Cgg

	return string;
}

/*
Q_strstrip

	Description:	Replace strip[x] in string with repl[x] or remove characters entirely
					Does not strip colours or any characters not specified. See Q_CleanStr
	Mutates:		string
	Return:			--

	Examples:		Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", "123" );	// "Bo1b is h2airy33"
					Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", "12" );	// "Bo1b is h2airy"
					Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", NULL );	// "Bob is hairy"
*/

void Q_strstrip( char *string, const char *strip, const char *repl ) {
	char		*out=string, *p=string, c;
	const char	*s=strip;
	size_t		replaceLen = repl?strlen( repl ):0;
	ptrdiff_t	offset=0;

	while ( (c = *p++) != '\0' )
	{
		for ( s=strip; *s; s++ )
		{
			offset = s-strip;
			if ( c == *s )
			{
				if ( !repl || offset >= replaceLen )
					c = *p++;
				else
					c = repl[offset];
				break;
			}
		}
		*out++ = c;
	}
	*out = '\0';
}

/*
Q_strchrs

	Description:	Find any characters in a string. Think of it as a shorthand strchr loop.
	Mutates:		--
	Return:			first instance of any characters found
					 otherwise NULL
*/

const char *Q_strchrs( const char *string, const char *search ) {
	const char *p, *s;

	for ( p=string; *p != '\0'; p++ ) {
		for ( s=search; *s != '\0'; s++ ) {
			if ( *p == *s ) {
				return p;
			}
		}
	}

	return NULL;
}

/*
Q_strrep

	Description:	Replace instances of 'old' in 's' with 'new'
	Mutates:		--
	Return:			malloced string containing the replacements
*/

char *Q_strrep( const char *string, const char *substr, const char *replacement ) {
	char *tok = NULL;
	char *newstr = NULL;

	tok = (char *)strstr( string, substr );
	if( tok == NULL ) return strdup( string );
	newstr = (char *)malloc( strlen( string ) - strlen( substr ) + strlen( replacement ) + 1 );
	if( newstr == NULL ) return NULL;
	memcpy( newstr, string, tok - string );
	memcpy( newstr + (tok - string), replacement, strlen( replacement ) );
	memcpy( newstr + (tok - string) + strlen( replacement ), tok + strlen( substr ), strlen( string ) - strlen( substr ) - ( tok - string ) );
	memset( newstr + strlen( string ) - strlen( substr ) + strlen( replacement ), 0, 1 );

	return newstr;
}

int Q_CountChar(const char *string, char tocount) {
	int count;
	
	for ( count=0; *string; string++ ) {
		if ( *string == tocount )
			count++;
	}
	
	return count;
}

size_t QDECL Com_sprintf(char *dest, size_t size, const char *fmt, ...) {
	size_t len;
	va_list		argptr;

	va_start (argptr,fmt);
	len = Q_vsnprintf(dest, size, fmt, argptr);
	va_end (argptr);

	if(len >= size)
		Com_Printf("Com_sprintf: Output length %d too short, require %d bytes.\n", (int)size, (int)(len + 1));
	
	return len;
}

#define VARARGS_BUFFERS		(4)
#define VARARGS_MASK		(VARARGS_BUFFERS-1)

// does a varargs printf into a temp buffer, so I don't need to have varargs versions of all text functions.
const char *va( const char *format, ... ) {
	static char buf[VARARGS_BUFFERS][32000]; // in case va is called by nested functions
	static int	index = 0;
	va_list		argptr;
	char		*s = buf[index++ & VARARGS_MASK];

	va_start( argptr, format );
	Q_vsnprintf( s, sizeof( *buf ), format, argptr );
	va_end( argptr );

	return s;
}

// Assumes buffer is atleast TRUNCATE_LENGTH big
void Com_TruncateLongString( char *buffer, size_t len, const char *s ) {
	size_t length = strlen( s );

	if ( length <= len )
		Q_strncpyz( buffer, s, len );
	else {
		Q_strncpyz( buffer, s, ( len / 2 ) - 3 );
		Q_strcat( buffer, len, " ... " );
		Q_strcat( buffer, len, s + length - ( len / 2 ) + 3 );
	}
}

/*

	INFO STRINGS

*/

// Searches the string for the given key and returns the associated value, or an empty string.
//	FIXME: overflow check?
char *Info_ValueForKey( const char *s, const char *key ) {
	char	pkey[BIG_INFO_KEY];
	static	char value[2][BIG_INFO_VALUE];	// use two buffers so compares
											// work without stomping on each other
	static	int	valueindex = 0;
	char	*o;
	
	if ( !s || !key ) {
		return "";
	}

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_ValueForKey: oversize infostring" );
	}

	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp (key, pkey) )
			return value[valueindex];

		if (!*s)
			break;
		s++;
	}

	return "";
}

// Used to itterate through all the key/value pairs in an info string
void Info_NextPair( const char **head, char *key, char *value ) {
	char	*o;
	const char	*s;

	s = *head;

	if ( *s == '\\' ) {
		s++;
	}
	key[0] = 0;
	value[0] = 0;

	o = key;
	while ( *s != '\\' ) {
		if ( !*s ) {
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	o = value;
	while ( *s != '\\' && *s ) {
		*o++ = *s++;
	}
	*o = 0;

	*head = s;
}

void Info_RemoveKey( char *s, const char *key ) {
	char	*start;
	char	pkey[MAX_INFO_KEY];
	char	value[MAX_INFO_VALUE];
	char	*o;

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey: oversize infostring" );
	}

	if (strchr (key, '\\')) {
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			memmove(start, s, strlen(s) + 1); // remove this part
			
			return;
		}

		if (!*s)
			return;
	}

}

void Info_RemoveKey_Big( char *s, const char *key ) {
	char	*start;
	char	pkey[BIG_INFO_KEY];
	char	value[BIG_INFO_VALUE];
	char	*o;

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey_Big: oversize infostring" );
	}

	if (strchr (key, '\\')) {
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			strcpy (start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}

// Some characters are illegal in info strings because they can mess up the server's parsing
qboolean Info_Validate( const char *s ) {
	if ( strchr( s, '\"' ) ) {
		return qfalse;
	}
	if ( strchr( s, ';' ) ) {
		return qfalse;
	}
	return qtrue;
}

// Changes or adds a key/value pair
void Info_SetValueForKey( char *s, const char *key, const char *value ) {
	char	newi[MAX_INFO_STRING];
	const char* blacklist = "\\;\"";

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	for(; *blacklist; ++blacklist)
	{
		if (strchr (key, *blacklist) || strchr (value, *blacklist))
		{
			Com_Printf (S_COLOR_YELLOW "Can't use keys or values with a '%c': %s = %s\n", *blacklist, key, value);
			return;
		}
	}
	
	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf( newi, sizeof( newi ), "\\%s\\%s", key, value );

	if (strlen(newi) + strlen(s) >= MAX_INFO_STRING)
	{
		Com_Printf ("Info string length exceeded\n");
		return;
	}

	Q_strcat( newi, sizeof( newi ), s );
	Q_strncpyz( s, newi, MAX_INFO_STRING );
}

// Changes or adds a key/value pair
//	Includes and retains zero-length values
void Info_SetValueForKey_Big( char *s, const char *key, const char *value ) {
	char	newi[BIG_INFO_STRING];
	const char* blacklist = "\\;\"";

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	for(; *blacklist; ++blacklist)
	{
		if (strchr (key, *blacklist) || strchr (value, *blacklist))
		{
			Com_Printf (S_COLOR_YELLOW "Can't use keys or values with a '%c': %s = %s\n", *blacklist, key, value);
			return;
		}
	}

	Info_RemoveKey_Big (s, key);
	if (!value)
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) >= BIG_INFO_STRING)
	{
		Com_Printf ("BIG Info string length exceeded\n");
		return;
	}

	Q_strcat( s, MAX_INFO_STRING, newi );
}

static qboolean Com_CharIsOneOfCharset( char c, char *set ) {
	unsigned int i;

	for ( i=0; i<strlen( set ); i++ ) {
		if( set[ i ] == c )
			return qtrue;
	}

	return qfalse;
}

char *Com_SkipCharset( char *s, char *sep ) {
	char	*p = s;

	while( p )
	{
		if( Com_CharIsOneOfCharset( *p, sep ) )
			p++;
		else
			break;
	}

	return p;
}

char *Com_SkipTokens( char *s, int numTokens, char *sep ) {
	int		sepCount = 0;
	char	*p = s;

	while( sepCount < numTokens )
	{
		if( Com_CharIsOneOfCharset( *p++, sep ) )
		{
			sepCount++;
			while( Com_CharIsOneOfCharset( *p, sep ) )
				p++;
		}
		else if( *p == '\0' )
			break;
	}

	if( sepCount == numTokens )
		return p;
	else
		return s;
}

#define NUM_TEMPVECS 8
#define TEMPVEC_MASK (NUM_TEMPVECS-1)
static vector3 tempVecs[NUM_TEMPVECS];

// This is just a convenience function for making temporary vectors
vector3 *tv( float x, float y, float z ) {
	static int index;
	vector3 *v = &tempVecs[index++ & TEMPVEC_MASK];

	VectorSet( v, x, y, z );

	return v;
}

static char tempStrs[NUM_TEMPVECS][32];

// This is just a convenience function for printing vectors
char *vtos( const vector3 *v ) {
	static int index=0;
	char *s = tempStrs[index++ & TEMPVEC_MASK];

	Com_sprintf( s, 32, "(%i %i %i)", (int)v->x, (int)v->y, (int)v->z );

	return s;
}

void Field_Clear( field_t *edit ) {
	memset( edit->buffer, 0, MAX_EDIT_LINE );
	edit->cursor = 0;
	edit->scroll = 0;
}

const char *gametypeStringShort[GT_NUM_GAMETYPES] = {
	"BLOOD",
	"1v1",
	"TEAMBLOOD",
	"FLAGS",
	"TROJAN"
};

const char *GametypeStringForID( int gametype ) {
	switch ( gametype ) {
	case GT_BLOODBATH:
		return "Bloodbath";
	case GT_DUEL:
		return "Duel";

	case GT_TEAMBLOOD:
		return "Team Bloodbath";
	case GT_FLAGS:
		return "Flags";
	case GT_TROJAN:
		return "Trojan";

	default:
		return "Unknown Gametype";
	}
}

int GametypeIDForString( const char *gametype ) {
		 if ( !Q_stricmp( gametype, "bloodbath" )
			||!Q_stricmp( gametype, "ffa" )
			||!Q_stricmp( gametype, "dm" )
			||!Q_stricmp( gametype, "deathmatch" ) )	return GT_BLOODBATH;
	else if ( !Q_stricmp( gametype, "duel" )
			||!Q_stricmp( gametype, "1v1" ) )			return GT_DUEL;
	else if ( !Q_stricmp( gametype, "tdm" )
			||!Q_stricmp( gametype, "tffa" )
			||!Q_stricmp( gametype, "team" ) )			return GT_TEAMBLOOD;
	else if ( !Q_stricmp( gametype, "flags" )
			||!Q_stricmp( gametype, "ctf" ) )			return GT_FLAGS;
	else if ( !Q_stricmp( gametype, "trojan" ) )		return GT_TROJAN;
	else												return -1;
}
