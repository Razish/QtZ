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

#include "ui_local.h"

uiInfo_t uiInfo;

static const char *MonthAbbrev[12] = {
	"Jan", "Feb", "Mar", "Apr",
	"May", "Jun", "Jul", "Aug",
	"Sep", "Oct", "Nov", "Dec",
};


static const char *skillLevels[] = {
  "nooblet",
  "Casual",
  "Hardcore"
};
static const int numSkillLevels = ARRAY_LEN( skillLevels );

typedef enum netSource_e {
	UIAS_LOCAL=0,
	UIAS_GLOBAL1,
	UIAS_GLOBAL2,
	UIAS_GLOBAL3,
	UIAS_GLOBAL4,
	UIAS_GLOBAL5,
	UIAS_FAVORITES,
	UIAS_NUM_SOURCES
} netSource_t;

static const char *netSources[UIAS_NUM_SOURCES] = {
	"LAN",
	"Pub 1",
	"Pub 2",
	"Pub 3",
	"Pub 4",
	"Pub 5",
	"Favorites"
};
static const int numNetSources = ARRAY_LEN( netSources );

static const serverFilter_t serverFilters[] = {
	{"All", "" },
	{"QtZ", "qtz" },
};
static const int numServerFilters = ARRAY_LEN( serverFilters );

static char *netnames[] = {
	"???",
	"UDP",
	"UDP6"
};

cvar_t *ui_debug;
cvar_t *ui_initialized;

void AssetCache( void ) {
	uiInfo.uiDC.Assets.gradientBar			= trap->R_RegisterShader( ASSET_GRADIENTBAR );
	uiInfo.uiDC.Assets.scrollBar			= trap->R_RegisterShader( ASSET_SCROLLBAR );
	uiInfo.uiDC.Assets.scrollBarArrowDown	= trap->R_RegisterShader( ASSET_SCROLLBAR_ARROWDOWN );
	uiInfo.uiDC.Assets.scrollBarArrowUp		= trap->R_RegisterShader( ASSET_SCROLLBAR_ARROWUP );
	uiInfo.uiDC.Assets.scrollBarArrowLeft	= trap->R_RegisterShader( ASSET_SCROLLBAR_ARROWLEFT );
	uiInfo.uiDC.Assets.scrollBarArrowRight	= trap->R_RegisterShader( ASSET_SCROLLBAR_ARROWRIGHT );
	uiInfo.uiDC.Assets.scrollBarThumb		= trap->R_RegisterShader( ASSET_SCROLL_THUMB );
	uiInfo.uiDC.Assets.sliderBar			= trap->R_RegisterShader( ASSET_SLIDER_BAR );
	uiInfo.uiDC.Assets.sliderThumb			= trap->R_RegisterShader( ASSET_SLIDER_THUMB );

	//QtZ: New composite crosshair style
//	for ( n=0; n<NUM_CROSSHAIRS; n++ )
//		uiInfo.uiDC.Assets.crosshairShader[n] = trap->R_RegisterShader( va("gfx/hud/crosshair_bit%i", (1<<n)) );

	uiInfo.newHighScoreSound = trap->S_RegisterSound("sound/feedback/voc_newhighscore.wav", qfalse);
}

void UI_DrawSides2(float x, float y, float w, float h, float size) {
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.xscale;
	trap->R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void UI_DrawTopBottom2(float x, float y, float w, float h, float size) {
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.yscale;
	trap->R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

// Coordinates are 640x480 virtual values
void UI_DrawRect2( float x, float y, float width, float height, float size, const vector4 *color ) {
	trap->R_SetColor( color );

	UI_DrawTopBottom2(x, y, width, height, size);
	UI_DrawSides2(x, y, width, height, size);

	trap->R_SetColor( NULL );
}

float Text_Width(const char *text, float scale, int limit) {
	unsigned int count;
	size_t len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *font;
	
		 if ( scale <= ui_smallFont->value )	font = &uiInfo.uiDC.Assets.smallFont;
	else if ( scale <= ui_bigFont->value )		font = &uiInfo.uiDC.Assets.textFont;
	else										font = &uiInfo.uiDC.Assets.bigFont;

	useScale = scale * font->glyphScale;
	out = 0;
	if (text) {
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s];
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}
	return (out * useScale);
}

float Text_Height(const char *text, float scale, int limit) {
	size_t len;
	unsigned int count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *font;

		 if ( scale <= ui_smallFont->value )	font = &uiInfo.uiDC.Assets.smallFont;
	else if ( scale <= ui_bigFont->value )		font = &uiInfo.uiDC.Assets.textFont;
	else										font = &uiInfo.uiDC.Assets.bigFont;

	useScale = scale * font->glyphScale;
	max = 0;
	if (text) {
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
				if (max < glyph->height) {
					max = (float)glyph->height;
				}
				s++;
				count++;
			}
		}
	}
	return (max * useScale);
}

void Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
	float w, h;
	w = width * scale;
	h = height * scale;
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap->R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void Text_Paint(float x, float y, float scale, const vector4 *color, const char *text, float adjust, int limit, int style) {
	size_t len;
	unsigned int count;
	vector4 newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &uiInfo.uiDC.Assets.textFont;
	if (scale <= ui_smallFont->value) {
		font = &uiInfo.uiDC.Assets.smallFont;
	} else if (scale >= ui_bigFont->value) {
		font = &uiInfo.uiDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
	if (text) {
		const char *s = text;
		trap->R_SetColor( color );
		memcpy(&newColor, color, sizeof(vector4));
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
			//int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
			//float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( &newColor, &g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor.a = color->a;
				trap->R_SetColor( &newColor );
				s += 2;
				continue;
			} else {
				vector4 colorBlack;
				float yadj = useScale * glyph->top;

				VectorCopy4( &g_color_table[ColorIndex(COLOR_BLACK)], &colorBlack );

				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack.a = newColor.a;
					trap->R_SetColor( &colorBlack );
					Text_PaintChar(x + ofs, y - yadj + ofs, 
						(float)glyph->imageWidth,
						(float)glyph->imageHeight,
						useScale, 
						glyph->s,
						glyph->t,
						glyph->s2,
						glyph->t2,
						glyph->glyph);
					trap->R_SetColor( &newColor );
					colorBlack.a = 1.0f;
				}
				Text_PaintChar(x, y - yadj, 
					(float)glyph->imageWidth,
					(float)glyph->imageHeight,
					useScale, 
					glyph->s,
					glyph->t,
					glyph->s2,
					glyph->t2,
					glyph->glyph);

				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
		}
		trap->R_SetColor( NULL );
	}
}

void Text_PaintWithCursor(float x, float y, float scale, vector4 *color, const char *text, size_t cursorPos, char cursor, int limit, int style) {
	size_t len;
	unsigned int count;
	vector4 newColor;
	glyphInfo_t *glyph, *glyph2;
	float yadj;
	float useScale;
	fontInfo_t *font = &uiInfo.uiDC.Assets.textFont;
	if (scale <= ui_smallFont->value) {
		font = &uiInfo.uiDC.Assets.smallFont;
	} else if (scale >= ui_bigFont->value) {
		font = &uiInfo.uiDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
	if (text) {
		const char *s = text;
		trap->R_SetColor( color );
		memcpy(&newColor, color, sizeof(vector4));
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		glyph2 = &font->glyphs[ (int) cursor];
		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
			//int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
			//float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( &newColor, &g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor.a = color->a;
				trap->R_SetColor( &newColor );
				s += 2;
				continue;
			} else {
				yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					vector4 colorBlack;
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;

					VectorCopy4( &g_color_table[ColorIndex(COLOR_BLACK)], &colorBlack );

					colorBlack.a = newColor.a;
					trap->R_SetColor( &colorBlack );
					Text_PaintChar(x + ofs, y - yadj + ofs, 
						(float)glyph->imageWidth,
						(float)glyph->imageHeight,
						useScale, 
						glyph->s,
						glyph->t,
						glyph->s2,
						glyph->t2,
						glyph->glyph);
					colorBlack.a = 1.0f;
					trap->R_SetColor( &newColor );
				}
				Text_PaintChar(x, y - yadj, 
					(float)glyph->imageWidth,
					(float)glyph->imageHeight,
					useScale, 
					glyph->s,
					glyph->t,
					glyph->s2,
					glyph->t2,
					glyph->glyph);

				yadj = useScale * glyph2->top;
				if (count == cursorPos && !((uiInfo.uiDC.realTime/BLINK_DIVISOR) & 1)) {
					Text_PaintChar(x, y - yadj, 
						(float)glyph2->imageWidth,
						(float)glyph2->imageHeight,
						useScale, 
						glyph2->s,
						glyph2->t,
						glyph2->s2,
						glyph2->t2,
						glyph2->glyph);
				}

				x += (glyph->xSkip * useScale);
				s++;
				count++;
			}
		}
		// need to paint cursor at end of text
		if (cursorPos == len && !((uiInfo.uiDC.realTime/BLINK_DIVISOR) & 1)) {
			yadj = useScale * glyph2->top;
			Text_PaintChar(x, y - yadj, 
				(float)glyph2->imageWidth,
				(float)glyph2->imageHeight,
				useScale, 
				glyph2->s,
				glyph2->t,
				glyph2->s2,
				glyph2->t2,
				glyph2->glyph);

		}

		trap->R_SetColor( NULL );
	}
}


static void Text_Paint_Limit(float *maxX, float x, float y, float scale, vector4 *color, const char* text, float adjust, int limit) {
	size_t len;
	unsigned int count;
	vector4 newColor;
	glyphInfo_t *glyph;
	if (text) {
		const char *s = text;
		float max = *maxX;
		float useScale;
		fontInfo_t *font = &uiInfo.uiDC.Assets.textFont;
		if (scale <= ui_smallFont->value) {
			font = &uiInfo.uiDC.Assets.smallFont;
		} else if (scale > ui_bigFont->value) {
			font = &uiInfo.uiDC.Assets.bigFont;
		}
		useScale = scale * font->glyphScale;
		trap->R_SetColor( color );
		len = strlen(text);					 
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
			if ( Q_IsColorString( s ) ) {
				memcpy( &newColor, &g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor.a = color->a;
				trap->R_SetColor( &newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (Text_Width(s, useScale, 1) + x > max) {
					*maxX = 0;
					break;
				}
				Text_PaintChar(x, y - yadj, 
					(float)glyph->imageWidth,
					(float)glyph->imageHeight,
					useScale, 
					glyph->s,
					glyph->t,
					glyph->s2,
					glyph->t2,
					glyph->glyph);
				x += (glyph->xSkip * useScale) + adjust;
				*maxX = x;
				count++;
				s++;
			}
		}
		trap->R_SetColor( NULL );
	}

}

void UI_DrawCenteredPic(qhandle_t image, int w, int h) {
	float x, y;
	x = (SCREEN_WIDTH - w) / 2;
	y = (SCREEN_HEIGHT - h) / 2;
	UI_DrawHandlePic(x, y, (float)w, (float)h, image);
}

static const char *UI_SelectedHead( size_t index, size_t *actual ) {
	size_t i, c=0;
	*actual = 0;

	for ( i=0; i<uiInfo.characterCount; i++ ) {
		if ( uiInfo.characterList[i].active ) {
			if ( c == index ) {
				*actual = i;
				return uiInfo.characterList[i].name;
			}
			else
				c++;
		}
	}
	return "";
}

static const char *UI_SelectedMap( size_t index, size_t *actual ) {
	size_t i, c=0;
	*actual = 0;
	for ( i=0; i<uiInfo.mapCount; i++ ) {
		if ( uiInfo.mapList[i].active ) {
			if ( c == index ) {
				*actual = i;
				return uiInfo.mapList[i].mapName;
			}
			else
				c++;
		}
	}
	return "";
}

struct serverStatusCvar_s {
	char *name, *altName;
} serverStatusCvars[] = {
	{ "sv_hostname", "Name" },
	{ "Address", "" },
	{ "gamename", "Game name" },
	{ "sv_gametype", "Game type" },
	{ "mapname", "Map" },
	{ "com_version", "" },
	{ "protocol", "" },
	{ "timelimit", "" },
	{ "fraglimit", "" },
	{ NULL, NULL }
};

static void UI_SortServerStatusInfo( serverStatusInfo_t *info ) {
	int i, j, index;
	char *tmp1, *tmp2;

	// FIXME: if "gamename" == "baseq3" or "missionpack" then
	// replace the gametype number by FFA, CTF etc.
	//
	index = 0;
	for (i = 0; serverStatusCvars[i].name; i++) {
		for (j = 0; j < info->numLines; j++) {
			if ( !info->lines[j][1] || info->lines[j][1][0] ) {
				continue;
			}
			if ( !Q_stricmp(serverStatusCvars[i].name, info->lines[j][0]) ) {
				// swap lines
				tmp1 = info->lines[index][0];
				tmp2 = info->lines[index][3];
				info->lines[index][0] = info->lines[j][0];
				info->lines[index][3] = info->lines[j][3];
				info->lines[j][0] = tmp1;
				info->lines[j][3] = tmp2;
				//
				if ( strlen(serverStatusCvars[i].altName) ) {
					info->lines[index][0] = serverStatusCvars[i].altName;
				}
				index++;
			}
		}
	}
}

static int UI_GetServerStatusInfo( char *serverAddress, serverStatusInfo_t *info ) {
	char *p, *score, *ping, *name;
	int i;
	size_t len;

	if (!info) {
		trap->LAN_GetServerStatus( serverAddress, NULL, 0);
		return qfalse;
	}
	memset(info, 0, sizeof(*info));
	if ( trap->LAN_GetServerStatus( serverAddress, info->text, sizeof(info->text)) ) {
		Q_strncpyz(info->address, serverAddress, sizeof(info->address));
		p = info->text;
		info->numLines = 0;
		info->lines[info->numLines][0] = "Address";
		info->lines[info->numLines][1] = "";
		info->lines[info->numLines][2] = "";
		info->lines[info->numLines][3] = info->address;
		info->numLines++;
		// get the cvars
		while (p && *p) {
			p = strchr(p, '\\');
			if (!p) break;
			*p++ = '\0';
			if (*p == '\\')
				break;
			info->lines[info->numLines][0] = p;
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			p = strchr(p, '\\');
			if (!p) break;
			*p++ = '\0';
			info->lines[info->numLines][3] = p;

			info->numLines++;
			if (info->numLines >= MAX_SERVERSTATUS_LINES)
				break;
		}
		// get the player list
		if (info->numLines < MAX_SERVERSTATUS_LINES-3) {
			// empty line
			info->lines[info->numLines][0] = "";
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			info->lines[info->numLines][3] = "";
			info->numLines++;
			// header
			info->lines[info->numLines][0] = "num";
			info->lines[info->numLines][1] = "score";
			info->lines[info->numLines][2] = "ping";
			info->lines[info->numLines][3] = "name";
			info->numLines++;
			// parse players
			i = 0;
			len = 0;
			while (p && *p) {
				if (*p == '\\')
					*p++ = '\0';
				if (!p)
					break;
				score = p;
				p = strchr(p, ' ');
				if (!p)
					break;
				*p++ = '\0';
				ping = p;
				p = strchr(p, ' ');
				if (!p)
					break;
				*p++ = '\0';
				name = p;
				Com_sprintf(&info->pings[len], sizeof(info->pings)-len, "%d", i);
				info->lines[info->numLines][0] = &info->pings[len];
				len += strlen(&info->pings[len]) + 1;
				info->lines[info->numLines][1] = score;
				info->lines[info->numLines][2] = ping;
				info->lines[info->numLines][3] = name;
				info->numLines++;
				if (info->numLines >= MAX_SERVERSTATUS_LINES)
					break;
				p = strchr(p, '\\');
				if (!p)
					break;
				*p++ = '\0';
				//
				i++;
			}
		}
		UI_SortServerStatusInfo( info );
		return qtrue;
	}
	return qfalse;
}

static void UI_BuildServerStatus( qboolean force ) {
	if ( uiInfo.nextFindPlayerRefresh )
		return;

	if ( !force ) {
		if ( !uiInfo.nextServerStatusRefresh || uiInfo.nextServerStatusRefresh > uiInfo.uiDC.realTime )
			return;
	}
	else {
		Menu_SetFeederSelection( NULL, FEEDER_SERVERSTATUS, 0, NULL );
		uiInfo.serverStatusInfo.numLines = 0;
		// reset all server status requests
		trap->LAN_GetServerStatus( NULL, NULL, 0 );
	}
	if ( uiInfo.serverStatus.currentServer > uiInfo.serverStatus.numDisplayServers || uiInfo.serverStatus.numDisplayServers == 0 )
		return;

	if ( UI_GetServerStatusInfo( uiInfo.serverStatusAddress, &uiInfo.serverStatusInfo ) ) {
		uiInfo.nextServerStatusRefresh = 0;
		UI_GetServerStatusInfo( uiInfo.serverStatusAddress, NULL );
	}
	else
		uiInfo.nextServerStatusRefresh = uiInfo.uiDC.realTime + 500;
}

// Convert ui's net source to AS_* used by trap calls.
int UI_SourceForLAN( void ) {
	switch ( ui_netSource->integer ) {
	default:
	case UIAS_LOCAL:
		return AS_LOCAL;
	case UIAS_GLOBAL1:
	case UIAS_GLOBAL2:
	case UIAS_GLOBAL3:
	case UIAS_GLOBAL4:
	case UIAS_GLOBAL5:
		return AS_GLOBAL;
	case UIAS_FAVORITES:
		return AS_FAVORITES;
	}
}

static qboolean updateModel = qtrue;
static qboolean updateOpponentModel = qtrue;
static void UI_FeederSelection(float feederID, size_t index) {
	static char info[MAX_STRING_CHARS];
	if (feederID == FEEDER_HEADS) {
		size_t actual;
		UI_SelectedHead(index, &actual);
		index = actual;
		if (index < uiInfo.characterCount) {
			trap->Cvar_Set( "cg_model", uiInfo.characterList[index].base);
			updateModel = qtrue;
		}
	} else if (feederID == FEEDER_Q3HEADS) {
		if (index < uiInfo.q3HeadCount) {
			trap->Cvar_Set( "cg_model", uiInfo.q3HeadNames[index]);
			updateModel = qtrue;
		}
	} else if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS) {
		size_t actual;
		UI_SelectedMap(index, &actual);
		trap->Cvar_Set("ui_mapIndex", va("%d", (int)index));
		ui_mapIndex->integer = (int)index;

		if (feederID == FEEDER_MAPS) {
			ui_currentMap->integer = (int)actual;
			trap->Cvar_Set("ui_currentMap", va("%d", (int)actual));
			trap->Cvar_Set("ui_opponentModel", uiInfo.mapList[ui_currentMap->integer].opponentName);
			updateOpponentModel = qtrue;
		} else {
			ui_currentNetMap->integer = (int)actual;
			trap->Cvar_Set("ui_currentNetMap", va("%d", (int)actual));
		}

	} else if (feederID == FEEDER_SERVERS) {
		uiInfo.serverStatus.currentServer = index;
		trap->LAN_GetServerInfo(UI_SourceForLAN(), uiInfo.serverStatus.displayServers[index], info, MAX_STRING_CHARS);
		uiInfo.serverStatus.currentServerPreview = trap->R_RegisterShader( va( "gfx/maps/%s", Info_ValueForKey( info, "mapname" ) ) );
	} else if (feederID == FEEDER_SERVERSTATUS) {
		//
	} else if (feederID == FEEDER_FINDPLAYER) {
		uiInfo.currentFoundPlayerServer = index;
		//
		if ( index < uiInfo.numFoundPlayerServers-1) {
			// build a new server status for this server
			Q_strncpyz(uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer], sizeof(uiInfo.serverStatusAddress));
			Menu_SetFeederSelection(NULL, FEEDER_SERVERSTATUS, 0, NULL);
			UI_BuildServerStatus(qtrue);
		}
	} else if (feederID == FEEDER_PLAYER_LIST) {
		uiInfo.playerIndex = index;
	} else if (feederID == FEEDER_TEAM_LIST) {
		uiInfo.teamIndex = index;
	} else if (feederID == FEEDER_MODS) {
		uiInfo.modIndex = index;
	} else if (feederID == FEEDER_DEMOS) {
		uiInfo.demoIndex = index;
	}
}

static void UI_BuildFindPlayerList(qboolean force) {
	static int numFound, numTimeOuts;
	int i, j, resend;
	serverStatusInfo_t info;
	char name[MAX_NETNAME];
	char infoString[MAX_STRING_CHARS];
	int  lanSource;

	if (!force) {
		if (!uiInfo.nextFindPlayerRefresh || uiInfo.nextFindPlayerRefresh > uiInfo.uiDC.realTime) {
			return;
		}
	}
	else {
		memset(&uiInfo.pendingServerStatus, 0, sizeof(uiInfo.pendingServerStatus));
		uiInfo.numFoundPlayerServers = 0;
		uiInfo.currentFoundPlayerServer = 0;
		trap->Cvar_VariableStringBuffer( "ui_findPlayer", uiInfo.findPlayerName, sizeof(uiInfo.findPlayerName));
		Q_CleanStr(uiInfo.findPlayerName);
		// should have a string of some length
		if (!strlen(uiInfo.findPlayerName)) {
			uiInfo.nextFindPlayerRefresh = 0;
			return;
		}
		// set resend time
		resend = ui_serverStatusTimeOut->integer / 2 - 10;
		if (resend < 50) {
			resend = 50;
		}
		trap->Cvar_Set("cl_serverStatusResendTime", va("%d", resend));
		// reset all server status requests
		trap->LAN_GetServerStatus( NULL, NULL, 0);
		//
		uiInfo.numFoundPlayerServers = 1;
		Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
			sizeof(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1]),
			"searching %d...", (int)uiInfo.pendingServerStatus.num);
		numFound = 0;
		numTimeOuts++;
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		// if this pending server is valid
		if (uiInfo.pendingServerStatus.server[i].valid) {
			// try to get the server status for this server
			if (UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[i].adrstr, &info ) ) {
				//
				numFound++;
				// parse through the server status lines
				for (j = 0; j < info.numLines; j++) {
					// should have ping info
					if ( !info.lines[j][2] || !info.lines[j][2][0] ) {
						continue;
					}
					// clean string first
					Q_strncpyz(name, info.lines[j][3], sizeof(name));
					Q_CleanStr(name);
					// if the player name is a substring
					if (Q_stristr(name, uiInfo.findPlayerName)) {
						// add to found server list if we have space (always leave space for a line with the number found)
						if (uiInfo.numFoundPlayerServers < MAX_FOUNDPLAYER_SERVERS-1) {
							//
							Q_strncpyz(uiInfo.foundPlayerServerAddresses[uiInfo.numFoundPlayerServers-1],
								uiInfo.pendingServerStatus.server[i].adrstr,
								sizeof(uiInfo.foundPlayerServerAddresses[0]));
							Q_strncpyz(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
								uiInfo.pendingServerStatus.server[i].name,
								sizeof(uiInfo.foundPlayerServerNames[0]));
							uiInfo.numFoundPlayerServers++;
						}
						else {
							// can't add any more so we're done
							uiInfo.pendingServerStatus.num = uiInfo.serverStatus.numDisplayServers;
						}
					}
				}
				Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
					sizeof(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1]),
					"searching %d/%d...", (int)uiInfo.pendingServerStatus.num, numFound);
				// retrieved the server status so reuse this spot
				uiInfo.pendingServerStatus.server[i].valid = qfalse;
			}
		}
		// if empty pending slot or timed out
		if (!uiInfo.pendingServerStatus.server[i].valid ||
			uiInfo.pendingServerStatus.server[i].startTime < uiInfo.uiDC.realTime - ui_serverStatusTimeOut->integer) {
				if (uiInfo.pendingServerStatus.server[i].valid) {
					numTimeOuts++;
				}
				// reset server status request for this address
				UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[i].adrstr, NULL );
				// reuse pending slot
				uiInfo.pendingServerStatus.server[i].valid = qfalse;
				// if we didn't try to get the status of all servers in the main browser yet
				if (uiInfo.pendingServerStatus.num < uiInfo.serverStatus.numDisplayServers) {
					uiInfo.pendingServerStatus.server[i].startTime = uiInfo.uiDC.realTime;
					lanSource = UI_SourceForLAN();
					trap->LAN_GetServerAddressString(lanSource, uiInfo.serverStatus.displayServers[uiInfo.pendingServerStatus.num],
						uiInfo.pendingServerStatus.server[i].adrstr, sizeof(uiInfo.pendingServerStatus.server[i].adrstr));
					trap->LAN_GetServerInfo(lanSource, uiInfo.serverStatus.displayServers[uiInfo.pendingServerStatus.num], infoString, sizeof(infoString));
					Q_strncpyz(uiInfo.pendingServerStatus.server[i].name, Info_ValueForKey(infoString, "hostname"), sizeof(uiInfo.pendingServerStatus.server[0].name));
					uiInfo.pendingServerStatus.server[i].valid = qtrue;
					uiInfo.pendingServerStatus.num++;
					Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
						sizeof(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1]),
						"searching %d/%d...", (int)uiInfo.pendingServerStatus.num, numFound);
				}
		}
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if (uiInfo.pendingServerStatus.server[i].valid) {
			break;
		}
	}
	// if still trying to retrieve server status info
	if (i < MAX_SERVERSTATUSREQUESTS) {
		uiInfo.nextFindPlayerRefresh = uiInfo.uiDC.realTime + 25;
	}
	else {
		// add a line that shows the number of servers found
		if (!uiInfo.numFoundPlayerServers) {
			Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1], sizeof(uiInfo.foundPlayerServerAddresses[0]), "no servers found");
		}
		else {
			Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1], sizeof(uiInfo.foundPlayerServerAddresses[0]),
				"%d server%s found with player %s", (int)(uiInfo.numFoundPlayerServers-1),
				uiInfo.numFoundPlayerServers == 2 ? "":"s", uiInfo.findPlayerName);
		}
		uiInfo.nextFindPlayerRefresh = 0;
		// show the server status info for the selected server
		UI_FeederSelection(FEEDER_FINDPLAYER, uiInfo.currentFoundPlayerServer);
	}
}

static void UI_RemoveServerFromDisplayList( int num ) {
	int i, j;

	for ( i=0; i<uiInfo.serverStatus.numDisplayServers; i++ ) {
		if ( uiInfo.serverStatus.displayServers[i] == num ) {
			uiInfo.serverStatus.numDisplayServers--;
			for ( j=i; j<uiInfo.serverStatus.numDisplayServers; j++ )
				uiInfo.serverStatus.displayServers[j] = uiInfo.serverStatus.displayServers[j+1];

			return;
		}
	}
}

static void UI_InsertServerIntoDisplayList( int num, size_t position ) {
	size_t i;

	if ( position < 0 || position > uiInfo.serverStatus.numDisplayServers )
		return;

	uiInfo.serverStatus.numDisplayServers++;
	for ( i=uiInfo.serverStatus.numDisplayServers; i>position; i-- )
		uiInfo.serverStatus.displayServers[i] = uiInfo.serverStatus.displayServers[i-1];

	uiInfo.serverStatus.displayServers[position] = num;
}

static void UI_BinaryServerInsertion( int num ) {
	int res;
	size_t offset, len, mid;

	// use binary search to insert server
	len = uiInfo.serverStatus.numDisplayServers;
	mid = len;
	offset = 0;
	res = 0;
	while ( mid > 0 ) {
		mid = len >> 1;
		//
		res = trap->LAN_CompareServers( UI_SourceForLAN(), uiInfo.serverStatus.sortKey, uiInfo.serverStatus.sortDir, num, uiInfo.serverStatus.displayServers[offset+mid] );
		// if equal
		if ( res == 0 ) {
			UI_InsertServerIntoDisplayList( num, offset+mid );
			return;
		}
		// if larger
		else if ( res == 1 ) {
			offset += mid;
			len -= mid;
		}
		// if smaller
		else
			len -= mid;
	}
	if ( res == 1 )
		offset++;

	UI_InsertServerIntoDisplayList( num, offset );
}

static void UI_BuildServerDisplayList( qboolean force ) {
	int i, count, clients, maxClients, ping, game, visible;
	char info[MAX_STRING_CHARS];
	static int numinvisible;
	int	lanSource;
	size_t len;

	if ( !(force || uiInfo.uiDC.realTime > uiInfo.serverStatus.nextDisplayRefresh) )
		return;

	// if we shouldn't reset
	if ( force == 2 )
		force = 0;

	// do motd updates here too
	trap->Cvar_VariableStringBuffer( "cl_motdString", uiInfo.serverStatus.motd, sizeof( uiInfo.serverStatus.motd ) );
	len = strlen( uiInfo.serverStatus.motd );
	if ( len == 0 ) {
		strcpy( uiInfo.serverStatus.motd, "Welcome to "QTZ_VERSION );
		len = strlen( uiInfo.serverStatus.motd );
	} 
	if ( len != uiInfo.serverStatus.motdLen ) {
		uiInfo.serverStatus.motdLen = len;
		uiInfo.serverStatus.motdWidth = -1;
	} 

	lanSource = UI_SourceForLAN();

	if ( force ) {
		numinvisible = 0;
		// clear number of displayed servers
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		// set list box index to zero
		Menu_SetFeederSelection( NULL, FEEDER_SERVERS, 0, NULL );
		// mark all servers as visible so we store ping updates for them
		trap->LAN_MarkServerVisible( lanSource, -1, qtrue );
	}

	// get the server count (comes from the master)
	count = trap->LAN_GetServerCount( lanSource );
	if ( count == -1 || (ui_netSource->integer == UIAS_LOCAL && count == 0) ) {
		// still waiting on a response from the master
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 500;
		return;
	}

	visible = qfalse;
	for ( i=0; i<count; i++ ) {
		// if we already got info for this server
		if ( !trap->LAN_ServerIsVisible( lanSource, i ) )
			continue;

		visible = qtrue;
		// get the ping for this server
		ping = trap->LAN_GetServerPing( lanSource, i );
		if ( ping > 0 || ui_netSource->integer == UIAS_FAVORITES ) {
			trap->LAN_GetServerInfo( lanSource, i, info, sizeof( info ) );

			clients = atoi( Info_ValueForKey( info, "clients" ) );
			uiInfo.serverStatus.numPlayersOnServers += clients;

			if ( ui_browserShowEmpty->integer == 0 ) {
				if ( clients == 0 ) {
					trap->LAN_MarkServerVisible( lanSource, i, qfalse );
					continue;
				}
			}

			if ( ui_browserShowFull->integer == 0 ) {
				maxClients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
				if ( clients == maxClients ) {
					trap->LAN_MarkServerVisible( lanSource, i, qfalse );
					continue;
				}
			}

			if ( ui_gametype->integer != -1 ) {
				game = atoi( Info_ValueForKey( info, "gametype" ) );
				if ( game != ui_gametype->integer ) {
					trap->LAN_MarkServerVisible( lanSource, i, qfalse );
					continue;
				}
			}

			if ( ui_serverFilterType->integer > 0 ) {
				if ( Q_stricmp( Info_ValueForKey( info, "game" ), serverFilters[ui_serverFilterType->integer].basedir ) ) {
					trap->LAN_MarkServerVisible( lanSource, i, qfalse );
					continue;
				}
			}
			// make sure we never add a favorite server twice
			if ( ui_netSource->integer == UIAS_FAVORITES )
				UI_RemoveServerFromDisplayList( i );

			// insert the server into the list
			UI_BinaryServerInsertion( i );
			// done with this server
			if ( ping > 0 ) {
				trap->LAN_MarkServerVisible( lanSource, i, qfalse );
				numinvisible++;
			}
		}
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime;

	// if there were no servers visible for ping updates
	if ( !visible ) {
	//	UI_StopServerRefresh();
	//	uiInfo.serverStatus.nextDisplayRefresh = 0;
	}
}

static void UI_StopServerRefresh( void ) {
	int count;

	// not currently refreshing
	if ( !uiInfo.serverStatus.refreshActive )
		return;

	uiInfo.serverStatus.refreshActive = qfalse;
	Com_Printf( "%d servers listed in browser with %d players.\n", (int)uiInfo.serverStatus.numDisplayServers, uiInfo.serverStatus.numPlayersOnServers );
	count = trap->LAN_GetServerCount( UI_SourceForLAN() );
	if ( count - uiInfo.serverStatus.numDisplayServers > 0 )
		Com_Printf( "%d servers not listed due to packet loss or pings higher than %d\n", count - (int)uiInfo.serverStatus.numDisplayServers, (int)trap->Cvar_VariableValue( "cl_maxPing" ) );
}

static void UI_DoServerRefresh( void ) {
	qboolean wait = qfalse;

	if ( !uiInfo.serverStatus.refreshActive )
		return;

	if ( ui_netSource->integer != UIAS_FAVORITES ) {
		if ( ui_netSource->integer == UIAS_LOCAL ) {
			if ( !trap->LAN_GetServerCount( AS_LOCAL ) )
				wait = qtrue;
		}
		else {
			if ( trap->LAN_GetServerCount( AS_GLOBAL ) < 0 )
				wait = qtrue;
		}
	}

	if ( uiInfo.uiDC.realTime < uiInfo.serverStatus.refreshtime ) {
		if ( wait )
			return;
	}

	// if still trying to retrieve pings
	if ( trap->LAN_UpdateVisiblePings( UI_SourceForLAN() ) )
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
	else if ( !wait ) {
		// get the last servers in the list
		UI_BuildServerDisplayList( 2 );
		// stop the refresh
		UI_StopServerRefresh();
	}

	UI_BuildServerDisplayList( qfalse );
}

int frameCount = 0;
int startTime;

#define	UI_FPS_FRAMES	4
void UI_Refresh( int realtime ) {
	static int index;
	static int	previousTimes[UI_FPS_FRAMES];

	//if ( !( trap->Key_GetCatcher() & KEYCATCH_UI ) ) {
	//	return;
	//}

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realtime;

	previousTimes[index % UI_FPS_FRAMES] = uiInfo.uiDC.frameTime;
	index++;
	if ( index > UI_FPS_FRAMES ) {
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < UI_FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		uiInfo.uiDC.FPS = 1000.0f * UI_FPS_FRAMES / (float)total;
	}

	if (Menu_Count() > 0) {
		// paint all the menus
		Menu_PaintAll();
		// refresh server browser list
		UI_DoServerRefresh();
		// refresh server status
		UI_BuildServerStatus(qfalse);
		// refresh find player list
		UI_BuildFindPlayerList(qfalse);
	} 

	// draw cursor
	UI_SetColor( NULL );
	if (Menu_Count() > 0 && (trap->Key_GetCatcher() & KEYCATCH_UI)) {
		UI_DrawHandlePic( (float)(uiInfo.uiDC.cursorx-16), (float)(uiInfo.uiDC.cursory-16), 32.0f, 32.0f, uiInfo.uiDC.Assets.cursor);
	}
}

void UI_Shutdown( void ) {
	trap->LAN_SaveServersToCache();
}

char *defaultMenu = NULL;

char *GetMenuBuffer(const char *filename) {
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUFILE];

	len = trap->FS_Open( filename, &f, FS_READ );
	if ( !f ) {
		trap->Print( S_COLOR_RED "menu file not found: %s, using default\n", filename );
		return defaultMenu;
	}
	if ( len >= MAX_MENUFILE ) {
		trap->Print( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", filename, len, MAX_MENUFILE );
		trap->FS_Close( f );
		return defaultMenu;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );
	//COM_Compress(buf);
	return buf;

}

qboolean Asset_Parse(int handle) {
	pc_token_t token;
	const char *tempStr;

	if (!trap->PC_ReadTokenHandle(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}

	while ( 1 ) {

		memset(&token, 0, sizeof(pc_token_t));

		if (!trap->PC_ReadTokenHandle(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			trap->R_RegisterFont(tempStr, pointSize, &uiInfo.uiDC.Assets.textFont);
			uiInfo.uiDC.Assets.fontRegistered = qtrue;
			continue;
		}

		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			trap->R_RegisterFont(tempStr, pointSize, &uiInfo.uiDC.Assets.smallFont);
			continue;
		}

		if (Q_stricmp(token.string, "bigFont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			trap->R_RegisterFont(tempStr, pointSize, &uiInfo.uiDC.Assets.bigFont);
			continue;
		}


		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.gradientBar = trap->R_RegisterShader(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuEnterSound = trap->S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuExitSound = trap->S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.itemFocusSound = trap->S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuBuzzSound = trap->S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) {
			if (!PC_String_Parse(handle, &uiInfo.uiDC.Assets.cursorStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.cursor = trap->R_RegisterShader( uiInfo.uiDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &uiInfo.uiDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &uiInfo.uiDC.Assets.shadowColor)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor.a;
			continue;
		}

	}
	return qfalse;
}

void Font_Report( void ) {
	int i;
	Com_Printf("Font Info\n");
	Com_Printf("=========\n");
	for ( i = 32; i < 96; i++) {
		Com_Printf("Glyph handle %i: %i\n", i, uiInfo.uiDC.Assets.textFont.glyphs[i].glyph);
	}
}

void UI_Report( void ) {
	String_Report();
	//Font_Report();

}

void UI_ParseMenu(const char *menuFile) {
	int handle;
	pc_token_t token;

	Com_Printf("Parsing menu file:%s\n", menuFile);

	handle = trap->PC_LoadSourceHandle(menuFile);
	if (!handle) {
		return;
	}

	while ( 1 ) {
		memset(&token, 0, sizeof(pc_token_t));
		if (!trap->PC_ReadTokenHandle( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle);
		}
	}
	trap->PC_FreeSourceHandle(handle);
}

qboolean Load_Menu(int handle) {
	pc_token_t token;

	if (!trap->PC_ReadTokenHandle(handle, &token))
		return qfalse;
	if (token.string[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		if (!trap->PC_ReadTokenHandle(handle, &token))
			return qfalse;

		if ( token.string[0] == 0 ) {
			return qfalse;
		}

		if ( token.string[0] == '}' ) {
			return qtrue;
		}

		UI_ParseMenu(token.string); 
	}
	return qfalse;
}

void UI_LoadMenus(const char *menuFile, qboolean reset) {
	pc_token_t token;
	int handle;
	int start;

	start = trap->Milliseconds();

	handle = trap->PC_LoadSourceHandle( menuFile );
	if (!handle) {
		Com_Printf( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );
		handle = trap->PC_LoadSourceHandle( "ui/menus.txt" );
		if (!handle) {
			trap->Error( ERR_DROP, S_COLOR_RED "default menu file not found: ui/menus.txt, unable to continue!" );
		}
	}

	if (reset) {
		Menu_Reset();
	}

	while ( 1 ) {
		if (!trap->PC_ReadTokenHandle(handle, &token))
			break;
		if( token.string[0] == 0 || token.string[0] == '}') {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "loadmenu") == 0) {
			if (Load_Menu(handle)) {
				continue;
			} else {
				break;
			}
		}
	}

	Com_Printf("UI menu load time = %d milli seconds\n", trap->Milliseconds() - start);

	trap->PC_FreeSourceHandle( handle );
}

void UI_Load( void ) {
	char lastName[1024];
	menuDef_t *menu = Menu_GetFocused();
	char *menuSet = UI_Cvar_VariableString("ui_menuFiles");
	if (menu && menu->window.name) {
		strcpy(lastName, menu->window.name);
	}
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/menus.txt";
	}

	String_Init();

	UI_LoadArenas();

	UI_LoadMenus(menuSet, qtrue);
	Menus_CloseAll();
	Menus_ActivateByName(lastName);

}

static void UI_SetCapFragLimits(qboolean uiVars) {
	int cap = 5;
	int frag = 10;
	if (uiVars) {
		trap->Cvar_Set("ui_captureLimit", va("%d", cap));
		trap->Cvar_Set("ui_fragLimit", va("%d", frag));
	} else {
		trap->Cvar_Set("capturelimit", va("%d", cap));
		trap->Cvar_Set("fraglimit", va("%d", frag));
	}
}

static void UI_DrawGameType(rectDef_t *rect, float scale, vector4 *color, int textStyle) {
	Text_Paint(rect->x, rect->y, scale, color, GametypeStringForID( ui_gametype->integer ), 0, 0, textStyle);
}

static void UI_DrawSkill(rectDef_t *rect, float scale, vector4 *color, int textStyle) {
	int i;
	i = (int)trap->Cvar_VariableValue( "g_difficulty" );
	if (i < 1 || i > numSkillLevels) {
		i = 1;
	}
	Text_Paint(rect->x, rect->y, scale, color, skillLevels[i-1],0, 0, textStyle);
}

static void UI_DrawTeamName(rectDef_t *rect, float scale, vector4 *color, qboolean blue, int textStyle) {
	Text_Paint(rect->x, rect->y, scale, color, va("%s: %s", (blue) ? "Blue" : "Red", UI_Cvar_VariableString((blue) ? "ui_blueTeam" : "ui_redTeam")),0, 0, textStyle);
}

static void UI_DrawTeamMember(rectDef_t *rect, float scale, vector4 *color, qboolean blue, int num, int textStyle) {
	// 0 - None
	// 1 - Human
	// 2..NumCharacters - Bot
	int value = (int)trap->Cvar_VariableValue( va( blue ? "ui_blueteam%i" : "ui_redteam%i", num ) );
	const char *text;
	if (value <= 0) {
		text = "Closed";
	} else if (value == 1) {
		text = "Human";
	} else {
		value -= 2;

		if (ui_gametype->integer >= GT_TEAMBLOOD) {
			if (value >= uiInfo.characterCount) {
				value = 0;
			}
			text = uiInfo.characterList[value].name;
		} else {
			if (value >= UI_GetNumBots()) {
				value = 0;
			}
			text = UI_GetBotNameByNumber(value);
		}
	}
	Text_Paint(rect->x, rect->y, scale, color, text, 0, 0, textStyle);
}

static void UI_DrawMapPreview(rectDef_t *rect, float scale, vector4 *color, qboolean net) {
	int map = (net) ? ui_currentNetMap->integer : ui_currentMap->integer;
	if (map < 0 || map > uiInfo.mapCount) {
		if (net) {
			ui_currentNetMap->integer = 0;
			trap->Cvar_Set("ui_currentNetMap", "0");
		} else {
			ui_currentMap->integer = 0;
			trap->Cvar_Set("ui_currentMap", "0");
		}
		map = 0;
	}

	if (uiInfo.mapList[map].levelShot == -1) {
		uiInfo.mapList[map].levelShot = trap->R_RegisterShader(uiInfo.mapList[map].imageName);
	}

	if (uiInfo.mapList[map].levelShot > 0) {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.mapList[map].levelShot);
	} else {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap->R_RegisterShader("gfx/maps/unknownmap"));
	}
}						 


static void UI_DrawMapTimeToBeat(rectDef_t *rect, float scale, vector4 *color, int textStyle) {
	int minutes, seconds, time;
	if (ui_currentMap->integer < 0 || ui_currentMap->integer > uiInfo.mapCount) {
		ui_currentMap->integer = 0;
		trap->Cvar_Set("ui_currentMap", "0");
	}

	time = uiInfo.mapList[ui_currentMap->integer].timeToBeat[ui_gametype->integer];

	minutes = time / 60;
	seconds = time % 60;

	Text_Paint(rect->x, rect->y, scale, color, va("%02i:%02i", minutes, seconds), 0, 0, textStyle);
}

static void UI_DrawPlayerModel(rectDef_t *rect) {
	static playerInfo_t info;
	char model[MAX_QPATH];
	char team[256];
	char head[256];
	vector3	viewangles;
	vector3	moveangles;

	strcpy(team, UI_Cvar_VariableString("ui_teamName"));
	strcpy(model, UI_Cvar_VariableString("cg_model"));
	strcpy(head, UI_Cvar_VariableString("cg_model"));
	if (updateModel) {
		memset( &info, 0, sizeof(playerInfo_t) );
		viewangles.yaw   = 180 - 10;
		viewangles.pitch = 0;
		viewangles.roll  = 0;
		VectorClear( &moveangles );
		UI_PlayerInfo_SetModel( &info, model, head, team);
		UI_PlayerInfo_SetInfo( &info, LEGS_IDLE, TORSO_STAND, &viewangles, &vec3_origin, WP_QUANTIZER, qfalse );
	//	UI_RegisterClientModelname( &info, model, head, team);
		updateModel = qfalse;
	}

	UI_DrawPlayer( rect->x, rect->y, rect->w, rect->h, &info, uiInfo.uiDC.realTime / 2);

}

static void UI_DrawNetSource(rectDef_t *rect, float scale, vector4 *color, int textStyle) {
	if (ui_netSource->integer < 0 || ui_netSource->integer > numNetSources) {
		ui_netSource->integer = 0;
	}
	Text_Paint(rect->x, rect->y, scale, color, va("Source: %s", netSources[ui_netSource->integer]), 0, 0, textStyle);
}

static void UI_DrawNetMapPreview(rectDef_t *rect, float scale, vector4 *color) {

	if (uiInfo.serverStatus.currentServerPreview > 0) {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.serverStatus.currentServerPreview);
	} else {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap->R_RegisterShader("gfx/menus/unknownmap"));
	}
}

static void UI_DrawNetFilter(rectDef_t *rect, float scale, vector4 *color, int textStyle) {
	if (ui_serverFilterType->integer < 0 || ui_serverFilterType->integer > numServerFilters) {
		ui_serverFilterType->integer = 0;
	}
	Text_Paint(rect->x, rect->y, scale, color, va("Filter: %s", serverFilters[ui_serverFilterType->integer].description), 0, 0, textStyle);
}

static void UI_DrawAllMapsSelection(rectDef_t *rect, float scale, vector4 *color, int textStyle, qboolean net) {
	int map = (net) ? ui_currentNetMap->integer : ui_currentMap->integer;
	if (map >= 0 && map < uiInfo.mapCount) {
		Text_Paint(rect->x, rect->y, scale, color, uiInfo.mapList[map].mapName, 0, 0, textStyle);
	}
}

static float UI_OwnerDrawWidth(int ownerDraw, float scale) {
	int i=0, value;
	const char *text;
	const char *s = NULL;

	switch (ownerDraw) {
	case UI_GAMETYPE:
		s = GametypeStringForID( ui_gametype->integer );
		break;
	case UI_SKILL:
		i = (int)trap->Cvar_VariableValue( "g_difficulty" );
		if (i < 1 || i > numSkillLevels) {
			i = 1;
		}
		s = skillLevels[i-1];
		break;
	case UI_BLUETEAMNAME:
		s = va("%s: %s", "Blue", UI_Cvar_VariableString("ui_blueTeam"));
		break;
	case UI_REDTEAMNAME:
		s = va("%s: %s", "Red", UI_Cvar_VariableString("ui_redTeam"));
		break;
	case UI_BLUETEAM1:
	case UI_BLUETEAM2:
	case UI_BLUETEAM3:
	case UI_BLUETEAM4:
	case UI_BLUETEAM5:
		value = (int)trap->Cvar_VariableValue(va("ui_blueteam%i", ownerDraw-UI_BLUETEAM1 + 1));
		if (value <= 0)
			text = "Closed";
		else if (value == 1)
			text = "Human";
		else
			text = "<FIXME>";
		s = va("%i. %s", ownerDraw-UI_BLUETEAM1 + 1, text);
		break;
	case UI_REDTEAM1:
	case UI_REDTEAM2:
	case UI_REDTEAM3:
	case UI_REDTEAM4:
	case UI_REDTEAM5:
		value = (int)trap->Cvar_VariableValue(va("ui_redteam%i", ownerDraw-UI_REDTEAM1 + 1));
		if (value <= 0)
			text = "Closed";
		else if (value == 1)
			text = "Human";
		else
			text = "";
		s = va("%i. %s", ownerDraw-UI_REDTEAM1 + 1, text);
		break;
	case UI_NETSOURCE:
		if (ui_netSource->integer < 0 || ui_netSource->integer > numNetSources) {
			ui_netSource->integer = 0;
		}
		s = va("Source: %s", netSources[ui_netSource->integer]);
		break;
	case UI_NETFILTER:
		if (ui_serverFilterType->integer < 0 || ui_serverFilterType->integer > numServerFilters) {
			ui_serverFilterType->integer = 0;
		}
		s = va("Filter: %s", serverFilters[ui_serverFilterType->integer].description );
		break;
	case UI_ALLMAPS_SELECTION:
		break;
	case UI_KEYBINDSTATUS:
		if (Display_KeyBindPending()) {
			s = "Waiting for new key... Press ESCAPE to cancel";
		} else {
			s = "Press ENTER or CLICK to change, Press BACKSPACE to clear";
		}
		break;
	case UI_SERVERREFRESHDATE:
		s = UI_Cvar_VariableString(va("ui_lastServerRefresh_%i", ui_netSource->integer));
		break;
	default:
		break;
  }

  if (s) {
	  return Text_Width(s, scale, 0);
  }
  return 0.0f;
}

static void UI_DrawBotName(rectDef_t *rect, float scale, vector4 *color, int textStyle) {
	size_t value = uiInfo.botIndex;
	int game = (int)trap->Cvar_VariableValue("sv_gametype");
	const char *text = "";
	if (game >= GT_TEAMBLOOD) {
		if (value >= uiInfo.characterCount) {
			value = 0;
		}
		text = uiInfo.characterList[value].name;
	} else {
		if (value >= UI_GetNumBots()) {
			value = 0;
		}
		text = UI_GetBotNameByNumber(value);
	}
	Text_Paint(rect->x, rect->y, scale, color, text, 0, 0, textStyle);
}

static void UI_DrawBotSkill(rectDef_t *rect, float scale, vector4 *color, int textStyle) {
	if (uiInfo.skillIndex >= 0 && uiInfo.skillIndex < numSkillLevels) {
		Text_Paint(rect->x, rect->y, scale, color, skillLevels[uiInfo.skillIndex], 0, 0, textStyle);
	}
}

static void UI_DrawRedBlue(rectDef_t *rect, float scale, vector4 *color, int textStyle) {
	Text_Paint(rect->x, rect->y, scale, color, (uiInfo.redBlue == 0) ? "Red" : "Blue", 0, 0, textStyle);
}

static void UI_DrawCrosshair(rectDef_t *rect, float scale, vector4 *color) {
	trap->R_SetColor( color );
	if (uiInfo.currentCrosshair < 0 || uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
		uiInfo.currentCrosshair = 0;
	}
	UI_DrawHandlePic( rect->x, rect->y - rect->h, rect->w, rect->h, uiInfo.uiDC.Assets.crosshairShader[uiInfo.currentCrosshair]);
	trap->R_SetColor( NULL );
}

static void UI_BuildPlayerList( void ) {
	uiClientState_t	cs;
	int		n, count, team, team2;
	size_t	playerTeamNumber;
	char	info[MAX_INFO_STRING];

	trap->GetClientState( &cs );
	trap->GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
	uiInfo.playerNumber = cs.clientNum;
	team = atoi(Info_ValueForKey(info, "t"));
	trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) );
	count = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	uiInfo.playerCount = 0;
	uiInfo.myTeamCount = 0;
	playerTeamNumber = 0;
	for( n = 0; n < count; n++ ) {
		trap->GetConfigString( CS_PLAYERS + n, info, MAX_INFO_STRING );

		if (info[0]) {
			Q_strncpyz( uiInfo.playerNames[uiInfo.playerCount], Info_ValueForKey( info, "n" ), sizeof( uiInfo.playerNames[0] ) );
			Q_CleanStr( uiInfo.playerNames[uiInfo.playerCount] );
			uiInfo.playerCount++;
			team2 = atoi(Info_ValueForKey(info, "t"));
			if (team2 == team) {
				Q_strncpyz( uiInfo.teamNames[uiInfo.myTeamCount], Info_ValueForKey( info, "n" ), sizeof( uiInfo.teamNames[0] ) );
				Q_CleanStr( uiInfo.teamNames[uiInfo.myTeamCount] );
				uiInfo.teamClientNums[uiInfo.myTeamCount] = n;
				if (uiInfo.playerNumber == n) {
					playerTeamNumber = uiInfo.myTeamCount;
				}
				uiInfo.myTeamCount++;
			}
		}
	}

	trap->Cvar_Set("cg_selectedPlayer", va("%d", (int)playerTeamNumber));

	n = (int)trap->Cvar_VariableValue("cg_selectedPlayer");
	if (n < 0 || n > uiInfo.myTeamCount) {
		n = 0;
	}
	if (n < uiInfo.myTeamCount) {
		trap->Cvar_Set("cg_selectedPlayerName", uiInfo.teamNames[n]);
	}
}


static void UI_DrawSelectedPlayer(rectDef_t *rect, float scale, vector4 *color, int textStyle) {
	if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) {
		uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
		UI_BuildPlayerList();
	}
	Text_Paint(rect->x, rect->y, scale, color, UI_Cvar_VariableString("cl_name"), 0, 0, textStyle);
}

static void UI_DrawServerRefreshDate(rectDef_t *rect, float scale, vector4 *color, int textStyle) {
	if (uiInfo.serverStatus.refreshActive) {
		vector4 lowLight, newColor;
		lowLight.r = 0.8f * color->r; 
		lowLight.g = 0.8f * color->g; 
		lowLight.b = 0.8f * color->b; 
		lowLight.a = 0.8f * color->a; 
		LerpColor( color, &lowLight, &newColor, 0.5f+0.5f*sinf( uiInfo.uiDC.realTime / PULSE_DIVISOR ) );
		Text_Paint(rect->x, rect->y, scale, &newColor, va("Getting info for %d servers (ESC to cancel)", trap->LAN_GetServerCount(UI_SourceForLAN())), 0, 0, textStyle);
	} else {
		char buff[64];
		Q_strncpyz(buff, UI_Cvar_VariableString(va("ui_lastServerRefresh_%i", ui_netSource->integer)), 64);
		Text_Paint(rect->x, rect->y, scale, color, va("Refresh Time: %s", buff), 0, 0, textStyle);
	}
}

static void UI_DrawServerMOTD(rectDef_t *rect, float scale, vector4 *color) {
	if (uiInfo.serverStatus.motdLen) {
		float maxX;

		if (uiInfo.serverStatus.motdWidth == -1) {
			uiInfo.serverStatus.motdWidth = 0;
			uiInfo.serverStatus.motdPaintX = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if (uiInfo.serverStatus.motdOffset > uiInfo.serverStatus.motdLen) {
			uiInfo.serverStatus.motdOffset = 0;
			uiInfo.serverStatus.motdPaintX = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if (uiInfo.uiDC.realTime > uiInfo.serverStatus.motdTime) {
			uiInfo.serverStatus.motdTime = uiInfo.uiDC.realTime + 10;
			if (uiInfo.serverStatus.motdPaintX <= rect->x + 2) {
				if (uiInfo.serverStatus.motdOffset < uiInfo.serverStatus.motdLen) {
					uiInfo.serverStatus.motdPaintX += Text_Width(&uiInfo.serverStatus.motd[uiInfo.serverStatus.motdOffset], scale, 1) - 1;
					uiInfo.serverStatus.motdOffset++;
				} else {
					uiInfo.serverStatus.motdOffset = 0;
					if (uiInfo.serverStatus.motdPaintX2 >= 0) {
						uiInfo.serverStatus.motdPaintX = uiInfo.serverStatus.motdPaintX2;
					} else {
						uiInfo.serverStatus.motdPaintX = rect->x + rect->w - 2;
					}
					uiInfo.serverStatus.motdPaintX2 = -1;
				}
			} else {
				//serverStatus.motdPaintX--;
				uiInfo.serverStatus.motdPaintX -= 2;
				if (uiInfo.serverStatus.motdPaintX2 >= 0) {
					//serverStatus.motdPaintX2--;
					uiInfo.serverStatus.motdPaintX2 -= 2;
				}
			}
		}

		maxX = rect->x + rect->w - 2;
		Text_Paint_Limit(&maxX, (float)uiInfo.serverStatus.motdPaintX, rect->y + rect->h - 3, scale, color, &uiInfo.serverStatus.motd[uiInfo.serverStatus.motdOffset], 0, 0); 
		if (uiInfo.serverStatus.motdPaintX2 >= 0) {
			float maxX2 = rect->x + rect->w - 2;
			Text_Paint_Limit(&maxX2, (float)uiInfo.serverStatus.motdPaintX2, rect->y + rect->h - 3, scale, color, uiInfo.serverStatus.motd, 0, uiInfo.serverStatus.motdOffset); 
		}
		if (uiInfo.serverStatus.motdOffset && maxX > 0) {
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if (uiInfo.serverStatus.motdPaintX2 == -1) {
				uiInfo.serverStatus.motdPaintX2 = rect->x + rect->w - 2;
			}
		} else {
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

	}
}

static void UI_DrawKeyBindStatus(rectDef_t *rect, float scale, vector4 *color, int textStyle) {
	//	int ofs = 0; TTimo: unused
	if (Display_KeyBindPending()) {
		Text_Paint(rect->x, rect->y, scale, color, "Waiting for new key... Press ESCAPE to cancel", 0, 0, textStyle);
	} else {
		Text_Paint(rect->x, rect->y, scale, color, "Press ENTER or CLICK to change, Press BACKSPACE to clear", 0, 0, textStyle);
	}
}

static void UI_DrawGLInfo(rectDef_t *rect, float scale, vector4 *color, int textStyle) {
	char * eptr;
	char buff[1024];
	const char *lines[64];
	int y, numLines, i;

	Text_Paint(rect->x + 2, rect->y, scale, color, va("VENDOR: %s", uiInfo.uiDC.glconfig.vendor_string), 0, 30, textStyle);
	Text_Paint(rect->x + 2, rect->y + 15, scale, color, va("VERSION: %s: %s", uiInfo.uiDC.glconfig.version_string,uiInfo.uiDC.glconfig.renderer_string), 0, 30, textStyle);
	Text_Paint(rect->x + 2, rect->y + 30, scale, color, va ("PIXELFORMAT: color(%d-bits) Z(%d-bits) stencil(%d-bits)", uiInfo.uiDC.glconfig.colorBits, uiInfo.uiDC.glconfig.depthBits, uiInfo.uiDC.glconfig.stencilBits), 0, 30, textStyle);

	// build null terminated extension strings
	// in TA this was not directly crashing, but displaying a nasty broken shader right in the middle
	// brought down the string size to 1024, there's not much that can be shown on the screen anyway
	Q_strncpyz(buff, uiInfo.uiDC.glconfig.extensions_string, 1024);
	eptr = buff;
	y = (int)rect->y + 45;
	numLines = 0;
	while ( y < rect->y + rect->h && *eptr )
	{
		while ( *eptr && *eptr == ' ' )
			*eptr++ = '\0';

		// track start of valid string
		if (*eptr && *eptr != ' ') {
			lines[numLines++] = eptr;
		}

		while ( *eptr && *eptr != ' ' )
			eptr++;
	}

	i = 0;
	while (i < numLines) {
		Text_Paint(rect->x + 2, (float)y, scale, color, lines[i++], 0, 20, textStyle);
		if (i < numLines) {
			Text_Paint(rect->x + rect->w / 2, (float)y, scale, color, lines[i++], 0, 20, textStyle);
		}
		y += 10;
		if (y > rect->y + rect->h - 11) {
			break;
		}
	}


}

// FIXME: table drive
//
static void UI_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vector4 *color, qhandle_t shader, int textStyle) {
	rectDef_t rect;

	rect.x = x + text_x;
	rect.y = y + text_y;
	rect.w = w;
	rect.h = h;

	switch (ownerDraw) {
	case UI_PLAYERMODEL:
		UI_DrawPlayerModel(&rect);
		break;
	case UI_GAMETYPE:
	case UI_JOINGAMETYPE:
		UI_DrawGameType(&rect, scale, color, textStyle);
		break;
	case UI_MAPPREVIEW:
		UI_DrawMapPreview(&rect, scale, color, qtrue);
		break;
	case UI_MAP_TIMETOBEAT:
		UI_DrawMapTimeToBeat(&rect, scale, color, textStyle);
		break;
	case UI_SKILL:
		UI_DrawSkill(&rect, scale, color, textStyle);
		break;
	case UI_BLUETEAMNAME:
		UI_DrawTeamName(&rect, scale, color, qtrue, textStyle);
		break;
	case UI_REDTEAMNAME:
		UI_DrawTeamName(&rect, scale, color, qfalse, textStyle);
		break;
	case UI_BLUETEAM1:
	case UI_BLUETEAM2:
	case UI_BLUETEAM3:
	case UI_BLUETEAM4:
	case UI_BLUETEAM5:
		UI_DrawTeamMember(&rect, scale, color, qtrue, ownerDraw - UI_BLUETEAM1 + 1, textStyle);
		break;
	case UI_REDTEAM1:
	case UI_REDTEAM2:
	case UI_REDTEAM3:
	case UI_REDTEAM4:
	case UI_REDTEAM5:
		UI_DrawTeamMember(&rect, scale, color, qfalse, ownerDraw - UI_REDTEAM1 + 1, textStyle);
		break;
	case UI_NETSOURCE:
		UI_DrawNetSource(&rect, scale, color, textStyle);
		break;
	case UI_NETMAPPREVIEW:
		UI_DrawNetMapPreview(&rect, scale, color);
		break;
	case UI_NETFILTER:
		UI_DrawNetFilter(&rect, scale, color, textStyle);
		break;
	case UI_ALLMAPS_SELECTION:
		UI_DrawAllMapsSelection(&rect, scale, color, textStyle, qtrue);
		break;
	case UI_MAPS_SELECTION:
		UI_DrawAllMapsSelection(&rect, scale, color, textStyle, qfalse);
		break;
	case UI_BOTNAME:
		UI_DrawBotName(&rect, scale, color, textStyle);
		break;
	case UI_BOTSKILL:
		UI_DrawBotSkill(&rect, scale, color, textStyle);
		break;
	case UI_REDBLUE:
		UI_DrawRedBlue(&rect, scale, color, textStyle);
		break;
	case UI_CROSSHAIR:
		UI_DrawCrosshair(&rect, scale, color);
		break;
	case UI_SELECTEDPLAYER:
		UI_DrawSelectedPlayer(&rect, scale, color, textStyle);
		break;
	case UI_SERVERREFRESHDATE:
		UI_DrawServerRefreshDate(&rect, scale, color, textStyle);
		break;
	case UI_SERVERMOTD:
		UI_DrawServerMOTD(&rect, scale, color);
		break;
	case UI_GLINFO:
		UI_DrawGLInfo(&rect,scale, color, textStyle);
		break;
	case UI_KEYBINDSTATUS:
		UI_DrawKeyBindStatus(&rect,scale, color, textStyle);
		break;
	default:
		break;
  }

}

static qboolean UI_OwnerDrawVisible(int flags) {
	qboolean vis = qtrue;

	while (flags) {

		if (flags & UI_SHOW_FFA) {
			if (trap->Cvar_VariableValue("sv_gametype") != GT_BLOODBATH) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FFA;
		}

		if (flags & UI_SHOW_NOTFFA) {
			if (trap->Cvar_VariableValue("sv_gametype") == GT_BLOODBATH) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFFA;
		}

		if (flags & UI_SHOW_FAVORITESERVERS) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if (ui_netSource->integer != UIAS_FAVORITES) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FAVORITESERVERS;
		} 
		if (flags & UI_SHOW_NOTFAVORITESERVERS) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if (ui_netSource->integer == UIAS_FAVORITES) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFAVORITESERVERS;
		} 
		if (flags & (UI_SHOW_ANYTEAMGAME)) {
			if (ui_gametype->integer < GT_TEAMBLOOD ) {
				vis = qfalse;
			}
			flags &= ~(UI_SHOW_ANYTEAMGAME);
		} 
		if (flags & (UI_SHOW_ANYNONTEAMGAME)) {
			if (ui_gametype->integer >= GT_TEAMBLOOD ) {
				vis = qfalse;
			}
			flags &= ~(UI_SHOW_ANYNONTEAMGAME);
		} 
		if (flags & UI_SHOW_NEWHIGHSCORE) {
			if (uiInfo.newHighScoreTime < uiInfo.uiDC.realTime) {
				vis = qfalse;
			} else {
				if (uiInfo.soundHighScore) {
					if (trap->Cvar_VariableValue("sv_killserver") == 0) {
						// wait on server to go down before playing sound
						trap->S_StartLocalSound(uiInfo.newHighScoreSound, CHAN_ANNOUNCER);
						uiInfo.soundHighScore = qfalse;
					}
				}
			}
			flags &= ~UI_SHOW_NEWHIGHSCORE;
		} 
		if (flags & UI_SHOW_NEWBESTTIME) {
			if (uiInfo.newBestTime < uiInfo.uiDC.realTime) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NEWBESTTIME;
		} 
		if (flags & UI_SHOW_DEMOAVAILABLE) {
			if (!uiInfo.demoAvailable) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_DEMOAVAILABLE;
		} else {
			flags = 0;
		}
	}
	return vis;
}

static size_t UI_MapCountByGameType( void ) {
	size_t i=0, c=0;

	for ( i=0; i<uiInfo.mapCount; i++ ) {
		uiInfo.mapList[i].active = qfalse;
		if ( uiInfo.mapList[i].typeBits & (1 << ui_gametype->integer) ) {
			c++;
			uiInfo.mapList[i].active = qtrue;
		}
	}

	return c;
}

static qboolean UI_GameType_HandleKey(int flags, float *special, int key, qboolean resetMap) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		size_t oldCount = UI_MapCountByGameType();

		// hard coded mess here
		if (key == K_MOUSE2) {
			ui_gametype->integer--;
			if (ui_gametype->integer < 0)
				ui_gametype->integer = GT_NUM_GAMETYPES-1;
		} else {
			ui_gametype->integer++;
			ui_gametype->integer %= GT_NUM_GAMETYPES;
		}

		trap->Cvar_Set("ui_gameType", va("%d", ui_gametype->integer));
		UI_SetCapFragLimits(qtrue);
		if (resetMap && oldCount != UI_MapCountByGameType()) {
			trap->Cvar_Set( "ui_currentMap", "0");
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, 0, NULL);
		}
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Skill_HandleKey(int flags, float *special, int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		int i = (int)trap->Cvar_VariableValue( "g_difficulty" );

		if (key == K_MOUSE2) {
			i--;
		} else {
			i++;
		}

		if (i < 1) {
			i = numSkillLevels;
		} else if (i > numSkillLevels) {
			i = 1;
		}

		trap->Cvar_Set("g_difficulty", va("%i", i));
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_TeamMember_HandleKey(int flags, float *special, int key, qboolean blue, int num) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		// 0 - None
		// 1 - Human
		// 2..NumCharacters - Bot
		const char *cvar = va(blue ? "ui_blueteam%i" : "ui_redteam%i", num);
		int value = (int)trap->Cvar_VariableValue(cvar);

		if (key == K_MOUSE2) {
			value--;
		} else {
			value++;
		}

		if (ui_gametype->integer >= GT_TEAMBLOOD) {
			if (value >= uiInfo.characterCount + 2) {
				value = 0;
			} else if (value < 0) {
				value = (int)uiInfo.characterCount + 2 - 1;
			}
		} else {
			if (value >= UI_GetNumBots() + 2) {
				value = 0;
			} else if (value < 0) {
				value = (int)UI_GetNumBots() + 2 - 1;
			}
		}

		trap->Cvar_Set(cvar, va("%i", value));
		return qtrue;
	}
	return qfalse;
}

static void UI_UpdatePendingPings( void ) { 
	trap->LAN_ResetPings( UI_SourceForLAN() );
	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;

}

static void UI_StartServerRefresh( qboolean full ) {
	char	*ptr;
	int		lanSource;

	qtime_t q;
	trap->RealTime( &q );
	trap->Cvar_Set( va( "ui_lastServerRefresh_%i", ui_netSource->integer ), va( "%s-%i, %i at %i:%i", MonthAbbrev[q.tm_mon], q.tm_mday, 1900+q.tm_year, q.tm_hour, q.tm_min ) );

	if ( !full ) {
		UI_UpdatePendingPings();
		return;
	}

	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 1000;
	// clear number of displayed servers
	uiInfo.serverStatus.numDisplayServers = 0;
	uiInfo.serverStatus.numPlayersOnServers = 0;

	lanSource = UI_SourceForLAN();
	// mark all servers as visible so we store ping updates for them
	trap->LAN_MarkServerVisible( lanSource, -1, qtrue );
	// reset all the pings
	trap->LAN_ResetPings( lanSource );
	//
	if( ui_netSource->integer == UIAS_LOCAL ) {
		trap->Cbuf_ExecuteText( EXEC_NOW, "localservers\n" );
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
		return;
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 5000;
	if( ui_netSource->integer >= UIAS_GLOBAL1 && ui_netSource->integer <= UIAS_GLOBAL5 ) {

		ptr = UI_Cvar_VariableString( "debug_protocol" );
		if ( strlen( ptr ) )
			trap->Cbuf_ExecuteText( EXEC_NOW, va( "globalservers %d %s full empty\n", ui_netSource->integer-1, ptr ) );
		else
			trap->Cbuf_ExecuteText( EXEC_NOW, va( "globalservers %d %d full empty\n", ui_netSource->integer-1, (int)trap->Cvar_VariableValue( "protocol" ) ) );
	}
}

static qboolean UI_NetSource_HandleKey(int flags, float *special, int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {

		if (key == K_MOUSE2) {
			ui_netSource->integer--;
		} else {
			ui_netSource->integer++;
		}

		if(ui_netSource->integer >= UIAS_GLOBAL1 && ui_netSource->integer <= UIAS_GLOBAL5)
		{
			char masterstr[2], cvarname[sizeof("sv_master1")];

			while(ui_netSource->integer >= UIAS_GLOBAL1 && ui_netSource->integer <= UIAS_GLOBAL5)
			{
				Com_sprintf(cvarname, sizeof(cvarname), "sv_master%d", ui_netSource->integer);
				trap->Cvar_VariableStringBuffer(cvarname, masterstr, sizeof(masterstr));
				if(*masterstr)
					break;

				if (key == K_MOUSE2) {
					ui_netSource->integer--;
				} else {
					ui_netSource->integer++;
				}
			}
		}

		if (ui_netSource->integer >= numNetSources) {
			ui_netSource->integer = 0;
		} else if (ui_netSource->integer < 0) {
			ui_netSource->integer = numNetSources - 1;
		}

		UI_BuildServerDisplayList(qtrue);
		if (!(ui_netSource->integer >= UIAS_GLOBAL1 && ui_netSource->integer <= UIAS_GLOBAL5)) {
			UI_StartServerRefresh(qtrue);
		}
		trap->Cvar_Set( "ui_netSource", va("%d", ui_netSource->integer));
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_NetFilter_HandleKey(int flags, float *special, int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {

		if (key == K_MOUSE2) {
			ui_serverFilterType->integer--;
		} else {
			ui_serverFilterType->integer++;
		}

		if (ui_serverFilterType->integer >= numServerFilters) {
			ui_serverFilterType->integer = 0;
		} else if (ui_serverFilterType->integer < 0) {
			ui_serverFilterType->integer = numServerFilters - 1;
		}
		UI_BuildServerDisplayList(qtrue);
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_BotName_HandleKey(int flags, float *special, int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		int game = (int)trap->Cvar_VariableValue("sv_gametype");
		size_t value = uiInfo.botIndex;

		if (key == K_MOUSE2) {
			value--;
		} else {
			value++;
		}

		if (game >= GT_TEAMBLOOD) {
			if (value >= uiInfo.characterCount + 2) {
				value = 0;
			} else if (value < 0) {
				value = uiInfo.characterCount + 2 - 1;
			}
		} else {
			if (value >= UI_GetNumBots() + 2) {
				value = 0;
			} else if (value < 0) {
				value = UI_GetNumBots() + 2 - 1;
			}
		}
		uiInfo.botIndex = value;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_BotSkill_HandleKey(int flags, float *special, int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		if (key == K_MOUSE2) {
			uiInfo.skillIndex--;
		} else {
			uiInfo.skillIndex++;
		}
		if (uiInfo.skillIndex >= numSkillLevels) {
			uiInfo.skillIndex = 0;
		} else if (uiInfo.skillIndex < 0) {
			uiInfo.skillIndex = numSkillLevels-1;
		}
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_RedBlue_HandleKey(int flags, float *special, int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		uiInfo.redBlue ^= 1;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Crosshair_HandleKey(int flags, float *special, int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		if (key == K_MOUSE2) {
			uiInfo.currentCrosshair--;
		} else {
			uiInfo.currentCrosshair++;
		}

		if (uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
			uiInfo.currentCrosshair = 0;
		} else if (uiInfo.currentCrosshair < 0) {
			uiInfo.currentCrosshair = NUM_CROSSHAIRS - 1;
		}
		trap->Cvar_Set("cg_drawCrosshair", va("%d", uiInfo.currentCrosshair)); 
		return qtrue;
	}
	return qfalse;
}



static qboolean UI_SelectedPlayer_HandleKey(int flags, float *special, int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		int selected;

		UI_BuildPlayerList();
		selected = (int)trap->Cvar_VariableValue("cg_selectedPlayer");

		if (key == K_MOUSE2) {
			selected--;
		} else {
			selected++;
		}

		if (selected > uiInfo.myTeamCount) {
			selected = 0;
		} else if (selected < 0) {
			selected = (int)uiInfo.myTeamCount;
		}

		if (selected == uiInfo.myTeamCount) {
			trap->Cvar_Set( "cg_selectedPlayerName", "Everyone");
		} else {
			trap->Cvar_Set( "cg_selectedPlayerName", uiInfo.teamNames[selected]);
		}
		trap->Cvar_Set( "cg_selectedPlayer", va("%d", selected));
	}
	return qfalse;
}


static qboolean UI_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
	switch (ownerDraw) {
	case UI_GAMETYPE:
	case UI_JOINGAMETYPE:
		return UI_GameType_HandleKey(flags, special, key, qtrue);
		break;
	case UI_SKILL:
		return UI_Skill_HandleKey(flags, special, key);
		break;
	case UI_BLUETEAMNAME:
	case UI_REDTEAMNAME:
	//	return UI_TeamName_HandleKey(flags, special, key, qfalse);
		break;
	case UI_BLUETEAM1:
	case UI_BLUETEAM2:
	case UI_BLUETEAM3:
	case UI_BLUETEAM4:
	case UI_BLUETEAM5:
		UI_TeamMember_HandleKey(flags, special, key, qtrue, ownerDraw - UI_BLUETEAM1 + 1);
		break;
	case UI_REDTEAM1:
	case UI_REDTEAM2:
	case UI_REDTEAM3:
	case UI_REDTEAM4:
	case UI_REDTEAM5:
		UI_TeamMember_HandleKey(flags, special, key, qfalse, ownerDraw - UI_REDTEAM1 + 1);
		break;
	case UI_NETSOURCE:
		UI_NetSource_HandleKey(flags, special, key);
		break;
	case UI_NETFILTER:
		UI_NetFilter_HandleKey(flags, special, key);
		break;
	case UI_BOTNAME:
		return UI_BotName_HandleKey(flags, special, key);
		break;
	case UI_BOTSKILL:
		return UI_BotSkill_HandleKey(flags, special, key);
		break;
	case UI_REDBLUE:
		UI_RedBlue_HandleKey(flags, special, key);
		break;
	case UI_CROSSHAIR:
		UI_Crosshair_HandleKey(flags, special, key);
		break;
	case UI_SELECTEDPLAYER:
		UI_SelectedPlayer_HandleKey(flags, special, key);
		break;
	default:
		break;
	}

	return qfalse;
}

static int QDECL UI_ServersQsortCompare( const void *arg1, const void *arg2 ) {
	return trap->LAN_CompareServers( UI_SourceForLAN(), uiInfo.serverStatus.sortKey, uiInfo.serverStatus.sortDir, *(int*)arg1, *(int*)arg2);
}

void UI_ServersSort(int column, qboolean force) {

	if ( !force ) {
		if ( uiInfo.serverStatus.sortKey == column ) {
			return;
		}
	}

	uiInfo.serverStatus.sortKey = column;
	qsort( &uiInfo.serverStatus.displayServers[0], uiInfo.serverStatus.numDisplayServers, sizeof(int), UI_ServersQsortCompare);
}

static void UI_LoadMods( void ) {
	size_t	numdirs;
	char	dirlist[2048];
	char	*dirptr;
	char	*descptr;
	int		i;
	size_t	dirlen;

	uiInfo.modCount = 0;
	numdirs = trap->FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr  = dirlist;
	for( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
		descptr = dirptr + dirlen;
		uiInfo.modList[uiInfo.modCount].modName = String_Alloc(dirptr);
		uiInfo.modList[uiInfo.modCount].modDescr = String_Alloc(descptr);
		dirptr += dirlen + strlen(descptr) + 1;
		uiInfo.modCount++;
		if (uiInfo.modCount >= MAX_MODS) {
			break;
		}
	}

}

#define NAMEBUFSIZE (MAX_DEMOS * 32)

static void UI_LoadDemosInDirectory( const char *directory ) {
	char			demolist[MAX_DEMOLIST] = {0}, *demoname = NULL;
	char			fileList[MAX_DEMOLIST] = {0}, *fileName = NULL;
	char			demoExt[32] = {0};
	int				i=0;
	size_t			len=0;
	size_t			numFiles=0;
	int				protocol = (int)trap->Cvar_VariableValue( "com_protocol" );

	if ( !protocol )
		protocol = (int)trap->Cvar_VariableValue( "protocol" );

	Com_sprintf( demoExt, sizeof( demoExt ), ".%s%d", DEMO_EXTENSION, protocol);

	uiInfo.demoCount += trap->FS_GetFileList( directory, demoExt, demolist, sizeof( demolist ) );

	demoname = demolist;

	if ( uiInfo.demoCount > MAX_DEMOS )
		uiInfo.demoCount = MAX_DEMOS;

	for( ; uiInfo.loadedDemos<uiInfo.demoCount; uiInfo.loadedDemos++)
	{
		len = strlen( demoname );
		Com_sprintf( uiInfo.demoList[uiInfo.loadedDemos], sizeof( uiInfo.demoList[0] ), "%s/%s", directory + strlen( DEMO_DIRECTORY )+1, demoname );
		demoname += len + 1;
	}

	numFiles = trap->FS_GetFileList( directory, "/", fileList, sizeof( fileList ) );

	fileName = fileList;
	for ( i=0; i<numFiles; i++ )
	{
		len = strlen( fileName );
		fileName[len] = '\0';
		if ( Q_stricmp( fileName, "." ) && Q_stricmp( fileName, ".." ) )
			UI_LoadDemosInDirectory( va( "%s/%s", directory, fileName ) );
		fileName += len+1;
	}

}

static void UI_LoadDemos( void ) {
	uiInfo.demoCount = 0;
	uiInfo.loadedDemos = 0;
	memset( uiInfo.demoList, 0, sizeof( uiInfo.demoList ) );
	UI_LoadDemosInDirectory( DEMO_DIRECTORY );
}

static qboolean UI_SetNextMap(size_t actual, size_t index) {
	size_t i;
	for (i = actual + 1; i < uiInfo.mapCount; i++) {
		if (uiInfo.mapList[i].active) {
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, index + 1, "skirmish");
			return qtrue;
		}
	}
	return qfalse;
}


static void UI_StartSkirmish(qboolean next) {
	int g, delay, temp;
	float skill;
	char buff[MAX_STRING_CHARS];

	if (next) {
		size_t actual;
		size_t index = (size_t)trap->Cvar_VariableValue("ui_mapIndex");
		UI_MapCountByGameType();
		UI_SelectedMap(index, &actual);
		if (UI_SetNextMap(actual, index)) {
		} else {
			UI_GameType_HandleKey(0, NULL, K_MOUSE1, qfalse);
			UI_MapCountByGameType();
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, 0, "skirmish");
		}
	}

	g = ui_gametype->integer;
	trap->Cvar_Set( "sv_gametype", va( "%d", g ) );
	trap->Cbuf_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", uiInfo.mapList[ui_currentMap->integer].mapLoadName) );
	skill = trap->Cvar_VariableValue( "g_difficulty" );
	trap->Cvar_Set("ui_scoreMap", uiInfo.mapList[ui_currentMap->integer].mapName);

	// set up sp overrides, will be replaced on postgame
	temp = (int)trap->Cvar_VariableValue( "capturelimit" );
	trap->Cvar_Set("ui_saveCaptureLimit", va("%i", temp));
	temp = (int)trap->Cvar_VariableValue( "fraglimit" );
	trap->Cvar_Set("ui_saveFragLimit", va("%i", temp));

	UI_SetCapFragLimits(qfalse);

	temp = (int)trap->Cvar_VariableValue( "cg_drawTimer" );
	trap->Cvar_Set("ui_drawTimer", va("%i", temp));
	temp = (int)trap->Cvar_VariableValue( "g_doWarmup" );
	trap->Cvar_Set("ui_doWarmup", va("%i", temp));
	temp = (int)trap->Cvar_VariableValue( "g_friendlyFire" );
	trap->Cvar_Set("ui_friendlyFire", va("%i", temp));
	temp = (int)trap->Cvar_VariableValue( "sv_maxClients" );
	trap->Cvar_Set("ui_maxClients", va("%i", temp));
	temp = (int)trap->Cvar_VariableValue( "g_warmup" );
	trap->Cvar_Set("ui_Warmup", va("%i", temp));
	temp = (int)trap->Cvar_VariableValue( "sv_pure" );
	trap->Cvar_Set("ui_pure", va("%i", temp));

	trap->Cvar_Set("cg_drawTimer", "1");
	trap->Cvar_Set("g_doWarmup", "1");
	trap->Cvar_Set("g_warmup", "15");
	trap->Cvar_Set("sv_pure", "0");
	trap->Cvar_Set("g_friendlyFire", "0");
	trap->Cvar_Set("g_redTeam", UI_Cvar_VariableString("ui_teamName"));
	trap->Cvar_Set("g_blueTeam", UI_Cvar_VariableString("ui_opponentName"));

	if ( (int)trap->Cvar_VariableValue("ui_recordSPDemo")) {
		Com_sprintf(buff, MAX_STRING_CHARS, "%s_%i", uiInfo.mapList[ui_currentMap->integer].mapLoadName, g);
		trap->Cvar_Set("ui_recordSPDemoName", buff);
	}

	delay = 500;

	if (g == GT_DUEL) {
		trap->Cvar_Set("sv_maxClients", "2");
		Com_sprintf( buff, sizeof(buff), "wait ; addbot %s %f "", %i \n", uiInfo.mapList[ui_currentMap->integer].opponentName, skill, delay);
		trap->Cbuf_ExecuteText( EXEC_APPEND, buff );
	}
	if (g >= GT_TEAMBLOOD ) {
		trap->Cbuf_ExecuteText( EXEC_APPEND, "wait 5; team Red\n" );
	}
}

static void UI_Update(const char *name) {
	int	val = (int)trap->Cvar_VariableValue(name);

	if (Q_stricmp(name, "ui_SetName") == 0) {
		trap->Cvar_Set( "cl_name", UI_Cvar_VariableString("ui_Name"));
	} else if (Q_stricmp(name, "ui_setRate") == 0) {
		float rate = trap->Cvar_VariableValue("cl_rate");
		if (rate >= 5000) {
			trap->Cvar_Set("cl_maxpackets", "30");
			trap->Cvar_Set("cl_packetdup", "1");
		} else if (rate >= 4000) {
			trap->Cvar_Set("cl_maxpackets", "15");
			trap->Cvar_Set("cl_packetdup", "2");		// favor less prediction errors when there's packet loss
		} else {
			trap->Cvar_Set("cl_maxpackets", "15");
			trap->Cvar_Set("cl_packetdup", "1");		// favor lower bandwidth
		}
	} else if (Q_stricmp(name, "ui_GetName") == 0) {
		trap->Cvar_Set( "ui_Name", UI_Cvar_VariableString("cl_name"));
	} else if (Q_stricmp(name, "r_colorbits") == 0) {
		switch (val) {
		case 0:
			trap->Cvar_SetValue( "r_depthbits", 0 );
			trap->Cvar_SetValue( "r_stencilbits", 0 );
			break;
		case 16:
			trap->Cvar_SetValue( "r_depthbits", 16 );
			trap->Cvar_SetValue( "r_stencilbits", 0 );
			break;
		case 32:
			trap->Cvar_SetValue( "r_depthbits", 24 );
			break;
		}
	} else if (Q_stricmp(name, "r_lodbias") == 0) {
		switch (val) {
		case 0:
			trap->Cvar_SetValue( "r_subdivisions", 4 );
			break;
		case 1:
			trap->Cvar_SetValue( "r_subdivisions", 12 );
			break;
		case 2:
			trap->Cvar_SetValue( "r_subdivisions", 20 );
			break;
		}
	} else if (Q_stricmp(name, "ui_mousePitch") == 0) {
		if (val == 0) {
			trap->Cvar_SetValue( "m_pitch", 0.022f );
		} else {
			trap->Cvar_SetValue( "m_pitch", -0.022f );
		}
	}
}

// Return true if the menu script should be deferred for later
static qboolean UI_DeferMenuScript ( const char **args ) {
	const char* name;

	// Whats the reason for being deferred?
	if (!String_Parse( args, &name)) 
	{
		return qfalse;
	}

	// Handle the custom cases
	if ( !Q_stricmp ( name, "VideoSetup" ) )
	{
		const char* warningMenuName;
		qboolean	deferred;

		// No warning menu specified
		if ( !String_Parse( args, &warningMenuName) )
		{
			return qfalse;
		}

		// Defer if the video options were modified
		deferred = trap->Cvar_VariableValue ( "ui_r_modified" ) ? qtrue : qfalse;

		if ( deferred )
		{
			// Open the warning menu
			Menus_OpenByName(warningMenuName);
		}

		return deferred;
	}
	else if ( !Q_stricmp ( name, "RulesBackout" ) )
	{
		qboolean deferred;

		deferred = trap->Cvar_VariableValue ( "ui_rules_backout" ) ? qtrue : qfalse ;

		trap->Cvar_Set ( "ui_rules_backout", "0" );

		return deferred;
	}

	return qfalse;
}

static int UI_GetIndexFromSelection(int actual) {
	int i=0, c=0;

	for ( i=0; i<uiInfo.mapCount; i++ ) {
		if ( uiInfo.mapList[i].active ) {
			if ( i == actual )
				return c;
			c++;
		}
	}

	return 0;
}

static void UI_RunMenuScript(const char **args) {
	const char *name, *name2;
	char buff[MAX_STRING_CHARS];

	if (String_Parse(args, &name)) {
		if (Q_stricmp(name, "StartServer") == 0) {
			int i, clients, oldclients;
			float skill;
			trap->Cvar_Set( "dedicated", va( "%d", Q_clampi( 0, ui_dedicated->integer, 2 ) ) );
			trap->Cvar_Set( "sv_gametype", va( "%d", Q_clampi( 0, ui_gametype->integer, GT_NUM_GAMETYPES-1 ) ) );
			trap->Cvar_Set( "g_redTeam", UI_Cvar_VariableString( "ui_teamName" ) );
			trap->Cvar_Set( "g_blueTeam", UI_Cvar_VariableString( "ui_opponentName" ) );
			trap->Cbuf_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", uiInfo.mapList[ui_currentNetMap->integer].mapLoadName ) );
			skill = trap->Cvar_VariableValue( "g_difficulty" );
			// set max clients based on spots
			oldclients = (int)trap->Cvar_VariableValue( "sv_maxClients" );
			clients = 0;
			for (i = 0; i < PLAYERS_PER_TEAM; i++) {
				int bot = (int)trap->Cvar_VariableValue( va("ui_blueteam%i", i+1));
				if (bot >= 0) {
					clients++;
				}
				bot = (int)trap->Cvar_VariableValue( va("ui_redteam%i", i+1));
				if (bot >= 0) {

					clients++;
				}
			}
			if (clients == 0) {
				clients = 8;
			}

			if (oldclients > clients) {
				clients = oldclients;
			}

			trap->Cvar_Set( "sv_maxClients", va( "%d", clients ) );

			for (i = 0; i < PLAYERS_PER_TEAM; i++) {
				int bot = (int)trap->Cvar_VariableValue( va( "ui_blueteam%i", i+1) );
				if (bot > 1) {
					if (ui_gametype->integer >= GT_TEAMBLOOD) {
						Com_sprintf( buff, sizeof( buff ), "addbot %s %f %s\n", uiInfo.characterList[bot-2].name, skill, "Blue" );
					} else {
						Com_sprintf( buff, sizeof( buff ), "addbot %s %f \n", UI_GetBotNameByNumber(bot-2), skill );
					}
					trap->Cbuf_ExecuteText( EXEC_APPEND, buff );
				}
				bot = (int)trap->Cvar_VariableValue( va("ui_redteam%i", i+1));
				if (bot > 1) {
					if (ui_gametype->integer >= GT_TEAMBLOOD) {
						Com_sprintf( buff, sizeof(buff), "addbot %s %f %s\n", uiInfo.characterList[bot-2].name, skill, "Red");
					} else {
						Com_sprintf( buff, sizeof(buff), "addbot %s %f \n", UI_GetBotNameByNumber(bot-2), skill);
					}
					trap->Cbuf_ExecuteText( EXEC_APPEND, buff );
				}
			}
		} else if (Q_stricmp(name, "updateSPMenu") == 0) {
			UI_SetCapFragLimits(qtrue);
			UI_MapCountByGameType();
			ui_mapIndex->integer = UI_GetIndexFromSelection(ui_currentMap->integer);
			trap->Cvar_Set("ui_mapIndex", va("%d", ui_mapIndex->integer));
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, ui_mapIndex->integer, "skirmish");
			UI_GameType_HandleKey(0, NULL, K_MOUSE1, qfalse);
			UI_GameType_HandleKey(0, NULL, K_MOUSE2, qfalse);
		} else if (Q_stricmp(name, "resetDefaults") == 0) {
			trap->Cbuf_ExecuteText( EXEC_APPEND, "exec default.cfg\n");
			trap->Cbuf_ExecuteText( EXEC_APPEND, "cvar_restart\n");
			Controls_SetDefaults();
			trap->Cbuf_ExecuteText( EXEC_APPEND, "vid_restart\n" );
		} else if (Q_stricmp(name, "loadArenas") == 0) {
			UI_LoadArenas();
			UI_MapCountByGameType();
			Menu_SetFeederSelection(NULL, FEEDER_ALLMAPS, 0, "createserver");
		} else if (Q_stricmp(name, "saveControls") == 0) {
			Controls_SetConfig(qtrue);
		} else if (Q_stricmp(name, "loadControls") == 0) {
			Controls_GetConfig();
		} else if (Q_stricmp(name, "clearError") == 0) {
			trap->Cvar_Set("com_errorMessage", "");
		} else if (Q_stricmp(name, "RefreshServers") == 0) {
			UI_StartServerRefresh(qtrue);
			UI_BuildServerDisplayList(qtrue);
		} else if (Q_stricmp(name, "RefreshFilter") == 0) {
			UI_StartServerRefresh(qfalse);
			UI_BuildServerDisplayList(qtrue);
		} else if (Q_stricmp(name, "LoadDemos") == 0) {
			UI_LoadDemos();
		} else if (Q_stricmp(name, "LoadMods") == 0) {
			UI_LoadMods();
		} else if (Q_stricmp(name, "RunMod") == 0) {
			trap->Cvar_Set( "fs_game", uiInfo.modList[uiInfo.modIndex].modName);
			trap->Cbuf_ExecuteText( EXEC_APPEND, "vid_restart;" );
		} else if (Q_stricmp(name, "RunDemo") == 0) {
			trap->Cbuf_ExecuteText( EXEC_APPEND, va("demo %s\n", uiInfo.demoList[uiInfo.demoIndex]));
		} else if (Q_stricmp(name, "Quake3") == 0) {
			trap->Cvar_Set( "fs_game", "");
			trap->Cbuf_ExecuteText( EXEC_APPEND, "vid_restart;" );
		} else if (Q_stricmp(name, "closeJoin") == 0) {
			if (uiInfo.serverStatus.refreshActive) {
				UI_StopServerRefresh();
				uiInfo.serverStatus.nextDisplayRefresh = 0;
				uiInfo.nextServerStatusRefresh = 0;
				uiInfo.nextFindPlayerRefresh = 0;
				UI_BuildServerDisplayList(qtrue);
			} else {
				Menus_CloseByName("joinserver");
				Menus_OpenByName("main");
			}
		} else if (Q_stricmp(name, "StopRefresh") == 0) {
			UI_StopServerRefresh();
			uiInfo.serverStatus.nextDisplayRefresh = 0;
			uiInfo.nextServerStatusRefresh = 0;
			uiInfo.nextFindPlayerRefresh = 0;
		} else if (Q_stricmp(name, "UpdateFilter") == 0) {
			if (ui_netSource->integer == UIAS_LOCAL) {
				UI_StartServerRefresh(qtrue);
			}
			UI_BuildServerDisplayList(qtrue);
			UI_FeederSelection(FEEDER_SERVERS, 0);
		} else if (Q_stricmp(name, "ServerStatus") == 0) {
			trap->LAN_GetServerAddressString(UI_SourceForLAN(), uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], uiInfo.serverStatusAddress, sizeof(uiInfo.serverStatusAddress));
			UI_BuildServerStatus(qtrue);
		} else if (Q_stricmp(name, "FoundPlayerServerStatus") == 0) {
			Q_strncpyz(uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer], sizeof(uiInfo.serverStatusAddress));
			UI_BuildServerStatus(qtrue);
			Menu_SetFeederSelection(NULL, FEEDER_FINDPLAYER, 0, NULL);
		} else if (Q_stricmp(name, "FindPlayer") == 0) {
			UI_BuildFindPlayerList(qtrue);
			// clear the displayed server status info
			uiInfo.serverStatusInfo.numLines = 0;
			Menu_SetFeederSelection(NULL, FEEDER_FINDPLAYER, 0, NULL);
		} else if (Q_stricmp(name, "JoinServer") == 0) {
			if (uiInfo.serverStatus.currentServer >= 0 && uiInfo.serverStatus.currentServer < uiInfo.serverStatus.numDisplayServers) {
				trap->LAN_GetServerAddressString(UI_SourceForLAN(), uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, 1024);
				trap->Cbuf_ExecuteText( EXEC_APPEND, va( "connect %s\n", buff ) );
			}
		} else if (Q_stricmp(name, "FoundPlayerJoinServer") == 0) {
			if (uiInfo.currentFoundPlayerServer >= 0 && uiInfo.currentFoundPlayerServer < uiInfo.numFoundPlayerServers) {
				trap->Cbuf_ExecuteText( EXEC_APPEND, va( "connect %s\n", uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer] ) );
			}
		} else if (Q_stricmp(name, "Quit") == 0) {
			trap->Cbuf_ExecuteText( EXEC_NOW, "quit");
		} else if (Q_stricmp(name, "Controls") == 0) {
			trap->Cvar_Set( "cl_paused", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("setup_menu2");
		} else if (Q_stricmp(name, "Leave") == 0) {
			trap->Cbuf_ExecuteText( EXEC_APPEND, "disconnect\n" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("main");
		} else if (Q_stricmp(name, "ServerSort") == 0) {
			int sortColumn;
			if (Int_Parse(args, &sortColumn)) {
				// if same column we're already sorting on then flip the direction
				if (sortColumn == uiInfo.serverStatus.sortKey) {
					uiInfo.serverStatus.sortDir = !uiInfo.serverStatus.sortDir;
				}
				// make sure we sort again
				UI_ServersSort(sortColumn, qtrue);
			}
		} else if (Q_stricmp(name, "nextSkirmish") == 0) {
			UI_StartSkirmish(qtrue);
		} else if (Q_stricmp(name, "SkirmishStart") == 0) {
			UI_StartSkirmish(qfalse);
		} else if (Q_stricmp(name, "closeingame") == 0) {
			trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
			trap->Key_ClearStates();
			trap->Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();
		} else if (Q_stricmp(name, "voteMap") == 0) {
			if (ui_currentNetMap->integer >=0 && ui_currentNetMap->integer < uiInfo.mapCount) {
				trap->Cbuf_ExecuteText( EXEC_APPEND, va("callvote map %s\n",uiInfo.mapList[ui_currentNetMap->integer].mapLoadName) );
			}
		} else if (Q_stricmp(name, "voteKick") == 0) {
			if (uiInfo.playerIndex < uiInfo.playerCount) {
				trap->Cbuf_ExecuteText( EXEC_APPEND, va("callvote kick %s\n", uiInfo.playerNames[uiInfo.playerIndex]) );
			}
		} else if (Q_stricmp(name, "voteGame") == 0) {
			if (ui_gametype->integer >= 0 && ui_gametype->integer < GT_NUM_GAMETYPES) {
				trap->Cbuf_ExecuteText( EXEC_APPEND, va("callvote gametype %i\n", ui_gametype->integer) );
			}
		} else if (Q_stricmp(name, "voteLeader") == 0) {
			if (uiInfo.teamIndex >= 0 && uiInfo.teamIndex < uiInfo.myTeamCount) {
				trap->Cbuf_ExecuteText( EXEC_APPEND, va("callteamvote leader %s\n",uiInfo.teamNames[uiInfo.teamIndex]) );
			}
		} else if (Q_stricmp(name, "addBot") == 0) {
			if (trap->Cvar_VariableValue("sv_gametype") >= GT_TEAMBLOOD) {
				trap->Cbuf_ExecuteText( EXEC_APPEND, va("addbot %s %i %s\n", uiInfo.characterList[uiInfo.botIndex].name, (int)(uiInfo.skillIndex+1), (uiInfo.redBlue == 0) ? "Red" : "Blue") );
			} else {
				trap->Cbuf_ExecuteText( EXEC_APPEND, va("addbot %s %i %s\n", UI_GetBotNameByNumber(uiInfo.botIndex), (int)(uiInfo.skillIndex+1), (uiInfo.redBlue == 0) ? "Red" : "Blue") );
			}
		} else if (Q_stricmp(name, "addFavorite") == 0) {
			if (ui_netSource->integer != UIAS_FAVORITES) {
				char name[MAX_HOSTNAMELENGTH];
				char addr[MAX_ADDRESSLENGTH];
				int res;

				trap->LAN_GetServerInfo(UI_SourceForLAN(), uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, MAX_STRING_CHARS);
				name[0] = addr[0] = '\0';
				Q_strncpyz(name, 	Info_ValueForKey(buff, "hostname"), sizeof( name ));
				Q_strncpyz(addr, 	Info_ValueForKey(buff, "addr"), sizeof( addr ));
				if (strlen(name) > 0 && strlen(addr) > 0) {
					res = trap->LAN_AddServer(AS_FAVORITES, name, addr);
					if (res == 0) {
						// server already in the list
						Com_Printf("Favorite already in list\n");
					}
					else if (res == -1) {
						// list full
						Com_Printf("Favorite list full\n");
					}
					else {
						// successfully added
						Com_Printf("Added favorite server %s\n", addr);
					}
				}
			}
		} else if (Q_stricmp(name, "deleteFavorite") == 0) {
			if (ui_netSource->integer == UIAS_FAVORITES) {
				char addr[MAX_ADDRESSLENGTH];
				trap->LAN_GetServerInfo(AS_FAVORITES, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, MAX_STRING_CHARS);
				addr[0] = '\0';
				Q_strncpyz(addr, 	Info_ValueForKey(buff, "addr"), sizeof( addr ));
				if (strlen(addr) > 0) {
					trap->LAN_RemoveServer(AS_FAVORITES, addr);
				}
			}
		} else if (Q_stricmp(name, "createFavorite") == 0) {
			char name[MAX_HOSTNAMELENGTH];
			char addr[MAX_ADDRESSLENGTH];
			int res;

			name[0] = addr[0] = '\0';
			Q_strncpyz(name, 	UI_Cvar_VariableString("ui_favoriteName"), sizeof( name ));
			Q_strncpyz(addr, 	UI_Cvar_VariableString("ui_favoriteAddress"), sizeof( addr ));
			if (strlen(name) > 0 && strlen(addr) > 0) {
				res = trap->LAN_AddServer(AS_FAVORITES, name, addr);
				if (res == 0) {
					// server already in the list
					Com_Printf("Favorite already in list\n");
				}
				else if (res == -1) {
					// list full
					Com_Printf("Favorite list full\n");
				}
				else {
					// successfully added
					Com_Printf("Added favorite server %s\n", addr);
				}
			}
		} else if (Q_stricmp(name, "update") == 0) {
			if (String_Parse(args, &name2)) {
				UI_Update(name2);
			}
		}
		else {
			Com_Printf("unknown UI script %s\n", name);
		}
	}
}

qboolean UI_hasSkinForBase(const char *base, const char *team) {
	char	test[MAX_QPATH];

	Com_sprintf( test, sizeof( test ), "models/players/%s/%s/lower_default.skin", base, team );

	if (trap->FS_Open(test, NULL, FS_READ)) {
		return qtrue;
	}
	Com_sprintf( test, sizeof( test ), "models/players/characters/%s/%s/lower_default.skin", base, team );

	if (trap->FS_Open(test, NULL, FS_READ)) {
		return qtrue;
	}
	return qfalse;
}

static size_t UI_HeadCountByTeam( void ) {
	static int init = 0;
	size_t i, c=0;

	if ( !init ) {
		for ( i=0; i<uiInfo.characterCount; i++ )
			uiInfo.characterList[i].reference = 0;
		init = 1;
	}

	// do names
	for ( i=0; i<uiInfo.characterCount; i++ )
		uiInfo.characterList[i].active = qfalse;

	return c;
}

static size_t UI_FeederCount(float feederID) {
	if (feederID == FEEDER_HEADS) {
		return UI_HeadCountByTeam();
	} else if (feederID == FEEDER_Q3HEADS) {
		return uiInfo.q3HeadCount;
	} else if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS) {
		return UI_MapCountByGameType();
	} else if (feederID == FEEDER_SERVERS) {
		return uiInfo.serverStatus.numDisplayServers;
	} else if (feederID == FEEDER_SERVERSTATUS) {
		return uiInfo.serverStatusInfo.numLines;
	} else if (feederID == FEEDER_FINDPLAYER) {
		return uiInfo.numFoundPlayerServers;
	} else if (feederID == FEEDER_PLAYER_LIST) {
		if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) {
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}
		return uiInfo.playerCount;
	} else if (feederID == FEEDER_TEAM_LIST) {
		if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) {
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}
		return uiInfo.myTeamCount;
	} else if (feederID == FEEDER_MODS) {
		return uiInfo.modCount;
	} else if (feederID == FEEDER_DEMOS) {
		return uiInfo.demoCount;
	}
	return 0u;
}

static const char *UI_FeederItemText(float feederID, size_t index, size_t column, qhandle_t *handle) {
	static char info[MAX_STRING_CHARS];
	static char hostname[1024];
	static char clientBuff[32];
	static size_t lastColumn = 0;
	static int lastTime = 0;
	*handle = -1;
	if (feederID == FEEDER_HEADS) {
		size_t actual;
		return UI_SelectedHead(index, &actual);
	} else if (feederID == FEEDER_Q3HEADS) {
		if (index < uiInfo.q3HeadCount) {
			return uiInfo.q3HeadNames[index];
		}
	} else if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS) {
		size_t actual;
		return UI_SelectedMap(index, &actual);
	} else if (feederID == FEEDER_SERVERS) {
		if (index < uiInfo.serverStatus.numDisplayServers) {
			int ping, game;
			if (lastColumn != column || lastTime > uiInfo.uiDC.realTime + 5000) {
				trap->LAN_GetServerInfo(UI_SourceForLAN(), uiInfo.serverStatus.displayServers[index], info, MAX_STRING_CHARS);
				lastColumn = column;
				lastTime = uiInfo.uiDC.realTime;
			}
			ping = atoi(Info_ValueForKey(info, "ping"));
			if (ping == -1) {
				// if we ever see a ping that is out of date, do a server refresh
				// UI_UpdatePendingPings();
			}
			switch (column) {
			case SORT_HOST : 
				if (ping <= 0) {
					return Info_ValueForKey(info, "addr");
				} else {
					if ( ui_netSource->integer == UIAS_LOCAL ) {
						int nettype = atoi(Info_ValueForKey(info, "nettype"));

						if (nettype < 0 || nettype >= ARRAY_LEN(netnames)) {
							nettype = 0;
						}

						Com_sprintf( hostname, sizeof(hostname), "%s [%s]",
							Info_ValueForKey(info, "hostname"),
							netnames[nettype] );
						return hostname;
					}
					else {
						Com_sprintf( hostname, sizeof(hostname), "%s", Info_ValueForKey(info, "hostname"));
						return hostname;
					}
				}
			case SORT_MAP : return Info_ValueForKey(info, "mapname");
			case SORT_CLIENTS : 
				Com_sprintf( clientBuff, sizeof(clientBuff), "%s (%s)", Info_ValueForKey(info, "clients"), Info_ValueForKey(info, "sv_maxclients"));
				return clientBuff;
			case SORT_GAME : 
				game = atoi(Info_ValueForKey(info, "gametype"));
				if (game >= 0 && game < GT_NUM_GAMETYPES) {
					return GametypeStringForID( game );
				} else {
					return "Unknown";
				}
			case SORT_PING : 
				if (ping <= 0) {
					return "...";
				} else {
					return Info_ValueForKey(info, "ping");
				}
			}
		}
	} else if (feederID == FEEDER_SERVERSTATUS) {
		if ( index < uiInfo.serverStatusInfo.numLines ) {
			if ( column < 4 ) {
				return uiInfo.serverStatusInfo.lines[index][column];
			}
		}
	} else if (feederID == FEEDER_FINDPLAYER) {
		if ( index < uiInfo.numFoundPlayerServers ) {
			//return uiInfo.foundPlayerServerAddresses[index];
			return uiInfo.foundPlayerServerNames[index];
		}
	} else if (feederID == FEEDER_PLAYER_LIST) {
		if (index < uiInfo.playerCount) {
			return uiInfo.playerNames[index];
		}
	} else if (feederID == FEEDER_TEAM_LIST) {
		if (index < uiInfo.myTeamCount) {
			return uiInfo.teamNames[index];
		}
	} else if (feederID == FEEDER_MODS) {
		if (index < uiInfo.modCount) {
			if (uiInfo.modList[index].modDescr && *uiInfo.modList[index].modDescr) {
				return uiInfo.modList[index].modDescr;
			} else {
				return uiInfo.modList[index].modName;
			}
		}
	} else if (feederID == FEEDER_DEMOS) {
		if (index < uiInfo.demoCount) {
			return uiInfo.demoList[index];
		}
	}
	return "";
}


static qhandle_t UI_FeederItemImage(float feederID, size_t index) {
	if (feederID == FEEDER_HEADS) {
		size_t actual;
		UI_SelectedHead(index, &actual);
		index = actual;
		if (index >= 0 && index < uiInfo.characterCount) {
			if (uiInfo.characterList[index].headImage == -1) {
				uiInfo.characterList[index].headImage = trap->R_RegisterShader(uiInfo.characterList[index].imageName);
			}
			return uiInfo.characterList[index].headImage;
		}
	} else if (feederID == FEEDER_Q3HEADS) {
		if (index >= 0 && index < uiInfo.q3HeadCount) {
			return uiInfo.q3HeadIcons[index];
		}
	} else if (feederID == FEEDER_ALLMAPS || feederID == FEEDER_MAPS) {
		size_t actual;
		UI_SelectedMap(index, &actual);
		index = actual;
		if (index >= 0 && index < uiInfo.mapCount) {
			if (uiInfo.mapList[index].levelShot == -1) {
				uiInfo.mapList[index].levelShot = trap->R_RegisterShader(uiInfo.mapList[index].imageName);
			}
			return uiInfo.mapList[index].levelShot;
		}
	}
	return 0;
}

static void UI_Pause(qboolean b) {
	if (b) {
		// pause the game and set the ui keycatcher
		trap->Cvar_Set( "cl_paused", "1" );
		trap->Key_SetCatcher( KEYCATCH_UI );
	} else {
		// unpause the game and clear the ui keycatcher
		trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
		trap->Key_ClearStates();
		trap->Cvar_Set( "cl_paused", "0" );
	}
}

static void UI_BuildQ3Model_List( void ) {
	size_t			numdirs, numfiles;
	char			dirlist[2048], filelist[2048];
	char			skinname[MAX_QPATH];
	char			scratch[256];
	char			*dirptr, *fileptr;
	int				i, j, k, dirty;
	size_t			dirlen, filelen;

	uiInfo.q3HeadCount = 0;

	// iterate directory of all player models
	numdirs = trap->FS_GetFileList("models/players", "/", dirlist, 2048 );
	dirptr  = dirlist;
	for (i=0; i<numdirs && uiInfo.q3HeadCount < MAX_PLAYERMODELS; i++,dirptr+=dirlen+1)
	{
		dirlen = strlen(dirptr);

		if (dirlen && dirptr[dirlen-1]=='/') dirptr[dirlen-1]='\0';

		if (!strcmp(dirptr,".") || !strcmp(dirptr,".."))
			continue;

		// iterate all skin files in directory
		numfiles = trap->FS_GetFileList( va("models/players/%s",dirptr), "tga", filelist, 2048 );
		fileptr  = filelist;
		for (j=0; j<numfiles && uiInfo.q3HeadCount < MAX_PLAYERMODELS;j++,fileptr+=filelen+1)
		{
			filelen = strlen(fileptr);

			COM_StripExtension(fileptr, skinname, sizeof(skinname));

			// look for icon_????
			if (Q_stricmpn(skinname, "icon_", 5) == 0 && !(Q_stricmp(skinname,"icon_blue") == 0 || Q_stricmp(skinname,"icon_red") == 0))
			{
				if (Q_stricmp(skinname, "icon_default") == 0) {
					Com_sprintf( scratch, sizeof(scratch), "%s", dirptr);
				} else {
					Com_sprintf( scratch, sizeof(scratch), "%s/%s",dirptr, skinname + 5);
				}
				dirty = 0;
				for(k=0;k<uiInfo.q3HeadCount;k++) {
					if (!Q_stricmp(scratch, uiInfo.q3HeadNames[uiInfo.q3HeadCount])) {
						dirty = 1;
						break;
					}
				}
				if (!dirty) {
					Com_sprintf( uiInfo.q3HeadNames[uiInfo.q3HeadCount], sizeof(uiInfo.q3HeadNames[uiInfo.q3HeadCount]), "%s", scratch);
					uiInfo.q3HeadIcons[uiInfo.q3HeadCount++] = trap->R_RegisterShader(va("models/players/%s/%s",dirptr,skinname));
				}
			}

		}
	}	

}

static float UI_GetValue( int ownerdraw ) { return 0.0f; }

#define XCVAR_DECL
	#include "ui_xcvar.h"
#undef XCVAR_DECL

typedef struct cvarTable_s {
	cvar_t			**cvar;
	const char		*name;
	const char		*defaultString;
	void			(*update)( void );
	const int		flags;
	const char		*description;
} cvarTable_t;

static cvarTable_t cvarTable[] = {
	#define XCVAR_LIST
		#include "ui_xcvar.h"
	#undef XCVAR_LIST
};

static const int cvarTableSize = ARRAY_LEN( cvarTable );

static void UI_RegisterCvars( void ) {
	int i = 0;
	cvarTable_t *cv = NULL;

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ )
		*cv->cvar = trap->Cvar_Get( cv->name, cv->defaultString, cv->flags, cv->description, cv->update );

	// now update them
	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		if ( (*cv->cvar)->update )
			(*cv->cvar)->update();
	}
}

static void UI_GetTeamColor( vector4 *color ) {}

void UI_Init( qboolean inGameLoad ) {
	const char *menuSet;

	uiInfo.inGameLoad = inGameLoad;

	UI_RegisterCvars();
	UI_InitMemory();

	// cache redundant calulations
	trap->GetGLConfig( &uiInfo.uiDC.glconfig );

	// for 640x480 virtualized screen
	uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * (1.0f/SCREEN_HEIGHT);
	uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * (1.0f/SCREEN_WIDTH);
	if ( uiInfo.uiDC.glconfig.vidWidth * SCREEN_HEIGHT > uiInfo.uiDC.glconfig.vidHeight * SCREEN_WIDTH ) {
		// wide screen
		uiInfo.uiDC.bias = 0.5f * ( uiInfo.uiDC.glconfig.vidWidth - ( uiInfo.uiDC.glconfig.vidHeight * (SCREEN_WIDTH/SCREEN_HEIGHT) ) );
	}
	else {
		// no wide screen
		uiInfo.uiDC.bias = 0;
	}


	//UI_Load();
	uiInfo.uiDC.registerShaderNoMip = trap->R_RegisterShader;
	uiInfo.uiDC.setColor = &UI_SetColor;
	uiInfo.uiDC.drawHandlePic = &UI_DrawHandlePic;
	uiInfo.uiDC.drawStretchPic = trap->R_DrawStretchPic;
	uiInfo.uiDC.drawText = &Text_Paint;
	uiInfo.uiDC.textWidth = &Text_Width;
	uiInfo.uiDC.textHeight = &Text_Height;
	uiInfo.uiDC.registerModel = trap->R_RegisterModel;
	uiInfo.uiDC.modelBounds = trap->R_ModelBounds;
	uiInfo.uiDC.fillRect = &UI_FillRect;
	uiInfo.uiDC.drawRect = &UI_DrawRect2;
	uiInfo.uiDC.drawSides = &UI_DrawSides2;
	uiInfo.uiDC.drawTopBottom = &UI_DrawTopBottom2;
	uiInfo.uiDC.clearScene = trap->R_ClearScene;
	uiInfo.uiDC.addRefEntityToScene = trap->R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene = trap->R_RenderScene;
	uiInfo.uiDC.registerFont = trap->R_RegisterFont;
	uiInfo.uiDC.ownerDrawItem = &UI_OwnerDraw;
	uiInfo.uiDC.getValue = &UI_GetValue;
	uiInfo.uiDC.ownerDrawVisible = &UI_OwnerDrawVisible;
	uiInfo.uiDC.runScript = &UI_RunMenuScript;
	uiInfo.uiDC.deferScript = &UI_DeferMenuScript;
	uiInfo.uiDC.getTeamColor = &UI_GetTeamColor;
	uiInfo.uiDC.setCVar = trap->Cvar_Set;
	uiInfo.uiDC.getCVarString = trap->Cvar_VariableStringBuffer;
	uiInfo.uiDC.getCVarValue = trap->Cvar_VariableValue;
	uiInfo.uiDC.drawTextWithCursor = &Text_PaintWithCursor;
	uiInfo.uiDC.setOverstrikeMode = trap->Key_SetOverstrikeMode;
	uiInfo.uiDC.getOverstrikeMode = trap->Key_GetOverstrikeMode;
	uiInfo.uiDC.startLocalSound = trap->S_StartLocalSound;
	uiInfo.uiDC.ownerDrawHandleKey = &UI_OwnerDrawHandleKey;
	uiInfo.uiDC.feederCount = &UI_FeederCount;
	uiInfo.uiDC.feederItemImage = &UI_FeederItemImage;
	uiInfo.uiDC.feederItemText = &UI_FeederItemText;
	uiInfo.uiDC.feederSelection = &UI_FeederSelection;
	uiInfo.uiDC.setBinding = trap->Key_SetBinding;
	uiInfo.uiDC.getBindingBuf = trap->Key_GetBindingBuf;
	uiInfo.uiDC.keynumToStringBuf = trap->Key_KeynumToStringBuf;
	uiInfo.uiDC.executeText = trap->Cbuf_ExecuteText;
	uiInfo.uiDC.Error = Com_Error; 
	uiInfo.uiDC.Print = Com_Printf; 
	uiInfo.uiDC.Pause = &UI_Pause;
	uiInfo.uiDC.ownerDrawWidth = &UI_OwnerDrawWidth;
	uiInfo.uiDC.registerSound = trap->S_RegisterSound;
	uiInfo.uiDC.startBackgroundTrack = trap->S_StartBackgroundTrack;
	uiInfo.uiDC.stopBackgroundTrack = trap->S_StopBackgroundTrack;

	Init_Display(&uiInfo.uiDC);

	String_Init();

	uiInfo.uiDC.cursor	= trap->R_RegisterShader( "cursor" );
	uiInfo.uiDC.whiteShader = trap->R_RegisterShader( "white" );

	AssetCache();

	//QtZ: Add new defines/macros for the menu system
	//RAZTODO: normalise screen coords :/
	trap->PC_AddGlobalDefine( va( "SCREEN_WIDTH %.2f", SCREEN_WIDTH ) );
	trap->PC_AddGlobalDefine( va( "SCREEN_HEIGHT %.2f", SCREEN_HEIGHT ) );
	//~QtZ

	uiInfo.characterCount = 0;

	menuSet = UI_Cvar_VariableString("ui_menuFiles");
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/menus.txt";
	}

#if 0
	if (uiInfo.inGameLoad) {
		UI_LoadMenus("ui/ingame.txt", qtrue);
	} else {
	}
#else 
	UI_LoadMenus(menuSet, qtrue);
	UI_LoadMenus("ui/ingame.txt", qfalse);
#endif

	Menus_CloseAll();

	trap->LAN_LoadCachedServers();

	UI_BuildQ3Model_List();
	UI_LoadBots();

	// sets defaults for ui temp cvars
	uiInfo.currentCrosshair = (int)trap->Cvar_VariableValue("cg_drawCrosshair");
	trap->Cvar_Set("ui_mousePitch", (trap->Cvar_VariableValue("m_pitch") >= 0) ? "0" : "1");

	trap->Cvar_Get( "debug_protocol", "", 0, NULL, NULL );
}

void UI_KeyEvent( int key, qboolean down ) {

	if (Menu_Count() > 0) {
		menuDef_t *menu = Menu_GetFocused();
		if (menu) {
			if (key == K_ESCAPE && down && !Menus_AnyFullScreenVisible()) {
				Menus_CloseAll();
			} else {
				Menu_HandleKey(menu, key, down );
			}
		} else {
			trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
			trap->Key_ClearStates();
			trap->Cvar_Set( "cl_paused", "0" );
		}
	}

	//if ((s > 0) && (s != menu_null_sound)) {
	//  trap->S_StartLocalSound( s, CHAN_LOCAL_SOUND );
	//}
}

void UI_MouseEvent( int dx, int dy ) {
	// update mouse screen position
	uiInfo.uiDC.cursorx += dx;
	if (uiInfo.uiDC.cursorx < 0)
		uiInfo.uiDC.cursorx = 0;
	else if (uiInfo.uiDC.cursorx > SCREEN_WIDTH)
		uiInfo.uiDC.cursorx = SCREEN_WIDTH;

	uiInfo.uiDC.cursory += dy;
	if (uiInfo.uiDC.cursory < 0)
		uiInfo.uiDC.cursory = 0;
	else if (uiInfo.uiDC.cursory > SCREEN_HEIGHT)
		uiInfo.uiDC.cursory = SCREEN_HEIGHT;

	if (Menu_Count() > 0) {
		//menuDef_t *menu = Menu_GetFocused();
		//Menu_HandleMouseMove(menu, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
		Display_MouseMove(NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
	}

}

void UI_LoadNonIngame( void ) {
	const char *menuSet = UI_Cvar_VariableString("ui_menuFiles");
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/menus.txt";
	}
	UI_LoadMenus(menuSet, qfalse);
	uiInfo.inGameLoad = qfalse;
}

static void UI_SetActiveMenu( uiMenuCommand_t menu ) {
	char buf[256];

	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	if (Menu_Count() > 0) {
		vector3 v;
		VectorClear( &v );
		switch ( menu ) {
		case UIMENU_NONE:
			trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_UI );
			trap->Key_ClearStates();
			trap->Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();

			return;
		case UIMENU_MAIN:
			trap->Cvar_Set( "sv_killserver", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			//trap->S_StartLocalSound( trap->S_RegisterSound("sound/misc/menu_background.wav", qfalse) , CHAN_LOCAL_SOUND );
			//trap->S_StartBackgroundTrack("sound/misc/menu_background.wav", NULL);
			if (uiInfo.inGameLoad) {
				UI_LoadNonIngame();
			}
			Menus_CloseAll();
			Menus_ActivateByName("main");
			trap->Cvar_VariableStringBuffer("com_errorMessage", buf, sizeof(buf));
			if (strlen(buf)) {
				Menus_ActivateByName("error_popmenu");
			}
			return;
		case UIMENU_TEAM:
			trap->Key_SetCatcher( KEYCATCH_UI );
			Menus_ActivateByName("team");
			return;
		case UIMENU_POSTGAME:
			trap->Cvar_Set( "sv_killserver", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			if (uiInfo.inGameLoad) {
				UI_LoadNonIngame();
			}
			Menus_CloseAll();
			Menus_ActivateByName("endofgame");
			return;
		case UIMENU_INGAME:
			trap->Cvar_Set( "cl_paused", "1" );
			trap->Key_SetCatcher( KEYCATCH_UI );
			UI_BuildPlayerList();
			Menus_CloseAll();
			Menus_ActivateByName("ingame");
			return;
		}
	}
}

static connState_t	lastConnState;
static char			lastLoadingText[MAX_INFO_VALUE];

static void UI_ReadableSize ( char *buf, size_t bufsize, int value ) {
	if (value > 1024*1024*1024 ) { // gigs
		Com_sprintf( buf, bufsize, "%d", value / (1024*1024*1024) );
		Com_sprintf( buf+strlen(buf), bufsize-strlen(buf), ".%02d GB", 
			(value % (1024*1024*1024))*100 / (1024*1024*1024) );
	} else if (value > 1024*1024 ) { // megs
		Com_sprintf( buf, bufsize, "%d", value / (1024*1024) );
		Com_sprintf( buf+strlen(buf), bufsize-strlen(buf), ".%02d MB", 
			(value % (1024*1024))*100 / (1024*1024) );
	} else if (value > 1024 ) { // kilos
		Com_sprintf( buf, bufsize, "%d KB", value / 1024 );
	} else { // bytes
		Com_sprintf( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in msec
static void UI_PrintTime ( char *buf, size_t bufsize, int time ) {
	time /= 1000;  // change to seconds

	if (time > 3600) { // in the hours range
		Com_sprintf( buf, bufsize, "%d hr %d min", time / 3600, (time % 3600) / 60 );
	} else if (time > 60) { // mins
		Com_sprintf( buf, bufsize, "%d min %d sec", time / 60, time % 60 );
	} else  { // secs
		Com_sprintf( buf, bufsize, "%d sec", time );
	}
}

void Text_PaintCenter(float x, float y, float scale, const vector4 *color, const char *text, float adjust) {
	float len = Text_Width(text, scale, 0);
	Text_Paint(x - len / 2, y, scale, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
}

void Text_PaintCenter_AutoWrapped(float x, float y, float xmax, float ystep, float scale, const vector4 *color, const char *str, float adjust) {
	float width;
	char *s1,*s2,*s3;
	char c_bcp;
	char buf[1024];

	if (!str || str[0]=='\0')
		return;

	Q_strncpyz(buf, str, sizeof(buf));
	s1 = s2 = s3 = buf;

	while (1) {
		do {
			s3++;
		} while (*s3!=' ' && *s3!='\0');
		c_bcp = *s3;
		*s3 = '\0';
		width = Text_Width(s1, scale, 0);
		*s3 = c_bcp;
		if (width > xmax) {
			if (s1==s2)
			{
				// fuck, don't have a clean cut, we'll overflow
				s2 = s3;
			}
			*s2 = '\0';
			Text_PaintCenter(x, y, scale, color, s1, adjust);
			y += ystep;
			if (c_bcp == '\0')
			{
				// that was the last word
				// we could start a new loop, but that wouldn't be much use
				// even if the word is too long, we would overflow it (see above)
				// so just print it now if needed
				s2++;
				if (*s2 != '\0') // if we are printing an overflowing line we have s2 == s3
					Text_PaintCenter(x, y, scale, color, s2, adjust);
				break;
			}
			s2++;
			s1 = s2;
			s3 = s2;
		}
		else
		{
			s2 = s3;
			if (c_bcp == '\0') // we reached the end
			{
				Text_PaintCenter(x, y, scale, color, s1, adjust);
				break;
			}
		}
	}
}

static void UI_DisplayDownloadInfo( const char *downloadName, float centerPoint, float yStart, float scale ) {
	static char dlText[]	= "Downloading:";
	static char etaText[]	= "Estimated time left:";
	static char xferText[]	= "Transfer rate:";

	int downloadSize, downloadCount, downloadTime;
	char dlSizeBuf[64], totalSizeBuf[64], xferRateBuf[64], dlTimeBuf[64];
	int xferRate;
	float leftWidth;
	const char *s;

	downloadSize = (int)trap->Cvar_VariableValue( "cl_downloadSize" );
	downloadCount = (int)trap->Cvar_VariableValue( "cl_downloadCount" );
	downloadTime = (int)trap->Cvar_VariableValue( "cl_downloadTime" );

	leftWidth = (SCREEN_WIDTH/2.0f);

	UI_SetColor( &g_color_table[ColorIndex(COLOR_WHITE)] );
	Text_PaintCenter(centerPoint, yStart + 112, scale, &g_color_table[ColorIndex(COLOR_WHITE)], dlText, 0);
	Text_PaintCenter(centerPoint, yStart + 192, scale, &g_color_table[ColorIndex(COLOR_WHITE)], etaText, 0);
	Text_PaintCenter(centerPoint, yStart + 248, scale, &g_color_table[ColorIndex(COLOR_WHITE)], xferText, 0);

	if (downloadSize > 0) {
		s = va( "%s (%d%%)", downloadName,
			(int)( (float)downloadCount * 100.0f / downloadSize ) );
	} else {
		s = downloadName;
	}

	Text_PaintCenter(centerPoint, yStart+136, scale, &g_color_table[ColorIndex(COLOR_WHITE)], s, 0);

	UI_ReadableSize( dlSizeBuf,		sizeof dlSizeBuf,		downloadCount );
	UI_ReadableSize( totalSizeBuf,	sizeof totalSizeBuf,	downloadSize );

	if (downloadCount < 4096 || !downloadTime) {
		Text_PaintCenter(leftWidth, yStart+216, scale, &g_color_table[ColorIndex(COLOR_WHITE)], "estimating", 0);
		Text_PaintCenter(leftWidth, yStart+160, scale, &g_color_table[ColorIndex(COLOR_WHITE)], va("(%s of %s copied)", dlSizeBuf, totalSizeBuf), 0);
	} else {
		if ((uiInfo.uiDC.realTime - downloadTime) / 1000) {
			xferRate = downloadCount / ((uiInfo.uiDC.realTime - downloadTime) / 1000);
		} else {
			xferRate = 0;
		}
		UI_ReadableSize( xferRateBuf, sizeof xferRateBuf, xferRate );

		// Extrapolate estimated completion time
		if (downloadSize && xferRate) {
			int n = downloadSize / xferRate; // estimated time for entire d/l in secs

			// We do it in K (/1024) because we'd overflow around 4MB
			UI_PrintTime ( dlTimeBuf, sizeof dlTimeBuf, 
				(n - (((downloadCount/1024) * n) / (downloadSize/1024))) * 1000);

			Text_PaintCenter(leftWidth, yStart+216, scale, &g_color_table[ColorIndex(COLOR_WHITE)], dlTimeBuf, 0);
			Text_PaintCenter(leftWidth, yStart+160, scale, &g_color_table[ColorIndex(COLOR_WHITE)], va("(%s of %s copied)", dlSizeBuf, totalSizeBuf), 0);
		} else {
			Text_PaintCenter(leftWidth, yStart+216, scale, &g_color_table[ColorIndex(COLOR_WHITE)], "estimating", 0);
			if (downloadSize) {
				Text_PaintCenter(leftWidth, yStart+160, scale, &g_color_table[ColorIndex(COLOR_WHITE)], va("(%s of %s copied)", dlSizeBuf, totalSizeBuf), 0);
			} else {
				Text_PaintCenter(leftWidth, yStart+160, scale, &g_color_table[ColorIndex(COLOR_WHITE)], va("(%s copied)", dlSizeBuf), 0);
			}
		}

		if (xferRate) {
			Text_PaintCenter(leftWidth, yStart+272, scale, &g_color_table[ColorIndex(COLOR_WHITE)], va("%s/Sec", xferRateBuf), 0);
		}
	}
}

// This will also be overlaid on the cgame info screen during loading to prevent it from blinking away too rapidly on local or lan games.
static void UI_DrawConnectScreen( qboolean overlay ) {
	uiClientState_t	cstate;
	const char		*s;
	char			info[MAX_INFO_VALUE], text[256];
	float			centerPoint, yStart, scale;

	menuDef_t *menu = Menus_FindByName("connect");
	
	if ( !overlay && menu ) {
		Menu_Paint(menu, qtrue);
	}

	if (!overlay) {
		centerPoint = (SCREEN_WIDTH/2.0f);
		yStart = 130;
		scale = 0.5f;
	} else {
		return;
	}

	// see what information we should display
	trap->GetClientState( &cstate );

	info[0] = '\0';
	if( trap->GetConfigString( CS_SERVERINFO, info, sizeof(info) ) ) {
		Text_PaintCenter(centerPoint, yStart, scale, &g_color_table[ColorIndex(COLOR_WHITE)], va( "Loading %s", Info_ValueForKey( info, "mapname" )), 0);
	}

	if (!Q_stricmp(cstate.servername,"localhost")) {
		Text_PaintCenter(centerPoint, yStart + 48, scale, &g_color_table[ColorIndex(COLOR_WHITE)], "Starting up...", ITEM_TEXTSTYLE_SHADOWEDMORE);
	} else {
		strcpy(text, va("Connecting to %s", cstate.servername));
		Text_PaintCenter(centerPoint, yStart + 48, scale, &g_color_table[ColorIndex(COLOR_WHITE)],text , ITEM_TEXTSTYLE_SHADOWEDMORE);
	}

	// display global MOTD at bottom
	Text_PaintCenter(centerPoint, 600, scale, &g_color_table[ColorIndex(COLOR_WHITE)], Info_ValueForKey( cstate.updateInfoString, "motd" ), 0);
	// print any server info (server full, bad version, etc)
	if ( cstate.connState < CA_CONNECTED ) {
		Text_PaintCenter_AutoWrapped(centerPoint, yStart + 176, 630, 20, scale, &g_color_table[ColorIndex(COLOR_WHITE)], cstate.messageString, 0);
	}

	if ( lastConnState > cstate.connState ) {
		lastLoadingText[0] = '\0';
	}
	lastConnState = cstate.connState;

	switch ( cstate.connState ) {
	case CA_CONNECTING:
		s = va("Awaiting connection...%i", cstate.connectPacketCount);
		break;
	case CA_CHALLENGING:
		s = va("Awaiting challenge...%i", cstate.connectPacketCount);
		break;
	case CA_CONNECTED: {
		char downloadName[MAX_INFO_VALUE];

		trap->Cvar_VariableStringBuffer( "cl_downloadName", downloadName, sizeof(downloadName) );
		if (*downloadName) {
			UI_DisplayDownloadInfo( downloadName, centerPoint, yStart, scale );
			return;
		}
					   }
					   s = "Awaiting gamestate...";
					   break;
	case CA_LOADING:
		return;
	case CA_PRIMED:
		return;
	default:
		return;
	}


	if (Q_stricmp(cstate.servername,"localhost")) {
		Text_PaintCenter(centerPoint, yStart + 80, scale, &g_color_table[ColorIndex(COLOR_WHITE)], s, 0);
	}

	// password required / connection rejected information goes here
}

uiImport_t *trap = NULL;
Q_EXPORT uiExport_t* QDECL GetModuleAPI( int apiVersion, uiImport_t *import ) {
	static uiExport_t uie = {0};
	
	assert( import );
	trap = import;
	Com_Printf	= trap->Print;
	Com_Error	= trap->Error;

	memset( &uie, 0, sizeof( uie ) );

	if ( apiVersion != UI_API_VERSION ) {
		trap->Print( "Mismatched UI_API_VERSION: expected %i, got %i\n", UI_API_VERSION, apiVersion );
		return NULL;
	}

	uie.Init				= UI_Init;
	uie.Shutdown			= UI_Shutdown;
	uie.KeyEvent			= UI_KeyEvent;
	uie.MouseEvent			= UI_MouseEvent;
	uie.Refresh				= UI_Refresh;
	uie.IsFullscreen		= Menus_AnyFullScreenVisible;
	uie.SetActiveMenu		= UI_SetActiveMenu;
	uie.ConsoleCommand		= UI_ConsoleCommand;
	uie.DrawConnectScreen	= UI_DrawConnectScreen;

	return &uie;
}
