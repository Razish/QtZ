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
// cg_info.c -- display information while data is being loading

#include "cg_local.h"

#define MAX_LOADING_PLAYER_ICONS	16
#define MAX_LOADING_ITEM_ICONS		26

static int			loadingPlayerIconCount;
static int			loadingItemIconCount;
static qhandle_t	loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];
static qhandle_t	loadingItemIcons[MAX_LOADING_ITEM_ICONS];


/*
===================
CG_DrawLoadingIcons
===================
*/
static void CG_DrawLoadingIcons( void ) {
	int		n;
	int		x, y;

	for( n = 0; n < loadingPlayerIconCount; n++ ) {
		x = 16 + n * 78;
		y = 324-40;
		CG_DrawPic( (float)x, (float)y, 64.0f, 64.0f, loadingPlayerIcons[n] );
	}

	for( n = 0; n < loadingItemIconCount; n++ ) {
		y = 400-40;
		if( n >= 13 ) {
			y += 40;
		}
		x = 16 + n % 13 * 48;
		CG_DrawPic( (float)x, (float)y, 32.0f, 32.0f, loadingItemIcons[n] );
	}
}


/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );

	trap->UpdateScreen();
}

/*
===================
CG_LoadingItem
===================
*/
void CG_LoadingItem( int itemNum ) {
	gitem_t		*item;

	item = &bg_itemlist[itemNum];
	
	if ( item->icon && loadingItemIconCount < MAX_LOADING_ITEM_ICONS ) {
		loadingItemIcons[loadingItemIconCount++] = trap->R_RegisterShaderNoMip( item->icon );
	}

	CG_LoadingString( item->pickup_name );
}

/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient( int clientNum ) {
	const char		*info;
	char			personality[MAX_QPATH];
	char			model[MAX_QPATH];
	char			iconName[MAX_QPATH];

	info = CG_ConfigString( CS_PLAYERS + clientNum );

	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
		Q_strncpyz( model, Info_ValueForKey( info, "m" ), sizeof( model ) );

		Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon.tga", model );
		
		loadingPlayerIcons[loadingPlayerIconCount] = trap->R_RegisterShaderNoMip( iconName );
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon.tga", model );
			loadingPlayerIcons[loadingPlayerIconCount] = trap->R_RegisterShaderNoMip( iconName );
		}
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon.tga", DEFAULT_MODEL );
			loadingPlayerIcons[loadingPlayerIconCount] = trap->R_RegisterShaderNoMip( iconName );
		}
		if ( loadingPlayerIcons[loadingPlayerIconCount] ) {
			loadingPlayerIconCount++;
		}
	}

	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof(personality) );
	Q_CleanStr( personality );

	CG_LoadingString( personality );
}


/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation( void ) {
	const char	*s;
	const char	*info;
	const char	*sysInfo;
	int			y;
	int			value;
	qhandle_t	levelshot;
	qhandle_t	detail;
	char		buf[MAX_CVAR_VALUE_STRING];

	info = CG_ConfigString( CS_SERVERINFO );
	sysInfo = CG_ConfigString( CS_SYSTEMINFO );

	s = Info_ValueForKey( info, "mapname" );
	levelshot = trap->R_RegisterShaderNoMip( va( "gfx/maps/%s.tga", s ) );
	if ( !levelshot ) {
		levelshot = trap->R_RegisterShaderNoMip( "menu/art/unknownmap" );
	}
	trap->R_SetColor( NULL );
	CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot );

	// blend a detail texture over it
	detail = trap->R_RegisterShader( "levelShotDetail" );
	trap->R_DrawStretchPic( 0.0f, 0.0f, (float)cgs.glconfig.vidWidth, (float)cgs.glconfig.vidHeight, 0, 0, 2.5f, 2, detail );

	// draw the icons of things as they are loaded
	CG_DrawLoadingIcons();

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if ( cg.infoScreenText[0] )
		UI_DrawProportionalString( ((int)SCREEN_WIDTH/2), 128-32, va("Loading... %s", cg.infoScreenText), UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &g_color_table[ColorIndex(COLOR_WHITE)] );
	else
		UI_DrawProportionalString( ((int)SCREEN_WIDTH/2), 128-32, "Awaiting snapshot...", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &g_color_table[ColorIndex(COLOR_WHITE)] );

	// draw info string information

	y = 180-32;

	// don't print server lines if playing a local game
	trap->Cvar_VariableStringBuffer( "sv_running", buf, sizeof( buf ) );
	if ( !atoi( buf ) ) {
		char hostname[MAX_CVAR_VALUE_STRING] = {0};
		// server hostname
		Q_strncpyz(hostname, Info_ValueForKey( info, "sv_hostname" ), sizeof( hostname ));
		Q_CleanStr(hostname);
		UI_DrawProportionalString( ((int)SCREEN_WIDTH/2), y, hostname, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &g_color_table[ColorIndex(COLOR_WHITE)] );
		y += PROP_HEIGHT;

		// pure server
		s = Info_ValueForKey( sysInfo, "sv_pure" );
		if ( s[0] == '1' ) {
			UI_DrawProportionalString( ((int)SCREEN_WIDTH/2), y, "Pure Server", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &g_color_table[ColorIndex(COLOR_WHITE)] );
			y += PROP_HEIGHT;
		}

		// server-specific message of the day
		s = CG_ConfigString( CS_MOTD );
		if ( s[0] ) {
			UI_DrawProportionalString( ((int)SCREEN_WIDTH/2), y, s, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &g_color_table[ColorIndex(COLOR_WHITE)] );
			y += PROP_HEIGHT;
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// map-specific message (long map name)
	s = CG_ConfigString( CS_MESSAGE );
	if ( s[0] ) {
		UI_DrawProportionalString( ((int)SCREEN_WIDTH/2), y, s, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &g_color_table[ColorIndex(COLOR_WHITE)] );
		y += PROP_HEIGHT;
	}

	// cheats warning
	s = Info_ValueForKey( sysInfo, "sv_cheats" );
	if ( s[0] == '1' ) {
		UI_DrawProportionalString( ((int)SCREEN_WIDTH/2), y, "CHEATS ARE ENABLED", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &g_color_table[ColorIndex(COLOR_WHITE)] );
		y += PROP_HEIGHT;
	}

	// game type
	s = BG_GetGametypeString( cgs.gametype );
	UI_DrawProportionalString( ((int)SCREEN_WIDTH/2), y, s, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &g_color_table[ColorIndex(COLOR_WHITE)] );
	y += PROP_HEIGHT;
		
	value = atoi( Info_ValueForKey( info, "timelimit" ) );
	if ( value ) {
		UI_DrawProportionalString( ((int)SCREEN_WIDTH/2), y, va( "timelimit %i", value ), UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &g_color_table[ColorIndex(COLOR_WHITE)] );
		y += PROP_HEIGHT;
	}

	if (cgs.gametype < GT_CTF ) {
		value = atoi( Info_ValueForKey( info, "fraglimit" ) );
		if ( value ) {
			UI_DrawProportionalString( ((int)SCREEN_WIDTH/2), y, va( "fraglimit %i", value ), UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &g_color_table[ColorIndex(COLOR_WHITE)] );
			y += PROP_HEIGHT;
		}
	}

	if (cgs.gametype >= GT_CTF) {
		value = atoi( Info_ValueForKey( info, "capturelimit" ) );
		if ( value ) {
			UI_DrawProportionalString( ((int)SCREEN_WIDTH/2), y, va( "capturelimit %i", value ), UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &g_color_table[ColorIndex(COLOR_WHITE)] );
			y += PROP_HEIGHT;
		}
	}
}

