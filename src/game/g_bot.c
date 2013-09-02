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
// g_bot.c

#include "g_local.h"


static int		g_numBots;
static char		*g_botInfos[MAX_BOTS];


int				g_numArenas;
static char		*g_arenaInfos[MAX_ARENAS];


#define BOT_BEGIN_DELAY_BASE		2000
#define BOT_BEGIN_DELAY_INCREMENT	1500

#define BOT_SPAWN_QUEUE_DEPTH	16

typedef struct {
	int		clientNum;
	int		spawnTime;
} botSpawnQueue_t;

static botSpawnQueue_t	botSpawnQueue[BOT_SPAWN_QUEUE_DEPTH];

cvar_t *bot_minplayers;

extern gentity_t	*podium1;
extern gentity_t	*podium2;
extern gentity_t	*podium3;

float G_Cvar_VariableValue( const char *name ) {
	char buf[MAX_CVAR_VALUE_STRING];
	trap->Cvar_VariableStringBuffer( name, buf, sizeof( buf ) );
	return (float)atof( buf );
}



/*
===============
G_ParseInfos
===============
*/
int G_ParseInfos( char *buf, int max, char *infos[] ) {
	char	*token;
	int		count;
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
		infos[count] = G_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
		if (infos[count]) {
			strcpy(infos[count], info);
			count++;
		}
	}
	return count;
}

/*
===============
G_LoadArenasFromFile
===============
*/
static void G_LoadArenasFromFile( char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_ARENAS_TEXT];

	len = trap->FS_Open( filename, &f, FS_READ );
	if ( !f ) {
		trap->Print( S_COLOR_RED "file not found: %s\n", filename );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap->FS_Close( f );
		trap->Print( S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_ARENAS_TEXT );
		return;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );

	g_numArenas += G_ParseInfos( buf, MAX_ARENAS - g_numArenas, &g_arenaInfos[g_numArenas] );
}

int G_GetMapTypeBits( char *type ) {
	if ( *type ) {
		int typeBits = 0;
		char tmp[MAX_TOKEN_CHARS] = {0}, *p = NULL, *delim = " ";

		Q_strncpyz( tmp, type, sizeof( tmp ) );
		p = strtok( tmp, delim );

		while ( p != NULL ) {
			int gt = BG_GetGametypeForString( p );
			if ( gt != -1 )
				typeBits |= (1<<gt);
			p = strtok( NULL, delim );
		}
		return typeBits;
	}

	return GTB_DM;
}

qboolean G_DoesMapSupportGametype( const char *mapname, int gametype ) {
	int i;
	char *type = NULL;

	if ( !level.arenas.infos[0] || !VALIDSTRING( mapname ) )
		return qfalse;

	for ( i=0; i<level.arenas.num; i++ ) {
		type = Info_ValueForKey( level.arenas.infos[i], "map" );

		if ( !Q_stricmp( mapname, type ) )
			break;
	}

	if ( i == level.arenas.num )
		return qfalse;

	type = Info_ValueForKey( level.arenas.infos[i], "type" );

	if ( G_GetMapTypeBits( type ) & (1<<gametype) )
		return qtrue;

	return qfalse;
}

const char *G_RefreshNextMap( int gametype )
{
	int			typeBits = 0;
	int			thisLevel = 0;
	int			desiredMap = 0;
	int			n = 0;
	char		*type = NULL;
	qboolean	loopingUp = qfalse;

	if ( !level.arenas.infos[0] )
		return NULL;

	for ( n=0; n<level.arenas.num; n++ ) {
		type = Info_ValueForKey( level.arenas.infos[n], "map" );

		if ( !Q_stricmp( level.rawmapname, type ) ) {
			thisLevel = n;
			break;
		}
	}

	desiredMap = thisLevel;

	n = thisLevel+1;
	while ( n != thisLevel )
	{ //now cycle through the arena list and find the next map that matches the gametype we're in
		if ( !level.arenas.infos[n] || n >= level.arenas.num )
		{
			//this shouldn't happen, but if it does we have a null entry break in the arena file if this is the case just break out of the loop instead of sticking in an infinite loop
			if ( loopingUp )
				break;
			n = 0;
			loopingUp = qtrue;
		}

		type = Info_ValueForKey( level.arenas.infos[n], "type" );
		
		typeBits = G_GetMapTypeBits( type );
		if ( typeBits & (1<<gametype) ) {
			desiredMap = n;
			break;
		}

		n++;
	}

	//If this is the only level for this game mode or we just can't find a map for this game mode, then nextmap will always restart.
	if ( desiredMap == thisLevel )
		trap->Cvar_Set( "nextmap", "map_restart 0" );
	//otherwise we have a valid nextmap to cycle to, so use it.
	else { 
		type = Info_ValueForKey( level.arenas.infos[desiredMap], "map" );
		trap->Cvar_Set( "nextmap", va( "map %s", type ) );
	}

	return Info_ValueForKey( level.arenas.infos[desiredMap], "map" );
}

/*
===============
G_LoadArenas
===============
*/
#define MAX_MAPS 256
#define MAPSBUFSIZE (MAX_MAPS * 32)

void G_LoadArenas( void ) {
	char	filelist[MAPSBUFSIZE], filename[MAX_QPATH], *fileptr;
	int		i, n, len, numFiles;

	level.arenas.num = 0;

	// get all arenas from .arena files
	numFiles = trap->FS_GetFileList( "maps", ".arena", filelist, ARRAY_LEN( filelist ) );

	fileptr = filelist;

	if (numFiles > MAX_MAPS)
		numFiles = MAX_MAPS;

	for ( i=0; i<numFiles; i++ ) {
		len = strlen( fileptr );
		Com_sprintf( filename, sizeof( filename ), "maps/%s", fileptr );
		G_LoadArenasFromFile( filename );
		fileptr += len + 1;
	}
	trap->Print( "%i arenas parsed\n", level.arenas.num );
	
	for ( n=0; n<level.arenas.num; n++ )
		Info_SetValueForKey( level.arenas.infos[n], "num", va( "%i", n ) );

	G_RefreshNextMap( level.gametype );
}


/*
===============
G_GetArenaInfoByNumber
===============
*/
const char *G_GetArenaInfoByMap( const char *map ) {
	int			n;

	for( n = 0; n < g_numArenas; n++ ) {
		if( Q_stricmp( Info_ValueForKey( g_arenaInfos[n], "map" ), map ) == 0 ) {
			return g_arenaInfos[n];
		}
	}

	return NULL;
}


/*
=================
PlayerIntroSound
=================
*/
static void PlayerIntroSound( const char *modelAndSkin ) {
	char	model[MAX_QPATH];
	char	*skin;

	Q_strncpyz( model, modelAndSkin, sizeof(model) );
	skin = strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	}
	else {
		skin = model;
	}

	if( Q_stricmp( skin, "default" ) == 0 ) {
		skin = model;
	}

	trap->Cbuf_ExecuteText( EXEC_APPEND, va( "play sound/player/announce/%s.wav\n", skin ) );
}

/*
===============
G_AddRandomBot
===============
*/
void G_AddRandomBot( int team ) {
	int		i, n, num;
	char	*value, netname[36], *teamstr;
	gclient_t	*cl;

	num = 0;
	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "cl_name" );
		//
		for ( i=0 ; i< sv_maxclients->integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
				continue;
			}
			if ( team >= 0 && cl->sess.sessionTeam != team ) {
				continue;
			}
			if ( !Q_stricmp( value, cl->pers.netname ) ) {
				break;
			}
		}
		if (i >= sv_maxclients->integer) {
			num++;
		}
	}
	num = (int)(random() * num);
	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "cl_name" );
		//
		for ( i=0 ; i< sv_maxclients->integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
				continue;
			}
			if ( team >= 0 && cl->sess.sessionTeam != team ) {
				continue;
			}
			if ( !Q_stricmp( value, cl->pers.netname ) ) {
				break;
			}
		}
		if (i >= sv_maxclients->integer) {
			num--;
			if (num <= 0) {
				if (team == TEAM_RED) teamstr = "red";
				else if (team == TEAM_BLUE) teamstr = "blue";
				else teamstr = "";
				strncpy(netname, value, sizeof(netname)-1);
				netname[sizeof(netname)-1] = '\0';
				Q_CleanStr(netname);
				trap->Cbuf_ExecuteText( EXEC_INSERT, va("addbot %s %f %s %i\n", netname, g_difficulty->value, teamstr, 0) );
				return;
			}
		}
	}
}

/*
===============
G_RemoveRandomBot
===============
*/
int G_RemoveRandomBot( int team ) {
	int i;
	gclient_t	*cl;

	for ( i=0 ; i< sv_maxclients->integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		trap->Cbuf_ExecuteText( EXEC_INSERT, va("clientkick %d\n", cl->ps.clientNum) );
		return qtrue;
	}
	return qfalse;
}

/*
===============
G_CountHumanPlayers
===============
*/
int G_CountHumanPlayers( int team ) {
	int i, num;
	gclient_t	*cl;

	num = 0;
	for ( i=0 ; i< sv_maxclients->integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_CountBotPlayers
===============
*/
int G_CountBotPlayers( int team ) {
	int i, n, num;
	gclient_t	*cl;

	num = 0;
	for ( i=0 ; i< sv_maxclients->integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		num++;
	}
	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_CheckMinimumPlayers
===============
*/
void G_CheckMinimumPlayers( void ) {
	int minplayers;
	int humanplayers, botplayers;
	static int checkminimumplayers_time;

	if (level.intermissiontime) return;
	//only check once each 10 seconds
	if (checkminimumplayers_time > level.time - 10000) {
		return;
	}
	checkminimumplayers_time = level.time;
	minplayers = bot_minplayers->integer;
	if (minplayers <= 0) return;

	if (level.gametype >= GT_TEAM) {
		if (minplayers >= sv_maxclients->integer / 2) {
			minplayers = (sv_maxclients->integer / 2) -1;
		}

		humanplayers = G_CountHumanPlayers( TEAM_RED );
		botplayers = G_CountBotPlayers(	TEAM_RED );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_RED );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_RED );
		}
		//
		humanplayers = G_CountHumanPlayers( TEAM_BLUE );
		botplayers = G_CountBotPlayers( TEAM_BLUE );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_BLUE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_BLUE );
		}
	}
	else if (level.gametype == GT_DUEL ) {
		if (minplayers >= sv_maxclients->integer) {
			minplayers = sv_maxclients->integer-1;
		}
		humanplayers = G_CountHumanPlayers( -1 );
		botplayers = G_CountBotPlayers( -1 );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			// try to remove spectators first
			if (!G_RemoveRandomBot( TEAM_SPECTATOR )) {
				// just remove the bot that is playing
				G_RemoveRandomBot( -1 );
			}
		}
	}
	else if (level.gametype == GT_DEATHMATCH) {
		if (minplayers >= sv_maxclients->integer) {
			minplayers = sv_maxclients->integer-1;
		}
		humanplayers = G_CountHumanPlayers( TEAM_FREE );
		botplayers = G_CountBotPlayers( TEAM_FREE );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_FREE );
		}
	}
}

/*
===============
G_CheckBotSpawn
===============
*/
void G_CheckBotSpawn( void ) {
	int		n;

	G_CheckMinimumPlayers();

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		ClientBegin( botSpawnQueue[n].clientNum );
		botSpawnQueue[n].spawnTime = 0;
	}
}


/*
===============
AddBotToSpawnQueue
===============
*/
static void AddBotToSpawnQueue( int clientNum, int delay ) {
	int		n;

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			botSpawnQueue[n].spawnTime = level.time + delay;
			botSpawnQueue[n].clientNum = clientNum;
			return;
		}
	}

	trap->Print( S_COLOR_YELLOW "Unable to delay spawn\n" );
	ClientBegin( clientNum );
}


/*
===============
G_RemoveQueuedBotBegin

Called on client disconnect to make sure the delayed spawn
doesn't happen on a freed index
===============
*/
void G_RemoveQueuedBotBegin( int clientNum ) {
	int		n;

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( botSpawnQueue[n].clientNum == clientNum ) {
			botSpawnQueue[n].spawnTime = 0;
			return;
		}
	}
}


/*
===============
G_BotConnect
===============
*/
qboolean G_BotConnect( int clientNum, qboolean restart ) {
	bot_settings_t	settings;
	char			userinfo[MAX_INFO_STRING];

	trap->SV_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );

	Q_strncpyz( settings.characterfile, Info_ValueForKey( userinfo, "characterfile" ), sizeof( settings.characterfile) );
	settings.skill = (float)atof( Info_ValueForKey( userinfo, "skill" ) );
	Q_strncpyz( settings.team, Info_ValueForKey( userinfo, "team" ), sizeof( settings.team ) );

	if ( !BotAISetupClient( clientNum, &settings, restart ) ) {
		trap->SV_GameDropClient( clientNum, "BotAISetupClient failed" );
		return qfalse;
	}

	return qtrue;
}


/*
===============
G_AddBot
===============
*/
static void G_AddBot( const char *name, float skill, const char *team, int delay, char *altname) {
	int clientNum;
	gentity_t *bot;
	char userinfo[MAX_INFO_STRING], *botinfo, *key, *s, *botname, *model;

	// get the botinfo from bots.txt
	botinfo = G_GetBotInfoByName( name );
	if ( !botinfo ) {
		trap->Print( S_COLOR_RED "Error: Bot '%s' not defined\n", name );
		return;
	}

	// create the bot's userinfo
	userinfo[0] = '\0';

	botname = Info_ValueForKey( botinfo, "funname" );
	if( !botname[0] ) {
		botname = Info_ValueForKey( botinfo, "cl_name" );
	}
	// check for an alternative name
	if (altname && altname[0]) {
		botname = altname;
	}
	Info_SetValueForKey( userinfo, "cl_name", botname );
	Info_SetValueForKey( userinfo, "cl_rate", "25000" );
	Info_SetValueForKey( userinfo, "skill", va("%1.2f", skill) );

	if ( skill >= 1 && skill < 2 ) {
		Info_SetValueForKey( userinfo, "handicap", "50" );
	}
	else if ( skill >= 2 && skill < 3 ) {
		Info_SetValueForKey( userinfo, "handicap", "70" );
	}
	else if ( skill >= 3 && skill < 4 ) {
		Info_SetValueForKey( userinfo, "handicap", "90" );
	}

	key = "cg_model";
	model = Info_ValueForKey( botinfo, key );
	if ( !*model )
		model = DEFAULT_MODEL;

	Info_SetValueForKey( userinfo, "model", model );

	key = "color1";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "4";
	}
	Info_SetValueForKey( userinfo, key, s );

	key = "color2";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "5";
	}
	Info_SetValueForKey( userinfo, key, s );

	s = Info_ValueForKey(botinfo, "aifile");
	if (!*s ) {
		trap->Print( S_COLOR_RED "Error: bot has no aifile specified\n" );
		return;
	}

	// have the server allocate a client slot
	clientNum = trap->SV_BotAllocateClient();
	if ( clientNum == -1 ) {
		trap->Print( S_COLOR_RED "Unable to add bot. All player slots are in use.\n" );
		trap->Print( S_COLOR_RED "Start server with more 'open' slots (or check setting of sv_maxclients cvar).\n" );
		return;
	}

	// initialize the bot settings
	if( !team || !*team ) {
		if( level.gametype >= GT_TEAM ) {
			if( PickTeam(clientNum) == TEAM_RED) {
				team = "red";
			}
			else {
				team = "blue";
			}
		}
		else {
			team = "red";
		}
	}
	Info_SetValueForKey( userinfo, "characterfile", Info_ValueForKey( botinfo, "aifile" ) );
	Info_SetValueForKey( userinfo, "skill", va( "%5.2f", skill ) );
	Info_SetValueForKey( userinfo, "team", team );

	bot = &g_entities[ clientNum ];
	bot->r.svFlags |= SVF_BOT;
	bot->inuse = qtrue;

	// register the userinfo
	trap->SV_SetUserinfo( clientNum, userinfo );

	// have it connect to the game as a normal client
	if ( ClientConnect( clientNum, qtrue, qtrue ) ) {
		return;
	}

	if( delay == 0 ) {
		ClientBegin( clientNum );
		return;
	}

	AddBotToSpawnQueue( clientNum, delay );
}


/*
===============
Svcmd_AddBot_f
===============
*/
void Svcmd_AddBot_f( void ) {
	float			skill;
	int				delay;
	char			name[MAX_TOKEN_CHARS];
	char			altname[MAX_TOKEN_CHARS];
	char			string[MAX_TOKEN_CHARS];
	char			team[MAX_TOKEN_CHARS];

	// are bots enabled?
	if ( !trap->Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	// name
	trap->Cmd_Argv( 1, name, sizeof( name ) );
	if ( !name[0] ) {
		trap->Print( "Usage: Addbot <botname> [skill 1-5] [team] [msec delay] [altname]\n" );
		return;
	}

	// skill
	trap->Cmd_Argv( 2, string, sizeof( string ) );
	if ( !string[0] ) {
		skill = 4;
	}
	else {
		skill = (float)atof( string );
	}

	// team
	trap->Cmd_Argv( 3, team, sizeof( team ) );

	// delay
	trap->Cmd_Argv( 4, string, sizeof( string ) );
	if ( !string[0] ) {
		delay = 0;
	}
	else {
		delay = atoi( string );
	}

	// alternative name
	trap->Cmd_Argv( 5, altname, sizeof( altname ) );

	G_AddBot( name, skill, team, delay, altname );

	// if this was issued during gameplay and we are playing locally,
	// go ahead and load the bot's media immediately
	if ( level.time - level.startTime > 1000 &&
		trap->Cvar_VariableIntegerValue( "cl_running" ) ) {
		trap->SV_GameSendServerCommand( -1, "loaddefered\n" );	// FIXME: spelled wrong, but not changing for demo
	}
}

/*
===============
Svcmd_BotList_f
===============
*/
void Svcmd_BotList_f( void ) {
	int i;
	char name[MAX_TOKEN_CHARS];
	char funname[MAX_TOKEN_CHARS];
	char model[MAX_TOKEN_CHARS];
	char aifile[MAX_TOKEN_CHARS];

	trap->Print("^1name             model            aifile              funname\n");
	for (i = 0; i < g_numBots; i++) {
		strcpy(name, Info_ValueForKey( g_botInfos[i], "cl_name" ));
		if ( !*name ) {
			strcpy(name, DEFAULT_NAME);
		}
		strcpy(funname, Info_ValueForKey( g_botInfos[i], "funname" ));
		if ( !*funname ) {
			strcpy(funname, "");
		}
		strcpy(model, Info_ValueForKey( g_botInfos[i], "cg_model" ));
		if ( !*model ) {
			strcpy(model, DEFAULT_MODEL);
		}
		strcpy(aifile, Info_ValueForKey( g_botInfos[i], "aifile"));
		if (!*aifile ) {
			strcpy(aifile, "bots/default_c.c");
		}
		trap->Print( "%-16s %-16s %-20s %-20s\n", name, model, aifile, funname );
	}
}


/*
===============
G_SpawnBots
===============
*/
static void G_SpawnBots( char *botList, int baseDelay ) {
	char		*bot;
	char		*p;
	float		skill;
	int			delay;
	char		bots[MAX_INFO_VALUE];

	podium1 = NULL;
	podium2 = NULL;
	podium3 = NULL;

	skill = G_Cvar_VariableValue( "g_difficulty" );
	if( skill < 1 ) {
		trap->Cvar_Set( "g_difficulty", "1" );
		skill = 1;
	}
	else if ( skill > 5 ) {
		trap->Cvar_Set( "g_difficulty", "5" );
		skill = 5;
	}

	Q_strncpyz( bots, botList, sizeof(bots) );
	p = &bots[0];
	delay = baseDelay;
	while( *p ) {
		//skip spaces
		while( *p && *p == ' ' ) {
			p++;
		}
		if( !p ) {
			break;
		}

		// mark start of bot name
		bot = p;

		// skip until space of null
		while( *p && *p != ' ' ) {
			p++;
		}
		if( *p ) {
			*p++ = 0;
		}

		// we must add the bot this way, calling G_AddBot directly at this stage
		// does "Bad Things"
		trap->Cbuf_ExecuteText( EXEC_INSERT, va("addbot %s %f free %i\n", bot, skill, delay) );

		delay += BOT_BEGIN_DELAY_INCREMENT;
	}
}


/*
===============
G_LoadBotsFromFile
===============
*/
static void G_LoadBotsFromFile( char *filename ) {
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

	g_numBots += G_ParseInfos( buf, MAX_BOTS - g_numBots, &g_botInfos[g_numBots] );
}

/*
===============
G_LoadBots
===============
*/
static void G_LoadBots( void ) {
	cvar_t	*botsFile;
	int			numdirs;
	char		filename[128];
	char		dirlist[1024];
	char*		dirptr;
	int			i;
	int			dirlen;

	if ( !trap->Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	g_numBots = 0;

	botsFile = trap->Cvar_Get( "g_botsFile", "", CVAR_INIT|CVAR_ROM, NULL, NULL );
	if( botsFile->string[0] ) {
		G_LoadBotsFromFile(botsFile->string);
	}
	else {
		G_LoadBotsFromFile("scripts/bots.txt");
	}

	// get all bots from .bot files
	numdirs = trap->FS_GetFileList("scripts", ".bot", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		G_LoadBotsFromFile(filename);
	}
	trap->Print( "%i bots parsed\n", g_numBots );
}



/*
===============
G_GetBotInfoByNumber
===============
*/
char *G_GetBotInfoByNumber( int num ) {
	if( num < 0 || num >= g_numBots ) {
		trap->Print( S_COLOR_RED "Invalid bot number: %i\n", num );
		return NULL;
	}
	return g_botInfos[num];
}


/*
===============
G_GetBotInfoByName
===============
*/
char *G_GetBotInfoByName( const char *name ) {
	int		n;
	char	*value;

	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "cl_name" );
		if ( !Q_stricmp( value, name ) ) {
			return g_botInfos[n];
		}
	}

	return NULL;
}

/*
===============
G_InitBots
===============
*/
void G_InitBots( void ) {
	G_LoadBots();
	G_LoadArenas();

	bot_minplayers = trap->Cvar_Get( "bot_minplayers", "0", CVAR_SERVERINFO, NULL, NULL );
}
