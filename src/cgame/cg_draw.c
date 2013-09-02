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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#include "../ui/ui_shared.h"

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

//QtZ: Zoom fov
extern float zoomFov; //this has to be global client-side

char systemChat[256];
char teamChat1[256];
char teamChat2[256];


int CG_Text_Width(const char *text, float scale, int limit) {
	int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
	// FIXME: see ui_main.c, same problem
	//	const unsigned char *s = text;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= ui_smallFont->value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > ui_bigFont->value) {
		font = &cgDC.Assets.bigFont;
	}
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
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}
	return (int)(out * useScale);
}

int CG_Text_Height(const char *text, float scale, int limit) {
	int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
	// TTimo: FIXME
	//	const unsigned char *s = text;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= ui_smallFont->value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > ui_bigFont->value) {
		font = &cgDC.Assets.bigFont;
	}
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
				if (max < (float)glyph->height) {
					max = (float)glyph->height;
				}
				s++;
				count++;
			}
		}
	}
	return (int)(max * useScale);
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
	float w, h;
	w = width * scale;
	h = height * scale;
	CG_AdjustFrom640( &x, &y, &w, &h );
	trap->R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint(float x, float y, float scale, vector4 *color, const char *text, float adjust, int limit, int style) {
	int len, count;
	vector4 newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= ui_smallFont->value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > ui_bigFont->value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
	if (text) {
		// TTimo: FIXME
		//		const unsigned char *s = text;
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
				float yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					vector4 colorBlack;
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;

					VectorCopy4( &g_color_table[ColorIndex(COLOR_BLACK)], &colorBlack );

					colorBlack.a = newColor.a;
					trap->R_SetColor( &colorBlack );
					CG_Text_PaintChar(x + ofs, y - yadj + ofs, 
						(float)glyph->imageWidth,
						(float)glyph->imageHeight,
						useScale, 
						glyph->s,
						glyph->t,
						glyph->s2,
						glyph->t2,
						glyph->glyph);
					colorBlack.a = 1.0;
					trap->R_SetColor( &newColor );
				}
				CG_Text_PaintChar(x, y - yadj, 
					(float)glyph->imageWidth,
					(float)glyph->imageHeight,
					useScale, 
					glyph->s,
					glyph->t,
					glyph->s2,
					glyph->t2,
					glyph->glyph);
				// CG_DrawPic(x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph);
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
		}
		trap->R_SetColor( NULL );
	}
}

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vector3 *origin, vector3 *angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons->boolean || !cg_drawIcons->boolean ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, &ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = (int)x;
	refdef.y = (int)y;
	refdef.width = (int)w;
	refdef.height = (int)h;

	refdef.time = cg.time;

	trap->R_ClearScene();
	trap->R_AddRefEntityToScene( &ent );
	trap->R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vector3 *headAngles ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vector3			origin;
	vector3			mins, maxs;

	ci = &cgs.clientinfo[ clientNum ];

	if ( cg_draw3dIcons->boolean ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap->R_ModelBounds( cm, &mins, &maxs );

		origin.z = -0.5f * ( mins.z + maxs.z );
		origin.y =  0.5f * ( mins.y + maxs.y );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7f * ( maxs.z - mins.z );		
		origin.x = len / 0.268f;	// len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( &origin, &ci->headOffset, &origin );

		CG_Draw3DModel( x, y, w, h, ci->headModel, ci->headSkin, &origin, headAngles );
	} else if ( cg_drawIcons->boolean ) {
		CG_DrawPic( x, y, w, h, ci->modelIcon );
	}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vector3			origin, angles;
	vector3			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons->boolean ) {

		VectorClear( &angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap->R_ModelBounds( cm, &mins, &maxs );

		origin.z = -0.5f * ( mins.z + maxs.z );
		origin.y =  0.5f * ( mins.y + maxs.y );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5f * ( maxs.z - mins.z );		
		origin.x = len / 0.268f;	// len / tan( fov/2 )

		angles.yaw = 60 * sinf( cg.time / 2000.0f );;

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, 0, &origin, &angles );
	} else if ( cg_drawIcons->boolean ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vector4		hcolor;

	hcolor.a = alpha;
	if ( team == TEAM_RED ) {
		hcolor.r = 1;
		hcolor.g = 0;
		hcolor.b = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor.r = 0;
		hcolor.g = 0;
		hcolor.b = 1;
	} else {
		return;
	}
	trap->R_SetColor( &hcolor );
	CG_DrawPic( (float)x, (float)y, (float)w, (float)h, cgs.media.teamStatusBar );
	trap->R_SetColor( NULL );
}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker( float y ) {
	int			t;
	float		size;
	vector3		angles;
	const char	*info;
	const char	*name;
	int			clientNum;

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if ( !cg.attackerTime ) {
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) {
		return y;
	}

	t = cg.time - cg.attackerTime;
	if ( t > ATTACKER_HEAD_TIME ) {
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

	angles.pitch = 0;
	angles.yaw = 180;
	angles.roll = 0;
	CG_DrawHead( SCREEN_WIDTH - size, y, size, size, clientNum, &angles );

	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = Info_ValueForKey(  info, "n" );
	y += size;
	CG_DrawBigString( (int)SCREEN_WIDTH - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), (int)y, name, 0.5f );

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, (int)y + 2, s, 1.0f);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#if 0
#define	FPS_FRAMES	4
static float CG_DrawFPS( float y ) {
	char		*s;
	int			w;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	int		fps;
	static	int	previous;
	int		t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap->Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "%ifps", fps );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

		CG_DrawBigString( 635 - w, y + 2, s, 1.0F);
	}

	return y + BIGCHAR_HEIGHT + 4;
}
#else
#define	FPS_FRAMES	16
#define FPS_MASK (FPS_FRAMES-1)

static float CG_DrawFPS( float y ) {
	static unsigned short previousTimes[FPS_FRAMES];
	static unsigned short index;
	static int	previous, lastupdate;
	int		t, i, total;
	unsigned short frameTime;
	vector4 fpsColour = { 1.0f, 1.0f, 1.0f, 1.0f }, fpsGood = { 0.0f, 1.0f, 0.0f, 1.0f }, fpsBad = { 1.0f, 0.0f, 0.0f, 1.0f };
	float point = 1.0f;//= min( cg.snap->ping / 300.0f, 1.0f );
	char *tmp = NULL;
	float fps, maxFPS = 1000.0f/(float)atof( CG_Cvar_VariableString( "com_frametime" ) );

	if ( !cg_drawFPS->boolean && !cg_debugHUD->boolean )
		return y;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap->Milliseconds();
	frameTime = t - previous;
	previous = t;
	if ( t - lastupdate > 50 ) {// don't sample faster than this
		lastupdate = t;
		previousTimes[index++ & FPS_MASK] = frameTime;
	}
	// average multiple frames together to smooth changes out a bit
	total = 0;
	for ( i = 0 ; i < FPS_FRAMES ; i++ )
		total += previousTimes[i];
	if ( !total )
		total = 1;

	fps = 1000.0f * (float)((float)FPS_FRAMES / (float)total);

	point = MIN( MAX(0.0f, fps) / MAX(60.0f, maxFPS), 1.0f );
	VectorLerp4( &fpsBad, point, &fpsGood, &fpsColour );
	fpsColour.a = 1.0f;

	tmp = va( "%3i FPS", (int)fps );
	CG_DrawSmallStringColor( (int)SCREEN_WIDTH-CG_Text_Width( tmp, ui_smallFont->value, 0 ), (int)y, tmp, &fpsColour );

	return y-14.0f;
}
#endif

static float CG_DrawPing( float y )
{
	vector4 pingColour = { 1.0f, 1.0f, 1.0f, 1.0f }, pingGood = { 0.0f, 1.0f, 0.0f, 1.0f }, pingBad = { 1.0f, 0.0f, 0.0f, 1.0f };
	float point = MIN( cg.snap->ping / 300.0f, 1.0f );
	char *tmp = NULL;

	if ( !cg_drawPing->boolean && cg_debugHUD->boolean )
		return y;

	VectorLerp4( &pingBad, point, &pingGood, &pingColour );
	pingColour.a = 1.0f;

	tmp = va( "%3i ping", cg.snap->ping );
	CG_DrawSmallStringColor( (int)SCREEN_WIDTH-CG_Text_Width( tmp, ui_smallFont->value, 0 ), (int)y, tmp, &pingColour );
	return y-14.0f;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, (int)y + 2, s, 1.0f);

	return y + BIGCHAR_HEIGHT + 4;
}


/*
=================
CG_DrawTeamOverlay
=================
*/

static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	int x, w, h, xx, pw;
	int i, len, count;
	const char *p;
	vector4		hcolor;
	int pwidth, lwidth, plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t	*item;
	float ret_y;

	if ( !cg_drawTeamOverlay->boolean )
		return y;

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE )
		return y; // Not on any team

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if ( !plyrs )
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if ( right )
		x = (int)SCREEN_WIDTH - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper )
		ret_y = (float)y + h;
	else {
		y -= h;
		ret_y = (float)y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor.r = 1.0f;
		hcolor.g = 0.0f;
		hcolor.b = 0.0f;
		hcolor.a = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor.r = 0.0f;
		hcolor.g = 0.0f;
		hcolor.b = 1.0f;
		hcolor.a = 0.33f;
	}
	trap->R_SetColor( &hcolor );
	CG_DrawPic( (float)x, y, (float)w, (float)h, cgs.media.teamStatusBar );
	trap->R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			hcolor.r = hcolor.g = hcolor.b = hcolor.a = 1.0f;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt( xx, (int)y,
				ci->name, &hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p)
					p = "unknown";
//				len = CG_DrawStrlen(p);
//				if (len > lwidth)
//					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth + 
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt( xx, (int)y,
					p, &hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth( ci->health, ci->armor, &hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 + 
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt( xx, (int)y, st, &hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if ( cg_weapons[ci->curWeapon].weaponIcon )
				CG_DrawPic( (float)xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, cg_weapons[ci->curWeapon].weaponIcon );
			else
				CG_DrawPic( (float)xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, cgs.media.deferShader );

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for ( pw=PW_NONE; pw<PW_NUM_POWERUPS; pw++ ) {
				if (ci->powerups & (1 << pw)) {

					if ( (item=BG_FindItemForPowerup( (powerup_t)pw )) ) {
						CG_DrawPic( (float)xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
						trap->R_RegisterShader( item->icon ) );
						if ( right )	xx -= TINYCHAR_WIDTH;
						else			xx += TINYCHAR_WIDTH;
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return (float)ret_y;
//#endif
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward( void ) { 
	vector4 *color;
	int		i, count;
	float	x, y;
	char	buf[32];

	if ( !cg_drawRewards->boolean ) {
		return;
	}

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			trap->S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		} else {
			return;
		}
	}

	trap->R_SetColor( color );

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = (SCREEN_WIDTH/2) - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.rewardCount[0] >= 10 ) {
		y = 56;
		x = (SCREEN_WIDTH/2) - ICON_SIZE/2;
		CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = ( SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen( buf ) ) / 2;
		CG_DrawStringExt( (int)x, (int)y+ICON_SIZE, buf, color, qfalse, qtrue,
								SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
	}
	else {

		count = cg.rewardCount[0];

		y = 56;
		x = (SCREEN_WIDTH/2) - count * ICON_SIZE/2;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
			x += ICON_SIZE;
		}
	}
	trap->R_SetColor( NULL );
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


typedef struct {
	float	frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	float	snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = (float)offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = (float)snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;

	//OSP: dont draw if a demo and we're running at a different timescale
	if ( cg.demoPlayback && timescale->value != 1.0f )
		return;

	//ydnar: don't draw if the server is respawning
//	if ( cg.serverRespawning )
//		return;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap->GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap->GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart
		return;
	}

	// also add text in center of screen
	s = "Connection Interrupted";
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( ((int)SCREEN_WIDTH/2) - w/2, 100, s, 1.0f);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = SCREEN_WIDTH - ICON_SIZE;
	y = SCREEN_HEIGHT - ICON_SIZE;

	CG_DrawPic( x, y, ICON_SIZE, ICON_SIZE, trap->R_RegisterShader("gfx/2d/net.tga" ) );
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer->boolean /*|| cgs.localServer*/ ) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	x = (int)SCREEN_WIDTH - ICON_SIZE;
	y = (int)SCREEN_HEIGHT - ICON_SIZE-96;

	trap->R_SetColor( NULL );
	CG_DrawPic( (float)x, (float)y, (float)ICON_SIZE, (float)ICON_SIZE, cgs.media.lagometerShader );

	ax = (float)x;
	ay = (float)y;
	aw = (float)ICON_SIZE;
	ah = (float)ICON_SIZE;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap->R_SetColor( &g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap->R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap->R_SetColor( &g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap->R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap->R_SetColor( &g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap->R_SetColor( &g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap->R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap->R_SetColor( &g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap->R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap->R_SetColor( NULL );

	if ( bg_synchronousClients->boolean ) {
		CG_DrawBigString( x, y, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int		x, y, w;
	int h;
	vector4 *color;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, (int)(1000 * cg_centertime->value) );
	if ( !color ) {
		return;
	}

	trap->R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = CG_Text_Width(linebuffer, 0.5f, 0);
		h = 26;//CG_Text_Height(linebuffer, 0.5, 0);
		x = ((int)SCREEN_WIDTH - w) / 2;
		CG_Text_Paint((float)x, (float)(y + h), 0.5f, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		y += h + 6;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap->R_SetColor( NULL );
}



/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(void)
{
	float		x = 0.0f, y = 0.0f, w = cg_crosshairSize->value, h = cg_crosshairSize->value, f = 0.0f;
	int			i = 0;
	vector4	finalColour = { 1.0 };

	if ( !cg_drawCrosshair->integer
		|| cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR
		|| (cg.zoomed && cg_crosshairDisableZoomed->boolean) )//zoom mask?
		return;

	if ( cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR &&
		(cg_crosshairCooldownWeapons->integer & (1<<cg.predictedPlayerState.weapon)) )
	{//QtZ: Weapon cooldown tics
		//QTZTODO: Per weapon tics? tics for ammo?
		for ( i=0; i<cg_crosshairCooldownTics->integer; i++ )
		{
			float cooldown = (float)Q_bumpi( cg.predictedPlayerState.weaponCooldown[cg.predictedPlayerState.weapon], weaponData[cg.predictedPlayerState.weapon].fireTime );
			float cdmax = 1.0f - (cooldown / weaponData[cg.predictedPlayerState.weapon].fireTime);
			float inc = RAD2DEG(M_PI*2)/cg_crosshairCooldownTics->value;
			float angle = inc*i;
			float cx = (SCREEN_WIDTH/2.0f) + cosf( DEG2RAD(angle) ) * cg_crosshairCooldownSpread->value;
			float cy = (SCREEN_HEIGHT/2.0f) + sinf( DEG2RAD(angle) ) * cg_crosshairCooldownSpread->value;
			float finalFade = (cdmax/0.2f);
			vector4 colour = { 1.0f, 1.0f, 1.0f, 1.0f };

			if ( cdmax < 0.2f )
				colour.a = finalFade;

			trap->R_SetColor( &colour );
			if ( angle/360.0f < cdmax )
				CG_DrawRotatePic2( cx, cy, 8, 8, angle-90.0f, cgs.media.crosshair.cooldownTic );
		}
		trap->R_SetColor( NULL );
	}

	// set color based on health
	if ( cg_crosshairHealth->boolean )
		CG_ColorForHealth( &finalColour );

//	else if ( cg.qtz.crosshairDamageTime > cg.time - cg_crosshairFeedbackTime->integer )
	{//QtZ: Colour crosshair if we recently hit someone (Damage feedback)
		float point = Q_clamp( 0.0f, (float)(cg.time - cg.crosshair.damageTime) / cg_crosshairFeedbackTime->value, 1.0f );
		finalColour.r = LERP2( cg.crosshair.feedbackColor.r/255.0f, cg.crosshair.baseColor.r/255.0f, point );
		finalColour.g = LERP2( cg.crosshair.feedbackColor.g/255.0f, cg.crosshair.baseColor.g/255.0f, point );
		finalColour.b = LERP2( cg.crosshair.feedbackColor.b/255.0f, cg.crosshair.baseColor.b/255.0f, point );
		finalColour.a = 1.0f;
	}
	trap->R_SetColor( &finalColour );

	w = h = cg_crosshairSize->value;

	// pulse the size of the crosshair when picking up items
	f = (float)(cg.time - cg.itemPickupBlendTime);
	if ( f > 0 && f < ITEM_BLOB_TIME )
	{
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

#if 0 // no longer doing this
	// pulse the size of the crosshair for damage feedback
	f = cg.time - cg.crosshair.damageTime;
	if ( f > 0 && f < cg_crosshairFeedbackTime.value ) {
		f /= cg_crosshairFeedbackTime.value;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}
#endif

	x = cg.refdef.x + 0.5f * (SCREEN_WIDTH - w);
	y = cg.refdef.y + 0.5f * (SCREEN_HEIGHT - h);
	CG_AdjustFrom640( &x, &y, &w, &h );

	for ( i=0; i<NUM_CROSSHAIRS; i++ ) {
		if ( cg_drawCrosshair->integer & (1<<i) )
			trap->R_DrawStretchPic( x, y, w, h, 0, 0, 1, 1, cgs.media.crosshair.images[i] );
	}
//	trap->R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), 
//		y + cg.refdef.y + 0.5 * (cg.refdef.height - h), 
//		w, h, 0, 0, 1, 1, hShader );
}

/*
=================
CG_DrawCrosshair3D
=================
*/
static void CG_DrawCrosshair3D(void)
{
	float		w;
	qhandle_t	hShader;
	float		f;
	int			ca;

	trace_t trace;
	vector3 endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if ( !cg_drawCrosshair->integer ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	w = cg_crosshairSize->value;

	// pulse the size of the crosshair when picking up items
	f = (float)(cg.time - cg.itemPickupBlendTime);
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
	}

	ca = cg_drawCrosshair->integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshair.images[ ca % NUM_CROSSHAIRS ];

	// Use a different method rendering the crosshair so players don't see two of them when
	// focusing their eyes at distant objects with high stereo separation
	// We are going to trace to the next shootable object and place the crosshair in front of it.

	// first get all the important renderer information
	trap->Cvar_VariableStringBuffer("r_zProj", rendererinfos, sizeof(rendererinfos));
	zProj = (float)atof(rendererinfos);
	trap->Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos, sizeof(rendererinfos));
	stereoSep = zProj / (float)atof(rendererinfos);
	
	xmax = zProj * tanf(cg.refdef.fov_x * M_PI / 360.0f);
	
	// let the trace run through until a change in stereo separation of the crosshair becomes less than one pixel.
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	VectorMA(&cg.refdef.vieworg, maxdist, &cg.refdef.viewaxis[0], &endpos);
	CG_Trace(&trace, &cg.refdef.vieworg, NULL, NULL, &endpos, 0, MASK_SHOT);
	
	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;
	
	VectorCopy(&trace.endpos, &ent.origin);
	
	// scale the crosshair so it appears the same size for all distances
	ent.radius = w / SCREEN_WIDTH * xmax * trace.fraction * maxdist / zProj;
	ent.customShader = hShader;

	trap->R_AddRefEntityToScene(&ent);
}



/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vector3		start, end;
	int			content;

	VectorCopy( &cg.refdef.vieworg, &start );
	VectorMA( &start, 131072, &cg.refdef.viewaxis[0], &end );

	CG_Trace( &trace, &start, &vec3_origin, &vec3_origin, &end, 
		cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY );
	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

	// if the player is in fog, don't show it
	content = CG_PointContents( &trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	vector4		*color;
	char		*name;
	float		w;

	if ( !cg_drawCrosshair->integer ) {
		return;
	}
	if ( !cg_drawCrosshairNames->boolean ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( !color ) {
		trap->R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
	color->a *= 0.5f;
	w = (float)CG_Text_Width(name, 0.3f, 0);
	CG_Text_Paint( (SCREEN_WIDTH/2) - w / 2, 190, 0.3f, color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
	trap->R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {
	CG_DrawBigString( ((int)SCREEN_WIDTH/2) - 9 * 8, 440, "SPECTATOR", 1.0f );
	if ( cgs.gametype == GT_DUEL )
		CG_DrawBigString( ((int)SCREEN_WIDTH/2) - 15 * 8, 460, "waiting to play", 1.0f );
	else if ( cgs.gametype >= GT_TEAM )
		CG_DrawBigString( ((int)SCREEN_WIDTH/2) - 39 * 8, 460, "press ESC and use the JOIN menu to play", 1.0f );
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	char	*s;
	int		sec;

	if ( !cgs.voteTime ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap->S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo);
	CG_DrawSmallString( 0, 58, s, 1.0F );
	s = "or press ESC then click Vote";
	CG_DrawSmallString( 0, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0F );
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
//	int key;

	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	float		x;
	vector4		color;
	const char	*name;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		return qfalse;
	}
	color.r = 1;
	color.g = 1;
	color.b = 1;
	color.a = 1;

	CG_DrawBigString( ((int)SCREEN_WIDTH/2) - 9 * 8, 24, "following", 1.0f );

	name = cgs.clientinfo[ cg.snap->ps.clientNum ].name;

	x = 0.5f * ( SCREEN_WIDTH - GIANT_WIDTH * CG_DrawStrlen( name ) );

	CG_DrawStringExt( (int)x, 40, name, &color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );

	return qtrue;
}



/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
	const char	*s;
	int			w;

	if ( cg_drawAmmoWarning->integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( ((int)SCREEN_WIDTH/2) - w / 2, 64, s, 1.0f );
}

/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	int			w;
	int			sec;
	int			i;
	float		scale;
	clientInfo_t	*ci1, *ci2;
	const char	*s;

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
		s = "Waiting for players";		
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString( ((int)SCREEN_WIDTH/2) - w / 2, 24, s, 1.0f );
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_DUEL) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
				if ( !ci1 ) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if ( ci1 && ci2 ) {
			s = va( "%s vs %s", ci1->name, ci2->name );
			w = CG_Text_Width(s, 0.6f, 0);
			CG_Text_Paint((SCREEN_WIDTH/2) - w / 2, 60, 0.6f, &g_color_table[ColorIndex(COLOR_WHITE)], s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		}
	} else {
		s = BG_GetGametypeString( cgs.gametype );
		w = CG_Text_Width(s, 0.6f, 0);
		CG_Text_Paint((SCREEN_WIDTH/2) - w / 2, 90, 0.6f, &g_color_table[ColorIndex(COLOR_WHITE)], s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}
	s = va( "Starts in: %i", sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;
		switch ( sec ) {
		case 0:
			trap->S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
			break;
		case 1:
			trap->S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
			break;
		case 2:
			trap->S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
			break;
		default:
			break;
		}
	}

	switch ( cg.warmupCount ) {
	case 0:
		scale = 0.54f;
		break;
	case 1:
		scale = 0.51f;
		break;
	case 2:
		scale = 0.48f;
		break;
	default:
		scale = 0.45f;
		break;
	}

	w = CG_Text_Width(s, scale, 0);
	CG_Text_Paint((SCREEN_WIDTH/2) - w / 2, 125, scale, &g_color_table[ColorIndex(COLOR_WHITE)], s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
}

//==================================================================================
/* 
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus( void ) {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap->Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}

void CG_DrawDebugInfo( void ) {
	CG_Text_Paint( 32.0f, 128.0f, 0.25f, &g_color_table[ColorIndex(COLOR_WHITE)], va( "movement dir: %d", cg.predictedPlayerState.movementDir ), 0.0f, -1, 0 );
}

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(stereoFrame_t stereoFrame)
{
	float y = 0.0f;
	if (cgs.orderPending && cg.time > cgs.orderTime) {
		CG_CheckOrderPending();
	}

	if ( !cg_draw2D->boolean ) {
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

/*
	if (cg.cameraMode) {
		return;
	}
*/
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawSpectator();

		if(stereoFrame == STEREO_CENTER)
			CG_DrawCrosshair();

		CG_DrawCrosshairNames();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {

			if ( cg_drawStatus->boolean ) {
				Menu_PaintAll();
				CG_DrawTimedMenus();
#ifdef _DEBUG
				CG_DrawDebugInfo();
#endif
			}
      
			CG_DrawAmmoWarning();

			if(stereoFrame == STEREO_CENTER)
				CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();

			//CG_DrawPersistantPowerup();

			CG_DrawReward();
		}
    
		if ( cgs.gametype >= GT_TEAM ) {
		}
	}

	CG_DrawVote();
	CG_DrawLagometer();

	y = SCREEN_HEIGHT-SMALLCHAR_HEIGHT;

	if ( !CG_DrawFollow() )
		CG_DrawWarmup();

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if ( !cg.scoreBoardShowing) {
		CG_DrawCenterString();
	}
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && (cg.snap->ps.pm_flags & PMF_SCOREBOARD) ) {
	//	CG_DrawTourneyScoreboard();
		return;
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	if(stereoView != STEREO_CENTER)
		CG_DrawCrosshair3D();

	// draw 3D view
	trap->R_RenderScene( &cg.refdef );

	// draw status bar and other floating elements
 	CG_Draw2D(stereoView);
}



