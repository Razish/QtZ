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
//
// gameinfo.c
//

#include "ui_local.h"

//
// arena and bot info
//

size_t			ui_numBots;
static char		*ui_botInfos[MAX_BOTS];

static size_t	ui_numArenas;
static char		*ui_arenaInfos[MAX_ARENAS];

size_t UI_ParseInfos( char *buf, size_t max, char *infos[] ) {
	char	*token;
	size_t	count;
	char	key[MAX_TOKEN_CHARS];
	char	info[MAX_INFO_STRING];

	count = 0;

	while ( 1 ) {
		token = COM_Parse( &buf );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		info[0] = '\0';
		while ( 1 ) {
			token = COM_ParseExt( &buf, qtrue );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );
			if ( !token[0] ) {
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( info, key, token );
		}
		//NOTE: extra space for arena number
		infos[count] = UI_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
		if (infos[count]) {
			strcpy(infos[count], info);
			count++;
		}
	}
	return count;
}

static void UI_LoadArenasFromFile( char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_ARENAS_TEXT];

	len = trap->FS_Open( filename, &f, FS_READ );
	if ( !f ) {
		trap->Print( S_COLOR_RED "file not found: %s\n", filename );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap->Print( S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_ARENAS_TEXT );
		trap->FS_Close( f );
		return;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );

	ui_numArenas += UI_ParseInfos( buf, MAX_ARENAS - ui_numArenas, &ui_arenaInfos[ui_numArenas] );
}

void UI_LoadArenas( void ) {
	size_t		numdirs;
	cvar_t		*arenasFile;
	char		filename[128];
	char		dirlist[1024];
	char		*dirptr;
	int			i, n;
	size_t		dirlen;
	char		*type;

	ui_numArenas = 0;
	uiInfo.mapCount = 0;

	arenasFile = trap->Cvar_Get( "g_arenasFile", "", CVAR_INIT|CVAR_ROM, NULL, NULL );
	if( arenasFile->string[0] ) {
		UI_LoadArenasFromFile(arenasFile->string);
	}
	else {
		UI_LoadArenasFromFile("scripts/arenas.txt");
	}

	// get all arenas from .arena files
	numdirs = trap->FS_GetFileList("scripts", ".arena", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		UI_LoadArenasFromFile(filename);
	}
	trap->Print( "%i arenas parsed\n", ui_numArenas );
	if (UI_OutOfMemory()) {
		trap->Print(S_COLOR_YELLOW"WARNING: not enough memory in pool to load all arenas\n");
	}

	for( n = 0; n < ui_numArenas; n++ ) {
		// determine type

		uiInfo.mapList[uiInfo.mapCount].mapLoadName = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "map"));
		uiInfo.mapList[uiInfo.mapCount].mapName = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "longname"));
		uiInfo.mapList[uiInfo.mapCount].levelShot = -1;
		uiInfo.mapList[uiInfo.mapCount].imageName = String_Alloc(va("gfx/maps/%s", uiInfo.mapList[uiInfo.mapCount].mapLoadName));
		uiInfo.mapList[uiInfo.mapCount].typeBits = 0;

		type = Info_ValueForKey( ui_arenaInfos[n], "type" );
		// if no type specified, it will be treated as "ffa"
		if( *type ) {
			if( strstr( type, "blood" ) ) {
				uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_BLOODBATH);
			}
			if( strstr( type, "duel" ) ) {
				uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_DUEL);
			}
			if( strstr( type, "flags" ) ) {
				uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_FLAGS);
			}
			if( strstr( type, "trojan" ) ) {
				uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_TROJAN);
			}
		} else {
			uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_BLOODBATH);
		}

		uiInfo.mapCount++;
		if (uiInfo.mapCount >= MAX_MAPS) {
			break;
		}
	}
}

static void UI_LoadBotsFromFile( char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_BOTS_TEXT];

	len = trap->FS_Open( filename, &f, FS_READ );
	if ( !f ) {
		trap->Print( S_COLOR_RED "file not found: %s\n", filename );
		return;
	}
	if ( len >= MAX_BOTS_TEXT ) {
		trap->Print( S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_BOTS_TEXT );
		trap->FS_Close( f );
		return;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );

	COM_Compress(buf);

	ui_numBots += UI_ParseInfos( buf, MAX_BOTS - ui_numBots, &ui_botInfos[ui_numBots] );
}

void UI_LoadBots( void ) {
	cvar_t		*botsFile;
	size_t		numdirs;
	char		filename[128];
	char		dirlist[1024];
	char		*dirptr;
	int			i;
	size_t		dirlen;

	ui_numBots = 0;

	botsFile = trap->Cvar_Get( "g_botsFile", "", CVAR_INIT|CVAR_ROM, NULL, NULL );
	if( *botsFile->string ) {
		UI_LoadBotsFromFile(botsFile->string);
	}
	else {
		UI_LoadBotsFromFile("scripts/bots.txt");
	}

	// get all bots from .bot files
	numdirs = trap->FS_GetFileList("scripts", ".bot", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		UI_LoadBotsFromFile(filename);
	}
	trap->Print( "%i bots parsed\n", ui_numBots );
}

static char *UI_GetBotInfoByNumber( size_t num ) {
	if( num >= ui_numBots ) {
		trap->Print( S_COLOR_RED "Invalid bot number: %i\n", num );
		return NULL;
	}
	return ui_botInfos[num];
}

static char *UI_GetBotInfoByName( const char *name ) {
	int		n;
	char	*value;

	for ( n = 0; n < ui_numBots ; n++ ) {
		value = Info_ValueForKey( ui_botInfos[n], "cl_name" );
		if ( !Q_stricmp( value, name ) ) {
			return ui_botInfos[n];
		}
	}

	return NULL;
}

size_t UI_GetNumBots( void ) {
	return ui_numBots;
}


char *UI_GetBotNameByNumber( size_t num ) {
	char *info = UI_GetBotInfoByNumber(num);
	if (info) {
		return Info_ValueForKey( info, "cl_name" );
	}
	return DEFAULT_MODEL;
}
