#include "cg_local.h"
#include "cg_weaponfx.h"

//================================
//	QUANTIZER
//================================

void FX_Quantizer_Missile( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward = { 0.0f };

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
		forward[2] = 1.0f;

//	trap_FX_PlayEffectID( cgs.effects.weapons.quantizerShotEffect, cent->lerpOrigin, forward, -1, -1 );
	trap_R_AddLightToScene( cent->lerpOrigin, 200, 0.13f, 0.13f, 0.87f );
}

void FX_Quantizer_HitWall( centity_t *cent, const struct weaponInfo_s *weapon, vec3_t origin, vec3_t dir )
{
//	trap_FX_PlayEffectID( cgs.effects.weapons.quantizerHitWallEffect, origin, dir, -1, -1 );
}

void FX_Quantizer_HitPlayer( centity_t *cent, const struct weaponInfo_s *weapon, vec3_t origin, vec3_t dir )
{
//	trap_FX_PlayEffectID( cgs.effects.weapons.quantizerHitPlayerEffect, origin, dir, -1, -1 );
}


//================================
//	REPEATER
//================================

void FX_Repeater_Missile(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward = { 0.0f };

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
		forward[2] = 1.0f;

//	trap_FX_PlayEffectID( cgs.effects.weapons.repeaterShotEffect, cent->lerpOrigin, forward, -1, -1 );
	trap_R_AddLightToScene( cent->lerpOrigin, 200, 0.87f, 0.13f, 0.13f );
}

void FX_Repeater_HitWall(  centity_t *cent, const struct weaponInfo_s *weapon, vec3_t origin, vec3_t dir )
{
//	trap_FX_PlayEffectID( cgs.effects.weapons.repeaterHitWallEffect, origin, dir, -1, -1 );
}

void FX_Repeater_HitPlayer( centity_t *cent, const struct weaponInfo_s *weapon, vec3_t origin, vec3_t dir )
{
//	trap_FX_PlayEffectID( cgs.effects.weapons.repeaterHitPlayerEffect, origin, dir, -1, -1 );
}


//================================
//	MORTAR
//================================

void FX_Mortar_Missile( centity_t *cent, const struct weaponInfo_s *weapon )
{
	refEntity_t ent;
	vec3_t ang;
	float vLen;
	float scale = 1.0f;
	refdef_t *refdef = &cg.refdef;

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( cent->lerpOrigin, ent.origin );

	VectorSubtract( ent.origin, refdef->vieworg, ent.axis[0] );
	vLen = VectorLength( ent.axis[0] );
	if ( VectorNormalize( ent.axis[0] ) <= 0.1f )
		return;

//	VectorCopy(refdef->viewaxis[2], ent.axis[2]);
//	CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);
	vectoangles( ent.axis[0], ang );
	ang[ROLL] = cent->miscTime;
	cent->miscTime += 12; //spin the half-sphere to give a "screwdriver" effect
	AnglesToAxis( ang, ent.axis );

	//radius must be a power of 2, and is the actual captured texture size
//	if (vLen < 128)			ent.radius = 256;
//	else if (vLen < 256)	ent.radius = 128;
//	else if (vLen < 512)	ent.radius = 64;
//	else					ent.radius = 32;
	scale = 0.37f;
	VectorScale( ent.axis[0],  scale, ent.axis[0] );
	VectorScale( ent.axis[1],  scale, ent.axis[1] );
	VectorScale( ent.axis[2], -scale, ent.axis[2] );


	ent.hModel = cgs.media.ringFlashModel; //RAZTODO: mortar projectile model

	VectorSet( ent.shaderRGBA, 128, 43, 255 );
	ent.shaderRGBA[3] = 48;
	ent.customShader = trap_R_RegisterShaderNoMip( "gfx/effects/shock_ripple" );
//	ent.renderfx = (RF_RGB_TINT|RF_FORCE_ENT_ALPHA);
	trap_R_AddRefEntityToScene( &ent );

	VectorSet( ent.shaderRGBA, 255, 255, 255 );
	ent.shaderRGBA[3] = 255;
	ent.customShader	= trap_R_RegisterShaderNoMip( "gfx/effects/caustic1" );
//	ent.renderfx		= (RF_RGB_TINT|RF_MINLIGHT);
	trap_R_AddRefEntityToScene( &ent );

	scale = (1.15 + Q_fabs( sinf( cg.time / 80.0f ) )*0.65f);
	VectorScale( ent.axis[0], scale, ent.axis[0] );
	VectorScale( ent.axis[1], scale, ent.axis[1] );
	VectorScale( ent.axis[2], scale, ent.axis[2] );
	VectorSet( ent.shaderRGBA, 51, 119, 255 );
	ent.customShader = trap_R_RegisterShaderNoMip( "gfx/effects/eplosion_wave" );
//	ent.renderfx = (RF_RGB_TINT|RF_ALPHA_DEPTH);
	trap_R_AddRefEntityToScene( &ent );

	trap_R_AddLightToScene( cent->lerpOrigin, 400, 0.13f, 0.43f, 0.87f );
}

void FX_Mortar_HitWall( centity_t *cent, const struct weaponInfo_s *weapon, vec3_t origin, vec3_t dir )
{
//	trap_FX_PlayEffectID( cgs.effects.weapons.mortarHitWallEffect, origin, dir, -1, -1 );
}

void FX_Mortar_HitPlayer( centity_t *cent, const struct weaponInfo_s *weapon, vec3_t origin, vec3_t dir )
{
//	trap_FX_PlayEffectID( cgs.effects.weapons.mortarHitPlayerEffect, origin, dir, -1, -1 );
}


//================================
//	DIVERGENCE
//================================

void FX_Divergence_Fire( centity_t *cent, vec3_t start, vec3_t end )
{
	vec3_t axis[36], move, move2, next_move, vec, temp, fwd;
	float  len;
	int    i, j, skip;

	localEntity_t	*leCore = CG_AllocLocalEntity(),	*leGlow = CG_AllocLocalEntity(),	*leMuzzle = CG_AllocLocalEntity();
	refEntity_t		*reCore = &leCore->refEntity,		*reGlow = &leGlow->refEntity,		*reMuzzle = &leMuzzle->refEntity;
	clientInfo_t	*ci = &cgs.clientinfo[cent->currentState.number];
	int				trailShader = trap_R_RegisterShaderNoMip( "divergenceCore" ),
					particleShader = trap_R_RegisterShaderNoMip( "gfx/effects/caustic1" ),
					muzzleShader = trap_R_RegisterShaderNoMip( "gfx/efx/wp/divergence/muzzle" )/*,
					muzzleEffect = cgs.effects.weapons.divergenceMuzzleNeutral*/;
	vec4_t color1 = { 1.0f }, color2 = { 1.0f };

	VectorCopy( ci->color1, color1 ); color1[3] = 1.0f;
	VectorCopy( ci->color2, color2 ); color2[3] = 1.0f;

	AngleVectors( cent->lerpAngles, fwd, NULL, NULL );

	if ( cgs.gametype >= GT_TEAM )
	{
		if ( ci->team == TEAM_RED )
		{
			VectorSet( color1, 1.0f, 0.0f, 0.0f );
			VectorSet( color2, 1.0f, 0.0f, 0.0f );
		//	muzzleEffect = cgs.effects.weapons.divergenceMuzzleRed;
		}
		else if ( ci->team == TEAM_BLUE )
		{
			VectorSet( color1, 0.0f, 0.0f, 1.0f );
			VectorSet( color2, 0.0f, 0.0f, 1.0f );
		//	muzzleEffect = cgs.effects.weapons.divergenceMuzzleBlue;
		}
		else
		{
			VectorSet( color1, 0.0f, 0.878431f, 1.0f );
			VectorSet( color2, 0.0f, 1.0f, 1.0f );
		}
	}
 
#define RADIUS   4
#define ROTATION 1
#define SPACING  5
 
	start[2] -= 4;
	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	PerpendicularVector(temp, vec);
	for (i = 0 ; i < 36; i++)
		RotatePointAroundVector(axis[i], vec, temp, i * 10);//banshee 2.4 was 10

//	CG_RailTrail( ci, start, end );

	//MUZZLE
	leMuzzle->leFlags = LEF_PUFF_DONT_SCALE;
	leMuzzle->leType = LE_FADE_RGB;//LE_SCALE_FADE;
	leMuzzle->startTime = cg.time;
	leMuzzle->endTime = cg.time + 1200;
	leMuzzle->lifeRate = 1.0 / (leMuzzle->endTime - leMuzzle->startTime);

	reMuzzle->shaderTime = cg.time / 1200.0f;
	reMuzzle->reType = RT_SPRITE;
	reMuzzle->radius = 5.8f;
	reMuzzle->customShader = muzzleShader;
//	reMuzzle->renderfx |= RF_RGB_TINT;

	reMuzzle->shaderRGBA[0] = color2[0]*255;	reMuzzle->shaderRGBA[1] = color2[1]*255;	reMuzzle->shaderRGBA[2] = color2[2]*255;	reMuzzle->shaderRGBA[3] = color2[3]*255;
	VectorSet( leMuzzle->color, color2[0]*0.75, color2[1]*0.75, color2[2]*0.75 );
	leMuzzle->color[3] = 1.0f;

	VectorCopy( start, reMuzzle->origin );
	VectorCopy( start, reMuzzle->oldorigin );
	leMuzzle->pos.trType = TR_STATIONARY;
	leMuzzle->pos.trTime = cg.time;

	//Glow
	leGlow->leType = LE_FADE_RGB;
	leGlow->startTime = cg.time;
	leGlow->endTime = cg.time + 1600;
	leGlow->lifeRate = 1.0 / (leGlow->endTime - leGlow->startTime);
	reGlow->shaderTime = cg.time / 1600.0f;
	reGlow->reType = RT_LINE;//RT_RAIL_CORE;
	reGlow->radius = 3.0f;
	reGlow->customShader = trailShader;
	VectorCopy(start, reGlow->origin);
	VectorCopy(end, reGlow->oldorigin);
		reGlow->shaderRGBA[0] = color1[0]*255;
		reGlow->shaderRGBA[1] = color1[1]*255;
		reGlow->shaderRGBA[2] = color1[2]*255;
		reGlow->shaderRGBA[3] = color1[3]*255;
	VectorSet( leGlow->color, color1[0]*0.75, color1[1]*0.75, color1[2]*0.75 );
	leGlow->color[3] = 1.0f;

	//Core
	leCore->leType = LE_FADE_RGB;
	leCore->startTime = cg.time;
	leCore->endTime = cg.time + 1600;
	leCore->lifeRate = 1.0 / (leCore->endTime - leCore->startTime);
	reCore->shaderTime = cg.time / 1600.0f;
	reCore->reType = RT_LINE;//RT_RAIL_CORE;
	reCore->radius = 1.0f;
	reCore->customShader = trailShader;
	VectorCopy(start, reCore->origin);
	VectorCopy(end, reCore->oldorigin);
		reCore->shaderRGBA[0] = 255;
		reCore->shaderRGBA[1] = 255;
		reCore->shaderRGBA[2] = 255;
		reCore->shaderRGBA[3] = 255;
	VectorSet( leCore->color, 1.0f, 1.0f, 1.0f );
	leCore->color[3] = 0.6f;

	AxisClear( reCore->axis );

	VectorMA(move, 20, vec, move);
	VectorCopy(move, next_move);
	VectorScale (vec, SPACING, vec);

	skip = -1;

	j = 18;
	for ( i=0; i<len; i+=SPACING )
	{
		if ( i != skip )
		{
			localEntity_t *le = CG_AllocLocalEntity();
			refEntity_t *re = &le->refEntity;

			skip = i + SPACING;
			le->leFlags = LEF_PUFF_DONT_SCALE;
			le->leType = LE_MOVE_SCALE_FADE;
			le->startTime = cg.time;
			le->endTime = cg.time + (i>>1) + 600;
			le->lifeRate = 1.0 / (le->endTime - le->startTime);

			re->shaderTime = cg.time / 2000.0f;
			re->reType = RT_SPRITE;
			re->radius = 1.8f;
			re->customShader = particleShader;
//			re->renderfx |= RF_RGB_TINT;

				re->shaderRGBA[0] = color2[0]*255;
				re->shaderRGBA[1] = color2[1]*255;
				re->shaderRGBA[2] = color2[2]*255;
				re->shaderRGBA[3] = color2[3]*255;

			VectorSet( le->color, color2[0]*0.75, color2[1]*0.75, color2[2]*0.75 );
			le->color[3] = 1.0f;

			le->pos.trType = TR_LINEAR;
			le->pos.trTime = cg.time;

			VectorCopy( move, move2);
			VectorMA(move2, RADIUS , axis[j], move2);
			VectorCopy(move2, le->pos.trBase);

			le->pos.trDelta[0] = axis[j][0]*6;
			le->pos.trDelta[1] = axis[j][1]*6;
			le->pos.trDelta[2] = axis[j][2]*6;
		}

		VectorAdd (move, vec, move);

		j = j + ROTATION < 36 ? j + ROTATION : (j + ROTATION) % 36;
	}

//	trap_FX_PlayEffectID( muzzleEffect, start, fwd, -1, -1 );
}
