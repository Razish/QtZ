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
// cg_drawtools.c -- helper functions called by cg_draw, cg_scoreboard, cg_info, etc
#include "cg_local.h"

// Adjusted for resolution and screen aspect ratio
void CG_AdjustFrom640( float *x, float *y, float *w, float *h ) {
#if 0
	// adjust for wide screens
	if ( cgs.glconfig.vidWidth * SCREEN_HEIGHT > cgs.glconfig.vidHeight * SCREEN_WIDTH ) {
		*x += 0.5f * ( cgs.glconfig.vidWidth - ( cgs.glconfig.vidHeight * SCREEN_WIDTH / SCREEN_HEIGHT ) );
	}
#endif
	// scale for screen sizes
	*x *= cgs.screenXScale;
	*y *= cgs.screenYScale;
	*w *= cgs.screenXScale;
	*h *= cgs.screenYScale;
}

// Coordinates are 640x480 virtual values
void CG_FillRect( float x, float y, float width, float height, const vector4 *color ) {
	trap->R_SetColor( color );

	CG_AdjustFrom640( &x, &y, &width, &height );
	trap->R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader );

	trap->R_SetColor( NULL );
}

// Coordinates are 640x480 virtual values
void CG_DrawSides(float x, float y, float w, float h, float size) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	size *= cgs.screenXScale;
	trap->R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
	trap->R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
}

void CG_DrawTopBottom(float x, float y, float w, float h, float size) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	size *= cgs.screenYScale;
	trap->R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
	trap->R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
}

// Coordinates are 640x480 virtual values
void CG_DrawRect( float x, float y, float width, float height, float size, const vector4 *color ) {
	trap->R_SetColor( color );

	CG_DrawTopBottom(x, y, width, height, size);
	CG_DrawSides(x, y, width, height, size);

	trap->R_SetColor( NULL );
}

// Coordinates are 640x480 virtual values
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	CG_AdjustFrom640( &x, &y, &width, &height );
	trap->R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

//QtZ: Added from JA
// Coordinates are 640x480 virtual values
//	A width of 0 will draw with the original image width
//	rotates around the upper right corner or center point of the passed in coordinates
void CG_DrawRotatePic( float x, float y, float width, float height, float angle, qboolean centered, qhandle_t hShader ) {
	CG_AdjustFrom640( &x, &y, &width, &height );
	trap->R_DrawRotatedPic( x, y, width, height, 0, 0, 1, 1, angle, centered, hShader );
}
//~QtZ

// Returns character count, skiping color escape codes
int CG_DrawStrlen( const char *str ) {
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

// This repeats a 64*64 tile graphic to fill the screen around a sized down refresh window.
static void CG_TileClearBox( int x, int y, int w, int h, qhandle_t hShader ) {
	float	s1, t1, s2, t2;

	s1 = x/64.0f;
	t1 = y/64.0f;
	s2 = (x+w)/64.0f;
	t2 = (y+h)/64.0f;
	trap->R_DrawStretchPic( (float)x, (float)y, (float)w, (float)h, s1, t1, s2, t2, hShader );
}

// Clear around a sized down screen
void CG_TileClear( void ) {
	int		top, bottom, left, right;
	int		w, h;

	w = cgs.glconfig.vidWidth;
	h = cgs.glconfig.vidHeight;

	if ( cg.refdef.x == 0 && cg.refdef.y == 0 && 
		cg.refdef.width == w && cg.refdef.height == h ) {
		return;		// full screen rendering
	}

	top = cg.refdef.y;
	bottom = top + cg.refdef.height-1;
	left = cg.refdef.x;
	right = left + cg.refdef.width-1;

	// clear above view screen
	CG_TileClearBox( 0, 0, w, top, cgs.media.backTileShader );

	// clear below view screen
	CG_TileClearBox( 0, bottom, w, h - bottom, cgs.media.backTileShader );

	// clear left of view screen
	CG_TileClearBox( 0, top, left, bottom - top + 1, cgs.media.backTileShader );

	// clear right of view screen
	CG_TileClearBox( right, top, w - right, bottom - top + 1, cgs.media.backTileShader );
}

#define NUM_FADE_COLORS 8
#define FADE_MASK (NUM_FADE_COLORS-1)
vector4 *CG_FadeColor( int startMsec, int totalMsec ) {
	static vector4 colors[NUM_FADE_COLORS];
	static int index = 0;
	int t;
	vector4 *color = NULL;

	if ( startMsec == 0 )
		return NULL;

	t = cg.time - startMsec;

	if ( t >= totalMsec )
		return NULL;

	color = &colors[index++ & FADE_MASK];
	// fade out
	if ( totalMsec - t < FADE_TIME )
		color->a = (totalMsec - t) * 1.0f/FADE_TIME;
	else
		color->a = 1.0f;
	color->r = color->g = color->b = 1.0f;

	return color;
}

//Returns qtrue if the fade is complete, don't draw shadows etc
//			qfalse if it has either partially faded, or not at all.
qboolean CG_FadeColor2( vector4 *color, int startMsec, int totalMsec ) {
	float dt = ((float)cg.time - (float)startMsec);

	if ( dt > totalMsec ) {
		color->a = 0.0f;
		return qtrue;
	}
	//RAZTODO: else if ( dt < 0 )
	else {
		dt = Q_fabs( dt );
		color->a = 1.0f - dt/totalMsec;
		return qfalse;
	}
}

vector4 *CG_TeamColor( int team ) {
	static vector4	red = {1, 0.2f, 0.2f, 1};
	static vector4	blue = {0.2f, 0.2f, 1, 1};
	static vector4	other = {1, 1, 1, 1};
	static vector4	spectator = {0.7f, 0.7f, 0.7f, 1};

	switch ( team ) {
	case TEAM_RED:
		return &red;
	case TEAM_BLUE:
		return &blue;
	case TEAM_SPECTATOR:
		return &spectator;
	default:
		return &other;
	}
}

void CG_GetColorForHealth( int health, int armor, vector4 *hcolor ) {
	int		count;
	int		max;

	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	if ( health <= 0 ) {
		VectorClear4( hcolor );	// black
		hcolor->a = 1;
		return;
	}
	count = armor;
	max = (int)(health * ARMOR_PROTECTION / ( 1.0f - ARMOR_PROTECTION ));
	if ( max < count ) {
		count = max;
	}
	health += count;

	// set the color based on health
	hcolor->r = 1.0f;
	hcolor->a = 1.0f;
	if ( health >= 100 )
		hcolor->b = 1.0f;
	else if ( health < 66 )
		hcolor->b = 0;
	else
		hcolor->b = ( health - 66 ) / 33.0f;

	if ( health > 60 )
		hcolor->g = 1.0f;
	else if ( health < 30 )
		hcolor->g = 0;
	else
		hcolor->g = ( health - 30 ) / 30.0f;
}

void CG_ColorForHealth( vector4 *hcolor ) {
	CG_GetColorForHealth( cg.snap->ps.stats[STAT_HEALTH], cg.snap->ps.stats[STAT_ARMOR], hcolor );
}
