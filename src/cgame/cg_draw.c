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

float CG_Text_Width(const char *text, float scale, int limit) {
	int count;
	size_t len;
	float out, useScale;
	glyphInfo_t *glyph;

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
	return (out * useScale);
}

float CG_Text_Height(const char *text, float scale, int limit) {
	size_t len;
	int count;
	float max, useScale;
	glyphInfo_t *glyph;
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
	return (max * useScale);
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
	float w, h;
	w = width * scale;
	h = height * scale;
	CG_AdjustFrom640( &x, &y, &w, &h );
	trap->R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint(float x, float y, float scale, const vector4 *color, const char *text, float adjust, int limit, int style) {
	size_t len;
	int count;
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
					colorBlack.a = 1.0f;
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

// Used for both the status bar and the scoreboard
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

// Used for both the status bar and the scoreboard
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
		const gitem_t *item;

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

void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team ) {
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

  LOWER RIGHT CORNER

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
		Com_sprintf( buf, sizeof( buf ), "%d", cg.rewardCount[0] );
		x = ( SCREEN_WIDTH - CG_Text_Width( buf, ui_smallFont->value, 0 ) ) / 2.0f;
		CG_Text_Paint( x, y+ICON_SIZE, ui_smallFont->value, color, buf, 0.0f, 0, 0 );
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

	LAGOMETER

*/

#define	LAG_SAMPLES		128


struct lagometer_s {
	float	frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	float	snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer;

// Adds the current interpolate / extrapolate bar for this frame
void CG_AddLagometerFrameInfo( void ) {
	int offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = (float)offset;
	lagometer.frameCount++;
}

// Each time a snapshot is received, log its ping time and the number of snapshots that were dropped before it.
//	Pass NULL for a dropped packet.
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

// Should we draw something different for long lag vs no packets?
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char	*s;
	float		w;

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
	w = CG_Text_Width( s, 1.0f, 0 );
	CG_Text_Paint( (SCREEN_WIDTH/2.0f) - (w/2.0f), 100, 1.0f, (vector4*)&g_color_table[ColorIndex(COLOR_WHITE)], s, 0.0f, 0, 0 );

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

static void CG_DrawLagometer( void ) {
	int		a, i;
	float	v, x, y, ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer->boolean /*|| cgs.localServer*/ ) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	x = SCREEN_WIDTH - ICON_SIZE;
	y = SCREEN_HEIGHT - ICON_SIZE-96;

	trap->R_SetColor( NULL );
	CG_DrawPic( x, y, ICON_SIZE, ICON_SIZE, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = ICON_SIZE;
	ah = ICON_SIZE;
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
	range = ah / 2.0f;
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

	CG_DrawDisconnect();
}



/*

	CENTER PRINTING

*/


// Called for important messages that should stay in the center of the screen for a few moments
void CG_CenterPrint( const char *str, int y ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}

static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	float	x, y, w, h;
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

	y = cg.centerPrintY - cg.centerPrintLines * CG_Text_Height( "\n", 0.5f, 0 )/ 2;

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
		h = 26;//CG_Text_Height(linebuffer, 0.5f, 0);
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

	CROSSHAIR

*/


static void CG_DrawCrosshair( void ) {
	float		x = 0.0f, y = 0.0f, w = cg_crosshairSize->value, h = cg_crosshairSize->value, f = 0.0f;
	int			i = 0;
	vector4	finalColour = { 1.0f };

	if ( !cg_drawCrosshair->integer
		|| cg.snap->ps.persistent[PERS_TEAM] == TEAM_SPECTATOR
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
				CG_DrawRotatePic( cx, cy, 8, 8, angle-90.0f, cgs.media.crosshair.cooldownTic, qtrue );
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
//	trap->R_DrawStretchPic( x + cg.refdef.x + 0.5f * (cg.refdef.width - w), 
//		y + cg.refdef.y + 0.5f * (cg.refdef.height - h), 
//		w, h, 0, 0, 1, 1, hShader );
}

static void CG_DrawCrosshair3D( void ) {
	float		w;
	qhandle_t	hShader;
	float		f;
	int			ca;

	trace_t trace;
	vector3 endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[MAX_CVAR_VALUE_STRING];
	refEntity_t ent;

	if ( !cg_drawCrosshair->integer ) {
		return;
	}

	if ( cg.snap->ps.persistent[PERS_TEAM] == TEAM_SPECTATOR) {
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

static void CG_DrawSpectator( void ) {
	float scale = 0.5f;
	char *s = "Following";

	if ( cgs.gametype >= GT_TEAMBLOOD )
		s = "press ESC and use the JOIN menu to play";

	CG_Text_Paint( (SCREEN_WIDTH/2) - CG_Text_Width( s, scale, 0 )/2, 440, scale, (vector4*)&g_color_table[ColorIndex(COLOR_WHITE)], s, 0.0f, 0, 0 );
}

static void CG_DrawVote( void ) {
	const char *s;
	int sec;

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
	CG_Text_Paint( 0, 58, 1.0f, (vector4*)&g_color_table[ColorIndex(COLOR_WHITE)], s, 0.0f, 0, 0 );
	s = "or press ESC then click Vote";
	CG_Text_Paint( 0, 58 + CG_Text_Height( s, 1.0f, 0 ) + 2, 1.0f, (vector4*)&g_color_table[ColorIndex(COLOR_WHITE)], s, 0.0f, 0, 0 );
}

static void CG_DrawIntermission( void ) {
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

static qboolean CG_DrawFollow( void ) {
	const char *s;
	float w;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) )
		return qfalse;
	
	s = "following";
	w = CG_Text_Width( s, ui_bigFont->value, 0 );
	CG_Text_Paint( (SCREEN_WIDTH/2.0f) - (w/2.0f), 24, ui_bigFont->value, (vector4*)&g_color_table[ColorIndex(COLOR_WHITE)], s, 0.0f, 0, 0 );

	s = cgs.clientinfo[cg.snap->ps.clientNum].name;
	w = CG_Text_Width( s, ui_bigFont->value, 0 );
	CG_Text_Paint( (SCREEN_WIDTH/2.0f) - (w/2.0f), 40, ui_bigFont->value, (vector4*)&g_color_table[ColorIndex(COLOR_WHITE)], s, 0.0f, 0, 0 );

	return qtrue;
}

static void CG_DrawAmmoWarning( void ) {
	const char *s = "LOW AMMO WARNING";
	float w;

	if ( cg_drawAmmoWarning->integer == 0 )
		return;

	if ( !cg.lowAmmoWarning )
		return;

	if ( cg.lowAmmoWarning == 2 )
		s = "OUT OF AMMO";

	w = CG_Text_Width( s, 1.0f, 0 );
	CG_Text_Paint( (SCREEN_WIDTH/2.0f) - (w/2.0f), 64, 1.0f, (vector4*)&g_color_table[ColorIndex(COLOR_WHITE)], s, 0.0f, 0, 0 );
}

static void CG_DrawWarmup( void ) {
	float		w, scale;
	int			sec, i;
	clientInfo_t	*ci1, *ci2;
	const char	*s;

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
		s = "Waiting for players";		
		w = CG_Text_Width( s, 0.5f, 0 );
		CG_Text_Paint( ((int)SCREEN_WIDTH/2) - w / 2, 24, 0.5f, (vector4*)&g_color_table[ColorIndex(COLOR_WHITE)], s, 0.0f, 0, 0 );
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
			CG_Text_Paint((SCREEN_WIDTH/2) - w / 2, 60, 0.6f, (vector4*)&g_color_table[ColorIndex(COLOR_WHITE)], s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		}
	} else {
		s = GametypeStringForID( cgs.gametype );
		w = CG_Text_Width(s, 0.6f, 0);
		CG_Text_Paint((SCREEN_WIDTH/2) - w / 2, 90, 0.6f, (vector4*)&g_color_table[ColorIndex(COLOR_WHITE)], s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
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
	CG_Text_Paint((SCREEN_WIDTH/2) - (w / 2.0f), 125, scale, (vector4*)&g_color_table[ColorIndex(COLOR_WHITE)], s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
}

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
//	CG_Text_Paint( 32.0f, 128.0f, 0.25f, &g_color_table[ColorIndex(COLOR_WHITE)], va( "movement dir: %d", cg.predictedPlayerState.movementDir ), 0.0f, -1, 0 );
}

static void CG_Draw2D(stereoFrame_t stereoFrame) {
	if ( !cg_draw2D->boolean )
		return;

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

	if ( cg.snap->ps.persistent[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawSpectator();

		if ( stereoFrame == STEREO_CENTER )
			CG_DrawCrosshair();

		CG_DrawCrosshairNames();
	}
	else {
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

			if ( stereoFrame == STEREO_CENTER )
				CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();

		//	CG_DrawPersistantPowerup();

			CG_DrawReward();
		}
	}

	CG_DrawVote();
	CG_DrawLagometer();
	CG_DrawChatbox();

	if ( !CG_DrawFollow() )
		CG_DrawWarmup();

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if ( !cg.scoreBoardShowing )
		CG_DrawCenterString();
}

// Perform all drawing needed to completely fill the screen
void CG_DrawActive( stereoFrame_t stereoView ) {
	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournament scoreboard instead
	if ( cg.snap->ps.persistent[PERS_TEAM] == TEAM_SPECTATOR && (cg.snap->ps.pm_flags & PMF_SCOREBOARD) ) {
	//	CG_DrawTourneyScoreboard();
		return;
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	if ( stereoView != STEREO_CENTER )
		CG_DrawCrosshair3D();

	// draw 3D view
	trap->R_RenderScene( &cg.refdef );

	// draw status bar and other floating elements
 	CG_Draw2D( stereoView );
}
