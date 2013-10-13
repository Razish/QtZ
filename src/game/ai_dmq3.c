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

/*****************************************************************************
 * name:		ai_dmq3.c
 *
 * desc:		Quake3 bot AI
 *
 * $Archive: /MissionPack/code/game/ai_dmq3.c $
 *
 *****************************************************************************/


#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_char.h"
#include "../botlib/be_ai_chat.h"
#include "../botlib/be_ai_gen.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/be_ai_weap.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_dmnet.h"
#include "ai_team.h"
//
#include "chars.h"				//characteristics
#include "inv.h"				//indexes into the inventory
#include "syn.h"				//synonyms
#include "match.h"				//string matching types and vars

// for the voice chats
#include "../../build/qtz/ui/menudef.h"

// from aasfile.h
#define AREACONTENTS_MOVER				1024
#define AREACONTENTS_MODELNUMSHIFT		24
#define AREACONTENTS_MAXMODELNUM		0xFF
#define AREACONTENTS_MODELNUM			(AREACONTENTS_MAXMODELNUM << AREACONTENTS_MODELNUMSHIFT)

#define IDEAL_ATTACKDIST			140

#define MAX_WAYPOINTS		128
//
bot_waypoint_t botai_waypoints[MAX_WAYPOINTS];
bot_waypoint_t *botai_freewaypoints;

//NOTE: not using a cvars which can be updated because the game should be reloaded anyway
int gametype;		//game type
int maxclients;		//maximum number of clients

cvar_t *bot_grapple;
cvar_t *bot_rocketjump;
cvar_t *bot_fastchat;
cvar_t *bot_nochat;
cvar_t *bot_testrchat;
cvar_t *bot_challenge;
cvar_t *bot_predictobstacles;
cvar_t *g_difficulty;

extern cvar_t *bot_developer;

vector3 lastteleport_origin;		//last teleport event origin
float lastteleport_time;		//last teleport event time
int max_bspmodelindex;			//maximum BSP model index

//CTF flag goals
bot_goal_t ctf_redflag;
bot_goal_t ctf_blueflag;
bot_goal_t ctf_neutralflag;
bot_goal_t redobelisk;
bot_goal_t blueobelisk;
bot_goal_t neutralobelisk;

#define MAX_ALTROUTEGOALS		32

int altroutegoals_setup;
aas_altroutegoal_t red_altroutegoals[MAX_ALTROUTEGOALS];
int red_numaltroutegoals;
aas_altroutegoal_t blue_altroutegoals[MAX_ALTROUTEGOALS];
int blue_numaltroutegoals;

void BotSetUserInfo(bot_state_t *bs, char *key, const char *value) {
	char userinfo[MAX_INFO_STRING];

	trap->SV_GetUserinfo(bs->client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, key, value);
	trap->SV_SetUserinfo(bs->client, userinfo);
	ClientUserinfoChanged( bs->client );
}

int BotCTFCarryingFlag(bot_state_t *bs) {
	if (gametype != GT_FLAGS) return CTF_FLAG_NONE;

	if (bs->inventory[INVENTORY_REDFLAG] > 0) return CTF_FLAG_RED;
	else if (bs->inventory[INVENTORY_BLUEFLAG] > 0) return CTF_FLAG_BLUE;
	return CTF_FLAG_NONE;
}

int BotTeam(bot_state_t *bs) {

	if (bs->client < 0 || bs->client >= MAX_CLIENTS) {
		return qfalse;
	}

    if (level.clients[bs->client].sess.sessionTeam == TEAM_RED) {
		return TEAM_RED;
	} else if (level.clients[bs->client].sess.sessionTeam == TEAM_BLUE) {
		return TEAM_BLUE;
	}

	return TEAM_FREE;
}

int BotOppositeTeam(bot_state_t *bs) {
	switch(BotTeam(bs)) {
		case TEAM_RED: return TEAM_BLUE;
		case TEAM_BLUE: return TEAM_RED;
		default: return TEAM_FREE;
	}
}

bot_goal_t *BotEnemyFlag(bot_state_t *bs) {
	if (BotTeam(bs) == TEAM_RED) {
		return &ctf_blueflag;
	}
	else {
		return &ctf_redflag;
	}
}

bot_goal_t *BotTeamFlag(bot_state_t *bs) {
	if (BotTeam(bs) == TEAM_RED) {
		return &ctf_redflag;
	}
	else {
		return &ctf_blueflag;
	}
}

qboolean EntityIsDead(aas_entityinfo_t *entinfo) {
	playerState_t ps;

	if (entinfo->number >= 0 && entinfo->number < MAX_CLIENTS) {
		//retrieve the current client state
		BotAI_GetClientState( entinfo->number, &ps );
		if (ps.pm_type != PM_NORMAL) return qtrue;
	}
	return qfalse;
}

qboolean EntityCarriesFlag(aas_entityinfo_t *entinfo) {
	if ( entinfo->powerups & ( 1 << PW_REDFLAG ) )
		return qtrue;
	if ( entinfo->powerups & ( 1 << PW_BLUEFLAG ) )
		return qtrue;
	if ( entinfo->powerups & ( 1 << PW_NEUTRALFLAG ) )
		return qtrue;
	return qfalse;
}

qboolean EntityIsInvisible(aas_entityinfo_t *entinfo) {
	// the flag is always visible
	if (EntityCarriesFlag(entinfo)) {
		return qfalse;
	}
	return qfalse;
}

qboolean EntityIsShooting(aas_entityinfo_t *entinfo) {
	if (entinfo->flags & EF_FIRING) {
		return qtrue;
	}
	return qfalse;
}

qboolean EntityIsChatting(aas_entityinfo_t *entinfo) {
	if (entinfo->flags & EF_TALK) {
		return qtrue;
	}
	return qfalse;
}

qboolean EntityHasQuad(aas_entityinfo_t *entinfo) {
	if (entinfo->powerups & (1 << PW_QUAD)) {
		return qtrue;
	}
	return qfalse;
}

int BotTrojanCarryingFlag(bot_state_t *bs) {
	if (gametype != GT_TROJAN) return qfalse;

	if (bs->inventory[INVENTORY_NEUTRALFLAG] > 0) return qtrue;
	return qfalse;
}

void BotRememberLastOrderedTask(bot_state_t *bs) {
	if (!bs->ordered) {
		return;
	}
	bs->lastgoal_decisionmaker = bs->decisionmaker;
	bs->lastgoal_ltgtype = bs->ltgtype;
	memcpy(&bs->lastgoal_teamgoal, &bs->teamgoal, sizeof(bot_goal_t));
	bs->lastgoal_teammate = bs->teammate;
}

int BotSetLastOrderedTask(bot_state_t *bs) {

	if (gametype == GT_FLAGS) {
		// don't go back to returning the flag if it's at the base
		if ( bs->lastgoal_ltgtype == LTG_RETURNFLAG ) {
			if ( BotTeam(bs) == TEAM_RED ) {
				if ( bs->redflagstatus == 0 ) {
					bs->lastgoal_ltgtype = 0;
				}
			}
			else {
				if ( bs->blueflagstatus == 0 ) {
					bs->lastgoal_ltgtype = 0;
				}
			}
		}
	}

	if ( bs->lastgoal_ltgtype ) {
		bs->decisionmaker = bs->lastgoal_decisionmaker;
		bs->ordered = qtrue;
		bs->ltgtype = bs->lastgoal_ltgtype;
		memcpy(&bs->teamgoal, &bs->lastgoal_teamgoal, sizeof(bot_goal_t));
		bs->teammate = bs->lastgoal_teammate;
		bs->teamgoal_time = FloatTime() + 300;
		//
		if ( gametype == GT_FLAGS ) {
			if ( bs->ltgtype == LTG_GETFLAG ) {
				bot_goal_t *tb, *eb;
				int tt, et;

				tb = BotTeamFlag(bs);
				eb = BotEnemyFlag(bs);
				tt = trap->aas->AAS_AreaTravelTimeToGoalArea(bs->areanum, &bs->origin, tb->areanum, TFL_DEFAULT);
				et = trap->aas->AAS_AreaTravelTimeToGoalArea(bs->areanum, &bs->origin, eb->areanum, TFL_DEFAULT);
				// if the travel time towards the enemy base is larger than towards our base
				if (et > tt) {
					//get an alternative route goal towards the enemy base
					BotGetAlternateRouteGoal(bs, BotOppositeTeam(bs));
				}
			}
		}
		return qtrue;
	}
	return qfalse;
}

void BotRefuseOrder(bot_state_t *bs) {
	if (!bs->ordered)
		return;
	// if the bot was ordered to do something
	if ( bs->order_time && bs->order_time > FloatTime() - 10 ) {
		trap->ea->EA_Action(bs->client, ACTION_NEGATIVE);
		BotVoiceChat(bs, bs->decisionmaker, VOICECHAT_NO);
		bs->order_time = 0;
	}
}

void BotCTFSeekGoals(bot_state_t *bs) {
	float rnd, l1, l2;
	int flagstatus, c;
	vector3 dir;
	aas_entityinfo_t entinfo;

	//when carrying a flag in ctf the bot should rush to the base
	if (BotCTFCarryingFlag(bs)) {
		//if not already rushing to the base
		if (bs->ltgtype != LTG_RUSHBASE) {
			BotRefuseOrder(bs);
			bs->ltgtype = LTG_RUSHBASE;
			bs->teamgoal_time = FloatTime() + CTF_RUSHBASE_TIME;
			bs->rushbaseaway_time = 0;
			bs->decisionmaker = bs->client;
			bs->ordered = qfalse;
			//
			switch(BotTeam(bs)) {
				case TEAM_RED: VectorSubtract(&bs->origin, &ctf_blueflag.origin, &dir); break;
				case TEAM_BLUE: VectorSubtract(&bs->origin, &ctf_redflag.origin, &dir); break;
				default: VectorSet(&dir, 999, 999, 999); break;
			}
			// if the bot picked up the flag very close to the enemy base
			if ( VectorLength(&dir) < 128 ) {
				// get an alternative route goal through the enemy base
				BotGetAlternateRouteGoal(bs, BotOppositeTeam(bs));
			} else {
				// don't use any alt route goal, just get the hell out of the base
				bs->altroutegoal.areanum = 0;
			}
			BotSetUserInfo(bs, "teamtask", va("%d", TEAMTASK_OFFENSE));
			BotVoiceChat(bs, -1, VOICECHAT_IHAVEFLAG);
		}
		else if (bs->rushbaseaway_time > FloatTime()) {
			if (BotTeam(bs) == TEAM_RED) flagstatus = bs->redflagstatus;
			else flagstatus = bs->blueflagstatus;
			//if the flag is back
			if (flagstatus == 0) {
				bs->rushbaseaway_time = 0;
			}
		}
		return;
	}
	// if the bot decided to follow someone
	if ( bs->ltgtype == LTG_TEAMACCOMPANY && !bs->ordered ) {
		// if the team mate being accompanied no longer carries the flag
		BotEntityInfo(bs->teammate, &entinfo);
		if (!EntityCarriesFlag(&entinfo)) {
			bs->ltgtype = 0;
		}
	}
	//
	if (BotTeam(bs) == TEAM_RED) flagstatus = bs->redflagstatus * 2 + bs->blueflagstatus;
	else flagstatus = bs->blueflagstatus * 2 + bs->redflagstatus;
	//if our team has the enemy flag and our flag is at the base
	if (flagstatus == 1) {
		//
		if (bs->owndecision_time < FloatTime()) {
			//if Not defending the base already
			if (!(bs->ltgtype == LTG_DEFENDKEYAREA &&
					(bs->teamgoal.number == ctf_redflag.number ||
					bs->teamgoal.number == ctf_blueflag.number))) {
				//if there is a visible team mate flag carrier
				c = BotTeamFlagCarrierVisible(bs);
				if (c >= 0 &&
						// and not already following the team mate flag carrier
						(bs->ltgtype != LTG_TEAMACCOMPANY || bs->teammate != c)) {
					//
					BotRefuseOrder(bs);
					//follow the flag carrier
					bs->decisionmaker = bs->client;
					bs->ordered = qfalse;
					//the team mate
					bs->teammate = c;
					//last time the team mate was visible
					bs->teammatevisible_time = FloatTime();
					//no message
					bs->teammessage_time = 0;
					//no arrive message
					bs->arrive_time = 1;
					//
					BotVoiceChat(bs, bs->teammate, VOICECHAT_ONFOLLOW);
					//get the team goal time
					bs->teamgoal_time = FloatTime() + TEAM_ACCOMPANY_TIME;
					bs->ltgtype = LTG_TEAMACCOMPANY;
					bs->formation_dist = 3.5f * 32;		//3.5 meter
					bs->owndecision_time = (int)(FloatTime() + 5);
				}
			}
		}
		return;
	}
	//if the enemy has our flag
	else if (flagstatus == 2) {
		//
		if (bs->owndecision_time < FloatTime()) {
			//if enemy flag carrier is visible
			c = BotEnemyFlagCarrierVisible(bs);
			if (c >= 0) {
				//FIXME: fight enemy flag carrier
			}
			//if not already doing something important
			if (bs->ltgtype != LTG_GETFLAG &&
				bs->ltgtype != LTG_RETURNFLAG &&
				bs->ltgtype != LTG_TEAMHELP &&
				bs->ltgtype != LTG_TEAMACCOMPANY &&
				bs->ltgtype != LTG_CAMPORDER &&
				bs->ltgtype != LTG_PATROL &&
				bs->ltgtype != LTG_GETITEM) {

				BotRefuseOrder(bs);
				bs->decisionmaker = bs->client;
				bs->ordered = qfalse;
				//
				if (random() < 0.5f) {
					//go for the enemy flag
					bs->ltgtype = LTG_GETFLAG;
				}
				else {
					bs->ltgtype = LTG_RETURNFLAG;
				}
				//no team message
				bs->teammessage_time = 0;
				//set the time the bot will stop getting the flag
				bs->teamgoal_time = FloatTime() + CTF_GETFLAG_TIME;
				//get an alternative route goal towards the enemy base
				BotGetAlternateRouteGoal(bs, BotOppositeTeam(bs));
				//
				bs->owndecision_time = (int)(FloatTime() + 5);
			}
		}
		return;
	}
	//if both flags Not at their bases
	else if (flagstatus == 3) {
		//
		if (bs->owndecision_time < FloatTime()) {
			// if not trying to return the flag and not following the team flag carrier
			if ( bs->ltgtype != LTG_RETURNFLAG && bs->ltgtype != LTG_TEAMACCOMPANY ) {
				//
				c = BotTeamFlagCarrierVisible(bs);
				// if there is a visible team mate flag carrier
				if (c >= 0) {
					BotRefuseOrder(bs);
					//follow the flag carrier
					bs->decisionmaker = bs->client;
					bs->ordered = qfalse;
					//the team mate
					bs->teammate = c;
					//last time the team mate was visible
					bs->teammatevisible_time = FloatTime();
					//no message
					bs->teammessage_time = 0;
					//no arrive message
					bs->arrive_time = 1;
					//
					BotVoiceChat(bs, bs->teammate, VOICECHAT_ONFOLLOW);
					//get the team goal time
					bs->teamgoal_time = FloatTime() + TEAM_ACCOMPANY_TIME;
					bs->ltgtype = LTG_TEAMACCOMPANY;
					bs->formation_dist = 3.5 * 32;		//3.5 meter
					//
					bs->owndecision_time = (int)(FloatTime() + 5);
				}
				else {
					BotRefuseOrder(bs);
					bs->decisionmaker = bs->client;
					bs->ordered = qfalse;
					//get the enemy flag
					bs->teammessage_time = FloatTime() + 2 * random();
					//get the flag
					bs->ltgtype = LTG_RETURNFLAG;
					//set the time the bot will stop getting the flag
					bs->teamgoal_time = FloatTime() + CTF_RETURNFLAG_TIME;
					//get an alternative route goal towards the enemy base
					BotGetAlternateRouteGoal(bs, BotOppositeTeam(bs));
					//
					bs->owndecision_time = (int)(FloatTime() + 5);
				}
			}
		}
		return;
	}
	// if the bot is ordered to do something
	if ( bs->lastgoal_ltgtype ) {
		bs->teamgoal_time += 60;
	}
	// if the bot decided to do something on its own and has a last ordered goal
	if ( !bs->ordered && bs->lastgoal_ltgtype ) {
		bs->ltgtype = 0;
	}
	//if already a CTF or team goal
	if (bs->ltgtype == LTG_TEAMHELP ||
			bs->ltgtype == LTG_TEAMACCOMPANY ||
			bs->ltgtype == LTG_DEFENDKEYAREA ||
			bs->ltgtype == LTG_GETFLAG ||
			bs->ltgtype == LTG_RUSHBASE ||
			bs->ltgtype == LTG_RETURNFLAG ||
			bs->ltgtype == LTG_CAMPORDER ||
			bs->ltgtype == LTG_PATROL ||
			bs->ltgtype == LTG_GETITEM ||
			bs->ltgtype == LTG_MAKELOVE_UNDER ||
			bs->ltgtype == LTG_MAKELOVE_ONTOP) {
		return;
	}
	//
	if (BotSetLastOrderedTask(bs))
		return;
	//
	if (bs->owndecision_time > FloatTime())
		return;;
	//if the bot is roaming
	if (bs->ctfroam_time > FloatTime())
		return;
	//if the bot has enough aggression to decide what to do
	if (BotAggression(bs) < 50)
		return;
	//set the time to send a message to the team mates
	bs->teammessage_time = FloatTime() + 2 * random();
	//
	if (bs->teamtaskpreference & (TEAMTP_ATTACKER|TEAMTP_DEFENDER)) {
		if (bs->teamtaskpreference & TEAMTP_ATTACKER) {
			l1 = 0.7f;
		}
		else {
			l1 = 0.2f;
		}
		l2 = 0.9f;
	}
	else {
		l1 = 0.4f;
		l2 = 0.7f;
	}
	//get the flag or defend the base
	rnd = random();
	if (rnd < l1 && ctf_redflag.areanum && ctf_blueflag.areanum) {
		bs->decisionmaker = bs->client;
		bs->ordered = qfalse;
		bs->ltgtype = LTG_GETFLAG;
		//set the time the bot will stop getting the flag
		bs->teamgoal_time = FloatTime() + CTF_GETFLAG_TIME;
		//get an alternative route goal towards the enemy base
		BotGetAlternateRouteGoal(bs, BotOppositeTeam(bs));
	}
	else if (rnd < l2 && ctf_redflag.areanum && ctf_blueflag.areanum) {
		bs->decisionmaker = bs->client;
		bs->ordered = qfalse;
		//
		if (BotTeam(bs) == TEAM_RED) memcpy(&bs->teamgoal, &ctf_redflag, sizeof(bot_goal_t));
		else memcpy(&bs->teamgoal, &ctf_blueflag, sizeof(bot_goal_t));
		//set the ltg type
		bs->ltgtype = LTG_DEFENDKEYAREA;
		//set the time the bot stops defending the base
		bs->teamgoal_time = FloatTime() + TEAM_DEFENDKEYAREA_TIME;
		bs->defendaway_time = 0;
	}
	else {
		bs->ltgtype = 0;
		//set the time the bot will stop roaming
		bs->ctfroam_time = FloatTime() + CTF_ROAM_TIME;
	}
	bs->owndecision_time = (int)(FloatTime() + 5);
#ifdef DEBUG
	BotPrintTeamGoal(bs);
#endif //DEBUG
}

void BotCTFRetreatGoals(bot_state_t *bs) {
	//when carrying a flag in ctf the bot should rush to the base
	if (BotCTFCarryingFlag(bs)) {
		//if not already rushing to the base
		if (bs->ltgtype != LTG_RUSHBASE) {
			BotRefuseOrder(bs);
			bs->ltgtype = LTG_RUSHBASE;
			bs->teamgoal_time = FloatTime() + CTF_RUSHBASE_TIME;
			bs->rushbaseaway_time = 0;
			bs->decisionmaker = bs->client;
			bs->ordered = qfalse;
		}
	}
}

void BotTrojanSeekGoals(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	float rnd, l1, l2;
	int c;

	//when carrying a flag in ctf the bot should rush to the base
	if (BotTrojanCarryingFlag(bs)) {
		//if not already rushing to the base
		if (bs->ltgtype != LTG_RUSHBASE) {
			BotRefuseOrder(bs);
			bs->ltgtype = LTG_RUSHBASE;
			bs->teamgoal_time = FloatTime() + CTF_RUSHBASE_TIME;
			bs->rushbaseaway_time = 0;
			bs->decisionmaker = bs->client;
			bs->ordered = qfalse;
			//get an alternative route goal towards the enemy base
			BotGetAlternateRouteGoal(bs, BotOppositeTeam(bs));
			//
			BotVoiceChat(bs, -1, VOICECHAT_IHAVEFLAG);
		}
		return;
	}
	// if the bot decided to follow someone
	if ( bs->ltgtype == LTG_TEAMACCOMPANY && !bs->ordered ) {
		// if the team mate being accompanied no longer carries the flag
		BotEntityInfo(bs->teammate, &entinfo);
		if (!EntityCarriesFlag(&entinfo)) {
			bs->ltgtype = 0;
		}
	}
	//our team has the flag
	if (bs->neutralflagstatus == 1) {
		if (bs->owndecision_time < FloatTime()) {
			// if not already following someone
			if (bs->ltgtype != LTG_TEAMACCOMPANY) {
				//if there is a visible team mate flag carrier
				c = BotTeamFlagCarrierVisible(bs);
				if (c >= 0) {
					BotRefuseOrder(bs);
					//follow the flag carrier
					bs->decisionmaker = bs->client;
					bs->ordered = qfalse;
					//the team mate
					bs->teammate = c;
					//last time the team mate was visible
					bs->teammatevisible_time = FloatTime();
					//no message
					bs->teammessage_time = 0;
					//no arrive message
					bs->arrive_time = 1;
					//
					BotVoiceChat(bs, bs->teammate, VOICECHAT_ONFOLLOW);
					//get the team goal time
					bs->teamgoal_time = FloatTime() + TEAM_ACCOMPANY_TIME;
					bs->ltgtype = LTG_TEAMACCOMPANY;
					bs->formation_dist = 3.5f * 32;		//3.5 meter
					bs->owndecision_time = (int)(FloatTime() + 5);
					return;
				}
			}
			//if already a CTF or team goal
			if (bs->ltgtype == LTG_TEAMHELP ||
					bs->ltgtype == LTG_TEAMACCOMPANY ||
					bs->ltgtype == LTG_DEFENDKEYAREA ||
					bs->ltgtype == LTG_GETFLAG ||
					bs->ltgtype == LTG_RUSHBASE ||
					bs->ltgtype == LTG_CAMPORDER ||
					bs->ltgtype == LTG_PATROL ||
					bs->ltgtype == LTG_ATTACKENEMYBASE ||
					bs->ltgtype == LTG_GETITEM ||
					bs->ltgtype == LTG_MAKELOVE_UNDER ||
					bs->ltgtype == LTG_MAKELOVE_ONTOP) {
				return;
			}
			//if not already attacking the enemy base
			if (bs->ltgtype != LTG_ATTACKENEMYBASE) {
				BotRefuseOrder(bs);
				bs->decisionmaker = bs->client;
				bs->ordered = qfalse;
				//
				if (BotTeam(bs) == TEAM_RED) memcpy(&bs->teamgoal, &ctf_blueflag, sizeof(bot_goal_t));
				else memcpy(&bs->teamgoal, &ctf_redflag, sizeof(bot_goal_t));
				//set the ltg type
				bs->ltgtype = LTG_ATTACKENEMYBASE;
				//set the time the bot will stop getting the flag
				bs->teamgoal_time = FloatTime() + TEAM_ATTACKENEMYBASE_TIME;
				bs->owndecision_time = (int)(FloatTime() + 5);
			}
		}
		return;
	}
	//enemy team has the flag
	else if (bs->neutralflagstatus == 2) {
		if (bs->owndecision_time < FloatTime()) {
			c = BotEnemyFlagCarrierVisible(bs);
			if (c >= 0) {
				//FIXME: attack enemy flag carrier
			}
			//if already a CTF or team goal
			if (bs->ltgtype == LTG_TEAMHELP ||
					bs->ltgtype == LTG_TEAMACCOMPANY ||
					bs->ltgtype == LTG_CAMPORDER ||
					bs->ltgtype == LTG_PATROL ||
					bs->ltgtype == LTG_GETITEM) {
				return;
			}
			// if not already defending the base
			if (bs->ltgtype != LTG_DEFENDKEYAREA) {
				BotRefuseOrder(bs);
				bs->decisionmaker = bs->client;
				bs->ordered = qfalse;
				//
				if (BotTeam(bs) == TEAM_RED) memcpy(&bs->teamgoal, &ctf_redflag, sizeof(bot_goal_t));
				else memcpy(&bs->teamgoal, &ctf_blueflag, sizeof(bot_goal_t));
				//set the ltg type
				bs->ltgtype = LTG_DEFENDKEYAREA;
				//set the time the bot stops defending the base
				bs->teamgoal_time = FloatTime() + TEAM_DEFENDKEYAREA_TIME;
				bs->defendaway_time = 0;
				bs->owndecision_time = (int)(FloatTime() + 5);
			}
		}
		return;
	}
	// if the bot is ordered to do something
	if ( bs->lastgoal_ltgtype ) {
		bs->teamgoal_time += 60;
	}
	// if the bot decided to do something on its own and has a last ordered goal
	if ( !bs->ordered && bs->lastgoal_ltgtype ) {
		bs->ltgtype = 0;
	}
	//if already a CTF or team goal
	if (bs->ltgtype == LTG_TEAMHELP ||
			bs->ltgtype == LTG_TEAMACCOMPANY ||
			bs->ltgtype == LTG_DEFENDKEYAREA ||
			bs->ltgtype == LTG_GETFLAG ||
			bs->ltgtype == LTG_RUSHBASE ||
			bs->ltgtype == LTG_RETURNFLAG ||
			bs->ltgtype == LTG_CAMPORDER ||
			bs->ltgtype == LTG_PATROL ||
			bs->ltgtype == LTG_ATTACKENEMYBASE ||
			bs->ltgtype == LTG_GETITEM ||
			bs->ltgtype == LTG_MAKELOVE_UNDER ||
			bs->ltgtype == LTG_MAKELOVE_ONTOP) {
		return;
	}
	//
	if (BotSetLastOrderedTask(bs))
		return;
	//
	if (bs->owndecision_time > FloatTime())
		return;;
	//if the bot is roaming
	if (bs->ctfroam_time > FloatTime())
		return;
	//if the bot has enough aggression to decide what to do
	if (BotAggression(bs) < 50)
		return;
	//set the time to send a message to the team mates
	bs->teammessage_time = FloatTime() + 2 * random();
	//
	if (bs->teamtaskpreference & (TEAMTP_ATTACKER|TEAMTP_DEFENDER)) {
		if (bs->teamtaskpreference & TEAMTP_ATTACKER) {
			l1 = 0.7f;
		}
		else {
			l1 = 0.2f;
		}
		l2 = 0.9f;
	}
	else {
		l1 = 0.4f;
		l2 = 0.7f;
	}
	//get the flag or defend the base
	rnd = random();
	if (rnd < l1 && ctf_neutralflag.areanum) {
		bs->decisionmaker = bs->client;
		bs->ordered = qfalse;
		bs->ltgtype = LTG_GETFLAG;
		//set the time the bot will stop getting the flag
		bs->teamgoal_time = FloatTime() + CTF_GETFLAG_TIME;
	}
	else if (rnd < l2 && ctf_redflag.areanum && ctf_blueflag.areanum) {
		bs->decisionmaker = bs->client;
		bs->ordered = qfalse;
		//
		if (BotTeam(bs) == TEAM_RED) memcpy(&bs->teamgoal, &ctf_redflag, sizeof(bot_goal_t));
		else memcpy(&bs->teamgoal, &ctf_blueflag, sizeof(bot_goal_t));
		//set the ltg type
		bs->ltgtype = LTG_DEFENDKEYAREA;
		//set the time the bot stops defending the base
		bs->teamgoal_time = FloatTime() + TEAM_DEFENDKEYAREA_TIME;
		bs->defendaway_time = 0;
	}
	else {
		bs->ltgtype = 0;
		//set the time the bot will stop roaming
		bs->ctfroam_time = FloatTime() + CTF_ROAM_TIME;
	}
	bs->owndecision_time = (int)(FloatTime() + 5);
#ifdef DEBUG
	BotPrintTeamGoal(bs);
#endif //DEBUG
}

void BotTrojanRetreatGoals(bot_state_t *bs) {
	//when carrying a flag in ctf the bot should rush to the enemy base
	if (BotTrojanCarryingFlag(bs)) {
		//if not already rushing to the base
		if (bs->ltgtype != LTG_RUSHBASE) {
			BotRefuseOrder(bs);
			bs->ltgtype = LTG_RUSHBASE;
			bs->teamgoal_time = FloatTime() + CTF_RUSHBASE_TIME;
			bs->rushbaseaway_time = 0;
			bs->decisionmaker = bs->client;
			bs->ordered = qfalse;
			//get an alternative route goal towards the enemy base
			BotGetAlternateRouteGoal(bs, BotOppositeTeam(bs));
		}
	}
}

void BotTeamGoals(bot_state_t *bs, int retreat) {

	if ( retreat ) {
		if (gametype == GT_FLAGS) {
			BotCTFRetreatGoals(bs);
		}
		else if (gametype == GT_TROJAN) {
			BotTrojanRetreatGoals(bs);
		}
	}
	else {
		if (gametype == GT_FLAGS) {
			//decide what to do in CTF mode
			BotCTFSeekGoals(bs);
		}
		else if (gametype == GT_TROJAN) {
			BotTrojanSeekGoals(bs);
		}
	}
	// reset the order time which is used to see if
	// we decided to refuse an order
	bs->order_time = 0;
}

int BotPointAreaNum(vector3 *origin) {
	int areanum, numareas, areas[10];
	vector3 end;

	areanum = trap->aas->AAS_PointAreaNum(origin);
	if (areanum) return areanum;
	VectorCopy(origin, &end);
	end.z += 10;
	numareas = trap->aas->AAS_TraceAreas(origin, &end, areas, NULL, 10);
	if (numareas > 0) return areas[0];
	return 0;
}

char *ClientName(int client, char *name, size_t size) {
	char buf[MAX_INFO_STRING];

	if (client < 0 || client >= MAX_CLIENTS) {
		BotAI_Print(PRT_ERROR, "ClientName: client out of range\n");
		return "[client out of range]";
	}
	trap->SV_GetConfigstring(CS_PLAYERS+client, buf, sizeof(buf));
	strncpy(name, Info_ValueForKey(buf, "n"), size-1);
	name[size-1] = '\0';
	Q_CleanStr( name );
	return name;
}

char *ClientSkin(int client, char *skin, size_t size) {
	char buf[MAX_INFO_STRING];

	if (client < 0 || client >= MAX_CLIENTS) {
		BotAI_Print(PRT_ERROR, "ClientSkin: client out of range\n");
		return "[client out of range]";
	}
	trap->SV_GetConfigstring(CS_PLAYERS+client, buf, sizeof(buf));
	strncpy(skin, Info_ValueForKey(buf, "m"), size-1);
	skin[size-1] = '\0';
	return skin;
}

int ClientFromName(char *name) {
	int i;
	char buf[MAX_INFO_STRING];
	static int maxclients;

	if (!maxclients)
		maxclients = trap->Cvar_VariableIntegerValue("sv_maxclients");
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		trap->SV_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		Q_CleanStr( buf );
		if (!Q_stricmp(Info_ValueForKey(buf, "n"), name)) return i;
	}
	return -1;
}

int ClientOnSameTeamFromName(bot_state_t *bs, char *name) {
	int i;
	char buf[MAX_INFO_STRING];
	static int maxclients;

	if (!maxclients)
		maxclients = trap->Cvar_VariableIntegerValue("sv_maxclients");
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		if (!BotSameTeam(bs, i))
			continue;
		trap->SV_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		Q_CleanStr( buf );
		if (!Q_stricmp(Info_ValueForKey(buf, "n"), name)) return i;
	}
	return -1;
}

char *stristr(char *str, char *charset) {
	int i;

	while(*str) {
		for (i = 0; charset[i] && str[i]; i++) {
			if (toupper(charset[i]) != toupper(str[i])) break;
		}
		if (!charset[i]) return str;
		str++;
	}
	return NULL;
}

char *EasyClientName(int client, char *buf, size_t size) {
	int i;
	char *str1, *str2, *ptr, c;
	char name[128];

	ClientName(client, name, sizeof(name));
	
	for (i = 0; name[i]; i++) name[i] &= 127;
	//remove all spaces
	for (ptr = strstr(name, " "); ptr; ptr = strstr(name, " ")) {
		memmove(ptr, ptr+1, strlen(ptr+1)+1);
	}
	//check for [x] and ]x[ clan names
	str1 = strstr(name, "[");
	str2 = strstr(name, "]");
	if (str1 && str2) {
		if (str2 > str1) memmove(str1, str2+1, strlen(str2+1)+1);
		else memmove(str2, str1+1, strlen(str1+1)+1);
	}
	//remove Mr prefix
	if ((name[0] == 'm' || name[0] == 'M') &&
			(name[1] == 'r' || name[1] == 'R')) {
		memmove(name, name+2, strlen(name+2)+1);
	}
	//only allow lower case alphabet characters
	ptr = name;
	while(*ptr) {
		c = *ptr;
		if ((c >= 'a' && c <= 'z') ||
				(c >= '0' && c <= '9') || c == '_') {
			ptr++;
		}
		else if (c >= 'A' && c <= 'Z') {
			*ptr += 'a' - 'A';
			ptr++;
		}
		else {
			memmove(ptr, ptr+1, strlen(ptr + 1)+1);
		}
	}
	strncpy(buf, name, size-1);
	buf[size-1] = '\0';
	return buf;
}

int BotSynonymContext(bot_state_t *bs) {
	int context;

	context = CONTEXT_NORMAL|CONTEXT_NEARBYITEM|CONTEXT_NAMES;
	//
	if (gametype == GT_FLAGS
		|| gametype == GT_TROJAN
		) {
		if (BotTeam(bs) == TEAM_RED) context |= CONTEXT_CTFREDTEAM;
		else context |= CONTEXT_CTFBLUETEAM;
	}
	return context;
}

void BotChooseWeapon(bot_state_t *bs) {
	int newweaponnum;

	if (bs->cur_ps.weaponstate == WEAPON_RAISING ||
			bs->cur_ps.weaponstate == WEAPON_DROPPING) {
		trap->ea->EA_SelectWeapon(bs->client, bs->weaponnum);
	}
	else {
		newweaponnum = trap->ai->BotChooseBestFightWeapon(bs->ws, bs->inventory);
		if (bs->weaponnum != newweaponnum) bs->weaponchange_time = FloatTime();
		bs->weaponnum = newweaponnum;
		//BotAI_Print(PRT_MESSAGE, "bs->weaponnum = %d\n", bs->weaponnum);
		trap->ea->EA_SelectWeapon(bs->client, bs->weaponnum);
	}
}

void BotSetupForMovement(bot_state_t *bs) {
	bot_initmove_t initmove;

	memset(&initmove, 0, sizeof(bot_initmove_t));
	VectorCopy(&bs->cur_ps.origin, &initmove.origin);
	VectorCopy(&bs->cur_ps.velocity, &initmove.velocity);
	VectorClear(&initmove.viewoffset);
	initmove.viewoffset.z += bs->cur_ps.viewheight;
	initmove.entitynum = bs->entitynum;
	initmove.client = bs->client;
	initmove.thinktime = bs->thinktime;
	//set the onground flag
	if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE) initmove.or_moveflags |= MFL_ONGROUND;
	//set the teleported flag
	if ((bs->cur_ps.pm_flags & PMF_TIME_KNOCKBACK) && (bs->cur_ps.pm_time > 0)) {
		initmove.or_moveflags |= MFL_TELEPORTED;
	}
	//set the waterjump flag
	if ((bs->cur_ps.pm_flags & PMF_TIME_WATERJUMP) && (bs->cur_ps.pm_time > 0)) {
		initmove.or_moveflags |= MFL_WATERJUMP;
	}
	//set presence type
	if (bs->cur_ps.pm_flags & PMF_DUCKED) initmove.presencetype = PRESENCE_CROUCH;
	else initmove.presencetype = PRESENCE_NORMAL;
	//
	if (bs->walker > 0.5) initmove.or_moveflags |= MFL_WALK;
	//
	VectorCopy(&bs->viewangles, &initmove.viewangles);
	//
	trap->ai->BotInitMoveState(bs->ms, &initmove);
}

void BotCheckItemPickup(bot_state_t *bs, int *oldinventory) {
	qboolean offence = qfalse;

	if (gametype <= GT_TEAMBLOOD)
		return;

	// go into offence if picked up the kamikaze or invulnerability
	// if not already wearing the kamikaze or invulnerability
	if (!oldinventory[INVENTORY_GUARD] && bs->inventory[INVENTORY_GUARD] >= 1)
		offence = qtrue;

	if ( offence ) {
		if ( !(bs->teamtaskpreference & TEAMTP_ATTACKER) )
			bs->teamtaskpreference |= TEAMTP_ATTACKER;
		bs->teamtaskpreference &= ~TEAMTP_DEFENDER;
	}
	else {
		if ( !(bs->teamtaskpreference & TEAMTP_DEFENDER) ) {
			if ( g_difficulty->integer <= 3 ) {
				if ( bs->ltgtype != LTG_DEFENDKEYAREA ) {
					if ( (gametype != GT_FLAGS || (bs->redflagstatus == 0 && bs->blueflagstatus == 0)) && (gametype != GT_TROJAN || bs->neutralflagstatus == 0) ) {
						// tell the leader we want to be on defense
						BotVoiceChat(bs, -1, VOICECHAT_WANTONDEFENSE);
					}
				}
			}
			bs->teamtaskpreference |= TEAMTP_DEFENDER;
		}
		bs->teamtaskpreference &= ~TEAMTP_ATTACKER;
	}
}

void BotUpdateInventory(bot_state_t *bs) {
	int oldinventory[MAX_ITEMS];

	//RAZMARK: Adding new weapons
	memcpy(oldinventory, bs->inventory, sizeof(oldinventory));

	//armor
	bs->inventory[INVENTORY_ARMOR] = bs->cur_ps.stats[STAT_ARMOR];

	//weapons
	bs->inventory[INVENTORY_QUANTIZER]	= !!(bs->cur_ps.stats[STAT_WEAPONS] & (1<<WP_QUANTIZER));
	bs->inventory[INVENTORY_REPEATER]	= !!(bs->cur_ps.stats[STAT_WEAPONS] & (1<<WP_REPEATER));
	bs->inventory[INVENTORY_SPLICER]	= !!(bs->cur_ps.stats[STAT_WEAPONS] & (1<<WP_SPLICER));
	bs->inventory[INVENTORY_MORTAR]		= !!(bs->cur_ps.stats[STAT_WEAPONS] & (1<<WP_MORTAR));
	bs->inventory[INVENTORY_DIVERGENCE]	= !!(bs->cur_ps.stats[STAT_WEAPONS] & (1<<WP_DIVERGENCE));

	//ammo

	//powerups
	bs->inventory[INVENTORY_HEALTH] = bs->cur_ps.stats[STAT_HEALTH];
	bs->inventory[INVENTORY_MEDKIT] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM] == MODELINDEX_MEDKIT;
	bs->inventory[INVENTORY_QUAD] = bs->cur_ps.powerups[PW_QUAD] != 0;
	bs->inventory[INVENTORY_REGEN] = bs->cur_ps.powerups[PW_REGEN] != 0;
	bs->inventory[INVENTORY_GUARD] = bs->cur_ps.stats[STAT_PERSISTENT_POWERUP] == MODELINDEX_GUARD;
	bs->inventory[INVENTORY_REDFLAG] = bs->cur_ps.powerups[PW_REDFLAG] != 0;
	bs->inventory[INVENTORY_BLUEFLAG] = bs->cur_ps.powerups[PW_BLUEFLAG] != 0;
	bs->inventory[INVENTORY_NEUTRALFLAG] = bs->cur_ps.powerups[PW_NEUTRALFLAG] != 0;

	BotCheckItemPickup(bs, oldinventory);
}

void BotUpdateBattleInventory(bot_state_t *bs, int enemy) {
	vector3 dir;
	aas_entityinfo_t entinfo;

	BotEntityInfo(enemy, &entinfo);
	VectorSubtract(&entinfo.origin, &bs->origin, &dir);
	bs->inventory[ENEMY_HEIGHT] = (int) dir.z;
	dir.z = 0;
	bs->inventory[ENEMY_HORIZONTAL_DIST] = (int) VectorLength(&dir);
	//FIXME: add num visible enemies and num visible team mates to the inventory
}

void BotBattleUseItems(bot_state_t *bs) {
	if ( bs->inventory[INVENTORY_HEALTH] < 60 )
	{
		if ( bs->inventory[INVENTORY_MEDKIT] > 0 )
			trap->ea->EA_Use( bs->client );
	}
}

void BotSetTeleportTime(bot_state_t *bs) {
	if ((bs->cur_ps.eFlags ^ bs->last_eFlags) & EF_TELEPORT_BIT) {
		bs->teleport_time = FloatTime();
	}
	bs->last_eFlags = bs->cur_ps.eFlags;
}

qboolean BotIsDead(bot_state_t *bs) {
	return (bs->cur_ps.pm_type == PM_DEAD);
}

qboolean BotIsObserver(bot_state_t *bs) {
	char buf[MAX_INFO_STRING];
	if (bs->cur_ps.pm_type == PM_SPECTATOR) return qtrue;
	trap->SV_GetConfigstring(CS_PLAYERS+bs->client, buf, sizeof(buf));
	if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) return qtrue;
	return qfalse;
}

qboolean BotIntermission(bot_state_t *bs) {
	//NOTE: we shouldn't be looking at the game code...
	if (level.intermissiontime) return qtrue;
	return (bs->cur_ps.pm_type == PM_FREEZE || bs->cur_ps.pm_type == PM_INTERMISSION);
}

qboolean BotInLavaOrSlime(bot_state_t *bs) {
	vector3 feet;

	VectorCopy(&bs->origin, &feet);
	feet.z -= 23;
	return (trap->aas->AAS_PointContents(&feet) & (CONTENTS_LAVA|CONTENTS_SLIME));
}

bot_waypoint_t *BotCreateWayPoint(char *name, vector3 *origin, int areanum) {
	bot_waypoint_t *wp;
	vector3 waypointmins = {-8, -8, -8}, waypointmaxs = {8, 8, 8};

	wp = botai_freewaypoints;
	if ( !wp ) {
		BotAI_Print( PRT_WARNING, "BotCreateWayPoint: Out of waypoints\n" );
		return NULL;
	}
	botai_freewaypoints = botai_freewaypoints->next;

	Q_strncpyz( wp->name, name, sizeof(wp->name) );
	VectorCopy(origin, &wp->goal.origin);
	VectorCopy(&waypointmins, &wp->goal.mins);
	VectorCopy(&waypointmaxs, &wp->goal.maxs);
	wp->goal.areanum = areanum;
	wp->next = NULL;
	wp->prev = NULL;
	return wp;
}

bot_waypoint_t *BotFindWayPoint(bot_waypoint_t *waypoints, char *name) {
	bot_waypoint_t *wp;

	for (wp = waypoints; wp; wp = wp->next) {
		if (!Q_stricmp(wp->name, name)) return wp;
	}
	return NULL;
}

void BotFreeWaypoints(bot_waypoint_t *wp) {
	bot_waypoint_t *nextwp;

	for (; wp; wp = nextwp) {
		nextwp = wp->next;
		wp->next = botai_freewaypoints;
		botai_freewaypoints = wp;
	}
}

void BotInitWaypoints( void ) {
	int i;

	botai_freewaypoints = NULL;
	for (i = 0; i < MAX_WAYPOINTS; i++) {
		botai_waypoints[i].next = botai_freewaypoints;
		botai_freewaypoints = &botai_waypoints[i];
	}
}

int TeamPlayIsOn( void ) {
	return ( gametype >= GT_TEAMBLOOD );
}

float BotAggression(bot_state_t *bs) {
	//RAZMARK: Adding new weapons
	//if the bot has quad
	if ( bs->inventory[INVENTORY_QUAD] )
		return 70;
	//if the enemy is located way higher than the bot
	if (bs->inventory[ENEMY_HEIGHT] > 200) return 0;
	//if the bot is very low on health
	if (bs->inventory[INVENTORY_HEALTH] < 60) return 0;
	//if the bot is low on health
	if (bs->inventory[INVENTORY_HEALTH] < 80) {
		//if the bot has insufficient armor
		if (bs->inventory[INVENTORY_ARMOR] < 40) return 0;
	}

	//if the bot can use the divergence
	if (bs->inventory[INVENTORY_DIVERGENCE] > 0 &&
			bs->inventory[INVENTORY_AMMODIVERGENCE] > 5) return 95;

	//if the bot can use the repeater
	if (bs->inventory[INVENTORY_REPEATER] > 0 &&
			bs->inventory[INVENTORY_AMMOREPEATER] > 40) return 85;

	//otherwise the bot is not feeling too good
	return 0;
}

float BotFeelingBad(bot_state_t *bs) {
	//RAZMARK: Adding new weapons
	if (bs->inventory[INVENTORY_HEALTH] < 40) {
		return 100;
	}
	if (bs->weaponnum == WP_QUANTIZER) {
		return 80;
	}
	if (bs->inventory[INVENTORY_HEALTH] < 60) {
		return 70;
	}
	return 0;
}

int BotWantsToRetreat(bot_state_t *bs) {
	aas_entityinfo_t entinfo;

	if (gametype == GT_FLAGS) {
		//always retreat when carrying a CTF flag
		if (BotCTFCarryingFlag(bs))
			return qtrue;
	}
	else if (gametype == GT_TROJAN) {
		//if carrying the flag then always retreat
		if (BotTrojanCarryingFlag(bs))
			return qtrue;
	}
	//
	if (bs->enemy >= 0) {
		BotEntityInfo(bs->enemy, &entinfo);
		// if the enemy is carrying a flag
		if (EntityCarriesFlag(&entinfo)) return qfalse;
	}
	//if the bot is getting the flag
	if (bs->ltgtype == LTG_GETFLAG)
		return qtrue;
	//
	if (BotAggression(bs) < 50)
		return qtrue;
	return qfalse;
}

int BotWantsToChase(bot_state_t *bs) {
	aas_entityinfo_t entinfo;

	if (gametype == GT_FLAGS) {
		//never chase when carrying a CTF flag
		if (BotCTFCarryingFlag(bs))
			return qfalse;
		//always chase if the enemy is carrying a flag
		BotEntityInfo(bs->enemy, &entinfo);
		if (EntityCarriesFlag(&entinfo))
			return qtrue;
	}
	else if (gametype == GT_TROJAN) {
		//never chase if carrying the flag
		if (BotTrojanCarryingFlag(bs))
			return qfalse;
		//always chase if the enemy is carrying a flag
		BotEntityInfo(bs->enemy, &entinfo);
		if (EntityCarriesFlag(&entinfo))
			return qtrue;
	}
	//if the bot is getting the flag
	if (bs->ltgtype == LTG_GETFLAG)
		return qfalse;
	//
	if (BotAggression(bs) > 50)
		return qtrue;
	return qfalse;
}

int BotWantsToHelp(bot_state_t *bs) {
	return qtrue;
}

int BotCanAndWantsToRocketJump(bot_state_t *bs) {
	float rocketjumper;

	//RAZMARK: Adding new weapons (rocket jumping)

	//if rocket jumping is disabled
	if (!bot_rocketjump->integer) return qfalse;
	//if no rocket launcher
	if (bs->inventory[INVENTORY_QUANTIZER] <= 0) return qfalse;
	//never rocket jump with the Quad
	if (bs->inventory[INVENTORY_QUAD]) return qfalse;
	//if low on health
	if (bs->inventory[INVENTORY_HEALTH] < 60) return qfalse;
	//if not full health
	if (bs->inventory[INVENTORY_HEALTH] < 90) {
		//if the bot has insufficient armor
		if (bs->inventory[INVENTORY_ARMOR] < 40) return qfalse;
	}
	rocketjumper = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_WEAPONJUMPING, 0, 1);
	if (rocketjumper < 0.5) return qfalse;
	return qtrue;
}

int BotHasPersistantPowerupAndWeapon(bot_state_t *bs) {
	// if the bot does not have a persistent powerup
	if ( !bs->inventory[INVENTORY_GUARD] )
		return qfalse;

	//if the bot is very low on health
	if (bs->inventory[INVENTORY_HEALTH] < 60) return qfalse;
	//if the bot is low on health
	if (bs->inventory[INVENTORY_HEALTH] < 80) {
		//if the bot has insufficient armor
		if (bs->inventory[INVENTORY_ARMOR] < 40) return qfalse;
	}

	//RAZMARK: Adding new weapons
	//if the bot can use the divergence
	if (bs->inventory[INVENTORY_DIVERGENCE] > 0 &&
			bs->inventory[INVENTORY_AMMODIVERGENCE] > 5) return qtrue;
	//if the bot can use the splicer
	if (bs->inventory[INVENTORY_SPLICER] > 0 &&
			bs->inventory[INVENTORY_AMMOSPLICER] > 50) return qtrue;
	return qfalse;
}

void BotGoCamp(bot_state_t *bs, bot_goal_t *goal) {
	float camper;

	bs->decisionmaker = bs->client;
	//set message time to zero so bot will NOT show any message
	bs->teammessage_time = 0;
	//set the ltg type
	bs->ltgtype = LTG_CAMP;
	//set the team goal
	memcpy(&bs->teamgoal, goal, sizeof(bot_goal_t));
	//get the team goal time
	camper = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CAMPER, 0, 1);
	if (camper > 0.99) bs->teamgoal_time = FloatTime() + 99999;
	else bs->teamgoal_time = FloatTime() + 120 + 180 * camper + random() * 15;
	//set the last time the bot started camping
	bs->camp_time = FloatTime();
	//the teammate that requested the camping
	bs->teammate = 0;
	//do NOT type arrive message
	bs->arrive_time = 1;
}

int BotWantsToCamp(bot_state_t *bs) {
	float camper;
	int cs, traveltime, besttraveltime;
	bot_goal_t goal, bestgoal;

	camper = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CAMPER, 0, 1);
	if (camper < 0.1) return qfalse;
	//if the bot has a team goal
	if (bs->ltgtype == LTG_TEAMHELP ||
			bs->ltgtype == LTG_TEAMACCOMPANY ||
			bs->ltgtype == LTG_DEFENDKEYAREA ||
			bs->ltgtype == LTG_GETFLAG ||
			bs->ltgtype == LTG_RUSHBASE ||
			bs->ltgtype == LTG_CAMP ||
			bs->ltgtype == LTG_CAMPORDER ||
			bs->ltgtype == LTG_PATROL) {
		return qfalse;
	}
	//if camped recently
	if (bs->camp_time > FloatTime() - 60 + 300 * (1-camper)) return qfalse;
	//
	if (random() > camper) {
		bs->camp_time = FloatTime();
		return qfalse;
	}
	//if the bot isn't healthy enough
	if (BotAggression(bs) < 50) return qfalse;

	//RAZMARK: Adding new weapons
	//the bot should have at least have the rocket launcher, the railgun or the bfg10k with some ammo
	if ( bs->inventory[INVENTORY_DIVERGENCE] <= 0 || bs->inventory[INVENTORY_AMMODIVERGENCE] < 10 )
		return qfalse;

	//find the closest camp spot
	besttraveltime = 99999;
	for (cs = trap->ai->BotGetNextCampSpotGoal(0, &goal); cs; cs = trap->ai->BotGetNextCampSpotGoal(cs, &goal)) {
		traveltime = trap->aas->AAS_AreaTravelTimeToGoalArea(bs->areanum, &bs->origin, goal.areanum, TFL_DEFAULT);
		if (traveltime && traveltime < besttraveltime) {
			besttraveltime = traveltime;
			memcpy(&bestgoal, &goal, sizeof(bot_goal_t));
		}
	}
	if (besttraveltime > 150) return qfalse;
	//ok found a camp spot, go camp there
	BotGoCamp(bs, &bestgoal);
	bs->ordered = qfalse;
	//
	return qtrue;
}

void BotDontAvoid(bot_state_t *bs, char *itemname) {
	bot_goal_t goal;
	int num;

	num = trap->ai->BotGetLevelItemGoal(-1, itemname, &goal);
	while(num >= 0) {
		trap->ai->BotRemoveFromAvoidGoals(bs->gs, goal.number);
		num = trap->ai->BotGetLevelItemGoal(num, itemname, &goal);
	}
}

void BotGoForPowerups(bot_state_t *bs) {

	//don't avoid any of the powerups anymore
	BotDontAvoid(bs, "Quad Damage");
	BotDontAvoid(bs, "Regeneration");
	BotDontAvoid(bs, "Battle Suit");
	BotDontAvoid(bs, "Speed");
	BotDontAvoid(bs, "Invisibility");
	//BotDontAvoid(bs, "Flight");
	//reset the long term goal time so the bot will go for the powerup
	//NOTE: the long term goal type doesn't change
	bs->ltg_time = 0;
}

void BotRoamGoal(bot_state_t *bs, vector3 *goal) {
	int pc, i;
	float len, rnd;
	vector3 dir, bestorg, belowbestorg;
	bsp_trace_t trace;

	for (i = 0; i < 10; i++) {
		//start at the bot origin
		VectorCopy(&bs->origin, &bestorg);
		rnd = random();
		if (rnd > 0.25) {
			//add a random value to the x-coordinate
			if (random() < 0.5) bestorg.x -= 800 * random() + 100;
			else bestorg.x += 800 * random() + 100;
		}
		if (rnd < 0.75) {
			//add a random value to the y-coordinate
			if (random() < 0.5) bestorg.y -= 800 * random() + 100;
			else bestorg.y += 800 * random() + 100;
		}
		//add a random value to the z-coordinate (NOTE: 48 = maxjump?)
		bestorg.z += 2 * 48 * crandom();
		//trace a line from the origin to the roam target
		BotAI_Trace(&trace, &bs->origin, NULL, NULL, &bestorg, bs->entitynum, MASK_SOLID);
		//direction and length towards the roam target
		VectorSubtract(&trace.endpos, &bs->origin, &dir);
		len = VectorNormalize(&dir);
		//if the roam target is far away enough
		if (len > 200) {
			//the roam target is in the given direction before walls
			VectorScale(&dir, len * trace.fraction - 40, &dir);
			VectorAdd(&bs->origin, &dir, &bestorg);
			//get the coordinates of the floor below the roam target
			belowbestorg.x = bestorg.x;
			belowbestorg.y = bestorg.y;
			belowbestorg.z = bestorg.z - 800;
			BotAI_Trace(&trace, &bestorg, NULL, NULL, &belowbestorg, bs->entitynum, MASK_SOLID);
			//
			if (!trace.startsolid) {
				trace.endpos.z++;
				pc = trap->SV_PointContents(&trace.endpos, bs->entitynum);
				if (!(pc & (CONTENTS_LAVA | CONTENTS_SLIME))) {
					VectorCopy(&bestorg, goal);
					return;
				}
			}
		}
	}
	VectorCopy(&bestorg, goal);
}

bot_moveresult_t BotAttackMove(bot_state_t *bs, int tfl) {
	int movetype, i, attackentity;
	float attack_skill, jumper, croucher, dist, strafechange_time;
	float attack_dist, attack_range;
	vector3 forward, backward, sideward, hordir, up = {0, 0, 1};
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	bot_goal_t goal;

	attackentity = bs->enemy;
	//
	if (bs->attackchase_time > FloatTime()) {
		//create the chase goal
		goal.entitynum = attackentity;
		goal.areanum = bs->lastenemyareanum;
		VectorCopy(&bs->lastenemyorigin, &goal.origin);
		VectorSet(&goal.mins, -8, -8, -8);
		VectorSet(&goal.maxs, 8, 8, 8);
		//initialize the movement state
		BotSetupForMovement(bs);
		//move towards the goal
		trap->ai->BotMoveToGoal(&moveresult, bs->ms, &goal, tfl);
		return moveresult;
	}
	//
	memset(&moveresult, 0, sizeof(bot_moveresult_t));
	//
	attack_skill = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_ATTACK_SKILL, 0, 1);
	jumper = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_JUMPER, 0, 1);
	croucher = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CROUCHER, 0, 1);
	//if the bot is really stupid
	if (attack_skill < 0.2) return moveresult;
	//initialize the movement state
	BotSetupForMovement(bs);
	//get the enemy entity info
	BotEntityInfo(attackentity, &entinfo);
	//direction towards the enemy
	VectorSubtract(&entinfo.origin, &bs->origin, &forward);
	//the distance towards the enemy
	dist = VectorNormalize(&forward);
	VectorNegate(&forward, &backward);
	//walk, crouch or jump
	movetype = MOVE_WALK;
	//
	if (bs->attackcrouch_time < FloatTime() - 1) {
		if (random() < jumper) {
			movetype = MOVE_JUMP;
		}
		//wait at least one second before crouching again
		else if (bs->attackcrouch_time < FloatTime() - 1 && random() < croucher) {
			bs->attackcrouch_time = FloatTime() + croucher * 5;
		}
	}
	if (bs->attackcrouch_time > FloatTime()) movetype = MOVE_CROUCH;
	//if the bot should jump
	if (movetype == MOVE_JUMP) {
		//if jumped last frame
		if (bs->attackjump_time > FloatTime()) {
			movetype = MOVE_WALK;
		}
		else {
			bs->attackjump_time = FloatTime() + 1;
		}
	}

	attack_dist = IDEAL_ATTACKDIST;
	attack_range = 40;

	//if the bot is stupid
	if (attack_skill <= 0.4) {
		//just walk to or away from the enemy
		if (dist > attack_dist + attack_range) {
			if (trap->ai->BotMoveInDirection(bs->ms, &forward, 400, movetype)) return moveresult;
		}
		if (dist < attack_dist - attack_range) {
			if (trap->ai->BotMoveInDirection(bs->ms, &backward, 400, movetype)) return moveresult;
		}
		return moveresult;
	}
	//increase the strafe time
	bs->attackstrafe_time += bs->thinktime;
	//get the strafe change time
	strafechange_time = 0.4f + (1 - attack_skill) * 0.2f;
	if (attack_skill > 0.7f) strafechange_time += crandom() * 0.2f;
	//if the strafe direction should be changed
	if (bs->attackstrafe_time > strafechange_time) {
		//some magic number :)
		if (random() > 0.935f) {
			//flip the strafe direction
			bs->flags ^= BFL_STRAFERIGHT;
			bs->attackstrafe_time = 0;
		}
	}
	//
	for (i = 0; i < 2; i++) {
		hordir.x = forward.x;
		hordir.y = forward.y;
		hordir.z = 0;
		VectorNormalize(&hordir);
		//get the sideward vector
		CrossProduct(&hordir, &up, &sideward);
		//reverse the vector depending on the strafe direction
		if (bs->flags & BFL_STRAFERIGHT) VectorNegate(&sideward, &sideward);
		//randomly go back a little
		if (random() > 0.9) {
			VectorAdd(&sideward, &backward, &sideward);
		}
		else {
			//walk forward or backward to get at the ideal attack distance
			if (dist > attack_dist + attack_range) {
				VectorAdd(&sideward, &forward, &sideward);
			}
			else if (dist < attack_dist - attack_range) {
				VectorAdd(&sideward, &backward, &sideward);
			}
		}
		//perform the movement
		if (trap->ai->BotMoveInDirection(bs->ms, &sideward, 400, movetype))
			return moveresult;
		//movement failed, flip the strafe direction
		bs->flags ^= BFL_STRAFERIGHT;
		bs->attackstrafe_time = 0;
	}
	//bot couldn't do any usefull movement
//	bs->attackchase_time = AAS_Time() + 6;
	return moveresult;
}

int BotSameTeam(bot_state_t *bs, int entnum) {

	if (bs->client < 0 || bs->client >= MAX_CLIENTS) {
		return qfalse;
	}

	if (entnum < 0 || entnum >= MAX_CLIENTS) {
		return qfalse;
	}

	if (gametype >= GT_TEAMBLOOD) {
		if (level.clients[bs->client].sess.sessionTeam == level.clients[entnum].sess.sessionTeam) return qtrue;
	}

	return qfalse;
}

qboolean InFieldOfVision(vector3 *viewangles, float fov, vector3 *angles) {
	int i;
	float diff, angle;

	for (i = 0; i < 2; i++) {
		angle = AngleMod(viewangles->data[i]);
		angles->data[i] = AngleMod(angles->data[i]);
		diff = angles->data[i] - angle;
		if (angles->data[i] > angle) {
			if (diff > 180.0) diff -= 360.0;
		}
		else {
			if (diff < -180.0) diff += 360.0;
		}
		if (diff > 0) {
			if (diff > fov * 0.5) return qfalse;
		}
		else {
			if (diff < -fov * 0.5) return qfalse;
		}
	}
	return qtrue;
}

// returns visibility in the range [0, 1] taking fog and water surfaces into account
float BotEntityVisible(int viewer, vector3 *eye, vector3 *viewangles, float fov, int ent) {
	int i, contents_mask, passent, hitent, infog, inwater, otherinfog, pc;
	float squaredfogdist, waterfactor, vis, bestvis;
	bsp_trace_t trace;
	aas_entityinfo_t entinfo;
	vector3 dir, entangles, start, end, middle;

	//calculate middle of bounding box
	BotEntityInfo(ent, &entinfo);
	VectorAdd(&entinfo.mins, &entinfo.maxs, &middle);
	VectorScale(&middle, 0.5, &middle);
	VectorAdd(&entinfo.origin, &middle, &middle);
	//check if entity is within field of vision
	VectorSubtract(&middle, eye, &dir);
	vectoangles(&dir, &entangles);
	if (!InFieldOfVision(viewangles, fov, &entangles)) return 0;
	//
	pc = trap->aas->AAS_PointContents(eye);
	infog = (pc & CONTENTS_FOG);
	inwater = (pc & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER));
	//
	bestvis = 0;
	for (i = 0; i < 3; i++) {
		//if the point is not in potential visible sight
		//if (!AAS_inPVS(eye, middle)) continue;
		//
		contents_mask = CONTENTS_SOLID|CONTENTS_PLAYERCLIP;
		passent = viewer;
		hitent = ent;
		VectorCopy(eye, &start);
		VectorCopy(&middle, &end);
		//if the entity is in water, lava or slime
		if (trap->aas->AAS_PointContents(&middle) & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER)) {
			contents_mask |= (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER);
		}
		//if eye is in water, lava or slime
		if (inwater) {
			if (!(contents_mask & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER))) {
				passent = ent;
				hitent = viewer;
				VectorCopy(&middle, &start);
				VectorCopy(eye, &end);
			}
			contents_mask ^= (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER);
		}
		//trace from start to end
		BotAI_Trace(&trace, &start, NULL, NULL, &end, passent, contents_mask);
		//if water was hit
		waterfactor = 1.0;
		if (trace.contents & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER)) {
			//if the water surface is translucent
			if (1) {
				//trace through the water
				contents_mask &= ~(CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER);
				BotAI_Trace(&trace, &trace.endpos, NULL, NULL, &end, passent, contents_mask);
				waterfactor = 0.5;
			}
		}
		//if a full trace or the hitent was hit
		if (trace.fraction >= 1 || trace.ent == hitent) {
			//check for fog, assuming there's only one fog brush where
			//either the viewer or the entity is in or both are in
			otherinfog = (trap->aas->AAS_PointContents(&middle) & CONTENTS_FOG);
			if (infog && otherinfog) {
				VectorSubtract(&trace.endpos, eye, &dir);
				squaredfogdist = VectorLengthSquared(&dir);
			}
			else if (infog) {
				VectorCopy(&trace.endpos, &start);
				BotAI_Trace(&trace, &start, NULL, NULL, eye, viewer, CONTENTS_FOG);
				VectorSubtract(eye, &trace.endpos, &dir);
				squaredfogdist = VectorLengthSquared(&dir);
			}
			else if (otherinfog) {
				VectorCopy(&trace.endpos, &end);
				BotAI_Trace(&trace, eye, NULL, NULL, &end, viewer, CONTENTS_FOG);
				VectorSubtract(&end, &trace.endpos, &dir);
				squaredfogdist = VectorLengthSquared(&dir);
			}
			else {
				//if the entity and the viewer are not in fog assume there's no fog in between
				squaredfogdist = 0;
			}
			//decrease visibility with the view distance through fog
			vis = 1 / ((squaredfogdist * 0.001f) < 1 ? 1 : (squaredfogdist * 0.001f));
			//if entering water visibility is reduced
			vis *= waterfactor;
			//
			if (vis > bestvis) bestvis = vis;
			//if pretty much no fog
			if (bestvis >= 0.95) return bestvis;
		}
		//check bottom and top of bounding box as well
		if (i == 0) middle.z += entinfo.mins.z;
		else if (i == 1) middle.z += entinfo.maxs.z - entinfo.mins.z;
	}
	return bestvis;
}

int BotFindEnemy(bot_state_t *bs, int curenemy) {
	int i, healthdecrease;
	float f, alertness, easyfragger, vis;
	float squaredist, cursquaredist;
	aas_entityinfo_t entinfo, curenemyinfo;
	vector3 dir, angles;

	alertness = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_ALERTNESS, 0, 1);
	easyfragger = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_EASY_FRAGGER, 0, 1);
	//check if the health decreased
	healthdecrease = bs->lasthealth > bs->inventory[INVENTORY_HEALTH];
	//remember the current health value
	bs->lasthealth = bs->inventory[INVENTORY_HEALTH];
	//
	if (curenemy >= 0) {
		BotEntityInfo(curenemy, &curenemyinfo);
		if (EntityCarriesFlag(&curenemyinfo)) return qfalse;
		VectorSubtract(&curenemyinfo.origin, &bs->origin, &dir);
		cursquaredist = VectorLengthSquared(&dir);
	}
	else {
		cursquaredist = 0;
	}

	//
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {

		if (i == bs->client) continue;
		//if it's the current enemy
		if (i == curenemy) continue;
		//
		BotEntityInfo(i, &entinfo);
		//
		if (!entinfo.valid) continue;
		//if the enemy isn't dead and the enemy isn't the bot self
		if (EntityIsDead(&entinfo) || entinfo.number == bs->entitynum) continue;
		//if the enemy is invisible and not shooting
		if (EntityIsInvisible(&entinfo) && !EntityIsShooting(&entinfo)) {
			continue;
		}
		//if not an easy fragger don't shoot at chatting players
		if (easyfragger < 0.5 && EntityIsChatting(&entinfo)) continue;
		//
		if (lastteleport_time > FloatTime() - 3) {
			VectorSubtract(&entinfo.origin, &lastteleport_origin, &dir);
			if (VectorLengthSquared(&dir) < Square(70)) continue;
		}
		//calculate the distance towards the enemy
		VectorSubtract(&entinfo.origin, &bs->origin, &dir);
		squaredist = VectorLengthSquared(&dir);
		//if this entity is not carrying a flag
		if (!EntityCarriesFlag(&entinfo))
		{
			//if this enemy is further away than the current one
			if (curenemy >= 0 && squaredist > cursquaredist) continue;
		} //end if
		//if the bot has no
		if (squaredist > Square(900.0 + alertness * 4000.0)) continue;
		//if on the same team
		if (BotSameTeam(bs, i)) continue;
		//if the bot's health decreased or the enemy is shooting
		if (curenemy < 0 && (healthdecrease || EntityIsShooting(&entinfo)))
			f = 360;
		else
			f = 90 + 90 - (90 - (squaredist > Square(810) ? Square(810) : squaredist) / (810 * 9));
		//check if the enemy is visible
		vis = BotEntityVisible(bs->entitynum, &bs->eye, &bs->viewangles, f, i);
		if (vis <= 0) continue;
		//if the enemy is quite far away, not shooting and the bot is not damaged
		if (curenemy < 0 && squaredist > Square(100) && !healthdecrease && !EntityIsShooting(&entinfo))
		{
			//check if we can avoid this enemy
			VectorSubtract(&bs->origin, &entinfo.origin, &dir);
			vectoangles(&dir, &angles);
			//if the bot isn't in the fov of the enemy
			if (!InFieldOfVision(&entinfo.angles, 90, &angles)) {
				//update some stuff for this enemy
				BotUpdateBattleInventory(bs, i);
				//if the bot doesn't really want to fight
				if (BotWantsToRetreat(bs)) continue;
			}
		}
		//found an enemy
		bs->enemy = entinfo.number;
		if (curenemy >= 0) bs->enemysight_time = FloatTime() - 2;
		else bs->enemysight_time = FloatTime();
		bs->enemysuicide = qfalse;
		bs->enemydeath_time = 0;
		bs->enemyvisible_time = FloatTime();
		return qtrue;
	}
	return qfalse;
}

int BotTeamFlagCarrierVisible(bot_state_t *bs) {
	int i;
	float vis;
	aas_entityinfo_t entinfo;

	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		if (i == bs->client)
			continue;
		//
		BotEntityInfo(i, &entinfo);
		//if this player is active
		if (!entinfo.valid)
			continue;
		//if this player is carrying a flag
		if (!EntityCarriesFlag(&entinfo))
			continue;
		//if the flag carrier is not on the same team
		if (!BotSameTeam(bs, i))
			continue;
		//if the flag carrier is not visible
		vis = BotEntityVisible(bs->entitynum, &bs->eye, &bs->viewangles, 360, i);
		if (vis <= 0)
			continue;
		//
		return i;
	}
	return -1;
}

int BotTeamFlagCarrier(bot_state_t *bs) {
	int i;
	aas_entityinfo_t entinfo;

	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		if (i == bs->client)
			continue;
		//
		BotEntityInfo(i, &entinfo);
		//if this player is active
		if (!entinfo.valid)
			continue;
		//if this player is carrying a flag
		if (!EntityCarriesFlag(&entinfo))
			continue;
		//if the flag carrier is not on the same team
		if (!BotSameTeam(bs, i))
			continue;
		//
		return i;
	}
	return -1;
}

int BotEnemyFlagCarrierVisible(bot_state_t *bs) {
	int i;
	float vis;
	aas_entityinfo_t entinfo;

	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		if (i == bs->client)
			continue;
		//
		BotEntityInfo(i, &entinfo);
		//if this player is active
		if (!entinfo.valid)
			continue;
		//if this player is carrying a flag
		if (!EntityCarriesFlag(&entinfo))
			continue;
		//if the flag carrier is on the same team
		if (BotSameTeam(bs, i))
			continue;
		//if the flag carrier is not visible
		vis = BotEntityVisible(bs->entitynum, &bs->eye, &bs->viewangles, 360, i);
		if (vis <= 0)
			continue;
		//
		return i;
	}
	return -1;
}

void BotVisibleTeamMatesAndEnemies(bot_state_t *bs, int *teammates, int *enemies, float range) {
	int i;
	float vis;
	aas_entityinfo_t entinfo;
	vector3 dir;

	if (teammates)
		*teammates = 0;
	if (enemies)
		*enemies = 0;
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		if (i == bs->client)
			continue;
		//
		BotEntityInfo(i, &entinfo);
		//if this player is active
		if (!entinfo.valid)
			continue;
		//if this player is carrying a flag
		if (!EntityCarriesFlag(&entinfo))
			continue;
		//if not within range
		VectorSubtract(&entinfo.origin, &bs->origin, &dir);
		if (VectorLengthSquared(&dir) > Square(range))
			continue;
		//if the flag carrier is not visible
		vis = BotEntityVisible(bs->entitynum, &bs->eye, &bs->viewangles, 360, i);
		if (vis <= 0)
			continue;
		//if the flag carrier is on the same team
		if (BotSameTeam(bs, i)) {
			if (teammates)
				(*teammates)++;
		}
		else {
			if (enemies)
				(*enemies)++;
		}
	}
}

void BotAimAtEnemy(bot_state_t *bs) {
	int i, enemyvisible;
	float dist, f, aim_skill, aim_accuracy, speed, reactiontime;
	vector3 dir, bestorigin, end, start, groundtarget, cmdmove, enemyvelocity;
	vector3 mins = {-4,-4,-4}, maxs = {4, 4, 4};
	weaponinfo_t wi;
	aas_entityinfo_t entinfo;
	bot_goal_t goal;
	bsp_trace_t trace;
	vector3 target;

	//if the bot has no enemy
	if (bs->enemy < 0) {
		return;
	}
	//get the enemy entity information
	BotEntityInfo(bs->enemy, &entinfo);
	//if this is not a player (should be an obelisk)
	if (bs->enemy >= MAX_CLIENTS) {
		//if the obelisk is visible
		VectorCopy(&entinfo.origin, &target);
		// if attacking an obelisk
		if ( bs->enemy == redobelisk.entitynum ||
			bs->enemy == blueobelisk.entitynum ) {
			target.z += 32;
		}
		//aim at the obelisk
		VectorSubtract(&target, &bs->eye, &dir);
		vectoangles(&dir, &bs->ideal_viewangles);
		//set the aim target before trying to attack
		VectorCopy(&target, &bs->aimtarget);
		return;
	}
	//
	//BotAI_Print(PRT_MESSAGE, "client %d: aiming at client %d\n", bs->entitynum, bs->enemy);
	//
	aim_skill = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_SKILL, 0, 1);
	aim_accuracy = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY, 0, 1);
	//
	if (aim_skill > 0.95f) {
		//don't aim too early
		reactiontime = 0.5f * trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_REACTIONTIME, 0, 1);
		if (bs->enemysight_time > FloatTime() - reactiontime) return;
		if (bs->teleport_time > FloatTime() - reactiontime) return;
	}

	//get the weapon information
	trap->ai->BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);

	//RAZMARK: Adding new weapons
	//get the weapon specific aim accuracy and or aim skill
//	if ( wi.number == WP_MACHINEGUN )
//		aim_accuracy = trap->Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_MACHINEGUN, 0, 1);

	aim_accuracy = 0.46f;

	//
	if (aim_accuracy <= 0) aim_accuracy = 0.0001f;
	//get the enemy entity information
	BotEntityInfo(bs->enemy, &entinfo);
	//if the enemy is invisible then shoot crappy most of the time
	if (EntityIsInvisible(&entinfo)) {
		if (random() > 0.1f) aim_accuracy *= 0.4f;
	}
	//
	VectorSubtract(&entinfo.origin, &entinfo.lastvisorigin, &enemyvelocity);
	VectorScale(&enemyvelocity, 1 / entinfo.update_time, &enemyvelocity);
	//enemy origin and velocity is remembered every 0.5 seconds
	if (bs->enemyposition_time < FloatTime()) {
		//
		bs->enemyposition_time = FloatTime() + 0.5f;
		VectorCopy(&enemyvelocity, &bs->enemyvelocity);
		VectorCopy(&entinfo.origin, &bs->enemyorigin);
	}
	//if not extremely skilled
	if (aim_skill < 0.9f) {
		VectorSubtract(&entinfo.origin, &bs->enemyorigin, &dir);
		//if the enemy moved a bit
		if (VectorLengthSquared(&dir) > Square(48)) {
			//if the enemy changed direction
			if (DotProduct(&bs->enemyvelocity, &enemyvelocity) < 0) {
				//aim accuracy should be worse now
				aim_accuracy *= 0.7f;
			}
		}
	}
	//check visibility of enemy
	enemyvisible = BotEntityVisible(bs->entitynum, &bs->eye, &bs->viewangles, 360.0f, bs->enemy) > 0;
	//if the enemy is visible
	if (enemyvisible) {
		//
		VectorCopy(&entinfo.origin, &bestorigin);
		bestorigin.z += 8;
		//get the start point shooting from
		//NOTE: the x and y projectile start offsets are ignored
		VectorCopy(&bs->origin, &start);
		start.z += bs->cur_ps.viewheight;
		start.z += wi.offset.z;
		//
		BotAI_Trace(&trace, &start, &mins, &maxs, &bestorigin, bs->entitynum, MASK_SHOT);
		//if the enemy is NOT hit
		if (trace.fraction <= 1 && trace.ent != entinfo.number) {
			bestorigin.z += 16;
		}
		//if it is not an instant hit weapon the bot might want to predict the enemy
		if (wi.speed) {
			//
			VectorSubtract(&bestorigin, &bs->origin, &dir);
			dist = VectorLength(&dir);
			VectorSubtract(&entinfo.origin, &bs->enemyorigin, &dir);
			//if the enemy is NOT pretty far away and strafing just small steps left and right
			if (!(dist > 100 && VectorLengthSquared(&dir) < Square(32))) {
				//if skilled enough do exact prediction
				if (aim_skill > 0.8 &&
					//if the weapon is ready to fire
					bs->cur_ps.weaponstate == WEAPON_READY) {
						aas_clientmove_t move;
						vector3 origin;

						VectorSubtract(&entinfo.origin, &bs->origin, &dir);
						//distance towards the enemy
						dist = VectorLength(&dir);
						//direction the enemy is moving in
						VectorSubtract(&entinfo.origin, &entinfo.lastvisorigin, &dir);
						//
						VectorScale(&dir, 1 / entinfo.update_time, &dir);
						//
						VectorCopy(&entinfo.origin, &origin);
						origin.z += 1;
						//
						VectorClear(&cmdmove);
						//AAS_ClearShownDebugLines();
						trap->aas->AAS_PredictClientMovement(&move, bs->enemy, &origin,
							PRESENCE_CROUCH, qfalse,
							&dir, &cmdmove, 0,
							(int)(dist * 10 / wi.speed), 0.1f, 0, 0, qfalse);
						VectorCopy(&move.endpos, &bestorigin);
						//BotAI_Print(PRT_MESSAGE, "%1.1f predicted speed = %f, frames = %f\n", FloatTime(), VectorLength(dir), dist * 10 / wi.speed);
				}
				//if not that skilled do linear prediction
				else if (aim_skill > 0.4) {
					VectorSubtract(&entinfo.origin, &bs->origin, &dir);
					//distance towards the enemy
					dist = VectorLength(&dir);
					//direction the enemy is moving in
					VectorSubtract(&entinfo.origin, &entinfo.lastvisorigin, &dir);
					dir.z = 0;
					//
					speed = VectorNormalize(&dir) / entinfo.update_time;
					//botimport.Print(PRT_MESSAGE, "speed = %f, wi->speed = %f\n", speed, wi->speed);
					//best spot to aim at
					VectorMA(&entinfo.origin, (dist / wi.speed) * speed, &dir, &bestorigin);
				}
			}
		}
		//if the projectile does radial damage
		if (aim_skill > 0.6 && wi.proj.damagetype & DAMAGETYPE_RADIAL) {
			//if the enemy isn't standing significantly higher than the bot
			if (entinfo.origin.z < bs->origin.z + 16) {
				//try to aim at the ground in front of the enemy
				VectorCopy(&entinfo.origin, &end);
				end.z -= 64;
				BotAI_Trace(&trace, &entinfo.origin, NULL, NULL, &end, entinfo.number, MASK_SHOT);
				//
				VectorCopy(&bestorigin, &groundtarget);
				if (trace.startsolid) groundtarget.z = entinfo.origin.z - 16;
				else groundtarget.z = trace.endpos.z - 8;
				//trace a line from projectile start to ground target
				BotAI_Trace(&trace, &start, NULL, NULL, &groundtarget, bs->entitynum, MASK_SHOT);
				//if hitpoint is not vertically too far from the ground target
				if (fabs(trace.endpos.z - groundtarget.z) < 50) {
					VectorSubtract(&trace.endpos, &groundtarget, &dir);
					//if the hitpoint is near enough the ground target
					if (VectorLengthSquared(&dir) < Square(60)) {
						VectorSubtract(&trace.endpos, &start, &dir);
						//if the hitpoint is far enough from the bot
						if (VectorLengthSquared(&dir) > Square(100)) {
							//check if the bot is visible from the ground target
							trace.endpos.z += 1;
							BotAI_Trace(&trace, &trace.endpos, NULL, NULL, &entinfo.origin, entinfo.number, MASK_SHOT);
							if (trace.fraction >= 1) {
								//botimport.Print(PRT_MESSAGE, "%1.1f aiming at ground\n", AAS_Time());
								VectorCopy(&groundtarget, &bestorigin);
							}
						}
					}
				}
			}
		}
		bestorigin.x += 20 * crandom() * (1 - aim_accuracy);
		bestorigin.y += 20 * crandom() * (1 - aim_accuracy);
		bestorigin.z += 10 * crandom() * (1 - aim_accuracy);
	}
	else {
		//
		VectorCopy(&bs->lastenemyorigin, &bestorigin);
		bestorigin.z += 8;
		//if the bot is skilled enough
		if (aim_skill > 0.5) {
			//do prediction shots around corners
			if ( wi.number == WP_MORTAR ) {
				//create the chase goal
				goal.entitynum = bs->client;
				goal.areanum = bs->areanum;
				VectorCopy(&bs->eye, &goal.origin);
				VectorSet(&goal.mins, -8, -8, -8);
				VectorSet(&goal.maxs, 8, 8, 8);
				//
				if (trap->ai->BotPredictVisiblePosition(&bs->lastenemyorigin, bs->lastenemyareanum, &goal, TFL_DEFAULT, &target)) {
					VectorSubtract(&target, &bs->eye, &dir);
					if (VectorLengthSquared(&dir) > Square(80)) {
						VectorCopy(&target, &bestorigin);
						bestorigin.z -= 20;
					}
				}
				aim_accuracy = 1;
			}
		}
	}
	//
	if (enemyvisible) {
		BotAI_Trace(&trace, &bs->eye, NULL, NULL, &bestorigin, bs->entitynum, MASK_SHOT);
		VectorCopy(&trace.endpos, &bs->aimtarget);
	}
	else {
		VectorCopy(&bestorigin, &bs->aimtarget);
	}
	//get aim direction
	VectorSubtract(&bestorigin, &bs->eye, &dir);
	//
	//distance towards the enemy
	dist = VectorLength(&dir);
	if (dist > 150)
		dist = 150;
	f = 0.6f + dist / 150 * 0.4f;
	aim_accuracy *= f;
	//add some random stuff to the aim direction depending on the aim accuracy
	if (aim_accuracy < 0.8) {
		VectorNormalize(&dir);
		for (i = 0; i < 3; i++)
			dir.data[i] += 0.3f * crandom() * (1 - aim_accuracy);
	}
	//set the ideal view angles
	vectoangles(&dir, &bs->ideal_viewangles);
	//take the weapon spread into account for lower skilled bots
	bs->ideal_viewangles.pitch += 6 * wi.vspread * crandom() * (1 - aim_accuracy);
	bs->ideal_viewangles.pitch = AngleMod(bs->ideal_viewangles.pitch);
	bs->ideal_viewangles.yaw += 6 * wi.hspread * crandom() * (1 - aim_accuracy);
	bs->ideal_viewangles.yaw = AngleMod(bs->ideal_viewangles.yaw);
	//if the bots should be really challenging
	if (bot_challenge->integer) {
		//if the bot is really accurate and has the enemy in view for some time
		if (aim_accuracy > 0.9f && bs->enemysight_time < FloatTime() - 1) {
			//set the view angles directly
			if (bs->ideal_viewangles.pitch > 180)
				bs->ideal_viewangles.pitch -= 360;
			VectorCopy(&bs->ideal_viewangles, &bs->viewangles);
			trap->ea->EA_View(bs->client, &bs->viewangles);
		}
	}
}

void BotCheckAttack(bot_state_t *bs) {
	float points, reactiontime, fov, firethrottle;
	int attackentity;
	bsp_trace_t bsptrace;
	//float selfpreservation;
	vector3 forward, right, start, end, dir, angles;
	weaponinfo_t wi;
	bsp_trace_t trace;
	aas_entityinfo_t entinfo;
	vector3 mins = {-8, -8, -8}, maxs = {8, 8, 8};

	attackentity = bs->enemy;
	//
	BotEntityInfo(attackentity, &entinfo);
	// if not attacking a player
	if (attackentity >= MAX_CLIENTS) {
		// if attacking an obelisk
		if ( entinfo.number == redobelisk.entitynum ||
			entinfo.number == blueobelisk.entitynum ) {
			// if obelisk is respawning return
			if ( g_entities[entinfo.number].activator &&
				g_entities[entinfo.number].activator->s.frame == 2 ) {
				return;
			}
		}
	}
	//
	reactiontime = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_REACTIONTIME, 0, 1);
	if (bs->enemysight_time > FloatTime() - reactiontime) return;
	if (bs->teleport_time > FloatTime() - reactiontime) return;
	//if changing weapons
	if (bs->weaponchange_time > FloatTime() - 0.1) return;
	//check fire throttle characteristic
	if (bs->firethrottlewait_time > FloatTime()) return;
	firethrottle = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_FIRETHROTTLE, 0, 1);
	if (bs->firethrottleshoot_time < FloatTime()) {
		if (random() > firethrottle) {
			bs->firethrottlewait_time = FloatTime() + firethrottle;
			bs->firethrottleshoot_time = 0;
		}
		else {
			bs->firethrottleshoot_time = FloatTime() + 1 - firethrottle;
			bs->firethrottlewait_time = 0;
		}
	}
	//
	//
	VectorSubtract(&bs->aimtarget, &bs->eye, &dir);
	//
	if (VectorLengthSquared(&dir) < Square(100))
		fov = 120;
	else
		fov = 50;
	//
	vectoangles(&dir, &angles);
	if (!InFieldOfVision(&bs->viewangles, fov, &angles))
		return;
	BotAI_Trace(&bsptrace, &bs->eye, NULL, NULL, &bs->aimtarget, bs->client, CONTENTS_SOLID|CONTENTS_PLAYERCLIP);
	if (bsptrace.fraction < 1 && bsptrace.ent != attackentity)
		return;

	//get the weapon info
	trap->ai->BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	//get the start point shooting from
	VectorCopy(&bs->origin, &start);
	start.z += bs->cur_ps.viewheight;
	AngleVectors(&bs->viewangles, &forward, &right, NULL);
	start.x += forward.x * wi.offset.x + right.x * wi.offset.y;
	start.y += forward.y * wi.offset.x + right.y * wi.offset.y;
	start.z += forward.z * wi.offset.x + right.z * wi.offset.y + wi.offset.z;
	//end point aiming at
	VectorMA(&start, 1000, &forward, &end);
	//a little back to make sure not inside a very close enemy
	VectorMA(&start, -12, &forward, &start);
	BotAI_Trace(&trace, &start, &mins, &maxs, &end, bs->entitynum, MASK_SHOT);
	//if the entity is a client
	if (trace.ent >= 0 && trace.ent < MAX_CLIENTS) {
		if (trace.ent != attackentity) {
			//if a teammate is hit
			if (BotSameTeam(bs, trace.ent))
				return;
		}
	}
	//if won't hit the enemy or not attacking a player (obelisk)
	if (trace.ent != attackentity || attackentity >= MAX_CLIENTS) {
		//if the projectile does radial damage
		if (wi.proj.damagetype & DAMAGETYPE_RADIAL) {
			if (trace.fraction * 1000 < wi.proj.radius) {
				points = (wi.proj.damage - 0.5f * trace.fraction * 1000) * 0.5f;
				if (points > 0) {
					return;
				}
			}
			//FIXME: check if a teammate gets radial damage
		}
	}
	//if fire has to be release to activate weapon
	if (wi.flags & WFL_FIRERELEASED) {
		if (bs->flags & BFL_ATTACKED) {
			trap->ea->EA_Attack(bs->client);
		}
	}
	else {
		trap->ea->EA_Attack(bs->client);
	}
	bs->flags ^= BFL_ATTACKED;
}

void BotMapScripts(bot_state_t *bs) {
	char info[1024];
	char mapname[128];
	int i, shootbutton;
	float aim_accuracy;
	aas_entityinfo_t entinfo;
	vector3 dir;

	trap->SV_GetServerinfo(info, sizeof(info));

	strncpy(mapname, Info_ValueForKey( info, "mapname" ), sizeof(mapname)-1);
	mapname[sizeof(mapname)-1] = '\0';

	if (!Q_stricmp(mapname, "q3tourney6")) {
		vector3 mins = {700, 204, 672}, maxs = {964, 468, 680};
		vector3 buttonorg = {304, 352, 920};
		//NOTE: NEVER use the func_bobbing in q3tourney6
		bs->tfl &= ~TFL_FUNCBOB;
		//if the bot is below the bounding box
		if (bs->origin.x > mins.x && bs->origin.x < maxs.x) {
			if (bs->origin.y > mins.y && bs->origin.y < maxs.y) {
				if (bs->origin.z < mins.z) {
					return;
				}
			}
		}
		shootbutton = qfalse;
		//if an enemy is below this bounding box then shoot the button
		for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {

			if (i == bs->client) continue;
			//
			BotEntityInfo(i, &entinfo);
			//
			if (!entinfo.valid) continue;
			//if the enemy isn't dead and the enemy isn't the bot self
			if (EntityIsDead(&entinfo) || entinfo.number == bs->entitynum) continue;
			//
			if (entinfo.origin.x > mins.x && entinfo.origin.x < maxs.x) {
				if (entinfo.origin.y > mins.y && entinfo.origin.y < maxs.y) {
					if (entinfo.origin.z < mins.z) {
						//if there's a team mate below the crusher
						if (BotSameTeam(bs, i)) {
							shootbutton = qfalse;
							break;
						}
						else {
							shootbutton = qtrue;
						}
					}
				}
			}
		}
		if (shootbutton) {
			bs->flags |= BFL_IDEALVIEWSET;
			VectorSubtract(&buttonorg, &bs->eye, &dir);
			vectoangles(&dir, &bs->ideal_viewangles);
			aim_accuracy = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY, 0, 1);
			bs->ideal_viewangles.pitch += 8 * crandom() * (1 - aim_accuracy);
			bs->ideal_viewangles.pitch = AngleMod(bs->ideal_viewangles.pitch);
			bs->ideal_viewangles.yaw += 8 * crandom() * (1 - aim_accuracy);
			bs->ideal_viewangles.yaw = AngleMod(bs->ideal_viewangles.yaw);
			//
			if (InFieldOfVision(&bs->viewangles, 20, &bs->ideal_viewangles)) {
				trap->ea->EA_Attack(bs->client);
			}
		}
	}
	else if (!Q_stricmp(mapname, "mpq3tourney6")) {
		//NOTE: NEVER use the func_bobbing in mpq3tourney6
		bs->tfl &= ~TFL_FUNCBOB;
	}
}

static vector3 VEC_UP		= {0, -1,  0};
static vector3 MOVEDIR_UP	= {0,  0,  1};
static vector3 VEC_DOWN		= {0, -2,  0};
static vector3 MOVEDIR_DOWN	= {0,  0, -1};

void BotSetMovedir(vector3 *angles, vector3 *movedir) {
	if (VectorCompare(angles, &VEC_UP)) {
		VectorCopy(&MOVEDIR_UP, movedir);
	}
	else if (VectorCompare(angles, &VEC_DOWN)) {
		VectorCopy(&MOVEDIR_DOWN, movedir);
	}
	else {
		AngleVectors(angles, movedir, NULL, NULL);
	}
}

// this is ugly
int BotModelMinsMaxs(int modelindex, int eType, int contents, vector3 *mins, vector3 *maxs) {
	gentity_t *ent;
	int i;

	ent = &g_entities[0];
	for (i = 0; i < level.num_entities; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}
		if ( eType && ent->s.eType != eType) {
			continue;
		}
		if ( contents && ent->r.contents != contents) {
			continue;
		}
		if (ent->s.modelindex == modelindex) {
			if (mins)
				VectorAdd(&ent->r.currentOrigin, &ent->r.mins, mins);
			if (maxs)
				VectorAdd(&ent->r.currentOrigin, &ent->r.maxs, maxs);
			return i;
		}
	}
	if (mins)
		VectorClear(mins);
	if (maxs)
		VectorClear(maxs);
	return 0;
}

int BotFuncButtonActivateGoal(bot_state_t *bs, int bspent, bot_activategoal_t *activategoal) {
	int i, areas[10], numareas, modelindex, entitynum;
	char model[128];
	float lip, dist, health, angle;
	vector3 size, start, end, mins, maxs, angles, points[10];
	vector3 movedir, origin, goalorigin, bboxmins, bboxmaxs;
	vector3 extramins = {1, 1, 1}, extramaxs = {-1, -1, -1};
	bsp_trace_t bsptrace;

	activategoal->shoot = qfalse;
	VectorClear(&activategoal->target);
	//create a bot goal towards the button
	trap->aas->AAS_ValueForBSPEpairKey(bspent, "model", model, sizeof(model));
	if (!*model)
		return qfalse;
	modelindex = atoi(model+1);
	if (!modelindex)
		return qfalse;
	VectorClear(&angles);
	entitynum = BotModelMinsMaxs(modelindex, ET_MOVER, 0, &mins, &maxs);
	//get the lip of the button
	trap->aas->AAS_FloatForBSPEpairKey(bspent, "lip", &lip);
	if (!lip) lip = 4;
	//get the move direction from the angle
	trap->aas->AAS_FloatForBSPEpairKey(bspent, "angle", &angle);
	VectorSet(&angles, 0, angle, 0);
	BotSetMovedir(&angles, &movedir);
	//button size
	VectorSubtract(&maxs, &mins, &size);
	//button origin
	VectorAdd(&mins, &maxs, &origin);
	VectorScale(&origin, 0.5, &origin);
	//touch distance of the button
	dist = fabsf(movedir.x) * size.x + fabsf(movedir.y) * size.y + fabsf(movedir.z) * size.z;
	dist *= 0.5f;
	//
	trap->aas->AAS_FloatForBSPEpairKey(bspent, "health", &health);
	//if the button is shootable
	if (health) {
		//calculate the shoot target
		VectorMA(&origin, -dist, &movedir, &goalorigin);
		//
		VectorCopy(&goalorigin, &activategoal->target);
		activategoal->shoot = qtrue;
		//
		BotAI_Trace(&bsptrace, &bs->eye, NULL, NULL, &goalorigin, bs->entitynum, MASK_SHOT);
		// if the button is visible from the current position
		if (bsptrace.fraction >= 1.0 || bsptrace.ent == entitynum) {
			//
			activategoal->goal.entitynum = entitynum; //NOTE: this is the entity number of the shootable button
			activategoal->goal.number = 0;
			activategoal->goal.flags = 0;
			VectorCopy(&bs->origin, &activategoal->goal.origin);
			activategoal->goal.areanum = bs->areanum;
			VectorSet(&activategoal->goal.mins, -8, -8, -8);
			VectorSet(&activategoal->goal.maxs, 8, 8, 8);
			//
			return qtrue;
		}
		else {
			//create a goal from where the button is visible and shoot at the button from there
			//add bounding box size to the dist
			trap->aas->AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, &bboxmins, &bboxmaxs);
			for (i = 0; i < 3; i++) {
				if ( movedir.data[i] < 0 )	dist += fabsf(movedir.data[i]) * fabsf(bboxmaxs.data[i]);
				else						dist += fabsf(movedir.data[i]) * fabsf(bboxmins.data[i]);
			}
			//calculate the goal origin
			VectorMA(&origin, -dist, &movedir, &goalorigin);
			//
			VectorCopy(&goalorigin, &start);
			start.z += 24;
			VectorCopy(&start, &end);
			end.z -= 512;
			numareas = trap->aas->AAS_TraceAreas(&start, &end, areas, points, 10);
			//
			for (i = numareas-1; i >= 0; i--) {
				if (trap->aas->AAS_AreaReachability(areas[i])) {
					break;
				}
			}
			if (i < 0) {
				// FIXME: trace forward and maybe in other directions to find a valid area
			}
			if (i >= 0) {
				//
				VectorCopy(&points[i], &activategoal->goal.origin);
				activategoal->goal.areanum = areas[i];
				VectorSet(&activategoal->goal.mins, 8, 8, 8);
				VectorSet(&activategoal->goal.maxs, -8, -8, -8);
				//
				for (i = 0; i < 3; i++)
				{
					if ( movedir.data[i] < 0 )	activategoal->goal.maxs.data[i] += fabsf(movedir.data[i]) * fabsf(extramaxs.data[i]);
					else						activategoal->goal.mins.data[i] += fabsf(movedir.data[i]) * fabsf(extramins.data[i]);
				} //end for
				//
				activategoal->goal.entitynum = entitynum;
				activategoal->goal.number = 0;
				activategoal->goal.flags = 0;
				return qtrue;
			}
		}
		return qfalse;
	}
	else {
		//add bounding box size to the dist
		trap->aas->AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, &bboxmins, &bboxmaxs);
		for (i = 0; i < 3; i++) {
			if ( movedir.data[i] < 0 )	dist += fabsf(movedir.data[i]) * fabsf(bboxmaxs.data[i]);
			else						dist += fabsf(movedir.data[i]) * fabsf(bboxmins.data[i]);
		}
		//calculate the goal origin
		VectorMA(&origin, -dist, &movedir, &goalorigin);
		//
		VectorCopy(&goalorigin, &start);
		start.z += 24;
		VectorCopy(&start, &end);
		end.z -= 100;
		numareas = trap->aas->AAS_TraceAreas(&start, &end, areas, NULL, 10);
		//
		for (i = 0; i < numareas; i++) {
			if (trap->aas->AAS_AreaReachability(areas[i])) {
				break;
			}
		}
		if (i < numareas) {
			//
			VectorCopy(&origin, &activategoal->goal.origin);
			activategoal->goal.areanum = areas[i];
			VectorSubtract(&mins, &origin, &activategoal->goal.mins);
			VectorSubtract(&maxs, &origin, &activategoal->goal.maxs);
			//
			for (i = 0; i < 3; i++)
			{
				if ( movedir.data[i] < 0 )	activategoal->goal.maxs.data[i] += fabsf(movedir.data[i]) * fabsf(extramaxs.data[i]);
				else						activategoal->goal.mins.data[i] += fabsf(movedir.data[i]) * fabsf(extramins.data[i]);
			} //end for
			//
			activategoal->goal.entitynum = entitynum;
			activategoal->goal.number = 0;
			activategoal->goal.flags = 0;
			return qtrue;
		}
	}
	return qfalse;
}

int BotFuncDoorActivateGoal(bot_state_t *bs, int bspent, bot_activategoal_t *activategoal) {
	int modelindex, entitynum;
	char model[MAX_INFO_STRING];
	vector3 mins, maxs, origin, angles;

	//shoot at the shootable door
	trap->aas->AAS_ValueForBSPEpairKey(bspent, "model", model, sizeof(model));
	if (!*model)
		return qfalse;
	modelindex = atoi(model+1);
	if (!modelindex)
		return qfalse;
	VectorClear(&angles);
	entitynum = BotModelMinsMaxs(modelindex, ET_MOVER, 0, &mins, &maxs);
	//door origin
	VectorAdd(&mins, &maxs, &origin);
	VectorScale(&origin, 0.5, &origin);
	VectorCopy(&origin, &activategoal->target);
	activategoal->shoot = qtrue;
	//
	activategoal->goal.entitynum = entitynum; //NOTE: this is the entity number of the shootable door
	activategoal->goal.number = 0;
	activategoal->goal.flags = 0;
	VectorCopy(&bs->origin, &activategoal->goal.origin);
	activategoal->goal.areanum = bs->areanum;
	VectorSet(&activategoal->goal.mins, -8, -8, -8);
	VectorSet(&activategoal->goal.maxs, 8, 8, 8);
	return qtrue;
}

int BotTriggerMultipleActivateGoal(bot_state_t *bs, int bspent, bot_activategoal_t *activategoal) {
	int i, areas[10], numareas, modelindex, entitynum;
	char model[128];
	vector3 start, end, mins, maxs, angles;
	vector3 origin, goalorigin;

	activategoal->shoot = qfalse;
	VectorClear(&activategoal->target);
	//create a bot goal towards the trigger
	trap->aas->AAS_ValueForBSPEpairKey(bspent, "model", model, sizeof(model));
	if (!*model)
		return qfalse;
	modelindex = atoi(model+1);
	if (!modelindex)
		return qfalse;
	VectorClear(&angles);
	entitynum = BotModelMinsMaxs(modelindex, 0, CONTENTS_TRIGGER, &mins, &maxs);
	//trigger origin
	VectorAdd(&mins, &maxs, &origin);
	VectorScale(&origin, 0.5, &origin);
	VectorCopy(&origin, &goalorigin);
	//
	VectorCopy(&goalorigin, &start);
	start.z += 24;
	VectorCopy(&start, &end);
	end.z -= 100;
	numareas = trap->aas->AAS_TraceAreas(&start, &end, areas, NULL, 10);
	//
	for (i = 0; i < numareas; i++) {
		if (trap->aas->AAS_AreaReachability(areas[i])) {
			break;
		}
	}
	if (i < numareas) {
		VectorCopy(&origin, &activategoal->goal.origin);
		activategoal->goal.areanum = areas[i];
		VectorSubtract(&mins, &origin, &activategoal->goal.mins);
		VectorSubtract(&maxs, &origin, &activategoal->goal.maxs);
		//
		activategoal->goal.entitynum = entitynum;
		activategoal->goal.number = 0;
		activategoal->goal.flags = 0;
		return qtrue;
	}
	return qfalse;
}

int BotPopFromActivateGoalStack(bot_state_t *bs) {
	if (!bs->activatestack)
		return qfalse;
	BotEnableActivateGoalAreas(bs->activatestack, qtrue);
	bs->activatestack->inuse = qfalse;
	bs->activatestack->justused_time = FloatTime();
	bs->activatestack = bs->activatestack->next;
	return qtrue;
}

int BotPushOntoActivateGoalStack(bot_state_t *bs, bot_activategoal_t *activategoal) {
	int i, best;
	float besttime;

	best = -1;
	besttime = FloatTime() + 9999;
	//
	for (i = 0; i < MAX_ACTIVATESTACK; i++) {
		if (!bs->activategoalheap[i].inuse) {
			if (bs->activategoalheap[i].justused_time < besttime) {
				besttime = bs->activategoalheap[i].justused_time;
				best = i;
			}
		}
	}
	if (best != -1) {
		memcpy(&bs->activategoalheap[best], activategoal, sizeof(bot_activategoal_t));
		bs->activategoalheap[best].inuse = qtrue;
		bs->activategoalheap[best].next = bs->activatestack;
		bs->activatestack = &bs->activategoalheap[best];
		return qtrue;
	}
	return qfalse;
}

void BotClearActivateGoalStack(bot_state_t *bs) {
	while(bs->activatestack)
		BotPopFromActivateGoalStack(bs);
}

void BotEnableActivateGoalAreas(bot_activategoal_t *activategoal, int enable) {
	int i;

	if (activategoal->areasdisabled == !enable)
		return;
	for (i = 0; i < activategoal->numareas; i++)
		trap->aas->AAS_EnableRoutingArea( activategoal->areas[i], enable );
	activategoal->areasdisabled = !enable;
}

int BotIsGoingToActivateEntity(bot_state_t *bs, int entitynum) {
	bot_activategoal_t *a;
	int i;

	for (a = bs->activatestack; a; a = a->next) {
		if (a->time < FloatTime())
			continue;
		if (a->goal.entitynum == entitynum)
			return qtrue;
	}
	for (i = 0; i < MAX_ACTIVATESTACK; i++) {
		if (bs->activategoalheap[i].inuse)
			continue;
		//
		if (bs->activategoalheap[i].goal.entitynum == entitynum) {
			// if the bot went for this goal less than 2 seconds ago
			if (bs->activategoalheap[i].justused_time > FloatTime() - 2)
				return qtrue;
		}
	}
	return qfalse;
}

//#define OBSTACLEDEBUG

// returns the number of the bsp entity to activate
//	goal->entitynum will be set to the game entity to activate
int BotGetActivateGoal(bot_state_t *bs, int entitynum, bot_activategoal_t *activategoal) {
	int i, ent, cur_entities[10], spawnflags, modelindex, areas[MAX_ACTIVATEAREAS*2], numareas, t;
	char model[MAX_INFO_STRING], tmpmodel[128];
	char target[128], classname[128];
	float health;
	char targetname[10][128];
	aas_entityinfo_t entinfo;
	aas_areainfo_t areainfo;
	vector3 origin, angles, absmins, absmaxs;

	memset(activategoal, 0, sizeof(bot_activategoal_t));
	BotEntityInfo(entitynum, &entinfo);
	Com_sprintf(model, sizeof( model ), "*%d", entinfo.modelindex);
	for (ent = trap->aas->AAS_NextBSPEntity(0); ent; ent = trap->aas->AAS_NextBSPEntity(ent)) {
		if (!trap->aas->AAS_ValueForBSPEpairKey(ent, "model", tmpmodel, sizeof(tmpmodel))) continue;
		if (!strcmp(model, tmpmodel)) break;
	}
	if (!ent) {
		BotAI_Print(PRT_ERROR, "BotGetActivateGoal: no entity found with model %s\n", model);
		return 0;
	}
	trap->aas->AAS_ValueForBSPEpairKey(ent, "classname", classname, sizeof(classname));
	if (!*classname) {
		BotAI_Print(PRT_ERROR, "BotGetActivateGoal: entity with model %s has no classname\n", model);
		return 0;
	}
	//if it is a door
	if (!strcmp(classname, "func_door")) {
		if (trap->aas->AAS_FloatForBSPEpairKey(ent, "health", &health)) {
			//if the door has health then the door must be shot to open
			if (health) {
				BotFuncDoorActivateGoal(bs, ent, activategoal);
				return ent;
			}
		}
		//
		trap->aas->AAS_IntForBSPEpairKey(ent, "spawnflags", &spawnflags);
		// if the door starts open then just wait for the door to return
		if ( spawnflags & 1 )
			return 0;
		//get the door origin
		if (!trap->aas->AAS_VectorForBSPEpairKey(ent, "origin", &origin)) {
			VectorClear(&origin);
		}
		//if the door is open or opening already
		if (!VectorCompare(&origin, &entinfo.origin))
			return 0;
		// store all the areas the door is in
		trap->aas->AAS_ValueForBSPEpairKey(ent, "model", model, sizeof(model));
		if (*model) {
			modelindex = atoi(model+1);
			if (modelindex) {
				VectorClear(&angles);
				BotModelMinsMaxs(modelindex, ET_MOVER, 0, &absmins, &absmaxs);
				//
				numareas = trap->aas->AAS_BBoxAreas(&absmins, &absmaxs, areas, MAX_ACTIVATEAREAS*2);
				// store the areas with reachabilities first
				for (i = 0; i < numareas; i++) {
					if (activategoal->numareas >= MAX_ACTIVATEAREAS)
						break;
					if ( !trap->aas->AAS_AreaReachability(areas[i]) ) {
						continue;
					}
					trap->aas->AAS_AreaInfo(areas[i], &areainfo);
					if (areainfo.contents & AREACONTENTS_MOVER) {
						activategoal->areas[activategoal->numareas++] = areas[i];
					}
				}
				// store any remaining areas
				for (i = 0; i < numareas; i++) {
					if (activategoal->numareas >= MAX_ACTIVATEAREAS)
						break;
					if ( trap->aas->AAS_AreaReachability(areas[i]) ) {
						continue;
					}
					trap->aas->AAS_AreaInfo(areas[i], &areainfo);
					if (areainfo.contents & AREACONTENTS_MOVER) {
						activategoal->areas[activategoal->numareas++] = areas[i];
					}
				}
			}
		}
	}
	// if the bot is blocked by or standing on top of a button
	if (!strcmp(classname, "func_button")) {
		return 0;
	}
	// get the targetname so we can find an entity with a matching target
	if (!trap->aas->AAS_ValueForBSPEpairKey(ent, "targetname", targetname[0], sizeof(targetname[0]))) {
		if (bot_developer->integer) {
			BotAI_Print(PRT_ERROR, "BotGetActivateGoal: entity with model \"%s\" has no targetname\n", model);
		}
		return 0;
	}
	// allow tree-like activation
	cur_entities[0] = trap->aas->AAS_NextBSPEntity(0);
	for (i = 0; i >= 0 && i < 10;) {
		for (ent = cur_entities[i]; ent; ent = trap->aas->AAS_NextBSPEntity(ent)) {
			if (!trap->aas->AAS_ValueForBSPEpairKey(ent, "target", target, sizeof(target))) continue;
			if (!strcmp(targetname[i], target)) {
				cur_entities[i] = trap->aas->AAS_NextBSPEntity(ent);
				break;
			}
		}
		if (!ent) {
			if (bot_developer->integer) {
				BotAI_Print(PRT_ERROR, "BotGetActivateGoal: no entity with target \"%s\"\n", targetname[i]);
			}
			i--;
			continue;
		}
		if (!trap->aas->AAS_ValueForBSPEpairKey(ent, "classname", classname, sizeof(classname))) {
			if (bot_developer->integer) {
				BotAI_Print(PRT_ERROR, "BotGetActivateGoal: entity with target \"%s\" has no classname\n", targetname[i]);
			}
			continue;
		}
		// BSP button model
		if (!strcmp(classname, "func_button")) {
			//
			if (!BotFuncButtonActivateGoal(bs, ent, activategoal))
				continue;
			// if the bot tries to activate this button already
			if ( bs->activatestack && bs->activatestack->inuse &&
				 bs->activatestack->goal.entitynum == activategoal->goal.entitynum &&
				 bs->activatestack->time > FloatTime() &&
				 bs->activatestack->start_time < FloatTime() - 2)
				continue;
			// if the bot is in a reachability area
			if ( trap->aas->AAS_AreaReachability(bs->areanum) ) {
				// disable all areas the blocking entity is in
				BotEnableActivateGoalAreas( activategoal, qfalse );
				//
				t = trap->aas->AAS_AreaTravelTimeToGoalArea(bs->areanum, &bs->origin, activategoal->goal.areanum, bs->tfl);
				// if the button is not reachable
				if (!t) {
					continue;
				}
				activategoal->time = FloatTime() + t * 0.01f + 5;
			}
			return ent;
		}
		// invisible trigger multiple box
		else if (!strcmp(classname, "trigger_multiple")) {
			//
			if (!BotTriggerMultipleActivateGoal(bs, ent, activategoal))
				continue;
			// if the bot tries to activate this trigger already
			if ( bs->activatestack && bs->activatestack->inuse &&
				 bs->activatestack->goal.entitynum == activategoal->goal.entitynum &&
				 bs->activatestack->time > FloatTime() &&
				 bs->activatestack->start_time < FloatTime() - 2)
				continue;
			// if the bot is in a reachability area
			if ( trap->aas->AAS_AreaReachability(bs->areanum) ) {
				// disable all areas the blocking entity is in
				BotEnableActivateGoalAreas( activategoal, qfalse );
				//
				t = trap->aas->AAS_AreaTravelTimeToGoalArea(bs->areanum, &bs->origin, activategoal->goal.areanum, bs->tfl);
				// if the trigger is not reachable
				if (!t) {
					continue;
				}
				activategoal->time = FloatTime() + t * 0.01f + 5;
			}
			return ent;
		}
		else if (!strcmp(classname, "func_timer")) {
			// just skip the func_timer
			continue;
		}
		// the actual button or trigger might be linked through a target_relay or target_delay
		else if (!strcmp(classname, "target_relay") || !strcmp(classname, "target_delay")) {
			if (trap->aas->AAS_ValueForBSPEpairKey(ent, "targetname", targetname[i+1], sizeof(targetname[0]))) {
				i++;
				cur_entities[i] = trap->aas->AAS_NextBSPEntity(0);
			}
		}
	}
#ifdef OBSTACLEDEBUG
	BotAI_Print(PRT_ERROR, "BotGetActivateGoal: no valid activator for entity with target \"%s\"\n", targetname[0]);
#endif
	return 0;
}

int BotGoForActivateGoal(bot_state_t *bs, bot_activategoal_t *activategoal) {
	aas_entityinfo_t activateinfo;

	activategoal->inuse = qtrue;
	if (!activategoal->time)
		activategoal->time = FloatTime() + 10;
	activategoal->start_time = FloatTime();
	BotEntityInfo(activategoal->goal.entitynum, &activateinfo);
	VectorCopy(&activateinfo.origin, &activategoal->origin);
	//
	if (BotPushOntoActivateGoalStack(bs, activategoal)) {
		// enter the activate entity AI node
		AIEnter_Seek_ActivateEntity(bs, "BotGoForActivateGoal");
		return qtrue;
	}
	else {
		// enable any routing areas that were disabled
		BotEnableActivateGoalAreas(activategoal, qtrue);
		return qfalse;
	}
}

void BotPrintActivateGoalInfo(bot_state_t *bs, bot_activategoal_t *activategoal, int bspent) {
	char netname[MAX_NETNAME];
	char classname[128];
	char buf[128];

	ClientName(bs->client, netname, sizeof(netname));
	trap->aas->AAS_ValueForBSPEpairKey(bspent, "classname", classname, sizeof(classname));
	if (activategoal->shoot) {
		Com_sprintf(buf, sizeof(buf), "%s: I have to shoot at a %s from %1.1f %1.1f %1.1f in area %d\n",
						netname, classname,
						activategoal->goal.origin.x,
						activategoal->goal.origin.y,
						activategoal->goal.origin.z,
						activategoal->goal.areanum);
	}
	else {
		Com_sprintf(buf, sizeof(buf), "%s: I have to activate a %s at %1.1f %1.1f %1.1f in area %d\n",
						netname, classname,
						activategoal->goal.origin.x,
						activategoal->goal.origin.y,
						activategoal->goal.origin.z,
						activategoal->goal.areanum);
	}
	trap->ea->EA_Say(bs->client, buf);
}

void BotRandomMove(bot_state_t *bs, bot_moveresult_t *moveresult) {
	vector3 dir, angles;

	angles.x = 0;
	angles.y = random() * 360;
	angles.z = 0;
	AngleVectors(&angles, &dir, NULL, NULL);

	trap->ai->BotMoveInDirection(bs->ms, &dir, 400, MOVE_WALK);

	moveresult->failure = qfalse;
	VectorCopy(&dir, &moveresult->movedir);
}

// Very basic handling of bots being blocked by other entities.
//	Check what kind of entity is blocking the bot and try to activate it.
//	If that's not an option then try to walk around or over the entity.
//	Before the bot ends in this part of the AI it should predict which doors to open, which buttons to activate etc.
void BotAIBlocked(bot_state_t *bs, bot_moveresult_t *moveresult, int activate) {
	int movetype, bspent;
	vector3 hordir, sideward, angles, up = {0, 0, 1};
	//vector3 start, end, mins, maxs;
	aas_entityinfo_t entinfo;
	bot_activategoal_t activategoal;

	// if the bot is not blocked by anything
	if (!moveresult->blocked) {
		bs->notblocked_time = FloatTime();
		return;
	}
	// if stuck in a solid area
	if ( moveresult->type == RESULTTYPE_INSOLIDAREA ) {
		// move in a random direction in the hope to get out
		BotRandomMove(bs, moveresult);
		//
		return;
	}
	// get info for the entity that is blocking the bot
	BotEntityInfo(moveresult->blockentity, &entinfo);
#ifdef OBSTACLEDEBUG
	ClientName(bs->client, netname, sizeof(netname));
	BotAI_Print(PRT_MESSAGE, "%s: I'm blocked by model %d\n", netname, entinfo.modelindex);
#endif // OBSTACLEDEBUG
	// if blocked by a bsp model and the bot wants to activate it
	if (activate && entinfo.modelindex > 0 && entinfo.modelindex <= max_bspmodelindex) {
		// find the bsp entity which should be activated in order to get the blocking entity out of the way
		bspent = BotGetActivateGoal(bs, entinfo.number, &activategoal);
		if (bspent) {
			//
			if (bs->activatestack && !bs->activatestack->inuse)
				bs->activatestack = NULL;
			// if not already trying to activate this entity
			if (!BotIsGoingToActivateEntity(bs, activategoal.goal.entitynum)) {
				//
				BotGoForActivateGoal(bs, &activategoal);
			}
			// if ontop of an obstacle or
			// if the bot is not in a reachability area it'll still
			// need some dynamic obstacle avoidance, otherwise return
			if (!(moveresult->flags & MOVERESULT_ONTOPOFOBSTACLE) &&
				trap->aas->AAS_AreaReachability(bs->areanum))
				return;
		}
		else {
			// enable any routing areas that were disabled
			BotEnableActivateGoalAreas(&activategoal, qtrue);
		}
	}
	// just some basic dynamic obstacle avoidance code
	hordir.x = moveresult->movedir.x;
	hordir.y = moveresult->movedir.y;
	hordir.z = 0;
	// if no direction just take a random direction
	if (VectorNormalize(&hordir) < 0.1) {
		VectorSet(&angles, 0, 360 * random(), 0);
		AngleVectors(&angles, &hordir, NULL, NULL);
	}
	//
	//if (moveresult->flags & MOVERESULT_ONTOPOFOBSTACLE) movetype = MOVE_JUMP;
	//else
	movetype = MOVE_WALK;
	// if there's an obstacle at the bot's feet and head then
	// the bot might be able to crouch through
	//VectorCopy(bs->origin, start);
	//start[2] += 18;
	//VectorMA(start, 5, hordir, end);
	//VectorSet(mins, -16, -16, -24);
	//VectorSet(maxs, 16, 16, 4);
	//
	//bsptrace = AAS_Trace(start, mins, maxs, end, bs->entitynum, MASK_PLAYERSOLID);
	//if (bsptrace.fraction >= 1) movetype = MOVE_CROUCH;
	// get the sideward vector
	CrossProduct(&hordir, &up, &sideward);
	//
	if (bs->flags & BFL_AVOIDRIGHT) VectorNegate(&sideward, &sideward);
	// try to crouch straight forward?
	if (movetype != MOVE_CROUCH || !trap->ai->BotMoveInDirection(bs->ms, &hordir, 400, movetype)) {
		// perform the movement
		if (!trap->ai->BotMoveInDirection(bs->ms, &sideward, 400, movetype)) {
			// flip the avoid direction flag
			bs->flags ^= BFL_AVOIDRIGHT;
			// flip the direction
			// VectorNegate(sideward, sideward);
			VectorMA(&sideward, -1, &hordir, &sideward);
			// move in the other direction
			trap->ai->BotMoveInDirection(bs->ms, &sideward, 400, movetype);
		}
	}
	//
	if (bs->notblocked_time < FloatTime() - 0.4) {
		// just reset goals and hope the bot will go into another direction?
		// is this still needed??
		if (bs->ainode == AINode_Seek_NBG) bs->nbg_time = 0;
		else if (bs->ainode == AINode_Seek_LTG) bs->ltg_time = 0;
	}
}

// Predict the route towards the goal and check if the bot will be blocked by certain obstacles.
//	When the bot has obstacles on its path the bot should figure out if they can be removed by activating certain entities.
int BotAIPredictObstacles(bot_state_t *bs, bot_goal_t *goal) {
	int modelnum, entitynum, bspent;
	bot_activategoal_t activategoal;
	aas_predictroute_t route;

	if (!bot_predictobstacles->integer)
		return qfalse;

	// always predict when the goal change or at regular intervals
	if (bs->predictobstacles_goalareanum == goal->areanum &&
		bs->predictobstacles_time > FloatTime() - 6) {
		return qfalse;
	}
	bs->predictobstacles_goalareanum = goal->areanum;
	bs->predictobstacles_time = FloatTime();

	// predict at most 100 areas or 10 seconds ahead
	trap->aas->AAS_PredictRoute(&route, bs->areanum, &bs->origin,
							goal->areanum, bs->tfl, 100, 1000,
							RSE_USETRAVELTYPE|RSE_ENTERCONTENTS,
							AREACONTENTS_MOVER, TFL_BRIDGE, 0);
	// if bot has to travel through an area with a mover
	if (route.stopevent & RSE_ENTERCONTENTS) {
		// if the bot will run into a mover
		if (route.endcontents & AREACONTENTS_MOVER) {
			//NOTE: this only works with bspc 2.1 or higher
			modelnum = (route.endcontents & AREACONTENTS_MODELNUM) >> AREACONTENTS_MODELNUMSHIFT;
			if (modelnum) {
				//
				entitynum = BotModelMinsMaxs(modelnum, ET_MOVER, 0, NULL, NULL);
				if (entitynum) {
					//NOTE: BotGetActivateGoal already checks if the door is open or not
					bspent = BotGetActivateGoal(bs, entitynum, &activategoal);
					if (bspent) {
						//
						if (bs->activatestack && !bs->activatestack->inuse)
							bs->activatestack = NULL;
						// if not already trying to activate this entity
						if (!BotIsGoingToActivateEntity(bs, activategoal.goal.entitynum)) {
							//
							//BotAI_Print(PRT_MESSAGE, "blocked by mover model %d, entity %d ?\n", modelnum, entitynum);
							//
							BotGoForActivateGoal(bs, &activategoal);
							return qtrue;
						}
						else {
							// enable any routing areas that were disabled
							BotEnableActivateGoalAreas(&activategoal, qtrue);
						}
					}
				}
			}
		}
	}
	else if (route.stopevent & RSE_USETRAVELTYPE) {
		if (route.endtravelflags & TFL_BRIDGE) {
			//FIXME: check if the bridge is available to travel over
		}
	}
	return qfalse;
}

void BotCheckConsoleMessages(bot_state_t *bs) {
	char botname[MAX_NETNAME], message[MAX_MESSAGE_SIZE], netname[MAX_NETNAME], *ptr;
	float chat_reply;
	int context, handle;
	bot_consolemessage_t m;
	bot_match_t match;

	//the name of this bot
	ClientName(bs->client, botname, sizeof(botname));
	//
	while((handle = trap->ai->BotNextConsoleMessage(bs->cs, &m)) != 0) {
		//if the chat state is flooded with messages the bot will read them quickly
		if (trap->ai->BotNumConsoleMessages(bs->cs) < 10) {
			//if it is a chat message the bot needs some time to read it
			if (m.type == CMS_CHAT && m.time > FloatTime() - (1 + random())) break;
		}
		//
		ptr = m.message;
		//if it is a chat message then don't unify white spaces and don't
		//replace synonyms in the netname
		if (m.type == CMS_CHAT) {
			//
			if (trap->ai->BotFindMatch(m.message, &match, MTCONTEXT_REPLYCHAT)) {
				ptr = m.message + match.variables[MESSAGE].offset;
			}
		}
		//unify the white spaces in the message
		trap->ai->UnifyWhiteSpaces(ptr);
		//replace synonyms in the right context
		context = BotSynonymContext(bs);
		trap->ai->BotReplaceSynonyms(ptr, context);
		//if there's no match
		if (!BotMatchMessage(bs, m.message)) {
			//if it is a chat message
			if (m.type == CMS_CHAT && !bot_nochat->integer) {
				//
				if (!trap->ai->BotFindMatch(m.message, &match, MTCONTEXT_REPLYCHAT)) {
					trap->ai->BotRemoveConsoleMessage(bs->cs, handle);
					continue;
				}
				//don't use eliza chats with team messages
				if (match.subtype & ST_TEAM) {
					trap->ai->BotRemoveConsoleMessage(bs->cs, handle);
					continue;
				}
				//
				trap->ai->BotMatchVariable(&match, NETNAME, netname, sizeof(netname));
				trap->ai->BotMatchVariable(&match, MESSAGE, message, sizeof(message));
				//if this is a message from the bot self
				if (bs->client == ClientFromName(netname)) {
					trap->ai->BotRemoveConsoleMessage(bs->cs, handle);
					continue;
				}
				//unify the message
				trap->ai->UnifyWhiteSpaces(message);
				//
				if (bot_testrchat->integer) {
					//
					trap->BotLibVarSet("bot_testrchat", "1");
					//if bot replies with a chat message
					if (trap->ai->BotReplyChat(bs->cs, message, context, CONTEXT_REPLY,
															NULL, NULL,
															NULL, NULL,
															NULL, NULL,
															botname, netname)) {
						BotAI_Print(PRT_MESSAGE, "------------------------\n");
					}
					else {
						BotAI_Print(PRT_MESSAGE, "**** no valid reply ****\n");
					}
				}
				//if at a valid chat position and not chatting already and not in teamplay
				else if (bs->ainode != AINode_Stand && BotValidChatPosition(bs) && !TeamPlayIsOn()) {
					chat_reply = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_REPLY, 0, 1);
					if (random() < 1.5 / (NumBots()+1) && random() < chat_reply) {
						//if bot replies with a chat message
						if (trap->ai->BotReplyChat(bs->cs, message, context, CONTEXT_REPLY,
																NULL, NULL,
																NULL, NULL,
																NULL, NULL,
																botname, netname)) {
							//remove the console message
							trap->ai->BotRemoveConsoleMessage(bs->cs, handle);
							bs->stand_time = FloatTime() + BotChatTime(bs);
							AIEnter_Stand(bs, "BotCheckConsoleMessages: reply chat");
							//EA_Say(bs->client, bs->cs.chatmessage);
							break;
						}
					}
				}
			}
		}
		//remove the console message
		trap->ai->BotRemoveConsoleMessage(bs->cs, handle);
	}
}

void BotCheckEvents(bot_state_t *bs, entityState_t *state) {
	int event;
	char buf[128];
	aas_entityinfo_t entinfo;

	//NOTE: this sucks, we're accessing the gentity_t directly
	//but there's no other fast way to do it right now
	if (bs->entityeventTime[state->number] == g_entities[state->number].eventTime) {
		return;
	}
	bs->entityeventTime[state->number] = g_entities[state->number].eventTime;
	//if it's an event only entity
	if (state->eType > ET_EVENTS) {
		event = (state->eType - ET_EVENTS) & ~EV_EVENT_BITS;
	}
	else {
		event = state->event & ~EV_EVENT_BITS;
	}
	//
	switch(event) {
		//client obituary event
		case EV_OBITUARY:
		{
			int target, attacker, mod;

			target = state->otherEntityNum;
			attacker = state->otherEntityNum2;
			mod = state->eventParm;
			//
			if (target == bs->client) {
				bs->botdeathtype = mod;
				bs->lastkilledby = attacker;
				//
				if (target == attacker ||
					target == ENTITYNUM_NONE ||
					target == ENTITYNUM_WORLD) bs->botsuicide = qtrue;
				else bs->botsuicide = qfalse;
				//
				bs->num_deaths++;
			}
			//else if this client was killed by the bot
			else if (attacker == bs->client) {
				bs->enemydeathtype = mod;
				bs->lastkilledplayer = target;
				bs->killedenemy_time = FloatTime();
				//
				bs->num_kills++;
			}
			else if (attacker == bs->enemy && target == attacker) {
				bs->enemysuicide = qtrue;
			}
			//
			if (gametype == GT_TROJAN) {
				//
				BotEntityInfo(target, &entinfo);
				if ( entinfo.powerups & ( 1 << PW_NEUTRALFLAG ) ) {
					if (!BotSameTeam(bs, target)) {
						bs->neutralflagstatus = 3;	//enemy dropped the flag
						bs->flagstatuschanged = qtrue;
					}
				}
			}
			break;
		}
		case EV_GLOBAL_SOUND:
		{
			if (state->eventParm < 0 || state->eventParm >= MAX_SOUNDS) {
				BotAI_Print(PRT_ERROR, "EV_GLOBAL_SOUND: eventParm (%d) out of range\n", state->eventParm);
				break;
			}
			trap->SV_GetConfigstring(CS_SOUNDS + state->eventParm, buf, sizeof(buf));
			if (!strcmp(buf, "sound/items/poweruprespawn.wav")) {
				//powerup respawned... go get it
				BotGoForPowerups(bs);
			}
			break;
		}
		case EV_GLOBAL_TEAM_SOUND:
		{
			if (gametype == GT_FLAGS) {
				switch(state->eventParm) {
					case GTS_RED_CAPTURE:
						bs->blueflagstatus = 0;
						bs->redflagstatus = 0;
						bs->flagstatuschanged = qtrue;
						break; //see BotMatch_CTF
					case GTS_BLUE_CAPTURE:
						bs->blueflagstatus = 0;
						bs->redflagstatus = 0;
						bs->flagstatuschanged = qtrue;
						break; //see BotMatch_CTF
					case GTS_RED_RETURN:
						//blue flag is returned
						bs->blueflagstatus = 0;
						bs->flagstatuschanged = qtrue;
						break;
					case GTS_BLUE_RETURN:
						//red flag is returned
						bs->redflagstatus = 0;
						bs->flagstatuschanged = qtrue;
						break;
					case GTS_RED_TAKEN:
						//blue flag is taken
						bs->blueflagstatus = 1;
						bs->flagstatuschanged = qtrue;
						break; //see BotMatch_CTF
					case GTS_BLUE_TAKEN:
						//red flag is taken
						bs->redflagstatus = 1;
						bs->flagstatuschanged = qtrue;
						break; //see BotMatch_CTF
				}
			}
			else if (gametype == GT_TROJAN) {
				switch(state->eventParm) {
					case GTS_RED_CAPTURE:
						bs->neutralflagstatus = 0;
						bs->flagstatuschanged = qtrue;
						break;
					case GTS_BLUE_CAPTURE:
						bs->neutralflagstatus = 0;
						bs->flagstatuschanged = qtrue;
						break;
					case GTS_RED_RETURN:
						//flag has returned
						bs->neutralflagstatus = 0;
						bs->flagstatuschanged = qtrue;
						break;
					case GTS_BLUE_RETURN:
						//flag has returned
						bs->neutralflagstatus = 0;
						bs->flagstatuschanged = qtrue;
						break;
					case GTS_RED_TAKEN:
						bs->neutralflagstatus = BotTeam(bs) == TEAM_RED ? 2 : 1; //FIXME: check Team_TakeFlagSound in g_team.c
						bs->flagstatuschanged = qtrue;
						break;
					case GTS_BLUE_TAKEN:
						bs->neutralflagstatus = BotTeam(bs) == TEAM_BLUE ? 2 : 1; //FIXME: check Team_TakeFlagSound in g_team.c
						bs->flagstatuschanged = qtrue;
						break;
				}
			}
			break;
		}
		case EV_PLAYER_TELEPORT_IN:
		{
			VectorCopy(&state->origin, &lastteleport_origin);
			lastteleport_time = FloatTime();
			break;
		}
		case EV_GENERAL_SOUND:
		{
			//if this sound is played on the bot
			if (state->number == bs->client) {
				if (state->eventParm < 0 || state->eventParm >= MAX_SOUNDS) {
					BotAI_Print(PRT_ERROR, "EV_GENERAL_SOUND: eventParm (%d) out of range\n", state->eventParm);
					break;
				}
				//check out the sound
				trap->SV_GetConfigstring(CS_SOUNDS + state->eventParm, buf, sizeof(buf));
			}
			break;
		}
		case EV_FOOTSTEP:
		case EV_FOOTSTEP_METAL:
		case EV_FOOTSPLASH:
		case EV_FOOTWADE:
		case EV_SWIM:
		case EV_FALL_SHORT:
		case EV_FALL_MEDIUM:
		case EV_FALL_FAR:
		case EV_STEP_4:
		case EV_STEP_8:
		case EV_STEP_12:
		case EV_STEP_16:
		case EV_JUMP_PAD:
		case EV_JUMP:
		case EV_TAUNT:
		case EV_WATER_TOUCH:
		case EV_WATER_LEAVE:
		case EV_WATER_UNDER:
		case EV_WATER_CLEAR:
		case EV_ITEM_PICKUP:
		case EV_GLOBAL_ITEM_PICKUP:
		case EV_NOAMMO:
		case EV_CHANGE_WEAPON:
		case EV_FIRE_WEAPON:
			//FIXME: either add to sound queue or mark player as someone making noise
			break;
		case EV_USE_ITEM0:
		case EV_USE_ITEM1:
		case EV_USE_ITEM2:
		case EV_USE_ITEM3:
		case EV_USE_ITEM4:
		case EV_USE_ITEM5:
		case EV_USE_ITEM6:
		case EV_USE_ITEM7:
		case EV_USE_ITEM8:
		case EV_USE_ITEM9:
		case EV_USE_ITEM10:
		case EV_USE_ITEM11:
		case EV_USE_ITEM12:
		case EV_USE_ITEM13:
		case EV_USE_ITEM14:
			break;
	}
}

void BotCheckSnapshot(bot_state_t *bs) {
	int ent;
	entityState_t state;

	//remove all avoid spots
	trap->ai->BotAddAvoidSpot(bs->ms, &vec3_origin, 0, AVOID_CLEAR);
	//
	ent = 0;
	while( ( ent = BotAI_GetSnapshotEntity( bs->client, ent, &state ) ) != -1 ) {
		//check the entity state for events
		BotCheckEvents(bs, &state);
	}
	//check the player state for events
	BotAI_GetEntityState(bs->client, &state);
	//copy the player state events to the entity state
	state.event = bs->cur_ps.externalEvent;
	state.eventParm = bs->cur_ps.externalEventParm;
	//
	BotCheckEvents(bs, &state);
}

void BotCheckAir(bot_state_t *bs) {
	if ( trap->aas->AAS_PointContents( &bs->eye ) & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA) )
		return;
	bs->lastair_time = FloatTime();
}

bot_goal_t *BotAlternateRoute(bot_state_t *bs, bot_goal_t *goal) {
	int t;

	// if the bot has an alternative route goal
	if (bs->altroutegoal.areanum) {
		//
		if (bs->reachedaltroutegoal_time)
			return goal;
		// travel time towards alternative route goal
		t = trap->aas->AAS_AreaTravelTimeToGoalArea(bs->areanum, &bs->origin, bs->altroutegoal.areanum, bs->tfl);
		if (t && t < 20) {
			//BotAI_Print(PRT_MESSAGE, "reached alternate route goal\n");
			bs->reachedaltroutegoal_time = FloatTime();
		}
		memcpy(goal, &bs->altroutegoal, sizeof(bot_goal_t));
		return &bs->altroutegoal;
	}
	return goal;
}

int BotGetAlternateRouteGoal(bot_state_t *bs, int base) {
	aas_altroutegoal_t *altroutegoals;
	bot_goal_t *goal;
	int numaltroutegoals, rnd;

	if (base == TEAM_RED) {
		altroutegoals = red_altroutegoals;
		numaltroutegoals = red_numaltroutegoals;
	}
	else {
		altroutegoals = blue_altroutegoals;
		numaltroutegoals = blue_numaltroutegoals;
	}
	if (!numaltroutegoals)
		return qfalse;
	rnd = (int)(random() * numaltroutegoals);
	if (rnd >= numaltroutegoals)
		rnd = numaltroutegoals-1;
	goal = &bs->altroutegoal;
	goal->areanum = altroutegoals[rnd].areanum;
	VectorCopy(&altroutegoals[rnd].origin, &goal->origin);
	VectorSet(&goal->mins, -8, -8, -8);
	VectorSet(&goal->maxs, 8, 8, 8);
	goal->entitynum = 0;
	goal->iteminfo = 0;
	goal->number = 0;
	goal->flags = 0;
	//
	bs->reachedaltroutegoal_time = 0;
	return qtrue;
}

void BotSetupAlternativeRouteGoals( void ) {

	if (altroutegoals_setup)
		return;
	if (gametype == GT_FLAGS) {
		if (trap->ai->BotGetLevelItemGoal(-1, "Neutral Flag", &ctf_neutralflag) < 0)
			BotAI_Print(PRT_WARNING, "No alt routes without Neutral Flag\n");
		if (ctf_neutralflag.areanum) {
			//
			red_numaltroutegoals = trap->aas->AAS_AlternativeRouteGoals(
										&ctf_neutralflag.origin, ctf_neutralflag.areanum,
										&ctf_redflag.origin, ctf_redflag.areanum, TFL_DEFAULT,
										red_altroutegoals, MAX_ALTROUTEGOALS,
										ALTROUTEGOAL_CLUSTERPORTALS|
										ALTROUTEGOAL_VIEWPORTALS);
			blue_numaltroutegoals = trap->aas->AAS_AlternativeRouteGoals(
										&ctf_neutralflag.origin, ctf_neutralflag.areanum,
										&ctf_blueflag.origin, ctf_blueflag.areanum, TFL_DEFAULT,
										blue_altroutegoals, MAX_ALTROUTEGOALS,
										ALTROUTEGOAL_CLUSTERPORTALS|
										ALTROUTEGOAL_VIEWPORTALS);
		}
	}
	else if (gametype == GT_TROJAN) {
		if (trap->ai->BotGetLevelItemGoal(-1, "Neutral Obelisk", &neutralobelisk) < 0)
			BotAI_Print(PRT_WARNING, "One Flag CTF without Neutral Obelisk\n");
		red_numaltroutegoals = trap->aas->AAS_AlternativeRouteGoals(
									&ctf_neutralflag.origin, ctf_neutralflag.areanum,
									&ctf_redflag.origin, ctf_redflag.areanum, TFL_DEFAULT,
									red_altroutegoals, MAX_ALTROUTEGOALS,
									ALTROUTEGOAL_CLUSTERPORTALS|
									ALTROUTEGOAL_VIEWPORTALS);
		blue_numaltroutegoals = trap->aas->AAS_AlternativeRouteGoals(
									&ctf_neutralflag.origin, ctf_neutralflag.areanum,
									&ctf_blueflag.origin, ctf_blueflag.areanum, TFL_DEFAULT,
									blue_altroutegoals, MAX_ALTROUTEGOALS,
									ALTROUTEGOAL_CLUSTERPORTALS|
									ALTROUTEGOAL_VIEWPORTALS);
	}
	altroutegoals_setup = qtrue;
}

void BotDeathmatchAI(bot_state_t *bs, float thinktime) {
	char name[144], buf[144];
	int i;

	//if the bot has just been setup
	if (bs->setupcount > 0) {
		bs->setupcount--;
		if (bs->setupcount > 0) return;
		//set the team
		if ( !bs->map_restart && level.gametype != GT_DUEL ) {
			Com_sprintf(buf, sizeof(buf), "team %s", bs->settings.team);
			trap->ea->EA_Command(bs->client, buf);
		}
		//set the chat name
		ClientName(bs->client, name, sizeof(name));
		trap->ai->BotSetChatName(bs->cs, name, bs->client);
		//
		bs->lastframe_health = bs->inventory[INVENTORY_HEALTH];
		bs->lasthitcount = bs->cur_ps.persistent[PERS_HITS];
		//
		bs->setupcount = 0;
		//
		BotSetupAlternativeRouteGoals();
	}
	//no ideal view set
	bs->flags &= ~BFL_IDEALVIEWSET;
	//
	if (!BotIntermission(bs)) {
		//set the teleport time
		BotSetTeleportTime(bs);
		//update some inventory values
		BotUpdateInventory(bs);
		//check out the snapshot
		BotCheckSnapshot(bs);
		//check for air
		BotCheckAir(bs);
	}
	//check the console messages
	BotCheckConsoleMessages(bs);

	//if the bot has no ai node
	if (!bs->ainode) {
		AIEnter_Seek_LTG(bs, "BotDeathmatchAI: no ai node");
	}
	//if the bot entered the game less than 8 seconds ago
	if (!bs->entergamechat && bs->entergame_time > FloatTime() - 8) {
		if (BotChat_EnterGame(bs)) {
			bs->stand_time = FloatTime() + BotChatTime(bs);
			AIEnter_Stand(bs, "BotDeathmatchAI: chat enter game");
		}
		bs->entergamechat = qtrue;
	}
	//reset the node switches from the previous frame
	BotResetNodeSwitches();
	//execute AI nodes
	for (i = 0; i < MAX_NODESWITCHES; i++) {
		if (bs->ainode(bs)) break;
	}
	//if the bot removed itself :)
	if (!bs->inuse) return;
	//if the bot executed too many AI nodes
	if (i >= MAX_NODESWITCHES) {
		trap->ai->BotDumpGoalStack(bs->gs);
		trap->ai->BotDumpAvoidGoals(bs->gs);
		BotDumpNodeSwitches(bs);
		ClientName(bs->client, name, sizeof(name));
		BotAI_Print(PRT_ERROR, "%s at %1.1f switched more than %d AI nodes\n", name, FloatTime(), MAX_NODESWITCHES);
	}
	//
	bs->lastframe_health = bs->inventory[INVENTORY_HEALTH];
	bs->lasthitcount = bs->cur_ps.persistent[PERS_HITS];
}

void BotSetEntityNumForGoalWithModel(bot_goal_t *goal, int eType, char *modelname) {
	gentity_t *ent;
	int i, modelindex;
	vector3 dir;

	modelindex = G_ModelIndex( modelname );
	ent = &g_entities[0];
	for (i = 0; i < level.num_entities; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}
		if ( eType && ent->s.eType != eType) {
			continue;
		}
		if (ent->s.modelindex != modelindex) {
			continue;
		}
		VectorSubtract(&goal->origin, &ent->s.origin, &dir);
		if (VectorLengthSquared(&dir) < Square(10)) {
			goal->entitynum = i;
			return;
		}
	}
}

void BotSetEntityNumForGoal(bot_goal_t *goal, char *classname) {
	gentity_t *ent;
	int i;
	vector3 dir;

	ent = &g_entities[0];
	for (i = 0; i < level.num_entities; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}
		if ( !Q_stricmp(ent->classname, classname) ) {
			continue;
		}
		VectorSubtract(&goal->origin, &ent->s.origin, &dir);
		if (VectorLengthSquared(&dir) < Square(10)) {
			goal->entitynum = i;
			return;
		}
	}
}

int BotGoalForBSPEntity( char *classname, bot_goal_t *goal ) {
	char value[MAX_INFO_STRING];
	vector3 origin, start, end;
	int ent, numareas, areas[10];

	memset(goal, 0, sizeof(bot_goal_t));
	for (ent = trap->aas->AAS_NextBSPEntity(0); ent; ent = trap->aas->AAS_NextBSPEntity(ent)) {
		if (!trap->aas->AAS_ValueForBSPEpairKey(ent, "classname", value, sizeof(value)))
			continue;
		if (!strcmp(value, classname)) {
			if (!trap->aas->AAS_VectorForBSPEpairKey(ent, "origin", &origin))
				return qfalse;
			VectorCopy(&origin, &goal->origin);
			VectorCopy(&origin, &start);
			start.z -= 32;
			VectorCopy(&origin, &end);
			end.z += 32;
			numareas = trap->aas->AAS_TraceAreas(&start, &end, areas, NULL, 10);
			if (!numareas)
				return qfalse;
			goal->areanum = areas[0];
			return qtrue;
		}
	}
	return qfalse;
}

void BotSetupDeathmatchAI( void ) {
	int ent, modelnum;
	char model[128];

	gametype = level.gametype;
	maxclients = trap->Cvar_VariableIntegerValue("sv_maxclients");

	bot_rocketjump			= trap->Cvar_Get( "bot_rocketjump", "1", 0, NULL, NULL );
	bot_grapple				= trap->Cvar_Get( "bot_grapple", "0", 0, NULL, NULL );
	bot_fastchat			= trap->Cvar_Get( "bot_fastchat", "0", 0, NULL, NULL );
	bot_nochat				= trap->Cvar_Get( "bot_nochat", "0", 0, NULL, NULL );
	bot_testrchat			= trap->Cvar_Get( "bot_testrchat", "0", 0, NULL, NULL );
	bot_challenge			= trap->Cvar_Get( "bot_challenge", "0", 0, NULL, NULL );
	bot_predictobstacles	= trap->Cvar_Get( "bot_predictobstacles", "1", 0, NULL, NULL );
	//
	if (gametype == GT_FLAGS) {
		if (trap->ai->BotGetLevelItemGoal(-1, "Red Flag", &ctf_redflag) < 0)
			BotAI_Print(PRT_WARNING, "CTF without Red Flag\n");
		if (trap->ai->BotGetLevelItemGoal(-1, "Blue Flag", &ctf_blueflag) < 0)
			BotAI_Print(PRT_WARNING, "CTF without Blue Flag\n");
	}
	else if (gametype == GT_TROJAN) {
		if (trap->ai->BotGetLevelItemGoal(-1, "Neutral Flag", &ctf_neutralflag) < 0)
			BotAI_Print(PRT_WARNING, "One Flag CTF without Neutral Flag\n");
		if (trap->ai->BotGetLevelItemGoal(-1, "Red Flag", &ctf_redflag) < 0)
			BotAI_Print(PRT_WARNING, "One Flag CTF without Red Flag\n");
		if (trap->ai->BotGetLevelItemGoal(-1, "Blue Flag", &ctf_blueflag) < 0)
			BotAI_Print(PRT_WARNING, "One Flag CTF without Blue Flag\n");
	}
	max_bspmodelindex = 0;
	for (ent = trap->aas->AAS_NextBSPEntity(0); ent; ent = trap->aas->AAS_NextBSPEntity(ent)) {
		if (!trap->aas->AAS_ValueForBSPEpairKey(ent, "model", model, sizeof(model))) continue;
		if (model[0] == '*') {
			modelnum = atoi(model+1);
			if (modelnum > max_bspmodelindex)
				max_bspmodelindex = modelnum;
		}
	}
	//initialize the waypoint heap
	BotInitWaypoints();
}

void BotShutdownDeathmatchAI( void ) {
	altroutegoals_setup = qfalse;
}
