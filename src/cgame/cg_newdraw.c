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

#include "cg_local.h"
#include "../ui/ui_shared.h"

extern displayContextDef_t cgDC;


// set in CG_ParseTeamInfo

//static int sortedTeamPlayers[TEAM_MAXOVERLAY];
//static int numSortedTeamPlayers;

//static char systemChat[256];
//static char teamChat1[256];
//static char teamChat2[256];

void CG_InitTeamChat(void) {
	memset(teamChat1, 0, sizeof(teamChat1));
	memset(teamChat2, 0, sizeof(teamChat2));
	memset(systemChat, 0, sizeof(systemChat));
}

void CG_SetPrintString(int type, const char *p) {
	if (type == SYSTEM_PRINT) {
		strcpy(systemChat, p);
	} else {
		strcpy(teamChat2, teamChat1);
		strcpy(teamChat1, p);
	}
}

void CG_CheckOrderPending(void) {
	if (cgs.gametype < GT_CTF) {
		return;
	}
	if (cgs.orderPending) {
		//clientInfo_t *ci = cgs.clientinfo + sortedTeamPlayers[cg_currentSelectedPlayer.integer];
		const char *p1, *p2, *b;
		p1 = p2 = b = NULL;
		switch (cgs.currentOrder) {
		case TEAMTASK_OFFENSE:
			p1 = VOICECHAT_ONOFFENSE;
			p2 = VOICECHAT_OFFENSE;
			b = "+button7; wait; -button7";
			break;
		case TEAMTASK_DEFENSE:
			p1 = VOICECHAT_ONDEFENSE;
			p2 = VOICECHAT_DEFEND;
			b = "+button8; wait; -button8";
			break;					
		case TEAMTASK_PATROL:
			p1 = VOICECHAT_ONPATROL;
			p2 = VOICECHAT_PATROL;
			b = "+button9; wait; -button9";
			break;
		case TEAMTASK_FOLLOW: 
			p1 = VOICECHAT_ONFOLLOW;
			p2 = VOICECHAT_FOLLOWME;
			b = "+button10; wait; -button10";
			break;
		case TEAMTASK_CAMP:
			p1 = VOICECHAT_ONCAMPING;
			p2 = VOICECHAT_CAMP;
			break;
		case TEAMTASK_RETRIEVE:
			p1 = VOICECHAT_ONGETFLAG;
			p2 = VOICECHAT_RETURNFLAG;
			break;
		case TEAMTASK_ESCORT:
			p1 = VOICECHAT_ONFOLLOWCARRIER;
			p2 = VOICECHAT_FOLLOWFLAGCARRIER;
			break;
		}

		if (cg_currentSelectedPlayer.integer == numSortedTeamPlayers) {
			// to everyone
			trap->SendConsoleCommand(va("cmd vsay_team %s\n", p2));
		} else {
			// for the player self
			if (sortedTeamPlayers[cg_currentSelectedPlayer.integer] == cg.snap->ps.clientNum && p1) {
				trap->SendConsoleCommand(va("teamtask %i\n", cgs.currentOrder));
				//trap->SendConsoleCommand(va("cmd say_team %s\n", p2));
				trap->SendConsoleCommand(va("cmd vsay_team %s\n", p1));
			} else if (p2) {
				//trap->SendConsoleCommand(va("cmd say_team %s, %s\n", ci->name,p));
				trap->SendConsoleCommand(va("cmd vtell %d %s\n", sortedTeamPlayers[cg_currentSelectedPlayer.integer], p2));
			}
		}
		if (b) {
			trap->SendConsoleCommand(b);
		}
		cgs.orderPending = qfalse;
	}
}

static void CG_SetSelectedPlayerName( void ) {
	if (cg_currentSelectedPlayer.integer >= 0 && cg_currentSelectedPlayer.integer < numSortedTeamPlayers) {
		clientInfo_t *ci = cgs.clientinfo + sortedTeamPlayers[cg_currentSelectedPlayer.integer];
		if (ci) {
			trap->Cvar_Set("cg_selectedPlayerName", ci->name);
			trap->Cvar_Set("cg_selectedPlayer", va("%d", sortedTeamPlayers[cg_currentSelectedPlayer.integer]));
			cgs.currentOrder = ci->teamTask;
		}
	} else {
		trap->Cvar_Set("cg_selectedPlayerName", "Everyone");
	}
}
int CG_GetSelectedPlayer( void ) {
	if (cg_currentSelectedPlayer.integer < 0 || cg_currentSelectedPlayer.integer >= numSortedTeamPlayers) {
		cg_currentSelectedPlayer.integer = 0;
	}
	return cg_currentSelectedPlayer.integer;
}

void CG_SelectNextPlayer( void ) {
	CG_CheckOrderPending();
	if (cg_currentSelectedPlayer.integer >= 0 && cg_currentSelectedPlayer.integer < numSortedTeamPlayers) {
		cg_currentSelectedPlayer.integer++;
	} else {
		cg_currentSelectedPlayer.integer = 0;
	}
	CG_SetSelectedPlayerName();
}

void CG_SelectPrevPlayer( void ) {
	CG_CheckOrderPending();
	if (cg_currentSelectedPlayer.integer > 0 && cg_currentSelectedPlayer.integer < numSortedTeamPlayers) {
		cg_currentSelectedPlayer.integer--;
	} else {
		cg_currentSelectedPlayer.integer = numSortedTeamPlayers;
	}
	CG_SetSelectedPlayerName();
}


static void CG_DrawPlayerArmorIcon( rectDef_t *rect, qboolean draw2D ) {
	vector3		angles;
	vector3		origin;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	if ( draw2D || ( !cg_draw3dIcons.integer && cg_drawIcons.integer) ) {
		CG_DrawPic( rect->x, rect->y + rect->h/2 + 1, rect->w, rect->h, cgs.media.armorIcon );
	} else if (cg_draw3dIcons.integer) {
		VectorClear( &angles );
		origin.x = 90;
		origin.y = 0;
		origin.z = -10;
		angles.yaw = ( cg.time & 2047 ) * 360 / 2048.0f;
		CG_Draw3DModel( rect->x, rect->y, rect->w, rect->h, cgs.media.armorModel, 0, &origin, &angles );
	}
}

static void CG_DrawPlayerArmorValue(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle) {
	char	num[16];
	int		value;
	playerState_t	*ps;

	ps = &cg.snap->ps;

	value = ps->stats[STAT_ARMOR];

	if (shader) {
		trap->R_SetColor( color );
		CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
		trap->R_SetColor( NULL );
	} else {
		Com_sprintf (num, sizeof(num), "%i", value);
		value = CG_Text_Width(num, scale, 0);
		CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
	}
}

static void CG_DrawPlayerAmmoIcon( rectDef_t *rect, qboolean draw2D ) {
	centity_t	*cent;
	vector3		angles;
	vector3		origin;

	cent = &cg_entities[cg.snap->ps.clientNum];

	if ( draw2D || (!cg_draw3dIcons.integer && cg_drawIcons.integer) ) {
		qhandle_t	icon;
		icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
		if ( icon ) {
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, icon );
		}
	} else if (cg_draw3dIcons.integer) {
		if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
			VectorClear( &angles );
			origin.x = 70;
			origin.y = 0;
			origin.z = 0;
			angles.yaw = 90 + 20 * sinf( cg.time / 1000.0f );
			CG_Draw3DModel( rect->x, rect->y, rect->w, rect->h, cg_weapons[ cent->currentState.weapon ].ammoModel, 0, &origin, &angles );
		}
	}
}

static void CG_DrawPlayerAmmoValue(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle) {
	char	num[16];
	int value;
	centity_t	*cent;
	playerState_t	*ps;

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	if ( cent->currentState.weapon ) {
		value = ps->ammo[cent->currentState.weapon];
		if ( value > -1 ) {
			if (shader) {
				trap->R_SetColor( color );
				CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
				trap->R_SetColor( NULL );
			} else {
				Com_sprintf (num, sizeof(num), "%i", value);
				value = CG_Text_Width(num, scale, 0);
				CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
			}
		}
	}

}



static void CG_DrawPlayerHead(rectDef_t *rect, qboolean draw2D) {
	vector3		angles;
	float		size, stretch;
	float		frac;
	float		x = rect->x;

	VectorClear( &angles );

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)(cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = rect->w * 1.25f * ( 1.5f - frac * 0.5f );

		stretch = size - rect->w * 1.25f;
		// kick in the direction of damage
		x -= stretch * 0.5f + cg.damageX * stretch * 0.5f;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cosf( crandom()*M_PI );
		cg.headEndPitch = 5 * cosf( crandom()*M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + (int)(random() * 2000);
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + (int)(random() * 2000);

			cg.headEndYaw = 180 + 20 * cosf( crandom()*M_PI );
			cg.headEndPitch = 5 * cosf( crandom()*M_PI );
		}

		size = rect->w * 1.25f;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles.yaw = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles.pitch = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

	CG_DrawHead( x, rect->y, rect->w, rect->h, cg.snap->ps.clientNum, &angles );
}

static void CG_DrawSelectedPlayerHealth( rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle ) {
	clientInfo_t *ci;
	int value;
	char num[16];

	ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if (ci) {
		if (shader) {
			trap->R_SetColor( color );
			CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
			trap->R_SetColor( NULL );
		} else {
			Com_sprintf (num, sizeof(num), "%i", ci->health);
			value = CG_Text_Width(num, scale, 0);
			CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
		}
	}
}

static void CG_DrawSelectedPlayerArmor( rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle ) {
	clientInfo_t *ci;
	int value;
	char num[16];
	ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if (ci) {
		if (ci->armor > 0) {
			if (shader) {
				trap->R_SetColor( color );
				CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
				trap->R_SetColor( NULL );
			} else {
				Com_sprintf (num, sizeof(num), "%i", ci->armor);
				value = CG_Text_Width(num, scale, 0);
				CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
			}
		}
	}
}

qhandle_t CG_StatusHandle(int task) {
	qhandle_t h;
	switch (task) {
	case TEAMTASK_OFFENSE :
		h = cgs.media.assaultShader;
		break;
	case TEAMTASK_DEFENSE :
		h = cgs.media.defendShader;
		break;
	case TEAMTASK_PATROL :
		h = cgs.media.patrolShader;
		break;
	case TEAMTASK_FOLLOW :
		h = cgs.media.followShader;
		break;
	case TEAMTASK_CAMP :
		h = cgs.media.campShader;
		break;
	case TEAMTASK_RETRIEVE :
		h = cgs.media.retrieveShader; 
		break;
	case TEAMTASK_ESCORT :
		h = cgs.media.escortShader; 
		break;
	default : 
		h = cgs.media.assaultShader;
		break;
	}
	return h;
}

static void CG_DrawSelectedPlayerStatus( rectDef_t *rect ) {
	clientInfo_t *ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if (ci) {
		qhandle_t h;
		if (cgs.orderPending) {
			// blink the icon
			if ( cg.time > cgs.orderTime - 2500 && (cg.time >> 9 ) & 1 ) {
				return;
			}
			h = CG_StatusHandle(cgs.currentOrder);
		}	else {
			h = CG_StatusHandle(ci->teamTask);
		}
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, h );
	}
}


static void CG_DrawPlayerStatus( rectDef_t *rect ) {
	clientInfo_t *ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	if (ci) {
		qhandle_t h = CG_StatusHandle(ci->teamTask);
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, h);
	}
}


static void CG_DrawSelectedPlayerName( rectDef_t *rect, float scale, vector4 *color, qboolean voice, int textStyle) {
	clientInfo_t *ci;
	ci = cgs.clientinfo + ((voice) ? cgs.currentVoiceClient : sortedTeamPlayers[CG_GetSelectedPlayer()]);
	if (ci) {
		CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, ci->name, 0, 0, textStyle);
	}
}

static void CG_DrawSelectedPlayerLocation( rectDef_t *rect, float scale, vector4 *color, int textStyle ) {
	clientInfo_t *ci;
	ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if (ci) {
		const char *p = CG_ConfigString(CS_LOCATIONS + ci->location);
		if (!p || !*p) {
			p = "unknown";
		}
		CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, p, 0, 0, textStyle);
	}
}

static void CG_DrawPlayerLocation( rectDef_t *rect, float scale, vector4 *color, int textStyle  ) {
	clientInfo_t *ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	if (ci) {
		const char *p = CG_ConfigString(CS_LOCATIONS + ci->location);
		if (!p || !*p) {
			p = "unknown";
		}
		CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, p, 0, 0, textStyle);
	}
}



static void CG_DrawSelectedPlayerWeapon( rectDef_t *rect ) {
	clientInfo_t *ci;

	ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if (ci) {
		if ( cg_weapons[ci->curWeapon].weaponIcon ) {
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_weapons[ci->curWeapon].weaponIcon );
		} else {
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.deferShader);
		}
	}
}

static void CG_DrawPlayerScore( rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle ) {
	char num[16];
	int value = cg.snap->ps.persistant[PERS_SCORE];

	if (shader) {
		trap->R_SetColor( color );
		CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
		trap->R_SetColor( NULL );
	} else {
		Com_sprintf (num, sizeof(num), "%i", value);
		value = CG_Text_Width(num, scale, 0);
		CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
	}
}

static void CG_DrawPlayerItem( rectDef_t *rect, float scale, qboolean draw2D) {
	int		value;
	vector3 origin, angles;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if ( value ) {
		CG_RegisterItemVisuals( value );

		if (qtrue) {
			CG_RegisterItemVisuals( value );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[ value ].icon );
		} else {
			VectorClear( &angles );
			origin.x = 90;
			origin.y = 0;
			origin.z = -10;
			angles.yaw = ( cg.time & 2047 ) * 360 / 2048.0f;
			CG_Draw3DModel(rect->x, rect->y, rect->w, rect->h, cg_items[ value ].models[0], 0, &origin, &angles );
		}
	}

}


static void CG_DrawSelectedPlayerPowerup( rectDef_t *rect, qboolean draw2D ) {
	clientInfo_t *ci;
	int j;
	float x, y;

	ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if (ci) {
		x = rect->x;
		y = rect->y;

		for (j = 0; j < PW_NUM_POWERUPS; j++) {
			if (ci->powerups & (1 << j)) {
				gitem_t	*item;
				item = BG_FindItemForPowerup( j );
				if (item) {
					CG_DrawPic( x, y, rect->w, rect->h, trap->R_RegisterShader( item->icon ) );
					x += 3;
					y += 3;
					return;
				}
			}
		}

	}
}


static void CG_DrawSelectedPlayerHead( rectDef_t *rect, qboolean draw2D, qboolean voice ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vector3			origin;
	vector3			mins, maxs, angles;


	ci = cgs.clientinfo + ((voice) ? cgs.currentVoiceClient : sortedTeamPlayers[CG_GetSelectedPlayer()]);

	if (ci) {
		if ( cg_draw3dIcons.integer ) {
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

			angles.pitch = 0;
			angles.yaw = 180;
			angles.roll = 0;

			CG_Draw3DModel( rect->x, rect->y, rect->w, rect->h, ci->headModel, ci->headSkin, &origin, &angles );
		} else if ( cg_drawIcons.integer ) {
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, ci->modelIcon );
		}

		// if they are deferred, draw a cross out
		if ( ci->deferred ) {
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.deferShader );
		}
	}

}


static void CG_DrawPlayerHealth(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle ) {
	playerState_t	*ps;
	int value;
	char	num[16];

	ps = &cg.snap->ps;

	value = ps->stats[STAT_HEALTH];

	if (shader) {
		trap->R_SetColor( color );
		CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
		trap->R_SetColor( NULL );
	} else {
		Com_sprintf (num, sizeof(num), "%i", value);
		value = CG_Text_Width(num, scale, 0);
		CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
	}
}


static void CG_DrawRedScore(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle ) {
	int value;
	char num[16];
	if ( cgs.scores1 == SCORE_NOT_PRESENT ) {
		Com_sprintf (num, sizeof(num), "-");
	}
	else {
		Com_sprintf (num, sizeof(num), "%i", cgs.scores1);
	}
	value = CG_Text_Width(num, scale, 0);
	CG_Text_Paint(rect->x + rect->w - value, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
}

static void CG_DrawBlueScore(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle ) {
	int value;
	char num[16];

	if ( cgs.scores2 == SCORE_NOT_PRESENT ) {
		Com_sprintf (num, sizeof(num), "-");
	}
	else {
		Com_sprintf (num, sizeof(num), "%i", cgs.scores2);
	}
	value = CG_Text_Width(num, scale, 0);
	CG_Text_Paint(rect->x + rect->w - value, rect->y + rect->h, scale, color, num, 0, 0, textStyle);
}

//QTZTODO: team name support? set by server =]
static void CG_DrawRedName(rectDef_t *rect, float scale, vector4 *color, int textStyle ) {
	//	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, cg_redTeamName.string , 0, 0, textStyle);
}

static void CG_DrawBlueName(rectDef_t *rect, float scale, vector4 *color, int textStyle ) {
	//	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, cg_blueTeamName.string, 0, 0, textStyle);
}

static void CG_DrawBlueFlagName(rectDef_t *rect, float scale, vector4 *color, int textStyle ) {
	int i;
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_RED  && cgs.clientinfo[i].powerups & ( 1<< PW_BLUEFLAG )) {
			CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, cgs.clientinfo[i].name, 0, 0, textStyle);
			return;
		}
	}
}

static void CG_DrawBlueFlagStatus(rectDef_t *rect, qhandle_t shader) {
	if (cgs.gametype != GT_CTF && cgs.gametype != GT_1FCTF) {
		return;
	}
	if (shader) {
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	} else {
		gitem_t *item = BG_FindItemForPowerup( PW_BLUEFLAG );
		if (item) {
			vector4 color = {0, 0, 1, 1};
			trap->R_SetColor(&color);
			if( cgs.blueflag >= 0 && cgs.blueflag <= 2 ) {
				CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[cgs.blueflag] );
			} else {
				CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[0] );
			}
			trap->R_SetColor(NULL);
		}
	}
}

static void CG_DrawBlueFlagHead(rectDef_t *rect) {
	int i;
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_RED  && cgs.clientinfo[i].powerups & ( 1<< PW_BLUEFLAG )) {
			vector3 angles;
			VectorClear( &angles );
			angles.yaw = 180 + 20 * sinf( cg.time / 650.0f );
			CG_DrawHead( rect->x, rect->y, rect->w, rect->h, 0, &angles );
			return;
		}
	}
}

static void CG_DrawRedFlagName(rectDef_t *rect, float scale, vector4 *color, int textStyle ) {
	int i;
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_BLUE  && cgs.clientinfo[i].powerups & ( 1<< PW_REDFLAG )) {
			CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, cgs.clientinfo[i].name, 0, 0, textStyle);
			return;
		}
	}
}

static void CG_DrawRedFlagStatus(rectDef_t *rect, qhandle_t shader) {
	if (cgs.gametype != GT_CTF && cgs.gametype != GT_1FCTF) {
		return;
	}
	if (shader) {
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	} else {
		gitem_t *item = BG_FindItemForPowerup( PW_REDFLAG );
		if (item) {
			vector4 color = {1, 0, 0, 1};
			trap->R_SetColor(&color);
			if( cgs.redflag >= 0 && cgs.redflag <= 2) {
				CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[cgs.redflag] );
			} else {
				CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[0] );
			}
			trap->R_SetColor(NULL);
		}
	}
}

static void CG_DrawRedFlagHead(rectDef_t *rect) {
	int i;
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_BLUE  && cgs.clientinfo[i].powerups & ( 1<< PW_REDFLAG )) {
			vector3 angles;
			VectorClear( &angles );
			angles.yaw = 180 + 20 * sinf( cg.time / 650.0f );
			CG_DrawHead( rect->x, rect->y, rect->w, rect->h, 0, &angles );
			return;
		}
	}
}

static void CG_OneFlagStatus(rectDef_t *rect) {
	if (cgs.gametype != GT_1FCTF) {
		return;
	} else {
		gitem_t *item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		if (item) {
			if( cgs.flagStatus >= 0 && cgs.flagStatus <= 4 ) {
				vector4 color = {1, 1, 1, 1};
				int index = 0;
				if (cgs.flagStatus == FLAG_TAKEN_RED) {
					color.g = color.b = 0;
					index = 1;
				} else if (cgs.flagStatus == FLAG_TAKEN_BLUE) {
					color.r = color.g = 0;
					index = 1;
				} else if (cgs.flagStatus == FLAG_DROPPED) {
					index = 2;
				}
				trap->R_SetColor(&color);
				CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[index] );
			}
		}
	}
}


static void CG_DrawCTFPowerUp(rectDef_t *rect) {
	int		value;

	if (cgs.gametype < GT_CTF) {
		return;
	}
	value = cg.snap->ps.stats[STAT_PERSISTANT_POWERUP];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[ value ].icon );
	}
}



static void CG_DrawTeamColor(rectDef_t *rect, vector4 *color) {
	CG_DrawTeamBackground( (int)rect->x, (int)rect->y, (int)rect->w, (int)rect->h, color->a, cg.snap->ps.persistant[PERS_TEAM] );
}

static void CG_DrawAreaPowerUp(rectDef_t *rect, int align, float special, float scale, vector4 *color) {
	char num[16];
	int sorted[MAX_POWERUPS], sortedTime[MAX_POWERUPS];
	int i, j, k, t, active;
	float f, *inc;
	playerState_t *ps;
	gitem_t	*item;
	rectDef_t r2;

	r2.x = rect->x;
	r2.y = rect->y;
	r2.w = rect->w;
	r2.h = rect->h;

	inc = (align == HUD_VERTICAL) ? &r2.y : &r2.x;

	ps = &cg.snap->ps;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}
		t = ps->powerups[ i ] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if ( t <= 0 || t >= 999000) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_FindItemForPowerup( sorted[i] );

		if (item) {
			t = ps->powerups[ sorted[i] ];
			if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
				trap->R_SetColor( NULL );
			} else {
				vector4	modulate;

				f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate.r = modulate.g = modulate.b = modulate.a = f;
				trap->R_SetColor( &modulate );
			}

			CG_DrawPic( r2.x, r2.y, r2.w * .75f, r2.h, trap->R_RegisterShader( item->icon ) );

			Com_sprintf (num, sizeof(num), "%i", sortedTime[i] / 1000);
			CG_Text_Paint(r2.x + (r2.w * .75f) + 3 , r2.y + r2.h, scale, color, num, 0, 0, 0);
			*inc += r2.w + special;
		}

	}
	trap->R_SetColor( NULL );

}

float CG_GetValue(int ownerDraw) {
	centity_t	*cent;
	clientInfo_t *ci;
	playerState_t	*ps;

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	switch (ownerDraw) {
	case CG_SELECTEDPLAYER_ARMOR:
		ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
		return (float)ci->armor;
		break;
	case CG_SELECTEDPLAYER_HEALTH:
		ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
		return (float)ci->health;
		break;
	case CG_PLAYER_ARMOR_VALUE:
		return (float)ps->stats[STAT_ARMOR];
		break;
	case CG_PLAYER_AMMO_VALUE:
		if ( cent->currentState.weapon ) {
			return (float)ps->ammo[cent->currentState.weapon];
		}
		break;
	case CG_PLAYER_SCORE:
		return (float)cg.snap->ps.persistant[PERS_SCORE];
		break;
	case CG_PLAYER_HEALTH:
		return (float)ps->stats[STAT_HEALTH];
		break;
	case CG_RED_SCORE:
		return (float)cgs.scores1;
		break;
	case CG_BLUE_SCORE:
		return (float)cgs.scores2;
		break;
	default:
		break;
	}
	return -1;
}

qboolean CG_OtherTeamHasFlag(void) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF) {
		int team = cg.snap->ps.persistant[PERS_TEAM];
		if (cgs.gametype == GT_1FCTF) {
			if (team == TEAM_RED && cgs.flagStatus == FLAG_TAKEN_BLUE) {
				return qtrue;
			} else if (team == TEAM_BLUE && cgs.flagStatus == FLAG_TAKEN_RED) {
				return qtrue;
			} else {
				return qfalse;
			}
		} else {
			if (team == TEAM_RED && cgs.redflag == FLAG_TAKEN) {
				return qtrue;
			} else if (team == TEAM_BLUE && cgs.blueflag == FLAG_TAKEN) {
				return qtrue;
			} else {
				return qfalse;
			}
		}
	}
	return qfalse;
}

qboolean CG_YourTeamHasFlag(void) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF) {
		int team = cg.snap->ps.persistant[PERS_TEAM];
		if (cgs.gametype == GT_1FCTF) {
			if (team == TEAM_RED && cgs.flagStatus == FLAG_TAKEN_RED) {
				return qtrue;
			} else if (team == TEAM_BLUE && cgs.flagStatus == FLAG_TAKEN_BLUE) {
				return qtrue;
			} else {
				return qfalse;
			}
		} else {
			if (team == TEAM_RED && cgs.blueflag == FLAG_TAKEN) {
				return qtrue;
			} else if (team == TEAM_BLUE && cgs.redflag == FLAG_TAKEN) {
				return qtrue;
			} else {
				return qfalse;
			}
		}
	}
	return qfalse;
}

// THINKABOUTME: should these be exclusive or inclusive.. 
// 
qboolean CG_OwnerDrawVisible(int flags) {

	if (flags & CG_SHOW_TEAMINFO) {
		return (cg_currentSelectedPlayer.integer == numSortedTeamPlayers);
	}

	if (flags & CG_SHOW_NOTEAMINFO) {
		return !(cg_currentSelectedPlayer.integer == numSortedTeamPlayers);
	}

	if (flags & CG_SHOW_OTHERTEAMHASFLAG) {
		return CG_OtherTeamHasFlag();
	}

	if (flags & CG_SHOW_YOURTEAMHASENEMYFLAG) {
		return CG_YourTeamHasFlag();
	}

	if (flags & (CG_SHOW_BLUE_TEAM_HAS_REDFLAG | CG_SHOW_RED_TEAM_HAS_BLUEFLAG)) {
		if (flags & CG_SHOW_BLUE_TEAM_HAS_REDFLAG && (cgs.redflag == FLAG_TAKEN || cgs.flagStatus == FLAG_TAKEN_RED)) {
			return qtrue;
		} else if (flags & CG_SHOW_RED_TEAM_HAS_BLUEFLAG && (cgs.blueflag == FLAG_TAKEN || cgs.flagStatus == FLAG_TAKEN_BLUE)) {
			return qtrue;
		}
		return qfalse;
	}

	if (flags & CG_SHOW_ANYTEAMGAME) {
		if( cgs.gametype >= GT_TEAM) {
			return qtrue;
		}
	}

	if (flags & CG_SHOW_ANYNONTEAMGAME) {
		if( cgs.gametype < GT_TEAM) {
			return qtrue;
		}
	}

	if (flags & CG_SHOW_ONEFLAG) {
		if( cgs.gametype == GT_1FCTF ) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if (flags & CG_SHOW_CTF) {
		if( cgs.gametype == GT_CTF ) {
			return qtrue;
		}
	}

	if (flags & CG_SHOW_HEALTHCRITICAL) {
		if (cg.snap->ps.stats[STAT_HEALTH] < 25) {
			return qtrue;
		}
	}

	if (flags & CG_SHOW_HEALTHOK) {
		if (cg.snap->ps.stats[STAT_HEALTH] >= 25) {
			return qtrue;
		}
	}

	if (flags & CG_SHOW_TOURNAMENT) {
		if( cgs.gametype == GT_DUEL ) {
			return qtrue;
		}
	}

	if (flags & CG_SHOW_DURINGINCOMINGVOICE) {
	}

	if (flags & CG_SHOW_IF_PLAYER_HAS_FLAG) {
		if (cg.snap->ps.powerups[PW_REDFLAG] || cg.snap->ps.powerups[PW_BLUEFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG]) {
			return qtrue;
		}
	}
	return qfalse;
}



static void CG_DrawPlayerHasFlag(rectDef_t *rect, qboolean force2D) {
	int adj = (force2D) ? 0 : 2;
	if( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
		CG_DrawFlagModel( rect->x + adj, rect->y + adj, rect->w - adj, rect->h - adj, TEAM_RED, force2D);
	} else if( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		CG_DrawFlagModel( rect->x + adj, rect->y + adj, rect->w - adj, rect->h - adj, TEAM_BLUE, force2D);
	} else if( cg.predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		CG_DrawFlagModel( rect->x + adj, rect->y + adj, rect->w - adj, rect->h - adj, TEAM_FREE, force2D);
	}
}

static void CG_DrawAreaSystemChat(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader) {
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, systemChat, 0, 0, 0);
}

static void CG_DrawAreaTeamChat(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader) {
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color,teamChat1, 0, 0, 0);
}

static void CG_DrawAreaChat(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader) {
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, teamChat2, 0, 0, 0);
}

const char *CG_GetKillerText(void) {
	const char *s = "";
	if ( cg.killerName[0] ) {
		s = va("Fragged by %s", cg.killerName );
	}
	return s;
}


static void CG_DrawKiller(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle ) {
	// fragged by ... line
	if ( cg.killerName[0] ) {
		float x = rect->x + rect->w / 2;
		CG_Text_Paint(x - CG_Text_Width(CG_GetKillerText(), scale, 0) / 2, rect->y + rect->h, scale, color, CG_GetKillerText(), 0, 0, textStyle);
	}

}


static void CG_DrawCapFragLimit(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle) {
	int limit = (cgs.gametype >= GT_CTF) ? cgs.capturelimit : cgs.fraglimit;
	CG_Text_Paint(rect->x, rect->y, scale, color, va("%2i", limit),0, 0, textStyle); 
}

static void CG_Draw1stPlace(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle) {
	if (cgs.scores1 != SCORE_NOT_PRESENT) {
		CG_Text_Paint(rect->x, rect->y, scale, color, va("%2i", cgs.scores1),0, 0, textStyle); 
	}
}

static void CG_Draw2ndPlace(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle) {
	if (cgs.scores2 != SCORE_NOT_PRESENT) {
		CG_Text_Paint(rect->x, rect->y, scale, color, va("%2i", cgs.scores2),0, 0, textStyle); 
	}
}

const char *CG_GetGameStatusText(void) {
	const char *s = "";
	if ( cgs.gametype < GT_TEAM) {
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			s = va("%s place with %i",CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),cg.snap->ps.persistant[PERS_SCORE] );
		}
	} else {
		if ( cg.teamScores[0] == cg.teamScores[1] ) {
			s = va("Teams are tied at %i", cg.teamScores[0] );
		} else if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			s = va("Red leads Blue, %i to %i", cg.teamScores[0], cg.teamScores[1] );
		} else {
			s = va("Blue leads Red, %i to %i", cg.teamScores[1], cg.teamScores[0] );
		}
	}
	return s;
}

static void CG_DrawGameStatus(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle ) {
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, CG_GetGameStatusText(), 0, 0, textStyle);
}

static void CG_DrawGameType(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle ) {
	CG_Text_Paint(rect->x, rect->y + rect->h, scale, color, BG_GetGametypeString( cgs.gametype ), 0, 0, textStyle);
}

static void CG_Text_Paint_Limit(float *maxX, float x, float y, float scale, vector4 *color, const char* text, float adjust, int limit) {
	int len, count;
	vector4 newColor;
	glyphInfo_t *glyph;
	if (text) {
		const char *s = text;
		float max = *maxX;
		float useScale;
		fontInfo_t *font = &cgDC.Assets.textFont;
		if (scale <= ui_smallFont.value) {
			font = &cgDC.Assets.smallFont;
		} else if (scale > ui_bigFont.value) {
			font = &cgDC.Assets.bigFont;
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
				if (CG_Text_Width(s, useScale, 1) + x > max) {
					*maxX = 0;
					break;
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
				x += (glyph->xSkip * useScale) + adjust;
				*maxX = x;
				count++;
				s++;
			}
		}
		trap->R_SetColor( NULL );
	}

}



#define PIC_WIDTH 12

void CG_DrawNewTeamInfo(rectDef_t *rect, float text_x, float text_y, float scale, vector4 *color, qhandle_t shader) {
	int xx;
	float y;
	int i, j, len, count;
	const char *p;
	vector4		hcolor;
	float pwidth, lwidth, maxx, leftOver;
	clientInfo_t *ci;
	gitem_t	*item;
	qhandle_t h;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			len = CG_Text_Width( ci->name, scale, 0);
			if (len > pwidth)
				pwidth = (float)len;
		}
	}

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_Text_Width(p, scale, 0);
			if (len > lwidth)
				lwidth = (float)len;
		}
	}

	y = rect->y;

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			xx = (int)rect->x + 1;
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( (float)xx, y, PIC_WIDTH, PIC_WIDTH, trap->R_RegisterShader( item->icon ) );
						xx += PIC_WIDTH;
					}
				}
			}

			// FIXME: max of 3 powerups shown properly
			xx = (int)rect->x + (PIC_WIDTH * 3) + 2;

			CG_GetColorForHealth( ci->health, ci->armor, &hcolor );
			trap->R_SetColor(&hcolor);
			CG_DrawPic( (float)xx, y + 1, PIC_WIDTH - 2, PIC_WIDTH - 2, cgs.media.heartShader );

			//Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);
			//CG_Text_Paint(xx, y + text_y, scale, hcolor, st, 0, 0); 

			// draw weapon icon
			xx += PIC_WIDTH + 1;

			// weapon used is not that useful, use the space for task
#if 0
			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, PIC_WIDTH, PIC_WIDTH, cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx, y, PIC_WIDTH, PIC_WIDTH, cgs.media.deferShader );
			}
#endif

			trap->R_SetColor(NULL);
			if (cgs.orderPending) {
				// blink the icon
				if ( cg.time > cgs.orderTime - 2500 && (cg.time >> 9 ) & 1 ) {
					h = 0;
				} else {
					h = CG_StatusHandle(cgs.currentOrder);
				}
			}	else {
				h = CG_StatusHandle(ci->teamTask);
			}

			if (h) {
				CG_DrawPic( (float)xx, y, PIC_WIDTH, PIC_WIDTH, h);
			}

			xx += PIC_WIDTH + 1;

			leftOver = rect->w - xx;
			maxx = xx + leftOver / 3;



			CG_Text_Paint_Limit(&maxx, (float)xx, y + text_y, scale, color, ci->name, 0, 0); 

			p = CG_ConfigString(CS_LOCATIONS + ci->location);
			if (!p || !*p) {
				p = "unknown";
			}

			xx += (int)(leftOver/3 + 2);
			maxx = rect->w - 4;

			CG_Text_Paint_Limit(&maxx, (float)xx, y + text_y, scale, color, p, 0, 0); 
			y += text_y + 2;
			if ( y + text_y + 2 > rect->y + rect->h ) {
				break;
			}

		}
	}
}


void CG_DrawTeamSpectators(rectDef_t *rect, float scale, vector4 *color, qhandle_t shader) {
	if (cg.spectatorLen) {
		float maxX;

		if (cg.spectatorWidth == -1) {
			cg.spectatorWidth = 0;
			cg.spectatorPaintX = (int)rect->x + 1;
			cg.spectatorPaintX2 = -1;
		}

		if (cg.spectatorOffset > cg.spectatorLen) {
			cg.spectatorOffset = 0;
			cg.spectatorPaintX = (int)rect->x + 1;
			cg.spectatorPaintX2 = -1;
		}

		if (cg.time > cg.spectatorTime) {
			cg.spectatorTime = cg.time + 10;
			if (cg.spectatorPaintX <= rect->x + 2) {
				if (cg.spectatorOffset < cg.spectatorLen) {
					cg.spectatorPaintX += CG_Text_Width(&cg.spectatorList[cg.spectatorOffset], scale, 1) - 1;
					cg.spectatorOffset++;
				} else {
					cg.spectatorOffset = 0;
					if (cg.spectatorPaintX2 >= 0) {
						cg.spectatorPaintX = cg.spectatorPaintX2;
					} else {
						cg.spectatorPaintX = (int)rect->x + (int)rect->w - 2;
					}
					cg.spectatorPaintX2 = -1;
				}
			} else {
				cg.spectatorPaintX--;
				if (cg.spectatorPaintX2 >= 0) {
					cg.spectatorPaintX2--;
				}
			}
		}

		maxX = rect->x + rect->w - 2;
		CG_Text_Paint_Limit(&maxX, (float)cg.spectatorPaintX, rect->y + rect->h - 3, scale, color, &cg.spectatorList[cg.spectatorOffset], 0, 0); 
		if (cg.spectatorPaintX2 >= 0) {
			float maxX2 = rect->x + rect->w - 2;
			CG_Text_Paint_Limit(&maxX2, (float)cg.spectatorPaintX2, rect->y + rect->h - 3, scale, color, cg.spectatorList, 0, cg.spectatorOffset); 
		}
		if (cg.spectatorOffset && maxX > 0) {
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if (cg.spectatorPaintX2 == -1) {
				cg.spectatorPaintX2 = (int)(rect->x + rect->w - 2);
			}
		} else {
			cg.spectatorPaintX2 = -1;
		}

	}
}



void CG_DrawMedal(int ownerDraw, rectDef_t *rect, float scale, vector4 *color, qhandle_t shader) {
	score_t *score = &cg.scores[cg.selectedScore];
	float value = 0;
	char *text = NULL;
	color->a = 0.25f;

	switch (ownerDraw) {
	case CG_ACCURACY:
		value = (float)score->accuracy;
		break;
	case CG_ASSISTS:
		value = (float)score->assistCount;
		break;
	case CG_DEFEND:
		value = (float)score->defendCount;
		break;
	case CG_EXCELLENT:
		value = (float)score->excellentCount;
		break;
	case CG_IMPRESSIVE:
		value = (float)score->impressiveCount;
		break;
	case CG_PERFECT:
		value = (float)score->perfect;
		break;
	case CG_GAUNTLET:
		value = (float)score->guantletCount;
		break;
	case CG_CAPTURES:
		value = (float)score->captures;
		break;
	}

	if (value > 0) {
		if (ownerDraw != CG_PERFECT) {
			if (ownerDraw == CG_ACCURACY) {
				text = va("%i%%", (int)value);
				if (value > 50) {
					color->a = 1.0f;
				}
			} else {
				text = va("%i", (int)value);
				color->a = 1.0f;
			}
		} else {
			if (value) {
				color->a = 1.0f;
			}
			text = "Wow";
		}
	}

	trap->R_SetColor(color);
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );

	if (text) {
		color->a = 1.0f;
		value = (float)CG_Text_Width(text, scale, 0);
		CG_Text_Paint(rect->x + (rect->w - value) / 2, rect->y + rect->h + 10 , scale, color, text, 0, 0, 0);
	}
	trap->R_SetColor(NULL);

}

//QtZ: New HUD elements
#define IDEAL_FPS (60.0f)
#define	FPS_FRAMES	16
#define FPS_MASK (FPS_FRAMES-1)

static void CG_DrawFPSInfo( rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle ) {
	static unsigned short previousTimes[FPS_FRAMES];
	static unsigned short index;
	static int	previous, lastupdate;
	int		t, i, total;
	unsigned short frameTime;
	vector4 fpsColour = { 1.0f, 1.0f, 1.0f, 1.0f }, fpsGood = { 0.0f, 1.0f, 0.0f, 1.0f }, fpsBad = { 1.0f, 0.0f, 0.0f, 1.0f };
	float point = 1.0f;//= min( cg.snap->ping / 300.0f, 1.0f );
	float fps, maxFPS = 1000.0f/(float)atof( CG_Cvar_VariableString( "com_frametime" ) );

	if ( !cg_drawFPS.boolean && !cg_debugHUD.boolean )
		return;

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

	point = MIN( MAX(0.0f, fps) / MAX(IDEAL_FPS, maxFPS), 1.0f );
	VectorLerp4( &fpsBad, point, &fpsGood, &fpsColour );
	fpsColour.a = 1.0f;

	CG_Text_Paint( rect->x, rect->y, scale, &fpsColour, va( "%i FPS", (int)fps ), 0, 0, textStyle );
}

#define BAD_PING (250.0f)
static void CG_DrawPingInfo( rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle )
{
	vector4 pingColour = { 1.0f, 1.0f, 1.0f, 1.0f }, pingGood = { 0.0f, 1.0f, 0.0f, 1.0f }, pingBad = { 1.0f, 0.0f, 0.0f, 1.0f };
	float point = MIN( cg.snap->ping / BAD_PING, 1.0f );

	if ( !cg_drawPing.boolean && !cg_debugHUD.boolean )
		return;

	VectorLerp4( &pingGood, point, &pingBad, &pingColour );
	pingColour.a = 1.0f;

	CG_Text_Paint( rect->x, rect->y, scale, &pingColour, va( "%i ping", cg.snap->ping ), 0, 0, textStyle );
}

static void CG_DrawTimer( rectDef_t *rect, float scale, vector4 *color, qhandle_t shader, int textStyle )
{
	vector4 *timeColour = NULL;
	int msec=0, secs=0, mins=0, limitSec=cgs.timelimit*60;

	if ( !cg_drawTimer.boolean && !cg_debugHUD.boolean && !cg.intermissionStarted )
		return;

	msec = cg.time-cgs.levelStartTime;
	secs = msec/1000;
	mins = secs/60;

	timeColour = &colorWhite;
	if ( cgs.timelimit ) {
		// final minute
		if ( secs >= limitSec-60 )
			timeColour = &colorRed;
		// last quarter
		else if ( secs >= limitSec-(limitSec/4) )
			timeColour = &colorOrange;
		// half way
		else if ( secs >= limitSec/2 )
			timeColour = &colorYellow;
	}

	if ( cgs.timelimit && cg_drawTimer.integer == 2 )
	{// count down
		msec = limitSec*1000 - (msec);
		secs = msec/1000;
		mins = secs/60;
	}

	secs %= 60;
	msec %= 1000;

	//QTZTODO: Masked timer?
	CG_Text_Paint( rect->x, rect->y, scale, timeColour, va( "%i:%02i", mins, secs ), 0, 0, textStyle );
}
//~QtZ

//
void CG_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vector4 *color, qhandle_t shader, int textStyle) {
	rectDef_t rect;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	//if (ownerDrawFlags != 0 && !CG_OwnerDrawVisible(ownerDrawFlags)) {
	//	return;
	//}

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	switch (ownerDraw) {
	case CG_PLAYER_ARMOR_ICON:
		CG_DrawPlayerArmorIcon(&rect, ownerDrawFlags & CG_SHOW_2DONLY);
		break;
	case CG_PLAYER_ARMOR_ICON2D:
		CG_DrawPlayerArmorIcon(&rect, qtrue);
		break;
	case CG_PLAYER_ARMOR_VALUE:
		CG_DrawPlayerArmorValue(&rect, scale, color, shader, textStyle);
		break;
	case CG_PLAYER_AMMO_ICON:
		CG_DrawPlayerAmmoIcon(&rect, ownerDrawFlags & CG_SHOW_2DONLY);
		break;
	case CG_PLAYER_AMMO_ICON2D:
		CG_DrawPlayerAmmoIcon(&rect, qtrue);
		break;
	case CG_PLAYER_AMMO_VALUE:
		CG_DrawPlayerAmmoValue(&rect, scale, color, shader, textStyle);
		break;
	case CG_SELECTEDPLAYER_HEAD:
		CG_DrawSelectedPlayerHead(&rect, ownerDrawFlags & CG_SHOW_2DONLY, qfalse);
		break;
	case CG_VOICE_HEAD:
		CG_DrawSelectedPlayerHead(&rect, ownerDrawFlags & CG_SHOW_2DONLY, qtrue);
		break;
	case CG_VOICE_NAME:
		CG_DrawSelectedPlayerName(&rect, scale, color, qtrue, textStyle);
		break;
	case CG_SELECTEDPLAYER_STATUS:
		CG_DrawSelectedPlayerStatus(&rect);
		break;
	case CG_SELECTEDPLAYER_ARMOR:
		CG_DrawSelectedPlayerArmor(&rect, scale, color, shader, textStyle);
		break;
	case CG_SELECTEDPLAYER_HEALTH:
		CG_DrawSelectedPlayerHealth(&rect, scale, color, shader, textStyle);
		break;
	case CG_SELECTEDPLAYER_NAME:
		CG_DrawSelectedPlayerName(&rect, scale, color, qfalse, textStyle);
		break;
	case CG_SELECTEDPLAYER_LOCATION:
		CG_DrawSelectedPlayerLocation(&rect, scale, color, textStyle);
		break;
	case CG_SELECTEDPLAYER_WEAPON:
		CG_DrawSelectedPlayerWeapon(&rect);
		break;
	case CG_SELECTEDPLAYER_POWERUP:
		CG_DrawSelectedPlayerPowerup(&rect, ownerDrawFlags & CG_SHOW_2DONLY);
		break;
	case CG_PLAYER_HEAD:
		CG_DrawPlayerHead(&rect, ownerDrawFlags & CG_SHOW_2DONLY);
		break;
	case CG_PLAYER_ITEM:
		CG_DrawPlayerItem(&rect, scale, ownerDrawFlags & CG_SHOW_2DONLY);
		break;
	case CG_PLAYER_SCORE:
		CG_DrawPlayerScore(&rect, scale, color, shader, textStyle);
		break;
	case CG_PLAYER_HEALTH:
		CG_DrawPlayerHealth(&rect, scale, color, shader, textStyle);
		break;
	case CG_RED_SCORE:
		CG_DrawRedScore(&rect, scale, color, shader, textStyle);
		break;
	case CG_BLUE_SCORE:
		CG_DrawBlueScore(&rect, scale, color, shader, textStyle);
		break;
	case CG_RED_NAME:
		CG_DrawRedName(&rect, scale, color, textStyle);
		break;
	case CG_BLUE_NAME:
		CG_DrawBlueName(&rect, scale, color, textStyle);
		break;
	case CG_BLUE_FLAGHEAD:
		CG_DrawBlueFlagHead(&rect);
		break;
	case CG_BLUE_FLAGSTATUS:
		CG_DrawBlueFlagStatus(&rect, shader);
		break;
	case CG_BLUE_FLAGNAME:
		CG_DrawBlueFlagName(&rect, scale, color, textStyle);
		break;
	case CG_RED_FLAGHEAD:
		CG_DrawRedFlagHead(&rect);
		break;
	case CG_RED_FLAGSTATUS:
		CG_DrawRedFlagStatus(&rect, shader);
		break;
	case CG_RED_FLAGNAME:
		CG_DrawRedFlagName(&rect, scale, color, textStyle);
		break;
	case CG_ONEFLAG_STATUS:
		CG_OneFlagStatus(&rect);
		break;
	case CG_PLAYER_LOCATION:
		CG_DrawPlayerLocation(&rect, scale, color, textStyle);
		break;
	case CG_TEAM_COLOR:
		CG_DrawTeamColor(&rect, color);
		break;
	case CG_CTF_POWERUP:
		CG_DrawCTFPowerUp(&rect);
		break;
	case CG_AREA_POWERUP:
		CG_DrawAreaPowerUp(&rect, align, special, scale, color);
		break;
	case CG_PLAYER_STATUS:
		CG_DrawPlayerStatus(&rect);
		break;
	case CG_PLAYER_HASFLAG:
		CG_DrawPlayerHasFlag(&rect, qfalse);
		break;
	case CG_PLAYER_HASFLAG2D:
		CG_DrawPlayerHasFlag(&rect, qtrue);
		break;
	case CG_AREA_SYSTEMCHAT:
		CG_DrawAreaSystemChat(&rect, scale, color, shader);
		break;
	case CG_AREA_TEAMCHAT:
		CG_DrawAreaTeamChat(&rect, scale, color, shader);
		break;
	case CG_AREA_CHAT:
		CG_DrawAreaChat(&rect, scale, color, shader);
		break;
	case CG_GAME_TYPE:
		CG_DrawGameType(&rect, scale, color, shader, textStyle);
		break;
	case CG_GAME_STATUS:
		CG_DrawGameStatus(&rect, scale, color, shader, textStyle);
		break;
	case CG_KILLER:
		CG_DrawKiller(&rect, scale, color, shader, textStyle);
		break;
	case CG_ACCURACY:
	case CG_ASSISTS:
	case CG_DEFEND:
	case CG_EXCELLENT:
	case CG_IMPRESSIVE:
	case CG_PERFECT:
	case CG_GAUNTLET:
	case CG_CAPTURES:
		CG_DrawMedal(ownerDraw, &rect, scale, color, shader);
		break;
	case CG_SPECTATORS:
		CG_DrawTeamSpectators(&rect, scale, color, shader);
		break;
	case CG_TEAMINFO:
		if (cg_currentSelectedPlayer.integer == numSortedTeamPlayers) {
			CG_DrawNewTeamInfo(&rect, text_x, text_y, scale, color, shader);
		}
		break;

	case CG_CAPFRAGLIMIT:
		CG_DrawCapFragLimit(&rect, scale, color, shader, textStyle);
		break;

	case CG_1STPLACE:
		CG_Draw1stPlace(&rect, scale, color, shader, textStyle);
		break;

	case CG_2NDPLACE:
		CG_Draw2ndPlace(&rect, scale, color, shader, textStyle);
		break;

		//QtZ: Added
	case CG_FPS_INFO:
		CG_DrawFPSInfo( &rect, scale, color, shader, textStyle );
		break;

	case CG_PING_INFO:
		CG_DrawPingInfo( &rect, scale, color, shader, textStyle );
		break;

	case CG_OBITUARY:
		//QTZTODO: Obituary HUD
		break;

	case CG_ITEMPICKUP:
		//QTZTODO: Item pickup HUD
		break;

	case CG_TIMER:
		CG_DrawTimer( &rect, scale, color, shader, textStyle );
		break;
	default:
		break;
  }
}

void CG_MouseEvent(int x, int y) {
	int n;

	if ( (cg.predictedPlayerState.pm_type == PM_NORMAL || cg.predictedPlayerState.pm_type == PM_SPECTATOR) && cg.showScores == qfalse) {
		trap->Key_SetCatcher(0);
		return;
	}

	cgs.cursorX += x;
	if (cgs.cursorX < 0)
		cgs.cursorX = 0;
	else if (cgs.cursorX > SCREEN_WIDTH)
		cgs.cursorX = SCREEN_WIDTH;

	cgs.cursorY += y;
	if (cgs.cursorY < 0)
		cgs.cursorY = 0;
	else if (cgs.cursorY > SCREEN_HEIGHT)
		cgs.cursorY = SCREEN_HEIGHT;

	n = Display_CursorType(cgs.cursorX, cgs.cursorY);
	cgs.activeCursor = 0;
	if (n == CURSOR_ARROW) {
		cgs.activeCursor = cgs.media.selectCursor;
	} else if (n == CURSOR_SIZER) {
		cgs.activeCursor = cgs.media.sizeCursor;
	}

	if (cgs.capturedItem) {
		Display_MouseMove(cgs.capturedItem, (float)x, (float)y);
	} else {
		Display_MouseMove(NULL, cgs.cursorX, cgs.cursorY);
	}

}

/*
==================
CG_HideTeamMenus
==================

*/
void CG_HideTeamMenu( void ) {
	Menus_CloseByName("teamMenu");
	Menus_CloseByName("getMenu");
}

/*
==================
CG_ShowTeamMenus
==================

*/
void CG_ShowTeamMenu( void ) {
	Menus_OpenByName("teamMenu");
}




/*
==================
CG_EventHandling
==================
type 0 - no event handling
1 - team menu
2 - hud editor

*/
void CG_EventHandling(int type) {
	cgs.eventHandling = type;
	if (type == CGAME_EVENT_NONE) {
		CG_HideTeamMenu();
	} else if (type == CGAME_EVENT_TEAMMENU) {
		//CG_ShowTeamMenu();
	} else if (type == CGAME_EVENT_SCOREBOARD) {
	}

}



void CG_KeyEvent(int key, qboolean down) {

	if (!down) {
		return;
	}

	if ( cg.predictedPlayerState.pm_type == PM_NORMAL || (cg.predictedPlayerState.pm_type == PM_SPECTATOR && cg.showScores == qfalse)) {
		CG_EventHandling(CGAME_EVENT_NONE);
		trap->Key_SetCatcher(0);
		return;
	}

	//if (key == trap->Key_GetKey("teamMenu") || !Display_CaptureItem(cgs.cursorX, cgs.cursorY)) {
	// if we see this then we should always be visible
	//  CG_EventHandling(CGAME_EVENT_NONE);
	//  trap->Key_SetCatcher(0);
	//}



	Display_HandleKey(key, down, (int)cgs.cursorX, (int)cgs.cursorY);

	if (cgs.capturedItem) {
		cgs.capturedItem = NULL;
	}	else {
		if (key == K_MOUSE2 && down) {
			cgs.capturedItem = Display_CaptureItem((int)cgs.cursorX, (int)cgs.cursorY);
		}
	}
}

int CG_ClientNumFromName(const char *p) {
	int i;
	for (i = 0; i < cgs.maxclients; i++) {
		if (cgs.clientinfo[i].infoValid && Q_stricmp(cgs.clientinfo[i].name, p) == 0) {
			return i;
		}
	}
	return -1;
}

void CG_ShowResponseHead(void) {
	Menus_OpenByName("voiceMenu");
	trap->Cvar_Set("cl_conXOffset", "72");
	cg.voiceTime = cg.time;
}

void CG_RunMenuScript(char **args) {
}

qboolean CG_DeferMenuScript (char **args) {
	return qfalse;
}

void CG_GetTeamColor(vector4 *color) {
		 if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
		VectorSet4( color, 1.0, 0.0, 0.0, 0.25f );
	else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
		VectorSet4( color, 0.0, 0.0, 1.0, 0.25f );
	else
		VectorSet4( color, 0.0, 0.17f, 0.0, 0.25f );
}

//QTZTODO: Item pickup HUD
#if 0
void CG_DrawPickupItem( void )
{
	linkedList_t *node = NULL;
	float x = ICON_SIZE*0.25f, y = SCREEN_HEIGHT-ICON_SIZE*0.25;
	int count = 0;

	trap->R_SetColor( NULL );

	if ( cg_debugHUD.boolean )
	{
		int itemNum = 0;
		count = 5;
		y -= ICON_SIZE*count;

		itemNum = BG_FindItem( "item_shield_sm_instant" )-bg_itemlist;
		CG_RegisterItemVisuals( itemNum );		CG_DrawPic( x, y, ICON_SIZE, ICON_SIZE, cg_items[itemNum].icon );		y += ICON_SIZE;
		itemNum = BG_FindItem( "item_shield_lrg_instant" )-bg_itemlist;
		CG_RegisterItemVisuals( itemNum );		CG_DrawPic( x, y, ICON_SIZE, ICON_SIZE, cg_items[itemNum].icon );		y += ICON_SIZE;
		itemNum = BG_FindItem( "item_medpak_instant" )-bg_itemlist;
		CG_RegisterItemVisuals( itemNum );		CG_DrawPic( x, y, ICON_SIZE, ICON_SIZE, cg_items[itemNum].icon );		y += ICON_SIZE;
		itemNum = BG_FindItem( "team_CTF_redflag" )-bg_itemlist;
		CG_RegisterItemVisuals( itemNum );		CG_DrawPic( x, y, ICON_SIZE, ICON_SIZE, cg_items[itemNum].icon );		y += ICON_SIZE;
		itemNum = BG_FindItem( "team_CTF_blueflag" )-bg_itemlist;
		CG_RegisterItemVisuals( itemNum );		CG_DrawPic( x, y, ICON_SIZE, ICON_SIZE, cg_items[itemNum].icon );		y += ICON_SIZE;
	}
	else
	{
		//First pass to remove expired items
		for ( node=cg.q3p.itemPickupRoot; node; node=LinkedList_Traverse( node ) )
		{
			itemPickup_t *item = (itemPickup_t*)node->data;
			if ( item->pickupTime < cg.time - cg_itemPickupTime.integer )
			{//Free this object
				free( item );
				LinkedList_RemoveObject( &cg.q3p.itemPickupRoot, node );
				node = cg.q3p.itemPickupRoot;//HACK: Return to start of list
				continue;
			}
			count++;
		}

		y -= ICON_SIZE*count;
		for ( node=cg.q3p.itemPickupRoot; node; node=LinkedList_Traverse( node ) )
		{
			itemPickup_t *item = (itemPickup_t*)node->data;
			CG_RegisterItemVisuals( item->itemNum );
			CG_DrawPic( x, y, ICON_SIZE, ICON_SIZE, cg_items[item->itemNum].icon );
			y += ICON_SIZE;
		}
	}
}
#endif

//QTZTODO: Obituary HUD
#if 0
void CG_DrawObituary( void )
{
	linkedList_t *node = NULL;
	float x = SCREEN_WIDTH/2.0f, y = 0.0f;
	float lineHeight = 20.0f;
	float iconSize = lineHeight*2.0f;
	int fontHandle = MenuFontToHandle( FONT_Q3PSMALL );
	float fontScale = 0.8f;//0.2175f;

	if ( !cg_newObituary.integer )
		return;

	trap->R_SetColor( NULL );

	if ( cg_debugHUD.boolean )
	{
		char *attackerName = "AttackerName";
		char *targetName = "TargetName";
		float attackerWidth = trap->R_Font_StrLenPixels( attackerName, fontHandle, fontScale );
		float iconWidth = 2.0f + iconSize + 2.0f;
		float victimWidth = trap->R_Font_StrLenPixels( targetName, fontHandle, fontScale );
		float totalWidth = attackerWidth + iconWidth + victimWidth;
		float startX = x - totalWidth/2.0f;

		trap->R_Font_DrawString( startX, y, attackerName, colorWhite, fontHandle, -1, fontScale );												startX += attackerWidth;
		CG_DrawPic( startX, y-(iconSize/2.0f)+lineHeight/2.0f, iconSize, iconSize, cg_weapons[ weaponFromMOD[MOD_QUANTIZER] ].weaponIcon );		startX += iconWidth;
		trap->R_Font_DrawString( startX, y, targetName, colorWhite, fontHandle, -1, fontScale );													startX += victimWidth;
		startX = x - totalWidth/2.0f;
		y += lineHeight;

		trap->R_Font_DrawString( startX, y, attackerName, colorWhite, fontHandle, -1, fontScale );												startX += attackerWidth;
		CG_DrawPic( startX, y-(iconSize/2.0f)+lineHeight/2.0f, iconSize, iconSize, cg_weapons[ weaponFromMOD[MOD_MORTAR] ].weaponIcon );		startX += iconWidth;
		trap->R_Font_DrawString( startX, y, targetName, colorWhite, fontHandle, -1, fontScale );													startX += victimWidth;
		startX = x - totalWidth/2.0f;
		y += lineHeight;

		trap->R_Font_DrawString( startX, y, attackerName, colorWhite, fontHandle, -1, fontScale );												startX += attackerWidth;
		CG_DrawPic( startX, y-(iconSize/2.0f)+lineHeight/2.0f, iconSize, iconSize, cg_weapons[ weaponFromMOD[MOD_DIVERGENCE] ].weaponIcon );	startX += iconWidth;
		trap->R_Font_DrawString( startX, y, targetName, colorWhite, fontHandle, -1, fontScale );													startX += victimWidth;
		startX = x - totalWidth/2.0f;
		y += lineHeight;
	}
	else
	{
		for ( node=cg.q3p.obituaryRoot; node; node=LinkedList_Traverse( node ) )
		{
			obituary_t *obituary = (obituary_t*)node->data;
			if ( obituary->deathTime < cg.time - cg_obituaryTime.integer )
			{//Free this object
				free( obituary );
				LinkedList_RemoveObject( &cg.q3p.obituaryRoot, node );
				node = cg.q3p.obituaryRoot;//HACK: Return to start of list
				continue;
			}
			else
			{
				float attackerWidth = trap->R_Font_StrLenPixels( obituary->attackerName, fontHandle, fontScale );
				float iconWidth = 2.0f + iconSize + 2.0f;
				float victimWidth = trap->R_Font_StrLenPixels( obituary->targetName, fontHandle, fontScale );
				float totalWidth = attackerWidth + iconWidth + victimWidth;
				float startX = x - totalWidth/2.0f;
				trap->R_Font_DrawString( startX, y, obituary->attackerName, colorWhite, fontHandle, -1, fontScale );						startX += attackerWidth;
				CG_DrawPic( startX, y-(iconSize/2.0f)+lineHeight/2.0f, iconSize, iconSize, cg_weapons[ weaponFromMOD[obituary->mod] ].weaponIcon );		startX += iconWidth;
				trap->R_Font_DrawString( startX, y, obituary->targetName, colorWhite, fontHandle, -1, fontScale );						startX += victimWidth;
				y += lineHeight;
			}
		}
	}
}
#endif

//QTZTODO: Flag carrier HUD
#if 0
void CG_DrawFlagCarrierName( void )
{
	int fontHandle = MenuFontToHandle( FONT_Q3PLARGE );
	float fontScale = 0.137f;
	vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	vector3 worldPos = { 0.0f, 0.0f, 0.0f };
	vector2 screenPos = { 0.0f, 0.0f };
	char text[] = "FLAG CARRIER";

	if ( (cgs.gametype < GT_CTF || !cg_drawFlagCarrier.boolean || cg.q3p.flagCarrierEntityNum == ENTITYNUM_NONE) && !cg_debugHUD.integer )
		return;

	VectorCopy( cg_entities[cg.q3p.flagCarrierEntityNum].lerpOrigin, worldPos );
	worldPos[2] += 64.0f;

	//FIXME: skews position on non 4:3 ratios
	// fix is in modbase/ja++
	CG_WorldCoordToScreenCoordFloat( worldPos, &screenPos[0], &screenPos[1] );

	trap->R_Font_DrawString( screenPos[0] - trap->R_Font_StrLenPixels( text, fontHandle, fontScale )/2.0, screenPos[1], text, color, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	//already drawn it this frame, reset ent num so we can scan again next frame
	cg.q3p.flagCarrierEntityNum = ENTITYNUM_NONE;

	trap->R_SetColor( NULL );
}
#endif
