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

typedef enum botInventory_e {
	INVENTORY_NONE=0,

	//armor
	INVENTORY_ARMOR,

	//weapons
	//RAZMARK: ADDING NEW WEAPONS
	INVENTORY_QUANTIZER,
	INVENTORY_REPEATER,
	INVENTORY_SPLICER,
	INVENTORY_MORTAR,
	INVENTORY_DIVERGENCE,

	//ammo here
	INVENTORY_AMMOREPEATER,
	INVENTORY_AMMOSPLICER,
	INVENTORY_AMMOMORTAR,
	INVENTORY_AMMODIVERGENCE,

	//powerups
	INVENTORY_HEALTH,
	INVENTORY_MEDKIT,
	INVENTORY_QUAD,
	INVENTORY_REGEN,
	INVENTORY_GUARD,

	INVENTORY_REDFLAG,
	INVENTORY_BLUEFLAG,
	INVENTORY_NEUTRALFLAG,

	//enemy stuff
	ENEMY_HORIZONTAL_DIST = 200,
	ENEMY_HEIGHT,
	NUM_VISIBLE_ENEMIES,
	NUM_VISIBLE_TEAMMATES,
} botInventory;

//item numbers (make sure they are in sync with bg_itemlist in bg_misc.c)
typedef enum botModelIndex_e {
	MODELINDEX_NONE=0,
	MODELINDEX_ARMORSMALL,
	MODELINDEX_ARMORMEDIUM,
	MODELINDEX_ARMORLARGE,
	MODELINDEX_ARMORMEGA,

	MODELINDEX_HEALTHSMALL,
	MODELINDEX_HEALTHMEDIUM,
	MODELINDEX_HEALTHLARGE,
	MODELINDEX_HEALTHMEGA,

	MODELINDEX_QUANTIZER,
	MODELINDEX_REPEATER,
	MODELINDEX_MORTAR,
	MODELINDEX_DIVERGENCE,

	MODELINDEX_AMMOALL,

	MODELINDEX_TELEPORTER,
	MODELINDEX_MEDKIT,
	MODELINDEX_QUAD,
	MODELINDEX_REGEN,

	MODELINDEX_GUARD,

	MODELINDEX_REDFLAG,
	MODELINDEX_BLUEFLAG,
	MODELINDEX_NEUTRALFLAG,
} botModelIndex_t;
