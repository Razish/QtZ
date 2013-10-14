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
// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_local.h"


// Modifies the entities position and axis by the given tag location
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	
	// lerp the tag
	trap->R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame, 1.0f - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( &parent->origin, &entity->origin );
	for ( i=0; i<3; i++ )
		VectorMA( &entity->origin, lerped.origin.data[i], &parent->axis[i], &entity->origin );

	MatrixMultiply( lerped.axis, parent->axis, entity->axis );
	entity->backlerp = parent->backlerp;
}

// Modifies the entities position and axis by the given tag location
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	matrix3			tempAxis;

//AxisClear( entity->axis );
	// lerp the tag
	trap->R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame, 1.0f - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( &parent->origin, &entity->origin );
	for ( i=0; i<3; i++ )
		VectorMA( &entity->origin, lerped.origin.data[i], &parent->axis[i], &entity->origin );

	MatrixMultiply( entity->axis, lerped.axis, tempAxis );
	MatrixMultiply( tempAxis, parent->axis, entity->axis );
}



/*

	FUNCTIONS CALLED EACH FRAME

*/

// Also called by event processing code
void CG_SetEntitySoundPosition( centity_t *cent ) {
	if ( cent->currentState.solid == SOLID_BMODEL ) {
		vector3	origin;
		vector3	*v;

		v = &cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
		VectorAdd( &cent->lerpOrigin, v, &origin );
		trap->S_UpdateEntityPosition( cent->currentState.number, &origin );
	} else {
		trap->S_UpdateEntityPosition( cent->currentState.number, &cent->lerpOrigin );
	}
}

// Add continuous entity effects, like local entity emission and lighting
static void CG_EntityEffects( centity_t *cent ) {

	// update sound origins
	CG_SetEntitySoundPosition( cent );

	// add loop sound
	if ( cent->currentState.loopSound ) {
		if (cent->currentState.eType != ET_SPEAKER) {
			trap->S_AddLoopingSound( cent->currentState.number, &cent->lerpOrigin, &vec3_origin, 
				cgs.gameSounds[ cent->currentState.loopSound ] );
		} else {
			trap->S_AddRealLoopingSound( cent->currentState.number, &cent->lerpOrigin, &vec3_origin, 
				cgs.gameSounds[ cent->currentState.loopSound ] );
		}
	}


	// constant light glow
	if ( cent->currentState.constantLight ) {
		int cl;
		float i, r, g, b;

		cl = cent->currentState.constantLight;
		r = (float)(cl			& 0xFF) / 255.0f;
		g = (float)((cl >> 8)	& 0xFF) / 255.0f;
		b = (float)((cl >> 16)	& 0xFF) / 255.0f;
		i = (float)((cl >> 24)	& 0xFF) * 4.0f;
		trap->R_AddLightToScene( &cent->lerpOrigin, i, r, g, b );
	}
}

static void CG_General( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if (!s1->modelindex) {
		return;
	}

	memset (&ent, 0, sizeof(ent));

	// set frame

	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	VectorCopy( &cent->lerpOrigin, &ent.origin);
	VectorCopy( &cent->lerpOrigin, &ent.oldorigin);

	ent.hModel = cgs.gameModels[s1->modelindex];

	// player model
	if (s1->number == cg.snap->ps.clientNum) {
		ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors
	}

	// convert angles to axis
	AnglesToAxis( &cent->lerpAngles, ent.axis );

	// add to refresh list
	trap->R_AddRefEntityToScene (&ent);
}

// Speaker entities can automatically play sounds
static void CG_Speaker( centity_t *cent ) {
	if ( ! cent->currentState.clientNum ) {	// FIXME: use something other than clientNum...
		return;		// not auto triggering
	}

	if ( cg.time < cent->miscTime ) {
		return;
	}

	trap->S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[cent->currentState.eventParm] );

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.clientNum = ent->random * 10;
	cent->miscTime = cg.time + cent->currentState.frame * 100 + (int)(cent->currentState.clientNum * 100 * crandom());
}

static void CG_Item( centity_t *cent ) {
	refEntity_t		ent;
	entityState_t	*es;
	const gitem_t	*item;
	int				msec;
	float			frac;
	float			scale;
	weaponInfo_t	*wi;

	es = &cent->currentState;
	if ( es->modelindex >= bg_numItems ) {
		trap->Error( ERR_DROP, "Bad item index %i on entity", es->modelindex );
	}

	// if set to invisible, skip
	if ( !es->modelindex || ( es->eFlags & EF_NODRAW ) ) {
		return;
	}

	item = &bg_itemlist[ es->modelindex ];
	if ( cg_simpleItems->boolean && item->giType != IT_TEAM ) {
		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_SPRITE;
		VectorCopy( &cent->lerpOrigin, &ent.origin );
		ent.radius = 14;
		ent.customShader = cg_items[es->modelindex].icon;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
		trap->R_AddRefEntityToScene(&ent);
		return;
	}

	// items bob up and down continuously
	scale = 0.005f + cent->currentState.number * 0.00001f;
	cent->lerpOrigin.z += 4 + cosf( ( cg.time + 1000 ) *  scale ) * 4;

	memset (&ent, 0, sizeof(ent));

	// autorotate at one of two speeds
	if ( item->giType == IT_HEALTH ) {
		VectorCopy( &cg.autoAnglesFast, &cent->lerpAngles );
		AxisCopy( cg.autoAxisFast, ent.axis );
	} else {
		VectorCopy( &cg.autoAngles, &cent->lerpAngles );
		AxisCopy( cg.autoAxis, ent.axis );
	}

	wi = NULL;
	// the weapons have their origin where they attatch to player
	// models, so we need to offset them or they will rotate
	// eccentricly
	if ( item->giType == IT_WEAPON ) {
		wi = &cg_weapons[item->giTag];
		cent->lerpOrigin.x -= 
			wi->weaponMidpoint.x * ent.axis[0].x +
			wi->weaponMidpoint.y * ent.axis[1].x +
			wi->weaponMidpoint.z * ent.axis[2].x;
		cent->lerpOrigin.y -= 
			wi->weaponMidpoint.x * ent.axis[0].y +
			wi->weaponMidpoint.y * ent.axis[1].y +
			wi->weaponMidpoint.z * ent.axis[2].y;
		cent->lerpOrigin.z -= 
			wi->weaponMidpoint.x * ent.axis[0].z +
			wi->weaponMidpoint.y * ent.axis[1].z +
			wi->weaponMidpoint.z * ent.axis[2].z;

		cent->lerpOrigin.z += 8;	// an extra height boost
	}
	
	ent.hModel = cg_items[es->modelindex].models[0];

	VectorCopy( &cent->lerpOrigin, &ent.origin);
	VectorCopy( &cent->lerpOrigin, &ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	// if just respawned, slowly scale up
	msec = cg.time - cent->miscTime;
	if ( msec >= 0 && msec < ITEM_SCALEUP_TIME ) {
		frac = (float)msec / ITEM_SCALEUP_TIME;
		VectorScale( &ent.axis[0], frac, &ent.axis[0] );
		VectorScale( &ent.axis[1], frac, &ent.axis[1] );
		VectorScale( &ent.axis[2], frac, &ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
	} else {
		frac = 1.0f;
	}

	// items without glow textures need to keep a minimum light value
	// so they are always visible
	if ( ( item->giType == IT_WEAPON ) ||
		 ( item->giType == IT_ARMOR ) ) {
		ent.renderfx |= RF_MINLIGHT;
	}

	// increase the size of the weapons when they are presented as items
	if ( item->giType == IT_WEAPON ) {
		VectorScale( &ent.axis[0], 1.5f, &ent.axis[0] );
		VectorScale( &ent.axis[1], 1.5f, &ent.axis[1] );
		VectorScale( &ent.axis[2], 1.5f, &ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
		trap->S_AddLoopingSound( cent->currentState.number, &cent->lerpOrigin, &vec3_origin, cgs.media.weaponHoverSound );
	}

	// add to refresh list
	trap->R_AddRefEntityToScene(&ent);

	if ( item->giType == IT_WEAPON && wi->barrelModel ) {
		refEntity_t	barrel;

		memset( &barrel, 0, sizeof( barrel ) );

		barrel.hModel = wi->barrelModel;

		VectorCopy( &ent.lightingOrigin, &barrel.lightingOrigin );
		barrel.shadowPlane = ent.shadowPlane;
		barrel.renderfx = ent.renderfx;

		CG_PositionRotatedEntityOnTag( &barrel, &ent, wi->weaponModel, "tag_barrel" );

		AxisCopy( ent.axis, barrel.axis );
		barrel.nonNormalizedAxes = ent.nonNormalizedAxes;

		trap->R_AddRefEntityToScene( &barrel );
	}

	// accompanying rings / spheres for powerups
	if ( !cg_simpleItems->boolean ) 
	{
		vector3 spinAngles;

		VectorClear( &spinAngles );

		if ( item->giType == IT_HEALTH || item->giType == IT_POWERUP )
		{
			if ( ( ent.hModel = cg_items[es->modelindex].models[1] ) != 0 )
			{
				if ( item->giType == IT_POWERUP )
				{
					ent.origin.z += 12;
					spinAngles.yaw = ( cg.time & 1023 ) * 360 / -1024.0f;
				}
				AnglesToAxis( &spinAngles, ent.axis );
				
				// scale up if respawning
				if ( frac != 1.0f ) {
					VectorScale( &ent.axis[0], frac, &ent.axis[0] );
					VectorScale( &ent.axis[1], frac, &ent.axis[1] );
					VectorScale( &ent.axis[2], frac, &ent.axis[2] );
					ent.nonNormalizedAxes = qtrue;
				}
				trap->R_AddRefEntityToScene( &ent );
			}
		}
	}
}

static void CG_Missile( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	const weaponInfo_t		*weapon;
//	int	col;

	s1 = &cent->currentState;
	if ( s1->weapon >= WP_NUM_WEAPONS )
		s1->weapon = 0;

	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	VectorCopy( &s1->angles, &cent->lerpAngles);

	// add trails
	if ( weapon->missileTrailFunc ) 
 		weapon->missileTrailFunc( cent, weapon );
/*
	if ( cent->currentState.modelindex == TEAM_RED ) {
		col = 1;
	}
	else if ( cent->currentState.modelindex == TEAM_BLUE ) {
		col = 2;
	}
	else {
		col = 0;
	}

	// add dynamic light
	if ( weapon->missileDlight ) {
		trap->R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight, 
			weapon->missileDlightColor[col][0], weapon->missileDlightColor[col][1], weapon->missileDlightColor[col][2] );
	}
*/
	// add dynamic light
	if ( weapon->missileDlight ) {
		trap->R_AddLightToScene(&cent->lerpOrigin, weapon->missileDlight, 
			weapon->missileDlightColor.r, weapon->missileDlightColor.g, weapon->missileDlightColor.z );
	}

	// add missile sound
	if ( weapon->missileSound ) {
		vector3	velocity;

		BG_EvaluateTrajectoryDelta( &cent->currentState.pos, cg.time, &velocity );

		trap->S_AddLoopingSound( cent->currentState.number, &cent->lerpOrigin, &velocity, weapon->missileSound );
	}

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( &cent->lerpOrigin, &ent.origin);
	VectorCopy( &cent->lerpOrigin, &ent.oldorigin);

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	// convert direction of travel into axis
	if ( VectorNormalize2( &s1->pos.trDelta, &ent.axis[0] ) == 0 ) {
		ent.axis[0].z = 1;
	}

	// spin as it moves
	if ( s1->pos.trType != TR_STATIONARY )
		RotateAroundDirection( ent.axis, (float)(cg.time / 4) );
	else
		RotateAroundDirection( ent.axis, (float)s1->time );

	// add to refresh list, possibly with quad glow
	CG_AddRefEntityWithPowerups( &ent, s1, TEAM_FREE );
}

static void CG_Mover( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( &cent->lerpOrigin, &ent.origin);
	VectorCopy( &cent->lerpOrigin, &ent.oldorigin);
	AnglesToAxis( &cent->lerpAngles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = ( cg.time >> 6 ) & 1;

	// get the model, either as a bmodel or a modelindex
	if ( s1->solid == SOLID_BMODEL ) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add to refresh list
	trap->R_AddRefEntityToScene(&ent);

	// add the secondary model
	if ( s1->modelindex2 ) {
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		trap->R_AddRefEntityToScene(&ent);
	}

}

// Also called as an event
void CG_Beam( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( &s1->pos.trBase, &ent.origin );
	VectorCopy( &s1->origin2, &ent.oldorigin );
	AxisClear( ent.axis );
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	trap->R_AddRefEntityToScene(&ent);
}

static void CG_Portal( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( &cent->lerpOrigin, &ent.origin );
	VectorCopy( &s1->origin2, &ent.oldorigin );
	ByteToDir( s1->eventParm, &ent.axis[0] );
	PerpendicularVector( &ent.axis[1], &ent.axis[0] );

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract( &vec3_origin, &ent.axis[1], &ent.axis[1] );

	CrossProduct( &ent.axis[0], &ent.axis[1], &ent.axis[2] );
	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->powerups;
	ent.frame = s1->frame;		// rotation speed
	ent.skinNum = (int)(s1->clientNum/256.0f * 360);	// roll offset

	// add to refresh list
	trap->R_AddRefEntityToScene(&ent);
}

// Also called by client movement prediction code
void CG_AdjustPositionForMover(const vector3 *in, int moverNum, int fromTime, int toTime, vector3 *out, vector3 *angles_in, vector3 *angles_out) {
	centity_t	*cent;
	vector3	oldOrigin, origin, deltaOrigin;
	vector3	oldAngles, angles, deltaAngles;

	if ( moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL ) {
		VectorCopy( in, out );
		VectorCopy(angles_in, angles_out);
		return;
	}

	cent = &cg_entities[ moverNum ];
	if ( cent->currentState.eType != ET_MOVER ) {
		VectorCopy( in, out );
		VectorCopy(angles_in, angles_out);
		return;
	}

	BG_EvaluateTrajectory( &cent->currentState.pos, fromTime, &oldOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, fromTime, &oldAngles );

	BG_EvaluateTrajectory( &cent->currentState.pos, toTime, &origin );
	BG_EvaluateTrajectory( &cent->currentState.apos, toTime, &angles );

	VectorSubtract( &origin, &oldOrigin, &deltaOrigin );
	VectorSubtract( &angles, &oldAngles, &deltaAngles );

	VectorAdd( in, &deltaOrigin, out );
	VectorAdd( angles_in, &deltaAngles, angles_out );
	// FIXME: origin change when on a rotating object
}

static void CG_InterpolateEntityPosition( centity_t *cent ) {
	vector3		current, next;
	float		f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if ( cg.nextSnap == NULL ) {
		trap->Error( ERR_DROP, "CG_InterpoateEntityPosition: cg.nextSnap == NULL" );
	}

	f = cg.frameInterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, &current );
	BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, &next );

	cent->lerpOrigin.x = current.x + f * ( next.x - current.x );
	cent->lerpOrigin.y = current.y + f * ( next.y - current.y );
	cent->lerpOrigin.z = current.z + f * ( next.z - current.z );

	BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, &current );
	BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, &next );

	cent->lerpAngles.x = LerpAngle( current.x, next.x, f );
	cent->lerpAngles.y = LerpAngle( current.y, next.y, f );
	cent->lerpAngles.z = LerpAngle( current.z, next.z, f );

}

static void CG_CalcEntityLerpPositions( centity_t *cent )
{
	if ( !cg_smoothClients->boolean )
	{// if this player does not want to see extrapolated players, make sure the clients use TR_INTERPOLATE
		if ( cent->currentState.number < MAX_CLIENTS ) {
			cent->currentState.pos.trType = TR_INTERPOLATE;
			cent->nextState.pos.trType = TR_INTERPOLATE;
		}
	}

	if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	if ( cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP && cent->currentState.number < MAX_CLIENTS )
	{// first see if we can interpolate between two snaps for linear extrapolated clients
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// just use the current frame and evaluate as best we can
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, &cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, &cent->lerpAngles );

	// adjust for riding a mover if it wasn't rolled into the predicted playerstate
	if ( cent != &cg.predictedPlayerEntity )
		CG_AdjustPositionForMover( &cent->lerpOrigin, cent->currentState.groundEntityNum, cg.snap->serverTime, cg.time, &cent->lerpOrigin, &cent->lerpAngles, &cent->lerpAngles );
}

static void CG_TeamBase( centity_t *cent ) {
	refEntity_t model;

	if ( cgs.gametype == GT_FLAGS || cgs.gametype == GT_TROJAN ) {
		// show the flag base
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( &cent->lerpOrigin, &model.lightingOrigin );
		VectorCopy( &cent->lerpOrigin, &model.origin );
		AnglesToAxis( &cent->currentState.angles, model.axis );
		if ( cent->currentState.modelindex == TEAM_RED ) {
			model.hModel = cgs.media.redFlagBaseModel;
		}
		else if ( cent->currentState.modelindex == TEAM_BLUE ) {
			model.hModel = cgs.media.blueFlagBaseModel;
		}
		else {
			model.hModel = cgs.media.neutralFlagBaseModel;
		}
		trap->R_AddRefEntityToScene( &model );
	}
}

static void CG_AddCEntity( centity_t *cent ) {
	// event-only entities will have been dealt with already
	if ( cent->currentState.eType >= ET_EVENTS ) {
		return;
	}

	// calculate the current origin
	CG_CalcEntityLerpPositions( cent );

	// add automatic effects
	CG_EntityEffects( cent );

	switch ( cent->currentState.eType ) {
	default:
		trap->Error( ERR_DROP, "Bad entity type: %i", cent->currentState.eType );
		break;
	case ET_INVISIBLE:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
		break;
	case ET_GENERAL:
		CG_General( cent );
		break;
	case ET_PLAYER:
		CG_Player( cent );
		break;
	case ET_ITEM:
		CG_Item( cent );
		break;
	case ET_MISSILE:
		CG_Missile( cent );
		break;
	case ET_MOVER:
		CG_Mover( cent );
		break;
	case ET_BEAM:
		CG_Beam( cent );
		break;
	case ET_PORTAL:
		CG_Portal( cent );
		break;
	case ET_SPEAKER:
		CG_Speaker( cent );
		break;
	case ET_TEAM:
		CG_TeamBase( cent );
		break;
	}
}

void CG_AddPacketEntities( void ) {
	int					num;
	centity_t			*cent;
	playerState_t		*ps;

	// set cg.frameInterpolation
	if ( cg.nextSnap ) {
		int		delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if ( delta == 0 ) {
			cg.frameInterpolation = 0;
		} else {
			cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime ) / delta;
		}
	} else {
		cg.frameInterpolation = 0;	// actually, it should never be used, because 
									// no entities should be marked as interpolating
	}

	// the auto-rotating items will all have the same axis
	cg.autoAngles.x = 0;
	cg.autoAngles.y = ( cg.time & 2047 ) * 360 / 2048.0f;
	cg.autoAngles.z = 0;

	cg.autoAnglesFast.x = 0;
	cg.autoAnglesFast.y = ( cg.time & 1023 ) * 360 / 1024.0f;
	cg.autoAnglesFast.z = 0;

	AnglesToAxis( &cg.autoAngles, cg.autoAxis );
	AnglesToAxis( &cg.autoAnglesFast, cg.autoAxisFast );

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );
	CG_AddCEntity( &cg.predictedPlayerEntity );

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ] );

	// add each entity sent over by the server
	for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		CG_AddCEntity( cent );
	}
}

