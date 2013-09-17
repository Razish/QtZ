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
#include "g_local.h"

char *ConcatArgs( int start ) {
	int i, c, tlen, len=0;
	char arg[MAX_STRING_CHARS];
	static char	line[MAX_STRING_CHARS];

	c = trap->Cmd_Argc();
	for ( i=start; i<c; i++ ) {
		trap->Cmd_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );

		if ( len + tlen >= sizeof( line )-1 )
			break;

		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 )
			line[len++] = ' ';
	}

	line[len] = 0;

	return line;
}

qboolean StringIsInteger( const char *s ) {
	int i, len;
	qboolean foundDigit = qfalse;

	len = strlen( s );

	for ( i=0; i<len; i++ ) {
		if ( !isdigit( s[i] ) )
			return qfalse;

		foundDigit = qtrue;
	}

	return foundDigit;
}

// Returns a player number for either a number or name string
// Returns -1 if invalid
int ClientNumberFromString( gentity_t *to, const char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		cleanName[MAX_STRING_CHARS];

	// numeric values could be slot numbers
	if ( StringIsInteger( s ) ) {
		idnum = atoi( s );
		if ( idnum >= 0 && idnum < level.maxclients ) {
			cl = &level.clients[idnum];
			if ( cl->pers.connected == CON_CONNECTED )
				return idnum;
		}
	}

	// check for a name match
	for ( idnum=0, cl=level.clients; idnum<level.maxclients; idnum++, cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED )
			continue;

		Q_strncpyz( cleanName, cl->pers.netname, sizeof( cleanName ) );
		Q_CleanStr( cleanName );
		if ( !Q_stricmp( cleanName, s ) )
			return idnum;
	}

	trap->SV_GameSendServerCommand( to-g_entities, va( "print \"User %s is not on the server\n\"", s ) );
	return -1;
}

void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	if ( client->sess.sessionTeam == TEAM_RED )
		trap->SV_GameSendServerCommand( -1, va( "cp \"%s\n" S_COLOR_WHITE " joined the red team.\n\"", client->pers.netname ) );
	else if ( client->sess.sessionTeam == TEAM_BLUE )
		trap->SV_GameSendServerCommand( -1, va( "cp \"%s\n" S_COLOR_WHITE " joined the blue team.\n\"", client->pers.netname ) );
	else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR )
		trap->SV_GameSendServerCommand( -1, va( "cp \"%s\n" S_COLOR_WHITE " joined the spectators.\n\"", client->pers.netname ) );
	else if ( client->sess.sessionTeam == TEAM_FREE )
		trap->SV_GameSendServerCommand( -1, va( "cp \"%s\n" S_COLOR_WHITE " joined the battle.\n\"", client->pers.netname ) );
}

void SetTeam( gentity_t *ent, char *s ) {
	int					clientNum, specClient=0;
	team_t				team, oldTeam;
	gclient_t			*client = ent->client;
	spectatorState_t	specState = SPECTATOR_NOT;

	// see what change is requested
	clientNum = client-level.clients;

	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	}

	else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	}

	else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	}

	else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	}

	// if running a team game, assign player to one of the teams
	else if ( level.gametype >= GT_TEAM ) {
		specState = SPECTATOR_NOT;
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) )
			team = TEAM_RED;
		else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) )
			team = TEAM_BLUE;
		else
			team = PickTeam( clientNum ); // pick the team with the least number of players

		if ( g_teamForceBalance->integer  ) {
			int		counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = TeamCount( clientNum, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( clientNum, TEAM_RED );

			// We allow a spread of two
			if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
				trap->SV_GameSendServerCommand( clientNum, "cp \"Red team has too many players.\n\"" );
				return; // ignore the request
			}
			if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
				trap->SV_GameSendServerCommand( clientNum, "cp \"Blue team has too many players.\n\"" );
				return; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}

	}
	else
		team = TEAM_FREE; // force them to spectators if there aren't any spots free

	// override decision if limiting the players
	if ( (level.gametype == GT_DUEL) && level.numNonSpectatorClients >= 2 )
		team = TEAM_SPECTATOR;
	else if ( g_maxGameClients->integer > 0 && level.numNonSpectatorClients >= g_maxGameClients->integer )
		team = TEAM_SPECTATOR;

	// decide if we will allow the change
	oldTeam = client->sess.sessionTeam;
	if ( team == oldTeam && team != TEAM_SPECTATOR )
		return;

	// if the player was dead leave the body
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		CopyToBodyQue( ent );
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die( ent, ent, ent, 100000, MOD_SUICIDE );

	}

	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR && oldTeam != team )
		AddTournamentQueue( client );

	// exploit fix: with 3 (any odd amount?) players connected, one could /callvote map x followed by /team s to force the vote
	G_ClearVote( ent );

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	BroadcastTeamChange( client, oldTeam );
	ClientUserinfoChanged( clientNum );
	ClientBegin( clientNum );
}

// If the client being followed leaves the game, or you just want to drop to free floating spectator mode
void StopFollowing( gentity_t *ent ) {
	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;	
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;	
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
}

static void Cmd_AddBot_f( gentity_t *ent ) {
	trap->SV_GameSendServerCommand( ent-g_entities, "print \"You can only add bots as the server\n\"" );
}

static qboolean G_VoteAllready( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	if ( !level.warmupTime ) {
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"allready is only available during warmup.\n\"" );
		return qfalse;
	}
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1 );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

static qboolean G_VoteCapturelimit( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Q_clampi( 0, atoi( arg2 ), 0x7FFFFFFF );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

static qboolean G_VoteClientkick( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = atoi ( arg2 );

	if ( n < 0 || n >= MAX_CLIENTS )
	{
		trap->SV_GameSendServerCommand( ent-g_entities, va("print \"invalid client number %d.\n\"", n ) );
		return qfalse;
	}

	if ( g_entities[n].client->pers.connected == CON_DISCONNECTED )
	{
		trap->SV_GameSendServerCommand( ent-g_entities, va("print \"there is no client with the client number %d.\n\"", n ) );
		return qfalse;
	}
		
	Com_sprintf( level.voteString, sizeof(level.voteString ), "%s %s", arg1, arg2 );
	Com_sprintf( level.voteDisplayString, sizeof(level.voteDisplayString), "%s %s", arg1, g_entities[n].client->pers.netname );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

static qboolean G_VoteCointoss( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1 );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

static qboolean G_VoteFraglimit( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Q_clampi( 0, atoi( arg2 ), 0x7FFFFFFF );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

static qboolean G_VoteGametype( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int gt = atoi( arg2 );

	if ( arg2[0] && isalpha( arg2[0] ) )
	{// ffa, ctf, tdm, etc
		gt = GametypeIDForString( arg2 );
		if ( gt == -1 )
		{
			trap->SV_GameSendServerCommand( ent-g_entities, va( "print \"Gametype (%s) unrecognised, defaulting to Deathmatch\n\"", arg2 ) );
			gt = GT_DEATHMATCH;
		}
	}
	else if ( gt < 0 || gt >= GT_NUM_GAMETYPES )
	{// numeric but out of range
		trap->SV_GameSendServerCommand( ent-g_entities, va( "print \"Gametype (%i) is out of range, defaulting to Deathmatch\n\"", gt ) );
		gt = GT_DEATHMATCH;
	}

	level.votingGametype = qtrue;
	level.votingGametypeTo = gt;

	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %d", arg1, gt );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", arg1, gametypeStringShort[gt] );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

static qboolean G_VoteShootFromEye( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Q_clampi( 0, atoi( arg2 ), 1 );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	return qtrue;
}

static qboolean G_VoteSpeedcaps( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Q_clampi( 0, atoi( arg2 ), 1 );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

static qboolean G_VoteKick( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int clientid = ClientNumberFromString( ent, arg2 );
	gentity_t *target = NULL;

	if ( clientid == -1 )
		return qfalse;

	target = &g_entities[clientid];
	if ( !target || !target->inuse || !target->client )
		return qfalse;

	Com_sprintf( level.voteString, sizeof( level.voteString ), "clientkick %d", clientid );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "kick %s", target->client->pers.netname );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

const char *G_GetArenaInfoByMap( const char *map );

static void Cmd_MapList_f( gentity_t *ent ) {
	int i, toggle=0;
	char map[24] = "--", buf[512] = {0};

	Q_strcat( buf, sizeof( buf ), "Map list:" );

	for ( i=0; i<level.arenas.num; i++ )
	{
		Q_strncpyz( map, Info_ValueForKey( level.arenas.infos[i], "map" ), sizeof( map ) );
		Q_CleanColorStr( map );

		if ( G_DoesMapSupportGametype( map, level.gametype ) )
		{
			char *tmpMsg = va( " ^%c%s", (++toggle&1) ? '2' : '3', map );
			if ( strlen( buf ) + strlen( tmpMsg ) >= sizeof( buf ) )
			{
				trap->SV_GameSendServerCommand( ent-g_entities, va( "print \"%s\"", buf ) );
				buf[0] = '\0';
			}
			Q_strcat( buf, sizeof( buf ), tmpMsg );
		}
	}

	trap->SV_GameSendServerCommand( ent-g_entities, va( "print \"%s\n\"", buf ) );
}

static qboolean G_VoteMap( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	char s[MAX_CVAR_VALUE_STRING] = {0}, *mapName = NULL, *mapName2 = NULL;
	const char *arenaInfo;

	if ( numArgs < 3 )
	{// didn't specify a map, show available maps
		Cmd_MapList_f( ent );
		return qfalse;
	}

	if ( !G_DoesMapSupportGametype( arg2, level.gametype ) ) {
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"That map is not supported by this gametype\n\"" );
		return qfalse;
	}

	//preserve the map rotation
	trap->Cvar_VariableStringBuffer( "nextmap", s, sizeof( s ) );
	if ( *s )
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", arg1, arg2, s );
	else
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );

	arenaInfo = G_GetArenaInfoByMap(arg2);
	if ( arenaInfo )
	{
		mapName = Info_ValueForKey(arenaInfo, "longname");
		mapName2 = Info_ValueForKey(arenaInfo, "map");
	}

	if ( !mapName || !mapName[0] )
		mapName = "ERROR";

	if ( !mapName2 || !mapName2[0] )
		mapName2 = "ERROR";

	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "map %s (%s)", mapName, mapName2 );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

static qboolean G_VoteNextmap( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	char	s[MAX_CVAR_VALUE_STRING];

	trap->Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
	if ( !*s ) {
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"nextmap not set.\n\"" );
		return qfalse;
	}

	Com_sprintf( level.voteString, sizeof( level.voteString ), "vstr nextmap");
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

static qboolean G_VotePause( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1 );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

static qboolean G_VoteShuffle( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1 );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

static qboolean G_VoteTimelimit( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	float tl = Q_clamp( 0.0f, (float)atof( arg2 ), 35790.0f );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %.3f", arg1, tl );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

static qboolean G_VoteWarmup( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Q_clampi( 0, atoi( arg2 ), 1 );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "g_doWarmup %i", arg1, n );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	return qtrue;
}

typedef struct voteString_s {
	const char		*string;
	const char		*aliases;	// space delimited list of aliases, will always show the real vote string
	qboolean		(*func)(gentity_t *ent, int numArgs, const char *arg1, const char *arg2);
	int				numArgs;	// number of REQUIRED arguments, not total/optional arguments
	unsigned int	validGT;	// bit-flag of valid gametypes
	qboolean		voteDelay;	// if true, will delay executing the vote string after it's accepted by japp_voteDelay
	const char		*shortHelp;	// NULL if no arguments needed
	const char		*longHelp;	// long help text, accessible with /vhelp and maybe /callvote help
} voteString_t;

static voteString_t validVoteStrings[] = {
	//	vote string				aliases									# args	valid gametypes				exec delay		short help	long help
	{	"allready",				"ready",			G_VoteAllready,		2,		GTB_ALL,					qfalse,			NULL,		"" },
	{	"capturelimit",			"caps",				G_VoteCapturelimit,	3,		GTB_CTF,					qtrue,			"<num>",	"" },
	{	"clientkick",			NULL,				G_VoteClientkick,	3,		GTB_ALL,					qfalse,			"<num>",	"" },
	{	"cointoss",				"coinflip",			G_VoteCointoss,		2,		GTB_ALL,					qfalse,			NULL,		"" },
	{	"fraglimit",			"frags",			G_VoteFraglimit,	3,		GTB_ALL & ~(GTB_CTF),		qtrue,			"<num>",	"" },
	{	"sv_gametype",			"gametype gt mode",	G_VoteGametype,		3,		GTB_ALL,					qtrue,			"<name>",	"" },
	{	"g_shootFromEye",		"shootfromeye",		G_VoteShootFromEye,	2,		GTB_ALL,					qfalse,			"<0-1>",	"" },
	{	"g_speedCaps",			"speedcaps",		G_VoteSpeedcaps,	3,		GTB_CTF,					qfalse,			"<0-1>",	"" },
	{	"kick",					NULL,				G_VoteKick,			3,		GTB_ALL,					qfalse,			"<name>",	"" },
	{	"map",					NULL,				G_VoteMap,			2,		GTB_ALL,					qtrue,			"<name>",	"" },
	{	"map_restart",			"restart",			NULL,				2,		GTB_ALL,					qtrue,			NULL,		"Restarts the current map\nExample: callvote map_restart" },
	{	"nextmap",				NULL,				G_VoteNextmap,		2,		GTB_ALL,					qtrue,			NULL,		"" },
	{	"pause",				NULL,				G_VotePause,		2,		GTB_ALL,					qfalse,			NULL,		"" },
	{	"shuffle",				NULL,				G_VoteShuffle,		2,		GTB_ALL & ~(GTB_NOTTEAM),	qtrue,			NULL,		"" },
	{	"timelimit",			"time",				G_VoteTimelimit,	3,		GTB_ALL,					qtrue,			"<num>",	"" },
	{	"warmup",				"dowarmup",			G_VoteWarmup,		3,		GTB_ALL,					qtrue,			"<0-1>",	"" },
};
static const int validVoteStringsSize = ARRAY_LEN( validVoteStrings );

static void Cmd_CallVote_f( gentity_t *ent ) {
	int			i=0, numArgs=trap->Cmd_Argc();
	char		arg1[MAX_CVAR_VALUE_STRING] = {0}, arg2[MAX_CVAR_VALUE_STRING] = {0};
	voteString_t *vote = NULL;

	if ( !g_allowVote->integer )
	{// not allowed to vote at all
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"Not allowed to vote\n\"" );
		return;
	}

	else if ( level.voteTime || level.voteExecuteTime >= level.time )
	{// vote in progress
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"A vote is already in progress\n\"" );
		return;
	}

	else if ( level.gametype != GT_DUEL && ent->client->sess.sessionTeam == TEAM_SPECTATOR )
	{// can't vote as a spectator, except in duel
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"Not allowed to call a vote as spectator\n\"" );
		return;
	}

	// make sure it is a valid command to vote on
	numArgs = trap->Cmd_Argc();
	trap->Cmd_Argv( 1, arg1, sizeof( arg1 ) );
	if ( numArgs > 1 )
		Q_strncpyz( arg2, ConcatArgs( 2 ), sizeof( arg2 ) ); //trap->Cmd_Argv( 2, arg2, sizeof( arg2 ) );

	//Raz: callvote exploit, filter \n and \r ==> in both args
	if ( Q_strchrs( arg1, ";\r\n" ) || Q_strchrs( arg2, ";\r\n" ) )
	{
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	for ( i=0; i<validVoteStringsSize; i++ )
	{// check for invalid votes
		if ( (g_voteDisable->integer & (1<<i)) )
			continue;

		if ( !Q_stricmp( arg1, validVoteStrings[i].string ) )
			break;

		if ( validVoteStrings[i].aliases )
		{// see if they're using an alias, and set arg1 to the actual vote string
			char tmp[MAX_TOKEN_CHARS] = {0}, *p = NULL, *delim = " ";
			Q_strncpyz( tmp, validVoteStrings[i].aliases, sizeof( tmp ) );
			p = strtok( tmp, delim );
			while ( p != NULL )
			{
				if ( !Q_stricmp( arg1, p ) )
				{
					Q_strncpyz( arg1, validVoteStrings[i].string, sizeof( arg1 ) );
					goto validVote;
				}
				p = strtok( NULL, delim );
			}
		}
	}
	if ( i == validVoteStringsSize )
	{// invalid vote string, abandon ship
		char buf[1024] = {0};
		int toggle = 0;
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"Allowed vote strings are: \"" );
		for ( i=0; i<validVoteStringsSize; i++ )
		{
			if ( (g_voteDisable->integer & (1<<i)) )
				continue;

			toggle = !toggle;
			if ( validVoteStrings[i].shortHelp ) {
				Q_strcat( buf, sizeof( buf ), va( "^%c%s %s ",
													toggle?'2':'3',
													validVoteStrings[i].string,
													validVoteStrings[i].shortHelp ) );
			}
			else {
				Q_strcat( buf, sizeof( buf ), va( "^%c%s ",
													toggle?'2':'3',
													validVoteStrings[i].string ) );
			}
		}

		//RAZTODO: buffer and send in multiple messages in case of overflow
		trap->SV_GameSendServerCommand( ent-g_entities, va( "print \"%s\n\"", buf ) );
		return;
	}

validVote:
	vote = &validVoteStrings[i];
	if ( !(vote->validGT & (1<<level.gametype)) ) {
		trap->SV_GameSendServerCommand( ent-g_entities, va( "print \"%s is not applicable in this gametype.\n\"", arg1 ) );
		return;
	}

	if ( numArgs < vote->numArgs ) {
		trap->SV_GameSendServerCommand( ent-g_entities, va( "print \"%s requires more arguments: %s\n\"", arg1, vote->shortHelp ) );
		return;
	}

	level.votingGametype = qfalse;

	level.voteExecuteDelay = vote->voteDelay ? g_voteDelay->integer : 0;

	if ( level.voteExecuteTime )
	{// there is still a vote to be executed, execute it and store the new vote
		level.voteExecuteTime = 0;
		if ( level.votePoll )
			trap->Cbuf_ExecuteText( EXEC_APPEND, va( "%s\n", level.voteString ) );
	}

	if ( vote->func )
	{
		if ( !vote->func( ent, numArgs, arg1, arg2 ) )
			return;
	}
	else
	{
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
		Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	}
	Q_strstrip( level.voteStringClean, "\"\n\r", NULL );

	trap->SV_GameSendServerCommand( -1, va( "print \"%s^7 called a vote (%s)\n\"", ent->client->pers.netname, level.voteStringClean ) );

	// start the voting, the caller automatically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;
	level.votePoll = qfalse;

	for ( i=0; i<level.maxclients; i++ ) {
		level.clients[i].gameFlags &= ~GF_VOTED;
		level.clients[i].pers.vote = 0;
	}

	ent->client->gameFlags |= GF_VOTED;
	ent->client->pers.vote = 1;

	trap->SV_SetConfigstring( CS_VOTE_TIME,	va( "%i", level.voteTime ) );
	trap->SV_SetConfigstring( CS_VOTE_STRING,	level.voteDisplayString );	
	trap->SV_SetConfigstring( CS_VOTE_YES,		va( "%i", level.voteYes ) );
	trap->SV_SetConfigstring( CS_VOTE_NO,		va( "%i", level.voteNo ) );	
}

static void Cmd_Drop_f( gentity_t *ent ) {
	char arg[128] = {0};

	trap->Cmd_Argv( 1, arg, sizeof( arg ) );

	if ( !Q_stricmp( arg, "flag" ) )
	{
		powerup_t powerup = (ent->client->ps.persistant[PERS_TEAM]==TEAM_RED) ? PW_BLUEFLAG : PW_REDFLAG;

		if ( !g_allowFlagDrop->integer ) {
			trap->SV_GameSendServerCommand( ent-g_entities, "print \"^3Not allowed to drop the flag\n\"" );
			return;
		}

		if ( level.gametype != GT_CTF || (ent->client->ps.powerups[powerup] <= level.time) ) {
			trap->SV_GameSendServerCommand( ent-g_entities, "print \"^3No flag to drop\n\"" );
			return;
		}

		if ( ent->client->ps.powerups[ powerup ] > level.time )
		{
			const gitem_t *item = BG_FindItemForPowerup( powerup );
			gentity_t *drop = NULL;
			vector3 angs = { 0.0f, 0.0f, 0.0f };

			AngleVectors( &ent->client->ps.viewangles, &angs, NULL, NULL );

			drop = Drop_Item( ent, item, angs.yaw );
			//Raz: speed caps
			drop->genericValue1 = trap->Milliseconds() - (int)ent->client->pers.teamState.flagsince;
		//	drop->genericValue2 = ent-g_entities;
		//	drop->genericValue2 |= (1<<5); // 6th bit indicates a client dropped it on purpose
		//	drop->genericValue2 |= level.time << 6;

			ent->client->ps.powerups[powerup] = 0;
		}
	}
	else if ( !Q_stricmp( arg, "weapon" ) )
	{
		weapon_t wp = (weapon_t)ent->client->ps.weapon, newWeap = -1;
		const gitem_t *item = NULL;
		gentity_t *drop = NULL;
		vector3 angs = { 0.0f, 0.0f, 0.0f };
		int ammo, i=0;

		if ( !g_allowWeaponDrop->integer ) {
			trap->SV_GameSendServerCommand( ent-g_entities, "print \"^3Not allowed to drop weapons\n\"" );
			return;
		}

		if ( !(ent->client->ps.stats[STAT_WEAPONS] & (1<<wp)) || !ent->client->ps.ammo[wp]
			|| wp == WP_QUANTIZER || wp <= WP_NONE || wp > WP_NUM_WEAPONS )
		{// We don't have this weapon or ammo for it, or it's a 'weapon' that can't be dropped
			trap->SV_GameSendServerCommand( ent-g_entities, "print \"^3Can't drop this weapon\n\"" );
			return;
		}

		item = BG_FindItemForWeapon( wp );

		if ( !ent->client->ps.ammo[wp] ) {
			trap->SV_GameSendServerCommand( ent-g_entities, "print \"^3No ammo for this weapon\n\"" );
			return;
		}
		else if ( ent->client->ps.ammo[wp] > item->quantity )
			ammo = item->quantity;
		else
			ammo = ent->client->ps.ammo[wp];

		AngleVectors( &ent->client->ps.viewangles, &angs, NULL, NULL );

		drop = Drop_Item( ent, item, angs.yaw );
		drop->count = ammo;
		ent->client->ps.ammo[wp] -= ammo;
		ent->client->ps.stats[STAT_WEAPONS] &= ~(1<<wp);

#if 1
		for ( i=0; i<WP_NUM_WEAPONS; i++ )
		{
			if ((ent->client->ps.stats[STAT_WEAPONS] & (1 << i)) && i != WP_NONE)
			{ //this one's good
				newWeap = (weapon_t)i;
				break;
			}
		}

		if (newWeap != -1)
		{
			ent->s.weapon = newWeap;
			ent->client->ps.weapon = newWeap;
		}
		else
		{
			ent->s.weapon = 0;
			ent->client->ps.weapon = 0;
		}

		G_AddEvent( ent, EV_NOAMMO, wp );
#else
		for ( i=wp+1; i != wp-1; i++ )
		{
			if ( i == WP_NUM_WEAPONS )
				i = WP_NONE+1;
			if ( ent->client->ps.stats[STAT_WEAPONS] & (1<<i) )
			{
				ent->client->ps.weapon = i;
				break;
			}
		}
		G_AddEvent( ent, EV_NOAMMO, wp );
#endif

		drop->genericValue2 = ent-g_entities;
		drop->genericValue2 |= (1<<5); // 6th bit indicates a client dropped it on purpose
		drop->genericValue2 |= level.time << 6;
		}
	else if ( !Q_stricmp( arg, "powerup" ) )
	{
		//RAZTODO: /drop powerup
		return;
	}
}

static void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];

	if ( trap->Cmd_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap->Cmd_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

static void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int clientnum, original;

	// first set them to spectator
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		SetTeam( ent, "spectator" );
	}

	if ( dir != 1 && dir != -1 ) {
		trap->Error( ERR_DROP, "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	// if dedicated follow client, just switch between the two auto clients
	if (ent->client->sess.spectatorClient < 0) {
		if (ent->client->sess.spectatorClient == -1) {
			ent->client->sess.spectatorClient = -2;
		} else if (ent->client->sess.spectatorClient == -2) {
			ent->client->sess.spectatorClient = -1;
		}
		return;
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}

void Cmd_FollowNext_f( gentity_t *ent ) {
	Cmd_FollowCycle_f( ent, 1 );
}

void Cmd_FollowPrev_f( gentity_t *ent ) {
	Cmd_FollowCycle_f( ent, -1 );
}

static void G_Give( gentity_t *ent, const char *name, const char *args, int argc ) {
	const gitem_t *it;
	int			i;
	qboolean	give_all = qfalse;
	gentity_t	*it_ent;
	trace_t		trace;

	if ( !Q_stricmp( name, "all" ) )
		give_all = qtrue;

	if ( give_all || !Q_stricmp( name, "health") )
	{
		if ( argc == 3 )
			ent->health = Q_clampi( 1, atoi( args ), MAX_HEALTH );
		else
			ent->health = MAX_HEALTH;
		if ( !give_all )
			return;
	}

	if ( give_all || !Q_stricmp( name, "armor" ) || !Q_stricmp( name, "shield" ) )
	{
		if ( argc == 3 )
			ent->client->ps.stats[STAT_ARMOR] = Q_clampi( 0, atoi( args ), MAX_ARMOR );
		else
			ent->client->ps.stats[STAT_ARMOR] = MAX_ARMOR;

		if ( !give_all )
			return;
	}

	if ( give_all || !Q_stricmp( name, "weapons" ) )
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << (WP_NUM_WEAPONS)) - (1 << WP_NONE);
		if ( !give_all )
			return;
	}
	
	if ( !give_all && !Q_stricmp( name, "weaponnum" ) )
	{
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << atoi( args ));
		return;
	}

	if ( give_all || !Q_stricmp( name, "ammo" ) )
	{
		int num = 999;
		if ( argc == 3 )
			num = atoi( args );
		for ( i=0; i<MAX_WEAPONS; i++ )
			ent->client->ps.ammo[i] = num;
		if ( !give_all )
			return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem( name );
		if ( !it )
			return;

		it_ent = G_Spawn();
		VectorCopy( &ent->r.currentOrigin, &it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem( it_ent, it );
		FinishSpawningItem( it_ent );
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item( it_ent, ent, &trace );
		if ( it_ent->inuse )
			G_FreeEntity( it_ent );
	}
}

static void Cmd_Give_f( gentity_t *ent )
{
	char name[MAX_TOKEN_CHARS] = {0};

	trap->Cmd_Argv( 1, name, sizeof( name ) );
	G_Give( ent, name, ConcatArgs( 2 ), trap->Cmd_Argc() );
}

static void Cmd_GiveOther_f( gentity_t *ent )
{
	char		name[MAX_TOKEN_CHARS] = {0};
	int			i;
	char		otherindex[MAX_TOKEN_CHARS];
	gentity_t	*otherEnt = NULL;

	trap->Cmd_Argv( 1, otherindex, sizeof( otherindex ) );
	if ( !otherindex[0] )
	{
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"giveother requires that the second argument be a client index number.\n\"" );
		return;
	}

	i = atoi( otherindex );
	if ( i < 0 || i >= MAX_CLIENTS )
	{
		trap->SV_GameSendServerCommand( ent-g_entities, va( "print \"%i is not a client index.\n\"", i ) );
		return;
	}

	otherEnt = &g_entities[i];
	if ( !otherEnt->inuse || !otherEnt->client )
	{
		trap->SV_GameSendServerCommand( ent-g_entities, va( "print \"%i is not an active client.\n\"", i ) );
		return;
	}

	trap->Cmd_Argv( 2, name, sizeof( name ) );

	G_Give( otherEnt, name, ConcatArgs( 3 ), trap->Cmd_Argc()-1 );
}

static void Cmd_God_f( gentity_t *ent ) {
	char *msg = NULL;

	ent->flags ^= FL_GODMODE;
	if ( !(ent->flags & FL_GODMODE) )
		msg = "godmode OFF";
	else
		msg = "godmode ON";

	trap->SV_GameSendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}

static void Cmd_Kill_f( gentity_t *ent ) {
	//OSP: pause
	if ( level.pause.state != PAUSE_NONE )
		return;

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die( ent, ent, ent, 100000, MOD_SUICIDE );
}

static void Cmd_Noclip_f( gentity_t *ent ) {
	char *msg = NULL;

	ent->client->noclip = (qboolean)!ent->client->noclip;
	if ( !ent->client->noclip )
		msg = "noclip OFF";
	else
		msg = "noclip ON";

	trap->SV_GameSendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}

static void Cmd_Ready_f( gentity_t *ent ) {
	char *publicMsg = NULL;
	gentity_t *e = NULL;
	int i = 0;

	ent->client->pers.ready = (qboolean)!ent->client->pers.ready;

	if ( ent->client->pers.ready ) {
		publicMsg = va( "cp \"%s\n^7is ready\"", ent->client->pers.netname );
		trap->SV_GameSendServerCommand( ent-g_entities, va( "cp \"^2You are ready\"" ) );
	}
	else {
		publicMsg = va( "cp \"%s\n^3is NOT ready\"", ent->client->pers.netname );
		trap->SV_GameSendServerCommand( ent-g_entities, va( "cp \"^3You are NOT ready\"" ) );
	}

	// send public message to everyone BUT this client, so they see their own message
	for ( i=0, e=g_entities; i<level.maxclients; i++, e++ ) {
		if ( e != ent )
			trap->SV_GameSendServerCommand( e-g_entities, publicMsg );
	}
}

static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message, char *locMsg ) {
	if ( !other || !other->inuse || !other->client )
		return;

	if ( other->client->pers.connected != CON_CONNECTED )
		return;

	if ( mode == SAY_TEAM  && !OnSameTeam(ent, other) )
		return;

	//QTZTODO: specmute
	/*
	// no chatting to players in tournements
	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
		&& other->client->sess.sessionTeam == TEAM_FREE
		&& ent->client->sess.sessionTeam != TEAM_FREE ) {
		//Hmm, maybe some option to do so if allowed?  Or at least in developer mode...
		return;
	}
	*/

	if ( locMsg )
	{
		trap->SV_GameSendServerCommand( other-g_entities, va("%s \"%s\" \"%s\" \"%c\" \"%s\"",
			mode == SAY_TEAM ? "ltchat" : "lchat",
			name, locMsg, color, message));
	}
	else
	{
		trap->SV_GameSendServerCommand( other-g_entities, va("%s \"%s%c%c%s\"", 
			mode == SAY_TEAM ? "tchat" : "chat",
			name, Q_COLOR_ESCAPE, color, message));
	}
}

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int			j, color;
	gentity_t	*other;
	char		name[96], location[64], text[MAX_SAY_TEXT];
	char		*locMsg = NULL;
	qboolean	isMeCmd = qfalse;

	if ( level.gametype < GT_TEAM && mode == SAY_TEAM )
		mode = SAY_ALL;

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say: %s: %s\n", ent->client->pers.netname, chatText );
		if ( !Q_stricmpn( chatText, "/me ", 4 ) ) {
			isMeCmd = qtrue;
			chatText += 4; //Skip "/me "
		}
		if ( isMeCmd ) {
			Com_sprintf( name, sizeof( name ), "^7* %s^7"EC_GLOBAL" ", ent->client->pers.netname );
			color = COLOR_WHITE;
		}
		else {
			Com_sprintf( name, sizeof( name ), "%s^7"EC_GLOBAL": ", ent->client->pers.netname );
			color = COLOR_GREEN;
		}
		break;

	case SAY_TEAM:
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		if ( Team_GetLocationMsg( ent, location, sizeof( location ) ) ) {
			Com_sprintf( name, sizeof( name ), EC_GLOBAL"(%s%c%c"EC_GLOBAL")"EC_GLOBAL": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
			locMsg = location;
		}
		else
			Com_sprintf( name, sizeof( name ), EC_GLOBAL"(%s%c%c"EC_GLOBAL")"EC_GLOBAL": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;

	case SAY_TELL:
		if ( target && level.gametype >= GT_TEAM && target->client->sess.sessionTeam == ent->client->sess.sessionTeam && Team_GetLocationMsg( ent, location, sizeof( location ) ) ) {
			Com_sprintf( name, sizeof( name ), EC_GLOBAL"[%s%c%c"EC_GLOBAL"]"EC_GLOBAL": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
			locMsg = location;
		}
		else
			Com_sprintf( name, sizeof( name ), EC_GLOBAL"[%s%c%c"EC_GLOBAL"]"EC_GLOBAL": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_PINK;
		break;
	}

	Q_strncpyz( text, chatText, sizeof(text) );
	Q_strstrip( text, "\n\r", NULL );

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text, locMsg );
		return;
	}

	// echo the text to the console
	if ( dedicated->integer )
		trap->Print( "%s%s\n", name, text);

	// send it to all the apropriate clients
	for ( j=0, other=g_entities; j<level.maxclients; j++, other++ )
		G_SayTo( ent, other, mode, color, name, text, locMsg );
}

static void Cmd_Say_f( gentity_t *ent ) {
	char *p = NULL;

	if ( trap->Cmd_Argc() < 2 )
		return;

	p = ConcatArgs( 1 );

	//Raz: BOF
	if ( strlen( p ) > MAX_SAY_TEXT ) {
		p[MAX_SAY_TEXT-1] = '\0';
		G_LogPrintf( "Cmd_Say_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}

	G_Say( ent, NULL, SAY_ALL, p );
}

static void Cmd_SayTeam_f( gentity_t *ent ) {
	char *p = NULL;

	if ( trap->Cmd_Argc() < 2 )
		return;

	p = ConcatArgs( 1 );

	//Raz: BOF
	if ( strlen( p ) > MAX_SAY_TEXT ) {
		p[MAX_SAY_TEXT-1] = '\0';
		G_LogPrintf( "Cmd_SayTeam_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}

	G_Say( ent, NULL, (level.gametype>=GT_TEAM) ? SAY_TEAM : SAY_ALL, p );
}

static void Cmd_Score_f( gentity_t *ent ) {
	ent->client->scoresWaiting = qtrue;
}

static void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if ( trap->Cmd_Argc() != 2 ) {
		oldTeam = ent->client->sess.sessionTeam;
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap->SV_GameSendServerCommand( ent-g_entities, "print \"Blue team\n\"" );
			break;
		case TEAM_RED:
			trap->SV_GameSendServerCommand( ent-g_entities, "print \"Red team\n\"" );
			break;
		case TEAM_FREE:
			trap->SV_GameSendServerCommand( ent-g_entities, "print \"Free team\n\"" );
			break;
		case TEAM_SPECTATOR:
			trap->SV_GameSendServerCommand( ent-g_entities, "print \"Spectator team\n\"" );
			break;
		}
		return;
	}

	if ( ent->client->switchTeamTime > level.time ) {
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"May not switch teams more than once per 5 seconds.\n\"" );
		return;
	}

	trap->Cmd_Argv( 1, s, sizeof( s ) );

	SetTeam( ent, s );
	ent->client->switchTeamTime = level.time + 5000;
}

static void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p, arg[MAX_TOKEN_CHARS];

	if ( trap->Cmd_Argc() < 2 )
		return;

	trap->Cmd_Argv( 1, arg, sizeof( arg ) );
	targetNum = ClientNumberFromString( ent, arg );
	if ( targetNum == -1 )
		return;

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client )
		return;

	p = ConcatArgs( 2 );

	//Raz: BOF
	if ( strlen( p ) > MAX_SAY_TEXT ) {
		p[MAX_SAY_TEXT-1] = '\0';
		G_LogPrintf( "Cmd_Tell_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT) )
		G_Say( ent, ent, SAY_TELL, p );
}

void Cmd_TargetUse_f( gentity_t *ent ) {
	if ( trap->Cmd_Argc() > 1 )
	{
		char sArg[MAX_STRING_CHARS] = {0};
		gentity_t *targ;

		trap->Cmd_Argv( 1, sArg, sizeof( sArg ) );
		targ = G_Find( NULL, FOFS( targetname ), sArg );

		while ( targ )
		{
			if ( targ->use )
				targ->use( targ, ent, ent );
			targ = G_Find( targ, FOFS( targetname ), sArg );
		}
	}
}

void Cmd_Vote_f( gentity_t *ent ) {
	char msg[64];

	if ( !level.voteTime ) {
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"No vote in progress\n\"" );
		return;
	}
	if ( ent->client->gameFlags & GF_VOTED ) {
		trap->SV_GameSendServerCommand( ent-g_entities, "print \"Vote already cast\n\"" );
		return;
	}
	if ( level.gametype != GT_DUEL )
	{
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			trap->SV_GameSendServerCommand( ent-g_entities, "print \"Not allowed to vote as spectator\n\"" );
			return;
		}
	}

	trap->SV_GameSendServerCommand( ent-g_entities, "print \"Vote cast\n\"" );

	ent->client->gameFlags |= GF_VOTED;

	trap->Cmd_Argv( 1, msg, sizeof( msg ) );

	if ( tolower( msg[0] ) == 'y' || msg[0] == '1' ) {
		level.voteYes++;
		ent->client->pers.vote = 1;
		trap->SV_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	} else {
		level.voteNo++;
		ent->client->pers.vote = 2;
		trap->SV_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
	}
}

/*
=================
ClientCommand
=================
*/
#define CMDFLAG_NOINTERMISSION	0x0001
#define CMDFLAG_CHEAT			0x0002
#define CMDFLAG_ALIVE			0x0004

#define CMDFAIL_NOINTERMISSION	0x0001
#define CMDFAIL_CHEAT			0x0002
#define CMDFAIL_ALIVE			0x0004
#define CMDFAIL_GAMETYPE		0x0008

typedef struct command_s {
	const char		*name;
	void			(*func)(gentity_t *ent);
	unsigned int	validGT;	// bit-flag of valid gametypes
	unsigned int	flags;
} command_t;

static int cmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((command_t*)b)->name );
}

command_t commands[] = {
	{ "addbot",				Cmd_AddBot_f,				GTB_ALL,					0 },
	{ "callvote",			Cmd_CallVote_f,				GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "drop",				Cmd_Drop_f,					GTB_ALL,					CMDFLAG_ALIVE },
	{ "follow",				Cmd_Follow_f,				GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "follownext",			Cmd_FollowNext_f,			GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "followprev",			Cmd_FollowPrev_f,			GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "give",				Cmd_Give_f,					GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE|CMDFLAG_NOINTERMISSION },
	{ "giveother",			Cmd_GiveOther_f,			GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE|CMDFLAG_NOINTERMISSION },
	{ "god",				Cmd_God_f,					GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE|CMDFLAG_NOINTERMISSION },
	{ "kill",				Cmd_Kill_f,					GTB_ALL,					CMDFLAG_ALIVE|CMDFLAG_NOINTERMISSION },
	{ "noclip",				Cmd_Noclip_f,				GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE|CMDFLAG_NOINTERMISSION },
	{ "ready",				Cmd_Ready_f	,				GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "say",				Cmd_Say_f,					GTB_ALL,					0 },
	{ "say_team",			Cmd_SayTeam_f,				GTB_ALL,					0 },
	{ "score",				Cmd_Score_f,				GTB_ALL,					0 },
	{ "team",				Cmd_Team_f,					GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "tell",				Cmd_Tell_f,					GTB_ALL,					0 },
	{ "t_use",				Cmd_TargetUse_f,			GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE },
//	{ "voice_cmd",			Cmd_VoiceCommand_f,			GTB_ALL & ~(GTB_NOTTEAM),	0 },
	{ "vote",				Cmd_Vote_f,					GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ NULL,					NULL,						GTB_ALL,					0 },
};
static size_t numCommands = ARRAY_LEN( commands );

/*
=================
ClientCommand
=================
*/

//returns the flags that failed to pass
// or 0 if the command is allowed to be executed
unsigned int G_CmdValid( const gentity_t *ent, const command_t *cmd )
{
	if ( (cmd->flags & CMDFLAG_NOINTERMISSION)
		&& level.intermissiontime )
		return CMDFAIL_NOINTERMISSION;

	if ( (cmd->flags & CMDFLAG_CHEAT)
		&& !sv_cheats->integer )
		return CMDFAIL_CHEAT;

	if ( (cmd->flags & CMDFLAG_ALIVE)
		&& (ent->health <= 0 || ent->client->sess.sessionTeam == TEAM_SPECTATOR) )
		return CMDFAIL_ALIVE;

	if ( !(cmd->validGT & (1<<level.gametype)) )
		return CMDFAIL_GAMETYPE;

	return 0;
}

void ClientCommand( int clientNum ) {
	gentity_t	*ent = NULL;
	char		cmd[MAX_TOKEN_CHARS] = {0};
	command_t	*command = NULL;

	ent = g_entities+clientNum;
	if ( !ent->client || ent->client->pers.connected != CON_CONNECTED )
		return; // not fully in game yet

	trap->Cmd_Argv( 0, cmd, sizeof( cmd ) );

	command = (command_t *)bsearch( cmd, commands, numCommands, sizeof( commands[0] ), cmdcmp );
	if ( !command ) {
		trap->SV_GameSendServerCommand( clientNum, va( "print \"Unknown command %s\n\"", cmd ) );
		return;
	}

	switch ( G_CmdValid( ent, command ) )
	{
	case 0:
		command->func( ent );
		break;

	case CMDFAIL_NOINTERMISSION:
		trap->SV_GameSendServerCommand( clientNum, va( "print \"Can't use %s in intermission\n\"", cmd ) );
		break;
	case CMDFAIL_CHEAT:
		trap->SV_GameSendServerCommand( clientNum, "print \"Cheats are not enabled\n\"" );
		break;
	case CMDFAIL_ALIVE:
		trap->SV_GameSendServerCommand( clientNum, va( "print \"You must be alive to use %s\n\"", cmd ) );
		break;
	case CMDFAIL_GAMETYPE:
		trap->SV_GameSendServerCommand( clientNum, va( "print \"%s is not applicable in this gametype\n\"", cmd ) );
		break;
	default:
		break;
	}
}

void G_PrintCommands( gentity_t *ent )
{
	const command_t *command = NULL;
	char buf[256] = {0};
	int toggle = 0;
	unsigned int count = 0;
	const unsigned int limit = 72;

	Q_strcat( buf, sizeof( buf ), "Regular commands:\n   " );

	for ( command=commands; command && command->name; command++ ) {
		char *tmpMsg = NULL;

		// if it's not allowed to be executed at the moment, continue
		if ( G_CmdValid( ent, command ) )
			continue;

		tmpMsg = va( " ^%c%s", (++toggle&1?COLOR_GREEN:COLOR_YELLOW), command->name );

		//newline if we reach limit
		if ( count >= limit ) {
			tmpMsg = va( "\n   %s", tmpMsg );
			count = 0;
		}

		if ( strlen( buf ) + strlen( tmpMsg ) >= sizeof( buf ) )
		{
			trap->SV_GameSendServerCommand( ent-g_entities, va( "print \"%s\"", buf ) );
			buf[0] = '\0';
		}
		count += strlen( tmpMsg );
		Q_strcat( buf, sizeof( buf ), tmpMsg );
	}

	trap->SV_GameSendServerCommand( ent-g_entities, va( "print \"%s\n\n\"", buf ) );
}
