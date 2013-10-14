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



/*

	PUSHMOVE

*/

typedef struct pushed_s {
	gentity_t	*ent;
	vector3	origin;
	vector3	angles;
	float	deltayaw;
} pushed_t;
pushed_t	pushed[MAX_GENTITIES], *pushed_p;

gentity_t	*G_TestEntityPosition( gentity_t *ent ) {
	trace_t	tr;
	int		mask;

	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_SOLID;
	}
	if ( ent->client ) {
		trap->SV_Trace( &tr, &ent->client->ps.origin, &ent->r.mins, &ent->r.maxs, &ent->client->ps.origin, ent->s.number, mask );
	} else {
		trap->SV_Trace( &tr, &ent->s.pos.trBase, &ent->r.mins, &ent->r.maxs, &ent->s.pos.trBase, ent->s.number, mask );
	}
	
	if (tr.startsolid)
		return &g_entities[ tr.entityNum ];
		
	return NULL;
}

void G_CreateRotationMatrix(vector3 *angles, matrix3 matrix) {
	AngleVectors(angles, &matrix[0], &matrix[1], &matrix[2]);
	VectorInverse(&matrix[1]);
}

void G_RotatePoint(vector3 *point, matrix3 matrix) {
	vector3 tvec;

	VectorCopy(point, &tvec);
	point->x = DotProduct(&matrix[0], &tvec);
	point->y = DotProduct(&matrix[1], &tvec);
	point->z = DotProduct(&matrix[2], &tvec);
}

// Returns qfalse if the move is blocked
qboolean G_TryPushingEntity( gentity_t *check, gentity_t *pusher, vector3 *move, vector3 *amove ) {
	matrix3		matrix, transpose;
	vector3		org, org2, move2;
	gentity_t	*block;

	// EF_MOVER_STOP will just stop when contacting another entity
	// instead of pushing it, but entities can still ride on top of it
	if ( ( pusher->s.eFlags & EF_MOVER_STOP ) && 
		check->s.groundEntityNum != pusher->s.number ) {
		return qfalse;
	}

	// save off the old position
	if (pushed_p > &pushed[MAX_GENTITIES]) {
		trap->Error( ERR_DROP, "pushed_p > &pushed[MAX_GENTITIES]" );
	}
	pushed_p->ent = check;
	VectorCopy (&check->s.pos.trBase, &pushed_p->origin);
	VectorCopy (&check->s.apos.trBase, &pushed_p->angles);
	if ( check->client ) {
		pushed_p->deltayaw = (float)check->client->ps.delta_angles.yaw;
		VectorCopy (&check->client->ps.origin, &pushed_p->origin);
	}
	pushed_p++;

	// try moving the contacted entity 
	// figure movement due to the pusher's amove
	G_CreateRotationMatrix( amove, transpose );
	MatrixTranspose( transpose, matrix );
	if ( check->client ) {
		VectorSubtract (&check->client->ps.origin, &pusher->r.currentOrigin, &org);
	}
	else {
		VectorSubtract (&check->s.pos.trBase, &pusher->r.currentOrigin, &org);
	}
	VectorCopy( &org, &org2 );
	G_RotatePoint( &org2, matrix );
	VectorSubtract (&org2, &org, &move2);
	// add movement
	VectorAdd (&check->s.pos.trBase, move, &check->s.pos.trBase);
	VectorAdd (&check->s.pos.trBase, &move2, &check->s.pos.trBase);
	if ( check->client ) {
		VectorAdd (&check->client->ps.origin, move, &check->client->ps.origin);
		VectorAdd (&check->client->ps.origin, &move2, &check->client->ps.origin);
		// make sure the client's view rotates when on a rotating mover
		check->client->ps.delta_angles.yaw += ANGLE2SHORT(amove->yaw);
	}

	// may have pushed them off an edge
	if ( check->s.groundEntityNum != pusher->s.number ) {
		check->s.groundEntityNum = ENTITYNUM_NONE;
	}

	block = G_TestEntityPosition( check );
	if (!block) {
		// pushed ok
		if ( check->client ) {
			VectorCopy( &check->client->ps.origin, &check->r.currentOrigin );
		} else {
			VectorCopy( &check->s.pos.trBase, &check->r.currentOrigin );
		}
		trap->SV_LinkEntity ((sharedEntity_t *)check);
		return qtrue;
	}

	// if it is ok to leave in the old position, do it
	// this is only relevent for riding entities, not pushed
	// Sliding trapdoors can cause this.
	VectorCopy( &(pushed_p-1)->origin, &check->s.pos.trBase);
	if ( check->client ) {
		VectorCopy( &(pushed_p-1)->origin, &check->client->ps.origin);
	}
	VectorCopy( &(pushed_p-1)->angles, &check->s.apos.trBase );
	block = G_TestEntityPosition (check);
	if ( !block ) {
		check->s.groundEntityNum = ENTITYNUM_NONE;
		pushed_p--;
		return qtrue;
	}

	// blocked
	return qfalse;
}

void G_ExplodeMissile( gentity_t *ent );

// Objects need to be moved back on a failed push, otherwise riders would continue to slide.
//	If qfalse is returned, *obstacle will be the blocking entity
qboolean G_MoverPush( gentity_t *pusher, vector3 *move, vector3 *amove, gentity_t **obstacle ) {
	int			i, e;
	gentity_t	*check;
	vector3		mins, maxs;
	pushed_t	*p;
	int			entityList[MAX_GENTITIES];
	int			listedEntities;
	vector3		totalMins, totalMaxs;

	*obstacle = NULL;


	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if ( pusher->r.currentAngles.x || pusher->r.currentAngles.y || pusher->r.currentAngles.z
		|| amove->x || amove->y || amove->z ) {
		float		radius;

		radius = RadiusFromBounds( &pusher->r.mins, &pusher->r.maxs );
		for ( i = 0 ; i < 3 ; i++ ) {
			mins.data[i] = pusher->r.currentOrigin.data[i] + move->data[i] - radius;
			maxs.data[i] = pusher->r.currentOrigin.data[i] + move->data[i] + radius;
			totalMins.data[i] = mins.data[i] - move->data[i];
			totalMaxs.data[i] = maxs.data[i] - move->data[i];
		}
	} else {
		for (i=0 ; i<3 ; i++) {
			mins.data[i] = pusher->r.absmin.data[i] + move->data[i];
			maxs.data[i] = pusher->r.absmax.data[i] + move->data[i];
		}

		VectorCopy( &pusher->r.absmin, &totalMins );
		VectorCopy( &pusher->r.absmax, &totalMaxs );
		for (i=0 ; i<3 ; i++) {
			if ( move->data[i] > 0 )	totalMaxs.data[i] += move->data[i];
			else						totalMins.data[i] += move->data[i];
		}
	}

	// unlink the pusher so we don't get it in the entityList
	trap->SV_UnlinkEntity( (sharedEntity_t *)pusher );

	listedEntities = trap->SV_AreaEntities( &totalMins, &totalMaxs, entityList, MAX_GENTITIES );

	// move the pusher to its final position
	VectorAdd( &pusher->r.currentOrigin, move, &pusher->r.currentOrigin );
	VectorAdd( &pusher->r.currentAngles, amove, &pusher->r.currentAngles );
	trap->SV_LinkEntity( (sharedEntity_t *)pusher );

	// see if any solid entities are inside the final position
	for ( e = 0 ; e < listedEntities ; e++ ) {
		check = &g_entities[ entityList[ e ] ];

		// only push items and players
		if ( check->s.eType != ET_ITEM && check->s.eType != ET_PLAYER && !check->physicsObject ) {
			continue;
		}

		// if the entity is standing on the pusher, it will definitely be moved
		if ( check->s.groundEntityNum != pusher->s.number ) {
			// see if the ent needs to be tested
			if ( check->r.absmin.x >= maxs.x ||
				 check->r.absmin.y >= maxs.y ||
				 check->r.absmin.z >= maxs.z ||
				 check->r.absmax.x <= mins.x ||
				 check->r.absmax.y <= mins.y ||
				 check->r.absmax.z <= mins.z ) {
				continue;
			}
			// see if the ent's bbox is inside the pusher's final position
			// this does allow a fast moving object to pass through a thin entity...
			if (!G_TestEntityPosition (check)) {
				continue;
			}
		}

		// the entity needs to be pushed
		if ( G_TryPushingEntity( check, pusher, move, amove ) ) {
			continue;
		}

		// the move was blocked an entity

		// bobbing entities are instant-kill and never get blocked
		if ( pusher->s.pos.trType == TR_SINE || pusher->s.apos.trType == TR_SINE ) {
			//Raz: don't care about the affector for pushers
			G_Damage( check, pusher, pusher, NULL, NULL, NULL, 99999, 0, MOD_CRUSH );
			continue;
		}

		
		// save off the obstacle so we can call the block function (crush, etc)
		*obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for ( p=pushed_p-1 ; p>=pushed ; p-- ) {
			VectorCopy (&p->origin, &p->ent->s.pos.trBase);
			VectorCopy (&p->angles, &p->ent->s.apos.trBase);
			if ( p->ent->client ) {
				p->ent->client->ps.delta_angles.yaw = (int)p->deltayaw;
				VectorCopy (&p->origin, &p->ent->client->ps.origin);
			}
			trap->SV_LinkEntity ((sharedEntity_t *)p->ent);
		}
		return qfalse;
	}

	return qtrue;
}

void G_MoverTeam( gentity_t *ent ) {
	vector3		move, amove;
	gentity_t	*part, *obstacle;
	vector3		origin, angles;

	obstacle = NULL;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;
	for (part = ent ; part ; part=part->teamchain) {
		// get current position
		BG_EvaluateTrajectory( &part->s.pos, level.time, &origin );
		BG_EvaluateTrajectory( &part->s.apos, level.time, &angles );
		VectorSubtract( &origin, &part->r.currentOrigin, &move );
		VectorSubtract( &angles, &part->r.currentAngles, &amove );
		if ( !G_MoverPush( part, &move, &amove, &obstacle ) ) {
			break;	// move was blocked
		}
	}

	if (part) {
		// go back to the previous position
		for ( part = ent ; part ; part = part->teamchain ) {
			part->s.pos.trTime += level.time - level.previousTime;
			part->s.apos.trTime += level.time - level.previousTime;
			BG_EvaluateTrajectory( &part->s.pos, level.time, &part->r.currentOrigin );
			BG_EvaluateTrajectory( &part->s.apos, level.time, &part->r.currentAngles );
			trap->SV_LinkEntity( (sharedEntity_t *)part );
		}

		// if the pusher has a "blocked" function, call it
		if (ent->blocked) {
			ent->blocked( ent, obstacle );
		}
		return;
	}

	// the move succeeded
	for ( part = ent ; part ; part = part->teamchain ) {
		// call the reached function if time is at or past end point
		if ( part->s.pos.trType == TR_LINEAR_STOP ) {
			if ( level.time >= part->s.pos.trTime + part->s.pos.trDuration ) {
				if ( part->reached ) {
					part->reached( part );
				}
			}
		}
	}
}

void G_RunMover( gentity_t *ent ) {
	// if not a team captain, don't do anything, because
	// the captain will handle everything
	if ( ent->flags & FL_TEAMSLAVE ) {
		return;
	}

	// if stationary at one of the positions, don't move anything
	if ( ent->s.pos.trType != TR_STATIONARY || ent->s.apos.trType != TR_STATIONARY ) {
		//OSP: pause
		if ( level.pause.state == PAUSE_NONE )
			G_MoverTeam( ent );
		else
			ent->s.pos.trTime += level.time - level.previousTime;
	}

	// check think function
	G_RunThink( ent );
}

/*

	GENERAL MOVERS

	Doors, plats, and buttons are all binary (two position) movers
	Pos1 is "at rest", pos2 is "activated"

*/

void SetMoverState( gentity_t *ent, moverState_t moverState, int time ) {
	vector3			delta;
	float			f;

	ent->moverState = moverState;

	ent->s.pos.trTime = time;
	switch( moverState ) {
	case MOVER_POS1:
		VectorCopy( &ent->pos1, &ent->s.pos.trBase );
		ent->s.pos.trType = TR_STATIONARY;
		break;
	case MOVER_POS2:
		VectorCopy( &ent->pos2, &ent->s.pos.trBase );
		ent->s.pos.trType = TR_STATIONARY;
		break;
	case MOVER_1TO2:
		VectorCopy( &ent->pos1, &ent->s.pos.trBase );
		VectorSubtract( &ent->pos2, &ent->pos1, &delta );
		f = 1000.0f / ent->s.pos.trDuration;
		VectorScale( &delta, f, &ent->s.pos.trDelta );
		ent->s.pos.trType = TR_LINEAR_STOP;
		break;
	case MOVER_2TO1:
		VectorCopy( &ent->pos2, &ent->s.pos.trBase );
		VectorSubtract( &ent->pos1, &ent->pos2, &delta );
		f = 1000.0f / ent->s.pos.trDuration;
		VectorScale( &delta, f, &ent->s.pos.trDelta );
		ent->s.pos.trType = TR_LINEAR_STOP;
		break;
	}
	BG_EvaluateTrajectory( &ent->s.pos, level.time, &ent->r.currentOrigin );	
	trap->SV_LinkEntity( (sharedEntity_t *)ent );
}

// All entities in a mover team will move from pos1 to pos2 in the same amount of time
void MatchTeam( gentity_t *teamLeader, moverState_t moverState, int time ) {
	gentity_t		*slave;

	for ( slave = teamLeader ; slave ; slave = slave->teamchain ) {
		SetMoverState( slave, moverState, time );
	}
}

void ReturnToPos1( gentity_t *ent ) {
	MatchTeam( ent, MOVER_2TO1, level.time );

	// looping sound
	ent->s.loopSound = ent->soundLoop;

	// starting sound
	if ( ent->sound2to1 ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
	}
}

void Reached_BinaryMover( gentity_t *ent ) {

	// stop the looping sound
	ent->s.loopSound = ent->soundLoop;

	if ( ent->moverState == MOVER_1TO2 ) {
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		// play sound
		if ( ent->soundPos2 ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
		}

		// return to pos1 after a delay
		ent->think = ReturnToPos1;
		ent->nextthink = level.time + (int)ent->wait;

		// fire targets
		if ( !ent->activator ) {
			ent->activator = ent;
		}
		G_UseTargets( ent, ent->activator );
	} else if ( ent->moverState == MOVER_2TO1 ) {
		// reached pos1
		SetMoverState( ent, MOVER_POS1, level.time );

		// play sound
		if ( ent->soundPos1 ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );
		}

		// close areaportals
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap->SV_AdjustAreaPortalState( (sharedEntity_t *)ent, qfalse );
		}
	} else {
		trap->Error( ERR_DROP, "Reached_BinaryMover: bad moverState" );
	}
}

void Use_BinaryMover( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	int		total;
	int		partial;

	// only the master should be used
	if ( ent->flags & FL_TEAMSLAVE ) {
		Use_BinaryMover( ent->teammaster, other, activator );
		return;
	}

	ent->activator = activator;

	if ( ent->moverState == MOVER_POS1 ) {
		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_1TO2, level.time + 50 );

		// starting sound
		if ( ent->sound1to2 ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}

		// looping sound
		ent->s.loopSound = ent->soundLoop;

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap->SV_AdjustAreaPortalState( (sharedEntity_t *)ent, qtrue );
		}
		return;
	}

	// if all the way up, just delay before coming down
	if ( ent->moverState == MOVER_POS2 ) {
		ent->nextthink = level.time + (int)ent->wait;
		return;
	}

	// only partway down before reversing
	if ( ent->moverState == MOVER_2TO1 ) {
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.pos.trTime;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_1TO2, level.time - ( total - partial ) );

		if ( ent->sound1to2 ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}
		return;
	}

	// only partway up before reversing
	if ( ent->moverState == MOVER_1TO2 ) {
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.pos.trTime;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_2TO1, level.time - ( total - partial ) );

		if ( ent->sound2to1 ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
		}
		return;
	}
}

// "pos1", "pos2", and "speed" should be set before calling, so the movement delta can be calculated
void InitMover( gentity_t *ent ) {
	vector3		move;
	float		distance;
	float		light;
	vector3		color;
	qboolean	lightSet, colorSet;
	char		*sound;

	// if the "model2" key is set, use a seperate model for drawing, but clip against the brushes
	if ( ent->model2 ) {
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}

	// if the "loopsound" key is set, use a constant looping sound when moving
	if ( G_SpawnString( "noise", "100", &sound ) ) {
		ent->s.loopSound = G_SoundIndex( sound );
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat( "light", "100", &light );
	colorSet = G_SpawnVector( "color", "1 1 1", &color );
	if ( lightSet || colorSet ) {
		int		r, g, b, i;

		r = (int)(color.r * 255);
		if ( r > 255 )
			r = 255;

		g = (int)(color.g * 255);
		if ( g > 255 )
			g = 255;

		b = (int)(color.b * 255);
		if ( b > 255 )
			b = 255;

		i = (int)(light / 4);
		if ( i > 255 )
			i = 255;
		ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}


	ent->use = Use_BinaryMover;
	ent->reached = Reached_BinaryMover;

	ent->moverState = MOVER_POS1;
	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_MOVER;
	VectorCopy (&ent->pos1, &ent->r.currentOrigin);
	trap->SV_LinkEntity ((sharedEntity_t *)ent);

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( &ent->pos1, &ent->s.pos.trBase );

	// calculate time to reach second position from speed
	VectorSubtract( &ent->pos2, &ent->pos1, &move );
	distance = VectorLength( &move );
	if ( ! ent->speed ) {
		ent->speed = 100;
	}
	VectorScale( &move, ent->speed, &ent->s.pos.trDelta );
	ent->s.pos.trDuration = (int)(distance * 1000 / ent->speed);
	if ( ent->s.pos.trDuration <= 0 ) {
		ent->s.pos.trDuration = 1;
	}
}

/*

	DOOR

	A use can be triggered either by a touch function, by being shot, or by being targeted by another entity.

*/

void Blocked_Door( gentity_t *ent, gentity_t *other ) {
	// remove anything other than a client
	if ( !other->client ) {
		// except CTF flags!!!!
		if( other->s.eType == ET_ITEM && other->item->giType == IT_TEAM ) {
			Team_DroppedFlagThink( other );
			return;
		}
		G_TempEntity( &other->s.origin, EV_ITEM_POP );
		G_FreeEntity( other );
		return;
	}

	if ( ent->damage ) {
		G_Damage( other, ent, ent, NULL, NULL, NULL, ent->damage, 0, MOD_CRUSH );
	}
	if ( ent->spawnflags & 4 ) {
		return;		// crushers don't reverse
	}

	// reverse direction
	Use_BinaryMover( ent, ent, other );
}

static void Touch_DoorTriggerSpectator( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	int axis;
	float doorMin, doorMax;
	vector3 origin;

	axis = ent->count;
	// the constants below relate to constants in Think_SpawnNewDoorTrigger()
	doorMin = ent->r.absmin.data[axis] + 100;
	doorMax = ent->r.absmax.data[axis] - 100;

	VectorCopy(&other->client->ps.origin, &origin);

	if (origin.data[axis] < doorMin ||
		origin.data[axis] > doorMax)
		return;

	if (fabs(origin.data[axis] - doorMax) < fabs(origin.data[axis] - doorMin))
		origin.data[axis] = doorMin - 10;
	else
		origin.data[axis] = doorMax + 10;

	TeleportPlayer(other, &origin, tv(10000000.0f, 0, 0));
}

void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( other->client && other->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		// if the door is not open and not opening
		if ( ent->parent->moverState != MOVER_1TO2 &&
			ent->parent->moverState != MOVER_POS2) {
			Touch_DoorTriggerSpectator( ent, other, trace );
		}
	}
	else if ( ent->parent->moverState != MOVER_1TO2 ) {
		Use_BinaryMover( ent->parent, ent, other );
	}
}

// All of the parts of a door have been spawned, so create a trigger that encloses all of them
void Think_SpawnNewDoorTrigger( gentity_t *ent ) {
	gentity_t		*other;
	vector3		mins, maxs;
	int			i, best;

	// set all of the slaves as shootable
	for ( other = ent ; other ; other = other->teamchain ) {
		other->takedamage = qtrue;
	}

	// find the bounds of everything on the team
	VectorCopy (&ent->r.absmin, &mins);
	VectorCopy (&ent->r.absmax, &maxs);

	for (other = ent->teamchain ; other ; other=other->teamchain) {
		AddPointToBounds (&other->r.absmin, &mins, &maxs);
		AddPointToBounds (&other->r.absmax, &mins, &maxs);
	}

	// find the thinnest axis, which will be the one we expand
	best = 0;
	for ( i = 1 ; i < 3 ; i++ ) {
		if ( maxs.data[i] - mins.data[i] < maxs.data[best] - mins.data[best] ) {
			best = i;
		}
	}
	maxs.data[best] += 120;
	mins.data[best] -= 120;

	// create a trigger with this size
	other = G_Spawn();
	other->classname = "door_trigger";
	VectorCopy (&mins, &other->r.mins);
	VectorCopy (&maxs, &other->r.maxs);
	other->parent = ent;
	other->r.contents = CONTENTS_TRIGGER;
	other->touch = Touch_DoorTrigger;
	// remember the thinnest axis
	other->count = best;
	trap->SV_LinkEntity ((sharedEntity_t *)other);

	MatchTeam( ent, ent->moverState, level.time );
}

void Think_MatchTeam( gentity_t *ent ) {
	MatchTeam( ent, ent->moverState, level.time );
}


/*QUAKED func_door (0 .5 .8) ? START_OPEN x CRUSHER
TOGGLE		wait in both the start and end states for a trigger event.
START_OPEN	the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER	monsters will not trigger this door

"model2"	.md3 model to also draw
"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"lip"		lip remaining at end of move (8 default)
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
"health"	if set, the door must be shot open
*/
void SP_func_door (gentity_t *ent) {
	vector3	abs_movedir;
	float	distance;
	vector3	size;
	float	lip;

	ent->sound1to2 = ent->sound2to1 = G_SoundIndex("sound/movers/doors/dr1_strt.wav");
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex("sound/movers/doors/dr1_end.wav");

	ent->blocked = Blocked_Door;

	// default speed of 400
	if (!ent->speed)
		ent->speed = 400;

	// default wait of 2 seconds
	if (!ent->wait)
		ent->wait = 2;
	ent->wait *= 1000;

	// default lip of 8 units
	G_SpawnFloat( "lip", "8", &lip );

	// default damage of 2 points
	G_SpawnInt( "dmg", "2", &ent->damage );

	// first position at start
	VectorCopy( &ent->s.origin, &ent->pos1 );

	// calculate second position
	trap->SV_SetBrushModel( (sharedEntity_t *)ent, ent->model );
	G_SetMovedir (&ent->s.angles, &ent->movedir);
	abs_movedir.x = fabsf(ent->movedir.x);
	abs_movedir.y = fabsf(ent->movedir.y);
	abs_movedir.z = fabsf(ent->movedir.z);
	VectorSubtract( &ent->r.maxs, &ent->r.mins, &size );
	distance = DotProduct( &abs_movedir, &size ) - lip;
	VectorMA( &ent->pos1, distance, &ent->movedir, &ent->pos2 );

	// if "start_open", reverse position 1 and 2
	if ( ent->spawnflags & 1 ) {
		vector3	temp;

		VectorCopy( &ent->pos2, &temp );
		VectorCopy( &ent->s.origin, &ent->pos2 );
		VectorCopy( &temp, &ent->pos1 );
	}

	InitMover( ent );

	ent->nextthink = level.time + sv_frametime->integer;

	if ( ! (ent->flags & FL_TEAMSLAVE ) ) {
		int health;

		G_SpawnInt( "health", "0", &health );
		if ( health ) {
			ent->takedamage = qtrue;
		}
		if ( ent->targetname || health ) {
			// non touch/shoot doors
			ent->think = Think_MatchTeam;
		} else {
			ent->think = Think_SpawnNewDoorTrigger;
		}
	}


}

/*

PLAT

*/

// Don't allow descent if a living player is on it
void Touch_Plat( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client || other->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	// delay return-to-pos1 by one second
	if ( ent->moverState == MOVER_POS2 ) {
		ent->nextthink = level.time + 1000;
	}
}

// If the plat is at the bottom position, start it going up
void Touch_PlatCenterTrigger(gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->parent->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent->parent, ent, other );
	}
}


// Spawn a trigger in the middle of the plat's low position
//	Elevator cars require that the trigger extend through the entire low position, not just sit on top of it.
void SpawnPlatTrigger( gentity_t *ent ) {
	gentity_t	*trigger;
	vector3	tmin, tmax;

	// the middle trigger will be a thin trigger just
	// above the starting position
	trigger = G_Spawn();
	trigger->classname = "plat_trigger";
	trigger->touch = Touch_PlatCenterTrigger;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->parent = ent;
	
	tmin.x = ent->pos1.x + ent->r.mins.x + 33;
	tmin.y = ent->pos1.y + ent->r.mins.y + 33;
	tmin.z = ent->pos1.z + ent->r.mins.z;

	tmax.x = ent->pos1.x + ent->r.maxs.x - 33;
	tmax.y = ent->pos1.y + ent->r.maxs.y - 33;
	tmax.z = ent->pos1.z + ent->r.maxs.z + 8;

	if ( tmax.x <= tmin.x ) {
		tmin.x = ent->pos1.x + (ent->r.mins.x + ent->r.maxs.x) *0.5f;
		tmax.x = tmin.x + 1;
	}
	if ( tmax.y <= tmin.y ) {
		tmin.y = ent->pos1.y + (ent->r.mins.y + ent->r.maxs.y) *0.5f;
		tmax.y = tmin.y + 1;
	}
	
	VectorCopy (&tmin, &trigger->r.mins);
	VectorCopy (&tmax, &trigger->r.maxs);

	trap->SV_LinkEntity ((sharedEntity_t *)trigger);
}


/*QUAKED func_plat (0 .5 .8) ?
Plats are always drawn in the extended position so they will light correctly.

"lip"		default 8, protrusion above rest position
"height"	total height of movement, defaults to model height
"speed"		overrides default 200.
"dmg"		overrides default 2
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_plat (gentity_t *ent) {
	float		lip, height;

	ent->sound1to2 = ent->sound2to1 = G_SoundIndex("sound/movers/plats/pt1_strt.wav");
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex("sound/movers/plats/pt1_end.wav");

	VectorClear (&ent->s.angles);

	G_SpawnFloat( "speed", "200", &ent->speed );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "wait", "1", &ent->wait );
	G_SpawnFloat( "lip", "8", &lip );

	ent->wait = 1000;

	// create second position
	trap->SV_SetBrushModel( (sharedEntity_t *)ent, ent->model );

	if ( !G_SpawnFloat( "height", "0", &height ) ) {
		height = (ent->r.maxs.z - ent->r.mins.z) - lip;
	}

	// pos1 is the rest (bottom) position, pos2 is the top
	VectorCopy( &ent->s.origin, &ent->pos2 );
	VectorCopy( &ent->pos2, &ent->pos1 );
	ent->pos1.z -= height;

	InitMover( ent );

	// touch function keeps the plat from returning while
	// a live player is standing on it
	ent->touch = Touch_Plat;

	ent->blocked = Blocked_Door;

	ent->parent = ent;	// so it can be treated as a door

	// spawn the trigger if one hasn't been custom made
	if ( !ent->targetname ) {
		SpawnPlatTrigger(ent);
	}
}


/*

	BUTTON

*/

void Touch_Button(gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent, other, other );
	}
}


/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of its angle, triggers all of its targets, waits some time, then returns to its original position where it can be triggered again.

"model2"	.md3 model to also draw
"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_button( gentity_t *ent ) {
	vector3		abs_movedir;
	float		distance;
	vector3		size;
	float		lip;

	ent->sound1to2 = G_SoundIndex("sound/movers/switches/butn2.wav");
	
	if ( !ent->speed ) {
		ent->speed = 40;
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->wait *= 1000;

	// first position
	VectorCopy( &ent->s.origin, &ent->pos1 );

	// calculate second position
	trap->SV_SetBrushModel( (sharedEntity_t *)ent, ent->model );

	G_SpawnFloat( "lip", "4", &lip );

	G_SetMovedir( &ent->s.angles, &ent->movedir );
	abs_movedir.x = fabsf(ent->movedir.x);
	abs_movedir.y = fabsf(ent->movedir.y);
	abs_movedir.z = fabsf(ent->movedir.z);
	VectorSubtract( &ent->r.maxs, &ent->r.mins, &size );
	distance = abs_movedir.x * size.x + abs_movedir.y * size.y + abs_movedir.z * size.z - lip;
	VectorMA (&ent->pos1, distance, &ent->movedir, &ent->pos2);

	if (ent->health) {
		// shootable button
		ent->takedamage = qtrue;
	} else {
		// touchable button
		ent->touch = Touch_Button;
	}

	InitMover( ent );
}



/*

	TRAIN

*/


#define TRAIN_START_ON		1
#define TRAIN_TOGGLE		2
#define TRAIN_BLOCK_STOPS	4

// The wait time at a corner has completed, so start moving again
void Think_BeginMoving( gentity_t *ent ) {
	ent->s.pos.trTime = level.time;
	ent->s.pos.trType = TR_LINEAR_STOP;
}

void Reached_Train( gentity_t *ent ) {
	gentity_t		*next;
	float			speed;
	vector3			move;
	float			length;

	// copy the appropriate values
	next = ent->nextTrain;
	if ( !next || !next->nextTrain ) {
		return;		// just stop
	}

	// fire all other targets
	G_UseTargets( next, NULL );

	// set the new trajectory
	ent->nextTrain = next->nextTrain;
	VectorCopy( &next->s.origin, &ent->pos1 );
	VectorCopy( &next->nextTrain->s.origin, &ent->pos2 );

	// if the path_corner has a speed, use that
	if ( next->speed ) {
		speed = next->speed;
	} else {
		// otherwise use the train's speed
		speed = ent->speed;
	}
	if ( speed < 1 ) {
		speed = 1;
	}

	// calculate duration
	VectorSubtract( &ent->pos2, &ent->pos1, &move );
	length = VectorLength( &move );

	ent->s.pos.trDuration = (int)(length * 1000 / speed);

	// Tequila comment: Be sure to send to clients after any fast move case
	ent->r.svFlags &= ~SVF_NOCLIENT;

	// Tequila comment: Fast move case
	if(ent->s.pos.trDuration<1) {
		// Tequila comment: As trDuration is used later in a division, we need to avoid that case now
		// With null trDuration,
		// the calculated rocks bounding box becomes infinite and the engine think for a short time
		// any entity is riding that mover but not the world entity... In rare case, I found it
		// can also stuck every map entities after func_door are used.
		// The desired effect with very very big speed is to have instant move, so any not null duration
		// lower than a frame duration should be sufficient.
		// Afaik, the negative case don't have to be supported.
		ent->s.pos.trDuration=1;

		// Tequila comment: Don't send entity to clients so it becomes really invisible 
		ent->r.svFlags |= SVF_NOCLIENT;
	}

	// looping sound
	ent->s.loopSound = next->soundLoop;

	// start it going
	SetMoverState( ent, MOVER_1TO2, level.time );

	// if there is a "wait" value on the target, don't start moving yet
	if ( next->wait ) {
		ent->nextthink = level.time + (int)(next->wait * 1000);
		ent->think = Think_BeginMoving;
		ent->s.pos.trType = TR_STATIONARY;
	}
}


// Link all the corners together
void Think_SetupTrainTargets( gentity_t *ent ) {
	gentity_t		*path, *next, *start;

	ent->nextTrain = G_Find( NULL, FOFS(targetname), ent->target );
	if ( !ent->nextTrain ) {
		trap->Print( "func_train at %s with an unfound target\n",
			vtos(&ent->r.absmin) );
		return;
	}

	start = NULL;
	for ( path = ent->nextTrain ; path != start ; path = next ) {
		if ( !start ) {
			start = path;
		}

		if ( !path->target ) {
			trap->Print( "Train corner at %s without a target\n",
				vtos(&path->s.origin) );
			return;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = NULL;
		do {
			next = G_Find( next, FOFS(targetname), path->target );
			if ( !next ) {
				trap->Print( "Train corner at %s without a target path_corner\n",
					vtos(&path->s.origin) );
				return;
			}
		} while ( strcmp( next->classname, "path_corner" ) );

		path->nextTrain = next;
	}

	// start the train moving from the first corner
	Reached_Train( ent );
}



/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8)
Train path corners.
Target: next path corner and other targets to fire
"speed" speed to move to the next corner
"wait" seconds to wait before behining move to next corner
*/
void SP_path_corner( gentity_t *self ) {
	if ( !self->targetname ) {
		trap->Print ("path_corner with no targetname at %s\n", vtos(&self->s.origin));
		G_FreeEntity( self );
		return;
	}
	// path corners don't need to be linked in
}



/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
A train is a mover that moves between path_corner target points.
Trains MUST HAVE AN ORIGIN BRUSH.
The train spawns at the first target it is pointing at.
"model2"	.md3 model to also draw
"speed"		default 100
"dmg"		default	2
"noise"		looping sound to play when the train is in motion
"target"	next path corner
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_train (gentity_t *self) {
	VectorClear (&self->s.angles);

	if (self->spawnflags & TRAIN_BLOCK_STOPS) {
		self->damage = 0;
	} else {
		if (!self->damage) {
			self->damage = 2;
		}
	}

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		trap->Print ("func_train without a target at %s\n", vtos(&self->r.absmin));
		G_FreeEntity( self );
		return;
	}

	trap->SV_SetBrushModel( (sharedEntity_t *)self, self->model );
	InitMover( self );

	self->reached = Reached_Train;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + sv_frametime->integer;
	self->think = Think_SetupTrainTargets;
}

/*

	STATIC

*/


/*QUAKED func_static (0 .5 .8) ?
A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_static( gentity_t *ent ) {
	trap->SV_SetBrushModel( (sharedEntity_t *)ent, ent->model );
	InitMover( ent );
	VectorCopy( &ent->s.origin, &ent->s.pos.trBase );
	VectorCopy( &ent->s.origin, &ent->r.currentOrigin );
}


/*

	ROTATING

*/


/*QUAKED func_rotating (0 .5 .8) ? START_ON - X_AXIS Y_AXIS
You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"model2"	.md3 model to also draw
"speed"		determines how fast it moves; default value is 100.
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_rotating (gentity_t *ent) {
	if ( !ent->speed ) {
		ent->speed = 100;
	}

	// set the axis of rotation
	ent->s.apos.trType = TR_LINEAR;
		 if ( ent->spawnflags & 4 )		ent->s.apos.trDelta.roll	= ent->speed;
	else if ( ent->spawnflags & 8 )		ent->s.apos.trDelta.pitch	= ent->speed;
	else								ent->s.apos.trDelta.yaw		= ent->speed;

	if (!ent->damage)
		ent->damage = 2;

	trap->SV_SetBrushModel( (sharedEntity_t *)ent, ent->model );
	InitMover( ent );

	VectorCopy( &ent->s.origin, &ent->s.pos.trBase );
	VectorCopy( &ent->s.pos.trBase, &ent->r.currentOrigin );
	VectorCopy( &ent->s.apos.trBase, &ent->r.currentAngles );

	trap->SV_LinkEntity( (sharedEntity_t *)ent );
}


/*

	BOBBING

*/


/*QUAKED func_bobbing (0 .5 .8) ? X_AXIS Y_AXIS
Normally bobs on the Z axis
"model2"	.md3 model to also draw
"height"	amplitude of bob (32 default)
"speed"		seconds to complete a bob cycle (4 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_bobbing (gentity_t *ent) {
	float		height;
	float		phase;

	G_SpawnFloat( "speed", "4", &ent->speed );
	G_SpawnFloat( "height", "32", &height );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "phase", "0", &phase );

	trap->SV_SetBrushModel( (sharedEntity_t *)ent, ent->model );
	InitMover( ent );

	VectorCopy( &ent->s.origin, &ent->s.pos.trBase );
	VectorCopy( &ent->s.origin, &ent->r.currentOrigin );

	ent->s.pos.trDuration = (int)(ent->speed * 1000);
	ent->s.pos.trTime = (int)(ent->s.pos.trDuration * phase);
	ent->s.pos.trType = TR_SINE;

	// set the axis of bobbing
		 if ( ent->spawnflags & 1 )		ent->s.pos.trDelta.x = height;
	else if ( ent->spawnflags & 2 )		ent->s.pos.trDelta.y = height;
	else								ent->s.pos.trDelta.z = height;
}

/*

	PENDULUM

*/


/*QUAKED func_pendulum (0 .5 .8) ?
You need to have an origin brush as part of this entity.
Pendulums always swing north / south on unrotated models.  Add an angles field to the model to allow rotation in other directions.
Pendulum frequency is a physical constant based on the length of the beam and gravity.
"model2"	.md3 model to also draw
"speed"		the number of degrees each way the pendulum swings, (30 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_pendulum(gentity_t *ent) {
	float		freq;
	float		length;
	float		phase;
	float		speed;

	G_SpawnFloat( "speed", "30", &speed );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "phase", "0", &phase );

	trap->SV_SetBrushModel( (sharedEntity_t *)ent, ent->model );

	// find pendulum length
	length = fabsf( ent->r.mins.z );
	if ( length < 8 ) {
		length = 8;
	}

	freq = 1 / ( M_PI * 2 ) * sqrtf( pm_gravity->value / ( 3 * length ) );

	ent->s.pos.trDuration = (int)( 1000 / freq );

	InitMover( ent );

	VectorCopy( &ent->s.origin, &ent->s.pos.trBase );
	VectorCopy( &ent->s.origin, &ent->r.currentOrigin );
	VectorCopy( &ent->s.angles, &ent->s.apos.trBase );

	ent->s.apos.trDuration = (int)(1000 / freq);
	ent->s.apos.trTime = (int)(ent->s.apos.trDuration * phase);
	ent->s.apos.trType = TR_SINE;
	ent->s.apos.trDelta.z = speed;
}
