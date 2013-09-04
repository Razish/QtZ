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

#include "g_local.h"

qboolean	G_SpawnString( const char *key, const char *defaultString, char **out ) {
	int		i;

	if ( !level.spawning ) {
		*out = (char *)defaultString;
//		trap->Error( ERR_DROP, "G_SpawnString() called while not spawning" );
	}

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		if ( !Q_stricmp( key, level.spawnVars[i][0] ) ) {
			*out = level.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

qboolean	G_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = (float)atof( s );
	return present;
}

qboolean	G_SpawnInt( const char *key, const char *defaultString, int *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean	G_SpawnVector( const char *key, const char *defaultString, vector3 *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out->x, &out->y, &out->z );
	return present;
}



//
// fields are needed for spawning from the entity string
//
typedef enum {
	F_INT,
	F_FLOAT,
	F_STRING,
	F_VECTOR,
	F_ANGLEHACK
} fieldtype_t;

typedef struct
{
	char	*name;
	size_t	ofs;
	fieldtype_t	type;
} spawnField_t;

spawnField_t fields[] = {
	// sorted alphabetically, not case sensitive
	{ "angle",					FOFS( s.angles ),				F_ANGLEHACK },
	{ "angles",					FOFS( s.angles ),				F_VECTOR },
	{ "classname",				FOFS( classname ),				F_STRING },
	{ "count",					FOFS( count ),					F_INT },
	{ "dmg",					FOFS( damage ),					F_INT },
	{ "health",					FOFS( health ),					F_INT },
	{ "message",				FOFS( message ),				F_STRING },
	{ "model",					FOFS( model ),					F_STRING },
	{ "model2",					FOFS( model2 ),					F_STRING },
	{ "origin",					FOFS( s.origin ),				F_VECTOR },
	{ "random",					FOFS( random ),					F_FLOAT },
	{ "spawnflags",				FOFS( spawnflags ),				F_INT },
	{ "speed",					FOFS( speed ),					F_FLOAT },
	{ "target",					FOFS( target ),					F_STRING },
	{ "targetname",				FOFS( targetname ),				F_STRING },
	{ "targetShaderName",		FOFS( targetShaderName ),		F_STRING },
	{ "targetShaderNewName",	FOFS( targetShaderNewName ),	F_STRING },
	{ "team",					FOFS( team ),					F_STRING },
	{ "wait",					FOFS( wait ),					F_FLOAT },
};


typedef struct spawn_s {
	char	*name;
	void	(*spawn)(gentity_t *ent);
} spawn_t;

void SP_info_player_start (gentity_t *ent);
void SP_info_player_deathmatch (gentity_t *ent);
void SP_info_player_intermission (gentity_t *ent);

void SP_func_plat (gentity_t *ent);
void SP_func_static (gentity_t *ent);
void SP_func_rotating (gentity_t *ent);
void SP_func_bobbing (gentity_t *ent);
void SP_func_pendulum( gentity_t *ent );
void SP_func_button (gentity_t *ent);
void SP_func_door (gentity_t *ent);
void SP_func_train (gentity_t *ent);
void SP_func_timer (gentity_t *self);

void SP_trigger_always (gentity_t *ent);
void SP_trigger_multiple (gentity_t *ent);
void SP_trigger_push (gentity_t *ent);
void SP_trigger_teleport (gentity_t *ent);
void SP_trigger_hurt (gentity_t *ent);

void SP_target_remove_powerups( gentity_t *ent );
void SP_target_give (gentity_t *ent);
void SP_target_delay (gentity_t *ent);
void SP_target_speaker (gentity_t *ent);
void SP_target_print (gentity_t *ent);
void SP_target_laser (gentity_t *self);
void SP_target_score( gentity_t *ent );
void SP_target_teleporter( gentity_t *ent );
void SP_target_relay (gentity_t *ent);
void SP_target_kill (gentity_t *ent);
void SP_target_position (gentity_t *ent);
void SP_target_location (gentity_t *ent);
void SP_target_push (gentity_t *ent);

void SP_light (gentity_t *self);
void SP_info_null (gentity_t *self);
void SP_info_notnull (gentity_t *self);
void SP_info_camp (gentity_t *self);
void SP_path_corner (gentity_t *self);

void SP_misc_teleporter_dest (gentity_t *self);
void SP_misc_model(gentity_t *ent);
void SP_misc_portal_camera(gentity_t *ent);
void SP_misc_portal_surface(gentity_t *ent);

void SP_shooter_rocket( gentity_t *ent );
void SP_shooter_plasma( gentity_t *ent );
void SP_shooter_grenade( gentity_t *ent );

void SP_team_CTF_redplayer( gentity_t *ent );
void SP_team_CTF_blueplayer( gentity_t *ent );

void SP_team_CTF_redspawn( gentity_t *ent );
void SP_team_CTF_bluespawn( gentity_t *ent );

void SP_item_botroam( gentity_t *ent ) { }

spawn_t	spawns[] = {
	{ "func_bobbing",				SP_func_bobbing },
	{ "func_button",				SP_func_button },
	{ "func_door",					SP_func_door },
	{ "func_group",					SP_info_null },
	{ "func_pendulum",				SP_func_pendulum },
	{ "func_plat",					SP_func_plat },
	{ "func_rotating",				SP_func_rotating },
	{ "func_static",				SP_func_static },
	{ "func_timer",					SP_func_timer },
	{ "func_train",					SP_func_train },
	{ "info_camp",					SP_info_camp },
	{ "info_null",					SP_info_null },
	{ "info_notnull",				SP_info_notnull },
	{ "info_player_deathmatch",		SP_info_player_deathmatch },
	{ "info_player_intermission",	SP_info_player_intermission },
	{ "info_player_start",			SP_info_player_start },
	{ "item_botroam",				SP_item_botroam },
	{ "light",						SP_light },
	{ "misc_model",					SP_misc_model },
	{ "misc_portal_camera",			SP_misc_portal_camera },
	{ "misc_portal_surface",		SP_misc_portal_surface },
	{ "misc_teleporter_dest",		SP_misc_teleporter_dest },
	{ "path_corner",				SP_path_corner },
	{ "shooter_grenade",			SP_shooter_grenade },
	{ "shooter_plasma",				SP_shooter_plasma },
	{ "shooter_rocket",				SP_shooter_rocket },
	{ "target_delay",				SP_target_delay },
	{ "target_give",				SP_target_give },
	{ "target_kill",				SP_target_kill },
	{ "target_laser",				SP_target_laser },
	{ "target_location",			SP_target_location },
	{ "target_position",			SP_target_position },
	{ "target_print",				SP_target_print },
	{ "target_push",				SP_target_push },
	{ "target_relay",				SP_target_relay },
	{ "target_remove_powerups",		SP_target_remove_powerups },
	{ "target_score",				SP_target_score },
	{ "target_speaker",				SP_target_speaker },
	{ "target_teleporter",			SP_target_teleporter },
	{ "team_CTF_bluespawn",			SP_team_CTF_bluespawn },
	{ "team_CTF_blueplayer",		SP_team_CTF_blueplayer },
	{ "team_CTF_redspawn",			SP_team_CTF_redspawn },
	{ "team_CTF_redplayer",			SP_team_CTF_redplayer },
	{ "trigger_always",				SP_trigger_always },
	{ "trigger_hurt",				SP_trigger_hurt },
	{ "trigger_multiple",			SP_trigger_multiple },
	{ "trigger_push",				SP_trigger_push },
	{ "trigger_teleport",			SP_trigger_teleport },
};

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/

static int itemcmp( const void *a, const void *b) {
	return strcmp( (const char *)a, ((gitem_t *)b)->classname);
}

static int spawncmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((spawn_t*)b)->name );
}

qboolean G_CallSpawn( gentity_t *ent ) {
	spawn_t	*s;
	gitem_t	*item;

	if ( !ent->classname ) {
		trap->Print ("G_CallSpawn: NULL classname\n");
		return qfalse;
	}

	// check item spawn functions
	item = (gitem_t *)bsearch( ent->classname, bg_itemlist, bg_numItems, sizeof( gitem_t ), itemcmp );
	if ( item ) {
		G_SpawnItem( ent, item );
		return qtrue;
	}

	// check normal spawn functions
	s = (spawn_t *)bsearch( ent->classname, spawns, ARRAY_LEN( spawns ), sizeof( spawn_t ), spawncmp );
	if ( s ) {
		s->spawn( ent );
		return qtrue;
	}

	trap->Print( "%s doesn't have a spawn function\n", ent->classname );
	return qfalse;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString( const char *string ) {
	char	*newb, *new_p;
	int		i,l;
	
	l = strlen(string) + 1;

	newb = G_Alloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( i=0 ; i< l ; i++ ) {
		if (string[i] == '\\' && i < l-1) {
			i++;
			if (string[i] == 'n') {
				*new_p++ = '\n';
			} else {
				*new_p++ = '\\';
			}
		} else {
			*new_p++ = string[i];
		}
	}
	
	return newb;
}




/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
static int fieldcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((spawnField_t*)b)->name );
}

void G_ParseField( const char *key, const char *value, gentity_t *ent ) {
	spawnField_t	*f;
	byte	*b;
	float	v;
	vector3	vec;

	f = (spawnField_t *)bsearch( key, fields, ARRAY_LEN( fields ), sizeof( spawnField_t ), fieldcmp );
	if ( f ) {
		b = (byte *)ent;

		switch( f->type ) {
		case F_STRING:
			*(char **)(b+f->ofs) = G_NewString (value);
			break;
		case F_VECTOR:
			//Raz: unsafe sscanf usage
			/* q3 code
			sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
			((float *)(b+f->ofs))[0] = vec[0];
			((float *)(b+f->ofs))[1] = vec[1];
			((float *)(b+f->ofs))[2] = vec[2];
			*/
			if ( sscanf( value, "%f %f %f", &vec.x, &vec.y, &vec.z ) == 3 ) {
				((float *)(b+f->ofs))[0] = vec.x;
				((float *)(b+f->ofs))[1] = vec.y;
				((float *)(b+f->ofs))[2] = vec.z;
			}
			else {
				Com_Printf( "G_ParseField: F_VECTOR (%s:%s) with incorrect amount of arguments. Using null vector\n", key, value );
				((float *)(b+f->ofs))[0] = ((float *)(b+f->ofs))[1] = ((float *)(b+f->ofs))[2] = 0.0f;
			}
			break;
		case F_INT:
			*(int *)(b+f->ofs) = atoi(value);
			break;
		case F_FLOAT:
			*(float *)(b+f->ofs) = (float)atof(value);
			break;
		case F_ANGLEHACK:
			v = (float)atof(value);
			((float *)(b+f->ofs))[0] = 0;
			((float *)(b+f->ofs))[1] = v;
			((float *)(b+f->ofs))[2] = 0;
			break;
		}
		return;
	}
}

#define ADJUST_AREAPORTAL() \
	if(ent->s.eType == ET_MOVER) \
	{ \
		trap->SV_LinkEntity((sharedEntity_t *)ent); \
		trap->SV_AdjustAreaPortalState((sharedEntity_t *)ent, qtrue); \
	}

/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
void G_SpawnGEntityFromSpawnVars( void ) {
	int			i;
	gentity_t	*ent;
#if 0
	char		*s, *value, *gametypeName;
#endif

	// get the next free entity
	ent = G_Spawn();

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		G_ParseField( level.spawnVars[i][0], level.spawnVars[i][1], ent );
	}

	// check for "notteam" flag
	if ( level.gametype >= GT_TEAM ) {
		G_SpawnInt( "notteam", "0", &i );
		if ( i ) {
			ADJUST_AREAPORTAL();
			G_FreeEntity( ent );
			return;
		}
	} else {
		G_SpawnInt( "notfree", "0", &i );
		if ( i ) {
			ADJUST_AREAPORTAL();
			G_FreeEntity( ent );
			return;
		}
	}

	//QTZTODO: Rewrite this plz, gametype aliases
	// format is "gametype: dm duel"
#if 0
	if( G_SpawnString( "gametype", NULL, &value ) ) {
		if( level.gametype >= 0 && level.gametype < GT_MAX_GAME_TYPE ) {
			gametypeName = gametypeNames[sv_gametype->integer];

			s = strstr( value, gametypeName );
			if( !s ) {
				ADJUST_AREAPORTAL();
				G_FreeEntity( ent );
				return;
			}
		}
	}
#endif

	// move editor origin to pos
	VectorCopy( &ent->s.origin, &ent->s.pos.trBase );
	VectorCopy( &ent->s.origin, &ent->r.currentOrigin );

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) ) {
		G_FreeEntity( ent );
	}
}



/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken( const char *string ) {
	int		l;
	char	*dest;

	l = strlen( string );
	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		trap->Error( ERR_DROP, "G_AddSpawnVarToken: MAX_SPAWN_VARS_CHARS" );
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l+1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( void ) {
	char		keyname[MAX_TOKEN_CHARS];
	char		com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap->SV_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		trap->Error( ERR_DROP, "G_ParseSpawnVars: found %s when expecting {",com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {	
		// parse key
		if ( !trap->SV_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			trap->Error( ERR_DROP, "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' ) {
			break;
		}
		
		// parse value	
		if ( !trap->SV_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			trap->Error( ERR_DROP, "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			trap->Error( ERR_DROP, "G_ParseSpawnVars: closing brace without data" );
		}
		if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
			trap->Error( ERR_DROP, "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( keyname );
		level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

	return qtrue;
}



/*QUAKED worldspawn (0 0 0) ?

Every map should have exactly one worldspawn.
"music"		music wav file
"gravity"	800 is default gravity
"message"	Text to print during connection process
*/
void SP_worldspawn( void ) {
	char	*s;

	G_SpawnString( "classname", "", &s );
	if ( Q_stricmp( s, "worldspawn" ) ) {
		trap->Error( ERR_DROP, "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	// make some data visible to connecting client
	trap->SV_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	trap->SV_SetConfigstring( CS_LEVEL_START_TIME, va("%i", level.startTime ) );

	G_SpawnString( "music", "", &s );
	trap->SV_SetConfigstring( CS_MUSIC, s );

	G_SpawnString( "message", "", &s );
	trap->SV_SetConfigstring( CS_MESSAGE, s );				// map specific message

	trap->SV_SetConfigstring( CS_MOTD, g_motd->string );		// message of the day

	G_SpawnString( "gravity", "1100", &s );
	trap->Cvar_Set( "pm_gravity", s );

	G_SpawnString( "enableDust", "0", &s );
	trap->Cvar_Set( "g_enableDust", s );

	G_SpawnString( "enableBreath", "0", &s );
	trap->Cvar_Set( "g_enableBreath", s );

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

	g_entities[ENTITYNUM_NONE].s.number = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].classname = "nothing";

	// see if we want a warmup time
	trap->SV_SetConfigstring( CS_WARMUP, "" );
	if ( g_restarted->integer ) {
		trap->Cvar_Set( "g_restarted", "0" );
		level.warmupTime = 0;
	} else if ( g_doWarmup->integer ) { // Turn it on
		level.warmupTime = -1;
		trap->SV_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
		G_LogPrintf( "Warmup:\n" );
	}

}


/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString( void ) {
	// allow calls to G_Spawn*()
	level.spawning = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars() ) {
		trap->Error( ERR_DROP, "SpawnEntities: no entities" );
	}
	SP_worldspawn();

	// parse ents
	while( G_ParseSpawnVars() ) {
		G_SpawnGEntityFromSpawnVars();
	}	

	level.spawning = qfalse;			// any future calls to G_Spawn*() will be errors
}

