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

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"


/*

	PACKET FILTERING
 

	You can add or remove addresses from the filter list with:
		addip <ip>
		removeip <ip>

	The ip address is specified in dot format, and you can use '*' to match any value
	so you can specify an entire class C network with "addip 192.246.40.*"

	Removeip will only remove an address specified exactly the same way.
	You cannot addip a subnet, then removeip a single host.

	listip -- Prints the current list of filters.

	g_filterban <0 or 1>
	If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.
	If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.

	TTimo NOTE: for persistence, bans are stored in g_banIPs cvar MAX_CVAR_VALUE_STRING
	The size of the cvar string buffer is limiting the banning to around 20 masks
	this could be improved by putting some g_banIPs2 g_banIps3 etc. maybe
	still, you should rely on PB for banning instead

*/

typedef struct ipFilter_s {
	uint32_t mask, compare;
} ipFilter_t;

#define	MAX_IPFILTERS	1024

static ipFilter_t	ipFilters[MAX_IPFILTERS];
static int			numIPFilters;

static qboolean StringToFilter (char *s, ipFilter_t *f) {
	char num[128];
	int i, j;
	byteAlias_t b, m;

	b.i = m.i = 0;

	for ( i=0; i<4; i++ ) {
		if ( *s < '0' || *s > '9' ) {
			if ( *s == '*' ) {
				// 'match any'
				// b[i] and m[i] to 0
				if ( !(*++s) )
					break;

				s++;
				continue;
			}
			trap->Print( "Bad filter address: %s\n", s );
			return qfalse;
		}
		
		j = 0;
		while ( *s >= '0' && *s <= '9' )
			num[j++] = *s++;

		num[j] = 0;
		b.c[i] = (char)atoi( num );
		m.b[i] = 0xFF;

		if ( !*s )
			break;
		s++;
	}
	
	f->mask = m.ui;
	f->compare = b.ui;

	return qtrue;
}

static void UpdateIPBans( void ) {
	byteAlias_t b, m;
	int i, j;
	char ip[64], iplist_final[MAX_CVAR_VALUE_STRING];

	*iplist_final = 0;
	for ( i=0; i<numIPFilters; i++ ) {
		if ( ipFilters[i].compare == 0xFFFFFFFFu )
			continue;

		b.ui = ipFilters[i].compare;
		m.ui = ipFilters[i].mask;
		*ip = 0;
		for ( j=0; j<4; j++ ) {
			if ( m.b[j] != 0xFF )
				Q_strcat( ip, sizeof( ip ), "*" );
			else
				Q_strcat( ip, sizeof( ip ), va( "%i", (int)b.c[j] ) );
			Q_strcat( ip, sizeof( ip ), (j<3) ? "." : " " );
		}		
		if ( strlen( iplist_final )+strlen( ip ) < MAX_CVAR_VALUE_STRING )
			Q_strcat( iplist_final, sizeof( iplist_final ), ip );
		else {
			Com_Printf( "g_banIPs overflowed at MAX_CVAR_VALUE_STRING\n" );
			break;
		}
	}

	trap->Cvar_Set( "g_banIPs", iplist_final );
}

qboolean G_FilterPacket( char *from ) {
	int i;
	uint32_t in;
	byteAlias_t m;
	char *p;

	if ( !g_filterBan->boolean )
		return qfalse;

	i = 0;
	p = from;
	while ( *p && i < 4 ) {
		m.b[i] = 0;
		while ( *p >= '0' && *p <= '9' ) {
			m.b[i] = m.b[i]*10 + (*p - '0');
			p++;
		}
		if ( !*p || *p == ':' )
			break;
		i++, p++;
	}
	
	in = m.ui;

	for ( i=0; i<numIPFilters; i++ ) {
		if ( (in & ipFilters[i].mask) == ipFilters[i].compare )
			return qtrue;
	}

	return qfalse;
}

static void AddIP( char *str ) {
	int i;

	for ( i=0; i<numIPFilters; i++ ) {
		if ( ipFilters[i].compare == 0xFFFFFFFFu )
			break; // free spot
	}
	if ( i == numIPFilters ) {
		if ( numIPFilters == MAX_IPFILTERS ) {
			trap->Print( "IP filter list is full\n" );
			return;
		}
		numIPFilters++;
	}
	
	if ( !StringToFilter( str, &ipFilters[i] ) )
		ipFilters[i].compare = 0xFFFFFFFFu;

	UpdateIPBans();
}

void G_ProcessIPBans( void ) {
	char *s = NULL, *t = NULL, str[MAX_CVAR_VALUE_STRING] = {0};

	Q_strncpyz( str, g_banIPs->string, sizeof( str ) );

	for ( t=s=g_banIPs->string; *t; t=s ) {
		s = strchr( s, ' ' );
		if ( !s )
			break;

		while ( *s == ' ' )
			*s++ = 0;

		if ( *t )
			AddIP( t );
	}
}

void Svcmd_AddIP_f( void ) {
	char str[MAX_TOKEN_CHARS];

	if ( trap->Cmd_Argc() < 2 ) {
		trap->Print( "Usage: addip <ip-mask>\n" );
		return;
	}

	trap->Cmd_Argv( 1, str, sizeof( str ) );

	AddIP( str );
}

void Svcmd_RemoveIP_f( void ) {
	ipFilter_t	f;
	int			i;
	char		str[MAX_TOKEN_CHARS];

	if ( trap->Cmd_Argc() < 2 ) {
		trap->Print("Usage: removeip <ip-mask>\n");
		return;
	}

	trap->Cmd_Argv( 1, str, sizeof( str ) );

	if (!StringToFilter (str, &f))
		return;

	for ( i=0; i<numIPFilters; i++ ) {
		if ( ipFilters[i].mask == f.mask && ipFilters[i].compare == f.compare ) {
			ipFilters[i].compare = 0xFFFFFFFFu;
			trap->Print( "Removed.\n" );

			UpdateIPBans();
			return;
		}
	}

	trap->Print( "Didn't find %s.\n", str );
}

void	Svcmd_EntityList_f( void ) {
	int			e;
	gentity_t		*check;

	check = g_entities+1;
	for (e = 1; e < level.num_entities ; e++, check++) {
		if ( !check->inuse ) {
			continue;
		}
		trap->Print("%3i:", e);
		switch ( check->s.eType ) {
		case ET_GENERAL:
			trap->Print("ET_GENERAL          ");
			break;
		case ET_PLAYER:
			trap->Print("ET_PLAYER           ");
			break;
		case ET_ITEM:
			trap->Print("ET_ITEM             ");
			break;
		case ET_MISSILE:
			trap->Print("ET_MISSILE          ");
			break;
		case ET_MOVER:
			trap->Print("ET_MOVER            ");
			break;
		case ET_BEAM:
			trap->Print("ET_BEAM             ");
			break;
		case ET_PORTAL:
			trap->Print("ET_PORTAL           ");
			break;
		case ET_SPEAKER:
			trap->Print("ET_SPEAKER          ");
			break;
		case ET_PUSH_TRIGGER:
			trap->Print("ET_PUSH_TRIGGER     ");
			break;
		case ET_TELEPORT_TRIGGER:
			trap->Print("ET_TELEPORT_TRIGGER ");
			break;
		case ET_INVISIBLE:
			trap->Print("ET_INVISIBLE        ");
			break;
		default:
			trap->Print("%3i                 ", check->s.eType);
			break;
		}

		if ( check->classname ) {
			trap->Print("%s", check->classname);
		}
		trap->Print("\n");
	}
}

gclient_t	*ClientForString( const char *s ) {
	gclient_t	*cl;
	int			i;
	int			idnum;

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			Com_Printf( "Bad client slot: %i\n", idnum );
			return NULL;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			trap->Print( "Client %i is not connected\n", idnum );
			return NULL;
		}
		return cl;
	}

	// check for a name match
	for ( i=0 ; i < level.maxclients ; i++ ) {
		cl = &level.clients[i];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !Q_stricmp( cl->pers.netname, s ) ) {
			return cl;
		}
	}

	trap->Print( "User %s is not on the server\n", s );

	return NULL;
}

// forceteam <player> <team>
void	Svcmd_ForceTeam_f( void ) {
	gclient_t	*cl;
	char		str[MAX_TOKEN_CHARS];

	if ( trap->Cmd_Argc() < 3 ) {
		trap->Print("Usage: forceteam <player> <team>\n");
		return;
	}

	// find the player
	trap->Cmd_Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );
	if ( !cl ) {
		return;
	}

	// set the team
	trap->Cmd_Argv( 2, str, sizeof( str ) );
	SetTeam( &g_entities[cl - level.clients], str );
}

char	*ConcatArgs( int start );

qboolean	ConsoleCommand( void ) {
	char	cmd[MAX_TOKEN_CHARS];

	trap->Cmd_Argv( 0, cmd, sizeof( cmd ) );

	if ( !Q_stricmp( cmd, "entitylist" ) ) {
		Svcmd_EntityList_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "forceteam" ) ) {
		Svcmd_ForceTeam_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "game_memory" ) ) {
		Svcmd_GameMem_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "addbot" ) ) {
		Svcmd_AddBot_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "botlist" ) ) {
		Svcmd_BotList_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "addip" ) ) {
		Svcmd_AddIP_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "removeip" ) ) {
		Svcmd_RemoveIP_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "listip" ) ) {
		trap->Cbuf_ExecuteText( EXEC_NOW, "g_banIPs\n" );
		return qtrue;
	}

	//OSP: pause
	if ( !Q_stricmp( cmd, "pause" ) ) {
		if ( level.pause.state == PAUSE_NONE ) {
			level.pause.state = PAUSE_PAUSED;
			level.pause.time = level.time + g_pauseTime->integer*1000;
		}
		else if ( level.pause.state == PAUSE_PAUSED ) {
			level.pause.state = PAUSE_UNPAUSING;
			level.pause.time = level.time + g_unpauseTime->integer*1000;
		}
		return qtrue;
	}

	if ( dedicated->boolean ) {
		if ( !Q_stricmp( cmd, "say" ) ) {
			trap->SV_GameSendServerCommand( -1, va( "print \"server: %s\n\"", ConcatArgs( 1 ) ) );
			return qtrue;
		}
	}

	return qfalse;
}

