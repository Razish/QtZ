//
// bg_weapons.c -- part of bg_pmove functionality

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

weaponData_t weaponData[WP_NUM_WEAPONS] = 
{
	//	ammoMax		ammoLow		ammoPerShot		fireTime	maxFireTime		range	canZoom		muzzle
	//RAZMARK: ADDING NEW WEAPONS
	{	0,			0,			0,				0,			0,				0,		qfalse,		{ 0, 0, 0 } },		// WP_NONE
	{	-1,			-1,			0,				800,		800,			0,		qfalse,		{ 0, 0, -6 } },		// WP_QUANTIZER
	{	250,		30,			1,				120,		120,			0,		qfalse,		{ 0, 0, -6 } },		// WP_REPEATER
	{	100,		35,			1,				25,			25,				768,	qfalse,		{ 0, 0, 0 } },		// WP_SPLICER
	{	40,			6,			1,				1000,		1000,			0,		qfalse,		{ 0, 0, -10 } },	// WP_MORTAR
	{	35,			5,			1,				2500,		4166,			16384,	qtrue,		{ 0, 4, -10 } },	// WP_DIVERGENCE
};

weaponName_t weaponNames[WP_NUM_WEAPONS] = {
	{	"WP_NONE",			"None"			},
	{	"WP_QUANTIZER",		"Quantizer"		},
	{	"WP_REPEATER",		"Repeater"		},
	{	"WP_SPLICER",		"Splicer"		},
	{	"WP_MORTAR",		"Mortar"		},
	{	"WP_DIVERGENCE",	"Divergence"	},
};
