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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"

qboolean	scr_initialized;		// ready to draw

cvar_t		*cl_timegraph;
cvar_t		*cl_debuggraph;
cvar_t		*cl_graphheight;
cvar_t		*cl_graphscale;
cvar_t		*cl_graphshift;

// Coordinates are 640x480 virtual values
void SCR_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t	hShader;

	assert( width != 0 );

	hShader = re->RegisterShader( picname );
	SCR_AdjustFrom640( &x, &y, &width, &height );
	re->DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

// Adjusted for resolution and screen aspect ratio
void SCR_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	float	xscale;
	float	yscale;

#if 0
		// adjust for wide screens
		if ( cls.glconfig.vidWidth * SCREEN_HEIGHT > cls.glconfig.vidHeight * SCREEN_WIDTH ) {
			*x += 0.5f * ( cls.glconfig.vidWidth - ( cls.glconfig.vidHeight * SCREEN_WIDTH / SCREEN_HEIGHT ) );
		}
#endif

	// scale for screen sizes
	xscale = cls.glconfig.vidWidth / SCREEN_WIDTH;
	yscale = cls.glconfig.vidHeight / SCREEN_HEIGHT;
	if ( x ) {
		*x *= xscale;
	}
	if ( y ) {
		*y *= yscale;
	}
	if ( w ) {
		*w *= xscale;
	}
	if ( h ) {
		*h *= yscale;
	}
}

// Coordinates are 640x480 virtual values
void SCR_FillRect( float x, float y, float width, float height, const vector4 *color ) {
	re->SetColor( color );

	SCR_AdjustFrom640( &x, &y, &width, &height );
	re->DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cls.whiteShader );

	re->SetColor( NULL );
}

// Coordinates are 640x480 virtual values
void SCR_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	SCR_AdjustFrom640( &x, &y, &width, &height );
	re->DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

// chars are drawn at 640x480 virtual screen size
static void SCR_DrawChar( int x, int y, float size, int ch ) {
	int row, col;
	float frow, fcol;
	float	ax, ay, aw, ah;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -size ) {
		return;
	}

	ax = (float)x;
	ay = (float)y;
	aw = (float)size;
	ah = (float)size;
	SCR_AdjustFrom640( &ax, &ay, &aw, &ah );

	row = ch>>4;
	col = ch&15;

	frow = row*0.0625f;
	fcol = col*0.0625f;
	size = 0.0625f;

	re->DrawStretchPic( ax, ay, aw, ah,
					   fcol, frow, 
					   fcol + size, frow + size, 
					   cls.charSetShader );
}

// small chars are drawn at native screen resolution
void SCR_DrawSmallChar( int x, int y, int ch ) {
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -SMALLCHAR_HEIGHT ) {
		return;
	}

	row = ch>>4;
	col = ch&15;

	frow = row*0.0625f;
	fcol = col*0.0625f;
	size = 0.0625f;

	re->DrawStretchPic( (float)x, (float)y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT,
					   fcol, frow, 
					   fcol + size, frow + size, 
					   cls.charSetShader );
}

// small chars are drawn at native screen resolution
void SCR_DrawSmallChar2( int x, int y, int ch, float scale ) {
	int row, col;
	float frow, fcol;
	float size = 1/16.0f;

	ch &= 255;

	if ( ch == ' ' || y < -SMALLCHAR_HEIGHT )
		return;

	row = ch>>4;
	col = ch&15;

	frow = row*size;
	fcol = col*size;
	
	re->DrawStretchPic( (float)x, (float)y, SMALLCHAR_WIDTH*scale, SMALLCHAR_HEIGHT*scale,
					   fcol, frow, 
					   fcol + size, frow + size, 
					   cls.charSetShader );
}

// Draws a multi-colored string with a drop shadow, optionally forcing to a fixed color.
//	Coordinates are 640x480 virtual values
void SCR_DrawStringExt( int x, int y, float size, const char *string, const vector4 *setColor, qboolean forceColor, qboolean noColorEscape ) {
	vector4		color;
	const char	*s;
	int			xx;

	// draw the drop shadow
	VectorSet4( &color, 0.0f, 0.0f, 0.0f, setColor->a );
	re->SetColor( &color );
	s = string;
	xx = x;
	while ( *s ) {
		if ( !noColorEscape && Q_IsColorString( s ) ) {
			s += 2;
			continue;
		}
		SCR_DrawChar( xx+2, y+2, size, *s );
		xx += (int)size;
		s++;
	}


	// draw the colored text
	s = string;
	xx = x;
	re->SetColor( setColor );
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				memcpy( &color, &g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color.a = setColor->a;
				re->SetColor( &color );
			}
			if ( !noColorEscape ) {
				s += 2;
				continue;
			}
		}
		SCR_DrawChar( xx, y, size, *s );
		xx += (int)size;
		s++;
	}
	re->SetColor( NULL );
}


void SCR_DrawBigString( int x, int y, const char *s, float alpha, qboolean noColorEscape ) {
	vector4 color;

	VectorSet4( &color, 1.0f, 1.0f, 1.0f, alpha );
	SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, &color, qfalse, noColorEscape );
}

void SCR_DrawBigStringColor( int x, int y, const char *s, vector4 *color, qboolean noColorEscape ) {
	SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, color, qtrue, noColorEscape );
}

// Draws a multi-colored string with a drop shadow, optionally forcing to a fixed color.
//	Coordinates are 640x480 virtual values
void SCR_DrawSmallStringExt( int x, int y, const char *string, vector4 *setColor, qboolean forceColor,
		qboolean noColorEscape ) {
	vector4		color;
	const char	*s;
	int			xx;

	// draw the colored text
	s = string;
	xx = x;
	re->SetColor( setColor );
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				memcpy( &color, &g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color.a = setColor->a;
				re->SetColor( &color );
			}
			if ( !noColorEscape ) {
				s += 2;
				continue;
			}
		}
		SCR_DrawSmallChar( xx, y, *s );
		xx += SMALLCHAR_WIDTH;
		s++;
	}
	re->SetColor( NULL );
}

// skips color escape codes
static int SCR_Strlen( const char *str ) {
	const char *s = str;
	int count = 0;

	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
		} else {
			count++;
			s++;
		}
	}

	return count;
}

int	SCR_GetBigStringWidth( const char *str ) {
	return SCR_Strlen( str ) * BIGCHAR_WIDTH;
}

void SCR_DrawDemoRecording( void ) {
	char	string[1024];
	int		pos;

	if ( !clc.demorecording ) {
		return;
	}
	if ( clc.spDemoRecording ) {
		return;
	}

	pos = FS_FTell( clc.demofile );
	sprintf( string, "RECORDING %s: %ik", clc.demoName, pos / 1024 );

	SCR_DrawStringExt( ((int)SCREEN_WIDTH/2) - (int)strlen( string ) * 4, 20, 8, string, &g_color_table[ColorIndex(COLOR_WHITE)], qtrue, qfalse );
}


#ifdef USE_VOIP
void SCR_DrawVoipMeter( void ) {
	char	buffer[16];
	char	string[256];
	int limit, i;

	if (!cl_voipShowMeter->integer)
		return;  // player doesn't want to show meter at all.
	else if (!cl_voipSend->integer)
		return;  // not recording at the moment.
	else if (clc.state != CA_ACTIVE)
		return;  // not connected to a server.
	else if (!clc.voipEnabled)
		return;  // server doesn't support VoIP.
	else if (clc.demoplaying)
		return;  // playing back a demo.
	else if (!cl_voip->integer)
		return;  // client has VoIP support disabled.

	limit = (int) (clc.voipPower * 10.0f);
	if (limit > 10)
		limit = 10;

	for (i = 0; i < limit; i++)
		buffer[i] = '*';
	while (i < 10)
		buffer[i++] = ' ';
	buffer[i] = '\0';

	sprintf( string, "VoIP: [%s]", buffer );
	SCR_DrawStringExt( ((int)SCREEN_WIDTH/2) - (int)strlen( string ) * 4, 10, 8, string, &g_color_table[ColorIndex(COLOR_WHITE)], qtrue, qfalse );
}
#endif

/*

	DEBUG GRAPH

*/

static	int			current;
static	float		values[1024];

void SCR_DebugGraph (float value) {
	values[current] = value;
	current = (current + 1) % ARRAY_LEN(values);
}

void SCR_DrawDebugGraph( void ) {
	int		a, x, y, w, i, h;
	float	v;

	//
	// draw the graph
	//
	w = cls.glconfig.vidWidth;
	x = 0;
	y = cls.glconfig.vidHeight;
	re->SetColor( &g_color_table[ColorIndex(COLOR_BLACK)] );
	re->DrawStretchPic((float)x, (float)(y - cl_graphheight->integer), (float)w, cl_graphheight->value, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );

	for (a=0 ; a<w ; a++)
	{
		i = (ARRAY_LEN(values)+current-1-(a % ARRAY_LEN(values))) % ARRAY_LEN(values);
		v = values[i];
		v = v * cl_graphscale->integer + cl_graphshift->integer;
		
		if (v < 0)
			v += cl_graphheight->integer * (1+(int)(-v / cl_graphheight->integer));
		h = (int)v % cl_graphheight->integer;
		re->DrawStretchPic( (float)(x+w-1-a), (float)(y - h), 1.0f, (float)h, 0, 0, 0, 0, cls.whiteShader );
	}
}

void SCR_Init( void ) {
	cl_timegraph	= Cvar_Get( "timegraph",	"0",	CVAR_CHEAT, NULL, NULL );
	cl_debuggraph	= Cvar_Get( "debuggraph",	"0",	CVAR_CHEAT, NULL, NULL );
	cl_graphheight	= Cvar_Get( "graphheight",	"32",	CVAR_CHEAT, NULL, NULL );
	cl_graphscale	= Cvar_Get( "graphscale",	"1",	CVAR_CHEAT, NULL, NULL );
	cl_graphshift	= Cvar_Get( "graphshift",	"0",	CVAR_CHEAT, NULL, NULL );

	scr_initialized = qtrue;
}

// This will be called twice if rendering in stereo mode
void SCR_DrawScreenField( stereoFrame_t stereoFrame ) {
	qboolean uiFullscreen;

	re->BeginFrame( stereoFrame );

	uiFullscreen = (cls.uiStarted && ui->IsFullscreen());

	// wide aspect ratio screens need to have the sides cleared
	// unless they are displaying game renderings
	if ( uiFullscreen || clc.state != CA_ACTIVE ) {
		if ( cls.glconfig.vidWidth * SCREEN_HEIGHT > cls.glconfig.vidHeight * SCREEN_WIDTH ) {
			re->SetColor( &g_color_table[ColorIndex(COLOR_BLACK)] );
			re->DrawStretchPic( 0, 0, (float)cls.glconfig.vidWidth, (float)cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader );
			re->SetColor( NULL );
		}
	}

	// if the menu is going to cover the entire screen, we
	// don't need to render anything under it
	if ( cls.uiStarted && !uiFullscreen ) {
		switch( clc.state ) {
		default:
			Com_Error( ERR_FATAL, "SCR_DrawScreenField: bad clc.state" );
			break;
		case CA_DISCONNECTED:
			// force menu up
			S_StopAllSounds();
			ui->SetActiveMenu( UIMENU_MAIN );
			break;
		case CA_CONNECTING:
		case CA_CHALLENGING:
		case CA_CONNECTED:
			// connecting clients will only show the connection dialog
			// refresh to update the time
			ui->Refresh( cls.realtime );
			ui->DrawConnectScreen( qfalse );
			break;
		case CA_LOADING:
		case CA_PRIMED:
			// draw the game information screen and loading progress
			CL_CGameRendering(stereoFrame);

			// also draw the connection information, so it doesn't
			// flash away too briefly on local or lan games
			// refresh to update the time
			ui->Refresh( cls.realtime );
			ui->DrawConnectScreen( qtrue );
			break;
		case CA_ACTIVE:
			// always supply STEREO_CENTER as vieworg offset is now done by the engine.
			CL_CGameRendering(stereoFrame);
			SCR_DrawDemoRecording();
#ifdef USE_VOIP
			SCR_DrawVoipMeter();
#endif
			break;
		}
	}

	// the menu draws next
	if ( Key_GetCatcher( ) & KEYCATCH_UI && cls.uiStarted ) {
		ui->Refresh( cls.realtime );
	}

	// console draws next
	Con_DrawConsole();

	// debug graph can be drawn on top of anything
	if ( cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer ) {
		SCR_DrawDebugGraph();
	}
}

// This is called every frame, and can also be called explicitly to flush text to the screen.
void SCR_UpdateScreen( void ) {
	static int	recursive;

	if ( !scr_initialized ) {
		return;				// not initialized yet
	}

	if ( ++recursive > 2 ) {
		Com_Error( ERR_FATAL, "SCR_UpdateScreen: recursively called" );
	}
	recursive = 1;

	// If there is no VM, there are also no rendering commands issued. Stop the renderer in
	// that case.
	if ( cls.uiStarted || com_dedicated->integer )
	{
		// XXX
		int in_anaglyphMode = Cvar_VariableIntegerValue("r_anaglyphMode");
		// if running in stereo, we need to draw the frame twice
		if ( cls.glconfig.stereoEnabled || in_anaglyphMode) {
			SCR_DrawScreenField( STEREO_LEFT );
			SCR_DrawScreenField( STEREO_RIGHT );
		} else {
			SCR_DrawScreenField( STEREO_CENTER );
		}

		if ( com_speeds->integer ) {
			re->EndFrame( &time_frontend, &time_backend );
		} else {
			re->EndFrame( NULL, NULL );
		}
	}
	
	recursive = 0;
}

