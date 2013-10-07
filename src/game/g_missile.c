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

#define	MISSILE_PRESTEP_TIME	50

//Raz: Fixing this in several ways. Some communities like the 'broken' behaviour.
//	g_randFix 0 == Broken on Linux, fine on Windows
//	g_randFix 1 == Fine on Linux, fine on Windows
//	g_randFix 2 == Broken on Linux, broken on Windows.
float RandFloat(float min, float max) {
	int randActual = rand();
	float randMax = 32768.0f;
#ifdef _WIN32
	if ( g_randFix->integer == 2 )
		randActual = (randActual<<16)|randActual;
#elif defined(__GCC__)
	if ( g_randFix->integer == 1 )
		randMax = RAND_MAX;
#endif
	return ((randActual * (max - min)) / randMax) + min;
}

void G_DeflectMissile( gentity_t *ent, gentity_t *missile, vector3 *forward ) 
{
	vector3	bounce_dir;
	int		i;
	float	speed;
	int		isowner = 0;
	vector3 missile_dir;

	if (missile->r.ownerNum == ent->s.number)
	{ //the original owner is bouncing the missile, so don't try to bounce it back at him
		isowner = 1;
	}

	//save the original speed
	speed = VectorNormalize( &missile->s.pos.trDelta );

	if (ent->client)
	{
		//VectorSubtract( ent->r.currentOrigin, missile->r.currentOrigin, missile_dir );
		AngleVectors(&ent->client->ps.viewangles, &missile_dir, 0, 0);
		VectorCopy(&missile_dir, &bounce_dir);
		//VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( &bounce_dir, DotProduct( forward, &missile_dir ), &bounce_dir );
		VectorNormalize( &bounce_dir );
	}
	else
	{
		VectorCopy(forward, &bounce_dir);
		VectorNormalize(&bounce_dir);
	}

	for ( i = 0; i < 3; i++ )
	{
		bounce_dir.data[i] += RandFloat( -1.0f, 1.0f );
	}

	VectorNormalize( &bounce_dir );
	VectorScale( &bounce_dir, speed, &missile->s.pos.trDelta );
	missile->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( &missile->r.currentOrigin, &missile->s.pos.trBase );
	//you are mine, now!
	missile->r.ownerNum = ent->s.number;

	/*RAZTODO: Rocket locking
	if ( missile->s.weapon == WP_ROCKET_LAUNCHER )
	{//stop homing
		missile->think = 0;
		missile->nextthink = 0;
	}*/
}

void G_BounceMissile_Q3( gentity_t *ent, trace_t *trace ) {
	vector3	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + (int)(( level.time - level.previousTime ) * trace->fraction);
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, &velocity );
	dot = DotProduct( &velocity, &trace->plane.normal );
	VectorMA( &velocity, -2*dot, &trace->plane.normal, &ent->s.pos.trDelta );

	if ( ent->flags & FL_BOUNCE_HALF ) {
		VectorScale( &ent->s.pos.trDelta, 0.65f, &ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal.z > 0.2f && VectorLength( &ent->s.pos.trDelta ) < 40 ) {
			G_SetOrigin( ent, &trace->endpos );
			ent->s.time = level.time / 4;
			return;
		}
	}

	VectorAdd( &ent->r.currentOrigin, &trace->plane.normal, &ent->r.currentOrigin);
	VectorCopy( &ent->r.currentOrigin, &ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}

void G_BounceMissile_JA( gentity_t *ent, trace_t *trace ) {
	vector3	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + (int)(( level.time - level.previousTime ) * trace->fraction);
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, &velocity );
	dot = DotProduct( &velocity, &trace->plane.normal );
	VectorMA( &velocity, -2*dot, &trace->plane.normal, &ent->s.pos.trDelta );


	if ( ent->flags & FL_BOUNCE_SHRAPNEL ) 
	{
		VectorScale( &ent->s.pos.trDelta, 0.25f, &ent->s.pos.trDelta );
		ent->s.pos.trType = TR_GRAVITY;

		// check for stop
		if ( trace->plane.normal.z > 0.7f && ent->s.pos.trDelta.z < 40 ) //this can happen even on very slightly sloped walls, so changed it from > 0 to > 0.7
		{
			G_SetOrigin( ent, &trace->endpos );
			ent->nextthink = level.time + 100;
			return;
		}
	}
	else if ( ent->flags & FL_BOUNCE_HALF ) 
	{
		VectorScale( &ent->s.pos.trDelta, 0.65f, &ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal.z > 0.2f && VectorLength( &ent->s.pos.trDelta ) < 40 ) 
		{
			G_SetOrigin( ent, &trace->endpos );
			return;
		}
	}

	VectorAdd( &ent->r.currentOrigin, &trace->plane.normal, &ent->r.currentOrigin);
	VectorCopy( &ent->r.currentOrigin, &ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;

	if (ent->bounceCount != -5)
		ent->bounceCount--;
}
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	if ( g_bounceMissileStyle->integer == 0 )
		G_BounceMissile_Q3( ent, trace );
	else
		G_BounceMissile_JA( ent, trace );
}
//QtZ: From Jedi Academy
gentity_t *CreateMissile( vector3 *org, vector3 *dir, float vel, int life, gentity_t *owner)
{
	gentity_t *missile = G_Spawn();
	
	missile->nextthink = level.time + life;
	missile->think = G_FreeEntity;
	missile->s.eType = ET_MISSILE;
	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	missile->parent = owner;
	missile->r.ownerNum = owner->s.number;

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time;// - MISSILE_PRESTEP_TIME;	// NOTENOTE This is a Quake 3 addition over JK2
	missile->target_ent = NULL;

	VectorSnap( org );
	VectorCopy( org, &missile->s.pos.trBase );
	VectorScale( dir, vel, &missile->s.pos.trDelta );
	VectorCopy( org, &missile->r.currentOrigin );
	VectorSnap( &missile->s.pos.trDelta );

	//Raz: Added
	missile->knockbackMulti = 1.0f;
	missile->knockbackMultiSelf = 1.0f;

	return missile;
}

// Explode a missile without an impact
void G_ExplodeMissile( gentity_t *ent ) {
	vector3 dir = { 0.0f, 0.0f, 1.0f }, origin = { 0.0f };

	BG_EvaluateTrajectory( &ent->s.pos, level.time, &origin );
	VectorSnap( &origin );
	G_SetOrigin( ent, &origin );

	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( &dir ) );
	ent->freeAfterEvent = qtrue;
	ent->takedamage = qfalse;

	if ( ent->splashDamage || ent->splashRadius )
	{
		if ( G_RadiusDamage( &ent->r.currentOrigin, ent->parent, (float)ent->splashDamage, (float)ent->splashRadius, ent, ent, ent->splashMethodOfDeath ) )
		{
			if ( ent->parent )
				g_entities[ent->parent->s.number].client->accuracy_hits++;
			else if ( ent->activator )
				g_entities[ent->activator->s.number].client->accuracy_hits++;
		}
	}

	trap->SV_LinkEntity( (sharedEntity_t *)ent );
}

void G_MissileImpact( gentity_t *ent, trace_t *trace ) {
	gentity_t		*other;
	qboolean		hitClient = qfalse;
	other = &g_entities[trace->entityNum];

	// check for bounce
	if ( !other->takedamage &&
		(ent->bounceCount > 0 || ent->bounceCount == -5) &&
		( ent->flags & ( FL_BOUNCE | FL_BOUNCE_HALF ) ) ) {
		G_BounceMissile( ent, trace );
	//	G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		return;
	}

	if ( other->s.eType == ET_MISSILE && ent->s.weapon == WP_QUANTIZER && other->s.weapon == WP_MORTAR )
	{//Quantizer can deflect mortar shots :3
		G_DeflectMissile( ent, other, &ent->s.pos.trDelta );
		other->r.ownerNum = ent->r.ownerNum;
		G_FreeEntity( ent );
		return;
	}

	// impact damage
	if (other->takedamage) {
		// FIXME: wrong damage direction?
		if ( ent->damage ) {
			vector3	velocity;

			if( LogAccuracyHit( other, &g_entities[ent->r.ownerNum] ) ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, &velocity );
			if ( VectorLength( &velocity ) == 0 ) {
				velocity.z = 1;	// stepped on a grenade
			}
			//Raz: The inflictor is the missile, for direct hits from quantizer/RLauncher/etc
			G_Damage( other, ent, &g_entities[ent->r.ownerNum], ent, NULL/*velocity*/, &ent->s.origin, ent->damage, 0, ent->methodOfDeath );
		}
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && other->client ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( &trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( &trace->plane.normal ) );
	} else {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( &trace->plane.normal ) );
	}

	ent->freeAfterEvent = qtrue;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	VectorSnapTowards( &trace->endpos, &ent->s.pos.trBase );	// save net bandwidth

	G_SetOrigin( ent, &trace->endpos );

	// splash damage (doesn't apply to person directly hit)
#if 0
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, 
			other, ent->splashMethodOfDeath ) ) {
			if( !hitClient ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
			}
		}
	}
#else
	//QtZ
	if ( (ent->splashDamage || ent->splashRadius)
		&& G_RadiusDamage( &trace->endpos, ent->parent, (float)ent->splashDamage, (float)ent->splashRadius, other, ent, ent->splashMethodOfDeath )
		&& !hitClient && g_entities[ent->r.ownerNum].client )
		g_entities[ent->r.ownerNum].client->accuracy_hits++;
#endif

	trap->SV_LinkEntity( (sharedEntity_t *)ent );
}

void G_RunMissile( gentity_t *ent ) {
	vector3		origin;
	trace_t		tr;
	int			passent;

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, &origin );

	// if this missile bounced off an invulnerability sphere
	if ( ent->target_ent ) {
		passent = ent->target_ent->s.number;
	}
	// prox mines that left the owner bbox will attach to anything, even the owner
	else {
		// ignore interactions with the missile owner
		passent = ent->r.ownerNum;
	}
	// trace a line from the previous position to the current position
	trap->SV_Trace( &tr, &ent->r.currentOrigin, &ent->r.mins, &ent->r.maxs, &origin, passent, ent->clipmask );

	if ( tr.startsolid || tr.allsolid ) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap->SV_Trace( &tr, &ent->r.currentOrigin, &ent->r.mins, &ent->r.maxs, &ent->r.currentOrigin, passent, ent->clipmask );
		tr.fraction = 0;
	}
	else {
		VectorCopy( &tr.endpos, &ent->r.currentOrigin );
	}

	trap->SV_LinkEntity( (sharedEntity_t *)ent );

	if ( tr.fraction != 1 ) {
		// never explode or bounce on sky
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent) {
				ent->parent->client->hook = NULL;
			}
			G_FreeEntity( ent );
			return;
		}
		G_MissileImpact( ent, &tr );
		if ( ent->s.eType != ET_MISSILE ) {
			return;		// exploded
		}
	}

	// check think function after bouncing
	G_RunThink( ent );
}
