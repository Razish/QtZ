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
/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"

qboolean m_entersound;		// after a frame, so caching won't disrupt the sound
qboolean newUI = qfalse;


/*
=================
UI_ClampCvar
=================
*/
float UI_ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

/*
=================
UI_StartDemoLoop
=================
*/
void UI_StartDemoLoop( void ) {
	trap->Cbuf_ExecuteText( EXEC_APPEND, "d1\n" );
}


char *UI_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap->Cmd_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


char *UI_Cvar_VariableString( const char *var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	trap->Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}

static void	UI_Cache_f( void ) {
	Display_CacheAll();
}

/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand( int realTime ) {
	char	*cmd;

	uiInfo.uiDC.frameTime = realTime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realTime;

	cmd = UI_Argv( 0 );

	// ensure minimum menu data is available
	//Menu_Cache();

	if ( Q_stricmp (cmd, "ui_test") == 0 ) {
		UI_ShowPostGame(qtrue);
	}

	if ( Q_stricmp (cmd, "ui_report") == 0 ) {
		UI_Report();
		return qtrue;
	}
	
	if ( Q_stricmp (cmd, "ui_load") == 0 ) {
		UI_Load();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "remapShader") == 0 ) {
		if (trap->Cmd_Argc() == 4) {
			char shader1[MAX_QPATH];
			char shader2[MAX_QPATH];
			char shader3[MAX_QPATH];
			
			Q_strncpyz(shader1, UI_Argv(1), sizeof(shader1));
			Q_strncpyz(shader2, UI_Argv(2), sizeof(shader2));
			Q_strncpyz(shader3, UI_Argv(3), sizeof(shader3));
			
			trap->R_RemapShader(shader1, shader2, shader3);
			return qtrue;
		}
	}

	if ( Q_stricmp (cmd, "ui_cache") == 0 ) {
		UI_Cache_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_teamOrders") == 0 ) {
		//UI_TeamOrdersMenu_f();
		return qtrue;
	}

	return qfalse;
}

/*
================
UI_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void UI_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	// expect valid pointers
#if 0
	*x = *x * uiInfo.uiDC.scale + uiInfo.uiDC.bias;
	*y *= uiInfo.uiDC.scale;
	*w *= uiInfo.uiDC.scale;
	*h *= uiInfo.uiDC.scale;
#endif

	*x *= uiInfo.uiDC.xscale;
	*y *= uiInfo.uiDC.yscale;
	*w *= uiInfo.uiDC.xscale;
	*h *= uiInfo.uiDC.yscale;

}

void UI_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t	hShader;

	hShader = trap->R_RegisterShader( picname );
	UI_AdjustFrom640( &x, &y, &width, &height );
	trap->R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

void UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ) {
	float	s0;
	float	s1;
	float	t0;
	float	t1;

	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}
	
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap->R_DrawStretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect( float x, float y, float width, float height, const vector4 *color ) {
	trap->R_SetColor( color );

	UI_AdjustFrom640( &x, &y, &width, &height );
	trap->R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );

	trap->R_SetColor( NULL );
}

void UI_DrawSides(float x, float y, float w, float h) {
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap->R_DrawStretchPic( x, y, 1, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x + w - 1, y, 1, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void UI_DrawTopBottom(float x, float y, float w, float h) {
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap->R_DrawStretchPic( x, y, w, 1, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x, y + h - 1, w, 1, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect( float x, float y, float width, float height, const vector4 *color ) {
	trap->R_SetColor( color );

  UI_DrawTopBottom(x, y, width, height);
  UI_DrawSides(x, y, width, height);

	trap->R_SetColor( NULL );
}

void UI_SetColor( const vector4 *rgba ) {
	trap->R_SetColor( rgba );
}

void UI_UpdateScreen( void ) {
	trap->UpdateScreen();
}


void UI_DrawTextBox (int x, int y, int width, int lines)
{
	UI_FillRect( (float)(x + BIGCHAR_WIDTH/2), (float)(y + BIGCHAR_HEIGHT/2), (float)(( width + 1 ) * BIGCHAR_WIDTH), (float)(( lines + 1 ) * BIGCHAR_HEIGHT), &colorBlack );
	UI_DrawRect( (float)(x + BIGCHAR_WIDTH/2), (float)(y + BIGCHAR_HEIGHT/2), (float)(( width + 1 ) * BIGCHAR_WIDTH), (float)(( lines + 1 ) * BIGCHAR_HEIGHT), &colorWhite );
}

qboolean UI_CursorInRect (int x, int y, int width, int height)
{
	if (uiInfo.uiDC.cursorx < x ||
		uiInfo.uiDC.cursory < y ||
		uiInfo.uiDC.cursorx > x+width ||
		uiInfo.uiDC.cursory > y+height)
		return qfalse;

	return qtrue;
}
