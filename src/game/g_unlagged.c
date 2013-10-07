//NT - unlagged - new time-shift client functions

#include "g_local.h"
#include "g_unlagged.h"

static int frametime = 25;
static int historytime = 250;
static int numTrails = 10; //historytime/frametime; 

// Clear out the given client's origin trails (should be called from ClientBegin and when the teleport bit is toggled)
void G_ResetTrail( gentity_t *ent ) {
	int		i, time;

	// fill up the origin trails with data (assume the current position for the last 1/2 second or so)
	ent->client->unlagged.trailHead = numTrails - 1;
	for ( i = ent->client->unlagged.trailHead, time = level.time; i >= 0; i--, time -= frametime ) {
		VectorCopy( &ent->r.mins, &ent->client->unlagged.trail[i].mins );
		VectorCopy( &ent->r.maxs, &ent->client->unlagged.trail[i].maxs );
		VectorCopy( &ent->r.currentOrigin, &ent->client->unlagged.trail[i].currentOrigin );
		VectorCopy( &ent->r.currentAngles, &ent->client->unlagged.trail[i].currentAngles );
		ent->client->unlagged.trail[i].leveltime = time;
		ent->client->unlagged.trail[i].time = time;
	}
}

// Keep track of where the client's been (usually called every ClientThink)
void G_StoreTrail( gentity_t *ent ) {
	int		head, newtime;

	head = ent->client->unlagged.trailHead;

	// if we're on a new frame
	if ( ent->client->unlagged.trail[head].leveltime < level.time ) {
		// snap the last head up to the end of frame time
		ent->client->unlagged.trail[head].time = level.previousTime;

		// increment the head
		ent->client->unlagged.trailHead++;
		if ( ent->client->unlagged.trailHead >= numTrails ) {
			ent->client->unlagged.trailHead = 0;
		}
		head = ent->client->unlagged.trailHead;
	}

	if ( ent->r.svFlags & SVF_BOT ) {
		// bots move only once per frame
		newtime = level.time;
	} else {
		// calculate the actual server time
		// (we set level.frameStartTime every G_RunFrame)
		newtime = level.previousTime + trap->Milliseconds() - level.frameStartTime;
		if ( newtime > level.time ) {
			newtime = level.time;
		} else if ( newtime <= level.previousTime ) {
			newtime = level.previousTime + 1;
		}
	}

	// store all the collision-detection info and the time
	VectorCopy( &ent->r.mins, &ent->client->unlagged.trail[head].mins );
	VectorCopy( &ent->r.maxs, &ent->client->unlagged.trail[head].maxs );
	VectorCopy( &ent->r.currentOrigin, &ent->client->unlagged.trail[head].currentOrigin );
	VectorCopy( &ent->r.currentAngles, &ent->client->unlagged.trail[head].currentAngles );
	ent->client->unlagged.trail[head].leveltime = level.time;
	ent->client->unlagged.trail[head].time = newtime;

	// FOR TESTING ONLY
//	Com_Printf("level.previousTime: %d, level.time: %d, newtime: %d\n", level.previousTime, level.time, newtime);
}

// Used below to interpolate between two previous vectors
//	Returns a vector "frac" times the distance between "start" and "end"
static void TimeShiftLerp( float frac, vector3 *start, vector3 *end, vector3 *result ) {
	float	comp = 1.0f - frac;

	result->x = frac * start->x + comp * end->x;
	result->y = frac * start->y + comp * end->y;
	result->z = frac * start->z + comp * end->z;
}

// Move a client back to where he was at the specified "time"
void G_TimeShiftClient( gentity_t *ent, int time ) {
	int		j, k;

	if ( time > level.time )
		time = level.time;

	// find two entries in the origin trail whose times sandwich "time"
	// assumes no two adjacent trail records have the same timestamp
	j = k = ent->client->unlagged.trailHead;
	do {
		if ( ent->client->unlagged.trail[j].time <= time )
			break;

		k = j;
		j--;
		if ( j < 0 )
			j = numTrails - 1;
	}
	while ( j != ent->client->unlagged.trailHead );

	// if we got past the first iteration above, we've sandwiched (or wrapped)
	if ( j != k ) {
		// make sure it doesn't get re-saved
		if ( ent->client->unlagged.saved.leveltime != level.time ) {
			// save the current origin and bounding box
			VectorCopy( &ent->r.mins, &ent->client->unlagged.saved.mins );
			VectorCopy( &ent->r.maxs, &ent->client->unlagged.saved.maxs );
			VectorCopy( &ent->r.currentOrigin, &ent->client->unlagged.saved.currentOrigin );
			VectorCopy( &ent->r.currentAngles, &ent->client->unlagged.saved.currentAngles );
			ent->client->unlagged.saved.leveltime = level.time;
		}

		// if we haven't wrapped back to the head, we've sandwiched, so
		// we shift the client's position back to where he was at "time"
		if ( j != ent->client->unlagged.trailHead )
		{
			float frac = (float)(ent->client->unlagged.trail[k].time - time) / (float)(ent->client->unlagged.trail[k].time - ent->client->unlagged.trail[j].time);

			// FOR TESTING ONLY
//			Com_Printf( "level time: %d, fire time: %d, j time: %d, k time: %d\n", level.time, time, ent->client->trail[j].time, ent->client->trail[k].time );

			// interpolate between the two origins to give position at time index "time"
			TimeShiftLerp( frac, &ent->client->unlagged.trail[k].currentOrigin, &ent->client->unlagged.trail[j].currentOrigin, &ent->r.currentOrigin );
			ent->r.currentAngles.yaw = LerpAngle( ent->client->unlagged.trail[k].currentAngles.yaw, ent->r.currentAngles.yaw, frac );

			// lerp these too, just for fun (and ducking)
			TimeShiftLerp( frac, &ent->client->unlagged.trail[k].mins, &ent->client->unlagged.trail[j].mins, &ent->r.mins );
			TimeShiftLerp( frac, &ent->client->unlagged.trail[k].maxs, &ent->client->unlagged.trail[j].maxs, &ent->r.maxs );

			// this will recalculate absmin and absmax
			trap->SV_LinkEntity( (sharedEntity_t *)ent );
		} else {
			// we wrapped, so grab the earliest
			VectorCopy( &ent->client->unlagged.trail[k].currentAngles, &ent->r.currentAngles );
			VectorCopy( &ent->client->unlagged.trail[k].currentOrigin, &ent->r.currentOrigin );
			VectorCopy( &ent->client->unlagged.trail[k].mins, &ent->r.mins );
			VectorCopy( &ent->client->unlagged.trail[k].maxs, &ent->r.maxs );

			// this will recalculate absmin and absmax
			trap->SV_LinkEntity( (sharedEntity_t *)ent );
		}
	}
}

// Move ALL clients back to where they were at the specified "time", except for "skip"
void G_TimeShiftAllClients( int time, gentity_t *skip ) {
	int i;
	gentity_t *ent;

	if ( time > level.time )
		time = level.time;

	// for every client
	for ( i=0, ent=g_entities; i<MAX_CLIENTS; i++, ent++ ) {
		if ( ent->client && ent->inuse && ent->client->sess.sessionTeam < TEAM_SPECTATOR && ent != skip )
			G_TimeShiftClient( ent, time );
	}
}

// Move a client back to where he was before the time shift
void G_UnTimeShiftClient( gentity_t *ent ) {
	// if it was saved
	if ( ent->client->unlagged.saved.leveltime == level.time ) {
		// move it back
		VectorCopy( &ent->client->unlagged.saved.mins, &ent->r.mins );
		VectorCopy( &ent->client->unlagged.saved.maxs, &ent->r.maxs );
		VectorCopy( &ent->client->unlagged.saved.currentOrigin, &ent->r.currentOrigin );
		VectorCopy( &ent->client->unlagged.saved.currentAngles, &ent->r.currentAngles );
		ent->client->unlagged.saved.leveltime = 0;

		// this will recalculate absmin and absmax
		trap->SV_LinkEntity( (sharedEntity_t *)ent );
	}
}

// Move ALL the clients back to where they were before the time shift, except for "skip"
void G_UnTimeShiftAllClients( gentity_t *skip ) {
	int i;
	gentity_t *ent;

	for ( i=0, ent=g_entities; i<MAX_CLIENTS; i++, ent++) {
		if ( ent->client && ent->inuse && ent->client->sess.sessionTeam < TEAM_SPECTATOR && ent != skip )
			G_UnTimeShiftClient( ent );
	}
}

void G_UpdateTrailData( void ) {
	frametime = sv_frametime->integer;
	historytime = g_delagHistory->integer;
	numTrails = MIN( historytime/frametime, NUM_CLIENT_TRAILS );
}
