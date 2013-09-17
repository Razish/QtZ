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
// g_weapon.c 
// perform the server side effects of a weapon firing

#include "g_local.h"

static	float	s_quadFactor;
static	vector3	forward, right, up;
static	vector3	muzzle;

/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vector3 *start, vector3 *impact, vector3 *dir, vector3 *endout ) {
	vector3 v, newv;
	float dot;

	VectorSubtract( impact, start, &v );
	dot = DotProduct( &v, dir );
	VectorMA( &v, -2*dot, dir, &newv );

	VectorNormalize(&newv);
	VectorMA(impact, 8192, &newv, endout);
}

/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if ( !target->takedamage ||
		target == attacker ||
		!target->client || !attacker->client ||
		target->client->ps.stats[STAT_HEALTH] <= 0 ||
		OnSameTeam( target, attacker ) )
		return qfalse;

	return qtrue;
}


/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePoint ( gentity_t *ent, vector3 *forward, vector3 *right, vector3 *up, vector3 *muzzlePoint ) {
	VectorCopy( &ent->s.pos.trBase, muzzlePoint );
	muzzlePoint->z += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	VectorSnap( muzzlePoint );
}

/*
===============
CalcMuzzlePointOrigin

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePointOrigin ( gentity_t *ent, vector3 *origin, vector3 *forward, vector3 *right, vector3 *up, vector3 *muzzlePoint ) {
	VectorCopy( &ent->s.pos.trBase, muzzlePoint );
	muzzlePoint->z += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	VectorSnap( muzzlePoint );
}

void RocketDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod )
{
	self->die = 0;
	self->r.contents = 0;

	G_ExplodeMissile( self );

	self->think = G_FreeEntity;
	self->nextthink = level.time;
}

#define QUANTIZER_DAMAGE (8)
#define QUANTIZER_VELOCITY (4200)
#define QUANTIZER_LIFE (10000)
#define QUANTIZER_SPLASHDAMAGE (15) //(11)
#define QUANTIZER_SPLASHRADIUS (64)
#define QUANTIZER_SIZE (6.0f)
static void WP_Quantizer_Fire( gentity_t *ent, int special )
{
	gentity_t *missile = NULL;
	vector3 dir = { 0.0f }, angs = { 0.0f };

	vectoangles( &forward, &angs );
	AngleVectors( &angs, &dir, NULL, NULL );

	missile = CreateMissile( &muzzle, &dir, QUANTIZER_VELOCITY, QUANTIZER_LIFE, ent );
	missile->think = G_ExplodeMissile;
	missile->classname = "quantizer_proj";
	missile->s.weapon = WP_QUANTIZER;
	missile->damage = QUANTIZER_DAMAGE;
	missile->splashDamage = QUANTIZER_SPLASHDAMAGE;
	missile->splashRadius = QUANTIZER_SPLASHRADIUS;
	missile->knockbackMulti = 4.0f;
	missile->knockbackMultiSelf = 5.0f;
	missile->knockbackDampVert = 0.45f;
	missile->dflags = DAMAGE_VERTICAL_KNOCKBACK;
	missile->methodOfDeath = MOD_QUANTIZER;
	missile->splashMethodOfDeath = MOD_QUANTIZER;
	missile->clipmask = MASK_SHOT;
	missile->bounceCount = 0;

	VectorSet( &missile->r.maxs, QUANTIZER_SIZE, QUANTIZER_SIZE, QUANTIZER_SIZE );
	VectorScale( &missile->r.maxs, -1, &missile->r.mins );
}


#define REPEATER_DAMAGE (7)
#define REPEATER_VELOCITY (8000)
#define REPEATER_LIFE (10000)
#define REPEATER_SIZE (3.0f)
static void WP_Repeater_Fire( gentity_t *ent, int special )
{
	gentity_t *missile = NULL;
	vector3 dir = { 0.0f }, angs = { 0.0f };

	vectoangles( &forward, &angs );
	AngleVectors( &angs, &dir, NULL, NULL );

	missile = CreateMissile( &muzzle, &dir, REPEATER_VELOCITY, REPEATER_LIFE, ent );
	missile->think = G_ExplodeMissile;
	missile->classname = "repeater_proj";
	missile->s.weapon = WP_REPEATER;
	missile->damage = REPEATER_DAMAGE;
	missile->dflags = DAMAGE_NO_KNOCKBACK;
	missile->methodOfDeath = MOD_REPEATER;
	missile->clipmask = MASK_SHOT;
	missile->bounceCount = 0;

	VectorSet( &missile->r.maxs, REPEATER_SIZE, REPEATER_SIZE, REPEATER_SIZE );
	VectorScale( &missile->r.maxs, -1, &missile->r.mins );
}


#define SPLICER_DAMAGE (3)
#define SPLICER_SIZE (0.5f)

void WP_Splicer_Fire( gentity_t *ent, int special ) {
	trace_t		trace;
	vector3		start = { 0.0f }, end = { 0.0f }, mins = { -SPLICER_SIZE }, maxs = { SPLICER_SIZE };
	gentity_t	*traceEnt=NULL, *tent=NULL;
	int			damage = (int)(SPLICER_DAMAGE * s_quadFactor);
	int			range = weaponData[WP_SPLICER].range;

	// time to compensate for lag
	if ( g_delagHitscan->integer && ent->client && !(ent->r.svFlags & SVF_BOT) )
		G_TimeShiftAllClients( ent->client->pers.cmd.serverTime, ent );

	//Start at eye position
	VectorCopy( &ent->client->ps.origin, &start );
	start.z += ent->client->ps.viewheight;

	VectorMA( &start, (float)range, &forward, &end );

	trap->SV_Trace( &trace, &muzzle, &mins, &maxs, &end, ent->s.number, MASK_SHOT );

	if ( trace.entityNum == ENTITYNUM_NONE )
	{
		if ( g_delagHitscan->integer && ent->client && !(ent->r.svFlags & SVF_BOT) )
			G_UnTimeShiftAllClients( ent );
		return;
	}

	traceEnt = &g_entities[trace.entityNum];

	//RAZTODO: headshots? does it matter?

	if ( traceEnt->takedamage )
		G_Damage( traceEnt, ent, ent, NULL, &forward, &trace.endpos, damage, 0, MOD_SPLICER );

	if ( traceEnt->takedamage && traceEnt->client )
	{
		tent = G_TempEntity( &trace.endpos, EV_MISSILE_HIT );
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte( &trace.plane.normal );
		tent->s.weapon = ent->s.weapon;
		if ( LogAccuracyHit( traceEnt, ent ) )
			ent->client->accuracy_hits++;
	}
	else if ( !(trace.surfaceFlags & SURF_NOIMPACT) )
	{
		tent = G_TempEntity( &trace.endpos, EV_MISSILE_MISS );
		tent->s.eventParm = DirToByte( &trace.plane.normal );
	}

	if ( g_delagHitscan->integer && ent->client && !(ent->r.svFlags & SVF_BOT) )
		G_UnTimeShiftAllClients( ent );
}


#define MORTAR_DAMAGE (40)
#define MORTAR_VELOCITY (1400)
#define MORTAR_LIFE (10000)
#define MORTAR_SPLASHDAMAGE (35)
#define MORTAR_SPLASHRADIUS (160)
#define MORTAR_SIZE (4.0f)
static void WP_Mortar_Fire( gentity_t *ent, int special )
{
	gentity_t	*missile = NULL;
	vector3		dir = { 0.0f }, angs = { 0.0f };
	int			damage = (int)(MORTAR_DAMAGE * s_quadFactor);

	vectoangles( &forward, &angs );
	angs.pitch -= 9.5f;
	AngleVectors( &angs, &dir, NULL, NULL );

	missile = CreateMissile( &muzzle, &dir, MORTAR_VELOCITY, MORTAR_LIFE, ent );
	missile->think = G_ExplodeMissile;
	missile->classname = "mortar_proj";
	missile->s.weapon = WP_MORTAR;
	missile->damage = damage;
	missile->splashDamage = MORTAR_SPLASHDAMAGE;
	missile->splashRadius = MORTAR_SPLASHRADIUS;
	missile->knockbackMulti = 3.0f;
	missile->knockbackMultiSelf = 1.65f;
	missile->knockbackDampVert = 0.85f;
	missile->dflags |= DAMAGE_VERTICAL_KNOCKBACK;
	missile->methodOfDeath = MOD_MORTAR;
	missile->splashMethodOfDeath = MOD_MORTAR;
	missile->clipmask = MASK_SHOT;
	missile->bounceCount = 0;
	missile->s.pos.trType = TR_GRAVITY;
//	missile->s.pos.trDelta[2] -= 32.0f;

	missile->takedamage = qtrue;
	missile->health = 55;
	missile->r.contents = MASK_SHOT;
	missile->die = RocketDie;

	VectorSet( &missile->r.maxs, MORTAR_SIZE, MORTAR_SIZE, MORTAR_SIZE );
	VectorScale( &missile->r.maxs, -1, &missile->r.mins );
}


// see bg_weapons.h for balance reasoning
#define DIVERGENCE_DAMAGE (115)
#define DIVERGENCE_HEADSHOT_MULTIPLIER (1.15f)
#define DIVERGENCE_TRACES (3)
#define DIVERGENCE_RANGE (8192)
#define DIVERGENCE_SIZE (0.5f)

qboolean G_CanDisruptify( gentity_t *ent ) {
	if ( !ent || !ent->inuse || !ent->client )
		return qtrue;

	return qfalse;
}

static void WP_Divergence_Fire( gentity_t *ent, int special )
{
	qboolean render_impact = qtrue;
	vector3 start = { 0.0f }, end = { 0.0f }, mins = { -DIVERGENCE_SIZE }, maxs = { DIVERGENCE_SIZE };
	trace_t tr = { 0 };
	gentity_t *traceEnt = NULL, *tent = NULL;
	int range = weaponData[WP_DIVERGENCE].range;
	int ignore = ent->s.number, traces = DIVERGENCE_TRACES;
	float headshotMulti = 1.0f;
	float damage = ((float)special/weaponData[WP_DIVERGENCE].maxFireTime)*(float)DIVERGENCE_DAMAGE;
	qboolean headshot=qfalse, airshot=qfalse, amazing=qfalse;
	int hits = 0;

	// time to compensate for lag
	if ( g_delagHitscan->integer && ent->client && !(ent->r.svFlags & SVF_BOT) )
		G_TimeShiftAllClients( ent->client->pers.cmd.serverTime, ent );

	//Start at eye position
	VectorCopy( &ent->client->ps.origin, &start );
	start.z += ent->client->ps.viewheight;

	while ( traces )
	{
		VectorMA( &start, (float)range, &forward, &end );
		trap->SV_Trace( &tr, &start, &mins, &maxs, &end, ignore, MASK_SHOT );

		traceEnt = &g_entities[tr.entityNum];

		if ( tr.allsolid || tr.startsolid )
			break;

		if ( !traceEnt->takedamage || !traceEnt->client )
			break;

		if ( traceEnt && traceEnt->client )
		{
			vector3 preAng = { 0.0f };
			int preHealth = traceEnt->health, preLegs = 0, preTorso = 0;
			int hitLoc = HL_NONE;

			if ( traceEnt->client )
			{
				preLegs = traceEnt->client->ps.legsAnim;
				preTorso = traceEnt->client->ps.torsoAnim;
				VectorCopy( &traceEnt->client->ps.viewangles, &preAng );
			}

			if ( LogAccuracyHit( traceEnt, ent ) )
				ent->client->accuracy_hits++;

			if ( !OnSameTeam( ent, traceEnt ) && g_enableHeadshots->integer && G_GetHitLocation( traceEnt, &tr.endpos ) == HL_HEAD )
			{
				headshot = qtrue;
				ent->client->tracking.headshotCount++;
				headshotMulti = DIVERGENCE_HEADSHOT_MULTIPLIER;
			}

			G_Damage( traceEnt, ent, ent, NULL, &forward, &tr.endpos, (int)(damage*s_quadFactor*headshotMulti), DAMAGE_NORMAL, MOD_DIVERGENCE );
			if ( !OnSameTeam( ent, traceEnt ) )
				hits++;

			if ( traceEnt->client )
			{
				if ( !OnSameTeam( ent, traceEnt ) && traceEnt->client->ps.groundEntityNum == ENTITYNUM_NONE
					/*&& ent->client->ps.groundEntityNum == ENTITYNUM_NONE*/ )
				{
					airshot = qtrue;
					ent->client->tracking.airshotCount++;
				}

				if ( preHealth > 0 && traceEnt->health <= 0 && G_CanDisruptify( traceEnt ) )
				{//disintegrate!
					vector3 dir = { 0.0f };
					VectorSubtract( &traceEnt->client->ps.origin, &ent->client->ps.origin, &dir );
					if ( hits > 1 || VectorLength( &dir ) > 2560.0f )
					{
						amazing = qtrue;
						ent->client->tracking.amazingCount++;
					}
				}
			}

			VectorCopy( &tr.endpos, &start );
			ignore = tr.entityNum;
			memset( &tr, 0, sizeof( tr ) );
			traces--;
			headshotMulti = 1.0f;
			hitLoc = HL_NONE;
			continue;
		}
		break;
	}

	if ( tr.surfaceFlags & SURF_NOIMPACT ) 
		render_impact = qfalse;

	tent = G_TempEntity( &tr.endpos, EV_HITSCANTRAIL );
	tent->s.clientNum = ent->s.number;
	tent->s.eventParm = DirToByte( &tr.plane.normal );
	tent->s.weapon = WP_DIVERGENCE;

	// reward sound for headshots, airshots, amazing
	//	done after all traces so it will only trigger once per frame
	if ( headshot )
		ent->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HEADSHOT;
	if ( airshot )
		ent->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_AIRSHOT;
	if ( amazing )
		ent->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_AMAZING;

	if ( g_delagHitscan->integer && ent->client && !(ent->r.svFlags & SVF_BOT) )
		G_UnTimeShiftAllClients( ent );
}

/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent, int special )
{
	s_quadFactor = ent->client->ps.powerups[PW_QUAD] ? g_quadFactor->value : 1.0f;

	// track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked
	ent->client->accuracy_shots++;

	// set aiming directions
	AngleVectors (&ent->client->ps.viewangles, &forward, &right, &up);

	CalcMuzzlePointOrigin ( ent, &ent->client->oldOrigin, &forward, &right, &up, &muzzle );

	// fire the specific weapon
	switch( ent->s.weapon ) {
		//RAZMARK: ADDING NEW WEAPONS
	case WP_QUANTIZER:
		WP_Quantizer_Fire( ent, special );
		break;
	case WP_REPEATER:
		WP_Repeater_Fire( ent, special );
		break;
	case WP_SPLICER:
		WP_Splicer_Fire( ent, special );
		break;
	case WP_MORTAR:
		WP_Mortar_Fire( ent, special );
		break;
	case WP_DIVERGENCE:
		WP_Divergence_Fire( ent, special );
		break;
	default:
		break;
	}
}
