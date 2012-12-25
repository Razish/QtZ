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
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_weaponfx.h"

/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum ) {
	weaponInfo_t	*weaponInfo;
	gitem_t			*item, *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	if ( !item->classname ) {
		CG_Error( "Couldn't find weapon %i", weaponNum );
	}
	CG_RegisterItemVisuals( item - bg_itemlist );

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel( item->world_model[0] );

	// calc midpoint for rotation
	trap_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader( item->icon );
	weaponInfo->ammoIcon = trap_R_RegisterShader( item->icon );

	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum ) {
			break;
		}
	}
	if ( ammo->classname && ammo->world_model[0] ) {
		weaponInfo->ammoModel = trap_R_RegisterModel( ammo->world_model[0] );
	}

	strcpy( path, item->world_model[0] );
	COM_StripExtension(path, path, sizeof(path));
	strcat( path, "_flash" );
	weaponInfo->flashModel = trap_R_RegisterModel( path );

	strcpy( path, item->world_model[0] );
	COM_StripExtension(path, path, sizeof(path));
	strcat( path, "_barrel" );
	weaponInfo->barrelModel = trap_R_RegisterModel( path );

	strcpy( path, item->world_model[0] );
	COM_StripExtension(path, path, sizeof(path));
	strcat( path, "_hand" );
	weaponInfo->handsModel = trap_R_RegisterModel( path );

	if ( !weaponInfo->handsModel ) {
		weaponInfo->handsModel = trap_R_RegisterModel( "models/weapons2/shotgun/shotgun_hand.md3" );
	}

	switch ( weaponNum ) {
	//RAZMARK: ADDING NEW WEAPONS
	case WP_QUANTIZER:
		weaponInfo->missileTrailFunc					= FX_Quantizer_Missile;
//		cgs.effects.weapons.quantizerShotEffect			= trap_FX_RegisterEffect( "quantizer/shot" );

		weaponInfo->missileHitWallFunc					= FX_Quantizer_HitWall;
//		cgs.effects.weapons.quantizerHitWallEffect		= trap_FX_RegisterEffect( "quantizer/hitwall" );

		weaponInfo->missileHitPlayerFunc				= FX_Quantizer_HitPlayer;
//		cgs.effects.weapons.quantizerHitPlayerEffect	= trap_FX_RegisterEffect( "quantizer/hitplayer" );

		weaponInfo->flashSound[0]						= trap_S_RegisterSound( "sound/weapons/quantizer/fire", qfalse );
//		weaponInfo->muzzleEffect						= trap_FX_RegisterEffect( "quantizer/muzzle" );
		break;

	case WP_REPEATER:
		weaponInfo->missileTrailFunc					= FX_Repeater_Missile;
		weaponInfo->missileHitPlayerFunc				= FX_Repeater_HitPlayer;
		weaponInfo->missileHitWallFunc					= FX_Repeater_HitWall;

		weaponInfo->flashSound[0]						= trap_S_RegisterSound( "sound/weapons/repeater/fire", qfalse );
		break;

	case WP_SPLICER:
		weaponInfo->fireFunc							= FX_Splicer_Beam;
		weaponInfo->missileHitPlayerFunc				= FX_Splicer_HitPlayer;
		weaponInfo->missileHitWallFunc					= FX_Splicer_HitWall;

		weaponInfo->flashSound[0]						= trap_S_RegisterSound( "sound/weapons/splicer/fire", qfalse );

		cgs.media.weapons.splicerCore					= trap_R_RegisterShader( "splicerCore" ); // weapons.shader
		cgs.media.weapons.splicerFlare					= trap_R_RegisterShader( "splicerFlare" ); // weapons.shader
		break;

	case WP_MORTAR:
		weaponInfo->missileTrailFunc					= FX_Mortar_Missile;
		weaponInfo->missileHitPlayerFunc				= FX_Mortar_HitPlayer;
		weaponInfo->missileHitWallFunc					= FX_Mortar_HitWall;

		weaponInfo->flashSound[0]						= trap_S_RegisterSound( "sound/weapons/mortar/fire", qfalse );
		cgs.media.weapons.mortarProjectile				= trap_R_RegisterModel( "models/weapons/mortar/projectile.md3" );
		weaponInfo->firingSound							= NULL_SOUND;
		weaponInfo->missileModel						= NULL_HANDLE;
		weaponInfo->missileSound						= NULL_SOUND;
		break;

	case WP_DIVERGENCE:
		weaponInfo->fireFunc							= FX_Divergence_Fire;

		weaponInfo->readySound							= trap_S_RegisterSound( "sound/weapons/divergence/idle", qfalse );
		weaponInfo->flashSound[0]						= trap_S_RegisterSound( "sound/weapons/divergence/fire", qfalse );
		cgs.media.weapons.divergenceCore				= trap_R_RegisterShader( "divergenceCore" ); // weapons.shader
		break;

	 default:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav", qfalse );
		break;
	}
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	gitem_t			*item;

	if ( itemNum < 0 || itemNum >= bg_numItems ) {
		CG_Error( "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems-1 );
	}

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( *itemInfo ) );
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap_R_RegisterModel( item->world_model[0] );

	itemInfo->icon = trap_R_RegisterShader( item->icon );

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH || 
		item->giType == IT_ARMOR || item->giType == IT_HOLDABLE ) {
		if ( item->world_model[1] ) {
			itemInfo->models[1] = trap_R_RegisterModel( item->world_model[1] );
		}
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame( clientInfo_t *ci, int frame ) {

	// change weapon
	if ( frame >= ci->animations[TORSO_DROP].firstFrame 
		&& frame < ci->animations[TORSO_DROP].firstFrame + 9 ) {
		return frame - ci->animations[TORSO_DROP].firstFrame + 6;
	}

	// stand attack
	if ( frame >= ci->animations[TORSO_ATTACK].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK].firstFrame;
	}

	// stand attack 2
	if ( frame >= ci->animations[TORSO_ATTACK2].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK2].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstFrame;
	}
	
	return 0;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 )	scale = -cg.xyspeed;
	else					scale =  cg.xyspeed;

	if ( cg_gunBobEnable.integer )
	{// gun angles from bobbing
		struct { float pitch; float yaw; float roll; } gunBob;
		if ( sscanf( cg_gunBob.string, "%f %f %f", &gunBob.pitch, &gunBob.yaw, &gunBob.roll ) != 3 )
		{
			gunBob.pitch = 0.005f;
			gunBob.yaw = 0.01f;
			gunBob.roll = 0.005f;
		}
		angles[PITCH]	+= scale * cg.bobfracsin * gunBob.pitch;
		angles[YAW]		+= scale * cg.bobfracsin * gunBob.yaw;
		angles[ROLL]	+= scale * cg.bobfracsin * gunBob.roll;

		// drop the weapon when landing
		delta = cg.time - cg.landTime;
			 if ( delta < LAND_DEFLECT_TIME )						origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
		else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME )	origin[2] += cg.landChange*0.25 * (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;

		// drop the weapon when stair climbing
		delta = cg.time - cg.stepTime;
			 if ( delta < STEP_TIME/2 )		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
		else if ( delta < STEP_TIME )		origin[2] -= cg.stepChange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);
	}

	if ( cg_gunIdleDriftEnable.integer )
	{// idle drift
		struct { float pitch; float yaw; float roll; float speed; } gunIdleDrift;
		if ( sscanf( cg_gunIdleDrift.string, "%f %f %f %f", &gunIdleDrift.pitch, &gunIdleDrift.yaw, &gunIdleDrift.roll, &gunIdleDrift.speed ) != 4 )
		{
			gunIdleDrift.pitch = 0.01f;
			gunIdleDrift.yaw = 0.01f;
			gunIdleDrift.roll = 0.01f;
			gunIdleDrift.speed = 0.001f;
		}
		scale = cg.xyspeed + 40;
		fracsin = sin( cg.time * gunIdleDrift.speed );
		angles[PITCH]	+= scale * fracsin * gunIdleDrift.pitch;
		angles[YAW]		+= scale * fracsin * gunIdleDrift.yaw;
		angles[ROLL]	+= scale * fracsin * gunIdleDrift.roll;
	}
}

/*
======================
CG_MachinegunSpinAngle
======================
*/
#define		SPIN_SPEED	0.9
#define		COAST_TIME	1000
static float	CG_MachinegunSpinAngle( centity_t *cent ) {
	int		delta;
	float	angle;
	float	speed;

	delta = cg.time - cent->pe.barrelTime;
	if ( cent->pe.barrelSpinning ) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
	}

	return angle;
}


/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) {
	// add powerup effects
	trap_R_AddRefEntityToScene( gun );

	if ( powerups & ( 1 << PW_QUAD ) ) {
		gun->customShader = cgs.media.quadWeaponShader;
		trap_R_AddRefEntityToScene( gun );
	}
}


/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team ) {
	refEntity_t	gun;
	refEntity_t	barrel;
	refEntity_t	flash;
	vec3_t		angles;
	weapon_t	weaponNum;
	weaponInfo_t	*weapon;
	centity_t	*nonPredictedCent;
	orientation_t	lerped;

	weaponNum = cent->currentState.weapon;

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if( weaponNum == WP_DIVERGENCE ) {
#if 0
		clientInfo_t *ci = &cgs.clientinfo[cent->currentState.clientNum];
		if( cent->pe.railFireTime + 1500 > cg.time ) {
			int scale = 255 * ( cg.time - cent->pe.railFireTime ) / 1500;
			gun.shaderRGBA[0] = ( ci->c1RGBA[0] * scale ) >> 8;
			gun.shaderRGBA[1] = ( ci->c1RGBA[1] * scale ) >> 8;
			gun.shaderRGBA[2] = ( ci->c1RGBA[2] * scale ) >> 8;
			gun.shaderRGBA[3] = 255;
		}
		else {
			Byte4Copy( ci->c1RGBA, gun.shaderRGBA );
		}
#else
		float cooldown = MIN( (float)cg.predictedPlayerState.weaponCooldown[cg.predictedPlayerState.weapon], (float)weaponData[cg.predictedPlayerState.weapon].maxFireTime );
		float cdPoint =  cooldown / (float)weaponData[cg.predictedPlayerState.weapon].maxFireTime;
		float blueHue = 200.0f/1.0f, redHue = 8.0f;
		vec3_t rgb = {0.0f};
		float hue = blueHue + cdPoint*(redHue - blueHue);
		HSL2RGB( hue/360.0f, 1.0f, 0.5f, &rgb[0], &rgb[1], &rgb[2] );
	//	Com_Printf( "%.2f, %.2f, %.2f\n", (float)cg.predictedPlayerState.qtz.weaponCooldown[cg.predictedPlayerState.weapon], (float)weaponData[cg.predictedPlayerState.weapon].maxFireTime, hue );
		gun.shaderRGBA[0] = rgb[0]*255;
		gun.shaderRGBA[1] = rgb[1]*255;
		gun.shaderRGBA[2] = rgb[2]*255;
		gun.shaderRGBA[3] = 255;
#endif
	}

	gun.hModel = weapon->weaponModel;
	if (!gun.hModel) {
		return;
	}

	if ( !ps ) {
		// add weapon ready sound
		cent->pe.lightningFiring = qfalse;
		if ( ( cent->currentState.eFlags & EF_FIRING ) && weapon->firingSound ) {
			// lightning gun and guantlet make a different sound when fire is held down
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound );
			cent->pe.lightningFiring = qtrue;
		} else if ( weapon->readySound ) {
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound );
		}
	}

	trap_R_LerpTag(&lerped, parent->hModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, "tag_weapon");
	VectorCopy(parent->origin, gun.origin);

	VectorMA(gun.origin, lerped.origin[0], parent->axis[0], gun.origin);

	// Make weapon appear left-handed for 2 and centered for 3
	if(ps && cg_drawGun.integer == 2)
		VectorMA(gun.origin, -lerped.origin[1], parent->axis[1], gun.origin);
	else if(!ps || cg_drawGun.integer != 3)
	       	VectorMA(gun.origin, lerped.origin[1], parent->axis[1], gun.origin);

	VectorMA(gun.origin, lerped.origin[2], parent->axis[2], gun.origin);

	MatrixMultiply(lerped.axis, ((refEntity_t *)parent)->axis, gun.axis);
	gun.backlerp = parent->backlerp;

	CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups );

	// add the spinning barrel
	if ( weapon->barrelModel ) {
		memset( &barrel, 0, sizeof( barrel ) );
		VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
		barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = parent->renderfx;

		barrel.hModel = weapon->barrelModel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = CG_MachinegunSpinAngle( cent );
		AnglesToAxis( angles, barrel.axis );

		CG_PositionRotatedEntityOnTag( &barrel, &gun, weapon->weaponModel, "tag_barrel" );

		CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
	}

	memset( &flash, 0, sizeof( flash ) );
//	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );

	CG_PositionEntityOnTag( &flash, &gun, gun.hModel, "tag_flash");
	//QtZ: Added from JA
	VectorCopy(flash.origin, cg.lastFPFlashPoint);

	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashModel;
	if (!flash.hModel) {
//		return;
	}
	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis( angles, flash.axis );

	// colorize the railgun blast
	if ( weaponNum == WP_DIVERGENCE ) {
#if 0
		clientInfo_t	*ci;

		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		flash.shaderRGBA[0] = 255 * ci->color1[0];
		flash.shaderRGBA[1] = 255 * ci->color1[1];
		flash.shaderRGBA[2] = 255 * ci->color1[2];
#else
		vec3_t flashColour = { 0.0f, 0.878431f, 1.0f };

		if ( cgs.gametype >= GT_TEAM )
		{
			if ( cgs.clientinfo[ cent->currentState.clientNum ].team == TEAM_RED )
				VectorSet( flashColour, 1.0f, 0.0f, 0.0f );
			else if ( cgs.clientinfo[ cent->currentState.clientNum ].team == TEAM_BLUE )
				VectorSet( flashColour, 0.0f, 0.0f, 1.0f );
		}

		flash.shaderRGBA[0] = flashColour[0]*255;
		flash.shaderRGBA[1] = flashColour[1]*255;
		flash.shaderRGBA[2] = flashColour[2]*255;
#endif
	}

	CG_PositionRotatedEntityOnTag( &flash, &gun, weapon->weaponModel, "tag_flash");
	trap_R_AddRefEntityToScene( &flash );

	if ( ps || cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum ) {
		// add lightning bolt
	//	CG_LightningBolt( nonPredictedCent, flash.origin );

		if ( weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2] ) {
			trap_R_AddLightToScene( flash.origin, 300 + (rand()&31), weapon->flashDlightColor[0],
				weapon->flashDlightColor[1], weapon->flashDlightColor[2] );
		}

		if ( weaponNum == WP_DIVERGENCE )
		{
			float cdPoint = MIN(cg.predictedPlayerState.weaponCooldown[weaponNum], weaponData[weaponNum].maxFireTime) / weaponData[weaponNum].maxFireTime;
			vec3_t flashColour = { 0.0f, 0.878431f, 1.0f };

			if ( cgs.gametype >= GT_TEAM )
			{
				if ( cgs.clientinfo[ cent->currentState.clientNum ].team == TEAM_RED )
					VectorSet( flashColour, 1.0f, 0.0f, 0.0f );
				else if ( cgs.clientinfo[ cent->currentState.clientNum ].team == TEAM_BLUE )
					VectorSet( flashColour, 0.0f, 0.0f, 1.0f );
			}

			trap_R_AddLightToScene( flash.origin, 300+(1000*cdPoint), flashColour[0], flashColour[1], flashColour[2] );
		}
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
extern float zoomFov;
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t		hand;
	centity_t		*cent;
	clientInfo_t	*ci;
	vec3_t			angles;
	weaponInfo_t	*weapon;
	float	cgFov = cg_fov.value;
	struct { float x; float y; float z; } pos;

	if ( sscanf( cg_gunAlign.string, "%f %f %f", &pos.x, &pos.y, &pos.z ) != 3 )
		pos.x = pos.y = pos.z = 0.0f;

	if (cgFov < 1)
		cgFov = 1;
	if (cgFov > 180)
		cgFov = 180;

	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;
	}

	// no gun if in third person view or a camera is active
	//if ( cg.renderingThirdPerson || cg.cameraMode) {
	if ( cg.renderingThirdPerson ) {
		return;
	}


	// allow the gun to be completely removed
	if ( !cg_drawGun.integer ) {
	//	vec3_t		origin;

		if ( cg.predictedPlayerState.eFlags & EF_FIRING ) {
			// special hack for lightning gun...
			//RAZFIXME: Splicer with cg_drawGun 0
		//	VectorCopy( cg.refdef.vieworg, origin );
		//	VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
		//	CG_LightningBolt( &cg_entities[ps->clientNum], origin );
		}
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	cent = &cg.predictedPlayerEntity;	// &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon( ps->weapon );
	weapon = &cg_weapons[ ps->weapon ];

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	{
		vec3_t gunLerp = { 0.0f };
		float point = 1.0f + (zoomFov-cg_zoomFov.value)/(cgFov-cg_zoomFov.value) * -1.0f;
		struct { float x; float y; float z; } gunAlign;
		struct { float x; float y; float z; } gunZoomAlign;

		if ( sscanf( cg_gunAlign.string, "%f %f %f", &gunAlign.x, &gunAlign.y, &gunAlign.z ) != 3 )
			gunAlign.x = gunAlign.y = gunAlign.z = 0.0f;

		if ( sscanf( cg_gunZoomAlign.string, "%f %f %f", &gunZoomAlign.x, &gunZoomAlign.y, &gunZoomAlign.z ) != 3 )
			gunZoomAlign.x = gunZoomAlign.y = gunZoomAlign.z = 0.0f;

		//lerp from base position to zoom position by how far we've zoomed
		gunLerp[0] = gunAlign.x + point*(gunZoomAlign.x - gunAlign.x);
		gunLerp[1] = gunAlign.y + point*(gunZoomAlign.y - gunAlign.y);
		gunLerp[2] = gunAlign.z + point*(gunZoomAlign.z - gunAlign.z);

		//now offset to hand position
		VectorMA( hand.origin, gunLerp[0], cg.refdef.viewaxis[0], hand.origin );
		VectorMA( hand.origin, gunLerp[1], cg.refdef.viewaxis[1], hand.origin );
		VectorMA( hand.origin, gunLerp[2], cg.refdef.viewaxis[2], hand.origin );
	}

	AnglesToAxis( angles, hand.axis );

	// map torso animations to weapon animations
	// get clientinfo for animation map
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame );
	hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame );
	hand.backlerp = cent->pe.torso.backlerp;

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity, ps->persistant[PERS_TEAM] );
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect( void ) {
	int		i;
	int		bits;
	int		count;
	int		x, y, w;
	char	*name;
	float	*color;

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );
	if ( !color ) {
		return;
	}
	trap_R_SetColor( color );

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// count the number of weapons owned
	bits = cg.snap->ps.stats[ STAT_WEAPONS ];
	count = 0;
	for ( i = 1 ; i < WP_NUM_WEAPONS ; i++ ) {
		if ( bits & ( 1 << i ) ) {
			count++;
		}
	}

	x = (SCREEN_WIDTH/2) - count * 20;
	y = 380;

	for ( i = 1 ; i < WP_NUM_WEAPONS ; i++ ) {
		if ( !( bits & ( 1 << i ) ) ) {
			continue;
		}

		CG_RegisterWeapon( i );

		// draw weapon icon
		CG_DrawPic( x, y, 32, 32, cg_weapons[i].weaponIcon );

		// draw selection marker
		if ( i == cg.weaponSelect ) {
			CG_DrawPic( x-4, y-4, 40, 40, cgs.media.selectShader );
		}

		// no ammo cross on top
		if ( !cg.snap->ps.ammo[ i ] ) {
			CG_DrawPic( x, y, 32, 32, cgs.media.noammoShader );
		}

		x += 40;
	}

	// draw the selected name
	if ( cg_weapons[ cg.weaponSelect ].item ) {
		name = cg_weapons[ cg.weaponSelect ].item->pickup_name;
		if ( name ) {
			w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;
			x = ( SCREEN_WIDTH - w ) / 2;
			CG_DrawBigStringColor(x, y - 22, name, color);
		}
	}

	trap_R_SetColor( NULL );
}


/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable( int i ) {
	if ( !cg.snap->ps.ammo[i] ) {
		return qfalse;
	}
	if ( ! (cg.snap->ps.stats[ STAT_WEAPONS ] & ( 1 << i ) ) ) {
		return qfalse;
	}

	return qtrue;
}

/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.predictedPlayerState.pm_type == PM_SPECTATOR )
		return;

	//OSP: pause
	if ( cg.snap->ps.pm_type == PM_FREEZE )
		return;

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i=0; i<WP_NUM_WEAPONS; i++ )
	{
		cg.weaponSelect++;
		if ( cg.weaponSelect == WP_NUM_WEAPONS )
			cg.weaponSelect = 0;
		if ( CG_WeaponSelectable( cg.weaponSelect ) )
			break;
	}
	if ( i == WP_NUM_WEAPONS )
		cg.weaponSelect = original;
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.predictedPlayerState.pm_type == PM_SPECTATOR )
		return;

	//OSP: pause
	if ( cg.snap->ps.pm_type == PM_FREEZE )
		return;

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i=0; i<WP_NUM_WEAPONS; i++ )
	{
		cg.weaponSelect--;
		if ( cg.weaponSelect == -1 )
			cg.weaponSelect = WP_NUM_WEAPONS - 1;
		if ( CG_WeaponSelectable( cg.weaponSelect ) )
			break;
	}
	if ( i == WP_NUM_WEAPONS )
		cg.weaponSelect = original;
}

/*
===============
CG_Weapon_f
===============
*/
weapon_t CG_WeaponForString( const char *weaponName )
{
	weapon_t wp = WP_NONE;
	for ( wp=WP_NONE; wp<WP_NUM_WEAPONS; wp++ )
	{
		if ( !Q_stricmp( weaponNames[wp].technicalName, weaponName ) || !Q_stricmp( weaponNames[wp].longName, weaponName ) )
			return wp;
	}
	return WP_NONE;
}

void CG_Weapon_f( void ) {
	int		num;
	char	args[128] = {0};

	if ( !cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.predictedPlayerState.pm_type == PM_SPECTATOR )
		return;

	//OSP: pause
	if ( cg.snap->ps.pm_type == PM_FREEZE )
		return;

	trap_Args( args, sizeof( args ) );
	num = CG_WeaponForString( args );
	if ( num == WP_NONE )
		num = atoi( args );

	if ( num <= WP_NONE || num >= WP_NUM_WEAPONS || !CG_WeaponSelectable( num ) )
		return;

	cg.weaponSelectTime = cg.time;

	// don't have the weapon
	if ( !(cg.snap->ps.stats[STAT_WEAPONS] & (1<<num)) )
		return;

	cg.weaponSelect = num;
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( void ) {
	int		i;

	cg.weaponSelectTime = cg.time;

	for ( i = MAX_WEAPONS-1 ; i > 0 ; i-- ) {
		if ( CG_WeaponSelectable( i ) ) {
			cg.weaponSelect = i;
			break;
		}
	}
}



/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

//QtZ: equivalent to g_weapon
void CalcMuzzlePoint( centity_t *cent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) 
{
	int weapontype;
	vec3_t muzzleOffPoint;

	weapontype = cent->currentState.weapon;
	VectorCopy( cent->lerpOrigin, muzzlePoint );

	VectorCopy(weaponData[weapontype].muzzle, muzzleOffPoint);

	if (weapontype > WP_NONE && weapontype < WP_NUM_WEAPONS)
	{// Use the table to generate the muzzlepoint;
		{// Crouching.  Use the add-to-Z method to adjust vertically.
			VectorMA(muzzlePoint, muzzleOffPoint[0], forward, muzzlePoint);
			VectorMA(muzzlePoint, muzzleOffPoint[1], right, muzzlePoint);
			muzzlePoint[2] += 36 + muzzleOffPoint[2]; // RAZFIXME: crouching?
		}
	}
}

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent, int special ) {
	entityState_t *ent;
	int				c;
	weaponInfo_t	*weap;

	ent = &cent->currentState;
	if ( ent->weapon == WP_NONE ) {
		return;
	}
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		CG_Error( "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ ent->weapon ];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;
	//QtZ
	//cent->currentState.constantLight = special; //Raz: WTF?

	if ( weap->fireFunc )
	{
		vec3_t muzzle, start, end, mins={-0.5}, maxs={0.5};
		vec3_t fwd,up,right;
		trace_t tr;

		AngleVectors( cent->lerpAngles, fwd, right, up );
		CalcMuzzlePoint( cent, fwd, right, up, muzzle );
		VectorCopy( cent->lerpOrigin, start );
		if ( ent->number == cg.clientNum )
			start[2] += cg.predictedPlayerState.viewheight; //FIXME: crouching for other clients
		VectorMA( start, 8192, fwd, end );
		CG_Trace( &tr, start, mins, maxs, end, ent->number, MASK_SHOT );

		if (ent->number == cg.snap->ps.clientNum && !cg.renderingThirdPerson && (cg.lastFPFlashPoint[0] ||cg.lastFPFlashPoint[1] || cg.lastFPFlashPoint[2]) )
			VectorCopy(cg.lastFPFlashPoint, muzzle);

		weap->fireFunc( cent, muzzle, tr.endpos );
	}

	// play quad sound if needed
	if ( cent->currentState.powerups & ( 1 << PW_QUAD ) ) {
		trap_S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound );
	}

	// play a sound
	for ( c = 0 ; c < 4 ; c++ ) {
		if ( !weap->flashSound[c] ) {
			break;
		}
	}
	if ( c > 0 ) {
		c = rand() % c;
		if ( weap->flashSound[c] )
		{
			trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound[c] );
		}
	}

	// do brass ejection
	if ( weap->ejectBrassFunc )
		weap->ejectBrassFunc( cent );
}


/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall( int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType ) {
	qhandle_t		mod;
	qhandle_t		mark;
	qhandle_t		shader;
	sfxHandle_t		sfx;
	float			radius;
	float			light;
	vec3_t			lightColor;
	localEntity_t	*le;
	qboolean		alphaFade;
	qboolean		isSprite;
	int				duration;
	weaponInfo_t *wi = &cg_weapons[weapon];

	mark = 0;
	radius = 32;
	sfx = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	if ( wi->missileHitWallFunc )
	{
		wi->missileHitWallFunc( &cg_entities[clientNum], wi, origin, dir );
		return;
	}

	if ( sfx ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, dir, 
							   mod,	shader,
							   duration, isSprite );
		le->light = light;
		VectorCopy( lightColor, le->lightColor );
	}

	//
	// impact mark
	//
	alphaFade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
	CG_ImpactMark( mark, origin, dir, random()*360, 1,1,1,1, alphaFade, radius, qfalse );
}


/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer( int weapon, vec3_t origin, vec3_t dir, int entityNum ) {
	weaponInfo_t *wi = &cg_weapons[weapon];

	if ( wi->missileHitPlayerFunc )
		wi->missileHitPlayerFunc( &cg_entities[entityNum], wi, origin, dir );

	CG_Bleed( origin, entityNum );
}

/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean	CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t		forward;
	centity_t	*cent;
	int			anim;

	if ( entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );
		VectorMA( muzzle, 14, forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;

}
