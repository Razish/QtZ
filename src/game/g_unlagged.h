#pragma once

void G_StoreTrail( gentity_t *ent );
void G_ResetTrail( gentity_t *ent );
void G_TimeShiftClient( gentity_t *ent, int time );
void G_TimeShiftAllClients( int time, gentity_t *skip );
void G_UnTimeShiftClient( gentity_t *ent );
void G_UnTimeShiftAllClients( gentity_t *skip );

void G_UpdateTrailData( void );

//NT - client origin trails
#define NUM_CLIENT_TRAILS 10
typedef struct clientTrail_s {
	vector3	mins, maxs;
	vector3	currentOrigin, currentAngles;
	int		time, leveltime;
} clientTrail_t;
