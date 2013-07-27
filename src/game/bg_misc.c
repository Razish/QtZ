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
// bg_misc.c -- both games misc functions, all completely stateless

#include "../qcommon/q_shared.h"
#include "bg_public.h"

#ifdef PROJECT_GAME
	#include "g_local.h"
#endif

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) suspended
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
*/

//RAZMARK: adding new weapons
gitem_t bg_itemlist[] = {
	// sorted alphabetically, case sensitive
	//	classname					pickup sound					world model																		icon							pickup name				quantity	respawn		giType					giTag					precaches	sounds
	{	"ammo_all",					"sound/ammo_pickup.wav",		{ "models/items/battery.md3",			NULL },									"icons/ammo_all",				"Ammo All",				50,			0,			IT_AMMO,				-1,						"",			"" },
	{	"holdable_medkit",			"sound/holdable_pickup.wav",	{ "models/holdable/medkit.md3",			"models/holdable/medkit_sphere.md3" },	"icons/holdable_medkit",		"Medkit",				60,			0,			IT_HOLDABLE,			HI_MEDKIT,				"",			"sound/items/use_medkit.wav" },
	{	"holdable_teleporter",		"sound/holdable_pickup.wav",	{ "models/holdable/teleporter.md3",		NULL },									"icons/holdable_teleporter",	"Personal Teleporter",	60,			0,			IT_HOLDABLE,			HI_TELEPORTER,			"",			"" },
	{	"item_armor_large",			"sound/item_pickup.wav",		{ "models/powerups/armor/large.md3",	NULL },									"icons/armor_large",			"100 Armor",			100,		30,			IT_ARMOR,				0,						"",			"" },
	{	"item_armor_medium",		"sound/item_pickup.wav",		{ "models/powerups/armor/medium.md3",	NULL },									"icons/armor_medium",			"50 Armor",				50,			30,			IT_ARMOR,				0,						"",			"" },
	{	"item_armor_small",			"sound/item_pickup.wav",		{ "models/powerups/armor/small.md3",	NULL },									"icons/armor_small",			"25 Armor",				25,			30,			IT_ARMOR,				0,						"",			"" },
	{	"item_health_large",		"sound/item_pickup.wav",		{ "models/powerups/health/large.md3",	NULL },									"icons/health_large",			"50 Health",			50,			35,			IT_HEALTH,				0,						"",			"" },
	{	"item_health_medium",		"sound/item_pickup.wav",		{ "models/powerups/health/medium.md3",	NULL },									"icons/health_medium",			"25 Health",			25,			35,			IT_HEALTH,				0,						"",			"" },
	{	"powerup_guard",			"sound/powerups/guard.wav",		{ "models/powerups/guard.md3",			NULL },									"icons/powerup_guard",			"Guard",				30,			0,			IT_PERSISTANT_POWERUP,	PW_GUARD,				"",			"" },
	{	"powerup_quad",				"sound/powerups/quad.wav",		{ "models/powerups/quad.md3",			"models/powerups/quad_ring.md3" },		"icons/powerup_quad",			"Quad Damage",			30,			0,			IT_POWERUP,				PW_QUAD,				"",			"sound/items/damage2.wav sound/items/damage3.wav" },
	{	"powerup_regen",			"sound/powerups/regen.wav",		{ "models/powerups/regen.md3",			"models/powerups/regen_ring.md3" },		"icons/powerup_regen",			"Regeneration",			30,			0,			IT_POWERUP,				PW_REGEN,				"",			"sound/items/regen.wav" },
	{	"team_CTF_blueflag",		NULL,							{ "models/flags/b_flag.md3",			NULL },									"icons/team_blueflag",			"Blue Flag",			0,			0,			IT_TEAM,				PW_BLUEFLAG,			"",			"" },
	{	"team_CTF_neutralflag",		NULL,							{ "models/flags/n_flag.md3",			NULL },									"icons/team_neutralflag",		"Neutral Flag",			0,			0,			IT_TEAM,				PW_NEUTRALFLAG,			"",			"" },
	{	"team_CTF_redflag",			NULL,							{ "models/flags/r_flag.md3",			NULL },									"icons/team_redflag",			"Red Flag",				0,			0,			IT_TEAM,				PW_REDFLAG,				"",			"" },
	{	"weapon_divergence",		"sound/weapon_pickup.wav",		{ "models/weapons/divergence/divergence.md3", NULL },							"icons/weapon_divergence",		"Divergence",			10,			0,			IT_WEAPON,				WP_DIVERGENCE,			"",			"" },
	{	"weapon_mortar",			"sound/weapon_pickup.wav",		{ "models/weapons/temp/temp.md3",		NULL },									"icons/weapon_mortar",			"Mortar",				15,			0,			IT_WEAPON,				WP_MORTAR,				"",			"" },
	{	"weapon_quantizer",			"sound/weapon_pickup.wav",		{ "models/weapons/temp/temp.md3",		NULL },									"icons/weapon_quantizer",		"Quantizer",			0,			0,			IT_WEAPON,				WP_QUANTIZER,			"",			"" },
	{	"weapon_repeater",			"sound/weapon_pickup.wav",		{ "models/weapons/temp/temp.md3",		NULL },									"icons/weapon_repeater",		"Repeater",				120,		0,			IT_WEAPON,				WP_REPEATER,			"",			"" },
	{	"weapon_splicer",			"sound/weapon_pickup.wav",		{ "models/weapons/temp/temp.md3",		NULL },									"icons/weapon_splicer",			"Splicer",				100,		0,			IT_WEAPON,				WP_SPLICER,				"",			"" },
	{	NULL,						NULL,							{ NULL,									NULL },									NULL,							NULL,					0,			0,			IT_BAD,					0,						"",			"" },
};
int bg_numItems = ARRAY_LEN( bg_itemlist );


/*
==============
BG_FindItemForPowerup
==============
*/
gitem_t	*BG_FindItemForPowerup( powerup_t pw ) {
	gitem_t *it = NULL;
	int i;

	for ( it=bg_itemlist, i=0; i<bg_numItems; it++, i++ ) {
		if ( (it->giType == IT_POWERUP || it->giType == IT_TEAM || it->giType == IT_PERSISTANT_POWERUP) &&  it->giTag == pw )
			return it;
	}

	Com_Error( ERR_DROP, "Powerup not found: %i", pw );

	return NULL;
}


/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t	*BG_FindItemForHoldable( holdable_t hi ) {
	gitem_t *it = NULL;
	int i;

	for ( it=bg_itemlist, i=0; i<bg_numItems; it++, i++ ) {
		if ( it->giType == IT_HOLDABLE && it->giTag == hi )
			return it;
	}

	Com_Error( ERR_DROP, "HoldableItem not found: %i", hi );

	return NULL;
}


/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t	*BG_FindItemForWeapon( weapon_t weapon ) {
	gitem_t	*it = NULL;
	int i=0;
	
	for ( it=bg_itemlist, i=0; i<bg_numItems; it++, i++ ) {
		if ( it->giType == IT_WEAPON && it->giTag == weapon )
			return it;
	}

	Com_Error( ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

/*
===============
BG_FindItem

===============
*/
static int itempickupcmp( const void *a, const void *b ) {
	return strcmp( (const char *)a, ((gitem_t *)b)->classname );
}

// case sensitive lookup for item classname
gitem_t	*BG_FindItem( const char *classname ) {
	return (gitem_t *)bsearch( classname, bg_itemlist, bg_numItems, sizeof( gitem_t ), itempickupcmp );
}

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vector3 origin;
	vector3 mins=ITEM_MINS, maxs=ITEM_MAXS;

	BG_EvaluateTrajectory( &item->pos, atTime, &origin );

	// we are ignoring ducked differences here
	if ( ps->origin.x - origin.x > maxs.x ||
		 ps->origin.x - origin.x < mins.x ||
		 ps->origin.y - origin.y > maxs.y ||
		 ps->origin.y - origin.y < mins.y ||
		 ps->origin.z - origin.z > maxs.z ||
		 ps->origin.z - origin.z < mins.z ) {
		return qfalse;
	}

	return qtrue;
}



/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/

qboolean BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps ) {
	gitem_t *item = NULL;

	if ( ent->modelindex < 0 || ent->modelindex >= bg_numItems )
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );

	item = &bg_itemlist[ent->modelindex];

	switch( item->giType ) {
	case IT_WEAPON:
		return qtrue;

	case IT_AMMO:
		if ( ps->ammo[item->giTag] >= weaponData[item->giTag].ammoMax )
			return qfalse;
		return qtrue;

	case IT_ARMOR:
		if ( ps->stats[STAT_ARMOR] >= item->quantity )
			return qfalse;
		return qtrue;

	case IT_HEALTH:
		if ( ps->stats[STAT_HEALTH] >= MAX_HEALTH )
			return qfalse;
		return qtrue;

	case IT_POWERUP:
		return qtrue;	// powerups are always picked up

	case IT_PERSISTANT_POWERUP:
		// can only hold one item at a time
		if ( ps->stats[STAT_PERSISTANT_POWERUP] )
			return qfalse;

		// check team only
		if ( !(ent->generic1 & (1<<ps->persistant[PERS_TEAM])) )
			return qfalse;

		return qtrue;

	case IT_TEAM: // team items, such as flags
		if( gametype == GT_1FCTF ) {
			// neutral flag can always be picked up
			if( item->giTag == PW_NEUTRALFLAG )
				return qtrue;

			if ( ps->powerups[PW_NEUTRALFLAG] &&
				((ps->persistant[PERS_TEAM] == TEAM_RED && item->giTag == PW_BLUEFLAG)
				|| (ps->persistant[PERS_TEAM] == TEAM_BLUE && item->giTag == PW_REDFLAG)) )
				return qtrue;
		}
		if( gametype == GT_CTF ) {
			// ent->modelindex2 is non-zero on items if they are dropped
			// we need to know this because we can pick up our dropped flag (and return it)
			// but we can't pick up our flag at base
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if (item->giTag == PW_BLUEFLAG ||
					(item->giTag == PW_REDFLAG && ent->modelindex2) ||
					(item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG]) )
					return qtrue;
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if (item->giTag == PW_REDFLAG ||
					(item->giTag == PW_BLUEFLAG && ent->modelindex2) ||
					(item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG]) )
					return qtrue;
			}
		}

		return qfalse;

	case IT_HOLDABLE:
		// can only hold one item at a time
		if ( ps->stats[STAT_HOLDABLE_ITEM] ) {
			return qfalse;
		}
		return qtrue;

	case IT_BAD:
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD" );
	default:
		#ifdef _DEBUG
			Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n", item->giType );
		#endif
         break;
	}

	return qfalse;
}

//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vector3 *result ) {
	float		deltaTime;
	float		phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( &tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001f;	// milliseconds to seconds
		VectorMA( &tr->trBase, deltaTime, &tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = sinf( deltaTime * M_PI * 2 );
		VectorMA( &tr->trBase, phase, &tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001f;	// milliseconds to seconds
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( &tr->trBase, deltaTime, &tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001f;	// milliseconds to seconds
		VectorMA( &tr->trBase, deltaTime, &tr->trDelta, result );
		result->z -= 0.5f * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trType );
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vector3 *result ) {
	float	deltaTime;
	float	phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
		VectorCopy( &tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = cosf( deltaTime * M_PI * 2 );	// derivative of sin = cos
		phase *= 0.5f;
		VectorScale( &tr->trDelta, phase, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( &tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001f;	// milliseconds to seconds
		VectorCopy( &tr->trDelta, result );
		result->z -= DEFAULT_GRAVITY * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trType );
		break;
	}
}

char *eventnames[EV_NUM_EVENTS] = {
	"EV_NONE",

	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",

	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",

	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",

	"EV_JUMP_PAD",			// boing sound at origin", jump sound on player

	"EV_JUMP",
	"EV_WATER_TOUCH",	// foot touches
	"EV_WATER_LEAVE",	// foot leaves
	"EV_WATER_UNDER",	// head touches
	"EV_WATER_CLEAR",	// head leaves

	"EV_ITEM_PICKUP",			// normal item pickups are predictable
	"EV_GLOBAL_ITEM_PICKUP",	// powerup / team sounds are broadcast to everyone

	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",

	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",

	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",		// no attenuation
	"EV_GLOBAL_TEAM_SOUND",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_HITSCANTRAIL",
	"EV_BULLET",				// otherEntity is the shooter

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	"EV_POWERUP_QUAD",
	"EV_POWERUP_REGEN",

	"EV_GIB_PLAYER",			// gib a previously living player
	"EV_SCOREPLUM",			// score plum

	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT",
	"EV_TAUNT_YES",
	"EV_TAUNT_NO",
	"EV_TAUNT_FOLLOWME",
	"EV_TAUNT_GETFLAG",
	"EV_TAUNT_GUARDBASE",
	"EV_TAUNT_PATROL"
};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

	#ifdef _DEBUG
		#ifdef QAGAME
		//	Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount, ps->eventSequence, eventnames[newEvent], eventParm);
		#else
		//	Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount, ps->eventSequence, eventnames[newEvent], eventParm);
		#endif
	#endif

	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_TouchJumpPad
========================
*/
void BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad ) {
	vector3	angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if ( ps->pm_type != PM_NORMAL ) {
		return;
	}

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if ( ps->jumppad_ent != jumppad->number ) {

		vectoangles( &jumppad->origin2, &angles);
		p = fabsf( AngleNormalize180( angles.pitch ) );
		if( p < 45 ) {
			effectNum = 0;
		} else {
			effectNum = 1;
		}
		BG_AddPredictableEventToPlayerstate( EV_JUMP_PAD, effectNum, ps );
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy( &jumppad->origin2, &ps->velocity );
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {
	int i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( &ps->origin, &s->pos.trBase );
	if ( snap ) {
		VectorSnap( &s->pos.trBase );
	}
	// set the trDelta for flag direction
	VectorCopy( &ps->velocity, &s->pos.trDelta );

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( &ps->viewangles, &s->apos.trBase );
	if ( snap ) {
		VectorSnap( &s->apos.trBase );
	}

	s->angles2.yaw = (float)ps->movementDir;

	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS )
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;

		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
#ifdef PROJECT_GAME
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {
	int i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( &ps->origin, &s->pos.trBase );
	if ( snap ) {
		VectorSnap( &s->pos.trBase );
	}
	// set the trDelta for flag direction and linear prediction
	VectorCopy( &ps->velocity, &s->pos.trDelta );
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = sv_frametime.integer;

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( &ps->viewangles, &s->apos.trBase );
	if ( snap ) {
		VectorSnap( &s->apos.trBase );
	}

	s->angles2.yaw = (float)ps->movementDir;

	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS )
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}
#endif

// MOD-weapon mapping array.
//RAZMARK: ADDING NEW WEAPONS

int weaponFromMOD[MOD_MAX] = {
	WP_NONE,
	WP_QUANTIZER,
	WP_REPEATER,
	WP_SPLICER,
	WP_MORTAR,
	WP_DIVERGENCE,
	WP_NONE,
	WP_NONE,
	WP_NONE,
	WP_NONE,
	WP_NONE,
	WP_NONE,
	WP_NONE,
	WP_NONE,
	WP_NONE,
};

const char *gametypeStringShort[GT_NUM_GAMETYPES] = {
	"DM",
	"1v1",
	"TDM",
	"CTF",
	"1FCTF"
};

const char *BG_GetGametypeString( int gametype )
{
	switch ( gametype )
	{
	case GT_DEATHMATCH:
		return "Deathmatch";
	case GT_DUEL:
		return "Duel";

	case GT_TEAM:
		return "Team Deathmatch";
	case GT_CTF:
		return "Capture The Flag";
	case GT_1FCTF:
		return "1-flag CTF";

	default:
		return "Unknown Gametype";
	}
}

int BG_GetGametypeForString( const char *gametype )
{
		 if ( !Q_stricmp( gametype, "ffa" )
			||!Q_stricmp( gametype, "dm" )
			||!Q_stricmp( gametype, "deathmatch" ) )	return GT_DEATHMATCH;
	else if ( !Q_stricmp( gametype, "duel" ) )			return GT_DUEL;
	else if ( !Q_stricmp( gametype, "tdm" )
			||!Q_stricmp( gametype, "tffa" )
			||!Q_stricmp( gametype, "team" ) )			return GT_TEAM;
	else if ( !Q_stricmp( gametype, "ctf" ) )			return GT_CTF;
	else if ( !Q_stricmp( gametype, "1fctf" ) )			return GT_1FCTF;
	else												return -1;
}
