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

// g_client.c -- client functions that don't happen every frame

static const vector3 playerMins = { MINS_X, MINS_Y, MINS_Z };
static const vector3 playerMaxs = { MAXS_X, MAXS_Y, MAXS_Z };

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent ) {

}

qboolean SpotWouldTelefrag( gentity_t *spot ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vector3		mins, maxs;

	VectorAdd( &spot->s.origin, &playerMins, &mins );
	VectorAdd( &spot->s.origin, &playerMaxs, &maxs );
	num = trap->SV_AreaEntities( &mins, &maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->client) {
			return qtrue;
		}

	}

	return qfalse;
}

#define	MAX_SPAWN_POINTS (128)

// Find the spot that we DON'T want to use
gentity_t *SelectNearestDeathmatchSpawnPoint( vector3 *from ) {
	gentity_t	*spot;
	vector3		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract( &spot->s.origin, from, &delta );
		dist = VectorLength( &delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}

// go to a random point that doesn't telefrag
gentity_t *SelectRandomDeathmatchSpawnPoint(qboolean isbot) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL && count < MAX_SPAWN_POINTS)
	{
		if(SpotWouldTelefrag(spot))
			continue;

		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
		{
			// spot is not for this human/bot player
			continue;
		}

		spots[count] = spot;
		count++;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[ selection ];
}

// Chooses a player start, deathmatch start, etc
gentity_t *SelectRandomFurthestSpawnPoint ( vector3 *avoidPoint, vector3 *origin, vector3 *angles, qboolean isbot ) {
	gentity_t	*spot;
	vector3		delta;
	float		dist;
	float		list_dist[MAX_SPAWN_POINTS];
	gentity_t	*list_spot[MAX_SPAWN_POINTS];
	int			numSpots, rnd, i, j;

	numSpots = 0;
	spot = NULL;

	while((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		if(SpotWouldTelefrag(spot))
			continue;

		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
		{
			// spot is not for this human/bot player
			continue;
		}

		VectorSubtract( &spot->s.origin, avoidPoint, &delta );
		dist = VectorLength( &delta );

		for (i = 0; i < numSpots; i++)
		{
			if(dist > list_dist[i])
			{
				if (numSpots >= MAX_SPAWN_POINTS)
					numSpots = MAX_SPAWN_POINTS - 1;
					
				for(j = numSpots; j > i; j--)
				{
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}
				
				list_dist[i] = dist;
				list_spot[i] = spot;
				
				numSpots++;
				break;
			}
		}
		
		if(i >= numSpots && numSpots < MAX_SPAWN_POINTS)
		{
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	
	if(!numSpots)
	{
		spot = G_Find(NULL, FOFS(classname), "info_player_deathmatch");

		if (!spot)
			trap->Error( ERR_DROP, "Couldn't find a spawn point" );

		VectorCopy (&spot->s.origin, origin);
		origin->z += 9;
		VectorCopy (&spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = (int)(random() * (numSpots / 2));

	VectorCopy (&list_spot[rnd]->s.origin, origin);
	origin->z += 9;
	VectorCopy (&list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

// Chooses a player start, deathmatch start, etc
gentity_t *SelectSpawnPoint ( vector3 *avoidPoint, vector3 *origin, vector3 *angles, qboolean isbot ) {
	return SelectRandomFurthestSpawnPoint( avoidPoint, origin, angles, isbot );

	/*
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint ( );
		if ( spot == nearestSpot ) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint ( );
		}		
	}

	// find a single player start spot
	if (!spot) {
		trap->Error( ERR_DROP, "Couldn't find a spawn point" );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
	*/
}

// Try to find a spawn point marked 'initial', otherwise use normal spawn selection.
gentity_t *SelectInitialSpawnPoint( vector3 *origin, vector3 *angles, qboolean isbot ) {
	gentity_t	*spot;

	spot = NULL;
	
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
		{
			continue;
		}
		
		if((spot->spawnflags & 0x01))
			break;
	}

	if (!spot || SpotWouldTelefrag(spot))
		return SelectSpawnPoint(&vec3_origin, origin, angles, isbot);

	VectorCopy (&spot->s.origin, origin);
	origin->z += 9;
	VectorCopy (&spot->s.angles, angles);

	return spot;
}

gentity_t *SelectSpectatorSpawnPoint( vector3 *origin, vector3 *angles ) {
	FindIntermissionPoint();

	VectorCopy( &level.intermission_origin, origin );
	VectorCopy( &level.intermission_angle, angles );

	return NULL;
}

void InitBodyQue( void ) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

// After sitting around for five seconds, fall into the ground and disappear
void BodySink( gentity_t *ent ) {
	if ( level.time - ent->timestamp > 6500 ) {
		// the body ques are never actually freed, they are just unlinked
		trap->SV_UnlinkEntity( (sharedEntity_t *)ent );
		ent->physicsObject = qfalse;
		return;	
	}
	ent->nextthink = level.time + 100;
	ent->s.pos.trBase.z -= 1;
}

// A player is respawning, so make an entity that looks just like the existing corpse to leave behind.
void CopyToBodyQue( gentity_t *ent ) {
	gentity_t		*body;
	int			contents;

	trap->SV_UnlinkEntity ((sharedEntity_t *)ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap->SV_PointContents( &ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	body->s = ent->s;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc
	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.number = ARRAY_INDEX( g_entities, body );
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( &ent->client->ps.velocity, &body->s.pos.trDelta );
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	// change the animation to the last-frame only, so the sequence
	// doesn't repeat anew for the body
	switch ( body->s.legsAnim & ~ANIM_TOGGLEBIT ) {
	case BOTH_DEATH1:
	case BOTH_DEAD1:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
		break;
	case BOTH_DEATH2:
	case BOTH_DEAD2:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
		break;
	case BOTH_DEATH3:
	case BOTH_DEAD3:
	default:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
		break;
	}

	body->r.svFlags = ent->r.svFlags;
	VectorCopy (&ent->r.mins, &body->r.mins);
	VectorCopy (&ent->r.maxs, &body->r.maxs);
	VectorCopy (&ent->r.absmin, &body->r.absmin);
	VectorCopy (&ent->r.absmax, &body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + 5000;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}


	VectorCopy ( &body->s.pos.trBase, &body->r.currentOrigin );
	trap->SV_LinkEntity ((sharedEntity_t *)body);
}

void SetClientViewAngle( gentity_t *ent, vector3 *angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle->data[i]);
		ent->client->ps.delta_angles.data[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, &ent->s.angles );
	VectorCopy (&ent->s.angles, &ent->client->ps.viewangles);
}

void ClientRespawn( gentity_t *ent ) {

	CopyToBodyQue (ent);
	ClientSpawn(ent);
}

// Returns number of players on a team
int TeamCount( int ignoreClientNum, team_t team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			count++;
		}
	}

	return count;
}

team_t PickTeam( int ignoreClientNum ) {
	int		counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount( ignoreClientNum, TEAM_BLUE );
	counts[TEAM_RED] = TeamCount( ignoreClientNum, TEAM_RED );

	if ( counts[TEAM_BLUE] > counts[TEAM_RED] ) {
		return TEAM_RED;
	}
	if ( counts[TEAM_RED] > counts[TEAM_BLUE] ) {
		return TEAM_BLUE;
	}
	// equal team count, so join the team with the lowest score
	if ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] ) {
		return TEAM_RED;
	}
	return TEAM_BLUE;
}

static void ClientCleanName(const char *in, char *out, size_t outSize) {
	int outpos = 0, colorlessLen = 0, spaces = 0;

	// discard leading spaces
	for(; *in == ' '; in++);
	
	for(; *in && outpos < outSize - 1; in++)
	{
		out[outpos] = *in;

		if(*in == ' ')
		{
			// don't allow too many consecutive spaces
			if(spaces > 2)
				continue;
			
			spaces++;
		}
		else if(outpos > 0 && out[outpos - 1] == Q_COLOR_ESCAPE)
		{
			if(Q_IsColorString(&out[outpos - 1]))
			{
				colorlessLen--;
				
				if(ColorIndex(*in) == 0)
				{
					// Disallow color black in names to prevent players
					// from getting advantage playing in front of black backgrounds
					outpos--;
					continue;
				}
			}
			else
			{
				spaces = 0;
				colorlessLen++;
			}
		}
		else
		{
			spaces = 0;
			colorlessLen++;
		}
		
		outpos++;
	}

	out[outpos] = '\0';

	// don't allow empty names
	if( *out == '\0' || colorlessLen == 0)
		Q_strncpyz(out, "UnnamedPlayer", outSize );
}


// Called from ClientConnect when the player first connects and directly by the server system when the player updates a userinfo variable.
//	The game can override any of the settings and call trap->SV_SetUserinfo if desired.
void ClientUserinfoChanged( int clientNum ) {
	gentity_t	*ent;
	gclient_t	*client;
	int			team;
	char		model[MAX_QPATH]={0}, oldname[MAX_NETNAME]={0}, color[8]={0};
	char		*s, userinfo[MAX_INFO_STRING]={0}, buf[MAX_INFO_STRING]={0};

	ent = g_entities + clientNum;
	client = ent->client;

	trap->SV_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	//QTZTODO: Userinfo validation ala JA++
	// check for malformed or illegal info strings
	if ( !Info_Validate( userinfo ) ) {
		strcpy( userinfo, "\\name\\badinfo" );
		// don't keep those clients and userinfo
		trap->SV_GameDropClient( clientNum, "Invalid userinfo" );
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	if ( !strcmp( s, "localhost" ) )
		client->pers.localClient = qtrue;

	// check the item prediction
	client->pers.predictItemPickup = qfalse;
	if ( atoi( Info_ValueForKey( userinfo, "cg_predictItems" ) ) )
		client->pers.predictItemPickup = qtrue;

	// set name
	Q_strncpyz ( oldname, client->pers.netname, sizeof( oldname ) );
	s = Info_ValueForKey( userinfo, "cl_name" );
	ClientCleanName( s, client->pers.netname, sizeof( client->pers.netname ) );

	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD )
			Q_strncpyz( client->pers.netname, "scoreboard", sizeof( client->pers.netname ) );
	}

	if ( client->pers.connected == CON_CONNECTED ) {
		if ( strcmp( oldname, client->pers.netname ) )
			trap->SV_GameSendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " renamed to %s\n\"", oldname, client->pers.netname ) );
	}

	// bots set their team a few frames later
	if ( level.gametype >= GT_TEAMBLOOD && g_entities[clientNum].r.svFlags & SVF_BOT ) {
		s = Info_ValueForKey( userinfo, "team" );
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) )
			team = TEAM_RED;
		else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) )
			team = TEAM_BLUE;
		else // pick the team with the least number of players
			team = PickTeam( clientNum );
	}
	else
		team = client->sess.sessionTeam;

	// set model
	Q_strncpyz( model, Info_ValueForKey( userinfo, "cg_model" ), sizeof( model ) );

	// colors
	Q_strncpyz( color, Info_ValueForKey( userinfo, "cg_color" ), sizeof( color ) );
	
	// send over a subset of the userinfo keys so other clients can print scoreboards, display models, and play custom sounds
	buf[0] = '\0';
	Q_strcat( buf, sizeof( buf ), va( "n\\%s\\", client->pers.netname ) );
	Q_strcat( buf, sizeof( buf ), va( "t\\%i\\", team ) );
	Q_strcat( buf, sizeof( buf ), va( "m\\%s\\", model ) );
	Q_strcat( buf, sizeof( buf ), va( "c\\%s\\", color ) );

	if ( ent->r.svFlags & SVF_BOT )
		Q_strcat( buf, sizeof( buf ), va( "s\\%s\\", Info_ValueForKey( userinfo, "skill" ) ) );

	trap->SV_SetConfigstring( CS_PLAYERS+clientNum, buf );

	// this is not the userinfo, more like the configstring actually
	G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, buf );
}

// Called when a player begins connecting to the server.
//	Called again for every map change or tournament restart.
//	The session information will be valid after exit.
//	Return NULL if the client should be allowed, otherwise return a string with the reason for denial.
//	Otherwise, the client will be sent the current gamestate and will eventually get to ClientBegin.
//	firstTime will be qtrue the very first time a client connects to the server machine, but qfalse on map changes and tournament restarts.
//	QTZTODO: ClientConnect/ClientUserinfoChanged from modbase/JA++	
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	char		*value, userinfo[MAX_INFO_STRING];
	gentity_t	*ent = &g_entities[clientNum];
	gclient_t	*client;

	trap->SV_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

 	// IP filtering
 	// recommanding PB based IP / GUID banning, the builtin system is pretty limited
 	// check to see if they are on the banned IP list
	value = Info_ValueForKey( userinfo, "ip" );
	if ( G_FilterPacket( value ) )
		return "You are banned from this server.";

	// we don't check password for bots and local client
	// NOTE: local client <-> "ip" "localhost"
	//   this means this client is not running in our current process
	if ( !isBot && strcmp( value, "localhost" ) ) {
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if ( g_password->string[0] && Q_stricmp( g_password->string, "none" ) &&
			strcmp( g_password->string, value) != 0) {
			return "Invalid password";
		}
	}

	// if a player reconnects quickly after a disconnect, the client disconnect may never be called, thus flag can get lost in the ether
	if ( ent->inuse ) {
		G_LogPrintf( "Forcing disconnect on active client: %i\n", clientNum );
		// so lets just fix up anything that should happen on a disconnect
		ClientDisconnect( clientNum );
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;
	memset( client, 0, sizeof(*client) );

	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if ( firstTime || level.newSession )
		G_InitSessionData( client, userinfo );
	G_ReadSessionData( client );

	if ( isBot ) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if ( !G_BotConnect( clientNum, !firstTime ) )
			return "BotConnectfailed";
	}

	// get and distribute relevent paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );
	ClientUserinfoChanged( clientNum );

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime )
		trap->SV_GameSendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname) );

	if ( level.gametype >= GT_TEAMBLOOD &&
		client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BroadcastTeamChange( client, -1 );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	return NULL;
}

// called when a client has finished connecting, and is ready to be placed into the level.
//	This will happen every level load, and on transition between teams, but doesn't happen on respawns
void ClientBegin( int clientNum ) {
	gentity_t	*ent;
	gclient_t	*client;
	int			flags;

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	if ( ent->r.linked ) {
		trap->SV_UnlinkEntity( (sharedEntity_t *)ent );
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;

	// locate ent at a spawn point
	ClientSpawn( ent );

	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		if ( level.gametype != GT_DUEL  ) {
			trap->SV_GameSendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname) );
		}
	}
	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	// count current clients and rank for scoreboard
	CalculateRanks();
}

// Called every time a client is placed fresh in the world: after the first ClientBegin, and after each respawn
//	Initializes all non-persistent parts of playerState
void ClientSpawn(gentity_t *ent) {
	int		index;
	vector3	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	int		persistent[MAX_PERSISTENT];
	gentity_t	*spawnPoint;
	gentity_t *tent;
	int		flags, gameFlags;
	int		savedPing;
//	char	*savedAreaBits;
	int		accuracy_hits, accuracy_shots;
	int		eventSequence;
	char	userinfo[MAX_INFO_STRING];

	index = ARRAY_INDEX( g_entities, ent );
	client = ent->client;

	VectorClear(&spawn_origin);

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		spawnPoint = SelectSpectatorSpawnPoint ( 
						&spawn_origin, &spawn_angles);
	} else if (level.gametype >= GT_FLAGS ) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint ( 
						client->sess.sessionTeam, 
						client->pers.teamState.state, 
						&spawn_origin, &spawn_angles,
						!!(ent->r.svFlags & SVF_BOT));
	}
	else
	{
		// the first spawn should be at a good looking spot
		if ( !client->pers.initialSpawn && client->pers.localClient )
		{
			client->pers.initialSpawn = qtrue;
			spawnPoint = SelectInitialSpawnPoint(&spawn_origin, &spawn_angles,
							     !!(ent->r.svFlags & SVF_BOT));
		}
		else
		{
			// don't spawn near existing origin if possible
			spawnPoint = SelectSpawnPoint ( 
				&client->ps.origin, 
				&spawn_origin, &spawn_angles, !!(ent->r.svFlags & SVF_BOT));
		}
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT);
	flags ^= EF_TELEPORT_BIT;
	gameFlags = ent->client->gameFlags & ( GF_VOTED );

	// clear everything but the persistent data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
//	savedAreaBits = client->areabits;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;
	for ( i = 0 ; i < MAX_PERSISTENT ; i++ ) {
		persistent[i] = client->ps.persistent[i];
	}
	eventSequence = client->ps.eventSequence;

	memset( client, 0, sizeof( *client ) );

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
//	client->areabits = savedAreaBits;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;
	client->lastkilled_client = -1;

	for ( i = 0 ; i < MAX_PERSISTENT ; i++ ) {
		client->ps.persistent[i] = persistent[i];
	}
	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistent[PERS_SPAWN_COUNT]++;
	client->ps.persistent[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	trap->SV_GetUserinfo( index, userinfo, sizeof(userinfo) );

	// clear entity values
	client->ps.eFlags = flags;

	client->gameFlags = gameFlags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;
	
	VectorCopy (&playerMins, &ent->r.mins);
	VectorCopy (&playerMaxs, &ent->r.maxs);

	client->ps.clientNum = index;

	//RAZMARK: ADDING NEW WEAPONS
	do { i = Q_irand( WP_NONE+1, WP_NUM_WEAPONS-1 ); } while ( i == WP_QUANTIZER );
	client->ps.stats[STAT_WEAPONS] = (1<<i)|(1<<WP_QUANTIZER);
	client->ps.ammo[WP_QUANTIZER] = -1;
	client->ps.ammo[i] = (int)floorf( (float)weaponData[i].ammoMax * 0.75f );
	client->ps.weapon = i;

	// health will count down towards max_health
	ent->health = client->ps.stats[STAT_HEALTH] = MAX_HEALTH;

	G_SetOrigin( ent, &spawn_origin );
	VectorCopy( &spawn_origin, &client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap->SV_GetUsercmd( ARRAY_INDEX( level.clients, client ), &ent->client->pers.cmd );
	SetClientViewAngle( ent, &spawn_angles );
	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity->integer * 1000;
	client->latched_buttons = 0;

	// set default animations
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if (!level.intermissiontime) {
		if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
			G_KillBox(ent);
			// force the base weapon up
			client->ps.weapon = WP_NONE+1;
			client->ps.weaponstate = WEAPON_READY;
			// fire the targets of the spawn point
			G_UseTargets(spawnPoint, ent);
			// select the highest weapon number available, after any spawn given items have fired
			client->ps.weapon = 1;

			for (i = WP_NUM_WEAPONS - 1 ; i > 0 ; i--) {
				if (client->ps.stats[STAT_WEAPONS] & (1 << i)) {
					client->ps.weapon = i;
					break;
				}
			}
			// positively link the client, even if the command times are weird
			VectorCopy(&ent->client->ps.origin, &ent->r.currentOrigin);

			tent = G_TempEntity(&ent->client->ps.origin, EV_PLAYER_TELEPORT_IN);
			tent->s.clientNum = ent->s.clientNum;

			trap->SV_LinkEntity ((sharedEntity_t *)ent);
		}
	} else {
		// move players to intermission
		MoveClientToIntermission(ent);
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ARRAY_INDEX( g_entities, ent ) );

	// run the presend to set anything else
	if ( ent->client->sess.spectatorState != SPECTATOR_FOLLOW )
		ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, !pm_float->boolean );
}

void G_ClearVote( gentity_t *ent ) {
	if ( !level.voteTime )
		return;

	if ( ent->client->gameFlags & GF_VOTED ) {
		if ( ent->client->pers.vote == 1 ) {
			level.voteYes--;
			trap->SV_SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
		}
		else if ( ent->client->pers.vote == 2 ) {
			level.voteNo--;
			trap->SV_SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
		}
	}
	ent->client->gameFlags &= ~(GF_VOTED);
	ent->client->pers.vote = 0;
}

// Called when a player drops from the server.
//	Will not be called between levels.
//	This should NOT be called directly by any game logic, call trap->SV_GameDropClient(), which will
//	call this and do server system housekeeping.
void ClientDisconnect( int clientNum ) {
	gentity_t		*ent;
	gentity_t		*tent;
	gentity_t		*flag = NULL;
	const gitem_t	*item = NULL;
	vector3			launchvel;
	int				i;

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin( clientNum );

	ent = g_entities + clientNum;
	if (!ent->client || ent->client->pers.connected == CON_DISCONNECTED) {
		return;
	}

	// stop any following clients
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR
			&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.clients[i].sess.spectatorClient == clientNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED 
		&& ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		tent = G_TempEntity( &ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = ent->s.clientNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems( ent );
	//	TossClientPersistantPowerups( ent );

		//OSP: New code for tossing flags
		if ( ent->client->ps.powerups[PW_REDFLAG] ) {
			item = BG_FindItem( "team_redflag" );
			ent->client->ps.powerups[PW_REDFLAG] = 0;
		}
		if ( ent->client->ps.powerups[PW_BLUEFLAG] ) {
			item = BG_FindItem( "team_blueflag" );
			ent->client->ps.powerups[PW_BLUEFLAG] = 0;
		}

		if ( item ) {
			//OSP: fix for suicide drop exploit through walls/gates
			launchvel.x = 0;    //crandom()*20;
			launchvel.y = 0;    //crandom()*20;
			launchvel.z = 0;    //10+random()*10;

			flag = LaunchItem( item, &ent->r.currentOrigin, &launchvel );
			flag->s.modelindex2 = ent->s.otherEntityNum2;    // JPW NERVE FIXME set player->otherentitynum2 with old modelindex2 from flag and restore here
			flag->message = ent->message;       // DHM - Nerve :: also restore item name
			// Clear out player's temp copies
			ent->s.otherEntityNum2 = 0;
			ent->message = NULL;
		}
	}

	G_LogPrintf( "ClientDisconnect: %i\n", clientNum );

	if( level.gametype == GT_DUEL && ent->client->sess.sessionTeam == TEAM_FREE &&
		level.intermissiontime ) {

		trap->Cbuf_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
		level.restarted = qtrue;
		level.changemap = NULL;
		level.intermissiontime = 0;
	}

	trap->SV_UnlinkEntity ((sharedEntity_t *)ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistent[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;
	ent->r.contents = 0;

	G_ClearVote( ent );

	trap->SV_SetConfigstring( CS_PLAYERS + clientNum, "");

	CalculateRanks();

	if ( ent->r.svFlags & SVF_BOT )
		BotAIShutdownClient( clientNum, qfalse );
}
