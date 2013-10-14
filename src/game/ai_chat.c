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
 * name:		ai_chat.c
 *
 * desc:		Quake3 bot AI
 *
 * $Archive: /MissionPack/code/game/ai_chat.c $
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
//
#include "chars.h"				//characteristics
#include "inv.h"				//indexes into the inventory
#include "syn.h"				//synonyms
#include "match.h"				//string matching types and vars

// for the voice chats
#include "../../build/qtz/ui/menudef.h"

#include "bg_local.h"

#define TIME_BETWEENCHATTING	25

int BotNumActivePlayers( void ) {
	int i, num;
	char buf[MAX_INFO_STRING];
	static int maxclients;

	if (!maxclients)
		maxclients = trap->Cvar_VariableIntegerValue("sv_maxclients");

	num = 0;
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		trap->SV_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) continue;
		//skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) continue;
		//
		num++;
	}
	return num;
}

int BotIsFirstInRankings(bot_state_t *bs) {
	int i, score;
	char buf[MAX_INFO_STRING];
	static int maxclients;
	playerState_t ps;

	if (!maxclients)
		maxclients = trap->Cvar_VariableIntegerValue("sv_maxclients");

	score = bs->cur_ps.persistent[PERS_SCORE];
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		trap->SV_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) continue;
		//skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) continue;
		//
		BotAI_GetClientState(i, &ps);
		if (score < ps.persistent[PERS_SCORE]) return qfalse;
	}
	return qtrue;
}

int BotIsLastInRankings(bot_state_t *bs) {
	int i, score;
	char buf[MAX_INFO_STRING];
	static int maxclients;
	playerState_t ps;

	if (!maxclients)
		maxclients = trap->Cvar_VariableIntegerValue("sv_maxclients");

	score = bs->cur_ps.persistent[PERS_SCORE];
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		trap->SV_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) continue;
		//skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) continue;
		//
		BotAI_GetClientState(i, &ps);
		if (score > ps.persistent[PERS_SCORE]) return qfalse;
	}
	return qtrue;
}

char *BotFirstClientInRankings( void ) {
	int i, bestscore, bestclient;
	char buf[MAX_INFO_STRING];
	static char name[32];
	static int maxclients;
	playerState_t ps;

	if (!maxclients)
		maxclients = trap->Cvar_VariableIntegerValue("sv_maxclients");

	bestscore = -999999;
	bestclient = 0;
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		trap->SV_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) continue;
		//skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) continue;
		//
		BotAI_GetClientState(i, &ps);
		if (ps.persistent[PERS_SCORE] > bestscore) {
			bestscore = ps.persistent[PERS_SCORE];
			bestclient = i;
		}
	}
	EasyClientName(bestclient, name, 32);
	return name;
}

char *BotLastClientInRankings( void ) {
	int i, worstscore, bestclient;
	char buf[MAX_INFO_STRING];
	static char name[32];
	static int maxclients;
	playerState_t ps;

	if (!maxclients)
		maxclients = trap->Cvar_VariableIntegerValue("sv_maxclients");

	worstscore = 999999;
	bestclient = 0;
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		trap->SV_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) continue;
		//skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) continue;
		//
		BotAI_GetClientState(i, &ps);
		if (ps.persistent[PERS_SCORE] < worstscore) {
			worstscore = ps.persistent[PERS_SCORE];
			bestclient = i;
		}
	}
	EasyClientName(bestclient, name, 32);
	return name;
}

char *BotRandomOpponentName(bot_state_t *bs) {
	int i, count;
	char buf[MAX_INFO_STRING];
	int opponents[MAX_CLIENTS], numopponents;
	static int maxclients;
	static char name[32];

	if (!maxclients)
		maxclients = trap->Cvar_VariableIntegerValue("sv_maxclients");

	numopponents = 0;
	opponents[0] = 0;
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		if (i == bs->client) continue;
		//
		trap->SV_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) continue;
		//skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) continue;
		//skip team mates
		if (BotSameTeam(bs, i)) continue;
		//
		opponents[numopponents] = i;
		numopponents++;
	}
	count = (int)(random() * numopponents);
	for (i = 0; i < numopponents; i++) {
		count--;
		if (count <= 0) {
			EasyClientName(opponents[i], name, sizeof(name));
			return name;
		}
	}
	EasyClientName(opponents[0], name, sizeof(name));
	return name;
}

char *BotMapTitle( void ) {
	char info[1024];
	static char mapname[128];

	trap->SV_GetServerinfo(info, sizeof(info));

	strncpy(mapname, Info_ValueForKey( info, "mapname" ), sizeof(mapname)-1);
	mapname[sizeof(mapname)-1] = '\0';

	return mapname;
}

const char *BotWeaponNameForMeansOfDeath(int mod) {
	return weaponNames[weaponFromMOD[mod]].longName;
}

char *BotRandomWeaponName( void ) {
	int rnd;

	//RAZMARK: Adding new weapons
	rnd = (int)(random() * WP_NUM_WEAPONS-0.1f);
	return (char *)weaponNames[rnd].longName;
}

int BotVisibleEnemies(bot_state_t *bs) {
	float vis;
	int i;
	aas_entityinfo_t entinfo;

	for (i = 0; i < MAX_CLIENTS; i++) {

		if (i == bs->client) continue;
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
		//if on the same team
		if (BotSameTeam(bs, i)) continue;
		//check if the enemy is visible
		vis = BotEntityVisible(bs->entitynum, &bs->eye, &bs->viewangles, 360, i);
		if (vis > 0) return qtrue;
	}
	return qfalse;
}

int BotValidChatPosition(bot_state_t *bs) {
	vector3 point, start, end, mins, maxs;
	bsp_trace_t trace;

	//if the bot is dead all positions are valid
	if (BotIsDead(bs)) return qtrue;
	//never start chatting with a powerup
	if ( bs->inventory[INVENTORY_QUAD] || bs->inventory[INVENTORY_REGEN] )
		return qfalse;
	//must be on the ground
	//if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE) return qfalse;
	//do not chat if in lava or slime
	VectorCopy(&bs->origin, &point);
	point.z -= 24;
	if (trap->SV_PointContents(&point,bs->entitynum) & (CONTENTS_LAVA|CONTENTS_SLIME)) return qfalse;
	//do not chat if under water
	VectorCopy(&bs->origin, &point);
	point.z += 32;
	if (trap->SV_PointContents(&point,bs->entitynum) & MASK_WATER) return qfalse;
	//must be standing on the world entity
	VectorCopy(&bs->origin, &start);
	VectorCopy(&bs->origin, &end);
	start.z += 1;
	end.z -= 10;
	trap->aas->AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, &mins, &maxs);
	BotAI_Trace(&trace, &start, &mins, &maxs, &end, bs->client, MASK_SOLID);
	if (trace.ent != ENTITYNUM_WORLD) return qfalse;
	//the bot is in a position where it can chat
	return qtrue;
}

int BotChat_EnterGame(bot_state_t *bs) {
	char name[32];
	float rnd;

	if (bot_nochat->integer) return qfalse;
	if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) return qfalse;
	//don't chat in teamplay
	if (TeamPlayIsOn()) return qfalse;
	// don't chat in tournament mode
	if (gametype == GT_DUEL) return qfalse;
	rnd = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_ENTEREXITGAME, 0, 1);
	if (!bot_fastchat->integer) {
		if (random() > rnd) return qfalse;
	}
	if (BotNumActivePlayers() <= 1) return qfalse;
	if (!BotValidChatPosition(bs)) return qfalse;
	BotAI_BotInitialChat(bs, "game_enter",
				EasyClientName(bs->client, name, 32),	// 0
				BotRandomOpponentName(bs),				// 1
				"[invalid var]",						// 2
				"[invalid var]",						// 3
				BotMapTitle(),							// 4
				NULL);
	bs->lastchat_time = FloatTime();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

int BotChat_ExitGame(bot_state_t *bs) {
	char name[32];
	float rnd;

	if (bot_nochat->integer) return qfalse;
	if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) return qfalse;
	//don't chat in teamplay
	if (TeamPlayIsOn()) return qfalse;
	// don't chat in tournament mode
	if (gametype == GT_DUEL) return qfalse;
	rnd = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_ENTEREXITGAME, 0, 1);
	if (!bot_fastchat->integer) {
		if (random() > rnd) return qfalse;
	}
	if (BotNumActivePlayers() <= 1) return qfalse;
	//
	BotAI_BotInitialChat(bs, "game_exit",
				EasyClientName(bs->client, name, 32),	// 0
				BotRandomOpponentName(bs),				// 1
				"[invalid var]",						// 2
				"[invalid var]",						// 3
				BotMapTitle(),							// 4
				NULL);
	bs->lastchat_time = FloatTime();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

int BotChat_StartLevel(bot_state_t *bs) {
	char name[32];
	float rnd;

	if (bot_nochat->integer) return qfalse;
	if (BotIsObserver(bs)) return qfalse;
	if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) return qfalse;
	//don't chat in teamplay
	if (TeamPlayIsOn()) {
	    trap->ea->EA_Command(bs->client, "vtaunt");
	    return qfalse;
	}
	// don't chat in tournament mode
	if (gametype == GT_DUEL) return qfalse;
	rnd = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_STARTENDLEVEL, 0, 1);
	if (!bot_fastchat->integer) {
		if (random() > rnd) return qfalse;
	}
	if (BotNumActivePlayers() <= 1) return qfalse;
	BotAI_BotInitialChat(bs, "level_start",
				EasyClientName(bs->client, name, 32),	// 0
				NULL);
	bs->lastchat_time = FloatTime();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

int BotChat_EndLevel(bot_state_t *bs) {
	char name[32];
	float rnd;

	if (bot_nochat->integer) return qfalse;
	if (BotIsObserver(bs)) return qfalse;
	if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) return qfalse;
	// teamplay
	if (TeamPlayIsOn()) 
	{
		if (BotIsFirstInRankings(bs)) {
			trap->ea->EA_Command(bs->client, "vtaunt");
		}
		return qtrue;
	}
	// don't chat in tournament mode
	if (gametype == GT_DUEL) return qfalse;
	rnd = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_STARTENDLEVEL, 0, 1);
	if (!bot_fastchat->integer) {
		if (random() > rnd) return qfalse;
	}
	if (BotNumActivePlayers() <= 1) return qfalse;
	//
	if (BotIsFirstInRankings(bs)) {
		BotAI_BotInitialChat(bs, "level_end_victory",
				EasyClientName(bs->client, name, 32),	// 0
				BotRandomOpponentName(bs),				// 1
				"[invalid var]",						// 2
				BotLastClientInRankings(),				// 3
				BotMapTitle(),							// 4
				NULL);
	}
	else if (BotIsLastInRankings(bs)) {
		BotAI_BotInitialChat(bs, "level_end_lose",
				EasyClientName(bs->client, name, 32),	// 0
				BotRandomOpponentName(bs),				// 1
				BotFirstClientInRankings(),				// 2
				"[invalid var]",						// 3
				BotMapTitle(),							// 4
				NULL);
	}
	else {
		BotAI_BotInitialChat(bs, "level_end",
				EasyClientName(bs->client, name, 32),	// 0
				BotRandomOpponentName(bs),				// 1
				BotFirstClientInRankings(),				// 2
				BotLastClientInRankings(),				// 3
				BotMapTitle(),							// 4
				NULL);
	}
	bs->lastchat_time = FloatTime();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

int BotChat_Death(bot_state_t *bs) {
	char name[32];
	float rnd;

	if (bot_nochat->integer) return qfalse;
	if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) return qfalse;
	rnd = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_DEATH, 0, 1);
	// don't chat in tournament mode
	if (gametype == GT_DUEL) return qfalse;
	//if fast chatting is off
	if (!bot_fastchat->integer) {
		if (random() > rnd) return qfalse;
	}
	if (BotNumActivePlayers() <= 1) return qfalse;
	//
	if (bs->lastkilledby >= 0 && bs->lastkilledby < MAX_CLIENTS)
		EasyClientName(bs->lastkilledby, name, 32);
	else
		strcpy(name, "[world]");
	//
	if (TeamPlayIsOn() && BotSameTeam(bs, bs->lastkilledby)) {
		if (bs->lastkilledby == bs->client) return qfalse;
		BotAI_BotInitialChat(bs, "death_teammate", name, NULL);
		bs->chatto = CHAT_TEAM;
	}
	else
	{
		//teamplay
		if (TeamPlayIsOn()) {
			trap->ea->EA_Command(bs->client, "vtaunt");
			return qtrue;
		}
		//
		if (bs->botdeathtype == MOD_WATER)
			BotAI_BotInitialChat(bs, "death_drown", BotRandomOpponentName(bs), NULL);
		else if (bs->botdeathtype == MOD_SLIME)
			BotAI_BotInitialChat(bs, "death_slime", BotRandomOpponentName(bs), NULL);
		else if (bs->botdeathtype == MOD_LAVA)
			BotAI_BotInitialChat(bs, "death_lava", BotRandomOpponentName(bs), NULL);
		else if (bs->botdeathtype == MOD_FALLING)
			BotAI_BotInitialChat(bs, "death_cratered", BotRandomOpponentName(bs), NULL);
		else if (bs->botsuicide || //all other suicides by own weapon
				bs->botdeathtype == MOD_CRUSH ||
				bs->botdeathtype == MOD_SUICIDE ||
				bs->botdeathtype == MOD_TARGET_LASER ||
				bs->botdeathtype == MOD_TRIGGER_HURT ||
				bs->botdeathtype == MOD_UNKNOWN)
			BotAI_BotInitialChat(bs, "death_suicide", BotRandomOpponentName(bs), NULL);
		else if (bs->botdeathtype == MOD_TELEFRAG)
			BotAI_BotInitialChat(bs, "death_telefrag", name, NULL);
		else {
			//choose between insult and praise
			if (random() < trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_INSULT, 0, 1)) {
				BotAI_BotInitialChat(bs, "death_insult",
							name,												// 0
							BotWeaponNameForMeansOfDeath(bs->botdeathtype),		// 1
							NULL);
			}
			else {
				BotAI_BotInitialChat(bs, "death_praise",
							name,												// 0
							BotWeaponNameForMeansOfDeath(bs->botdeathtype),		// 1
							NULL);
			}
		}
		bs->chatto = CHAT_ALL;
	}
	bs->lastchat_time = FloatTime();
	return qtrue;
}

int BotChat_Kill(bot_state_t *bs) {
	char name[32];
	float rnd;

	if (bot_nochat->integer) return qfalse;
	if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) return qfalse;
	rnd = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_KILL, 0, 1);
	// don't chat in tournament mode
	if (gametype == GT_DUEL) return qfalse;
	//if fast chat is off
	if (!bot_fastchat->integer) {
		if (random() > rnd) return qfalse;
	}
	if (bs->lastkilledplayer == bs->client) return qfalse;
	if (BotNumActivePlayers() <= 1) return qfalse;
	if (!BotValidChatPosition(bs)) return qfalse;
	//
	if (BotVisibleEnemies(bs)) return qfalse;
	//
	EasyClientName(bs->lastkilledplayer, name, 32);
	//
	bs->chatto = CHAT_ALL;
	if (TeamPlayIsOn() && BotSameTeam(bs, bs->lastkilledplayer)) {
		BotAI_BotInitialChat(bs, "kill_teammate", name, NULL);
		bs->chatto = CHAT_TEAM;
	}
	else
	{
		//don't chat in teamplay
		if (TeamPlayIsOn()) {
			trap->ea->EA_Command(bs->client, "vtaunt");
			return qfalse;			// don't wait
		}
		//
		if (bs->enemydeathtype == MOD_TELEFRAG) {
			BotAI_BotInitialChat(bs, "kill_telefrag", name, NULL);
		}
		//choose between insult and praise
		else if (random() < trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_INSULT, 0, 1)) {
			BotAI_BotInitialChat(bs, "kill_insult", name, NULL);
		}
		else {
			BotAI_BotInitialChat(bs, "kill_praise", name, NULL);
		}
	}
	bs->lastchat_time = FloatTime();
	return qtrue;
}

int BotChat_EnemySuicide(bot_state_t *bs) {
	char name[32];
	float rnd;

	if (bot_nochat->integer) return qfalse;
	if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) return qfalse;
	if (BotNumActivePlayers() <= 1) return qfalse;
	//
	rnd = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_ENEMYSUICIDE, 0, 1);
	//don't chat in teamplay
	if (TeamPlayIsOn()) return qfalse;
	// don't chat in tournament mode
	if (gametype == GT_DUEL) return qfalse;
	//if fast chat is off
	if (!bot_fastchat->integer) {
		if (random() > rnd) return qfalse;
	}
	if (!BotValidChatPosition(bs)) return qfalse;
	//
	if (BotVisibleEnemies(bs)) return qfalse;
	//
	if (bs->enemy >= 0) EasyClientName(bs->enemy, name, 32);
	else strcpy(name, "");
	BotAI_BotInitialChat(bs, "enemy_suicide", name, NULL);
	bs->lastchat_time = FloatTime();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

int BotChat_HitTalking(bot_state_t *bs) {
	char name[32], *weap;
	int lasthurt_client;
	float rnd;

	if (bot_nochat->integer) return qfalse;
	if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) return qfalse;
	if (BotNumActivePlayers() <= 1) return qfalse;
	lasthurt_client = g_entities[bs->client].client->lasthurt_client;
	if (!lasthurt_client) return qfalse;
	if (lasthurt_client == bs->client) return qfalse;
	//
	if (lasthurt_client < 0 || lasthurt_client >= MAX_CLIENTS) return qfalse;
	//
	rnd = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_HITTALKING, 0, 1);
	//don't chat in teamplay
	if (TeamPlayIsOn()) return qfalse;
	// don't chat in tournament mode
	if (gametype == GT_DUEL) return qfalse;
	//if fast chat is off
	if (!bot_fastchat->integer) {
		if (random() > rnd * 0.5f) return qfalse;
	}
	if (!BotValidChatPosition(bs)) return qfalse;
	//
	ClientName(g_entities[bs->client].client->lasthurt_client, name, sizeof(name));
	weap = (char*)BotWeaponNameForMeansOfDeath(g_entities[bs->client].client->lasthurt_mod);
	//
	BotAI_BotInitialChat(bs, "hit_talking", name, weap, NULL);
	bs->lastchat_time = FloatTime();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

int BotChat_HitNoDeath(bot_state_t *bs) {
	char name[32], *weap;
	float rnd;
	int lasthurt_client;
	aas_entityinfo_t entinfo;

	lasthurt_client = g_entities[bs->client].client->lasthurt_client;
	if (!lasthurt_client) return qfalse;
	if (lasthurt_client == bs->client) return qfalse;
	//
	if (lasthurt_client < 0 || lasthurt_client >= MAX_CLIENTS) return qfalse;
	//
	if (bot_nochat->integer) return qfalse;
	if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) return qfalse;
	if (BotNumActivePlayers() <= 1) return qfalse;
	rnd = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_HITNODEATH, 0, 1);
	//don't chat in teamplay
	if (TeamPlayIsOn()) return qfalse;
	// don't chat in tournament mode
	if (gametype == GT_DUEL) return qfalse;
	//if fast chat is off
	if (!bot_fastchat->integer) {
		if (random() > rnd * 0.5f) return qfalse;
	}
	if (!BotValidChatPosition(bs)) return qfalse;
	//
	if (BotVisibleEnemies(bs)) return qfalse;
	//
	BotEntityInfo(bs->enemy, &entinfo);
	if (EntityIsShooting(&entinfo)) return qfalse;
	//
	ClientName(lasthurt_client, name, sizeof(name));
	weap = (char*)BotWeaponNameForMeansOfDeath(g_entities[bs->client].client->lasthurt_mod);
	//
	BotAI_BotInitialChat(bs, "hit_nodeath", name, weap, NULL);
	bs->lastchat_time = FloatTime();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

int BotChat_HitNoKill(bot_state_t *bs) {
	char name[32], *weap;
	float rnd;
	aas_entityinfo_t entinfo;

	if (bot_nochat->integer) return qfalse;
	if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) return qfalse;
	if (BotNumActivePlayers() <= 1) return qfalse;
	rnd = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_HITNOKILL, 0, 1);
	//don't chat in teamplay
	if (TeamPlayIsOn()) return qfalse;
	// don't chat in tournament mode
	if (gametype == GT_DUEL) return qfalse;
	//if fast chat is off
	if (!bot_fastchat->integer) {
		if (random() > rnd * 0.5f) return qfalse;
	}
	if (!BotValidChatPosition(bs)) return qfalse;
	//
	if (BotVisibleEnemies(bs)) return qfalse;
	//
	BotEntityInfo(bs->enemy, &entinfo);
	if (EntityIsShooting(&entinfo)) return qfalse;
	//
	ClientName(bs->enemy, name, sizeof(name));
	weap = (char*)BotWeaponNameForMeansOfDeath(g_entities[bs->enemy].client->lasthurt_mod);
	//
	BotAI_BotInitialChat(bs, "hit_nokill", name, weap, NULL);
	bs->lastchat_time = FloatTime();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

int BotChat_Random(bot_state_t *bs) {
	float rnd;
	char name[32];

	if (bot_nochat->integer) return qfalse;
	if (BotIsObserver(bs)) return qfalse;
	if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) return qfalse;
	// don't chat in tournament mode
	if (gametype == GT_DUEL) return qfalse;
	//don't chat when doing something important :)
	if (bs->ltgtype == LTG_TEAMHELP ||
		bs->ltgtype == LTG_TEAMACCOMPANY ||
		bs->ltgtype == LTG_RUSHBASE) return qfalse;
	//
	rnd = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_RANDOM, 0, 1);
	if (random() > bs->thinktime * 0.1f) return qfalse;
	if (!bot_fastchat->integer) {
		if (random() > rnd) return qfalse;
		if (random() > 0.25f) return qfalse;
	}
	if (BotNumActivePlayers() <= 1) return qfalse;
	//
	if (!BotValidChatPosition(bs)) return qfalse;
	//
	if (BotVisibleEnemies(bs)) return qfalse;
	//
	if (bs->lastkilledplayer == bs->client) {
		strcpy(name, BotRandomOpponentName(bs));
	}
	else {
		EasyClientName(bs->lastkilledplayer, name, sizeof(name));
	}
	if (TeamPlayIsOn()) {
		trap->ea->EA_Command(bs->client, "vtaunt");
		return qfalse;			// don't wait
	}
	//
	if (random() < trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_MISC, 0, 1)) {
		BotAI_BotInitialChat(bs, "random_misc",
					BotRandomOpponentName(bs),	// 0
					name,						// 1
					"[invalid var]",			// 2
					"[invalid var]",			// 3
					BotMapTitle(),				// 4
					BotRandomWeaponName(),		// 5
					NULL);
	}
	else {
		BotAI_BotInitialChat(bs, "random_insult",
					BotRandomOpponentName(bs),	// 0
					name,						// 1
					"[invalid var]",			// 2
					"[invalid var]",			// 3
					BotMapTitle(),				// 4
					BotRandomWeaponName(),		// 5
					NULL);
	}
	bs->lastchat_time = FloatTime();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

float BotChatTime(bot_state_t *bs) {
	int cpm;

	cpm = trap->ai->Characteristic_BInteger(bs->character, CHARACTERISTIC_CHAT_CPM, 1, 4000);

	return (float)trap->ai->BotChatLength(bs->cs) * 30 / cpm;
}

void BotChatTest(bot_state_t *bs) {

	char name[32];
	char *weap;
	int num, i;

	num = trap->ai->BotNumInitialChats(bs->cs, "game_enter");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "game_enter",
					EasyClientName(bs->client, name, 32),	// 0
					BotRandomOpponentName(bs),				// 1
					"[invalid var]",						// 2
					"[invalid var]",						// 3
					BotMapTitle(),							// 4
					NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "game_exit");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "game_exit",
					EasyClientName(bs->client, name, 32),	// 0
					BotRandomOpponentName(bs),				// 1
					"[invalid var]",						// 2
					"[invalid var]",						// 3
					BotMapTitle(),							// 4
					NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "level_start");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "level_start",
					EasyClientName(bs->client, name, 32),	// 0
					NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "level_end_victory");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "level_end_victory",
				EasyClientName(bs->client, name, 32),	// 0
				BotRandomOpponentName(bs),				// 1
				BotFirstClientInRankings(),				// 2
				BotLastClientInRankings(),				// 3
				BotMapTitle(),							// 4
				NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "level_end_lose");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "level_end_lose",
				EasyClientName(bs->client, name, 32),	// 0
				BotRandomOpponentName(bs),				// 1
				BotFirstClientInRankings(),				// 2
				BotLastClientInRankings(),				// 3
				BotMapTitle(),							// 4
				NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "level_end");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "level_end",
				EasyClientName(bs->client, name, 32),	// 0
				BotRandomOpponentName(bs),				// 1
				BotFirstClientInRankings(),				// 2
				BotLastClientInRankings(),				// 3
				BotMapTitle(),							// 4
				NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	EasyClientName(bs->lastkilledby, name, sizeof(name));
	num = trap->ai->BotNumInitialChats(bs->cs, "death_drown");
	for (i = 0; i < num; i++)
	{
		//
		BotAI_BotInitialChat(bs, "death_drown", name, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "death_slime");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "death_slime", name, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "death_lava");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "death_lava", name, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "death_cratered");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "death_cratered", name, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "death_suicide");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "death_suicide", name, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "death_telefrag");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "death_telefrag", name, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "death_insult");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "death_insult",
					name,												// 0
					BotWeaponNameForMeansOfDeath(bs->botdeathtype),		// 1
					NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "death_praise");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "death_praise",
					name,												// 0
					BotWeaponNameForMeansOfDeath(bs->botdeathtype),		// 1
					NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	//
	EasyClientName(bs->lastkilledplayer, name, 32);
	//
	num = trap->ai->BotNumInitialChats(bs->cs, "kill_telefrag");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "kill_telefrag", name, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "kill_insult");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "kill_insult", name, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "kill_praise");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "kill_praise", name, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "enemy_suicide");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "enemy_suicide", name, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	ClientName(g_entities[bs->client].client->lasthurt_client, name, sizeof(name));
	weap = (char*)BotWeaponNameForMeansOfDeath(g_entities[bs->client].client->lasthurt_client);
	num = trap->ai->BotNumInitialChats(bs->cs, "hit_talking");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "hit_talking", name, weap, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "hit_nodeath");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "hit_nodeath", name, weap, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "hit_nokill");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "hit_nokill", name, weap, NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	//
	if (bs->lastkilledplayer == bs->client) {
		strcpy(name, BotRandomOpponentName(bs));
	}
	else {
		EasyClientName(bs->lastkilledplayer, name, sizeof(name));
	}
	//
	num = trap->ai->BotNumInitialChats(bs->cs, "random_misc");
	for (i = 0; i < num; i++)
	{
		//
		BotAI_BotInitialChat(bs, "random_misc",
					BotRandomOpponentName(bs),	// 0
					name,						// 1
					"[invalid var]",			// 2
					"[invalid var]",			// 3
					BotMapTitle(),				// 4
					BotRandomWeaponName(),		// 5
					NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
	num = trap->ai->BotNumInitialChats(bs->cs, "random_insult");
	for (i = 0; i < num; i++)
	{
		BotAI_BotInitialChat(bs, "random_insult",
					BotRandomOpponentName(bs),	// 0
					name,						// 1
					"[invalid var]",			// 2
					"[invalid var]",			// 3
					BotMapTitle(),				// 4
					BotRandomWeaponName(),		// 5
					NULL);
		trap->ai->BotEnterChat(bs->cs, 0, CHAT_ALL);
	}
}
