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
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

#if defined(PROJECT_GAME)
	#include "g_local.h"
#elif defined(PROJECT_CGAME)
	#include "cg_local.h"
#endif

pmove_t		*pm;
pml_t		pml;

// movement parameters
float pm_stopspeed = 100.0f;
float pm_duckScale = 0.25f;
float pm_swimScale = 0.50f;
float pm_wateraccelerate = 4.0f;
float pm_flyaccelerate = 8.0f;
float pm_waterfriction = 1.0f;
float pm_spectatorfriction = 5.0f;

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent ) {
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum ) {
	int		i;

	if ( entityNum == ENTITYNUM_WORLD ) {
		return;
	}
	if ( pm->numtouch == MAXTOUCH ) {
		return;
	}

	// see if it is already added
	for ( i = 0 ; i < pm->numtouch ; i++ ) {
		if ( pm->touchents[ i ] == entityNum ) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
===================
PM_StartTorsoAnim
===================
*/
static void PM_StartTorsoAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	pm->ps->torsoAnim = ( ( pm->ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}
static void PM_StartLegsAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}

static void PM_ContinueLegsAnim( int anim ) {
	if ( ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartLegsAnim( anim );
}

static void PM_ContinueTorsoAnim( int anim ) {
	if ( ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
	if ( pm->ps->torsoTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartTorsoAnim( anim );
}

static void PM_ForceLegsAnim( int anim ) {
	pm->ps->legsTimer = 0;
	PM_StartLegsAnim( anim );
}


/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( vector3 *in, vector3 *normal, vector3 *out, float overbounce ) {
	float	backoff;
	float	change;
	int		i;
	
	backoff = DotProduct( in, normal );
	
	if ( backoff < 0 )	backoff *= overbounce;
	else				backoff /= overbounce;

	for ( i=0; i<3; i++ ) {
		change = normal->data[i]*backoff;
		out->data[i] = in->data[i] - change;
	}
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void ) {
	vector3	vec;
	vector3 *vel;
	float	speed, newspeed, control;
	float	drop;
	
	vel = &pm->ps->velocity;
	
	VectorCopy( vel, &vec );
	if ( pml.walking )
		vec.z = 0;	// ignore slope movement

	speed = VectorLength(&vec);
	if (speed < 1) {
		vel->x = 0;
		vel->y = 0;		// allow sinking underwater
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply ground friction
	if ( pm->waterlevel <= 1 
		&& pml.walking
		&& !(pml.groundTrace.surfaceFlags & SURF_SLICK) )
	{// if getting knocked back, no friction
		if ( !(pm->ps->pm_flags & PMF_TIME_KNOCKBACK) ) {
			control = speed < pm_stopspeed ? pm_stopspeed : speed;
			drop += control * pm_friction->value * pml.frametime;
		}
	}

	// apply water friction even if just wading
	if ( pm->waterlevel )
		drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;

	if ( pm->ps->pm_type == PM_SPECTATOR )
		drop += speed*pm_spectatorfriction*pml.frametime;

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel->x = vel->x * newspeed;
	vel->y = vel->y * newspeed;
	vel->z = vel->z * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( vector3 *wishDir, float wishSpeed, float accel ) {
	float addSpeed, accelSpeed, currentSpeed;
	int i;

	currentSpeed = DotProduct( &pm->ps->velocity, wishDir );
	addSpeed = wishSpeed - currentSpeed;

	if ( addSpeed <= 0 )
		return;

	accelSpeed = accel*pml.frametime*wishSpeed;
	if (accelSpeed > addSpeed)
		accelSpeed = addSpeed;

	for ( i=0; i<3; i++ )
		pm->ps->velocity.data[i] += accelSpeed * wishDir->data[i];
}

/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd ) {
	int		max = abs( cmd->forwardmove );
	float	total;
	float	scale;

	if ( abs( cmd->rightmove ) > max )	max = abs( cmd->rightmove );
	if ( abs( cmd->upmove ) > max )		max = abs( cmd->upmove );

	if ( !max )
		return 0;

	total = sqrtf( (float)(cmd->forwardmove*cmd->forwardmove + cmd->rightmove*cmd->rightmove + cmd->upmove*cmd->upmove) );
	scale = pm_speed->value * (float)max / ( 127.0f * total );

	return scale;
}

/*
================
PM_SetMovementDir

Determine the rotation of the legs relative
to the facing dir
================
*/
static void PM_SetMovementDir( void ) {
		 if ( pm->cmd.rightmove == 0	&& pm->cmd.forwardmove > 0 )	pm->ps->movementDir = 0; // W
	else if ( pm->cmd.rightmove < 0		&& pm->cmd.forwardmove > 0 )	pm->ps->movementDir = 1; // W + A
	else if ( pm->cmd.rightmove < 0		&& pm->cmd.forwardmove == 0 )	pm->ps->movementDir = 2; // A
	else if ( pm->cmd.rightmove < 0		&& pm->cmd.forwardmove < 0 )	pm->ps->movementDir = 3; // A + S
	else if ( pm->cmd.rightmove == 0	&& pm->cmd.forwardmove < 0 )	pm->ps->movementDir = 4; // S
	else if ( pm->cmd.rightmove > 0		&& pm->cmd.forwardmove < 0 )	pm->ps->movementDir = 5; // S + D
	else if ( pm->cmd.rightmove > 0		&& pm->cmd.forwardmove == 0 )	pm->ps->movementDir = 6; // D
	else if ( pm->cmd.rightmove > 0		&& pm->cmd.forwardmove > 0 )	pm->ps->movementDir = 7; // D + W
//	else																pm->ps->movementDir = 8; // ???
}

/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump( void ) {
	float dot = 0.0f;
	if ( pm->ps->pm_flags & PMF_RESPAWNED )
		return qfalse;		// don't allow jump until all buttons are up

	//not holding jump
	if ( pm->cmd.upmove < 10 )
		return qfalse;

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{//Jump off walls if we're in the air and it's been 1.25 seconds since we did it last.
		if ( pm_wallJumpEnable->boolean
			&& !(pm->ps->pm_flags & PMF_JUMP_HELD)
			&& pm->cmd.forwardmove > 0
			&& pm->ps->velocity.z > -256
			&& !pm->ps->wallJumpTime )
		{//debouncer finished, pressing jump, not falling too far
			vector3	forward, back;
			vector3 mins={ -0.5f, -0.5f, -0.5f }, maxs={ 0.5f, 0.5f, 0.5f };
			vector3 angs = { 0.0f, 0.0f, 0.0f };
			trace_t	trace;

			angs.yaw = pm->ps->viewangles.yaw;

			//This will only use our yaw angle - we'll offset the pitch later to get a boost up
			AngleVectors( &angs, &forward, NULL, NULL );
			VectorMA( &pm->ps->origin, -36, &forward, &back );
			pm->trace( &trace, &pm->ps->origin, &mins, &maxs, &back, pm->ps->clientNum, pm->tracemask );

			if ( trace.fraction < 1.0f )
			{//wall behind us, jump off it
				angs.pitch = -17.0f; //aim up a bit
				AngleVectors( &angs, &forward, NULL, NULL );
				//push up to ~360 ups, based on how close we are to the wall
				VectorMA( &pm->ps->velocity, (pm_jumpVelocity->value/2.0f) + ( (pm_jumpVelocity->value/2.0f) * ( 1.0f-trace.fraction) ), &forward, &pm->ps->velocity );
				pm->ps->wallJumpTime = pm_wallJumpDebounce->integer;

				goto dojump;
			}
		}
		pm->cmd.upmove = 0;
		return qfalse;
	}

dojump:
	if ( pm_bunnyHopEnable->boolean && !pm->ps->bunnyHopTime )
	{//debouncer finished (so we can't gain ludicrous speed on ramps)
	//	vector3 angs;
	//	AngleVectors( pm->ps->viewangles, angs, NULL, NULL );
	//	VectorMA( pm->ps->velocity, 26.0f, angs, pm->ps->velocity );
		pm->ps->bunnyHopTime = pm_bunnyHopDebounce->integer;
	}
	else if ( pm->ps->pm_flags & PMF_JUMP_HELD )
	{// must wait for jump to be released. clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pm->ps->velocity.z = pm_jumpVelocity->value;

	if ( pm_rampJumpEnable->boolean ) {
		vector3 velNorm;
		float speed = VectorNormalize2( &pm->ps->velocity, &velNorm );
		velNorm.z = 0.0f;
		dot = DotProduct( &velNorm, &pml.groundTrace.plane.normal ) * -1.0f;
		//Raz: something like [vel = MA(vel, jumpVel, normal)] may be better
		//		currently we only mutate z velocity by the ramp's normal and our speed
		if ( pml.groundPlane && dot > 0.0f )
			pm->ps->velocity.z += dot*speed;
	}

	if ( pm_doubleJumpEnable->boolean ) {
		if ( pm->ps->doubleJumpTime > 0 && pml.groundPlane )//!pm->ps->qtz.wallJumpTime )
			pm->ps->velocity.z += pm_doubleJumpPush->value;

		pm->ps->doubleJumpTime = pm_doubleJumpDebounce->integer;
	}

	PM_AddEvent( EV_JUMP );

	if ( pm->cmd.forwardmove >= 0 ) {
		PM_ForceLegsAnim( LEGS_JUMP );
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} else {
		PM_ForceLegsAnim( LEGS_JUMPB );
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	pml.groundPlane = qfalse; // jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	return qtrue;
}

/*
=============
PM_CheckWaterJump
=============
*/
static qboolean	PM_CheckWaterJump( void ) {
	vector3	spot;
	int		cont;
	vector3	flatforward;

	if (pm->ps->pm_time) {
		return qfalse;
	}

	// check for water jump
	if ( pm->waterlevel != 2 ) {
		return qfalse;
	}

	flatforward.x = pml.forward.x;
	flatforward.y = pml.forward.y;
	flatforward.z = 0;
	VectorNormalize (&flatforward);

	VectorMA (&pm->ps->origin, 30, &flatforward, &spot);
	spot.z += 4;
	cont = pm->pointcontents (&spot, pm->ps->clientNum );
	if ( !(cont & CONTENTS_SOLID) ) {
		return qfalse;
	}

	spot.z += 16;
	cont = pm->pointcontents (&spot, pm->ps->clientNum );
	if ( cont & (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY) ) {
		return qfalse;
	}

	// jump out of water
	VectorScale (&pml.forward, 200, &pm->ps->velocity);
	pm->ps->velocity.z = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove( void ) {
	// waterjump has no control, but falls

	PM_StepSlideMove( qtrue );

	pm->ps->velocity.z -= pm_gravity->value * pml.frametime;
	if (pm->ps->velocity.z < 0) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove( void ) {
	int		i;
	vector3	wishvel;
	float	wishspeed;
	vector3	wishdir;
	float	scale;
	float	vel;

	if ( PM_CheckWaterJump() ) {
		PM_WaterJumpMove();
		return;
	}
#if 0
	// jump = head for surface
	if ( pm->cmd.upmove >= 10 ) {
		if (pm->ps->velocity[2] > -300) {
			if ( pm->watertype == CONTENTS_WATER ) {
				pm->ps->velocity[2] = 100;
			} else if (pm->watertype == CONTENTS_SLIME) {
				pm->ps->velocity[2] = 80;
			} else {
				pm->ps->velocity[2] = 50;
			}
		}
	}
#endif
	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel.x = 0;
		wishvel.y = 0;
		wishvel.z = -60;		// sink towards bottom
	} else {
		for (i=0; i<3; i++)
			wishvel.data[i] = scale * pml.forward.data[i]*pm->cmd.forwardmove + scale * pml.right.data[i]*pm->cmd.rightmove;

		wishvel.z += scale * pm->cmd.upmove;
	}

	VectorCopy (&wishvel, &wishdir);
	wishspeed = VectorNormalize(&wishdir);

	if (wishspeed > pm_speed->value * pm_swimScale )
		wishspeed = pm_speed->value * pm_swimScale;

	PM_Accelerate (&wishdir, wishspeed, pm_wateraccelerate);

	// make sure we can go up slopes easily under water
	if ( pml.groundPlane && DotProduct( &pm->ps->velocity, &pml.groundTrace.plane.normal ) < 0 ) {
		vel = VectorLength(&pm->ps->velocity);
		// slide along the ground plane
		PM_ClipVelocity (&pm->ps->velocity, &pml.groundTrace.plane.normal, &pm->ps->velocity, OVERCLIP );

		VectorNormalize(&pm->ps->velocity);
		VectorScale(&pm->ps->velocity, vel, &pm->ps->velocity);
	}

	PM_SlideMove( qfalse );
}

/*
===================
PM_InvulnerabilityMove

Only with the invulnerability powerup
===================
*/
static void PM_InvulnerabilityMove( void ) {
	pm->cmd.forwardmove = 0;
	pm->cmd.rightmove = 0;
	pm->cmd.upmove = 0;
	VectorClear(&pm->ps->velocity);
}

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove( void ) {
	int		i;
	vector3	wishvel;
	float	wishspeed;
	vector3	wishdir;
	float	scale;

	// normal slowdown
	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel.x = 0;
		wishvel.y = 0;
		wishvel.z = 0;
	} else {
		for (i=0 ; i<3 ; i++) {
			wishvel.data[i] = scale * pml.forward.data[i]*pm->cmd.forwardmove + scale * pml.right.data[i]*pm->cmd.rightmove;
		}

		wishvel.z += scale * pm->cmd.upmove;
	}

	VectorCopy (&wishvel, &wishdir);
	wishspeed = VectorNormalize(&wishdir);

	PM_Accelerate (&wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove( qfalse );
}

/*
===================
PM_AirMove

===================
*/

void PM_AirControl( vector3 *wishDir, float wishspeed ) {
	float zspeed, speed, dot;

	if ( (pm->ps->movementDir != 2 && pm->ps->movementDir != 6) || wishspeed <= 0.0f )
		return;

	// save the speed
	zspeed = pm->ps->velocity.z;
	pm->ps->velocity.z = 0;
	speed = VectorNormalize( &pm->ps->velocity );

	dot = DotProduct( &pm->ps->velocity, wishDir );
	if ( dot > 0.0f ) {
		pm->ps->velocity.x = pm->ps->velocity.x*speed + wishDir->x*dot*pml.frametime;
		pm->ps->velocity.y = pm->ps->velocity.y*speed + wishDir->y*dot*pml.frametime;
		VectorNormalize( &pm->ps->velocity );
	}
	
	// accelerate based on angle
	pm->ps->velocity.x *= speed;
	pm->ps->velocity.y *= speed;
	pm->ps->velocity.z  = zspeed;
}

static void PM_AirMove( void ) {
	vector3		wishVel, wishDir;
	float		fmove, smove, wishSpeed, scale;

	if ( pm->ps->pm_type != PM_SPECTATOR && pm_wallJumpEnable->boolean )
		PM_CheckJump();

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;
	scale = PM_CmdScale( &pm->cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward.z = 0;
	pml.right.z = 0;
	VectorNormalize( &pml.forward );
	VectorNormalize( &pml.right );

	wishVel.x = pml.forward.x*fmove + pml.right.x*smove;
	wishVel.y = pml.forward.y*fmove + pml.right.y*smove;
	wishVel.z = 0;

	VectorCopy( &wishVel, &wishDir );
	wishSpeed = VectorNormalize( &wishDir );
	wishSpeed *= scale;

	PM_Accelerate( &wishDir, wishSpeed, pm_acceleration->value );
	PM_AirControl( &wishDir, wishSpeed );

	if ( pml.groundPlane )
		PM_ClipVelocity( &pm->ps->velocity, &pml.groundTrace.plane.normal, &pm->ps->velocity, OVERCLIP );

	PM_StepSlideMove ( qtrue );
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( void ) {
	vector3		wishvel;
	float		fmove, smove;
	vector3		wishdir;
	float		wishspeed, scale, vel;
	usercmd_t	cmd;

	if ( pm->waterlevel > 2 && DotProduct( &pml.forward, &pml.groundTrace.plane.normal ) > 0 ) {
		// begin swimming
		PM_WaterMove();
		return;
	}

	if ( PM_CheckJump () ) {
		if ( pm->waterlevel > 1 )
			PM_WaterMove();
		else
			PM_AirMove();
		return;
	}

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward.z = 0;
	pml.right.z = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity( &pml.forward, &pml.groundTrace.plane.normal, &pml.forward, OVERCLIP );
	PM_ClipVelocity( &pml.right, &pml.groundTrace.plane.normal, &pml.right, OVERCLIP );

	VectorNormalize( &pml.forward );
	VectorNormalize( &pml.right );

	wishvel.x = (pml.forward.x*fmove) + (pml.right.x*smove);
	wishvel.y = (pml.forward.y*fmove) + (pml.right.y*smove);
	wishvel.z = (pml.forward.z*fmove) + (pml.right.z*smove);
	// when going up or down slopes the wish velocity should Not be zero
//	wishvel[2] = 0;

	VectorCopy( &wishvel, &wishdir );
	wishspeed = VectorNormalize(&wishdir);
	wishspeed *= scale;

	// clamp the speed lower if ducking
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		if (wishspeed > pm_speed->value * pm_duckScale)
			wishspeed = pm_speed->value * pm_duckScale;
	}

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel ) {
		float waterScale;

		waterScale = pm->waterlevel / 3.0f;
		waterScale = 1.0f - ( 1.0f - pm_swimScale ) * waterScale;
		if (wishspeed > pm_speed->value * waterScale)
			wishspeed = pm_speed->value * waterScale;
	}

	PM_Accelerate (&wishdir, wishspeed, pm_acceleration->value);

	if ( (pml.groundTrace.surfaceFlags & SURF_SLICK) || (pm->ps->pm_flags & PMF_TIME_KNOCKBACK) )
		pm->ps->velocity.z -= pm_gravity->value * pml.frametime;

	vel = VectorLength( &pm->ps->velocity );

	// slide along the ground plane
	PM_ClipVelocity( &pm->ps->velocity, &pml.groundTrace.plane.normal, &pm->ps->velocity, OVERCLIP );

	if ( pm_overbounce->boolean ) {
		// don't decrease velocity when going up or down a slope
		VectorNormalize( &pm->ps->velocity );
		VectorScale( &pm->ps->velocity, vel, &pm->ps->velocity );
	}

	// don't do anything if standing still
	if ( !pm->ps->velocity.x && !pm->ps->velocity.y )
		return;

	PM_StepSlideMove( qfalse );
}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void ) {
	float	forward;

	if ( !pml.walking ) {
		return;
	}

	// extra friction

	forward = VectorLength (&pm->ps->velocity);
	forward -= 20;
	if ( forward <= 0 ) {
		VectorClear (&pm->ps->velocity);
	} else {
		VectorNormalize (&pm->ps->velocity);
		VectorScale (&pm->ps->velocity, forward, &pm->ps->velocity);
	}
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void ) {
	float	speed, drop, friction, control, newspeed;
	vector3		wishvel;
	float		fmove, smove;
	vector3		wishdir;
	float		wishspeed;
	float		scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength (&pm->ps->velocity);
	if (speed < 1)
		VectorCopy (&vec3_origin, &pm->ps->velocity);
	else
	{
		drop = 0;

		friction = pm_friction->value * 1.5f;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (&pm->ps->velocity, newspeed, &pm->ps->velocity);
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;
	
	wishvel.x = (pml.forward.x*fmove) + (pml.right.x*smove);
	wishvel.y = (pml.forward.y*fmove) + (pml.right.y*smove);
	wishvel.z = (pml.forward.z*fmove) + (pml.right.z*smove) + (pm->cmd.upmove);

	VectorCopy (&wishvel, &wishdir);
	wishspeed = VectorNormalize(&wishdir);
	wishspeed *= scale;

	PM_Accelerate( &wishdir, wishspeed, pm_acceleration->value );

	// move
	VectorMA (&pm->ps->origin, pml.frametime, &pm->ps->velocity, &pm->ps->origin);
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void ) {
	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	if ( pml.groundTrace.surfaceFlags & SURF_METALSTEPS ) {
		return EV_FOOTSTEP_METAL;
	}
	return EV_FOOTSTEP;
}


/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand( void ) {
	float		delta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;

	// decide which landing animation to use
	if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP )
		PM_ForceLegsAnim( LEGS_LANDB );
	else
		PM_ForceLegsAnim( LEGS_LAND );

	pm->ps->legsTimer = TIMER_LAND;

	// calculate the exact velocity on landing
	dist = pm->ps->origin.z - pml.previous_origin.z;
	vel = pml.previous_velocity.z;
	acc = -pm_gravity->value;

	a = acc / 2;
	b = vel;
	c = -dist;

	den =  b * b - 4 * a * c;
	if ( den < 0 )
		return;

	t = (-b - sqrtf( den )) / (2 * a);

	delta = vel + t * acc;
	delta = delta*delta * 0.0001f;

	delta *= 0.75f;

	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 )
		return;

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 )
		delta *= 0.25f;
	if ( pm->waterlevel == 1 )
		delta *= 0.5f;

	if ( delta < 1 )
		return;

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if ( !(pml.groundTrace.surfaceFlags & SURF_NODAMAGE) )  {
		if ( delta > 60 ) {
			PM_AddEvent( EV_FALL_FAR );
		} else if ( delta > 40 ) {
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[STAT_HEALTH] > 0 ) {
				PM_AddEvent( EV_FALL_MEDIUM );
			}
		} else if ( delta > 7 ) {
			PM_AddEvent( EV_FALL_SHORT );
		} else {
			PM_AddEvent( PM_FootstepForSurface() );
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}

/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid( trace_t *trace ) {
	int			i, j, k;
	vector3		point;

	// jitter around
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			for (k = -1; k <= 1; k++) {
				VectorCopy(&pm->ps->origin, &point);
				point.x += (float) i;
				point.y += (float) j;
				point.z += (float) k;
				pm->trace (trace, &point, &pm->mins, &pm->maxs, &point, pm->ps->clientNum, pm->tracemask);
				if ( !trace->allsolid ) {
					point.x = pm->ps->origin.x;
					point.y = pm->ps->origin.y;
					point.z = pm->ps->origin.z - 0.25f;

					pm->trace (trace, &pm->ps->origin, &pm->mins, &pm->maxs, &point, pm->ps->clientNum, pm->tracemask);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}


/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void ) {
	trace_t		trace;
	vector3		point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		// we just transitioned into freefall

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( &pm->ps->origin, &point );
		point.z -= 64;

		pm->trace (&trace, &pm->ps->origin, &pm->mins, &pm->maxs, &point, pm->ps->clientNum, pm->tracemask);
		if ( trace.fraction == 1.0 ) {
			if ( pm->cmd.forwardmove >= 0 ) {
				PM_ForceLegsAnim( LEGS_JUMP );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
				PM_ForceLegsAnim( LEGS_JUMPB );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void ) {
	vector3		point;
	trace_t		trace;

	point.x = pm->ps->origin.x;
	point.y = pm->ps->origin.y;
	point.z = pm->ps->origin.z - 0.25f;

	pm->trace (&trace, &pm->ps->origin, &pm->mins, &pm->maxs, &point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid ) {
		if ( !PM_CorrectAllSolid(&trace) )
			return;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 ) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity.z > 0 && DotProduct( &pm->ps->velocity, &trace.plane.normal ) > 10 ) {
		// go into jump animation
		if ( pm->cmd.forwardmove >= 0 ) {
			PM_ForceLegsAnim( LEGS_JUMP );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		} else {
			PM_ForceLegsAnim( LEGS_JUMPB );
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}
	
	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal.z < MIN_WALK_NORMAL ) {
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		// just hit the ground
		PM_CrashLand();

		// don't do landing time if we were just going down a slope
		if ( pml.previous_velocity.z < -200 ) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//	pm->ps->velocity[2] = 0;

	PM_AddTouchEnt( trace.entityNum );
}


/*
=============
PM_SetWaterLevel	FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel( void ) {
	vector3		point;
	int			cont;
	int			sample1;
	int			sample2;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	point.x = pm->ps->origin.x;
	point.y = pm->ps->origin.y;
	point.z = pm->ps->origin.z + MINS_Z + 1;	
	cont = pm->pointcontents( &point, pm->ps->clientNum );

	if ( cont & MASK_WATER ) {
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point.z = pm->ps->origin.z + MINS_Z + sample1;
		cont = pm->pointcontents (&point, pm->ps->clientNum );
		if ( cont & MASK_WATER ) {
			pm->waterlevel = 2;
			point.z = pm->ps->origin.z + MINS_Z + sample2;
			cont = pm->pointcontents (&point, pm->ps->clientNum );
			if ( cont & MASK_WATER ){
				pm->waterlevel = 3;
			}
		}
	}

}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck (void)
{
	trace_t	trace;

	pm->ps->pm_flags &= ~PMF_INVULEXPAND;

	pm->mins.x = MINS_X;
	pm->mins.y = MINS_Y;

	pm->maxs.x = MAXS_X;
	pm->maxs.y = MAXS_Y;

	pm->mins.z = MINS_Z;

	if (pm->ps->pm_type == PM_DEAD)
	{
		pm->maxs.z = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	if (pm->cmd.upmove < 0)
	{	// duck
		pm->ps->pm_flags |= PMF_DUCKED;
	}
	else
	{	// stand up if possible
		if (pm->ps->pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pm->maxs.z = 32;
			pm->trace (&trace, &pm->ps->origin, &pm->mins, &pm->maxs, &pm->ps->origin, pm->ps->clientNum, pm->tracemask );
			if (!trace.allsolid)
				pm->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		pm->maxs.z = 16;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	}
	else
	{
		pm->maxs.z = 32;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}
}

/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps( void ) {
	float		bobmove;
	int			old;
	qboolean	footstep;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrtf( pm->ps->velocity.x * pm->ps->velocity.x + pm->ps->velocity.y * pm->ps->velocity.y );

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {

		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 ) {
			PM_ContinueLegsAnim( LEGS_SWIM );
		}
		return;
	}

	// if not trying to move
	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) {
		if (  pm->xyspeed < 5 ) {
			pm->ps->bobCycle = 0;	// start at beginning of cycle again
			if ( pm->ps->pm_flags & PMF_DUCKED ) {
				PM_ContinueLegsAnim( LEGS_IDLECR );
			} else {
				PM_ContinueLegsAnim( LEGS_IDLE );
			}
		}
		return;
	}
	

	footstep = qfalse;

	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		bobmove = 0.5;	// ducked characters bob much faster
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			PM_ContinueLegsAnim( LEGS_BACKCR );
		}
		else {
			PM_ContinueLegsAnim( LEGS_WALKCR );
		}
		// ducked characters never play footsteps
	/*
	} else 	if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			bobmove = 0.4;	// faster speeds bob faster
			footstep = qtrue;
		} else {
			bobmove = 0.3;
		}
		PM_ContinueLegsAnim( LEGS_BACK );
	*/
	} else {
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			bobmove = 0.4f;	// faster speeds bob faster
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				PM_ContinueLegsAnim( LEGS_BACK );
			}
			else {
				PM_ContinueLegsAnim( LEGS_RUN );
			}
			footstep = qtrue;
		} else {
			bobmove = 0.3f;	// walking bobs slow
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				PM_ContinueLegsAnim( LEGS_BACKWALK );
			}
			else {
				PM_ContinueLegsAnim( LEGS_WALK );
			}
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)( old + bobmove * pml.msec ) & 255;

	// if we just crossed a cycle boundary, play an apropriate footstep event
	if ( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 ) {
		if ( pm->waterlevel == 0 ) {
			// on ground will only play sounds if running
			if ( footstep && !pm->noFootsteps ) {
				PM_AddEvent( PM_FootstepForSurface() );
			}
		} else if ( pm->waterlevel == 1 ) {
			// splashing
			PM_AddEvent( EV_FOOTSPLASH );
		} else if ( pm->waterlevel == 2 ) {
			// wading / swimming at surface
			PM_AddEvent( EV_SWIM );
		} else if ( pm->waterlevel == 3 ) {
			// no sound when completely underwater

		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void ) {		// FIXME?
	//
	// if just entered a water volume, play a sound
	//
	if (!pml.previous_waterlevel && pm->waterlevel) {
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (pml.previous_waterlevel && !pm->waterlevel) {
		PM_AddEvent( EV_WATER_LEAVE );
	}

	//
	// check for head just going under water
	//
	if (pml.previous_waterlevel != 3 && pm->waterlevel == 3) {
		PM_AddEvent( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if (pml.previous_waterlevel == 3 && pm->waterlevel != 3) {
		PM_AddEvent( EV_WATER_CLEAR );
	}
}


/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange( int weapon ) {
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		return;
	}

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		return;
	}
	
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		return;
	}

	PM_AddEvent( EV_CHANGE_WEAPON );
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += 200;
	PM_StartTorsoAnim( TORSO_DROP );
}


/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange( void ) {
	int		weapon;

	weapon = pm->cmd.weapon;
	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		weapon = WP_NONE;
	}

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		weapon = WP_NONE;
	}

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += 250;
	PM_StartTorsoAnim( TORSO_RAISE );
}


/*
==============
PM_TorsoAnimation

==============
*/
//RAZMARK: ADDING NEW WEAPONS
const animNumber_t weaponAnimations[WP_NUM_WEAPONS] = {
	TORSO_STAND2,	//WP_NONE

	TORSO_STAND,	// WP_QUANTIZER
	TORSO_STAND,	// WP_REPEATER
	TORSO_STAND,	// WP_SPLICER
	TORSO_STAND,	// WP_MORTAR
	TORSO_STAND,	// WP_DIVERGENCE
};

static void PM_TorsoAnimation( void )
{
	if ( pm->ps->weaponstate == WEAPON_READY )
		PM_ContinueTorsoAnim( weaponAnimations[pm->ps->weapon] );
}


/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
int PM_GetWeaponChargeTime( void ) {
	return pm->ps->weaponCooldown[pm->ps->weapon];
}

//Decrease the timers for all our weapons despite not having them out
void PM_DecreaseWeaponChargeTimes( void )
{
	weapon_t wp;
	for ( wp=WP_NONE; wp<WP_NUM_WEAPONS; wp++ )
	{
		if ( pm->ps->weaponCooldown[wp] < weaponData[wp].maxFireTime )
			pm->ps->weaponCooldown[wp] += pml.msec;
		if ( pm->ps->weaponCooldown[wp] > weaponData[wp].maxFireTime )
			pm->ps->weaponCooldown[wp] = weaponData[wp].maxFireTime;
	}
}

void PM_IncreaseWeaponChargeTime( int addTime ) {
	pm->ps->weaponCooldown[pm->ps->weapon] += addTime;
}

void PM_DecreaseWeaponChargeTime( int addTime ) {
	pm->ps->weaponCooldown[pm->ps->weapon] -= addTime;
}

void PM_SetWeaponChargeTime( int weaponTime ) {
	pm->ps->weaponCooldown[pm->ps->weapon] = weaponTime;
}

static void PM_Weapon( void ) {
	// don't allow attack until all buttons are up
	if ( (pm->ps->pm_flags & PMF_RESPAWNED) || pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR )
		return;

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->ps->weapon = WP_NONE;
		return;
	}

	// check for item using
	if ( pm->cmd.buttons & BUTTON_USE_HOLDABLE ) {
		if ( !(pm->ps->pm_flags & PMF_USE_ITEM_HELD) ) {
			if ( bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_MEDKIT
				&& pm->ps->stats[STAT_HEALTH] >= MAX_HEALTH ) {
				// don't use medkit if at max health
			} else {
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				PM_AddEvent( EV_USE_ITEM0 + bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag );
				pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
			}
			return;
		}
	} else {
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}


	// make weapon function
	if ( pm->ps->weaponTime > 0 )
		pm->ps->weaponTime -= pml.msec;
	PM_DecreaseWeaponChargeTimes(); //QtZ

	// check for weapon change
	// can't change if weapon is firing, but can change again if lowering or raising
	//QtZ
	{//if ( pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING ) {
		if ( pm->ps->weapon != pm->cmd.weapon ) {
			PM_BeginWeaponChange( pm->cmd.weapon );
		}
	}

	if ( pm->ps->weaponTime > 0 )
		return;

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		PM_FinishWeaponChange();
		return;
	}

	//QtZ
	if ( PM_GetWeaponChargeTime() < weaponData[pm->ps->weapon].fireTime )
		return;

	if ( pm->ps->weaponstate == WEAPON_RAISING ) {
		pm->ps->weaponstate = WEAPON_READY;
		PM_StartTorsoAnim( weaponAnimations[pm->ps->weapon] );
		return;
	}

	// check for fire
	if ( ! (pm->cmd.buttons & BUTTON_ATTACK) ) {
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	// start the animation even if out of ammo
	PM_StartTorsoAnim( TORSO_ATTACK );

	pm->ps->weaponstate = WEAPON_FIRING;

	// check for out of ammo
	if ( ! pm->ps->ammo[ pm->ps->weapon ] ) {
		PM_AddEvent( EV_NOAMMO );
		pm->ps->weaponTime += 500;
		return;
	}

	// take an ammo away if not infinite
	if ( pm->ps->ammo[ pm->ps->weapon ] != -1 ) {
		pm->ps->ammo[ pm->ps->weapon ]--;
	}

	// fire weapon
	//QtZ
	//PM_AddEvent( EV_FIRE_WEAPON );
	BG_AddPredictableEventToPlayerstate( EV_FIRE_WEAPON, PM_GetWeaponChargeTime(), pm->ps );

	PM_SetWeaponChargeTime( 0 );
}

/*
================
PM_Animate
================
*/

static void PM_Animate( void ) {
	if ( pm->cmd.buttons & BUTTON_GESTURE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GESTURE );
			pm->ps->torsoTimer = TIMER_GESTURE;
			PM_AddEvent( EV_TAUNT );
		}
	}
}


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void ) {
	// drop misc timing counter
	if ( pm->ps->pm_time ) {
		if ( pml.msec >= pm->ps->pm_time ) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		}
		else
			pm->ps->pm_time -= pml.msec;
	}

	// legs animation timer
	if ( pm->ps->legsTimer > 0 ) {
		pm->ps->legsTimer -= pml.msec;
		if (pm->ps->legsTimer < 0)
			pm->ps->legsTimer = 0;
	}

	// torso animation timer
	if ( pm->ps->torsoTimer > 0 ) {
		pm->ps->torsoTimer -= pml.msec;
		if (pm->ps->torsoTimer < 0)
			pm->ps->torsoTimer = 0;
	}

	// wall jump debounce
	if ( pm->ps->wallJumpTime > 0 ) {
		pm->ps->wallJumpTime -= pml.msec;
		if (pm->ps->wallJumpTime < 0)
			pm->ps->wallJumpTime = 0;
	}

	// bunny hop debounce
	if ( pm->ps->bunnyHopTime > 0 ) {
		pm->ps->bunnyHopTime -= pml.msec;
		if (pm->ps->bunnyHopTime < 0)
			pm->ps->bunnyHopTime = 0;
	}

	// double jump debounce
	if ( pm->ps->doubleJumpTime > 0 ) {
		pm->ps->doubleJumpTime -= pml.msec;
		if (pm->ps->doubleJumpTime < 0)
			pm->ps->doubleJumpTime = 0;
	}
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated isntead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd ) {
	short		temp;
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_FREEZE ) {
		return;		// no view changes at all
	}

	if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0 ) {
		return;		// no view changes at all
	}

	// circularly clamp the angles with deltas
	for ( i=0; i<3; i++ ) {
		temp = cmd->angles[i] + (short)ps->delta_angles.data[i];
		if ( i == 0/*PITCH*/ ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles.data[i] =  16000 - cmd->angles[i];
				temp =  16000;
			}
			else if ( temp < -16000 ) {
				ps->delta_angles.data[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		ps->viewangles.data[i] = SHORT2ANGLE(temp);
	}

}


/*
================
PmoveSingle

================
*/
void PmoveSingle (pmove_t *pmove) {
	pm = pmove;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 ) {
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK ) {
		pm->ps->eFlags |= EF_TALK;
	} else {
		pm->ps->eFlags &= ~EF_TALK;
	}

	// set the firing flag for continuous beam weapons
	if ( !(pm->ps->pm_flags & PMF_RESPAWNED) && pm->ps->pm_type != PM_INTERMISSION && pm->ps->pm_type != PM_NOCLIP
		&& ( pm->cmd.buttons & BUTTON_ATTACK ) && pm->ps->ammo[ pm->ps->weapon ] ) {
		pm->ps->eFlags |= EF_FIRING;
	} else {
		pm->ps->eFlags &= ~EF_FIRING;
	}

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[STAT_HEALTH] > 0 && 
		!( pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) ) ) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK ) {
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset (&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
		 if ( pml.msec < 1 )	pml.msec = 1;
	else if ( pml.msec > 200 )	pml.msec = 200;
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy (&pm->ps->origin, &pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy (&pm->ps->velocity, &pml.previous_velocity);

	pml.frametime = pml.msec * 0.001f;

	// update the viewangles
	PM_UpdateViewAngles( pm->ps, &pm->cmd );

	AngleVectors (&pm->ps->viewangles, &pml.forward, &pml.right, &pml.up);

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if ( pm->cmd.forwardmove < 0 ) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) ) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		PM_CheckDuck ();
		PM_FlyMove ();
		PM_DropTimers ();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		PM_NoclipMove ();
		PM_DropTimers ();
		return;
	}

	if (pm->ps->pm_type == PM_FREEZE) {
		return;		// no movement at all
	}

	if ( pm->ps->pm_type == PM_INTERMISSION ) {
		return;		// no movement at all
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck ();

	// set groundentity
	PM_GroundTrace();

	if ( pm->ps->pm_type == PM_DEAD )
		PM_DeadMove ();

	PM_DropTimers();

	if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP )
		PM_WaterJumpMove();
	else if ( pm->waterlevel > 1 )
		PM_WaterMove(); // swimming
	else if ( pml.walking )
		PM_WalkMove(); // walking on ground
	else
		PM_AirMove(); // airborne

	PM_Animate();

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	// weapons
	PM_Weapon();

	// torso animation
	PM_TorsoAnimation();

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();

	// snap some parts of playerstate to save network bandwidth
	if ( !pm_float->boolean )
		VectorSnap( &pm->ps->velocity );
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove (pmove_t *pmove) {
	int finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime )
		return;	// should not happen

	if ( finalTime > pmove->ps->commandTime + 1000 )
		pmove->ps->commandTime = finalTime - 1000;

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount+1) & ((1<<PS_PMOVEFRAMECOUNTBITS)-1);

	while ( pmove->ps->commandTime != finalTime ) {
		int msec = Q_clampi( 1, finalTime-pmove->ps->commandTime, pm_frametime->integer );
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle( pmove );

		if ( pmove->ps->pm_flags & PMF_JUMP_HELD )
			pmove->cmd.upmove = 20;
	}
}
