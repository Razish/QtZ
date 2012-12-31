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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sv_local.h"

serverImport_t svi;

/*
===============
SV_SendConfigstring

Creates and sends the server command necessary to update the CS index for the
given client
===============
*/
static void SV_SendConfigstring(client_t *client, int index)
{
	int maxChunkSize = MAX_STRING_CHARS - 24;
	int len;

	len = strlen(sv.configstrings[index]);

	if( len >= maxChunkSize ) {
		int		sent = 0;
		int		remaining = len;
		char	*cmd;
		char	buf[MAX_STRING_CHARS];

		while (remaining > 0 ) {
			if ( sent == 0 ) {
				cmd = "bcs0";
			}
			else if( remaining < maxChunkSize ) {
				cmd = "bcs2";
			}
			else {
				cmd = "bcs1";
			}
			Q_strncpyz( buf, &sv.configstrings[index][sent],
				maxChunkSize );

			SV_SendServerCommand( client, "%s %i \"%s\"\n", cmd,
				index, buf );

			sent += (maxChunkSize - 1);
			remaining -= (maxChunkSize - 1);
		}
	} else {
		// standard cs, just send it
		SV_SendServerCommand( client, "cs %i \"%s\"\n", index,
			sv.configstrings[index] );
	}
}

/*
===============
SV_UpdateConfigstrings

Called when a client goes from CS_PRIMED to CS_ACTIVE.  Updates all
Configstring indexes that have changed while the client was in CS_PRIMED
===============
*/
void SV_UpdateConfigstrings(client_t *client)
{
	int index;

	for( index = 0; index <= MAX_CONFIGSTRINGS; index++ ) {
		// if the CS hasn't changed since we went to CS_PRIMED, ignore
		if(!client->csUpdated[index])
			continue;

		// do not always send server info to all clients
		if ( index == CS_SERVERINFO && client->gentity &&
			(client->gentity->r.svFlags & SVF_NOSERVERINFO) ) {
			continue;
		}
		SV_SendConfigstring(client, index);
		client->csUpdated[index] = qfalse;
	}
}

/*
===============
SV_SetConfigstring

===============
*/
void SV_SetConfigstring (int index, const char *val) {
	int		i;
	client_t	*client;

	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		svi.Error (ERR_DROP, "SV_SetConfigstring: bad index %i", index);
	}

	if ( !val ) {
		val = "";
	}

	// don't bother broadcasting an update if no change
	if ( !strcmp( val, sv.configstrings[ index ] ) ) {
		return;
	}

	// change the string in sv
	svi.Z_Free( sv.configstrings[index] );
	sv.configstrings[index] = CopyString( val );

	// send it to all the clients if we aren't
	// spawning a new server
	if ( sv.state == SS_GAME || sv.restarting ) {

		// send the data to all relevent clients
		for (i = 0, client = svs.clients; i < sv_maxclients->integer ; i++, client++) {
			if ( client->state < CS_ACTIVE ) {
				if ( client->state == CS_PRIMED )
					client->csUpdated[ index ] = qtrue;
				continue;
			}
			// do not always send server info to all clients
			if ( index == CS_SERVERINFO && client->gentity && (client->gentity->r.svFlags & SVF_NOSERVERINFO) ) {
				continue;
			}
		
			SV_SendConfigstring(client, index);
		}
	}
}

/*
===============
SV_GetConfigstring

===============
*/
void SV_GetConfigstring( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		svi.Error( ERR_DROP, "SV_GetConfigstring: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		svi.Error (ERR_DROP, "SV_GetConfigstring: bad index %i", index);
	}
	if ( !sv.configstrings[index] ) {
		buffer[0] = 0;
		return;
	}

	Q_strncpyz( buffer, sv.configstrings[index], bufferSize );
}


/*
===============
SV_SetUserinfo

===============
*/
void SV_SetUserinfo( int index, const char *val ) {
	if ( index < 0 || index >= sv_maxclients->integer ) {
		svi.Error (ERR_DROP, "SV_SetUserinfo: bad index %i", index);
	}

	if ( !val ) {
		val = "";
	}

	Q_strncpyz( svs.clients[index].userinfo, val, sizeof( svs.clients[ index ].userinfo ) );
	Q_strncpyz( svs.clients[index].name, Info_ValueForKey( val, "name" ), sizeof(svs.clients[index].name) );
}



/*
===============
SV_GetUserinfo

===============
*/
void SV_GetUserinfo( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		svi.Error( ERR_DROP, "SV_GetUserinfo: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= sv_maxclients->integer ) {
		svi.Error (ERR_DROP, "SV_GetUserinfo: bad index %i", index);
	}
	Q_strncpyz( buffer, svs.clients[ index ].userinfo, bufferSize );
}


/*
================
SV_CreateBaseline

Entity baselines are used to compress non-delta messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
static void SV_CreateBaseline( void ) {
	sharedEntity_t *svent;
	int				entnum;	

	for ( entnum = 1; entnum < sv.num_entities ; entnum++ ) {
		svent = SV_GentityNum(entnum);
		if (!svent->r.linked) {
			continue;
		}
		svent->s.number = entnum;

		//
		// take current state as baseline
		//
		sv.svEntities[entnum].baseline = svent->s;
	}
}


/*
===============
SV_BoundMaxClients

===============
*/
static void SV_BoundMaxClients( int minimum ) {
	// get the current maxclients value
	svi.Cvar_Get( "sv_maxclients", "12", 0, NULL );

	sv_maxclients->modified = qfalse;

	if ( sv_maxclients->integer < minimum ) {
		svi.Cvar_Set( "sv_maxclients", va("%i", minimum) );
	} else if ( sv_maxclients->integer > MAX_CLIENTS ) {
		svi.Cvar_Set( "sv_maxclients", va("%i", MAX_CLIENTS) );
	}
}


/*
===============
SV_Startup

Called when a host starts a map when it wasn't running
one before.  Successive map or map_restart commands will
NOT cause this to be called, unless the game is exited to
the menu system first.
===============
*/
static void SV_Startup( void ) {
	if ( svs.initialized ) {
		svi.Error( ERR_FATAL, "SV_Startup: svs.initialized" );
	}
	SV_BoundMaxClients( 1 );

	svs.clients = svi.Z_Malloc (sizeof(client_t) * sv_maxclients->integer );
	if ( com_dedicated->integer ) {
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * 64;
	} else {
		// we don't need nearly as many when playing locally
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * 64;
	}
	svs.initialized = qtrue;

	// Don't respect sv_killserver unless a server is actually running
	if ( sv_killserver->integer ) {
		svi.Cvar_Set( "sv_killserver", "0" );
	}

	svi.Cvar_Set( "sv_running", "1" );
	
	// Join the ipv6 multicast group now that a map is running so clients can scan for us on the local network.
	svi.NET_JoinMulticast6();
}


/*
==================
SV_ChangeMaxClients
==================
*/
void SV_ChangeMaxClients( void ) {
	int		oldMaxClients;
	int		i;
	client_t	*oldClients;
	int		count;

	// get the highest client number in use
	count = 0;
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			if (i > count)
				count = i;
		}
	}
	count++;

	oldMaxClients = sv_maxclients->integer;
	// never go below the highest client number in use
	SV_BoundMaxClients( count );
	// if still the same
	if ( sv_maxclients->integer == oldMaxClients ) {
		return;
	}

	oldClients = svi.Hunk_AllocateTempMemory( count * sizeof(client_t) );
	// copy the clients to hunk memory
	for ( i = 0 ; i < count ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			oldClients[i] = svs.clients[i];
		}
		else {
			Com_Memset(&oldClients[i], 0, sizeof(client_t));
		}
	}

	// free old clients arrays
	svi.Z_Free( svs.clients );

	// allocate new clients
	svs.clients = svi.Z_Malloc ( sv_maxclients->integer * sizeof(client_t) );
	Com_Memset( svs.clients, 0, sv_maxclients->integer * sizeof(client_t) );

	// copy the clients over
	for ( i = 0 ; i < count ; i++ ) {
		if ( oldClients[i].state >= CS_CONNECTED ) {
			svs.clients[i] = oldClients[i];
		}
	}

	// free the old clients on the hunk
	svi.Hunk_FreeTempMemory( oldClients );
	
	// allocate new snapshot entities
	if ( com_dedicated->integer ) {
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * 64;
	} else {
		// we don't need nearly as many when playing locally
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * 64;
	}
}

/*
================
SV_ClearServer
================
*/
static void SV_ClearServer(void) {
	int i;

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( sv.configstrings[i] ) {
			svi.Z_Free( sv.configstrings[i] );
		}
	}
	Com_Memset (&sv, 0, sizeof(sv));
}

/*
================
SV_TouchCGame

  touch the cgame.vm so that a pure client can load it if it's in a seperate pk3
================
*/
static void SV_TouchCGame(void) {
	fileHandle_t	f;
	char filename[MAX_QPATH];

	Com_sprintf( filename, sizeof(filename), "vm/%s.qvm", "cgame" );
	svi.FS_FOpenFileRead( filename, &f, qfalse );
	if ( f ) {
		svi.FS_FCloseFile( f );
	}
}

/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
This is NOT called for map_restart
================
*/
void SV_SpawnServer( char *server, qboolean killBots ) {
	int			i;
	int			checksum;
	qboolean	isBot;
	char		systemInfo[16384];
	const char	*p;

	// shut down the existing game if it is running
	SV_ShutdownGameProgs();

	Com_Printf ("------ Server Initialization ------\n");
	Com_Printf ("Server: %s\n",server);

	// if not running a dedicated server CL_MapLoading will connect the client to the server
	// also print some status stuff
	svi.CL_MapLoading();

	// make sure all the client stuff is unloaded
	svi.CL_ShutdownAll(qfalse);

	// clear the whole hunk because we're (re)loading the server
	svi.Hunk_Clear();

#ifndef DEDICATED
	// Restart renderer
	svi.CL_StartHunkUsers( qtrue );
#endif

	// clear collision map data
	svi.CM_ClearMap();

	// init client structures and svs.numSnapshotEntities 
	if ( !svi.Cvar_VariableValue("sv_running") ) {
		SV_Startup();
	} else {
		// check for maxclients change
		if ( sv_maxclients->modified ) {
			SV_ChangeMaxClients();
		}
	}

	// clear pak references
	svi.FS_ClearPakReferences(0);

	// allocate the snapshot entities on the hunk
	svs.snapshotEntities = svi.Hunk_Alloc( sizeof(entityState_t)*svs.numSnapshotEntities, h_high );
	svs.nextSnapshotEntities = 0;

	// toggle the server bit so clients can detect that a
	// server has changed
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// set nextmap to the same map, but it may be overriden
	// by the game startup or another console command
	svi.Cvar_Set( "nextmap", "map_restart 0");
//	svi.Cvar_Set( "nextmap", va("map %s", server) );

	for (i=0 ; i<sv_maxclients->integer ; i++) {
		// save when the server started for each client already connected
		if (svs.clients[i].state >= CS_CONNECTED) {
			svs.clients[i].oldServerTime = sv.time;
		}
	}

	// wipe the entire per-level structure
	SV_ClearServer();
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		sv.configstrings[i] = CopyString("");
	}

	// make sure we are not paused
	svi.Cvar_Set("cl_paused", "0");

	// get a new checksum feed and restart the file system
	sv.checksumFeed = ( ((int) rand() << 16) ^ rand() ) ^ svi.Com_Milliseconds();
	svi.FS_Restart( sv.checksumFeed );

	svi.CM_LoadMap( va("maps/%s.bsp", server), qfalse, &checksum );

	// set serverinfo visible name
	svi.Cvar_Set( "mapname", server );

	svi.Cvar_Set( "sv_mapChecksum", va("%i",checksum) );

	// serverid should be different each time
	sv.serverId = (*svi.com_frameTime);
	sv.restartedServerId = sv.serverId; // I suppose the init here is just to be safe
	sv.checksumFeedServerId = sv.serverId;
	svi.Cvar_Set( "sv_serverid", va("%i", sv.serverId ) );

	// clear physics interaction links
	SV_ClearWorld ();
	
	// media configstring setting should be done during
	// the loading stage, so connected clients don't have
	// to load during actual gameplay
	sv.state = SS_LOADING;

	// load and spawn all other entities
	SV_InitGameProgs();

	// don't allow a map_restart if game is modified
	sv_gametype->modified = qfalse;

	// run a few frames to allow everything to settle
	for (i = 0;i < 3; i++)
	{
		svi.VM_Call (gvm, GAME_RUN_FRAME, sv.time);
		SV_BotFrame (sv.time);
		sv.time += 100;
		svs.time += 100;
	}

	// create a baseline for more efficient communications
	SV_CreateBaseline ();

	for (i=0 ; i<sv_maxclients->integer ; i++) {
		// send the new gamestate to all connected clients
		if (svs.clients[i].state >= CS_CONNECTED) {
			char	*denied;

			if ( svs.clients[i].netchan.remoteAddress.type == NA_BOT ) {
				if ( killBots ) {
					SV_DropClient( &svs.clients[i], "" );
					continue;
				}
				isBot = qtrue;
			}
			else {
				isBot = qfalse;
			}

			// connect the client again
			denied = svi.VM_ExplicitArgPtr( gvm, svi.VM_Call( gvm, GAME_CLIENT_CONNECT, i, qfalse, isBot ) );	// firstTime = qfalse
			if ( denied ) {
				// this generally shouldn't happen, because the client
				// was connected before the level change
				SV_DropClient( &svs.clients[i], denied );
			} else {
				if( !isBot ) {
					// when we get the next packet from a connected client,
					// the new gamestate will be sent
					svs.clients[i].state = CS_CONNECTED;
				}
				else {
					client_t		*client;
					sharedEntity_t	*ent;

					client = &svs.clients[i];
					client->state = CS_ACTIVE;
					ent = SV_GentityNum( i );
					ent->s.number = i;
					client->gentity = ent;

					client->deltaMessage = -1;
					client->lastSnapshotTime = 0;	// generate a snapshot immediately

					svi.VM_Call( gvm, GAME_CLIENT_BEGIN, i );
				}
			}
		}
	}	

	// run another frame to allow things to look at all the players
	svi.VM_Call (gvm, GAME_RUN_FRAME, sv.time);
	SV_BotFrame (sv.time);
	sv.time += 100;
	svs.time += 100;

	if ( sv_pure->integer ) {
		// the server sends these to the clients so they will only
		// load pk3s also loaded at the server
		p = svi.FS_LoadedPakChecksums();
		svi.Cvar_Set( "sv_paks", p );
		if (strlen(p) == 0) {
			Com_Printf( "WARNING: sv_pure set but no PK3 files loaded\n" );
		}
		p = svi.FS_LoadedPakNames();
		svi.Cvar_Set( "sv_pakNames", p );

		// if a dedicated pure server we need to touch the cgame because it could be in a
		// seperate pk3 file and the client will need to load the latest cgame.qvm
		if ( com_dedicated->integer ) {
			SV_TouchCGame();
		}
	}
	else {
		svi.Cvar_Set( "sv_paks", "" );
		svi.Cvar_Set( "sv_pakNames", "" );
	}
	// the server sends these to the clients so they can figure
	// out which pk3s should be auto-downloaded
	p = svi.FS_ReferencedPakChecksums();
	svi.Cvar_Set( "sv_referencedPaks", p );
	p = svi.FS_ReferencedPakNames();
	svi.Cvar_Set( "sv_referencedPakNames", p );

	// save systeminfo and serverinfo strings
	Q_strncpyz( systemInfo, svi.Cvar_InfoString_Big( CVAR_SYSTEMINFO ), sizeof( systemInfo ) );
	(*svi.cvar_modifiedFlags) &= ~CVAR_SYSTEMINFO;
	SV_SetConfigstring( CS_SYSTEMINFO, systemInfo );

	SV_SetConfigstring( CS_SERVERINFO, svi.Cvar_InfoString( CVAR_SERVERINFO ) );
	(*svi.cvar_modifiedFlags) &= ~CVAR_SERVERINFO;

	// any media configstring setting now should issue a warning
	// and any configstring changes should be reliably transmitted
	// to all clients
	sv.state = SS_GAME;

	// send a heartbeat now so the master will get up to date info
	SV_Heartbeat_f();

	svi.Hunk_SetMark();

	Com_Printf ("-----------------------------------\n");
}

/*
===============
SV_Init

Only called at main exe startup, not for each game
===============
*/

// from common, should be read-only
cvar_t *cl_paused;
cvar_t *com_cl_running;
cvar_t *com_dedicated;
cvar_t *com_gamename;
cvar_t *com_protocol;
cvar_t *com_speeds;
cvar_t *com_sv_running;
cvar_t *com_timescale;
cvar_t *sv_paused;
// end common

Q_EXPORT serverExport_t* QDECL GetServerAPI ( int apiVersion, serverImport_t *import )
{
	static serverExport_t sve;
	int index;

	svi = *import;

	// serverinfo vars
	sv_gametype				= svi.Cvar_Get( "g_gametype",					"0",				CVAR_SERVERINFO|CVAR_LATCH,		NULL );
							  svi.Cvar_Get( "sv_keywords",					"",					CVAR_SERVERINFO,				NULL );
	sv_mapname				= svi.Cvar_Get( "mapname",						"nomap",			CVAR_SERVERINFO|CVAR_ROM,		NULL );
	sv_privateClients		= svi.Cvar_Get( "sv_privateClients",			"0",				CVAR_SERVERINFO,				NULL );
	sv_hostname				= svi.Cvar_Get( "sv_hostname",					"noname",			CVAR_SERVERINFO|CVAR_ARCHIVE,	NULL );
	sv_maxclients			= svi.Cvar_Get( "sv_maxclients",				"8",				CVAR_SERVERINFO|CVAR_LATCH,		NULL );

	sv_minRate				= svi.Cvar_Get( "sv_minRate",					"0",				CVAR_ARCHIVE|CVAR_SERVERINFO,	NULL );
	sv_maxRate				= svi.Cvar_Get( "sv_maxRate",					"0",				CVAR_ARCHIVE|CVAR_SERVERINFO,	NULL );
	sv_dlRate				= svi.Cvar_Get( "sv_dlRate",					"100",				CVAR_ARCHIVE|CVAR_SERVERINFO,	NULL );
	sv_minPing				= svi.Cvar_Get( "sv_minPing",					"0",				CVAR_ARCHIVE|CVAR_SERVERINFO,	NULL );
	sv_maxPing				= svi.Cvar_Get( "sv_maxPing",					"0",				CVAR_ARCHIVE|CVAR_SERVERINFO,	NULL );
	sv_floodProtect			= svi.Cvar_Get( "sv_floodProtect",				"1",				CVAR_ARCHIVE|CVAR_SERVERINFO,	NULL );

	// systeminfo
							  svi.Cvar_Get( "sv_cheats",					"1",				CVAR_SYSTEMINFO|CVAR_ROM,		NULL );
	sv_serverid				= svi.Cvar_Get( "sv_serverid",					"0",				CVAR_SYSTEMINFO|CVAR_ROM,		NULL );
	sv_pure					= svi.Cvar_Get( "sv_pure",						"1",				CVAR_SYSTEMINFO,				NULL );
#ifdef USE_VOIP
	sv_voip					= svi.Cvar_Get( "sv_voip",						"1",				CVAR_SYSTEMINFO|CVAR_LATCH,		NULL );
	svi.Cvar_CheckRange(sv_voip, 0, 1, qtrue);
#endif
							  svi.Cvar_Get( "sv_paks",						"",					CVAR_SYSTEMINFO|CVAR_ROM,		NULL );
							  svi.Cvar_Get( "sv_pakNames",					"",					CVAR_SYSTEMINFO|CVAR_ROM,		NULL );
							  svi.Cvar_Get( "sv_referencedPaks",			"",					CVAR_SYSTEMINFO|CVAR_ROM,		NULL );
							  svi.Cvar_Get( "sv_referencedPakNames",		"",					CVAR_SYSTEMINFO|CVAR_ROM,		NULL );

	// server vars
	sv_rconPassword			= svi.Cvar_Get( "rconPassword",					"",					CVAR_TEMP,						NULL );
	sv_privatePassword		= svi.Cvar_Get( "sv_privatePassword",			"",					CVAR_TEMP,						NULL );
	sv_fps					= svi.Cvar_Get( "sv_fps",						"40",				CVAR_ARCHIVE,						NULL );
	sv_timeout				= svi.Cvar_Get( "sv_timeout",					"200",				CVAR_TEMP,						NULL );
	sv_zombietime			= svi.Cvar_Get( "sv_zombietime",				"2",				CVAR_TEMP,						NULL );
							  svi.Cvar_Get( "nextmap",						"",					CVAR_TEMP,						NULL );

	sv_allowDownload		= svi.Cvar_Get( "sv_allowDownload",				"0",				CVAR_SERVERINFO,				NULL );
							  svi.Cvar_Get( "sv_dlURL",						"",					CVAR_SERVERINFO|CVAR_ARCHIVE,	NULL );
	
	sv_master[0]			= svi.Cvar_Get( "sv_master1",					MASTER_SERVER_NAME,	CVAR_NONE,						NULL );
	for(index = 1; index < MAX_MASTER_SERVERS; index++)
		sv_master[index]	= svi.Cvar_Get( va( "sv_master%d", index+1 ),	"",					CVAR_ARCHIVE,					NULL );

	sv_reconnectlimit		= svi.Cvar_Get( "sv_reconnectlimit",			"3",				CVAR_NONE,						NULL );
	sv_showloss				= svi.Cvar_Get( "sv_showloss",					"0",				CVAR_NONE,						NULL );
	sv_padPackets			= svi.Cvar_Get( "sv_padPackets",				"0",				CVAR_NONE,						NULL );
	sv_killserver			= svi.Cvar_Get( "sv_killserver",				"0",				CVAR_NONE,						NULL );
	sv_mapChecksum			= svi.Cvar_Get( "sv_mapChecksum",				"",					CVAR_ROM,						NULL );
	sv_lanForceRate			= svi.Cvar_Get( "sv_lanForceRate",				"1",				CVAR_ARCHIVE,					NULL );
#ifndef STANDALONE
	sv_strictAuth			= svi.Cvar_Get( "sv_strictAuth",				"1",				CVAR_ARCHIVE,					NULL );
#endif
	sv_banFile				= svi.Cvar_Get( "sv_banFile",					"serverbans.dat",	CVAR_ARCHIVE,					NULL );


	// from common, should be read-only
	cl_paused				= svi.Cvar_Get( "cl_paused",		"0",						CVAR_ROM,					NULL );
	com_cl_running			= svi.Cvar_Get( "cl_running",		"0",						CVAR_ROM,					NULL );
//#ifdef DEDICATED
//	com_dedicated			= svi.Cvar_Get( "dedicated",		"1",						CVAR_INIT,					NULL);
//	svi.Cvar_CheckRange( com_dedicated, 1, 2, qtrue );
//#else
	com_dedicated			= svi.Cvar_Get( "dedicated",		"0",						CVAR_LATCH,					NULL);
	svi.Cvar_CheckRange( com_dedicated, 0, 2, qtrue );
//#endif
	com_gamename			= svi.Cvar_Get( "com_gamename",		GAMENAME_FOR_MASTER,		CVAR_SERVERINFO|CVAR_INIT,	NULL );
	com_protocol			= svi.Cvar_Get( "com_protocol",		va("%i", PROTOCOL_VERSION),	CVAR_SERVERINFO|CVAR_INIT,	NULL );
	com_speeds				= svi.Cvar_Get( "com_speeds",		"0",						CVAR_NONE,					NULL );
	com_sv_running			= svi.Cvar_Get( "sv_running",		"0",						CVAR_ROM,					NULL );
	com_timescale			= svi.Cvar_Get( "timescale",		"1",						CVAR_CHEAT|CVAR_SYSTEMINFO,	NULL );
	sv_paused				= svi.Cvar_Get( "sv_paused",		"0",						CVAR_ROM,					NULL );
	// end common

	SV_AddOperatorCommands ();

	// initialize bot cvars so they are listed and can be set before loading the botlib
	SV_BotInitCvars();

	// init the botlib here because we need the pre-compiler in the UI
	SV_BotInitBotLib();
	
	// Load saved bans
	svi.Cbuf_AddText("rehashbans\n");

	sve.BotDrawDebugPolygons = BotDrawDebugPolygons;
	sve.Frame = SV_Frame;
	sve.FrameMsec = SV_FrameMsec;
	sve.GameCommand = SV_GameCommand;
	sve.PacketEvent = SV_PacketEvent;
	sve.SendQueuedPackets = SV_SendQueuedPackets;
	sve.Shutdown = SV_Shutdown;
	sve.ShutdownGameProgs = SV_ShutdownGameProgs;

	return &sve;
}


/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage( char *message ) {
	int			i, j;
	client_t	*cl;
	
	// send it twice, ignoring rate
	for ( j = 0 ; j < 2 ; j++ ) {
		for (i=0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++) {
			if (cl->state >= CS_CONNECTED) {
				// don't send a disconnect to a local client
				if ( cl->netchan.remoteAddress.type != NA_LOOPBACK ) {
					SV_SendServerCommand( cl, "print \"%s\n\"\n", message );
					SV_SendServerCommand( cl, "disconnect \"%s\"", message );
				}
				// force a snapshot to be sent
				cl->lastSnapshotTime = 0;
				SV_SendClientSnapshot( cl );
			}
		}
	}
}


/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown( char *finalmsg ) {
	if ( !com_sv_running || !com_sv_running->integer ) {
		return;
	}

	Com_Printf( "----- Server Shutdown (%s) -----\n", finalmsg );

	svi.NET_LeaveMulticast6();

	if ( svs.clients && !(*svi.com_errorEntered) ) {
		SV_FinalMessage( finalmsg );
	}

	SV_RemoveOperatorCommands();
	SV_MasterShutdown();
	SV_ShutdownGameProgs();

	// free current level
	SV_ClearServer();

	// free server static data
	if(svs.clients)
	{
		int index;
		
		for(index = 0; index < sv_maxclients->integer; index++)
			SV_FreeClient(&svs.clients[index]);
		
		svi.Z_Free(svs.clients);
	}
	Com_Memset( &svs, 0, sizeof( svs ) );

	svi.Cvar_Set( "sv_running", "0" );

	Com_Printf( "---------------------------\n" );

	// disconnect any local clients
	if( sv_killserver->integer != 2 )
		svi.CL_Disconnect( qfalse );
}

