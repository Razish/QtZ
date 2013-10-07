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
// g_combat.c

#include "g_local.h"

int G_GetHitLocation(gentity_t *target, vector3 *ppoint)
{
	vector3	point, point_dir;
	vector3	forward, right, up;
	vector3	tangles, tcenter;
	float	tradius;
	float	udot, fdot, rdot;
	int		Vertical, Forward, Lateral;
	int		HitLoc;

	// Get target forward, right and up.
	// Ignore player's pitch and roll.
	if ( target->client )
		VectorSet( &tangles, 0, target->r.currentAngles.yaw, 0 );

	AngleVectors( &tangles, &forward, &right, &up );

	// Get center of target.
	VectorAdd( &target->r.absmin, &target->r.absmax, &tcenter );
	VectorScale( &tcenter, 0.5f, &tcenter );

	// Get radius width of target.
	tradius = (	fabsf( target->r.maxs.x ) +
				fabsf( target->r.maxs.y ) +
				fabsf( target->r.mins.x ) +
				fabsf( target->r.mins.y ) ) / 4.0f;

	// Get impact point.
	if ( ppoint && !VectorCompare( ppoint, &vec3_origin ) )
		VectorCopy( ppoint, &point );
	else
		return HL_NONE;

/*
//get impact dir
	if(pdir && !VectorCompare(pdir, vec3_origin))
	{
		VectorCopy(pdir, dir);
	}
	else
	{
		return;
	}

//put point at controlled distance from center
	VectorSubtract(point, tcenter, tempvec);
	tempvec[2] = 0;
	hdist = VectorLength(tempvec);

	VectorMA(point, hdist - tradius, dir, point);
	//now a point on the surface of a cylinder with a radius of tradius
*/	
	VectorSubtract( &point, &tcenter, &point_dir );
	VectorNormalize( &point_dir );

	// Get bottom to top (vertical) position index
	udot = DotProduct( &up, &point_dir );
		 if ( udot >  0.800 )	Vertical = 4;
	else if ( udot >  0.400 )	Vertical = 3;
	else if ( udot > -0.333 )	Vertical = 2;
	else if ( udot > -0.666 )	Vertical = 1;
	else						Vertical = 0;

	// Get back to front (forward) position index.
	fdot = DotProduct( &forward, &point_dir );
		 if ( fdot >  0.666 )	Forward = 4;
	else if ( fdot >  0.333 )	Forward = 3;
	else if ( fdot > -0.333 )	Forward = 2;
	else if ( fdot > -0.666 )	Forward = 1;
	else						Forward = 0;

	// Get left to right (lateral) position index.
	rdot = DotProduct( &right, &point_dir );
		 if ( rdot >  0.666 )	Lateral = 4;
	else if ( rdot >  0.333 )	Lateral = 3;
	else if ( rdot > -0.333 )	Lateral = 2;
	else if ( rdot > -0.666 )	Lateral = 1;
	else						Lateral = 0;

	HitLoc = Vertical*25 + Forward*5 + Lateral;

	if ( HitLoc <= 10 )
	{//Feet
		if ( rdot > 0 )		return HL_FOOT_RT;
		else				return HL_FOOT_LT;
	}

	else if ( HitLoc <= 50 )
	{//Legs
		if ( rdot > 0 )		return HL_LEG_RT;
		else				return HL_LEG_LT;
	}

	else if ( HitLoc == 56 || HitLoc == 60 || HitLoc == 61 || HitLoc == 65 || HitLoc == 66 || HitLoc == 70 )
	{//Hands
		if ( rdot > 0 )		return HL_HAND_RT;
		else				return HL_HAND_LT;
	}

	else if ( HitLoc == 83 || HitLoc == 87 || HitLoc == 88 || HitLoc == 92 || HitLoc == 93 || HitLoc == 97 )
	{//Arms
		if ( rdot > 0 )		return HL_ARM_RT;
		else				return HL_ARM_LT;
	}

	else if ( (HitLoc >= 107 && HitLoc <= 109) || (HitLoc >= 112 && HitLoc <= 114) || (HitLoc >= 117 && HitLoc <= 119) )
	{//Head
		return HL_HEAD;
	}

	else
	{
		if ( udot < 0.3f )
			return HL_WAIST;

		else if ( fdot < 0.0f )
		{
				 if ( rdot >  0.4f )	return HL_BACK_RT;
			else if ( rdot < -0.4f )	return HL_BACK_LT;
			else if ( fdot <  0.0f )	return HL_BACK;
		}
		else
		{
				 if ( rdot >  0.3f )	return HL_CHEST_RT;
			else if ( rdot < -0.3f )	return HL_CHEST_LT;
			else if ( fdot <  0.0f)		return HL_CHEST;
		}
	}

	return HL_NONE;
}

char *hitLocName[HL_MAX] = {
	"none",					//HL_NONE = 0,
	"right foot",			//HL_FOOT_RT,
	"left foot",			//HL_FOOT_LT,
	"right leg",			//HL_LEG_RT,
	"left leg",				//HL_LEG_LT,
	"waist",				//HL_WAIST,
	"back right shoulder",	//HL_BACK_RT,
	"back left shoulder",	//HL_BACK_LT,
	"back",					//HL_BACK,
	"front right shouler",	//HL_CHEST_RT,
	"front left shoulder",	//HL_CHEST_LT,
	"chest",				//HL_CHEST,
	"right arm",			//HL_ARM_RT,
	"left arm",				//HL_ARM_LT,
	"right hand",			//HL_HAND_RT,
	"left hand",			//HL_HAND_LT,
	"head",					//HL_HEAD
	"generic1",				//HL_GENERIC1,
	"generic2",				//HL_GENERIC2,
	"generic3",				//HL_GENERIC3,
	"generic4",				//HL_GENERIC4,
	"generic5",				//HL_GENERIC5,
	"generic6"				//HL_GENERIC6
};

void ScorePlum( gentity_t *ent, vector3 *origin, int score ) {
	gentity_t *plum;

	plum = G_TempEntity( origin, EV_SCOREPLUM );
	// only send this temp entity to a single client
	plum->r.svFlags |= SVF_SINGLECLIENT;
	plum->r.singleClient = ent->s.number;
	//
	plum->s.otherEntityNum = ent->s.number;
	plum->s.time = score;
}

// Adds score to both the client and his team
void AddScore( gentity_t *ent, vector3 *origin, int score ) {
	if ( !ent->client ) {
		return;
	}
	// no scoring during pre-match warmup
	if ( level.warmupTime ) {
		return;
	}
	// show score plum
	ScorePlum(ent, origin, score);
	//
	ent->client->ps.persistant[PERS_SCORE] += score;
	if ( level.gametype == GT_TEAMBLOOD )
		level.teamScores[ ent->client->ps.persistant[PERS_TEAM] ] += score;
	CalculateRanks();
}

// Toss the weapon and powerups for the killed player
void TossClientItems( gentity_t *self ) {
	const gitem_t		*item;
	int			weapon;
	float		angle;
	int			i;
	gentity_t	*drop;

	// drop the weapon
	weapon = self->s.weapon;

	if ( !(self->client->ps.stats[STAT_WEAPONS] & (1 << weapon)) )
		weapon = WP_NONE;

	//RAZMARK: ADDING NEW WEAPONS
	if ( weapon > WP_QUANTIZER && self->client->ps.ammo[weapon] )
	{
		// find the item type for this weapon
		item = BG_FindItemForWeapon( (weapon_t)weapon );

		// spawn the item
		Drop_Item( self, item, 0 );
	}

	// drop all the powerups if not in teamplay
	if ( level.gametype != GT_TEAMBLOOD ) {
		angle = 45;
		for ( i = 1 ; i < PW_NUM_POWERUPS ; i++ ) {
			if ( self->client->ps.powerups[i] > level.time ) {
				item = BG_FindItemForPowerup( (powerup_t)i );
				if ( !item ) {
					continue;
				}
				drop = Drop_Item( self, item, angle );
				// decide how many seconds it has left
				drop->count = ( self->client->ps.powerups[ i ] - level.time ) / 1000;
				if ( drop->count < 1 ) {
					drop->count = 1;
				}
				angle += 45;
			}
		}
	}
}

void TossClientPersistantPowerups( gentity_t *ent ) {
	gentity_t	*powerup;

	if( !ent->client ) {
		return;
	}

	if( !ent->client->persistantPowerup ) {
		return;
	}

	powerup = ent->client->persistantPowerup;

	powerup->r.svFlags &= ~SVF_NOCLIENT;
	powerup->s.eFlags &= ~EF_NODRAW;
	powerup->r.contents = CONTENTS_TRIGGER;
	trap->SV_LinkEntity( (sharedEntity_t *)powerup );

	ent->client->ps.stats[STAT_PERSISTANT_POWERUP] = 0;
	ent->client->persistantPowerup = NULL;
}

void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vector3		dir;

	if ( attacker && attacker != self ) {
		VectorSubtract (&attacker->s.pos.trBase, &self->s.pos.trBase, &dir);
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract (&inflictor->s.pos.trBase, &self->s.pos.trBase, &dir);
	} else {
		self->client->ps.stats[STAT_DEAD_YAW] = (int)self->s.angles.yaw;
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = (int)vectoyaw( &dir );
}

void GibEntity( gentity_t *self, int killer ) {
	G_AddEvent( self, EV_GIB_PLAYER, killer );
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
}

void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	if ( self->health > GIB_HEALTH )
		return;

	GibEntity( self, 0 );
}


// these are just for logging, the client prints its own messages
//RAZMARK: Adding new weapons
char	*modNames[MOD_MAX] = {
	"MOD_UNKNOWN",

	"MOD_QUANTIZER",
	"MOD_REPEATER",
	"MOD_SPLICER",
	"MOD_MORTAR",
	"MOD_DIVERGENCE",

	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",
};

void CheckAlmostCapture( gentity_t *self, gentity_t *attacker ) {
	gentity_t	*ent;
	vector3		dir;
	char		*classname;

	//RAZTODO: plz2rewrite
	// if this player was carrying a flag
	if ( self->client->ps.powerups[PW_REDFLAG] ||
		self->client->ps.powerups[PW_BLUEFLAG] ||
		self->client->ps.powerups[PW_NEUTRALFLAG] ) {
		// get the goal flag this player should have been going for
		if ( level.gametype == GT_FLAGS ) {
			if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
				classname = "team_blueflag";
			}
			else {
				classname = "team_redflag";
			}
		}
		else {
			if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
				classname = "team_redflag";
			}
			else {
				classname = "team_blueflag";
			}
		}
		ent = NULL;
		do
		{
			ent = G_Find(ent, FOFS(classname), classname);
		} while (ent && (ent->flags & FL_DROPPED_ITEM));
		// if we found the destination flag and it's not picked up
		if (ent && !(ent->r.svFlags & SVF_NOCLIENT) ) {
			// if the player was *very* close
			VectorSubtract( &self->client->ps.origin, &ent->s.origin, &dir );
			if ( VectorLength(&dir) < 200 ) {
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				if ( attacker->client ) {
					attacker->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				}
			}
		}
	}
}

void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	gentity_t	*ent;
	gclient_t	*cl;
	int			i, anim, contents, killer;
	char		*killerName, *obit;

	if ( self->client->ps.pm_type == PM_DEAD )
		return;

	if ( level.intermissiontime )
		return;

	// check for an almost capture
	CheckAlmostCapture( self, attacker );

	if ((self->client->ps.eFlags & EF_TICKING) && self->activator) {
		self->client->ps.eFlags &= ~EF_TICKING;
		self->activator->think = G_FreeEntity;
		self->activator->nextthink = level.time;
	}
	self->client->ps.pm_type = PM_DEAD;

	if ( attacker ) {
		killer = attacker->s.number;
		if ( attacker->client ) {
			killerName = attacker->client->pers.netname;
		} else {
			killerName = "<non-client>";
		}
	} else {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( killer < 0 || killer >= MAX_CLIENTS ) {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( meansOfDeath < 0 || meansOfDeath >= ARRAY_LEN( modNames ) ) {
		obit = "<bad obituary>";
	} else {
		obit = modNames[meansOfDeath];
	}

	G_LogPrintf("Kill: %i %i %i: %s killed %s by %s\n", 
		killer, self->s.number, meansOfDeath, killerName, 
		self->client->pers.netname, obit );

	// broadcast the death event to everyone
	ent = G_TempEntity( &self->r.currentOrigin, EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
	ent->r.svFlags = SVF_BROADCAST;	// send to everyone

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;

	if (attacker && attacker->client) {
		attacker->client->lastkilled_client = self->s.number;

		if ( attacker == self || OnSameTeam (self, attacker ) ) {
			AddScore( attacker, &self->r.currentOrigin, -1 );
		} else {
			AddScore( attacker, &self->r.currentOrigin, 1 );

			// check for two kills in a short amount of time
			// if this is close enough to the last kill, give a reward sound
			if ( level.time - attacker->client->lastKillTime < CARNAGE_REWARD_TIME ) {
				// play excellent on player
				attacker->client->ps.persistant[PERS_EXCELLENT_COUNT]++;

				// add the sprite over the player's head
				attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				attacker->client->ps.eFlags |= EF_AWARD_EXCELLENT;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
			}
			attacker->client->lastKillTime = level.time;

		}
	} else {
		AddScore( self, &self->r.currentOrigin, -1 );
	}

	// Add team bonuses
	Team_FragBonuses(self, inflictor, attacker);

	// if I committed suicide, the flag does not fall, it returns.
	if (meansOfDeath == MOD_SUICIDE) {
		if ( self->client->ps.powerups[PW_NEUTRALFLAG] ) {		// only happens in One Flag CTF
			Team_ReturnFlag( TEAM_FREE );
			self->client->ps.powerups[PW_NEUTRALFLAG] = 0;
		}
		else if ( self->client->ps.powerups[PW_REDFLAG] ) {		// only happens in standard CTF
			Team_ReturnFlag( TEAM_RED );
			self->client->ps.powerups[PW_REDFLAG] = 0;
		}
		else if ( self->client->ps.powerups[PW_BLUEFLAG] ) {	// only happens in standard CTF
			Team_ReturnFlag( TEAM_BLUE );
			self->client->ps.powerups[PW_BLUEFLAG] = 0;
		}
	}

	TossClientItems( self );
	TossClientPersistantPowerups( self );

	self->client->scoresWaiting = qtrue;

	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i=0, cl=level.clients; i<level.maxclients; i++, cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam != TEAM_SPECTATOR )
			continue;

		if ( cl->sess.spectatorClient == self->s.number ) {
			cl->scoresWaiting = qtrue;
		}
	}

	self->takedamage = qtrue;	// can still be gibbed

	self->s.weapon = WP_NONE;
	self->s.powerups = 0;
	self->r.contents = CONTENTS_CORPSE;

	self->s.angles.pitch = 0;
	self->s.angles.roll = 0;
	LookAtKiller (self, inflictor, attacker);

	VectorCopy( &self->s.angles, &self->client->ps.viewangles );

	self->s.loopSound = 0;

	self->r.maxs.z = -8;

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->client->respawnTime = level.time + 1700;

	// remove powerups
	memset( self->client->ps.powerups, 0, sizeof(self->client->ps.powerups) );

	// never gib in a nodrop
	contents = trap->SV_PointContents( &self->r.currentOrigin, -1 );

	if ( (self->health <= GIB_HEALTH && !(contents & CONTENTS_NODROP)) || meansOfDeath == MOD_SUICIDE) {
		// gib death
		GibEntity( self, killer );
	} else {
		// normal death
		static int deathAnim;

		switch ( deathAnim ) {
		case 0:
			anim = BOTH_DEATH1;
			break;
		case 1:
			anim = BOTH_DEATH2;
			break;
		case 2:
		default:
			anim = BOTH_DEATH3;
			break;
		}

		// for the no-blood option, we need to prevent the health
		// from going to gib level
		if ( self->health <= GIB_HEALTH ) {
			self->health = GIB_HEALTH+1;
		}

		self->client->ps.legsAnim = 
			( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
		self->client->ps.torsoAnim = 
			( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

		G_AddEvent( self, EV_DEATH1 + deathAnim, killer );

		// the body can still be gibbed
		self->die = body_die;

		// globally cycle through the different death animations
		deathAnim = ( deathAnim + 1 ) % 3;
	}

	trap->SV_LinkEntity ((sharedEntity_t *)self);

}

int CheckArmor( gentity_t *ent, int damage, int dflags ) {
	gclient_t	*client;
	int			save, count;

	if ( !damage )
		return 0;

	client = ent->client;

	if ( !client || (dflags & DAMAGE_NO_ARMOR) )
		return 0;

	// armor
	count = client->ps.stats[STAT_ARMOR];
	save = (int)ceilf( damage * ARMOR_PROTECTION );
	if ( save >= count )
		save = count;

	if ( !save )
		return 0;

	client->ps.stats[STAT_ARMOR] -= save;

	return save;
}

int RaySphereIntersections( vector3 *origin, float radius, vector3 *point, vector3 *dir, vector3 intersections[2] ) {
	float b, c, d, t;

	//	| origin - (point + t * dir) | = radius
	//	a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	//	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	//	c = (point[0] - origin[0])^2 + (point[1] - origin[1])^2 + (point[2] - origin[2])^2 - radius^2;

	// normalize dir so a = 1
	VectorNormalize(dir);
	b = 2 * (dir->x * (point->x - origin->x) + dir->y * (point->y - origin->y) + dir->z * (point->z - origin->z));
	c = (point->x - origin->x) * (point->x - origin->x) +
		(point->y - origin->y) * (point->y - origin->y) +
		(point->z - origin->z) * (point->z - origin->z) -
		radius * radius;

	d = b * b - 4 * c;
	if (d > 0) {
		t = (- b + sqrtf(d)) / 2;
		VectorMA(point, t, dir, &intersections[0]);
		t = (- b - sqrtf(d)) / 2;
		VectorMA(point, t, dir, &intersections[1]);
		return 2;
	}
	else if (d == 0) {
		t = (- b ) / 2;
		VectorMA(point, t, dir, &intersections[0]);
		return 1;
	}
	return 0;
}

// NOTE: This function is recursive for partially reflected friendly fire. In that case; targ, inflictor, attacker == the traitor
//	I've also added an 'affector', which will be the missile entity for weapons so I can apply per-missile knockback multipliers
//	This is to aid the quantizer movement and rocket jumping tricks
//
//	targ		entity that is being damaged (player, object)
//	inflictor	entity that is causing the damage (rocket, or NULL for world)
//	attacker	entity that caused the inflictor to damage targ (player, or NULL for world)
//	affector	in the case of friendly fire, will be the inflictor from the previous call, and inflictor will be the traitor
//	dir			direction of the attack for knockback
//	point		point at which the damage is being inflicted, used for headshots (NULL for world)
//	damage		amount of damage being inflicted
//	knockback	force to be applied against targ as a result of the damage
//	dflags		these flags are used to control how G_Damage works
void G_Damage( gentity_t *real_targ, gentity_t *real_inflictor, gentity_t *real_attacker, gentity_t *real_affector, vector3 *real_dir, vector3 *real_point, int real_damage, int real_dflags, int real_mod ) {
	gentity_t	*targ=real_targ, *inflictor=real_inflictor, *attacker=real_attacker, *affector=real_affector;
	vector3		dir, point;
	int			take, asave, knockback;
	int			damage=real_damage, dflags=real_dflags, mod=real_mod;

	if ( real_dir )
		VectorCopy( real_dir, &dir );
	if ( real_point )
		VectorCopy( real_point, &point );

	if ( !targ->takedamage )
		return;

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued )
		return;

	if ( !inflictor )
		inflictor = &g_entities[ENTITYNUM_WORLD];
	if ( !attacker )
		attacker = &g_entities[ENTITYNUM_WORLD];

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER ) {
		if ( targ->use && targ->moverState == MOVER_POS1 )
			targ->use( targ, inflictor, attacker );
		return;
	}

	if ( targ->client && targ->client->noclip )
		return;

	//QtZ
	if ( !real_dir )
		VectorSet( &dir, flrand( 0.0f, 0.2f ), flrand( 0.0f, 0.2f ), 0.7f );
	else
		VectorNormalize( &dir );

	//Raz: Added for Quantizer
	if ( affector /*!damage && affector && !affector->splashDamage && affector->splashRadius*/ )
		knockback = affector->splashDamage;
	else
		knockback = damage;

	if ( knockback > 200 )
		knockback = 200;
	if ( targ->flags & FL_NO_KNOCKBACK )
		knockback = 0;
	if ( dflags & DAMAGE_NO_KNOCKBACK )
		knockback = 0;

	//Raz: Quantizer/RL extra knockback
	if ( affector ) {
		if ( attacker->client && attacker->s.eType == ET_PLAYER && (attacker==targ || OnSameTeam( attacker, targ )) )
			knockback = (int)(knockback * affector->knockbackMultiSelf);
		else
			knockback = (int)(knockback * affector->knockbackMulti);
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client ) {
		vector3	kvel;
		float	mass;

		mass = 200;

		if ( g_debugDamage->integer )
			Com_Printf( " \nDirectional knockback: %.2f, %.2f, %.2f\n", dir.x, dir.y, dir.z );

		VectorScale (&dir, g_knockback->value * (float)knockback / mass, &kvel);

		if ( g_debugDamage->integer )
			Com_Printf( "Scaled knockback: %.2f, %.2f, %.2f\n", kvel.x, kvel.y, kvel.z );

		if ( affector
			&& (affector->dflags & DAMAGE_VERTICAL_KNOCKBACK)
			&& affector->knockbackDampVert != 0.0f
			)//&& attacker != targ )
		{//Attacking someone else with a DAMAGE_VERTICAL_KNOCKBACK flag, dampen the x/y knockback
			kvel.x *= affector->knockbackDampVert;
			kvel.y *= affector->knockbackDampVert;
		}

		if ( g_debugDamage->integer )
			Com_Printf( "Final knockback: %.2f, %.2f, %.2f\n \n", kvel.x, kvel.y, kvel.z );

		VectorAdd (&targ->client->ps.velocity, &kvel, &targ->client->ps.velocity);

		// set the timer so that the other client can't cancel out the movement immediately
		if ( !targ->client->ps.pm_time ) {
			targ->client->ps.pm_time = Q_clampi( 50, knockback*2, 200 );
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	//QtZ: Moved this up here to bypass friendly fire checks
	//	add to the attacker's hit counter
	if ( attacker->client && targ->client && targ != attacker && targ->health > 0
			&& targ->s.eType != ET_MISSILE && targ->s.eType != ET_GENERAL) {
		if ( OnSameTeam( targ, attacker ) )
			attacker->client->ps.persistant[PERS_HITS]--;
		else
			attacker->client->ps.persistant[PERS_HITS]++;
		attacker->client->ps.persistant[PERS_ATTACKEE_ARMOR] = (targ->health<<8)|(targ->client->ps.stats[STAT_ARMOR]);
	}

	// check for completely getting out of the damage
	if ( !(dflags & DAMAGE_NO_PROTECTION) ) {

		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
		if ( targ != attacker && OnSameTeam (targ, attacker)  ) {
			if ( !g_friendlyFire->integer )
				return;
			else if ( g_friendlyFire->integer == 2 )
				G_Damage( real_attacker, real_attacker, real_attacker, real_affector, real_dir, real_point, (int)(real_damage*g_friendlyFireScale->value), real_dflags|DAMAGE_NO_KNOCKBACK, real_mod );
			else if ( g_friendlyFire->integer == 3 ) {
				G_Damage( real_attacker, real_attacker, real_attacker, real_affector, real_dir, real_point, (int)(real_damage*g_friendlyFireScale->value), real_dflags|DAMAGE_NO_KNOCKBACK, real_mod );
				return;
			}
		}

		// check for godmode
		if ( targ->flags & FL_GODMODE )
			return;
	}

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	//Raz: Only do this if it's not a recursive reflected friendly-fire call, in which case targ==inflictor
	if ( targ == attacker && targ != inflictor )
//	if ( targ == attacker)
		damage /= 2;

	take = damage;

	// save some from armor
	asave = CheckArmor (targ, take, dflags);
	take -= asave;

	if ( g_debugDamage->integer ) {
		char *tmp = va( "^3G_Damage(): ^7Hit ^5%2i ^7(^1%3i^7/^2%3i^7) for %3i (^1%3i^7/^2%3i^7)",
			targ->s.number,
			targ->health,
			targ->client ? targ->client->ps.stats[STAT_ARMOR] : 0,
			damage,
			take,
			asave );
		trap->SV_GameSendServerCommand( attacker-g_entities, va( "chat \"%s\"", tmp ) );
		trap->Print( tmp );
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( targ->client ) {
		targ->client->ps.persistant[PERS_ATTACKER]	= attacker ? attacker->s.number : ENTITYNUM_WORLD;
		targ->client->damage_armor					+= asave;
		targ->client->damage_blood					+= take;
		targ->client->damage_knockback				+= knockback;
		if ( real_dir ) {
			VectorCopy ( &dir, &targ->client->damage_from );
			targ->client->damage_fromWorld = qfalse;
		}
		else {
			VectorCopy ( &targ->r.currentOrigin, &targ->client->damage_from );
			targ->client->damage_fromWorld = qtrue;
		}
	}

	// See if it's the player hurting the emeny flag carrier
	if ( level.gametype == GT_FLAGS)
		Team_CheckHurtCarrier( targ, attacker );

	if ( targ->client ) {
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;
	}

	// do the damage
	if ( take ) {
		targ->health = targ->health - take;
		if ( targ->client )
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
			
		if ( targ->health <= 0 ) {
			if ( targ->client )
				targ->flags |= FL_NO_KNOCKBACK;

			if ( targ->health < -999 )
				targ->health = -999;

			targ->enemy = attacker;
			targ->die( targ, inflictor, attacker, take, mod );
			return;
		}
		else if ( targ->pain )
			targ->pain( targ, attacker, take );
	}

}


// Returns qtrue if the inflictor can directly damage the target. Used for explosions and melee attacks.
qboolean CanDamage (gentity_t *targ, vector3 *origin) {
	vector3	dest;
	trace_t	tr;
	vector3	midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd (&targ->r.absmin, &targ->r.absmax, &midpoint);
	VectorScale (&midpoint, 0.5, &midpoint);

	VectorCopy (&midpoint, &dest);
	trap->SV_Trace ( &tr, origin, &vec3_origin, &vec3_origin, &dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	// this should probably check in the plane of projection, 
	// rather than in world coordinate, and also include Z
	VectorCopy (&midpoint, &dest);
	dest.x += 15.0;
	dest.y += 15.0;
	trap->SV_Trace ( &tr, origin, &vec3_origin, &vec3_origin, &dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (&midpoint, &dest);
	dest.x += 15.0;
	dest.y -= 15.0;
	trap->SV_Trace ( &tr, origin, &vec3_origin, &vec3_origin, &dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (&midpoint, &dest);
	dest.x -= 15.0;
	dest.y += 15.0;
	trap->SV_Trace ( &tr, origin, &vec3_origin, &vec3_origin, &dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (&midpoint, &dest);
	dest.x -= 15.0;
	dest.y -= 15.0;
	trap->SV_Trace ( &tr, origin, &vec3_origin, &vec3_origin, &dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;


	return qfalse;
}

qboolean G_RadiusDamage( vector3 *origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, gentity_t *missile, int mod )
{
	float		points, dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vector3		mins, maxs;
	vector3		v;
	vector3		dir;
	int			i, e;
	qboolean	hitClient = qfalse;

	if ( radius < 1 ) {
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		mins.data[i] = origin->data[i] - radius;
		maxs.data[i] = origin->data[i] + radius;
	}

	numListedEntities = trap->SV_AreaEntities( &mins, &maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
				 if ( origin->data[i] < ent->r.absmin.data[i] )	v.data[i] = ent->r.absmin.data[i] - origin->data[i];
			else if ( origin->data[i] > ent->r.absmax.data[i] )	v.data[i] = origin->data[i] - ent->r.absmax.data[i];
			else												v.data[i] = 0;
		}

		dist = VectorLength( &v );
		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0f - dist / radius );

		if( CanDamage (ent, origin) ) {
			if( LogAccuracyHit( ent, attacker ) ) {
				hitClient = qtrue;
			}
			VectorSubtract (&ent->r.currentOrigin, origin, &dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir.z += 24;
			//QtZ: The affector here is the missile, so I can apply knockback multipliers for the quantizer and such
			G_Damage (ent, NULL, attacker, missile, &dir, origin, (int)points, DAMAGE_RADIUS, mod);
		}
	}

	return hitClient;
}
