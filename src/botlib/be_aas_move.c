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

/*****************************************************************************
 * name:		be_aas_move.c
 *
 * desc:		AAS
 *
 * $Archive: /MissionPack/code/botlib/be_aas_move.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_libvar.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

extern botlib_import_t botimport;

aas_settings_t aassettings;

//#define AAS_MOVE_DEBUG

int AAS_DropToFloor(vector3 *origin, vector3 *mins, vector3 *maxs) {
	vector3 end;
	bsp_trace_t trace;

	VectorCopy(origin, &end);
	end.z -= 100;
	trace = AAS_Trace(origin, mins, maxs, &end, 0, CONTENTS_SOLID);
	if (trace.startsolid) return qfalse;
	VectorCopy(&trace.endpos, origin);
	return qtrue;
}

void AAS_InitSettings( void ) {
	aassettings.phys_gravitydirection.x		= 0;
	aassettings.phys_gravitydirection.y		= 0;
	aassettings.phys_gravitydirection.z		= -1;
	aassettings.phys_friction				= LibVarValue("phys_friction", "6");
	aassettings.phys_stopspeed				= LibVarValue("phys_stopspeed", "100");
	aassettings.phys_gravity				= LibVarValue("phys_gravity", "800");
	aassettings.phys_waterfriction			= LibVarValue("phys_waterfriction", "1");
	aassettings.phys_watergravity			= LibVarValue("phys_watergravity", "400");
	aassettings.phys_maxvelocity			= LibVarValue("phys_maxvelocity", "320");
	aassettings.phys_maxwalkvelocity		= LibVarValue("phys_maxwalkvelocity", "320");
	aassettings.phys_maxcrouchvelocity		= LibVarValue("phys_maxcrouchvelocity", "100");
	aassettings.phys_maxswimvelocity		= LibVarValue("phys_maxswimvelocity", "150");
	aassettings.phys_walkaccelerate			= LibVarValue("phys_walkaccelerate", "10");
	aassettings.phys_airaccelerate			= LibVarValue("phys_airaccelerate", "1");
	aassettings.phys_swimaccelerate			= LibVarValue("phys_swimaccelerate", "4");
	aassettings.phys_maxstep				= LibVarValue("phys_maxstep", "19");
	aassettings.phys_maxsteepness			= LibVarValue("phys_maxsteepness", "0.7");
	aassettings.phys_maxwaterjump			= LibVarValue("phys_maxwaterjump", "18");
	aassettings.phys_maxbarrier				= LibVarValue("phys_maxbarrier", "33");
	aassettings.phys_jumpvel				= LibVarValue("phys_jumpvel", "270");
	aassettings.phys_falldelta5				= LibVarValue("phys_falldelta5", "40");
	aassettings.phys_falldelta10			= LibVarValue("phys_falldelta10", "60");
	aassettings.rs_waterjump				= LibVarValue("rs_waterjump", "400");
	aassettings.rs_teleport					= LibVarValue("rs_teleport", "50");
	aassettings.rs_barrierjump				= LibVarValue("rs_barrierjump", "100");
	aassettings.rs_startcrouch				= LibVarValue("rs_startcrouch", "300");
	aassettings.rs_startgrapple				= LibVarValue("rs_startgrapple", "500");
	aassettings.rs_startwalkoffledge		= LibVarValue("rs_startwalkoffledge", "70");
	aassettings.rs_startjump				= LibVarValue("rs_startjump", "300");
	aassettings.rs_rocketjump				= LibVarValue("rs_rocketjump", "500");
	aassettings.rs_bfgjump					= LibVarValue("rs_bfgjump", "500");
	aassettings.rs_jumppad					= LibVarValue("rs_jumppad", "250");
	aassettings.rs_aircontrolledjumppad		= LibVarValue("rs_aircontrolledjumppad", "300");
	aassettings.rs_funcbob					= LibVarValue("rs_funcbob", "300");
	aassettings.rs_startelevator			= LibVarValue("rs_startelevator", "50");
	aassettings.rs_falldamage5				= LibVarValue("rs_falldamage5", "300");
	aassettings.rs_falldamage10				= LibVarValue("rs_falldamage10", "500");
	aassettings.rs_maxfallheight			= LibVarValue("rs_maxfallheight", "0");
	aassettings.rs_maxjumpfallheight		= LibVarValue("rs_maxjumpfallheight", "450");
}

// returns qtrue if the bot is against a ladder
int AAS_AgainstLadder(vector3 *origin) {
	int areanum, i, facenum, side;
	vector3 org;
	aas_plane_t *plane;
	aas_face_t *face;
	aas_area_t *area;

	VectorCopy(origin, &org);
	areanum = AAS_PointAreaNum(&org);
	if (!areanum)
	{
		org.x += 1;
		areanum = AAS_PointAreaNum(&org);
		if (!areanum)
		{
			org.y += 1;
			areanum = AAS_PointAreaNum(&org);
			if (!areanum)
			{
				org.x -= 2;
				areanum = AAS_PointAreaNum(&org);
				if (!areanum)
				{
					org.y -= 2;
					areanum = AAS_PointAreaNum(&org);
				}
			}
		}
	}
	//if in solid... wrrr shouldn't happen
	if (!areanum) return qfalse;
	//if not in a ladder area
	if (!(aasworld.areasettings[areanum].areaflags & AREA_LADDER)) return qfalse;
	//if a crouch only area
	if (!(aasworld.areasettings[areanum].presencetype & PRESENCE_NORMAL)) return qfalse;
	//
	area = &aasworld.areas[areanum];
	for (i = 0; i < area->numfaces; i++)
	{
		facenum = aasworld.faceindex[area->firstface + i];
		side = facenum < 0;
		face = &aasworld.faces[abs(facenum)];
		//if the face isn't a ladder face
		if (!(face->faceflags & FACE_LADDER)) continue;
		//get the plane the face is in
		plane = &aasworld.planes[face->planenum ^ side];
		//if the origin is pretty close to the plane
		if (fabsf(DotProduct(&plane->normal, origin) - plane->dist) < 3)
		{
			if (AAS_PointInsideFace(abs(facenum), origin, 0.1f)) return qtrue;
		}
	}
	return qfalse;
}

// returns qtrue if the bot is on the ground
int AAS_OnGround(vector3 *origin, int presencetype, int passent) {
	aas_trace_t trace;
	vector3 end, up = {0, 0, 1};
	aas_plane_t *plane;

	VectorCopy(origin, &end);
	end.z -= 10;

	trace = AAS_TraceClientBBox(origin, &end, presencetype, passent);

	//if in solid
	if (trace.startsolid) return qfalse;
	//if nothing hit at all
	if (trace.fraction >= 1.0f) return qfalse;
	//if too far from the hit plane
	if (origin->z - trace.endpos.z > 10) return qfalse;
	//check if the plane isn't too steep
	plane = AAS_PlaneFromNum(trace.planenum);
	if (DotProduct(&plane->normal, &up) < aassettings.phys_maxsteepness) return qfalse;
	//the bot is on the ground
	return qtrue;
}

// returns qtrue if a bot at the given position is swimming
int AAS_Swimming(vector3 *origin) {
	vector3 testorg;

	VectorCopy(origin, &testorg);
	testorg.z -= 2;
	if (AAS_PointContents(&testorg) & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER)) return qtrue;
	return qfalse;
}

static vector3 VEC_UP		= {0, -1,  0};
static vector3 MOVEDIR_UP	= {0,  0,  1};
static vector3 VEC_DOWN		= {0, -2,  0};
static vector3 MOVEDIR_DOWN	= {0,  0, -1};

void AAS_SetMovedir(vector3 *angles, vector3 *movedir) {
	if (VectorCompare(angles, &VEC_UP))
	{
		VectorCopy(&MOVEDIR_UP, movedir);
	}
	else if (VectorCompare(angles, &VEC_DOWN))
	{
		VectorCopy(&MOVEDIR_DOWN, movedir);
	}
	else
	{
		AngleVectors(angles, movedir, NULL, NULL);
	}
}

void AAS_JumpReachRunStart(aas_reachability_t *reach, vector3 *runstart) {
	vector3 hordir, start, cmdmove;
	aas_clientmove_t move;

	//
	hordir.x = reach->start.x - reach->end.x;
	hordir.y = reach->start.y - reach->end.y;
	hordir.z = 0;
	VectorNormalize(&hordir);
	//start point
	VectorCopy(&reach->start, &start);
	start.z += 1;
	//get command movement
	VectorScale(&hordir, 400, &cmdmove);
	//
	AAS_PredictClientMovement(&move, -1, &start, PRESENCE_NORMAL, qtrue,
								&vec3_origin, &cmdmove, 1, 2, 0.1f,
								SE_ENTERWATER|SE_ENTERSLIME|SE_ENTERLAVA|
								SE_HITGROUNDDAMAGE|SE_GAP, 0, qfalse);
	VectorCopy(&move.endpos, runstart);
	//don't enter slime or lava and don't fall from too high
	if (move.stopevent & (SE_ENTERSLIME|SE_ENTERLAVA|SE_HITGROUNDDAMAGE))
	{
		VectorCopy(&start, runstart);
	}
}

// returns the Z velocity when rocket jumping at the origin
float AAS_WeaponJumpZVelocity(vector3 *origin, float radiusdamage) {
	vector3 kvel, v, start, end, forward, right, viewangles, dir;
	float	mass, knockback, points;
	vector3 rocketoffset = {8, 8, -8};
	vector3 botmins = {-16, -16, -24};
	vector3 botmaxs = {16, 16, 32};
	bsp_trace_t bsptrace;

	//look down (90 degrees)
	viewangles.pitch = 90;
	viewangles.yaw = 0;
	viewangles.roll = 0;
	//get the start point shooting from
	VectorCopy(origin, &start);
	start.z += 8; //view offset Z
	AngleVectors(&viewangles, &forward, &right, NULL);
	start.x += forward.x * rocketoffset.x + right.x * rocketoffset.y;
	start.y += forward.y * rocketoffset.x + right.y * rocketoffset.y;
	start.z += forward.z * rocketoffset.x + right.z * rocketoffset.y + rocketoffset.z;
	//end point of the trace
	VectorMA(&start, 500, &forward, &end);
	//trace a line to get the impact point
	bsptrace = AAS_Trace(&start, NULL, NULL, &end, 1, CONTENTS_SOLID);
	//calculate the damage the bot will get from the rocket impact
	VectorAdd(&botmins, &botmaxs, &v);
	VectorMA(origin, 0.5f, &v, &v);
	VectorSubtract(&bsptrace.endpos, &v, &v);
	//
	points = radiusdamage - 0.5f * VectorLength(&v);
	if (points < 0) points = 0;
	//the owner of the rocket gets half the damage
	points *= 0.5f;
	//mass of the bot (p_client.c: PutClientInServer)
	mass = 200;
	//knockback is the same as the damage points
	knockback = points;
	//direction of the damage (from trace.endpos to bot origin)
	VectorSubtract(origin, &bsptrace.endpos, &dir);
	VectorNormalize(&dir);
	//damage velocity
	VectorScale(&dir, 1600.0f * (float)knockback / mass, &kvel);	//the rocket jump hack...
	//rocket impact velocity + jump velocity
	return kvel.z + aassettings.phys_jumpvel;
}

float AAS_RocketJumpZVelocity(vector3 *origin) {
	//rocket radius damage is 120 (p_weapon.c: Weapon_RocketLauncher_Fire)
	return AAS_WeaponJumpZVelocity(origin, 120);
}

float AAS_BFGJumpZVelocity(vector3 *origin) {
	//bfg radius damage is 1000 (p_weapon.c: weapon_bfg_fire)
	return AAS_WeaponJumpZVelocity(origin, 120);
}

// applies ground friction to the given velocity
void AAS_Accelerate(vector3 *velocity, float frametime, vector3 *wishdir, float wishspeed, float accel) {
	// q2 style
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct(velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	accelspeed = accel*frametime*wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}
	
	for (i=0 ; i<3 ; i++) {
		velocity->data[i] += accelspeed*wishdir->data[i];	
	}
}

// applies ground friction to the given velocity
void AAS_ApplyFriction(vector3 *vel, float friction, float stopspeed, float frametime) {
	float speed, control, newspeed;

	//horizontal speed
	speed = sqrtf(vel->x*vel->x + vel->y*vel->y);
	if (speed)
	{
		control = speed < stopspeed ? stopspeed : speed;
		newspeed = speed - frametime * control * friction;
		if (newspeed < 0) newspeed = 0;
		newspeed /= speed;
		vel->x *= newspeed;
		vel->y *= newspeed;
	}
}

int AAS_ClipToBBox(aas_trace_t *trace, vector3 *start, vector3 *end, int presencetype, vector3 *mins, vector3 *maxs) {
	int i, side;
	float front, back, frac, planedist;
	vector3 bboxmins, bboxmaxs, absmins, absmaxs, dir, mid;

	AAS_PresenceTypeBoundingBox(presencetype, &bboxmins, &bboxmaxs);
	VectorSubtract(mins, &bboxmaxs, &absmins);
	VectorSubtract(maxs, &bboxmins, &absmaxs);
	//
	VectorCopy(end, &trace->endpos);
	trace->fraction = 1;
	for (i = 0; i < 3; i++)
	{
		if (start->data[i] < absmins.data[i] && end->data[i] < absmins.data[i]) return qfalse;
		if (start->data[i] > absmaxs.data[i] && end->data[i] > absmaxs.data[i]) return qfalse;
	}
	//check bounding box collision
	VectorSubtract(end, start, &dir);
	frac = 1;
	for (i = 0; i < 3; i++)
	{
		//get plane to test collision with for the current axis direction
		if (dir.data[i] > 0) planedist = absmins.data[i];
		else planedist = absmaxs.data[i];
		//calculate collision fraction
		front = start->data[i] - planedist;
		back = end->data[i] - planedist;
		frac = front / (front-back);
		//check if between bounding planes of next axis
		side = i + 1;
		if (side > 2) side = 0;
		mid.data[side] = start->data[side] + dir.data[side] * frac;
		if (mid.data[side] > absmins.data[side] && mid.data[side] < absmaxs.data[side])
		{
			//check if between bounding planes of next axis
			side++;
			if (side > 2) side = 0;
			mid.data[side] = start->data[side] + dir.data[side] * frac;
			if (mid.data[side] > absmins.data[side] && mid.data[side] < absmaxs.data[side])
			{
				mid.data[i] = planedist;
				break;
			}
		}
	}
	//if there was a collision
	if (i != 3)
	{
		trace->startsolid = qfalse;
		trace->fraction = frac;
		trace->ent = 0;
		trace->planenum = 0;
		trace->area = 0;
		trace->lastarea = 0;
		//trace endpos
		VectorMA( start, frac, &dir, &trace->endpos );
		return qtrue;
	}
	return qfalse;
}

// predicts the movement
//	assumes regular bounding box sizes
//	NOTE: out of water jumping is not included
//	NOTE: grappling hook is not included
//	origin			origin to start with
//	presencetype	presence type to start with
//	velocity		velocity to start with
//	cmdmove			client command movement
//	cmdframes		number of frame cmdmove is valid
//	maxframes		maximum number of predicted frames
//	frametime		duration of one predicted frame
//	stopevent		events that stop the prediction
//	stopareanum		stop as soon as entered this area
int AAS_ClientMovementPrediction(struct aas_clientmove_s *move,
								int entnum, vector3 *origin,
								int presencetype, int onground,
								vector3 *velocity, vector3 *cmdmove,
								int cmdframes,
								int maxframes, float frametime,
								int stopevent, int stopareanum,
								vector3 *mins, vector3 *maxs, int visualize)
{
	float phys_friction, phys_stopspeed, phys_gravity, phys_waterfriction;
	float phys_watergravity;
	float phys_walkaccelerate, phys_airaccelerate, phys_swimaccelerate;
	float phys_maxwalkvelocity, phys_maxcrouchvelocity, phys_maxswimvelocity;
	float phys_maxstep, phys_maxsteepness, phys_jumpvel, friction;
	float gravity, delta, maxvel, wishspeed, accelerate;
	//float velchange, newvel;
	//int ax;
	int n, i, j, pc, step, swimming, crouch, event, jump_frame, areanum;
	int areas[20], numareas;
	vector3 points[20];
	vector3 org, end, feet, start, stepend, lastorg, wishdir;
	vector3 frame_test_vel, old_frame_test_vel, left_test_vel;
	vector3 up = {0, 0, 1};
	aas_plane_t *plane, *plane2;
	aas_trace_t trace, steptrace;
	
	if (frametime <= 0) frametime = 0.1f;
	//
	phys_friction = aassettings.phys_friction;
	phys_stopspeed = aassettings.phys_stopspeed;
	phys_gravity = aassettings.phys_gravity;
	phys_waterfriction = aassettings.phys_waterfriction;
	phys_watergravity = aassettings.phys_watergravity;
	phys_maxwalkvelocity = aassettings.phys_maxwalkvelocity;// * frametime;
	phys_maxcrouchvelocity = aassettings.phys_maxcrouchvelocity;// * frametime;
	phys_maxswimvelocity = aassettings.phys_maxswimvelocity;// * frametime;
	phys_walkaccelerate = aassettings.phys_walkaccelerate;
	phys_airaccelerate = aassettings.phys_airaccelerate;
	phys_swimaccelerate = aassettings.phys_swimaccelerate;
	phys_maxstep = aassettings.phys_maxstep;
	phys_maxsteepness = aassettings.phys_maxsteepness;
	phys_jumpvel = aassettings.phys_jumpvel * frametime;
	//
	memset(move, 0, sizeof(aas_clientmove_t));
	memset(&trace, 0, sizeof(aas_trace_t));
	//start at the current origin
	VectorCopy(origin, &org);
	org.z += 0.25f;
	//velocity to test for the first frame
	VectorScale(velocity, frametime, &frame_test_vel);
	//
	jump_frame = -1;
	//predict a maximum of 'maxframes' ahead
	for (n = 0; n < maxframes; n++)
	{
		swimming = AAS_Swimming(&org);
		//get gravity depending on swimming or not
		gravity = swimming ? phys_watergravity : phys_gravity;
		//apply gravity at the START of the frame
		frame_test_vel.z = frame_test_vel.z - (gravity * 0.1f * frametime);
		//if on the ground or swimming
		if (onground || swimming)
		{
			friction = swimming ? phys_friction : phys_waterfriction;
			//apply friction
			VectorScale(&frame_test_vel, 1/frametime, &frame_test_vel);
			AAS_ApplyFriction(&frame_test_vel, friction, phys_stopspeed, frametime);
			VectorScale(&frame_test_vel, frametime, &frame_test_vel);
		}
		crouch = qfalse;
		//apply command movement
		if (n < cmdframes)
		{
			//ax = 0;
			maxvel = phys_maxwalkvelocity;
			accelerate = phys_airaccelerate;
			VectorCopy(cmdmove, &wishdir);
			if (onground)
			{
				if (cmdmove->z < -300)
				{
					crouch = qtrue;
					maxvel = phys_maxcrouchvelocity;
				}
				//if not swimming and upmove is positive then jump
				if (!swimming && cmdmove->z > 1)
				{
					//jump velocity minus the gravity for one frame + 5 for safety
					frame_test_vel.z = phys_jumpvel - (gravity * 0.1f * frametime) + 5;
					jump_frame = n;
					//jumping so air accelerate
					accelerate = phys_airaccelerate;
				}
				else
				{
					accelerate = phys_walkaccelerate;
				}
				//ax = 2;
			}
			if (swimming)
			{
				maxvel = phys_maxswimvelocity;
				accelerate = phys_swimaccelerate;
				//ax = 3;
			}
			else
			{
				wishdir.z = 0;
			}
			//
			wishspeed = VectorNormalize(&wishdir);
			if (wishspeed > maxvel) wishspeed = maxvel;
			VectorScale(&frame_test_vel, 1/frametime, &frame_test_vel);
			AAS_Accelerate(&frame_test_vel, frametime, &wishdir, wishspeed, accelerate);
			VectorScale(&frame_test_vel, frametime, &frame_test_vel);
			/*
			for (i = 0; i < ax; i++)
			{
				velchange = (cmdmove[i] * frametime) - frame_test_vel[i];
				if (velchange > phys_maxacceleration) velchange = phys_maxacceleration;
				else if (velchange < -phys_maxacceleration) velchange = -phys_maxacceleration;
				newvel = frame_test_vel[i] + velchange;
				//
				if (frame_test_vel[i] <= maxvel && newvel > maxvel) frame_test_vel[i] = maxvel;
				else if (frame_test_vel[i] >= -maxvel && newvel < -maxvel) frame_test_vel[i] = -maxvel;
				else frame_test_vel[i] = newvel;
			}
			*/
		}
		if (crouch)
		{
			presencetype = PRESENCE_CROUCH;
		}
		else if (presencetype == PRESENCE_CROUCH)
		{
			if (AAS_PointPresenceType(&org) & PRESENCE_NORMAL)
			{
				presencetype = PRESENCE_NORMAL;
			}
		}
		//save the current origin
		VectorCopy(&org, &lastorg);
		//move linear during one frame
		VectorCopy(&frame_test_vel, &left_test_vel);
		j = 0;
		do
		{
			VectorAdd(&org, &left_test_vel, &end);
			//trace a bounding box
			trace = AAS_TraceClientBBox(&org, &end, presencetype, entnum);
			//
//#ifdef AAS_MOVE_DEBUG
			if (visualize)
			{
				if (trace.startsolid) botimport.Print(PRT_MESSAGE, "PredictMovement: start solid\n");
				AAS_DebugLine(&org, &trace.endpos, LINECOLOR_RED);
			}
//#endif //AAS_MOVE_DEBUG
			//
			if (stopevent & (SE_ENTERAREA|SE_TOUCHJUMPPAD|SE_TOUCHTELEPORTER|SE_TOUCHCLUSTERPORTAL))
			{
				numareas = AAS_TraceAreas(&org, &trace.endpos, areas, points, 20);
				for (i = 0; i < numareas; i++)
				{
					if (stopevent & SE_ENTERAREA)
					{
						if (areas[i] == stopareanum)
						{
							VectorCopy(&points[i], &move->endpos);
							VectorScale(&frame_test_vel, 1.0f/frametime, &move->velocity);
							move->endarea = areas[i];
							move->trace = trace;
							move->stopevent = SE_ENTERAREA;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return qtrue;
						}
					}
					//NOTE: if not the first frame
					if ((stopevent & SE_TOUCHJUMPPAD) && n)
					{
						if (aasworld.areasettings[areas[i]].contents & AREACONTENTS_JUMPPAD)
						{
							VectorCopy(&points[i], &move->endpos);
							VectorScale(&frame_test_vel, 1.0f/frametime, &move->velocity);
							move->endarea = areas[i];
							move->trace = trace;
							move->stopevent = SE_TOUCHJUMPPAD;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return qtrue;
						}
					}
					if (stopevent & SE_TOUCHTELEPORTER)
					{
						if (aasworld.areasettings[areas[i]].contents & AREACONTENTS_TELEPORTER)
						{
							VectorCopy(&points[i], &move->endpos);
							move->endarea = areas[i];
							VectorScale(&frame_test_vel, 1.0f/frametime, &move->velocity);
							move->trace = trace;
							move->stopevent = SE_TOUCHTELEPORTER;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return qtrue;
						}
					}
					if (stopevent & SE_TOUCHCLUSTERPORTAL)
					{
						if (aasworld.areasettings[areas[i]].contents & AREACONTENTS_CLUSTERPORTAL)
						{
							VectorCopy(&points[i], &move->endpos);
							move->endarea = areas[i];
							VectorScale(&frame_test_vel, 1.0f/frametime, &move->velocity);
							move->trace = trace;
							move->stopevent = SE_TOUCHCLUSTERPORTAL;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return qtrue;
						}
					}
				}
			}
			//
			if (stopevent & SE_HITBOUNDINGBOX)
			{
				if (AAS_ClipToBBox(&trace, &org, &trace.endpos, presencetype, mins, maxs))
				{
					VectorCopy(&trace.endpos, &move->endpos);
					move->endarea = AAS_PointAreaNum(&move->endpos);
					VectorScale(&frame_test_vel, 1.0f/frametime, &move->velocity);
					move->trace = trace;
					move->stopevent = SE_HITBOUNDINGBOX;
					move->presencetype = presencetype;
					move->endcontents = 0;
					move->time = n * frametime;
					move->frames = n;
					return qtrue;
				}
			}
			//move the entity to the trace end point
			VectorCopy(&trace.endpos, &org);
			//if there was a collision
			if (trace.fraction < 1.0f)
			{
				//get the plane the bounding box collided with
				plane = AAS_PlaneFromNum(trace.planenum);
				//
				if (stopevent & SE_HITGROUNDAREA)
				{
					if (DotProduct(&plane->normal, &up) > phys_maxsteepness)
					{
						VectorCopy(&org, &start);
						start.z += 0.5f;
						if (AAS_PointAreaNum(&start) == stopareanum)
						{
							VectorCopy(&start, &move->endpos);
							move->endarea = stopareanum;
							VectorScale(&frame_test_vel, 1.0f/frametime, &move->velocity);
							move->trace = trace;
							move->stopevent = SE_HITGROUNDAREA;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return qtrue;
						}
					}
				}
				//assume there's no step
				step = qfalse;
				//if it is a vertical plane and the bot didn't jump recently
				if (plane->normal.z == 0 && (jump_frame < 0 || n - jump_frame > 2))
				{
					//check for a step
					VectorMA(&org, -0.25f, &plane->normal, &start);
					VectorCopy(&start, &stepend);
					start.z += phys_maxstep;
					steptrace = AAS_TraceClientBBox(&start, &stepend, presencetype, entnum);
					//
					if (!steptrace.startsolid)
					{
						plane2 = AAS_PlaneFromNum(steptrace.planenum);
						if (DotProduct(&plane2->normal, &up) > phys_maxsteepness)
						{
							VectorSubtract(&end, &steptrace.endpos, &left_test_vel);
							left_test_vel.z = 0;
							frame_test_vel.z = 0;
//#ifdef AAS_MOVE_DEBUG
							if (visualize)
							{
								if (steptrace.endpos.z - org.z > 0.125f)
								{
									VectorCopy(&org, &start);
									start.z = steptrace.endpos.z;
									AAS_DebugLine(&org, &start, LINECOLOR_BLUE);
								}
							}
//#endif //AAS_MOVE_DEBUG
							org.z = steptrace.endpos.z;
							step = qtrue;
						}
					}
				}
				//
				if (!step)
				{
					//velocity left to test for this frame is the projection
					//of the current test velocity into the hit plane 
					VectorMA(&left_test_vel, -DotProduct(&left_test_vel, &plane->normal), &plane->normal, &left_test_vel);
					//store the old velocity for landing check
					VectorCopy(&frame_test_vel, &old_frame_test_vel);
					//test velocity for the next frame is the projection
					//of the velocity of the current frame into the hit plane 
					VectorMA(&frame_test_vel, -DotProduct(&frame_test_vel, &plane->normal), &plane->normal, &frame_test_vel);
					//check for a landing on an almost horizontal floor
					if (DotProduct(&plane->normal, &up) > phys_maxsteepness)
					{
						onground = qtrue;
					}
					if (stopevent & SE_HITGROUNDDAMAGE)
					{
						delta = 0;
						if (old_frame_test_vel.z < 0 &&
								frame_test_vel.z > old_frame_test_vel.z &&
								!onground)
						{
							delta = old_frame_test_vel.z;
						}
						else if (onground)
						{
							delta = frame_test_vel.z - old_frame_test_vel.z;
						}
						if (delta)
						{
							delta = delta * 10;
							delta = delta * delta * 0.0001f;
							if (swimming) delta = 0;
							// never take falling damage if completely underwater
							/*
							if (ent->waterlevel == 3) return;
							if (ent->waterlevel == 2) delta *= 0.25f;
							if (ent->waterlevel == 1) delta *= 0.5f;
							*/
							if (delta > 40)
							{
								VectorCopy(&org, &move->endpos);
								move->endarea = AAS_PointAreaNum(&org);
								VectorCopy(&frame_test_vel, &move->velocity);
								move->trace = trace;
								move->stopevent = SE_HITGROUNDDAMAGE;
								move->presencetype = presencetype;
								move->endcontents = 0;
								move->time = n * frametime;
								move->frames = n;
								return qtrue;
							}
						}
					}
				}
			}
			//extra check to prevent endless loop
			if (++j > 20) return qfalse;
		//while there is a plane hit
		} while(trace.fraction < 1.0f);
		//if going down
		if (frame_test_vel.z <= 10)
		{
			//check for a liquid at the feet of the bot
			VectorCopy(&org, &feet);
			feet.z -= 22;
			pc = AAS_PointContents(&feet);
			//get event from pc
			event = SE_NONE;
			if (pc & CONTENTS_LAVA) event |= SE_ENTERLAVA;
			if (pc & CONTENTS_SLIME) event |= SE_ENTERSLIME;
			if (pc & CONTENTS_WATER) event |= SE_ENTERWATER;
			//
			areanum = AAS_PointAreaNum(&org);
			if (aasworld.areasettings[areanum].contents & AREACONTENTS_LAVA)
				event |= SE_ENTERLAVA;
			if (aasworld.areasettings[areanum].contents & AREACONTENTS_SLIME)
				event |= SE_ENTERSLIME;
			if (aasworld.areasettings[areanum].contents & AREACONTENTS_WATER)
				event |= SE_ENTERWATER;
			//if in lava or slime
			if (event & stopevent)
			{
				VectorCopy(&org, &move->endpos);
				move->endarea = areanum;
				VectorScale(&frame_test_vel, 1.0f/frametime, &move->velocity);
				move->stopevent = event & stopevent;
				move->presencetype = presencetype;
				move->endcontents = pc;
				move->time = n * frametime;
				move->frames = n;
				return qtrue;
			}
		}
		//
		onground = AAS_OnGround(&org, presencetype, entnum);
		//if onground and on the ground for at least one whole frame
		if (onground)
		{
			if (stopevent & SE_HITGROUND)
			{
				VectorCopy(&org, &move->endpos);
				move->endarea = AAS_PointAreaNum(&org);
				VectorScale(&frame_test_vel, 1.0f/frametime, &move->velocity);
				move->trace = trace;
				move->stopevent = SE_HITGROUND;
				move->presencetype = presencetype;
				move->endcontents = 0;
				move->time = n * frametime;
				move->frames = n;
				return qtrue;
			}
		}
		else if (stopevent & SE_LEAVEGROUND)
		{
			VectorCopy(&org, &move->endpos);
			move->endarea = AAS_PointAreaNum(&org);
			VectorScale(&frame_test_vel, 1/frametime, &move->velocity);
			move->trace = trace;
			move->stopevent = SE_LEAVEGROUND;
			move->presencetype = presencetype;
			move->endcontents = 0;
			move->time = n * frametime;
			move->frames = n;
			return qtrue;
		}
		else if (stopevent & SE_GAP)
		{
			aas_trace_t gaptrace;

			VectorCopy(&org, &start);
			VectorCopy(&start, &end);
			end.z -= 48 + aassettings.phys_maxbarrier;
			gaptrace = AAS_TraceClientBBox(&start, &end, PRESENCE_CROUCH, -1);
			//if solid is found the bot cannot walk any further and will not fall into a gap
			if (!gaptrace.startsolid)
			{
				//if it is a gap (lower than one step height)
				if (gaptrace.endpos.z < org.z - aassettings.phys_maxstep - 1)
				{
					if (!(AAS_PointContents(&end) & CONTENTS_WATER))
					{
						VectorCopy(&lastorg, &move->endpos);
						move->endarea = AAS_PointAreaNum(&lastorg);
						VectorScale(&frame_test_vel, 1.0f/frametime, &move->velocity);
						move->trace = trace;
						move->stopevent = SE_GAP;
						move->presencetype = presencetype;
						move->endcontents = 0;
						move->time = n * frametime;
						move->frames = n;
						return qtrue;
					}
				}
			}
		}
	}
	//
	VectorCopy(&org, &move->endpos);
	move->endarea = AAS_PointAreaNum(&org);
	VectorScale(&frame_test_vel, 1.0f/frametime, &move->velocity);
	move->stopevent = SE_NONE;
	move->presencetype = presencetype;
	move->endcontents = 0;
	move->time = n * frametime;
	move->frames = n;
	//
	return qtrue;
}

int AAS_PredictClientMovement(struct aas_clientmove_s *move,
								int entnum, vector3 *origin,
								int presencetype, int onground,
								vector3 *velocity, vector3 *cmdmove,
								int cmdframes,
								int maxframes, float frametime,
								int stopevent, int stopareanum, int visualize)
{
	vector3 mins, maxs;
	return AAS_ClientMovementPrediction(move, entnum, origin, presencetype, onground,
										velocity, cmdmove, cmdframes, maxframes,
										frametime, stopevent, stopareanum,
										&mins, &maxs, visualize);
}

int AAS_ClientMovementHitBBox(struct aas_clientmove_s *move,
								int entnum, vector3 *origin,
								int presencetype, int onground,
								vector3 *velocity, vector3 *cmdmove,
								int cmdframes,
								int maxframes, float frametime,
								vector3 *mins, vector3 *maxs, int visualize)
{
	return AAS_ClientMovementPrediction(move, entnum, origin, presencetype, onground,
										velocity, cmdmove, cmdframes, maxframes,
										frametime, SE_HITBOUNDINGBOX, 0,
										mins, maxs, visualize);
}

void AAS_TestMovementPrediction(int entnum, vector3 *origin, vector3 *dir) {
	vector3 velocity, cmdmove;
	aas_clientmove_t move;

	VectorClear(&velocity);
	if (!AAS_Swimming(origin)) dir->z = 0;
	VectorNormalize(dir);
	VectorScale(dir, 400, &cmdmove);
	cmdmove.z = 224;
	AAS_ClearShownDebugLines();
	AAS_PredictClientMovement(&move, entnum, origin, PRESENCE_NORMAL, qtrue,
									&velocity, &cmdmove, 13, 13, 0.1f, SE_HITGROUND, 0, qtrue);//SE_LEAVEGROUND);
	if (move.stopevent & SE_LEAVEGROUND)
	{
		botimport.Print(PRT_MESSAGE, "leave ground\n");
	}
}

// calculates the horizontal velocity needed to perform a jump from start to end
//	returns qfalse if too high or too far from start to end
//	zvel	z velocity for jump
//	start	start position of jump
//	end		end position of jump
//	*speed	returned speed for jump
int AAS_HorizontalVelocityForJump(float zvel, vector3 *start, vector3 *end, float *velocity) {
	float phys_gravity, phys_maxvelocity;
	float maxjump, height2fall, t, top;
	vector3 dir;

	phys_gravity = aassettings.phys_gravity;
	phys_maxvelocity = aassettings.phys_maxvelocity;

	//maximum height a player can jump with the given initial z velocity
	maxjump = 0.5f * phys_gravity * (zvel / phys_gravity) * (zvel / phys_gravity);
	//top of the parabolic jump
	top = start->z + maxjump;
	//height the bot will fall from the top
	height2fall = top - end->z;
	//if the goal is to high to jump to
	if (height2fall < 0)
	{
		*velocity = phys_maxvelocity;
		return 0;
	}
	//time a player takes to fall the height
	t = sqrtf(height2fall / (0.5f * phys_gravity));
  	//direction from start to end
	VectorSubtract(end, start, &dir);
	//
	if ( (t + zvel / phys_gravity) == 0.0f ) {
		*velocity = phys_maxvelocity;
		return 0;
	}
	//calculate horizontal speed
	*velocity = sqrtf(dir.x*dir.x + dir.y*dir.y) / (t + zvel / phys_gravity);
	//the horizontal speed must be lower than the max speed
	if (*velocity > phys_maxvelocity)
	{
		*velocity = phys_maxvelocity;
		return 0;
	}
	return 1;
}
