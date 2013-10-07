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

// The server says this item is used on this level
void CG_RegisterWeapon( int weaponNum ) {
	weaponInfo_t	*weaponInfo;
	const gitem_t	*item, *ammo;
	char			path[MAX_QPATH];
	vector3			mins, maxs;
	int				i;

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == WP_NONE )
		return;

	if ( weaponInfo->registered )
		return;

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item=bg_itemlist, i=0; i<bg_numItems; item++, i++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	if ( !item->classname )
		trap->Error( ERR_DROP, "Couldn't find weapon %i", weaponNum );

	CG_RegisterItemVisuals( item - bg_itemlist );

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap->R_RegisterModel( item->world_model[0] );

	// calc midpoint for rotation
	trap->R_ModelBounds( weaponInfo->weaponModel, &mins, &maxs );
	for ( i=0; i<3; i++ )
		weaponInfo->weaponMidpoint.data[i] = mins.data[i] + 0.5f * (maxs.data[i] - mins.data[i]);

	weaponInfo->weaponIcon = trap->R_RegisterShader( item->icon );
	weaponInfo->ammoIcon = trap->R_RegisterShader( item->icon );

	for ( ammo=bg_itemlist, i=0; i<bg_numItems-1; ammo++, i++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum ) {
			break;
		}
	}
	if ( ammo->classname && ammo->world_model[0] ) {
		weaponInfo->ammoModel = trap->R_RegisterModel( ammo->world_model[0] );
	}

	strcpy( path, item->world_model[0] );
	COM_StripExtension(path, path, sizeof(path));
	strcat( path, "_barrel" );
	weaponInfo->barrelModel = trap->R_RegisterModel( path );

	strcpy( path, item->world_model[0] );
	COM_StripExtension(path, path, sizeof(path));
	strcat( path, "_hand" );
	weaponInfo->handsModel = trap->R_RegisterModel( path );

	if ( !weaponInfo->handsModel )
		weaponInfo->handsModel = trap->R_RegisterModel( "models/weapons/temp/temp.md3" );

	switch ( weaponNum ) {
	//RAZMARK: ADDING NEW WEAPONS
	case WP_QUANTIZER:
		weaponInfo->missileTrailFunc					= FX_Quantizer_Missile;
//		cgs.effects.weapons.quantizerShotEffect			= trap->FX_RegisterEffect( "quantizer/shot" );

		weaponInfo->missileHitWallFunc					= FX_Quantizer_HitWall;
//		cgs.effects.weapons.quantizerHitWallEffect		= trap->FX_RegisterEffect( "quantizer/hitwall" );

		weaponInfo->missileHitPlayerFunc				= FX_Quantizer_HitPlayer;
//		cgs.effects.weapons.quantizerHitPlayerEffect	= trap->FX_RegisterEffect( "quantizer/hitplayer" );

		weaponInfo->flashSound[0]						= trap->S_RegisterSound( "sound/weapons/quantizer/fire", qfalse );
//		weaponInfo->muzzleEffect						= trap->FX_RegisterEffect( "quantizer/muzzle" );
		break;

	case WP_REPEATER:
		weaponInfo->missileTrailFunc					= FX_Repeater_Missile;
		weaponInfo->missileHitPlayerFunc				= FX_Repeater_HitPlayer;
		weaponInfo->missileHitWallFunc					= FX_Repeater_HitWall;

		weaponInfo->flashSound[0]						= trap->S_RegisterSound( "sound/weapons/repeater/fire", qfalse );
		break;

	case WP_SPLICER:
		// these are handled manually
		weaponInfo->fireFunc							= NULL;
		weaponInfo->missileHitPlayerFunc				= NULL;
		weaponInfo->missileHitWallFunc					= NULL;

		weaponInfo->flashSound[0]						= trap->S_RegisterSound( "sound/weapons/splicer/fire", qfalse );

		cgs.media.weapons.splicerCore					= trap->R_RegisterShader( "splicerCore" ); // weapons.shader
		cgs.media.weapons.splicerFlare					= trap->R_RegisterShader( "splicerFlare" ); // weapons.shader
		break;

	case WP_MORTAR:
		weaponInfo->missileTrailFunc					= FX_Mortar_Missile;
		weaponInfo->missileHitPlayerFunc				= FX_Mortar_HitPlayer;
		weaponInfo->missileHitWallFunc					= FX_Mortar_HitWall;

		weaponInfo->flashSound[0]						= trap->S_RegisterSound( "sound/weapons/mortar/fire", qfalse );
		cgs.media.weapons.mortarProjectile				= trap->R_RegisterModel( "models/weapons/mortar/projectile.md3" );
		weaponInfo->firingSound							= NULL_SOUND;
		weaponInfo->missileModel						= NULL_HANDLE;
		weaponInfo->missileSound						= NULL_SOUND;
		break;

	case WP_DIVERGENCE:
		weaponInfo->fireFunc							= FX_Divergence_Fire;

		weaponInfo->readySound							= trap->S_RegisterSound( "sound/weapons/divergence/idle", qfalse );
		weaponInfo->flashSound[0]						= trap->S_RegisterSound( "sound/weapons/divergence/fire", qfalse );
		cgs.media.weapons.divergenceCore				= trap->R_RegisterShader( "divergenceCore" ); // weapons.shader
		break;

	 default:
		VectorSet( &weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound[0] = trap->S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav", qfalse );
		break;
	}
}

// The server says this item is used on this level
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	const gitem_t			*item;

	if ( itemNum < 0 || itemNum >= bg_numItems ) {
		trap->Error( ERR_DROP, "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems-1 );
	}

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( *itemInfo ) );
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap->R_RegisterModel( item->world_model[0] );

	itemInfo->icon = trap->R_RegisterShader( item->icon );

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH || 
		item->giType == IT_ARMOR || item->giType == IT_HOLDABLE ) {
		if ( item->world_model[1] ) {
			itemInfo->models[1] = trap->R_RegisterModel( item->world_model[1] );
		}
	}
}


/*

	VIEW WEAPON

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

static void CG_CalculateWeaponPosition( vector3 *origin, vector3 *angles ) {
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( &cg.refdef.vieworg, origin );
	VectorCopy( &cg.refdefViewAngles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 )	scale = -cg.xyspeed;
	else					scale =  cg.xyspeed;

	if ( cg_gunBobEnable->integer )
	{// gun angles from bobbing
		angles->pitch	+= scale * cg.bobfracsin * cg.gunBob.pitch;
		angles->yaw		+= scale * cg.bobfracsin * cg.gunBob.yaw;
		angles->roll	+= scale * cg.bobfracsin * cg.gunBob.roll;

		// drop the weapon when landing
		delta = cg.time - cg.landTime;
			 if ( delta < LAND_DEFLECT_TIME )						origin->z += cg.landChange*0.25f * delta / LAND_DEFLECT_TIME;
		else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME )	origin->z += cg.landChange*0.25f * (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;

		// drop the weapon when stair climbing
		delta = cg.time - cg.stepTime;
			 if ( delta < STEP_TIME/2 )		origin->z -= cg.stepChange*0.25f * delta / (STEP_TIME/2);
		else if ( delta < STEP_TIME )		origin->z -= cg.stepChange*0.25f * (STEP_TIME - delta) / (STEP_TIME/2);
	}

	if ( cg_gunIdleDriftEnable->integer )
	{// idle drift
		scale = cg.xyspeed + 40;
		fracsin = sinf( cg.time * cg.gunIdleDrift.speed );
		angles->pitch	+= scale * fracsin * cg.gunIdleDrift.amount.pitch;
		angles->yaw		+= scale * fracsin * cg.gunIdleDrift.amount.yaw;
		angles->roll	+= scale * fracsin * cg.gunIdleDrift.amount.roll;
	}
}

#define		SPIN_SPEED	0.9f
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

		speed = 0.5f * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
	}

	return angle;
}

static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) {
	// add powerup effects
	trap->R_AddRefEntityToScene( gun );

	if ( powerups & ( 1 << PW_QUAD ) ) {
		gun->customShader = cgs.media.quadWeaponShader;
		trap->R_AddRefEntityToScene( gun );
	}
}

// Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
//	The main player will have this called for BOTH cases, so effects like light and sound should only be done on the world model case.
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team ) {
	refEntity_t gun, barrel;
	vector3 angles;
	weapon_t weaponNum;
	weaponInfo_t *weapon = NULL;
	centity_t *nonPredictedCent = NULL;
	orientation_t lerped;

	weaponNum = (weapon_t)cent->currentState.weapon;

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( &parent->lightingOrigin, &gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	if( weaponNum == WP_DIVERGENCE ) {
		float cooldown = MIN( (float)cg.predictedPlayerState.weaponCooldown[cg.predictedPlayerState.weapon], (float)weaponData[cg.predictedPlayerState.weapon].maxFireTime );
		float cdPoint =  cooldown / (float)weaponData[cg.predictedPlayerState.weapon].maxFireTime;
		float blueHue = 200.0f, redHue = 10.0f;
		vector3 rgb = {0.0f};
		float hue = blueHue + cdPoint*(redHue - blueHue);
		HSL2RGB( hue/360.0f, 1.0f, 0.5f, &rgb.r, &rgb.g, &rgb.b );
		gun.shaderRGBA[0] = (byte)(rgb.r*255);
		gun.shaderRGBA[1] = (byte)(rgb.g*255);
		gun.shaderRGBA[2] = (byte)(rgb.b*255);
		gun.shaderRGBA[3] = 255;
	}

	gun.hModel = weapon->weaponModel;
	if ( !gun.hModel )
		return;

	if ( !ps ) {
		// add weapon ready sound
		if ( (cent->currentState.eFlags & EF_FIRING) && weapon->firingSound )
			trap->S_AddLoopingSound( cent->currentState.number, &cent->lerpOrigin, &vec3_origin, weapon->firingSound );
		else if ( weapon->readySound )
			trap->S_AddLoopingSound( cent->currentState.number, &cent->lerpOrigin, &vec3_origin, weapon->readySound );
	}

	trap->R_LerpTag(&lerped, parent->hModel, parent->oldframe, parent->frame, 1.0f - parent->backlerp, "tag_weapon");
	VectorCopy(&parent->origin, &gun.origin);

	VectorMA(&gun.origin, lerped.origin.x, &parent->axis[0], &gun.origin);
	VectorMA(&gun.origin, lerped.origin.y, &parent->axis[1], &gun.origin);
	VectorMA(&gun.origin, lerped.origin.z, &parent->axis[2], &gun.origin);

	MatrixMultiply(lerped.axis, parent->axis, gun.axis);
	gun.backlerp = parent->backlerp;

	CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups );

	// add the spinning barrel
	if ( weapon->barrelModel ) {
		memset( &barrel, 0, sizeof( barrel ) );
		VectorCopy( &parent->lightingOrigin, &barrel.lightingOrigin );
		barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = parent->renderfx;

		barrel.hModel = weapon->barrelModel;
		angles.yaw = 0;
		angles.pitch = 0;
		angles.roll = CG_MachinegunSpinAngle( cent );
		AnglesToAxis( &angles, barrel.axis );

		CG_PositionRotatedEntityOnTag( &barrel, &gun, weapon->weaponModel, "tag_barrel" );

		CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if ( (nonPredictedCent - cg_entities) != cent->currentState.clientNum )
		nonPredictedCent = cent;

	memset( &barrel, 0, sizeof( barrel ) );
//	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );

	CG_PositionEntityOnTag( &barrel, &gun, gun.hModel, "tag_flash");
	//QtZ: Added from JA
	VectorCopy( &barrel.origin, &cg.lastFPFlashPoint );

	if ( ps || cent->currentState.number != cg.predictedPlayerState.clientNum ) {
		if ( nonPredictedCent->currentState.weapon == WP_SPLICER && (nonPredictedCent->currentState.eFlags & EF_FIRING) )
			FX_Splicer_Beam( nonPredictedCent, &barrel.origin, NULL );
	}
}

extern float zoomFov;

// Add the weapon, and flash for the player's view
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t		hand;
	centity_t		*cent;
	clientInfo_t	*ci;
	vector3			angles;
	weaponInfo_t	*weapon;
	float cgFov = cg_fovViewmodel->integer ? cg_fovViewmodel->value : cg_fov->value;

	if (cgFov < 1)
		cgFov = 1;
	if (cgFov > 180)
		cgFov = 180;

	if ( ps->persistent[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;
	}

	// allow the gun to be completely removed
	if ( !cg_drawGun->integer ) {
		vector3 origin;

		if ( cg.predictedPlayerState.weapon == WP_SPLICER && (cg.predictedPlayerState.eFlags & EF_FIRING) ) {
			VectorCopy( &cg.refdef.vieworg, &origin );
			VectorMA( &origin, -8.0f, &cg.refdef.viewaxis[2], &origin );
			FX_Splicer_Beam( &cg_entities[ps->clientNum], &origin, NULL );
		}

		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun )
		return;

	cent = &cg.predictedPlayerEntity;	// &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon( ps->weapon );
	weapon = &cg_weapons[ ps->weapon ];

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition( &hand.origin, &angles );

	{
		vector3 gunLerp = { 0.0f };
		float point = 1.0f + (zoomFov-cg_zoomFov->value)/(cg_fov->value-cg_zoomFov->value) * -1.0f;

		//lerp from base position to zoom position by how far we've zoomed
		VectorLerp( &cg.gunAlign.basePos, point, &cg.gunAlign.zoomPos, &gunLerp );

		//now offset to hand position
		VectorMA( &hand.origin, gunLerp.x, &cg.refdef.viewaxis[0], &hand.origin );
		VectorMA( &hand.origin, gunLerp.y, &cg.refdef.viewaxis[1], &hand.origin );
		VectorMA( &hand.origin, gunLerp.z, &cg.refdef.viewaxis[2], &hand.origin );
	}

	AnglesToAxis( &angles, hand.axis );

	if ( cg_fovViewmodel->integer ) {
		float fracDistFOV = tanf( cg.refdef.fov_x * ( M_PI/180 ) * 0.5f );
		float fracWeapFOV = ( 1.0f / fracDistFOV ) * tanf( cgFov * ( M_PI/180 ) * 0.5f );
		VectorScale( &hand.axis[0], fracWeapFOV, &hand.axis[0] );
	}

	// map torso animations to weapon animations
	// get clientinfo for animation map
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame );
	hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame );
	hand.backlerp = cent->pe.torso.backlerp;

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity, ps->persistent[PERS_TEAM] );
}

/*

	WEAPON SELECTION

*/

void CG_DrawWeaponSelect( void ) {
	int		i;
	int		bits;
	int		count;
	float	x, y, w;
	char	*name;
	vector4	*color;

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );
	if ( !color ) {
		return;
	}
	trap->R_SetColor( color );

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
		CG_DrawPic( (float)x, (float)y, 32, 32, cg_weapons[i].weaponIcon );

		// draw selection marker
		if ( i == cg.weaponSelect ) {
			CG_DrawPic( (float)(x-4), (float)(y-4), 40, 40, cgs.media.selectShader );
		}

		// no ammo cross on top
		if ( !cg.snap->ps.ammo[ i ] ) {
			CG_DrawPic( (float)x, (float)y, 32, 32, cgs.media.noammoShader );
		}

		x += 40;
	}

	// draw the selected name
	if ( cg_weapons[ cg.weaponSelect ].item ) {
		name = cg_weapons[ cg.weaponSelect ].item->pickup_name;
		if ( name ) {
			w = CG_Text_Width( name, ui_smallFont->value, 0 );
			x = ( (int)SCREEN_WIDTH - w ) / 2;
			CG_Text_Paint( x, y - 22, ui_smallFont->value, color, name, 0.0f, 0, 0 );
		}
	}

	trap->R_SetColor( NULL );
}

static qboolean CG_WeaponSelectable( int i ) {
	if ( !cg.snap->ps.ammo[i] ) {
		return qfalse;
	}
	if ( ! (cg.snap->ps.stats[ STAT_WEAPONS ] & ( 1 << i ) ) ) {
		return qfalse;
	}

	return qtrue;
}

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

	trap->Cmd_Args( args, sizeof( args ) );
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

// The current weapon has just run out of ammo
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

	WEAPON EVENTS

*/

//QtZ: equivalent to g_weapon
void CalcMuzzlePoint( centity_t *cent, vector3 *forward, vector3 *right, vector3 *up, vector3 *muzzlePoint ) 
{
	int weapontype;
	vector3 muzzleOffPoint;

	weapontype = cent->currentState.weapon;
	VectorCopy( &cent->lerpOrigin, muzzlePoint );

	VectorCopy(&weaponData[weapontype].muzzle, &muzzleOffPoint);

	if (weapontype > WP_NONE && weapontype < WP_NUM_WEAPONS)
	{// Use the table to generate the muzzlepoint;
		{// Crouching.  Use the add-to-Z method to adjust vertically.
			VectorMA(muzzlePoint, muzzleOffPoint.x, forward, muzzlePoint);
			VectorMA(muzzlePoint, muzzleOffPoint.y, right, muzzlePoint);
			muzzlePoint->z += DEFAULT_VIEWHEIGHT + muzzleOffPoint.z; // RAZFIXME: crouching?
		}
	}
}

// Caused by an EV_FIRE_WEAPON event
void CG_FireWeapon( centity_t *cent, int special ) {
	entityState_t *ent;
	int c;
	weaponInfo_t *weap;

	ent = &cent->currentState;
	if ( ent->weapon == WP_NONE )
		return;

	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		trap->Error( ERR_DROP, "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
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
		vector3 muzzle, start, end, mins={-0.5}, maxs={0.5};
		vector3 fwd,up,right;
		trace_t tr;

		AngleVectors( &cent->lerpAngles, &fwd, &right, &up );
		CalcMuzzlePoint( cent, &fwd, &right, &up, &muzzle );
		VectorCopy( &cent->lerpOrigin, &start );
		if ( ent->number == cg.clientNum )
			start.z += cg.predictedPlayerState.viewheight; //FIXME: crouching for other clients
		VectorMA( &start, 8192, &fwd, &end );
		CG_Trace( &tr, &start, &mins, &maxs, &end, ent->number, MASK_SHOT );

		if (ent->number == cg.snap->ps.clientNum && cg_drawGun->integer && (cg.lastFPFlashPoint.x ||cg.lastFPFlashPoint.y || cg.lastFPFlashPoint.z) )
			VectorCopy( &cg.lastFPFlashPoint, &muzzle );

		weap->fireFunc( cent, &muzzle, &tr.endpos );
	}

	// play quad sound if needed
	if ( cent->currentState.powerups & ( 1 << PW_QUAD ) )
		trap->S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound );

	// play a sound
	for ( c = 0 ; c < 4 ; c++ ) {
		if ( !weap->flashSound[c] )
			break;
	}
	if ( c > 0 ) {
		c = rand() % c;
		if ( weap->flashSound[c] )
			trap->S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound[c] );
	}

	// do brass ejection
	if ( weap->ejectBrassFunc )
		weap->ejectBrassFunc( cent );
}

// Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
void CG_MissileHitWall( int weapon, int clientNum, vector3 *origin, vector3 *dir, impactSound_t soundType ) {
	qhandle_t		mod;
	qhandle_t		mark;
	qhandle_t		shader;
	sfxHandle_t		sfx;
	float			radius;
	float			light;
	vector3			lightColor;
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
	lightColor.r = 1;
	lightColor.g = 1;
	lightColor.b = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	if ( wi->missileHitWallFunc )
	{
		wi->missileHitWallFunc( &cg_entities[clientNum], wi, origin, dir );
		return;
	}

	if ( sfx ) {
		trap->S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, dir, 
							   mod,	shader,
							   duration, isSprite );
		le->light = light;
		VectorCopy( &lightColor, &le->lightColor );
	}

	//
	// impact mark
	//
	alphaFade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
	CG_ImpactMark( mark, origin, dir, random()*360, 1,1,1,1, alphaFade, radius, qfalse );
}

void CG_MissileHitPlayer( int weapon, vector3 *origin, vector3 *dir, int entityNum ) {
	weaponInfo_t *wi = &cg_weapons[weapon];

	if ( wi->missileHitPlayerFunc )
		wi->missileHitPlayerFunc( &cg_entities[entityNum], wi, origin, dir );

	CG_Bleed( origin, entityNum );
}

static qboolean	CG_CalcMuzzlePoint( int entityNum, vector3 *muzzle ) {
	vector3		forward;
	centity_t	*cent;
	int			anim;

	if ( entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( &cg.snap->ps.origin, muzzle );
		muzzle->z += cg.snap->ps.viewheight;
		AngleVectors( &cg.snap->ps.viewangles, &forward, NULL, NULL );
		VectorMA( muzzle, 14, &forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}

	VectorCopy( &cent->currentState.pos.trBase, muzzle );

	AngleVectors( &cent->currentState.apos.trBase, &forward, NULL, NULL );
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		muzzle->z += CROUCH_VIEWHEIGHT;
	} else {
		muzzle->z += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, &forward, muzzle );

	return qtrue;

}
