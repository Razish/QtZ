// Filename:-	bg_weapons.h
//
// This crosses both client and server.  It could all be crammed into bg_public, but isolation of this type of data is best.

#pragma once

/* Planning for weapons
	WP_NONE

	WP_QUANTIZER		--	Spawn with. Infinite ammo.
							Fast projectile (1600 ups?)
							Low damage (7?)
							High knockback on self and team. Low knockback on opponents. Useful for trick jumping and gaining momentum very quickly (2.5 and 0.7?)
							Can deflect rockets randomly
							Temporarily weakens armor? (0.6 absorb rate?)
							Damage increases over projectile life-time?
	WP_REPEATER			--	Fast projectile (1800 ups?)
							Low damage
							Fast fire time
	WP_DRONE_LAUNCHER	--	High arc projectile(s?) (800 ups?)
							High damage (55?)
							Actively seeks nearby enemies
							Sticks to surfaces
							Explodes on timer
	WP_ROCKET_LAUNCHER  --  Can curve with mouse?
	WP_SPLICER			--	Hitscan
							Mid-high damage to flesh, armor protects
							Fast fire time
							Limited range
	WP_DIVERGENCE		--	Penetrating hitscan
							Very high damage at 100% charge (115)
							Headshot multiplier (1.65)
							2500ms fire time (115 + 69 assuming two very quick body shots)
							Will only be 60% charged by 2500ms, takes 4166ms for full charge

	WP_NUM_WEAPONS
*/

//NOTE: MUST NOT HAVE MORE THAN 16, for weapon cooldowns in playerState_t
//When adding to this list, you must adjust all code tagged with "//RAZMARK: ADDING NEW WEAPONS"
typedef enum weapon_e {
	WP_NONE,

	WP_QUANTIZER,
	WP_REPEATER,
	WP_MORTAR,
	WP_DIVERGENCE,
	WP_NUM_WEAPONS
} weapon_t;

typedef struct weaponData_s
{
	int			ammoMax;			// Index to proper ammo slot
	int			ammoLow;			// Count when ammo is low

	int			ammoPerShot;		// Amount of energy used per shot
	int			fireTime;			// Amount of time between firings
	int			maxFireTime;		// Will only 'charge' to this amount, mostly for the divergence
	int			range;				// Range of weapon

	qboolean	canZoom;			// Boolean of whether you can zoom with this weapon

	vec3_t		muzzle;				// Muzzle point of weapon
	vec3_t		kickAngles;			// Only used client-side
} weaponData_t;

typedef struct weaponName_s
{
	const char *technicalName;
	const char *longName;
} weaponName_t;

extern weaponData_t weaponData[WP_NUM_WEAPONS];
extern weaponName_t weaponNames[WP_NUM_WEAPONS];
