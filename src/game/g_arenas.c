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
//
// g_arenas.c
//

#include "g_local.h"


gentity_t	*podium1;
gentity_t	*podium2;
gentity_t	*podium3;

void UpdateTournamentInfo( void ) {
	int			i;
	gentity_t	*player;
	int			playerClientNum;
	int			n, accuracy, perfect;
	int score1, score2;
	qboolean won;
	char		buf[32];
	char		msg[MAX_STRING_CHARS];
	size_t		msglen;

	// find the real player
	player = NULL;
	for (i = 0; i < level.maxclients; i++ ) {
		player = &g_entities[i];
		if ( !player->inuse ) {
			continue;
		}
		if ( !( player->r.svFlags & SVF_BOT ) ) {
			break;
		}
	}
	// this should never happen!
	if ( !player || i == level.maxclients ) {
		return;
	}
	playerClientNum = i;

	CalculateRanks();

	if ( level.clients[playerClientNum].sess.sessionTeam == TEAM_SPECTATOR ) {
		Com_sprintf( msg, sizeof(msg), "postgame %i %i 0 0 0 0 0 0 0 0 0 0 0", level.numNonSpectatorClients, playerClientNum );
	}
	else {
		if( player->client->accuracy_shots ) {
			accuracy = player->client->accuracy_hits * 100 / player->client->accuracy_shots;
		}
		else {
			accuracy = 0;
		}
		won = qfalse;
		if (level.gametype >= GT_FLAGS) {
			score1 = level.teamScores[TEAM_RED];
			score2 = level.teamScores[TEAM_BLUE];
			if (level.clients[playerClientNum].sess.sessionTeam	== TEAM_RED) {
				won = (level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE]);
			} else {
				won = (level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED]);
			}
		} else {
			if (&level.clients[playerClientNum] == &level.clients[ level.sortedClients[0] ]) {
				won = qtrue;
				score1 = level.clients[ level.sortedClients[0] ].ps.persistent[PERS_SCORE];
				score2 = level.clients[ level.sortedClients[1] ].ps.persistent[PERS_SCORE];
			} else {
				score2 = level.clients[ level.sortedClients[0] ].ps.persistent[PERS_SCORE];
				score1 = level.clients[ level.sortedClients[1] ].ps.persistent[PERS_SCORE];
			}
		}
		if (won && player->client->ps.persistent[PERS_KILLED] == 0) {
			perfect = 1;
		} else {
			perfect = 0;
		}
		Com_sprintf( msg, sizeof(msg), "postgame %i %i %i %i %i %i %i %i %i %i %i %i %i", level.numNonSpectatorClients, playerClientNum, accuracy,
			player->client->ps.persistent[PERS_IMPRESSIVE_COUNT], player->client->ps.persistent[PERS_EXCELLENT_COUNT],player->client->ps.persistent[PERS_DEFEND_COUNT],
			player->client->ps.persistent[PERS_ASSIST_COUNT], player->client->ps.persistent[PERS_SCORE],
			perfect, score1, score2, level.time, player->client->ps.persistent[PERS_CAPTURES] );

	}

	msglen = strlen( msg );
	for( i = 0; i < level.numNonSpectatorClients; i++ ) {
		n = level.sortedClients[i];
		Com_sprintf( buf, sizeof(buf), " %i %i %i", n, level.clients[n].ps.persistent[PERS_RANK], level.clients[n].ps.persistent[PERS_SCORE] );
		msglen += strlen( buf );
		if( msglen >= sizeof(msg) ) {
			break;
		}
		strcat( msg, buf );
	}
	trap->Cbuf_ExecuteText( EXEC_APPEND, msg );
}
