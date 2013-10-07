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
// cg_effects.c -- these functions generate localentities, usually as a result
// of event processing

#include "cg_local.h"

// Adds a smoke puff or blood trail localEntity.
localEntity_t *CG_SmokePuff( const vector3 *p, const vector3 *vel, 
				   float radius,
				   float r, float g, float b, float a,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader ) {
	static int	seed = 0x92;
	localEntity_t	*le;
	refEntity_t		*re;
//	int fadeInTime = startTime + duration / 2;

	le = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius = radius;

	re = &le->refEntity;
	re->rotation = Q_random( &seed ) * 360;
	re->radius = radius;
	re->shaderTime = startTime / 1000.0f;

	le->leType = LE_MOVE_SCALE_FADE;
	le->startTime = startTime;
	le->fadeInTime = fadeInTime;
	le->endTime = startTime + (int)duration;
	if ( fadeInTime > startTime ) {
		le->lifeRate = 1.0f / ( le->endTime - le->fadeInTime );
	}
	else {
		le->lifeRate = 1.0f / ( le->endTime - le->startTime );
	}
	le->color.r = r;
	le->color.g = g; 
	le->color.b = b;
	le->color.a = a;


	le->pos.trType = TR_LINEAR;
	le->pos.trTime = startTime;
	VectorCopy( vel, &le->pos.trDelta );
	VectorCopy( p, &le->pos.trBase );

	VectorCopy( p, &re->origin );
	re->customShader = hShader;

	re->shaderRGBA[0] = (byte)(le->color.r * 255.0f);
	re->shaderRGBA[1] = (byte)(le->color.g * 255.0f);
	re->shaderRGBA[2] = (byte)(le->color.b * 255.0f);
	re->shaderRGBA[3] = 0xff;

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}

// Player teleporting in or out
void CG_SpawnEffect( vector3 *org ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + 500;
	le->lifeRate = 1.0f / ( le->endTime - le->startTime );

	le->color.r = le->color.g = le->color.b = le->color.a = 1.0f;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.teleportEffectModel;
	AxisClear( re->axis );

	VectorCopy( org, &re->origin );
	re->origin.z += 16;
}

void CG_ScorePlum( int client, vector3 *org, int score ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vector3			angles;
	static vector3 lastPos;

	// only visualize for the client that scored
	if (client != cg.predictedPlayerState.clientNum || cg_scorePlums->integer == 0) {
		return;
	}

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_SCOREPLUM;
	le->startTime = cg.time;
	le->endTime = cg.time + 4000;
	le->lifeRate = 1.0f / ( le->endTime - le->startTime );

	
	le->color.r = le->color.g = le->color.b = le->color.a = 1.0f;
	le->radius = (float)score;
	
	VectorCopy( org, &le->pos.trBase );
	if (org->z >= lastPos.z - 20 && org->z <= lastPos.z + 20) {
		le->pos.trBase.z -= 20;
	}

	//trap->Print( "Plum origin %i %i %i -- %i\n", (int)org[0], (int)org[1], (int)org[2], (int)Distance(org, lastPos));
	VectorCopy(org, &lastPos);


	re = &le->refEntity;

	re->reType = RT_SPRITE;
	re->radius = 16;

	VectorClear(&angles);
	AnglesToAxis( &angles, re->axis );
}

localEntity_t *CG_MakeExplosion( vector3 *origin, vector3 *dir, 
								qhandle_t hModel, qhandle_t shader,
								int msec, qboolean isSprite ) {
	float			ang;
	localEntity_t	*ex;
	int				offset;
	vector3			tmpVec, newOrigin;

	if ( msec <= 0 ) {
		trap->Error( ERR_DROP, "CG_MakeExplosion: msec = %i", msec );
	}

	// skew the time a bit so they aren't all in sync
	offset = rand() & 63;

	ex = CG_AllocLocalEntity();
	if ( isSprite ) {
		ex->leType = LE_SPRITE_EXPLOSION;

		// randomly rotate sprite orientation
		ex->refEntity.rotation = (float)(rand() % 360);
		VectorScale( dir, 16, &tmpVec );
		VectorAdd( &tmpVec, origin, &newOrigin );
	} else {
		ex->leType = LE_EXPLOSION;
		VectorCopy( origin, &newOrigin );

		// set axis with random rotate
		if ( !dir ) {
			AxisClear( ex->refEntity.axis );
		} else {
			ang = (float)(rand() % 360);
			VectorCopy( dir, &ex->refEntity.axis[0] );
			RotateAroundDirection( ex->refEntity.axis, ang );
		}
	}

	ex->startTime = cg.time - offset;
	ex->endTime = ex->startTime + msec;

	// bias the time so all shader effects start correctly
	ex->refEntity.shaderTime = ex->startTime / 1000.0f;

	ex->refEntity.hModel = hModel;
	ex->refEntity.customShader = shader;

	// set origin
	VectorCopy( &newOrigin, &ex->refEntity.origin );
	VectorCopy( &newOrigin, &ex->refEntity.oldorigin );

	ex->color.r = ex->color.g = ex->color.b = 1.0;

	return ex;
}

// This is the spurt of blood when a character gets hit
void CG_Bleed( vector3 *origin, int entityNum ) {
	localEntity_t	*ex;

	if ( !(cg_gore->integer & GORELEVEL_BLOOD) )
		return;

	ex = CG_AllocLocalEntity();
	ex->leType = LE_EXPLOSION;

	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 500;
	
	VectorCopy ( origin, &ex->refEntity.origin);
	ex->refEntity.reType = RT_SPRITE;
	ex->refEntity.rotation = (float)(rand() % 360);
	ex->refEntity.radius = 24;

	ex->refEntity.customShader = cgs.media.bloodExplosionShader;

	// don't show player's own blood in view
	if ( entityNum == cg.snap->ps.clientNum ) {
		ex->refEntity.renderfx |= RF_THIRD_PERSON;
	}
}

void CG_LaunchGib( vector3 *origin, vector3 *velocity, qhandle_t hModel ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 5000 + (int)(random() * 3000);

	VectorCopy( origin, &re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	VectorCopy( origin, &le->pos.trBase );
	VectorCopy( velocity, &le->pos.trDelta );
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.6f;

	le->leBounceSoundType = LEBS_BLOOD;
	le->leMarkType = LEMT_BLOOD;
}

#define	GIB_VELOCITY	250
#define	GIB_JUMP		250

// Generated a bunch of gibs launching out from the bodies location
void CG_GibPlayer( vector3 *playerOrigin ) {
	vector3	origin, velocity;

	if ( !(cg_gore->integer & GORELEVEL_BLOOD) )
		return;

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*GIB_VELOCITY;
	velocity.y = crandom()*GIB_VELOCITY;
	velocity.z = crandom()*GIB_VELOCITY + GIB_JUMP;
	if ( rand() & 1 )
		CG_LaunchGib( &origin, &velocity, cgs.media.gibSkull );
	else
		CG_LaunchGib( &origin, &velocity, cgs.media.gibBrain );

	// allow gibs to be turned off for speed
	if ( !(cg_gore->integer & GORELEVEL_GIBS) )
		return;

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*GIB_VELOCITY;
	velocity.y = crandom()*GIB_VELOCITY;
	velocity.z = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( &origin, &velocity, cgs.media.gibAbdomen );

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*GIB_VELOCITY;
	velocity.y = crandom()*GIB_VELOCITY;
	velocity.z = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( &origin, &velocity, cgs.media.gibArm );

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*GIB_VELOCITY;
	velocity.y = crandom()*GIB_VELOCITY;
	velocity.z = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( &origin, &velocity, cgs.media.gibChest );

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*GIB_VELOCITY;
	velocity.y = crandom()*GIB_VELOCITY;
	velocity.z = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( &origin, &velocity, cgs.media.gibFist );

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*GIB_VELOCITY;
	velocity.y = crandom()*GIB_VELOCITY;
	velocity.z = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( &origin, &velocity, cgs.media.gibFoot );

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*GIB_VELOCITY;
	velocity.y = crandom()*GIB_VELOCITY;
	velocity.z = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( &origin, &velocity, cgs.media.gibForearm );

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*GIB_VELOCITY;
	velocity.y = crandom()*GIB_VELOCITY;
	velocity.z = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( &origin, &velocity, cgs.media.gibIntestine );

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*GIB_VELOCITY;
	velocity.y = crandom()*GIB_VELOCITY;
	velocity.z = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( &origin, &velocity, cgs.media.gibLeg );

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*GIB_VELOCITY;
	velocity.y = crandom()*GIB_VELOCITY;
	velocity.z = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( &origin, &velocity, cgs.media.gibLeg );
}

void CG_LaunchExplode( vector3 *origin, vector3 *velocity, qhandle_t hModel ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 10000 + (int)(random() * 6000);

	VectorCopy( origin, &re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	VectorCopy( origin, &le->pos.trBase );
	VectorCopy( velocity, &le->pos.trDelta );
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.1f;

	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

#define	EXP_VELOCITY	100
#define	EXP_JUMP		150

// Generated a bunch of gibs launching out from the bodies location
void CG_BigExplode( vector3 *playerOrigin ) {
	vector3	origin, velocity;

	if ( !(cg_gore->integer & GORELEVEL_BLOOD) )
		return;

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*EXP_VELOCITY;
	velocity.y = crandom()*EXP_VELOCITY;
	velocity.z = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( &origin, &velocity, cgs.media.smoke2 );

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*EXP_VELOCITY;
	velocity.y = crandom()*EXP_VELOCITY;
	velocity.z = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( &origin, &velocity, cgs.media.smoke2 );

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*EXP_VELOCITY*1.5f;
	velocity.y = crandom()*EXP_VELOCITY*1.5f;
	velocity.z = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( &origin, &velocity, cgs.media.smoke2 );

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*EXP_VELOCITY*2.0f;
	velocity.y = crandom()*EXP_VELOCITY*2.0f;
	velocity.z = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( &origin, &velocity, cgs.media.smoke2 );

	VectorCopy( playerOrigin, &origin );
	velocity.x = crandom()*EXP_VELOCITY*2.5f;
	velocity.y = crandom()*EXP_VELOCITY*2.5f;
	velocity.z = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( &origin, &velocity, cgs.media.smoke2 );
}

