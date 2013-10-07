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
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "../ui/ui_shared.h"
extern menuDef_t *menuScoreboard;



void CG_TargetCommand_f( void ) {
	int		targetNum;

	targetNum = CG_CrosshairPlayer();
	if ( !targetNum )
		return;

	trap->SendConsoleCommand( va( "gc %i %i", targetNum, atoi( trap->Cmd_Argv( 1 ) ) ) );
}

// Debugging command to print the current position
static void CG_Viewpos_f( void ) {
	trap->Print ("(%i %i %i) : %i\n", (int)cg.refdef.vieworg.x, (int)cg.refdef.vieworg.y, (int)cg.refdef.vieworg.z, (int)cg.refdefViewAngles.yaw);
}


static void CG_ScoresDown_f( void ) {

	CG_BuildSpectatorString();
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap->SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
			cg.numScores = 0;
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

static void CG_ScoresUp_f( void ) {
	if ( cg.showScores ) {
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

extern menuDef_t *menuScoreboard;
void Menu_Reset( void );			// FIXME: add to right include file

static void CG_LoadHud_f( void) {
	const char *hudSet;

	String_Init();
	Menu_Reset();
	
	hudSet = cg_hudFiles->string;
	if ( hudSet[0] == '\0' )
		hudSet = "ui/hud.txt";

	CG_LoadMenus( hudSet );
	menuScoreboard = NULL;
}


static void CG_scrollScoresDown_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qtrue);
	}
}


static void CG_scrollScoresUp_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qfalse);
	}
}

static void CG_TellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap->Cmd_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap->SendClientCommand( command );
}

static void CG_TellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap->Cmd_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap->SendClientCommand( command );
}

static void CG_VoiceTellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap->Cmd_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap->SendClientCommand( command );
}

static void CG_VoiceTellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap->Cmd_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap->SendClientCommand( command );
}

static void CG_TauntKillInsult_f (void ) {
	trap->SendConsoleCommand("cmd vsay kill_insult\n");
}

static void CG_TauntPraise_f (void ) {
	trap->SendConsoleCommand("cmd vsay praise\n");
}

static void CG_TauntTaunt_f (void ) {
	trap->SendConsoleCommand("cmd vtaunt\n");
}

static void CG_TauntDeathInsult_f (void ) {
	trap->SendConsoleCommand("cmd vsay death_insult\n");
}

static void CG_TaskSuicide_f (void ) {
	int		clientNum;
	char	command[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	Com_sprintf( command, 128, "tell %i suicide", clientNum );
	trap->SendClientCommand( command );
}

typedef struct consoleCommand_s {
	const char *name;
	void (*function)( void );
} consoleCommand_t;

static consoleCommand_t	commands[] = {
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "+zoom", CG_ZoomDown_f },
	{ "-zoom", CG_ZoomUp_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "tcmd", CG_TargetCommand_f },
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },

	{ "vtell_target", CG_VoiceTellTarget_f },
	{ "vtell_attacker", CG_VoiceTellAttacker_f },
	{ "loadhud", CG_LoadHud_f },
	{ "tauntKillInsult", CG_TauntKillInsult_f },
	{ "tauntPraise", CG_TauntPraise_f },
	{ "tauntTaunt", CG_TauntTaunt_f },
	{ "tauntDeathInsult", CG_TauntDeathInsult_f },
	{ "scoresDown", CG_scrollScoresDown_f },
	{ "scoresUp", CG_scrollScoresUp_f },
	{ "loaddeferred", CG_LoadDeferredPlayers },
	{ "messageModeAll", CG_MessageModeAll },
	{ "messageModeTeam", CG_MessageModeTeam },

	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	{ "kill", NULL },
	{ "say", NULL },
	{ "say_team", NULL },
	{ "tell", NULL },
	{ "vsay", NULL },
	{ "vsay_team", NULL },
	{ "vtell", NULL },
	{ "vtaunt", NULL },
	{ "vosay", NULL },
	{ "vosay_team", NULL },
	{ "votell", NULL },
	{ "give", NULL },
	{ "god", NULL },
	{ "notarget", NULL },
	{ "noclip", NULL },
	{ "where", NULL },
	{ "team", NULL },
	{ "follow", NULL },
	{ "follownext", NULL },
	{ "followprev", NULL },
	{ "addbot", NULL },
	{ "setviewpos", NULL },
	{ "callvote", NULL },
	{ "vote", NULL },
	{ "callteamvote", NULL },
	{ "teamvote", NULL },
	{ "stats", NULL },
	{ "loaddefered", NULL }, // spelled wrong, but not changing for demo
};

// Let the client system know about all of our commands so it can perform tab completion
void CG_InitConsoleCommands( void ) {
	int i;

	for ( i=0; i<ARRAY_LEN( commands ); i++ )
		trap->AddCommand( commands[i].name, commands[i].function, NULL );
}

void CG_ShutdownConsoleCommands( void ) {
	int i;

	for ( i=0; i<ARRAY_LEN( commands ); i++ )
		trap->RemoveCommand( commands[i].name );
}
