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
// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering
#include "cg_local.h"


/*

	MODEL TESTING

	The viewthing and gun positioning tools from Q2 have been integrated and enhanced into a single model testing facility.

	Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

	The names must be the full pathname after the basedir, like "models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

	Testmodel will create a fake entity 100 units in front of the current view position, directly facing the viewer.
	It will remain immobile, so you can move around it to view it from different angles.

	Testgun will cause the model to follow the player around and supress the real view weapon model.
	The default frame 0 of most guns is completely off screen, so you will probably have to cycle a couple frames to see it.

	"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the frame or skin of the testmodel.
	
	If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let you adjust the positioning.

	Note that none of the model testing features update while the game is paused, so it may be convenient to test
	with deathmatch set to 1 so that bringing down the console doesn't pause the game.

*/

// Creates an entity in front of the current position, which can then be moved around
void CG_TestModel_f( void ) {
	vector3		angles;

	memset( &cg.testModelEntity, 0, sizeof(cg.testModelEntity) );
	if ( trap->Cmd_Argc() < 2 ) {
		return;
	}

	Q_strncpyz (cg.testModelName, trap->Cmd_Argv( 1 ), MAX_QPATH );
	cg.testModelEntity.hModel = trap->R_RegisterModel( cg.testModelName );

	if ( trap->Cmd_Argc() == 3 ) {
		cg.testModelEntity.backlerp = (float)atof( trap->Cmd_Argv( 2 ) );
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (! cg.testModelEntity.hModel ) {
		trap->Print( "Can't register model\n" );
		return;
	}

	VectorMA( &cg.refdef.vieworg, 100, &cg.refdef.viewaxis[0], &cg.testModelEntity.origin );

	angles.pitch = 0;
	angles.yaw = 180 + cg.refdefViewAngles.yaw;
	angles.roll = 0;

	AnglesToAxis( &angles, cg.testModelEntity.axis );
	cg.testGun = qfalse;
}

// Replaces the current view weapon with the given model
void CG_TestGun_f( void ) {
	CG_TestModel_f();
	cg.testGun = qtrue;
	cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;
}


void CG_TestModelNextFrame_f( void ) {
	cg.testModelEntity.frame++;
	trap->Print( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelPrevFrame_f( void ) {
	cg.testModelEntity.frame--;
	if ( cg.testModelEntity.frame < 0 ) {
		cg.testModelEntity.frame = 0;
	}
	trap->Print( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelNextSkin_f( void ) {
	cg.testModelEntity.skinNum++;
	trap->Print( "skin %i\n", cg.testModelEntity.skinNum );
}

void CG_TestModelPrevSkin_f( void ) {
	cg.testModelEntity.skinNum--;
	if ( cg.testModelEntity.skinNum < 0 ) {
		cg.testModelEntity.skinNum = 0;
	}
	trap->Print( "skin %i\n", cg.testModelEntity.skinNum );
}

static void CG_AddTestModel( void ) {
	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap->R_RegisterModel( cg.testModelName );
	if (! cg.testModelEntity.hModel ) {
		trap->Print ("Can't register model\n");
		return;
	}

	// if testing a gun, set the origin relative to the view origin
	if ( cg.testGun ) {
		VectorCopy( &cg.refdef.vieworg, &cg.testModelEntity.origin );
		VectorCopy( &cg.refdef.viewaxis[0], &cg.testModelEntity.axis[0] );
		VectorCopy( &cg.refdef.viewaxis[1], &cg.testModelEntity.axis[1] );
		VectorCopy( &cg.refdef.viewaxis[2], &cg.testModelEntity.axis[2] );

		// allow the position to be adjusted
		cg.testModelEntity.origin.x += cg.refdef.viewaxis[0].x * cg.gunAlign.basePos.x;
		cg.testModelEntity.origin.y += cg.refdef.viewaxis[1].y * cg.gunAlign.basePos.y;
		cg.testModelEntity.origin.z += cg.refdef.viewaxis[2].z * cg.gunAlign.basePos.z;
	}

	trap->R_AddRefEntityToScene( &cg.testModelEntity );
}

// Sets the coordinates of the rendered window
static void CG_CalcVrect( void ) {
	cg.refdef.width = cgs.glconfig.vidWidth;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;
}

#define	FOCUS_DISTANCE	512

static void CG_StepOffset( void ) {
	int		timeDelta;
	
	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if ( timeDelta < STEP_TIME ) {
		cg.refdef.vieworg.z -= cg.stepChange * (STEP_TIME - timeDelta) / STEP_TIME;
	}
}

static void CG_OffsetFirstPersonView( void ) {
	vector3 *origin, *angles;
	float bob, ratio, delta, speed, f;
	vector3 predictedVelocity;
	int timeDelta;
	
	if ( cg.snap->ps.pm_type == PM_INTERMISSION )
		return;

	origin = &cg.refdef.vieworg;
	angles = &cg.refdefViewAngles;

	// if dead, fix the angle and don't add any kick
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		angles->roll = 40;
		angles->pitch = -15;
		angles->yaw = (float)cg.snap->ps.stats[STAT_DEAD_YAW];
		origin->z += cg.predictedPlayerState.viewheight;
		return;
	}

	// add angles based on damage kick
	if ( cg.damageTime ) {
		ratio = (float)(cg.time - cg.damageTime);
		if ( ratio < DAMAGE_DEFLECT_TIME ) {
			ratio /= DAMAGE_DEFLECT_TIME;
			angles->pitch += ratio * cg.v_dmg_pitch;
			angles->roll += ratio * cg.v_dmg_roll;
		} else {
			ratio = 1.0f - ( ratio - DAMAGE_DEFLECT_TIME ) / DAMAGE_RETURN_TIME;
			if ( ratio > 0 ) {
				angles->pitch += ratio * cg.v_dmg_pitch;
				angles->roll += ratio * cg.v_dmg_roll;
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = ( cg.time - cg.landTime) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	angles[PITCH] += ratio * cg.fall_value;
#endif

	// add angles based on velocity
	VectorCopy( &cg.predictedPlayerState.velocity, &predictedVelocity );

	delta = DotProduct ( &predictedVelocity, &cg.refdef.viewaxis[0]);
	angles->pitch += delta * cg_runpitch->value;
	
	delta = DotProduct ( &predictedVelocity, &cg.refdef.viewaxis[1]);
	angles->roll -= delta * cg_runroll->value;

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	if ( cg_viewBobEnable->boolean ) {
		delta = cg.bobfracsin * cg.viewBob.pitch * speed;
		if ( cg.predictedPlayerState.pm_flags & PMF_DUCKED )
			delta *= 3;		// crouching
		angles->pitch += delta;
		delta = cg.bobfracsin * cg.viewBob.roll * speed;
		if ( cg.predictedPlayerState.pm_flags & PMF_DUCKED )
			delta *= 3;		// crouching accentuates roll
		if ( cg.bobcycle & 1 )
			delta = -delta;
		angles->roll += delta;
	}


	// add view height
	origin->z += cg.predictedPlayerState.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if ( timeDelta < DUCK_TIME) {
		cg.refdef.vieworg.z -= cg.duckChange * (DUCK_TIME - timeDelta) / DUCK_TIME;
	}

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * cg.viewBob.up;
	if (bob > 6) {
		bob = 6;
	}

	origin->z += bob;


	// add fall height
	if ( cg.viewBob.fall )
	{
		delta = (float)(cg.time - cg.landTime);
		if ( delta < LAND_DEFLECT_TIME ) {
			f = delta / LAND_DEFLECT_TIME;
			cg.refdef.vieworg.z += cg.landChange * f;
		} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
			delta -= LAND_DEFLECT_TIME;
			f = 1.0f - ( delta / LAND_RETURN_TIME );
			cg.refdef.vieworg.z += cg.landChange * f;
		}
	}

	// add step offset
	CG_StepOffset();

	// pivot the eye based on a neck length
#if 0
	{
#define	NECK_LENGTH		8
	vector3			forward, up;
 
	cg.refdef.vieworg[2] -= NECK_LENGTH;
	AngleVectors( cg.refdefViewAngles, forward, NULL, up );
	VectorMA( cg.refdef.vieworg, 3, forward, cg.refdef.vieworg );
	VectorMA( cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg );
	}
#endif
}

void CG_ZoomDown_f( void ) { 
	if ( cg.zoomed ) {
		return;
	}
	cg.zoomed = qtrue;
	cg.zoomTime = cg.time;
}

void CG_ZoomUp_f( void ) { 
	if ( !cg.zoomed ) {
		return;
	}
	cg.zoomed = qfalse;
	cg.zoomTime = cg.time;
}

#define	WAVE_AMPLITUDE	1
#define	WAVE_FREQUENCY	0.4f

float zoomFov; //this has to be global client-side

// Fixed fov at intermissions, otherwise account for fov variable and zooms.
static int CG_CalcFov( void ) {
	float	x;
	float	phase;
	float	v;
	float	fov_x = cg_fov->value, fov_y;
	int		inwater;
	int		contents;

	if (cg.zoomed)
	{
		if (zoomFov > fov_x)
			zoomFov = fov_x;
		zoomFov -= cg.frametime * cg_zoomTime->value;//0.075f;

		if (zoomFov < cg_zoomFov->value)
			zoomFov = cg_zoomFov->value;
		else if (zoomFov > fov_x)
			zoomFov = fov_x;
		else
		{// Still zooming
			static int zoomSoundTime = 0;
			if (zoomSoundTime < cg.time || zoomSoundTime > cg.time + 10000)
			{
				trap->S_StartSound(&cg.refdef.vieworg, ENTITYNUM_WORLD, CHAN_LOCAL, cgs.media.flightSound);
				zoomSoundTime = cg.time + 300;
			}
		}

		if (zoomFov < 5.0f)
			zoomFov = 5.0f;		// hack to fix zoom during vid restart
		fov_x = zoomFov;
	}
	else 
	{
		if (zoomFov > fov_x)
			zoomFov = fov_x;
		zoomFov += cg.frametime * cg_zoomTime->value;//0.075f;

		if (zoomFov > fov_x)
			zoomFov = fov_x;
		else if (zoomFov < cg_zoomFov->value)
			zoomFov = cg_zoomFov->value;
		else
		{
			static int zoomSoundTime = 0;
			if ( zoomSoundTime < cg.time || zoomSoundTime > cg.time + 10000 )
			{
				trap->S_StartSound( &cg.refdef.vieworg, ENTITYNUM_WORLD, CHAN_LOCAL, cgs.media.flightSound );
				zoomSoundTime = cg.time + 300;
			}
		}

		if (zoomFov > fov_x)
			zoomFov = fov_x;
		fov_x = zoomFov;
	}

	if ( cg_fovAspectAdjust->integer ) {
		// Based on LordHavoc's code for Darkplaces
		// http://www.quakeworld.nu/forum/topic/53/what-does-your-qw-look-like/page/30
		const float baseAspect = 0.75f; // 3/4
		const float aspect = (float)cgs.glconfig.vidWidth/(float)cgs.glconfig.vidHeight;
		const float desiredFov = fov_x;

		fov_x = atanf( tanf( desiredFov*M_PI / 360.0f ) * baseAspect*aspect )*360.0f / M_PI;
	}

	x = cg.refdef.width / tanf( fov_x / 360.0f * M_PI );
	fov_y = atan2f( (float)cg.refdef.height, x );
	fov_y = fov_y * 360.0f / M_PI;

	// warp if underwater
	contents = CG_PointContents( &cg.refdef.vieworg, -1 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ){
		phase = cg.time / 1000.0f * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sinf( phase );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}
	else {
		inwater = qfalse;
	}

	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	if (cg.zoomed)
	{
		cg.zoomSensitivity = zoomFov/cg_fov->value;
	}
	else if ( !cg.zoomed ) {
		cg.zoomSensitivity = 1;
	} else {
		cg.zoomSensitivity = cg.refdef.fov_y / 75.0f;
	}

	return inwater;
}

static void CG_DamageBlendBlob( void ) {
	int			t;
	int			maxTime;
	refEntity_t		ent;

	if ( !cg.damageValue ) {
		return;
	}

	//if (cg.cameraMode) {
	//	return;
	//}

	maxTime = DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if ( t <= 0 || t >= maxTime ) {
		return;
	}


	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	VectorMA( &cg.refdef.vieworg, 8, &cg.refdef.viewaxis[0], &ent.origin );
	VectorMA( &ent.origin, cg.damageX * -8, &cg.refdef.viewaxis[1], &ent.origin );
	VectorMA( &ent.origin, cg.damageY * 8, &cg.refdef.viewaxis[2], &ent.origin );

	ent.radius = cg.damageValue * 3;
	ent.customShader = cgs.media.viewBloodShader;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = (byte)(200 * ( 1.0f - ((float)t / maxTime) ));
	trap->R_AddRefEntityToScene( &ent );
}

// Sets cg.refdef view values
static int CG_CalcViewValues( void ) {
	playerState_t	*ps;

	memset( &cg.refdef, 0, sizeof( cg.refdef ) );

	// strings for in game rendering
	// Q_strncpyz( cg.refdef.text[0], "Park Ranger", sizeof(cg.refdef.text[0]) );
	// Q_strncpyz( cg.refdef.text[1], "19", sizeof(cg.refdef.text[1]) );

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;
/*
	if (cg.cameraMode) {
		vector3 origin, angles;
		if (trap->getCameraInfo(cg.time, &origin, &angles)) {
			VectorCopy(origin, cg.refdef.vieworg);
			angles[ROLL] = 0;
			VectorCopy(angles, cg.refdefViewAngles);
			AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
			return CG_CalcFov();
		} else {
			cg.cameraMode = qfalse;
		}
	}
*/
	// intermission view
	if ( ps->pm_type == PM_INTERMISSION ) {
		VectorCopy( &ps->origin, &cg.refdef.vieworg );
		VectorCopy( &ps->viewangles, &cg.refdefViewAngles );
		AnglesToAxis( &cg.refdefViewAngles, cg.refdef.viewaxis );
		return CG_CalcFov();
	}

	cg.bobcycle = ( ps->bobCycle & 128 ) >> 7;
	cg.bobfracsin = fabsf( sinf( ( ps->bobCycle & 127 ) / 127.0f * M_PI ) );
	cg.xyspeed = sqrtf( ps->velocity.x * ps->velocity.x +
		ps->velocity.y * ps->velocity.y );


	VectorCopy( &ps->origin, &cg.refdef.vieworg );
	VectorCopy( &ps->viewangles, &cg.refdefViewAngles );

	// add error decay
	if ( cg_errorDecay->value > 0 ) {
		int		t;
		float	f;

		t = cg.time - cg.predictedErrorTime;
		f = ( cg_errorDecay->value - t ) / cg_errorDecay->value;
		if ( f > 0 && f < 1 ) {
			VectorMA( &cg.refdef.vieworg, f, &cg.predictedError, &cg.refdef.vieworg );
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	// offset for local bobbing and kicks
	CG_OffsetFirstPersonView();

	// position eye relative to origin
	AnglesToAxis( &cg.refdefViewAngles, cg.refdef.viewaxis );

	if ( cg.hyperspace ) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	//QtZ: post process
	cg.refdef.rdflags |= RDF_POSTPROCESS;

	// field of view
	return CG_CalcFov();
}

static void CG_PowerupTimerSounds( void ) {
	int		i;
	int		t;

	// powerup timers going away
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		t = cg.snap->ps.powerups[i];
		if ( t <= cg.time ) {
			continue;
		}
		if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			continue;
		}
		if ( ( t - cg.time ) / POWERUP_BLINK_TIME != ( t - cg.oldTime ) / POWERUP_BLINK_TIME ) {
			trap->S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound );
		}
	}
}

void CG_AddBufferedSound( sfxHandle_t sfx ) {
	if ( !sfx )
		return;
	cg.soundBuffer[cg.soundBufferIn] = sfx;
	cg.soundBufferIn = (cg.soundBufferIn + 1) % MAX_SOUNDBUFFER;
	if (cg.soundBufferIn == cg.soundBufferOut) {
		cg.soundBufferOut++;
	}
}

static void CG_PlayBufferedSounds( void ) {
	if ( cg.soundTime < cg.time ) {
		if (cg.soundBufferOut != cg.soundBufferIn && cg.soundBuffer[cg.soundBufferOut]) {
			trap->S_StartLocalSound(cg.soundBuffer[cg.soundBufferOut], CHAN_ANNOUNCER);
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		}
	}
}

// Generates and draws a game scene and status information at the given time.
//	If demoPlayback is set, local movement prediction will not be enabled
void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback ) {
	int		inwater;

	cg.time = serverTime;
	cg.demoPlayback = demoPlayback;

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if ( cg.infoScreenText[0] != 0 ) {
		CG_DrawInformation();
		return;
	}

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap->S_ClearLoopingSounds(qfalse);

	// clear all the render lists
	trap->R_ClearScene();

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
		CG_DrawInformation();
		return;
	}

	// let the client system know what our weapon and zoom settings are
	trap->SetUserCmdValue( cg.weaponSelect, cg.zoomSensitivity );

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	// build cg.refdef
	inwater = CG_CalcViewValues();

	// first person blend blobs, done after AnglesToAxis
	CG_DamageBlendBlob();

	// build the render lists
	if ( !cg.hyperspace ) {
		CG_AddPacketEntities();			// adter calcViewValues, so predicted player state is correct
		CG_AddMarks();
		CG_AddParticles();
		CG_AddLocalEntities();
	}
	CG_AddViewWeapon( &cg.predictedPlayerState );

	// add buffered sounds
	CG_PlayBufferedSounds();

	// play buffered voice chats
	CG_PlayBufferedVoiceChats();

	// finish up the rest of the refdef
	if ( cg.testModelEntity.hModel ) {
		CG_AddTestModel();
	}
	cg.refdef.time = cg.time;
	memcpy( cg.refdef.areamask, cg.snap->areamask, sizeof( cg.refdef.areamask ) );

	// warning sounds when powerup is wearing off
	CG_PowerupTimerSounds();

	// update audio positions
	trap->S_Respatialize( cg.snap->ps.clientNum, &cg.refdef.vieworg, cg.refdef.viewaxis, inwater );

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	if ( stereoView != STEREO_RIGHT ) {
		cg.frametime = cg.time - cg.oldTime;
		if ( cg.frametime < 0 ) {
			cg.frametime = 0;
		}
		cg.oldTime = cg.time;
		CG_AddLagometerFrameInfo();
	}
	if (timescale->value != cg_timescaleFadeEnd->value) {
		if (timescale->value < cg_timescaleFadeEnd->value) {
			timescale->value += cg_timescaleFadeSpeed->value * ((float)cg.frametime) / 1000;
			if (timescale->value > cg_timescaleFadeEnd->value)
				timescale->value = cg_timescaleFadeEnd->value;
		}
		else {
			timescale->value -= cg_timescaleFadeSpeed->value * ((float)cg.frametime) / 1000;
			if (timescale->value < cg_timescaleFadeEnd->value)
				timescale->value = cg_timescaleFadeEnd->value;
		}
		if (cg_timescaleFadeSpeed->value) {
			trap->Cvar_Set( "timescale", va( "%f", timescale->value ) );
		}
	}

	// actually issue the rendering calls
	CG_DrawActive( stereoView );
}

