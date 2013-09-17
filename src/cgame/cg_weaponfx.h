#pragma once
#include "cg_local.h"

void FX_Quantizer_Missile	( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_Quantizer_HitWall	( centity_t *cent, const struct weaponInfo_s *weapon, vector3 *origin, vector3 *dir );
void FX_Quantizer_HitPlayer	( centity_t *cent, const struct weaponInfo_s *weapon, vector3 *origin, vector3 *dir );

void FX_Repeater_Missile	( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_Repeater_HitWall	( centity_t *cent, const struct weaponInfo_s *weapon, vector3 *origin, vector3 *dir );
void FX_Repeater_HitPlayer	( centity_t *cent, const struct weaponInfo_s *weapon, vector3 *origin, vector3 *dir );

void FX_Splicer_Beam		( centity_t *cent, vector3 *start, vector3 *end );

void FX_Mortar_Missile		( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_Mortar_HitWall		( centity_t *cent, const struct weaponInfo_s *weapon, vector3 *origin, vector3 *dir );
void FX_Mortar_HitPlayer	( centity_t *cent, const struct weaponInfo_s *weapon, vector3 *origin, vector3 *dir );

void FX_Divergence_Fire		( centity_t *cent, vector3 *start, vector3 *end );
