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
 * name:		ai_main.c
 *
 * desc:		Quake3 bot AI
 *
 * $Archive: /MissionPack/code/game/ai_main.c $
 *
 *****************************************************************************/


#include "g_local.h"
#include "../qcommon/q_shared.h"
#include "../botlib/botlib.h"		//bot lib interface
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
#include "ai_vcmd.h"

//
#include "chars.h"
#include "inv.h"
#include "syn.h"

#ifndef MAX_PATH
#define MAX_PATH		144
#endif


//bot states
bot_state_t	*botstates[MAX_CLIENTS];
//number of bots
int numbots;
//floating point time
float floattime;
//time to do a regular update
float regularupdate_time;
//
int bot_interbreed;
int bot_interbreedmatchcount;
//
cvar_t *bot_thinktime;
cvar_t *bot_memorydump;
cvar_t *bot_saveroutingcache;
cvar_t *bot_pause;
cvar_t *bot_report;
cvar_t *bot_testsolid;
cvar_t *bot_testclusters;
cvar_t *bot_developer;
cvar_t *bot_interbreedchar;
cvar_t *bot_interbreedbots;
cvar_t *bot_interbreedcycle;
cvar_t *bot_interbreedwrite;


void ExitLevel( void );

void QDECL BotAI_Print(int type, char *fmt, ...) {
	char str[2048];
	va_list ap;

	va_start(ap, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);

	switch(type) {
		case PRT_MESSAGE: {
			trap->Print("%s", str);
			break;
		}
		case PRT_WARNING: {
			trap->Print( S_COLOR_YELLOW "Warning: %s", str );
			break;
		}
		case PRT_ERROR: {
			trap->Print( S_COLOR_RED "Error: %s", str );
			break;
		}
		case PRT_FATAL: {
			trap->Print( S_COLOR_RED "Fatal: %s", str );
			break;
		}
		case PRT_EXIT: {
			trap->Error( ERR_DROP, S_COLOR_RED "Exit: %s", str );
			break;
		}
		default: {
			trap->Print( "unknown print type\n" );
			break;
		}
	}
}

void BotAI_Trace(bsp_trace_t *bsptrace, vector3 *start, vector3 *mins, vector3 *maxs, vector3 *end, int passent, int contentmask) {
	trace_t trace;

	trap->SV_Trace(&trace, start, mins, maxs, end, passent, contentmask);
	//copy the trace information
	bsptrace->allsolid = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy(&trace.endpos, &bsptrace->endpos);
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy(&trace.plane.normal, &bsptrace->plane.normal);
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}

int BotAI_GetClientState( int clientNum, playerState_t *state ) {
	gentity_t	*ent;

	ent = &g_entities[clientNum];
	if ( !ent->inuse ) {
		return qfalse;
	}
	if ( !ent->client ) {
		return qfalse;
	}

	memcpy( state, &ent->client->ps, sizeof(playerState_t) );
	return qtrue;
}

int BotAI_GetEntityState( int entityNum, entityState_t *state ) {
	gentity_t	*ent;

	ent = &g_entities[entityNum];
	memset( state, 0, sizeof(entityState_t) );
	if (!ent->inuse) return qfalse;
	if (!ent->r.linked) return qfalse;
	if (ent->r.svFlags & SVF_NOCLIENT) return qfalse;
	memcpy( state, &ent->s, sizeof(entityState_t) );
	return qtrue;
}

int BotAI_GetSnapshotEntity( int clientNum, int sequence, entityState_t *state ) {
	int		entNum;

	entNum = trap->SV_BotGetSnapshotEntity( clientNum, sequence );
	if ( entNum == -1 ) {
		memset(state, 0, sizeof(entityState_t));
		return -1;
	}

	BotAI_GetEntityState( entNum, state );

	return sequence + 1;
}

void QDECL BotAI_BotInitialChat( bot_state_t *bs, char *type, ... ) {
	int		i, mcontext;
	va_list	ap;
	char	*p;
	char	*vars[MAX_MATCHVARIABLES];

	memset(vars, 0, sizeof(vars));
	va_start(ap, type);
	p = va_arg(ap, char *);
	for (i = 0; i < MAX_MATCHVARIABLES; i++) {
		if( !p ) {
			break;
		}
		vars[i] = p;
		p = va_arg(ap, char *);
	}
	va_end(ap);

	mcontext = BotSynonymContext(bs);

	trap->ai->BotInitialChat( bs->cs, type, mcontext, vars[0], vars[1], vars[2], vars[3], vars[4], vars[5], vars[6], vars[7] );
}

void BotTestAAS(vector3 *origin) {
	int areanum;
	aas_areainfo_t info;

	if (bot_testsolid->integer) {
		if (!trap->aas->AAS_Initialized()) return;
		areanum = BotPointAreaNum(origin);
		if (areanum) BotAI_Print(PRT_MESSAGE, "\remtpy area");
		else BotAI_Print(PRT_MESSAGE, "\r^1SOLID area");
	}
	else if (bot_testclusters->integer) {
		if (!trap->aas->AAS_Initialized()) return;
		areanum = BotPointAreaNum(origin);
		if (!areanum)
			BotAI_Print(PRT_MESSAGE, "\r^1Solid!                              ");
		else {
			trap->aas->AAS_AreaInfo(areanum, &info);
			BotAI_Print(PRT_MESSAGE, "\rarea %d, cluster %d       ", areanum, info.cluster);
		}
	}
}

void BotReportStatus(bot_state_t *bs) {
	char goalname[MAX_MESSAGE_SIZE];
	char netname[MAX_MESSAGE_SIZE];
	char flagstatus[32];
	//
	ClientName(bs->client, netname, sizeof(netname));

	strcpy(flagstatus, "  ");
	if (gametype == GT_FLAGS) {
		if (BotCTFCarryingFlag(bs)) {
			if (BotTeam(bs) == TEAM_RED) strcpy(flagstatus, S_COLOR_RED"F ");
			else strcpy(flagstatus, S_COLOR_BLUE"F ");
		}
	}
	else if (gametype == GT_TROJAN) {
		if (BotTrojanCarryingFlag(bs)) {
			if (BotTeam(bs) == TEAM_RED) strcpy(flagstatus, S_COLOR_RED"F ");
			else strcpy(flagstatus, S_COLOR_BLUE"F ");
		}
	}

	switch(bs->ltgtype) {
		case LTG_TEAMHELP:
		{
			EasyClientName(bs->teammate, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s %s: helping %s\n", netname, flagstatus, goalname);
			break;
		}
		case LTG_TEAMACCOMPANY:
		{
			EasyClientName(bs->teammate, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s %s: accompanying %s\n", netname, flagstatus, goalname);
			break;
		}
		case LTG_DEFENDKEYAREA:
		{
			trap->ai->BotGoalName(bs->teamgoal.number, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s %s: defending %s\n", netname, flagstatus, goalname);
			break;
		}
		case LTG_GETITEM:
		{
			trap->ai->BotGoalName(bs->teamgoal.number, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s %s: getting item %s\n", netname, flagstatus, goalname);
			break;
		}
		case LTG_KILL:
		{
			ClientName(bs->teamgoal.entitynum, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s %s: killing %s\n", netname, flagstatus, goalname);
			break;
		}
		case LTG_CAMP:
		case LTG_CAMPORDER:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s %s: camping\n", netname, flagstatus);
			break;
		}
		case LTG_PATROL:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s %s: patrolling\n", netname, flagstatus);
			break;
		}
		case LTG_GETFLAG:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s %s: capturing flag\n", netname, flagstatus);
			break;
		}
		case LTG_RUSHBASE:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s %s: rushing base\n", netname, flagstatus);
			break;
		}
		case LTG_RETURNFLAG:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s %s: returning flag\n", netname, flagstatus);
			break;
		}
		case LTG_ATTACKENEMYBASE:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s %s: attacking the enemy base\n", netname, flagstatus);
			break;
		}
		default:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s %s: roaming\n", netname, flagstatus);
			break;
		}
	}
}

void BotTeamplayReport( void ) {
	int i;
	char buf[MAX_INFO_STRING];

	BotAI_Print(PRT_MESSAGE, S_COLOR_RED"RED\n");
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		//
		if ( !botstates[i] || !botstates[i]->inuse ) continue;
		//
		trap->SV_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) continue;
		//skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_RED) {
			BotReportStatus(botstates[i]);
		}
	}
	BotAI_Print(PRT_MESSAGE, S_COLOR_BLUE"BLUE\n");
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		//
		if ( !botstates[i] || !botstates[i]->inuse ) continue;
		//
		trap->SV_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) continue;
		//skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_BLUE) {
			BotReportStatus(botstates[i]);
		}
	}
}

void BotSetInfoConfigString(bot_state_t *bs) {
	char goalname[MAX_MESSAGE_SIZE], netname[MAX_MESSAGE_SIZE], action[MAX_MESSAGE_SIZE];
	char carrying[32];
	const char *cs;
	bot_goal_t goal;
	//
	ClientName(bs->client, netname, sizeof(netname));

	strcpy(carrying, "  ");
	if (gametype == GT_FLAGS) {
		if (BotCTFCarryingFlag(bs)) {
			strcpy(carrying, "F ");
		}
	}
	else if (gametype == GT_TROJAN) {
		if (BotTrojanCarryingFlag(bs)) {
			strcpy(carrying, "F ");
		}
	}

	switch(bs->ltgtype) {
		case LTG_TEAMHELP:
		{
			EasyClientName(bs->teammate, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "helping %s", goalname);
			break;
		}
		case LTG_TEAMACCOMPANY:
		{
			EasyClientName(bs->teammate, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "accompanying %s", goalname);
			break;
		}
		case LTG_DEFENDKEYAREA:
		{
			trap->ai->BotGoalName(bs->teamgoal.number, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "defending %s", goalname);
			break;
		}
		case LTG_GETITEM:
		{
			trap->ai->BotGoalName(bs->teamgoal.number, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "getting item %s", goalname);
			break;
		}
		case LTG_KILL:
		{
			ClientName(bs->teamgoal.entitynum, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "killing %s", goalname);
			break;
		}
		case LTG_CAMP:
		case LTG_CAMPORDER:
		{
			Com_sprintf(action, sizeof(action), "camping");
			break;
		}
		case LTG_PATROL:
		{
			Com_sprintf(action, sizeof(action), "patrolling");
			break;
		}
		case LTG_GETFLAG:
		{
			Com_sprintf(action, sizeof(action), "capturing flag");
			break;
		}
		case LTG_RUSHBASE:
		{
			Com_sprintf(action, sizeof(action), "rushing base");
			break;
		}
		case LTG_RETURNFLAG:
		{
			Com_sprintf(action, sizeof(action), "returning flag");
			break;
		}
		case LTG_ATTACKENEMYBASE:
		{
			Com_sprintf(action, sizeof(action), "attacking the enemy base");
			break;
		}
		default:
		{
			trap->ai->BotGetTopGoal(bs->gs, &goal);
			trap->ai->BotGoalName(goal.number, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "roaming %s", goalname);
			break;
		}
	}
  	cs = va( "\\c\\%s\\a\\%s", carrying, action );
  	trap->SV_SetConfigstring (CS_BOTINFO + bs->client, cs);
}

void BotUpdateInfoConfigStrings( void ) {
	int i;
	char buf[MAX_INFO_STRING];

	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		//
		if ( !botstates[i] || !botstates[i]->inuse )
			continue;
		//
		trap->SV_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n")))
			continue;
		BotSetInfoConfigString(botstates[i]);
	}
}

void BotInterbreedBots( void ) {
	float ranks[MAX_CLIENTS];
	int parent1, parent2, child;
	int i;

	// get rankings for all the bots
	for (i = 0; i < MAX_CLIENTS; i++) {
		if ( botstates[i] && botstates[i]->inuse ) {
			ranks[i] = (float)botstates[i]->num_kills * 2 - (float)botstates[i]->num_deaths;
		}
		else {
			ranks[i] = -1;
		}
	}

	if (trap->ai->GeneticParentsAndChildSelection(MAX_CLIENTS, ranks, &parent1, &parent2, &child)) {
		trap->ai->BotInterbreedGoalFuzzyLogic(botstates[parent1]->gs, botstates[parent2]->gs, botstates[child]->gs);
		trap->ai->BotMutateGoalFuzzyLogic(botstates[child]->gs, 1);
	}
	// reset the kills and deaths
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			botstates[i]->num_kills = 0;
			botstates[i]->num_deaths = 0;
		}
	}
}

void BotWriteInterbreeded(char *filename) {
	float rank, bestrank;
	int i, bestbot;

	bestrank = 0;
	bestbot = -1;
	// get the best bot
	for (i = 0; i < MAX_CLIENTS; i++) {
		if ( botstates[i] && botstates[i]->inuse ) {
			rank = (float)botstates[i]->num_kills * 2 - (float)botstates[i]->num_deaths;
		}
		else {
			rank = -1;
		}
		if (rank > bestrank) {
			bestrank = rank;
			bestbot = i;
		}
	}
	if (bestbot >= 0) {
		//write out the new goal fuzzy logic
		trap->ai->BotSaveGoalFuzzyLogic(botstates[bestbot]->gs, filename);
	}
}

// add link back into ExitLevel?
void BotInterbreedEndMatch( void ) {

	if (!bot_interbreed) return;
	bot_interbreedmatchcount++;
	if (bot_interbreedmatchcount >= bot_interbreedcycle->integer) {
		bot_interbreedmatchcount = 0;
		//
		if (strlen(bot_interbreedwrite->string)) {
			BotWriteInterbreeded(bot_interbreedwrite->string);
			trap->Cvar_Set("bot_interbreedwrite", "");
		}
		BotInterbreedBots();
	}
}

void BotInterbreeding( void ) {
	int i;

	if (!strlen(bot_interbreedchar->string)) return;
	//make sure we are in tournament mode
	if (gametype != GT_DUEL) {
		trap->Cvar_Set("sv_gametype", va("%d", GT_DUEL));
		ExitLevel();
		return;
	}
	//shutdown all the bots
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			BotAIShutdownClient(botstates[i]->client, qfalse);
		}
	}
	//make sure all item weight configs are reloaded and Not shared
	trap->BotLibVarSet("bot_reloadcharacters", "1");
	//add a number of bots using the desired bot character
	for (i = 0; i < bot_interbreedbots->integer; i++) {
		trap->Cbuf_ExecuteText( EXEC_INSERT, va("addbot %s %.2f free %i %s%d\n",
						bot_interbreedchar->string, g_difficulty->value, i * 50, bot_interbreedchar->string, i) );
	}
	//
	trap->Cvar_Set("bot_interbreedchar", "");
	bot_interbreed = qtrue;
}

void BotEntityInfo(int entnum, aas_entityinfo_t *info) {
	trap->aas->AAS_EntityInfo(entnum, info);
}

int NumBots( void ) {
	return numbots;
}

float AngleDifference(float ang1, float ang2) {
	float diff;

	diff = ang1 - ang2;
	if (ang1 > ang2) {
		if (diff > 180.0f) diff -= 360.0f;
	}
	else {
		if (diff < -180.0f) diff += 360.0f;
	}
	return diff;
}

float BotChangeViewAngle(float angle, float ideal_angle, float speed) {
	float move;

	angle = AngleMod(angle);
	ideal_angle = AngleMod(ideal_angle);
	if (angle == ideal_angle) return angle;
	move = ideal_angle - angle;
	if (ideal_angle > angle) {
		if (move > 180.0f) move -= 360.0f;
	}
	else {
		if (move < -180.0f) move += 360.0f;
	}
	if (move > 0) {
		if (move > speed) move = speed;
	}
	else {
		if (move < -speed) move = -speed;
	}
	return AngleMod(angle + move);
}

void BotChangeViewAngles(bot_state_t *bs, float thinktime) {
	float diff, factor, maxchange, anglespeed, disired_speed;
	int i;

	if (bs->ideal_viewangles.pitch > 180) bs->ideal_viewangles.pitch -= 360;
	//
	if (bs->enemy >= 0) {
		factor = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_VIEW_FACTOR, 0.01f, 1);
		maxchange = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_VIEW_MAXCHANGE, 1, 1800);
	}
	else {
		factor = 0.05f;
		maxchange = 360;
	}
	if (maxchange < 240)
		maxchange = 240;
	maxchange *= thinktime;
	for (i = 0; i < 2; i++) {
		//
		if (bot_challenge->integer) {
			//smooth slowdown view model
			diff = fabsf(AngleDifference(bs->viewangles.data[i], bs->ideal_viewangles.data[i]));
			anglespeed = diff * factor;
			if (anglespeed > maxchange) anglespeed = maxchange;
			bs->viewangles.data[i] = BotChangeViewAngle(bs->viewangles.data[i],
											bs->ideal_viewangles.data[i], anglespeed);
		}
		else {
			//over reaction view model
			bs->viewangles.data[i] = AngleMod(bs->viewangles.data[i]);
			bs->ideal_viewangles.data[i] = AngleMod(bs->ideal_viewangles.data[i]);
			diff = AngleDifference(bs->viewangles.data[i], bs->ideal_viewangles.data[i]);
			disired_speed = diff * factor;
			bs->viewanglespeed.data[i] += (bs->viewanglespeed.data[i] - disired_speed);
			if (bs->viewanglespeed.data[i] >  180) bs->viewanglespeed.data[i] =  maxchange;
			if (bs->viewanglespeed.data[i] < -180) bs->viewanglespeed.data[i] = -maxchange;
			anglespeed = bs->viewanglespeed.data[i];
			if (anglespeed >  maxchange) anglespeed =  maxchange;
			if (anglespeed < -maxchange) anglespeed = -maxchange;
			bs->viewangles.data[i] += anglespeed;
			bs->viewangles.data[i] = AngleMod(bs->viewangles.data[i]);
			//demping
			bs->viewanglespeed.data[i] *= 0.45f * (1 - factor);
		}
		//BotAI_Print(PRT_MESSAGE, "ideal_angles %f %f\n", bs->ideal_viewangles[0], bs->ideal_viewangles[1], bs->ideal_viewangles[2]);`
		//bs->viewangles[i] = bs->ideal_viewangles[i];
	}
	//bs->viewangles[PITCH] = 0;
	if (bs->viewangles.pitch > 180)
		bs->viewangles.pitch -= 360;
	//elementary action: view
	trap->ea->EA_View(bs->client, &bs->viewangles);
}

void BotInputToUserCommand(bot_input_t *bi, usercmd_t *ucmd, ivector3 *delta_angles, int time) {
	vector3 angles, forward, right;
	short temp;
	int j;
	float f, r, u, m;

	//clear the whole structure
	memset(ucmd, 0, sizeof(usercmd_t));
	//the duration for the user command in milli seconds
	ucmd->serverTime = time;
	//
	if (bi->actionflags & ACTION_DELAYEDJUMP) {
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}
	//set the buttons
	if (bi->actionflags & ACTION_RESPAWN)		ucmd->buttons = BUTTON_ATTACK;
	if (bi->actionflags & ACTION_ATTACK)		ucmd->buttons |= BUTTON_ATTACK;
	if (bi->actionflags & ACTION_TALK)			ucmd->buttons |= BUTTON_TALK;
	if (bi->actionflags & ACTION_GESTURE)		ucmd->buttons |= BUTTON_GESTURE;
	if (bi->actionflags & ACTION_USE)			ucmd->buttons |= BUTTON_USE_HOLDABLE;
	if (bi->actionflags & ACTION_WALK)			ucmd->buttons |= BUTTON_WALKING;
#if 0
	if (bi->actionflags & ACTION_AFFIRMATIVE)	ucmd->buttons |= BUTTON_AFFIRMATIVE;
	if (bi->actionflags & ACTION_NEGATIVE)		ucmd->buttons |= BUTTON_NEGATIVE;
	if (bi->actionflags & ACTION_GETFLAG)		ucmd->buttons |= BUTTON_GETFLAG;
	if (bi->actionflags & ACTION_GUARDBASE)		ucmd->buttons |= BUTTON_GUARDBASE;
	if (bi->actionflags & ACTION_PATROL)		ucmd->buttons |= BUTTON_PATROL;
	if (bi->actionflags & ACTION_FOLLOWME)		ucmd->buttons |= BUTTON_FOLLOWME;
#endif
	//
	ucmd->weapon = (byte)bi->weapon;
	//set the view angles
	//NOTE: the ucmd->angles are the angles WITHOUT the delta angles
	ucmd->angles[0] = ANGLE2SHORT(bi->viewangles.pitch);
	ucmd->angles[1] = ANGLE2SHORT(bi->viewangles.yaw);
	ucmd->angles[2] = ANGLE2SHORT(bi->viewangles.roll);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		temp = (short)((int)ucmd->angles[j] - delta_angles->data[j]);
		/*NOTE: disabled because temp should be mod first
		if ( j == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) temp = 16000;
			else if ( temp < -16000 ) temp = -16000;
		}
		*/
		ucmd->angles[j] = temp;
	}
	//NOTE: movement is relative to the REAL view angles
	//get the horizontal forward and right vector
	//get the pitch in the range [-180, 180]
	if (bi->dir.z) angles.pitch = bi->viewangles.pitch;
	else angles.pitch = 0;
	angles.yaw = bi->viewangles.yaw;
	angles.roll = 0;
	AngleVectors(&angles, &forward, &right, NULL);
	//bot input speed is in the range [0, 400]
	bi->speed = bi->speed * 127 / 400;
	//set the view independent movement
	f = DotProduct(&forward, &bi->dir);
	r = DotProduct(&right, &bi->dir);
	u = fabsf(forward.z) * bi->dir.z;
	m = fabsf(f);

	if (fabsf(r) > m) {
		m = fabsf(r);
	}

	if (fabsf(u) > m) {
		m = fabsf(u);
	}

	if (m > 0) {
		f *= bi->speed / m;
		r *= bi->speed / m;
		u *= bi->speed / m;
	}

	ucmd->forwardmove = (signed char)f;
	ucmd->rightmove = (signed char)r;
	ucmd->upmove = (signed char)u;

	if ( bi->actionflags & ACTION_MOVEFORWARD )	ucmd->forwardmove = 127;
	if ( bi->actionflags & ACTION_MOVEBACK )	ucmd->forwardmove = -127;
	if ( bi->actionflags & ACTION_MOVERIGHT )	ucmd->rightmove = 127;
	if ( bi->actionflags & ACTION_MOVELEFT )	ucmd->rightmove = -127;
	if ( bi->actionflags & ACTION_JUMP )		ucmd->upmove = 127;
	if ( bi->actionflags & ACTION_CROUCH )		ucmd->upmove = -127;
}

void BotUpdateInput(bot_state_t *bs, int time, int elapsed_time) {
	bot_input_t bi;
	int j;

	//add the delta angles to the bot's current view angles
	for (j = 0; j < 3; j++) {
		bs->viewangles.data[j] = AngleMod(bs->viewangles.data[j] + SHORT2ANGLE(bs->cur_ps.delta_angles.data[j]));
	}
	//change the bot view angles
	BotChangeViewAngles(bs, (float) elapsed_time / 1000);
	//retrieve the bot input
	trap->ea->EA_GetInput(bs->client, (float) time / 1000, &bi);
	//respawn hack
	if (bi.actionflags & ACTION_RESPAWN) {
		if (bs->lastucmd.buttons & BUTTON_ATTACK) bi.actionflags &= ~(ACTION_RESPAWN|ACTION_ATTACK);
	}
	//convert the bot input to a usercmd
	BotInputToUserCommand(&bi, &bs->lastucmd, &bs->cur_ps.delta_angles, time);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		bs->viewangles.data[j] = AngleMod(bs->viewangles.data[j] - SHORT2ANGLE(bs->cur_ps.delta_angles.data[j]));
	}
}

void BotAIRegularUpdate( void ) {
	if (regularupdate_time < FloatTime()) {
		trap->ai->BotUpdateEntityItems();
		regularupdate_time = FloatTime() + 0.3f;
	}
}

void RemoveColorEscapeSequences( char *text ) {
	int i, l;

	l = 0;
	for ( i = 0; text[i]; i++ ) {
		if (Q_IsColorString(&text[i])) {
			i++;
			continue;
		}
		if (text[i] > 0x7E)
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

int BotAI(int client, float thinktime) {
	bot_state_t *bs;
	char buf[1024], *args;
	int j;

	trap->ea->EA_ResetInput(client);
	//
	bs = botstates[client];
	if (!bs || !bs->inuse) {
		BotAI_Print(PRT_FATAL, "BotAI: client %d is not setup\n", client);
		return qfalse;
	}

	//retrieve the current client state
	BotAI_GetClientState( client, &bs->cur_ps );

	//retrieve any waiting server commands
	while( trap->SV_BotGetConsoleMessage(client, buf, sizeof(buf)) ) {
		//have buf point to the command and args to the command arguments
		args = strchr( buf, ' ');
		if (!args) continue;
		*args++ = '\0';

		//remove color espace sequences from the arguments
		RemoveColorEscapeSequences( args );

		if (!Q_stricmp(buf, "cp "))
			{ /*CenterPrintf*/ }
		else if (!Q_stricmp(buf, "cs"))
			{ /*ConfigStringModified*/ }
		else if (!Q_stricmp(buf, "print")) {
			//remove first and last quote from the chat message
			memmove(args, args+1, strlen(args));
			args[strlen(args)-1] = '\0';
			trap->ai->BotQueueConsoleMessage(bs->cs, CMS_NORMAL, args);
		}
		else if (!Q_stricmp(buf, "chat")) {
			//remove first and last quote from the chat message
			memmove(args, args+1, strlen(args));
			args[strlen(args)-1] = '\0';
			trap->ai->BotQueueConsoleMessage(bs->cs, CMS_CHAT, args);
		}
		else if (!Q_stricmp(buf, "tchat")) {
			//remove first and last quote from the chat message
			memmove(args, args+1, strlen(args));
			args[strlen(args)-1] = '\0';
			trap->ai->BotQueueConsoleMessage(bs->cs, CMS_CHAT, args);
		}
		else if (!Q_stricmp(buf, "vchat")) {
			BotVoiceChatCommand(bs, SAY_ALL, args);
		}
		else if (!Q_stricmp(buf, "vtchat")) {
			BotVoiceChatCommand(bs, SAY_TEAM, args);
		}
		else if (!Q_stricmp(buf, "vtell")) {
			BotVoiceChatCommand(bs, SAY_TELL, args);
		}
		else if (!Q_stricmp(buf, "scores"))
			{ /*FIXME: parse scores?*/ }
	}
	//add the delta angles to the bot's current view angles
	for (j = 0; j < 3; j++) {
		bs->viewangles.data[j] = AngleMod(bs->viewangles.data[j] + SHORT2ANGLE(bs->cur_ps.delta_angles.data[j]));
	}
	//increase the local time of the bot
	bs->ltime += thinktime;
	//
	bs->thinktime = thinktime;
	//origin of the bot
	VectorCopy(&bs->cur_ps.origin, &bs->origin);
	//eye coordinates of the bot
	VectorCopy(&bs->cur_ps.origin, &bs->eye);
	bs->eye.z += bs->cur_ps.viewheight;
	//get the area the bot is in
	bs->areanum = BotPointAreaNum(&bs->origin);
	//the real AI
	BotDeathmatchAI(bs, thinktime);
	//set the weapon selection every AI frame
	trap->ea->EA_SelectWeapon(bs->client, bs->weaponnum);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		bs->viewangles.data[j] = AngleMod(bs->viewangles.data[j] - SHORT2ANGLE(bs->cur_ps.delta_angles.data[j]));
	}
	//everything was ok
	return qtrue;
}

void BotScheduleBotThink( void ) {
	int i, botnum;

	botnum = 0;

	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		//initialize the bot think residual time
		botstates[i]->botthink_residual = bot_thinktime->integer * botnum / numbots;
		botnum++;
	}
}

void BotWriteSessionData(bot_state_t *bs) {
	const char	*s;
	const char	*var;

	s = va(
			"%i %i %i %i %i %i %i %i"
			" %f %f %f"
			" %f %f %f"
			" %f %f %f",
		bs->lastgoal_decisionmaker,
		bs->lastgoal_ltgtype,
		bs->lastgoal_teammate,
		bs->lastgoal_teamgoal.areanum,
		bs->lastgoal_teamgoal.entitynum,
		bs->lastgoal_teamgoal.flags,
		bs->lastgoal_teamgoal.iteminfo,
		bs->lastgoal_teamgoal.number,
		bs->lastgoal_teamgoal.origin.x,
		bs->lastgoal_teamgoal.origin.y,
		bs->lastgoal_teamgoal.origin.z,
		bs->lastgoal_teamgoal.mins.x,
		bs->lastgoal_teamgoal.mins.y,
		bs->lastgoal_teamgoal.mins.z,
		bs->lastgoal_teamgoal.maxs.x,
		bs->lastgoal_teamgoal.maxs.y,
		bs->lastgoal_teamgoal.maxs.z
		);

	var = va( "botsession%i", bs->client );

	trap->Cvar_Set( var, s );
}

void BotReadSessionData(bot_state_t *bs) {
	char	s[MAX_CVAR_VALUE_STRING];
	const char	*var;

	var = va( "botsession%i", bs->client );
	trap->Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf(s,
			"%i %i %i %i %i %i %i %i"
			" %f %f %f"
			" %f %f %f"
			" %f %f %f",
		&bs->lastgoal_decisionmaker,
		&bs->lastgoal_ltgtype,
		&bs->lastgoal_teammate,
		&bs->lastgoal_teamgoal.areanum,
		&bs->lastgoal_teamgoal.entitynum,
		&bs->lastgoal_teamgoal.flags,
		&bs->lastgoal_teamgoal.iteminfo,
		&bs->lastgoal_teamgoal.number,
		&bs->lastgoal_teamgoal.origin.x,
		&bs->lastgoal_teamgoal.origin.y,
		&bs->lastgoal_teamgoal.origin.z,
		&bs->lastgoal_teamgoal.mins.x,
		&bs->lastgoal_teamgoal.mins.y,
		&bs->lastgoal_teamgoal.mins.z,
		&bs->lastgoal_teamgoal.maxs.x,
		&bs->lastgoal_teamgoal.maxs.y,
		&bs->lastgoal_teamgoal.maxs.z
		);
}

int BotAISetupClient(int client, struct bot_settings_s *settings, qboolean restart) {
	char filename[MAX_PATH], name[MAX_PATH];
	bot_state_t *bs;
	int errnum;

	if (!botstates[client]) botstates[client] = G_Alloc(sizeof(bot_state_t));
	bs = botstates[client];

	if (bs && bs->inuse) {
		BotAI_Print(PRT_FATAL, "BotAISetupClient: client %d already setup\n", client);
		return qfalse;
	}

	if (!trap->aas->AAS_Initialized()) {
		BotAI_Print(PRT_FATAL, "AAS not initialized\n");
		return qfalse;
	}

	//load the bot character
	bs->character = trap->ai->BotLoadCharacter(settings->characterfile, settings->skill);
	if (!bs->character) {
		BotAI_Print(PRT_FATAL, "couldn't load skill %f from %s\n", settings->skill, settings->characterfile);
		return qfalse;
	}
	//copy the settings
	memcpy(&bs->settings, settings, sizeof(bot_settings_t));
	//allocate a goal state
	bs->gs = trap->ai->BotAllocGoalState(client);
	//load the item weights
	trap->ai->Characteristic_String(bs->character, CHARACTERISTIC_ITEMWEIGHTS, filename, MAX_PATH);
	errnum = trap->ai->BotLoadItemWeights(bs->gs, filename);
	if (errnum != BLERR_NOERROR) {
		trap->ai->BotFreeGoalState(bs->gs);
		return qfalse;
	}
	//allocate a weapon state
	bs->ws = trap->ai->BotAllocWeaponState();
	//load the weapon weights
	trap->ai->Characteristic_String(bs->character, CHARACTERISTIC_WEAPONWEIGHTS, filename, MAX_PATH);
	errnum = trap->ai->BotLoadWeaponWeights(bs->ws, filename);
	if (errnum != BLERR_NOERROR) {
		trap->ai->BotFreeGoalState(bs->gs);
		trap->ai->BotFreeWeaponState(bs->ws);
		return qfalse;
	}
	//allocate a chat state
	bs->cs = trap->ai->BotAllocChatState();
	//load the chat file
	trap->ai->Characteristic_String(bs->character, CHARACTERISTIC_CHAT_FILE, filename, MAX_PATH);
	trap->ai->Characteristic_String(bs->character, CHARACTERISTIC_CHAT_NAME, name, MAX_PATH);
	errnum = trap->ai->BotLoadChatFile(bs->cs, filename, name);
	if (errnum != BLERR_NOERROR) {
		trap->ai->BotFreeChatState(bs->cs);
		trap->ai->BotFreeGoalState(bs->gs);
		trap->ai->BotFreeWeaponState(bs->ws);
		return qfalse;
	}

	bs->inuse = qtrue;
	bs->client = client;
	bs->entitynum = client;
	bs->setupcount = 4;
	bs->entergame_time = FloatTime();
	bs->ms = trap->ai->BotAllocMoveState();
	bs->walker = trap->ai->Characteristic_BFloat(bs->character, CHARACTERISTIC_WALKER, 0, 1);
	numbots++;

	if (trap->Cvar_VariableIntegerValue("bot_testichat")) {
		trap->BotLibVarSet("bot_testichat", "1");
		BotChatTest(bs);
	}
	//NOTE: reschedule the bot thinking
	BotScheduleBotThink();
	//if interbreeding start with a mutation
	if (bot_interbreed) {
		trap->ai->BotMutateGoalFuzzyLogic(bs->gs, 1);
	}
	// if we kept the bot client
	if (restart) {
		BotReadSessionData(bs);
	}
	//bot has been setup succesfully
	return qtrue;
}

int BotAIShutdownClient(int client, qboolean restart) {
	bot_state_t *bs;

	bs = botstates[client];
	if (!bs || !bs->inuse) {
		//BotAI_Print(PRT_ERROR, "BotAIShutdownClient: client %d already shutdown\n", client);
		return qfalse;
	}

	if (restart) {
		BotWriteSessionData(bs);
	}

	if (BotChat_ExitGame(bs)) {
		trap->ai->BotEnterChat(bs->cs, bs->client, CHAT_ALL);
	}

	trap->ai->BotFreeMoveState(bs->ms);
	//free the goal state`			
	trap->ai->BotFreeGoalState(bs->gs);
	//free the chat file
	trap->ai->BotFreeChatState(bs->cs);
	//free the weapon weights
	trap->ai->BotFreeWeaponState(bs->ws);
	//free the bot character
	trap->ai->BotFreeCharacter(bs->character);
	//
	BotFreeWaypoints(bs->checkpoints);
	BotFreeWaypoints(bs->patrolpoints);
	//clear activate goal stack
	BotClearActivateGoalStack(bs);
	//clear the bot state
	memset(bs, 0, sizeof(bot_state_t));
	//set the inuse flag to qfalse
	bs->inuse = qfalse;
	//there's one bot less
	numbots--;
	//everything went ok
	return qtrue;
}

// called when a bot enters the intermission or observer mode and when the level is changed
void BotResetState(bot_state_t *bs) {
	int client, entitynum, inuse;
	int movestate, goalstate, chatstate, weaponstate;
	bot_settings_t settings;
	int character;
	playerState_t ps;							//current player state
	float entergame_time;

	//save some things that should not be reset here
	memcpy(&settings, &bs->settings, sizeof(bot_settings_t));
	memcpy(&ps, &bs->cur_ps, sizeof(playerState_t));
	inuse = bs->inuse;
	client = bs->client;
	entitynum = bs->entitynum;
	character = bs->character;
	movestate = bs->ms;
	goalstate = bs->gs;
	chatstate = bs->cs;
	weaponstate = bs->ws;
	entergame_time = bs->entergame_time;
	//free checkpoints and patrol points
	BotFreeWaypoints(bs->checkpoints);
	BotFreeWaypoints(bs->patrolpoints);
	//reset the whole state
	memset(bs, 0, sizeof(bot_state_t));
	//copy back some state stuff that should not be reset
	bs->ms = movestate;
	bs->gs = goalstate;
	bs->cs = chatstate;
	bs->ws = weaponstate;
	memcpy(&bs->cur_ps, &ps, sizeof(playerState_t));
	memcpy(&bs->settings, &settings, sizeof(bot_settings_t));
	bs->inuse = inuse;
	bs->client = client;
	bs->entitynum = entitynum;
	bs->character = character;
	bs->entergame_time = entergame_time;
	//reset several states
	if (bs->ms) trap->ai->BotResetMoveState(bs->ms);
	if (bs->gs) trap->ai->BotResetGoalState(bs->gs);
	if (bs->ws) trap->ai->BotResetWeaponState(bs->ws);
	if (bs->gs) trap->ai->BotResetAvoidGoals(bs->gs);
	if (bs->ms) trap->ai->BotResetAvoidReach(bs->ms);
}

int BotAILoadMap( int restart ) {
	int			i;
	cvar_t *mapname;

	if (!restart) {
		mapname = trap->Cvar_Get( "mapname", "", CVAR_SERVERINFO|CVAR_ROM, NULL, NULL );
		trap->BotLibLoadMap( mapname->string );
	}

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			BotResetState( botstates[i] );
			botstates[i]->setupcount = 4;
		}
	}

	BotSetupDeathmatchAI();

	return qtrue;
}

int BotAIStartFrame(int time) {
	int i;
	gentity_t	*ent;
	bot_entitystate_t state;
	int elapsed_time, thinktime;
	static int local_time;
	static int botlib_residual;
	static int lastbotthink_time;

	G_CheckBotSpawn();

	if (bot_report->integer) {
//		BotTeamplayReport();
//		trap->Cvar_Set("bot_report", "0");
		BotUpdateInfoConfigStrings();
	}

	if (bot_pause->integer) {
		// execute bot user commands every frame
		for( i = 0; i < MAX_CLIENTS; i++ ) {
			if( !botstates[i] || !botstates[i]->inuse ) {
				continue;
			}
			if( g_entities[i].client->pers.connected != CON_CONNECTED ) {
				continue;
			}
			botstates[i]->lastucmd.forwardmove = 0;
			botstates[i]->lastucmd.rightmove = 0;
			botstates[i]->lastucmd.upmove = 0;
			botstates[i]->lastucmd.buttons = 0;
			botstates[i]->lastucmd.serverTime = time;
			trap->SV_ClientThink( botstates[i]->client, &botstates[i]->lastucmd );
		}
		return qtrue;
	}

	if (bot_memorydump->integer) {
		trap->BotLibVarSet("memorydump", "1");
		trap->Cvar_Set("bot_memorydump", "0");
	}
	if (bot_saveroutingcache->integer) {
		trap->BotLibVarSet("saveroutingcache", "1");
		trap->Cvar_Set("bot_saveroutingcache", "0");
	}
	//check if bot interbreeding is activated
	BotInterbreeding();
	//cap the bot think time
	if (bot_thinktime->integer > 200) {
		trap->Cvar_Set("bot_thinktime", "200");
	}
	//if the bot think time changed we should reschedule the bots
	if (bot_thinktime->integer != lastbotthink_time) {
		lastbotthink_time = bot_thinktime->integer;
		BotScheduleBotThink();
	}

	elapsed_time = time - local_time;
	local_time = time;

	botlib_residual += elapsed_time;

	if (elapsed_time > bot_thinktime->integer) thinktime = elapsed_time;
	else thinktime = bot_thinktime->integer;

	// update the bot library
	if ( botlib_residual >= thinktime ) {
		botlib_residual -= thinktime;

		trap->BotLibStartFrame((float) time / 1000);

		if (!trap->aas->AAS_Initialized()) return qfalse;

		//update entities in the botlib
		for (i = 0; i < MAX_GENTITIES; i++) {
			ent = &g_entities[i];
			if (!ent->inuse) {
				trap->BotLibUpdateEntity(i, NULL);
				continue;
			}
			if (!ent->r.linked) {
				trap->BotLibUpdateEntity(i, NULL);
				continue;
			}
			if (ent->r.svFlags & SVF_NOCLIENT) {
				trap->BotLibUpdateEntity(i, NULL);
				continue;
			}
			// do not update missiles
			if (ent->s.eType == ET_MISSILE) {
				trap->BotLibUpdateEntity(i, NULL);
				continue;
			}
			// do not update event only entities
			if (ent->s.eType > ET_EVENTS) {
				trap->BotLibUpdateEntity(i, NULL);
				continue;
			}
			//
			memset(&state, 0, sizeof(bot_entitystate_t));
			//
			VectorCopy(&ent->r.currentOrigin, &state.origin);
			if (i < MAX_CLIENTS) {
				VectorCopy(&ent->s.apos.trBase, &state.angles);
			} else {
				VectorCopy(&ent->r.currentAngles, &state.angles);
			}
			VectorCopy(&ent->s.origin2, &state.old_origin);
			VectorCopy(&ent->r.mins, &state.mins);
			VectorCopy(&ent->r.maxs, &state.maxs);
			state.type = ent->s.eType;
			state.flags = ent->s.eFlags;
			if (ent->r.bmodel) state.solid = SOLID_BSP;
			else state.solid = SOLID_BBOX;
			state.groundent = ent->s.groundEntityNum;
			state.modelindex = ent->s.modelindex;
			state.modelindex2 = ent->s.modelindex2;
			state.frame = ent->s.frame;
			state.event = ent->s.event;
			state.eventParm = ent->s.eventParm;
			state.powerups = ent->s.powerups;
			state.legsAnim = ent->s.legsAnim;
			state.torsoAnim = ent->s.torsoAnim;
			state.weapon = ent->s.weapon;
			//
			trap->BotLibUpdateEntity(i, &state);
		}

		BotAIRegularUpdate();
	}

	floattime = trap->aas->AAS_Time();

	// execute scheduled bot AI
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		//
		botstates[i]->botthink_residual += elapsed_time;
		//
		if ( botstates[i]->botthink_residual >= thinktime ) {
			botstates[i]->botthink_residual -= thinktime;

			if (!trap->aas->AAS_Initialized()) return qfalse;

			if (g_entities[i].client->pers.connected == CON_CONNECTED) {
				BotAI(i, (float) thinktime / 1000);
			}
		}
	}


	// execute bot user commands every frame
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		if( g_entities[i].client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		BotUpdateInput(botstates[i], time, elapsed_time);
		trap->SV_ClientThink( botstates[i]->client, &botstates[i]->lastucmd );
	}

	return qtrue;
}

int BotInitLibrary( void ) {
	char buf[MAX_CVAR_VALUE_STRING];

	//set the maxclients and maxentities library variables before calling BotSetupLibrary
	trap->Cvar_VariableStringBuffer("sv_maxclients", buf, sizeof(buf));
	if (!strlen(buf)) strcpy(buf, "8");
	trap->BotLibVarSet("maxclients", buf);
	Com_sprintf(buf, sizeof(buf), "%d", MAX_GENTITIES);
	trap->BotLibVarSet("maxentities", buf);
	//bsp checksum
	trap->Cvar_VariableStringBuffer("sv_mapChecksum", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("sv_mapChecksum", buf);
	//maximum number of aas links
	trap->Cvar_VariableStringBuffer("max_aaslinks", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("max_aaslinks", buf);
	//maximum number of items in a level
	trap->Cvar_VariableStringBuffer("max_levelitems", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("max_levelitems", buf);
	//game type
	trap->Cvar_VariableStringBuffer("sv_gametype", buf, sizeof(buf));
	if (!strlen(buf)) strcpy(buf, "0");
	trap->BotLibVarSet("sv_gametype", buf);
	//bot developer mode and log file
	trap->BotLibVarSet("bot_developer", bot_developer->string);
	trap->Cvar_VariableStringBuffer("logfile", buf, sizeof(buf));
	trap->BotLibVarSet("log", buf);
	//no chatting
	trap->Cvar_VariableStringBuffer("bot_nochat", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("nochat", buf);
	//visualize jump pads
	trap->Cvar_VariableStringBuffer("bot_visualizejumppads", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("bot_visualizejumppads", buf);
	//forced clustering calculations
	trap->Cvar_VariableStringBuffer("bot_forceclustering", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("forceclustering", buf);
	//forced reachability calculations
	trap->Cvar_VariableStringBuffer("bot_forcereachability", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("forcereachability", buf);
	//force writing of AAS to file
	trap->Cvar_VariableStringBuffer("bot_forcewrite", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("forcewrite", buf);
	//no AAS optimization
	trap->Cvar_VariableStringBuffer("bot_aasoptimize", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("aasoptimize", buf);
	//
	trap->Cvar_VariableStringBuffer("bot_saveroutingcache", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("saveroutingcache", buf);
	//reload instead of cache bot character files
	trap->Cvar_VariableStringBuffer("bot_reloadcharacters", buf, sizeof(buf));
	if (!strlen(buf)) strcpy(buf, "0");
	trap->BotLibVarSet("bot_reloadcharacters", buf);
	//base directory
	trap->Cvar_VariableStringBuffer("fs_basepath", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("basedir", buf);
	//game directory
	trap->Cvar_VariableStringBuffer("fs_game", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("gamedir", buf);
	//home directory
	trap->Cvar_VariableStringBuffer("fs_homepath", buf, sizeof(buf));
	if (strlen(buf)) trap->BotLibVarSet("homedir", buf);
	//
	//RAZTODO: See if this causes issues
//	trap->BotLibDefine("MISSIONPACK");
	//setup the bot library
	return trap->SV_BotLibSetup();
}

int BotAISetup( int restart ) {
	int			errnum;

	bot_thinktime			= trap->Cvar_Get( "bot_thinktime", "100", CVAR_CHEAT, NULL, NULL );
	bot_memorydump			= trap->Cvar_Get( "bot_memorydump", "0", CVAR_CHEAT, NULL, NULL );
	bot_saveroutingcache	= trap->Cvar_Get( "bot_saveroutingcache", "0", CVAR_CHEAT, NULL, NULL );
	bot_pause				= trap->Cvar_Get( "bot_pause", "0", CVAR_CHEAT, NULL, NULL );
	bot_report				= trap->Cvar_Get( "bot_report", "0", CVAR_CHEAT, NULL, NULL );
	bot_testsolid			= trap->Cvar_Get( "bot_testsolid", "0", CVAR_CHEAT, NULL, NULL );
	bot_testclusters		= trap->Cvar_Get( "bot_testclusters", "0", CVAR_CHEAT, NULL, NULL );
	bot_developer			= trap->Cvar_Get( "bot_developer", "0", CVAR_CHEAT, NULL, NULL );
	bot_interbreedchar		= trap->Cvar_Get( "bot_interbreedchar", "", 0, NULL, NULL );
	bot_interbreedbots		= trap->Cvar_Get( "bot_interbreedbots", "10", 0, NULL, NULL );
	bot_interbreedcycle		= trap->Cvar_Get( "bot_interbreedcycle", "20", 0, NULL, NULL );
	bot_interbreedwrite		= trap->Cvar_Get( "bot_interbreedwrite", "", 0, NULL, NULL );

	//if the game is restarted for a tournament
	if (restart) {
		return qtrue;
	}

	//initialize the bot states
	memset( botstates, 0, sizeof(botstates) );

	errnum = BotInitLibrary();
	if (errnum != BLERR_NOERROR) return qfalse;
	return qtrue;
}

int BotAIShutdown( int restart ) {

	int i;

	//if the game is restarted for a tournament
	if ( restart ) {
		//shutdown all the bots in the botlib
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (botstates[i] && botstates[i]->inuse) {
				BotAIShutdownClient(botstates[i]->client, restart);
			}
		}
		//don't shutdown the bot library
	}
	else {
		trap->SV_BotLibShutdown();
	}
	return qtrue;
}

