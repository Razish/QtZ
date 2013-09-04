#include "cg_local.h"

static char *TB_NearestPickup( void ) {
	float bestDist = 999999999.9f;
	int i = 0;
	centity_t *cent=NULL, *bestEnt=NULL;

	for ( cent=cg_entities, i=MAX_CLIENTS;
		i<ENTITYNUM_WORLD;
		cent++, i++ )
	{
		if ( cent->currentState.eType == ET_ITEM ) {
			float dist = Distance( &cent->lerpOrigin, &cg.predictedPlayerState.origin );
			if ( dist < bestDist ) {
				bestDist = dist;
				bestEnt = cent;
			}
		}
	}

	if ( !bestEnt || (bestEnt->currentState.eFlags & (EF_NODRAW)) )
		return "";
	
	return bg_itemlist[bestEnt->currentState.modelindex].pickup_name;
}

static char *TB_NearestAlly( void ) {
	float bestDist = 999999999.9f;
	int bestClient = -1;
	int i = 0;

	for ( i=0; i<cgs.maxclients; i++ ) {
		if ( i != cg.clientNum && cgs.clientinfo[i].team == cg.snap->ps.persistant[PERS_TEAM] ) {
			float dist = Distance( &cg_entities[i].lerpOrigin, &cg.predictedPlayerState.origin );
			if ( dist < bestDist ) {
				bestDist = dist;
				bestClient = i;
			}
		}
	}

	if ( bestClient == -1 )
		return "";
	
	return cgs.clientinfo[bestClient].name;
}

static char *TB_LastPickup( void ) {
	if ( cg.itemPickup )
		return bg_itemlist[cg.itemPickup].pickup_name;
	else
		return "";
}

static char *TB_Location( void ) {
	return (char *)CG_ConfigString( CS_LOCATIONS + cgs.clientinfo[cg.clientNum].location );
}

static char *TB_Health( void ) {
	return va( "%i", cg.snap->ps.stats[STAT_HEALTH] );
}

static char *TB_Armor( void ) {
	return va( "%i", cg.snap->ps.stats[STAT_ARMOR] );
}

static char *TB_Weapon( void ) {
	return (char *)weaponNames[cg.weaponSelect].longName;
}

static char *TB_Time( void ) {
	int msec=0, secs=0, mins=0;

	msec = cg.time-cgs.levelStartTime;
	secs = msec/1000;
	mins = secs/60;

	secs %= 60;
	msec %= 1000;

	return va( "%i:%02i", mins, secs );
}

typedef struct teamBind_s {
	char *key;
	char *(*GetValue)( void );
} teamBind_t;

teamBind_t teamBinds[] = {
	{ "#p",	TB_NearestPickup }, // Nearest pickup
	{ "#n",	TB_NearestAlly },	// Nearest ally
	{ "#r",	TB_LastPickup },	// Recent/last pickup
	{ "#l",	TB_Location },		// Location
	{ "#h",	TB_Health },		// Health
	{ "#a",	TB_Armor },			// Armor
	{ "#w",	TB_Weapon },		// Weapon
	{ "#t",	TB_Time },			// Time (XX:YY counting up)
};
static const int numTeamBinds = ARRAY_LEN( teamBinds );

void CG_HandleTeamBinds( char *buf, int bufsize ) {
	char *p = buf;
	int i=0;
	for ( i=0; i<numTeamBinds; i++ ) {
		char *tmp = NULL;
		if ( !strstr( p, teamBinds[i].key ) )
			continue;

		tmp = Q_strrep( p, teamBinds[i].key, teamBinds[i].GetValue() );
		if ( p != buf )
			free( p );
		p = tmp;
	}
	Q_strncpyz( buf, p, bufsize );
	if ( p != buf )
		free( p );
}
