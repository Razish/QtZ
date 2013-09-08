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
// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"

// for the voice chats
#include "../../build/qtz/ui/menudef.h"
//==========================================================================

/*
=============
CG_Obituary
=============
*/
static void CG_Obituary( entityState_t *ent )
{
	int				mod = ent->eventParm, target = ent->otherEntityNum, attacker = ent->otherEntityNum2;
	const char		*targetInfo = NULL, *attackerInfo = NULL;
	char			targetName[32] = {0}, attackerName[32] = {0};
	clientInfo_t	*ci = NULL;

	if ( target < 0 || target >= MAX_CLIENTS )
		trap->Error( ERR_DROP, "CG_Obituary: target out of range" );
	ci = &cgs.clientinfo[target];

	if ( attacker < 0 || attacker >= MAX_CLIENTS ) {
		attacker = ENTITYNUM_WORLD;
		attackerInfo = NULL;
	}
	else
		attackerInfo = CG_ConfigString( CS_PLAYERS + attacker );

	if ( !(targetInfo = CG_ConfigString( CS_PLAYERS + target )) )
		return;

	Q_strncpyz( targetName, Info_ValueForKey( targetInfo, "n" ), sizeof( targetName ) - 2);
	strcat( targetName, S_COLOR_WHITE );

	// "You fragged" message, not for self
	if ( attacker == cg.snap->ps.clientNum && attacker != target )
		CG_CenterPrint( va( "You fragged %s", targetName ), (int)(SCREEN_HEIGHT * 0.2f) );

	// check for double client messages
	if ( !attackerInfo )
	{
		attacker = ENTITYNUM_WORLD;
		strcpy( attackerName, "noname" );
	}
	else
	{
		Q_strncpyz( attackerName, Info_ValueForKey( attackerInfo, "n" ), sizeof(attackerName) - 2);
		strcat( attackerName, S_COLOR_WHITE );
		// check for kill messages about the current clientNum
		if ( target == cg.snap->ps.clientNum )
			Q_strncpyz( cg.killerName, attackerName, sizeof( cg.killerName ) );
	}

//	if ( attacker == target )

	//RAZMARK: Adding new weapons
	if ( attacker != ENTITYNUM_WORLD )
	{
		trap->Print( "%s was killed by %s\n", targetName, attackerName );
		//QTZTODO: Obituary HUD
#if 0
		if ( cg_newObituary.boolean && weaponFromMOD[mod] != WP_NONE )
		{
			obituary_t *obituary = (obituary_t *)malloc( sizeof( obituary_t ) );
			memset( obituary, 0, sizeof( obituary_t ) );

			Q_strncpyz( obituary->targetName, targetName, sizeof( obituary->targetName ) );
			obituary->mod = mod;
			Q_strncpyz( obituary->attackerName, attackerName, sizeof( obituary->attackerName ) );
			obituary->deathTime = cg.time;

			LinkedList_PushObject( &cg.obituaryRoot, obituary );
		}
#endif
	}

	else // we don't know what it was
		trap->Print( "%s died\n", targetName );
}

//==========================================================================

/*
===============
CG_UseItem
===============
*/
static void CG_UseItem( centity_t *cent ) {
	clientInfo_t *ci;
	int			itemNum, clientNum;
	gitem_t		*item;
	entityState_t *es;

	es = &cent->currentState;
	
	itemNum = (es->event & ~EV_EVENT_BITS) - EV_USE_ITEM0;
	if ( itemNum < 0 || itemNum > HI_NUM_HOLDABLE ) {
		itemNum = 0;
	}

	// print a message if the local player
	if ( es->number == cg.snap->ps.clientNum ) {
		if ( !itemNum ) {
			CG_CenterPrint( "No item to use", (int)(SCREEN_HEIGHT * 0.30f) );
		} else {
			item = BG_FindItemForHoldable( (holdable_t)itemNum );
			CG_CenterPrint( va("Use %s", item->pickup_name), (int)(SCREEN_HEIGHT * 0.30f) );
		}
	}

	switch ( itemNum ) {
	default:
	case HI_NONE:
		trap->S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.useNothingSound );
		break;

	case HI_TELEPORTER:
		break;

	case HI_MEDKIT:
		clientNum = cent->currentState.clientNum;
		if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
			ci = &cgs.clientinfo[ clientNum ];
			ci->medkitUsageTime = cg.time;
		}
		trap->S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.medkitSound );
		break;
	}

}

/*
================
CG_ItemPickup

A new item was picked up this frame
================
*/
static void CG_ItemPickup( int itemNum )
{
	//QTZTODO: Item pickup HUD
#if 0
	itemPickup_t *newItem = (itemPickup_t *)malloc( sizeof( itemPickup_t ) );
	memset( newItem, 0, sizeof( itemPickup_t ) );

	newItem->itemNum = itemNum;
	newItem->pickupTime = cg.time;
	LinkedList_PushObject( &cg.itemPickupRoot, newItem );
#endif

	cg.itemPickup = itemNum;
	cg.itemPickupTime = cg.time;
	cg.itemPickupBlendTime = cg.time;
	// see if it should be the grabbed weapon
	if ( bg_itemlist[itemNum].giType == IT_WEAPON ) {
		// select it immediately
		if ( cg_autoswitch->boolean && bg_itemlist[itemNum].giTag > cg.predictedPlayerState.weapon ) {
			cg.weaponSelectTime = cg.time;
			cg.weaponSelect = bg_itemlist[itemNum].giTag;
		}
	}

}

/*
================
CG_WaterLevel

Returns waterlevel for entity origin
================
*/
int CG_WaterLevel(centity_t *cent) {
	vector3 point;
	int contents, sample1, sample2, anim, waterlevel;

	// get waterlevel, accounting for ducking
	waterlevel = 0;
	VectorCopy(&cent->lerpOrigin, &point);
	point.z += MINS_Z + 1;
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;

	if (anim == LEGS_WALKCR || anim == LEGS_IDLECR)
		point.z += CROUCH_VIEWHEIGHT;
	else
		point.z += DEFAULT_VIEWHEIGHT;

	contents = CG_PointContents(&point, -1);

	if (contents & MASK_WATER) {
		sample2 = (int)point.z - MINS_Z;
		sample1 = sample2 / 2;
		waterlevel = 1;
		point.z = cent->lerpOrigin.z + MINS_Z + sample1;
		contents = CG_PointContents(&point, -1);

		if (contents & MASK_WATER) {
			waterlevel = 2;
			point.z = cent->lerpOrigin.z + MINS_Z + sample2;
			contents = CG_PointContents(&point, -1);

			if (contents & MASK_WATER) {
				waterlevel = 3;
			}
		}
	}

	return waterlevel;
}

/*
================
CG_PainEvent

Also called by playerstate transition
================
*/
void CG_PainEvent( centity_t *cent, int health ) {
	char	*snd;

	// don't do more than two pain sounds a second
	if ( cg.time - cent->pe.painTime < 500 ) {
		return;
	}

	if ( health < 25 ) {
		snd = "*pain25_1.wav";
	} else if ( health < 50 ) {
		snd = "*pain50_1.wav";
	} else if ( health < 75 ) {
		snd = "*pain75_1.wav";
	} else {
		snd = "*pain100_1.wav";
	}
	// play a gurp sound instead of a normal pain sound
	if (CG_WaterLevel(cent) >= 1) {
		if (rand()&1) {
			trap->S_StartSound(NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound(cent->currentState.number, "sound/player/gurp1.wav"));
		} else {
			trap->S_StartSound(NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound(cent->currentState.number, "sound/player/gurp2.wav"));
		}
	} else {
		trap->S_StartSound(NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound(cent->currentState.number, snd));
	}
	// save pain time for programitic twitch animation
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}



/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/

static void CG_FootstepEvent( int entnum, sfxHandle_t sfx ) {
	if ( cg_footsteps->boolean )
		trap->S_StartSound( NULL, entnum, CHAN_BODY, sfx );
}

#define	DEBUGNAME(x) if(cg_debugEvents->boolean){trap->Print(x"\n");}
void CG_EntityEvent( centity_t *cent, vector3 *position ) {
	entityState_t	*es;
	int				event;
	vector3			dir;
	const char		*s;
	int				clientNum;
	clientInfo_t	*ci;

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;

	if ( cg_debugEvents->boolean ) {
		trap->Print( "ent:%3i  event:%3i ", es->number, event );
	}

	if ( !event ) {
		DEBUGNAME("ZEROEVENT");
		return;
	}

	clientNum = es->clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	switch ( event ) {
	//
	// movement generated events
	//
	case EV_FOOTSTEP:
		DEBUGNAME("EV_FOOTSTEP");
		CG_FootstepEvent( es->number, cgs.media.footsteps[ ci->footsteps ][rand()&3] );
		break;
	case EV_FOOTSTEP_METAL:
		DEBUGNAME("EV_FOOTSTEP_METAL");
		CG_FootstepEvent( es->number, cgs.media.footsteps[ FOOTSTEP_METAL ][rand()&3] );
		break;
	case EV_FOOTSPLASH:
		DEBUGNAME("EV_FOOTSPLASH");
		CG_FootstepEvent( es->number, cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		break;
	case EV_FOOTWADE:
		DEBUGNAME("EV_FOOTWADE");
		CG_FootstepEvent( es->number, cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		break;
	case EV_SWIM:
		DEBUGNAME("EV_SWIM");
		CG_FootstepEvent( es->number, cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		break;


	case EV_FALL_SHORT:
		DEBUGNAME("EV_FALL_SHORT");
		trap->S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.landSound );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -8;
			cg.landTime = cg.time;
		}
		break;
	case EV_FALL_MEDIUM:
		DEBUGNAME("EV_FALL_MEDIUM");
		// use normal pain sound
		trap->S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*pain100_1.wav" ) );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
		}
		break;
	case EV_FALL_FAR:
		DEBUGNAME("EV_FALL_FAR");
		trap->S_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall1.wav" ) );
		cent->pe.painTime = cg.time;	// don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}
		break;

	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:		// smooth out step up transitions
		DEBUGNAME("EV_STEP");
	{
		float	oldStep;
		int		delta;
		int		step;

		if ( clientNum != cg.predictedPlayerState.clientNum ) {
			break;
		}
		// if we are interpolating, we don't need to smooth steps
		if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) || bg_synchronousClients->boolean )
			break;

		// check for stepping up before a previous step is completed
		delta = cg.time - cg.stepTime;
		if (delta < STEP_TIME) {
			oldStep = cg.stepChange * (STEP_TIME - delta) / STEP_TIME;
		} else {
			oldStep = 0;
		}

		// add this amount
		step = 4 * (event - EV_STEP_4 + 1 );
		cg.stepChange = oldStep + step;
		if ( cg.stepChange > MAX_STEP_CHANGE ) {
			cg.stepChange = MAX_STEP_CHANGE;
		}
		cg.stepTime = cg.time;
		break;
	}

	case EV_JUMP_PAD:
		DEBUGNAME("EV_JUMP_PAD");
//		trap->Print( "EV_JUMP_PAD w/effect #%i\n", es->eventParm );
		{
			vector3			up = {0, 0, 1};


			CG_SmokePuff( &cent->lerpOrigin, &up, 
						  32, 
						  1, 1, 1, 0.33f,
						  1000, 
						  cg.time, 0,
						  LEF_PUFF_DONT_SCALE, 
						  0/*cgs.media.smokePuffShader*/ );
		}

		// boing sound at origin, jump sound on player
		trap->S_StartSound ( &cent->lerpOrigin, -1, CHAN_VOICE, cgs.media.jumpPadSound );
		trap->S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );
		break;

	case EV_JUMP:
		DEBUGNAME("EV_JUMP");
		trap->S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );
		break;
	case EV_TAUNT:
		DEBUGNAME("EV_TAUNT");
		trap->S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*taunt.wav" ) );
		break;
	case EV_TAUNT_YES:
		DEBUGNAME("EV_TAUNT_YES");
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_YES);
		break;
	case EV_TAUNT_NO:
		DEBUGNAME("EV_TAUNT_NO");
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_NO);
		break;
	case EV_TAUNT_FOLLOWME:
		DEBUGNAME("EV_TAUNT_FOLLOWME");
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_FOLLOWME);
		break;
	case EV_TAUNT_GETFLAG:
		DEBUGNAME("EV_TAUNT_GETFLAG");
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_ONGETFLAG);
		break;
	case EV_TAUNT_GUARDBASE:
		DEBUGNAME("EV_TAUNT_GUARDBASE");
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_ONDEFENSE);
		break;
	case EV_TAUNT_PATROL:
		DEBUGNAME("EV_TAUNT_PATROL");
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_ONPATROL);
		break;
	case EV_WATER_TOUCH:
		DEBUGNAME("EV_WATER_TOUCH");
		trap->S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrInSound );
		break;
	case EV_WATER_LEAVE:
		DEBUGNAME("EV_WATER_LEAVE");
		trap->S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound );
		break;
	case EV_WATER_UNDER:
		DEBUGNAME("EV_WATER_UNDER");
		trap->S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound );
		break;
	case EV_WATER_CLEAR:
		DEBUGNAME("EV_WATER_CLEAR");
		trap->S_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*gasp.wav" ) );
		break;

	case EV_ITEM_PICKUP:
		DEBUGNAME("EV_ITEM_PICKUP");
		{
			gitem_t	*item;
			int		index;

			index = es->eventParm;		// player predicted

			if ( index < 1 || index >= bg_numItems ) {
				break;
			}
			item = &bg_itemlist[ index ];

			// powerups and team items will have a separate global sound, this one
			// will be played at prediction time
			if ( item->giType == IT_POWERUP || item->giType == IT_TEAM) {
				trap->S_StartSound (NULL, es->number, CHAN_AUTO,	cgs.media.n_healthSound );
			} else if (item->giType == IT_PERSISTANT_POWERUP) {
				switch (item->giTag ) {
					case PW_GUARD:
						trap->S_StartSound (NULL, es->number, CHAN_AUTO,	cgs.media.guardSound );
					break;
				}
			} else {
				trap->S_StartSound (NULL, es->number, CHAN_AUTO,	trap->S_RegisterSound( item->pickup_sound, qfalse ) );
			}

			// show icon and name on status bar
			if ( es->number == cg.snap->ps.clientNum ) {
				CG_ItemPickup( index );
			}
		}
		break;

	case EV_GLOBAL_ITEM_PICKUP:
		DEBUGNAME("EV_GLOBAL_ITEM_PICKUP");
		{
			gitem_t	*item;
			int		index;

			index = es->eventParm;		// player predicted

			if ( index < 1 || index >= bg_numItems ) {
				break;
			}
			item = &bg_itemlist[ index ];
			// powerup pickups are global
			if( item->pickup_sound ) {
				trap->S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, trap->S_RegisterSound( item->pickup_sound, qfalse ) );
			}

			// show icon and name on status bar
			if ( es->number == cg.snap->ps.clientNum ) {
				CG_ItemPickup( index );
			}
		}
		break;

	//
	// weapon events
	//
	case EV_NOAMMO:
		DEBUGNAME("EV_NOAMMO");
//		trap->S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
		if ( es->number == cg.snap->ps.clientNum ) {
			CG_OutOfAmmoChange();
		}
		break;
	case EV_CHANGE_WEAPON:
		DEBUGNAME("EV_CHANGE_WEAPON");
		trap->S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.selectSound );
		break;
	case EV_FIRE_WEAPON:
		DEBUGNAME("EV_FIRE_WEAPON");
		//QtZ
		CG_FireWeapon( cent, es->eventParm );
		break;

	case EV_USE_ITEM0:
		DEBUGNAME("EV_USE_ITEM0");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM1:
		DEBUGNAME("EV_USE_ITEM1");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM2:
		DEBUGNAME("EV_USE_ITEM2");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM3:
		DEBUGNAME("EV_USE_ITEM3");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM4:
		DEBUGNAME("EV_USE_ITEM4");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM5:
		DEBUGNAME("EV_USE_ITEM5");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM6:
		DEBUGNAME("EV_USE_ITEM6");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM7:
		DEBUGNAME("EV_USE_ITEM7");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM8:
		DEBUGNAME("EV_USE_ITEM8");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM9:
		DEBUGNAME("EV_USE_ITEM9");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM10:
		DEBUGNAME("EV_USE_ITEM10");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM11:
		DEBUGNAME("EV_USE_ITEM11");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM12:
		DEBUGNAME("EV_USE_ITEM12");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM13:
		DEBUGNAME("EV_USE_ITEM13");
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM14:
		DEBUGNAME("EV_USE_ITEM14");
		CG_UseItem( cent );
		break;

	//=================================================================

	//
	// other events
	//
	case EV_PLAYER_TELEPORT_IN:
		DEBUGNAME("EV_PLAYER_TELEPORT_IN");
		trap->S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.teleInSound );
		CG_SpawnEffect( position);
		break;

	case EV_PLAYER_TELEPORT_OUT:
		DEBUGNAME("EV_PLAYER_TELEPORT_OUT");
		trap->S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.teleOutSound );
		CG_SpawnEffect(  position);
		break;

	case EV_ITEM_POP:
		DEBUGNAME("EV_ITEM_POP");
		trap->S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.respawnSound );
		break;
	case EV_ITEM_RESPAWN:
		DEBUGNAME("EV_ITEM_RESPAWN");
		cent->miscTime = cg.time;	// scale up from this
		trap->S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.respawnSound );
		break;

	case EV_SCOREPLUM:
		DEBUGNAME("EV_SCOREPLUM");
		CG_ScorePlum( cent->currentState.otherEntityNum, &cent->lerpOrigin, cent->currentState.time );
		break;

	//
	// missile impacts
	//
	case EV_MISSILE_HIT:
		DEBUGNAME("EV_MISSILE_HIT");
		ByteToDir( es->eventParm, &dir );
		CG_MissileHitPlayer( es->weapon, position, &dir, es->otherEntityNum );
		break;

	case EV_MISSILE_MISS:
		DEBUGNAME("EV_MISSILE_MISS");
		ByteToDir( es->eventParm, &dir );
		CG_MissileHitWall( es->weapon, 0, position, &dir, IMPACTSOUND_DEFAULT );
		break;

	case EV_MISSILE_MISS_METAL:
		DEBUGNAME("EV_MISSILE_MISS_METAL");
		ByteToDir( es->eventParm, &dir );
		CG_MissileHitWall( es->weapon, 0, position, &dir, IMPACTSOUND_METAL );
		break;

	case EV_HITSCANTRAIL:
		DEBUGNAME("EV_HITSCANTRAIL");
		// assume this is the right weapon
		//cent->currentState.weapon = WP_DIVERGENCE;

		if ( es->clientNum == cg.clientNum /*&& RAZFIXME: Check for spectating?*/)
		{//We don't need this event for ourself - the client draws its own weapon effects in CG_FireWeapon
			return;
		}

		//RAZTODO: Calculate muzzle from es->clientNum
		//RAZFIXME: Make sure this works
	//	CG_RailTrail(ci, es->origin2, es->pos.trBase);

		// if the end was on a nomark surface, don't make an explosion
		if ( es->eventParm != 255 ) {
			ByteToDir( es->eventParm, &dir );
			CG_MissileHitWall( es->weapon, es->clientNum, position, &dir, IMPACTSOUND_DEFAULT );
		}
		break;

	case EV_GENERAL_SOUND:
		DEBUGNAME("EV_GENERAL_SOUND");
		if ( cgs.gameSounds[ es->eventParm ] ) {
			trap->S_StartSound (NULL, es->number, CHAN_VOICE, cgs.gameSounds[ es->eventParm ] );
		} else {
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			trap->S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, s ) );
		}
		break;

	case EV_GLOBAL_SOUND:	// play from the player's head so it never diminishes
		DEBUGNAME("EV_GLOBAL_SOUND");
		if ( cgs.gameSounds[ es->eventParm ] ) {
			trap->S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gameSounds[ es->eventParm ] );
		} else {
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			trap->S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, CG_CustomSound( es->number, s ) );
		}
		break;

	case EV_GLOBAL_TEAM_SOUND:	// play from the player's head so it never diminishes
		{
			DEBUGNAME("EV_GLOBAL_TEAM_SOUND");
			switch( es->eventParm ) {
				case GTS_RED_CAPTURE: // CTF: red team captured the blue flag, 1FCTF: red team captured the neutral flag
					if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED )
						CG_AddBufferedSound( cgs.media.captureYourTeamSound );
					else
						CG_AddBufferedSound( cgs.media.captureOpponentSound );
					break;
				case GTS_BLUE_CAPTURE: // CTF: blue team captured the red flag, 1FCTF: blue team captured the neutral flag
					if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
						CG_AddBufferedSound( cgs.media.captureYourTeamSound );
					else
						CG_AddBufferedSound( cgs.media.captureOpponentSound );
					break;
				case GTS_RED_RETURN: // CTF: blue flag returned, 1FCTF: never used
					if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED )
						CG_AddBufferedSound( cgs.media.returnYourTeamSound );
					else
						CG_AddBufferedSound( cgs.media.returnOpponentSound );
					//
					CG_AddBufferedSound( cgs.media.blueFlagReturnedSound );
					break;
				case GTS_BLUE_RETURN: // CTF red flag returned, 1FCTF: neutral flag returned
					if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
						CG_AddBufferedSound( cgs.media.returnYourTeamSound );
					else
						CG_AddBufferedSound( cgs.media.returnOpponentSound );
					//
					CG_AddBufferedSound( cgs.media.redFlagReturnedSound );
					break;

				case GTS_RED_TAKEN: // CTF: red team took blue flag, 1FCTF: blue team took the neutral flag
					// if this player picked up the flag then a sound is played in CG_CheckLocalSounds
					if (cg.snap->ps.powerups[PW_BLUEFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG]) {
					}
					else {
						if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
							if (cgs.gametype == GT_1FCTF) 
								CG_AddBufferedSound( cgs.media.yourTeamTookTheFlagSound );
							else
							CG_AddBufferedSound( cgs.media.enemyTookYourFlagSound );
						}
						else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
							if (cgs.gametype == GT_1FCTF)
								CG_AddBufferedSound( cgs.media.enemyTookTheFlagSound );
							else
 							CG_AddBufferedSound( cgs.media.yourTeamTookEnemyFlagSound );
						}
					}
					break;
				case GTS_BLUE_TAKEN: // CTF: blue team took the red flag, 1FCTF red team took the neutral flag
					// if this player picked up the flag then a sound is played in CG_CheckLocalSounds
					if (cg.snap->ps.powerups[PW_REDFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG]) {
					}
					else {
						if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
							if (cgs.gametype == GT_1FCTF)
								CG_AddBufferedSound( cgs.media.yourTeamTookTheFlagSound );
							else
							CG_AddBufferedSound( cgs.media.enemyTookYourFlagSound );
						}
						else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
							if (cgs.gametype == GT_1FCTF)
								CG_AddBufferedSound( cgs.media.enemyTookTheFlagSound );
							else
							CG_AddBufferedSound( cgs.media.yourTeamTookEnemyFlagSound );
						}
					}
					break;
				case GTS_REDTEAM_SCORED:
					CG_AddBufferedSound(cgs.media.redScoredSound);
					break;
				case GTS_BLUETEAM_SCORED:
					CG_AddBufferedSound(cgs.media.blueScoredSound);
					break;
				case GTS_REDTEAM_TOOK_LEAD:
					CG_AddBufferedSound(cgs.media.redLeadsSound);
					break;
				case GTS_BLUETEAM_TOOK_LEAD:
					CG_AddBufferedSound(cgs.media.blueLeadsSound);
					break;
				case GTS_TEAMS_ARE_TIED:
					CG_AddBufferedSound( cgs.media.teamsTiedSound );
					break;
				default:
					break;
			}
			break;
		}

	case EV_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
		DEBUGNAME("EV_PAIN");
		if ( cent->currentState.number != cg.snap->ps.clientNum ) {
			CG_PainEvent( cent, es->eventParm );
		}
		break;

	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
		DEBUGNAME("EV_DEATHx");

		if (CG_WaterLevel(cent) >= 1) {
			trap->S_StartSound(NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, "*drown.wav"));
		} else {
			trap->S_StartSound(NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, va("*death%i.wav", event - EV_DEATH1 + 1)));
		}

		break;


	case EV_OBITUARY:
		DEBUGNAME("EV_OBITUARY");
		CG_Obituary( es );
		break;

	//
	// powerup events
	//
	case EV_POWERUP_QUAD:
		DEBUGNAME("EV_POWERUP_QUAD");
		if ( es->number == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_QUAD;
			cg.powerupTime = cg.time;
		}
		trap->S_StartSound (NULL, es->number, CHAN_ITEM, cgs.media.quadSound );
		break;
	case EV_POWERUP_REGEN:
		DEBUGNAME("EV_POWERUP_REGEN");
		if ( es->number == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_REGEN;
			cg.powerupTime = cg.time;
		}
		trap->S_StartSound (NULL, es->number, CHAN_ITEM, cgs.media.regenSound );
		break;

	case EV_GIB_PLAYER:
		DEBUGNAME("EV_GIB_PLAYER");
		// don't play gib sound when using the kamikaze because it interferes
		// with the kamikaze sound, downside is that the gib sound will also
		// not be played when someone is gibbed while just carrying the kamikaze
		trap->S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.gibSound );
		CG_GibPlayer( &cent->lerpOrigin );
		break;

	case EV_STOPLOOPINGSOUND:
		DEBUGNAME("EV_STOPLOOPINGSOUND");
		trap->S_StopLoopingSound( es->number );
		es->loopSound = 0;
		break;

	case EV_DEBUG_LINE:
		DEBUGNAME("EV_DEBUG_LINE");
		CG_Beam( cent );
		break;

	default:
		DEBUGNAME("UNKNOWN");
		trap->Error( ERR_DROP, "Unknown event: %i", event );
		break;
	}

}


/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents( centity_t *cent ) {
	// check for event-only entities
	if ( cent->currentState.eType > ET_EVENTS ) {
		if ( cent->previousEvent ) {
			return;	// already fired
		}
		// if this is a player event set the entity number of the client entity number
		if ( cent->currentState.eFlags & EF_PLAYER_EVENT ) {
			cent->currentState.number = cent->currentState.otherEntityNum;
		}

		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	} else {
		// check for events riding with another entity
		if ( cent->currentState.event == cent->previousEvent ) {
			return;
		}
		cent->previousEvent = cent->currentState.event;
		if ( ( cent->currentState.event & ~EV_EVENT_BITS ) == 0 ) {
			return;
		}
	}

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, &cent->lerpOrigin );
	CG_SetEntitySoundPosition( cent );

	CG_EntityEvent( cent, &cent->lerpOrigin );
}

