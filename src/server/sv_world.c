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
// world.c -- world query functions

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sv_local.h"

// Returns a headnode that can be used for testing or clipping to a given entity.
//	If the entity is a bsp model, the headnode will be returned, otherwise a custom box tree will be constructed.
clipHandle_t SV_ClipHandleForEntity( const sharedEntity_t *ent ) {
	if ( ent->r.bmodel ) {
		// explicit hulls in the BSP model
		return CM_InlineModel( ent->s.modelindex );
	}
	if ( ent->r.svFlags & SVF_CAPSULE ) {
		// create a temp capsule from bounding box sizes
		return CM_TempBoxModel( &ent->r.mins, &ent->r.maxs, qtrue );
	}

	// create a temp tree from bounding box sizes
	return CM_TempBoxModel( &ent->r.mins, &ent->r.maxs, qfalse );
}



/*

	ENTITY CHECKING

	To avoid linearly searching through lists of entities during environment testing, the world is carved up with
	an evenly spaced, axially aligned bsp tree. Entities are kept in chains either at the final leafs, or at the
	first node that splits them, which prevents having to deal with multiple fragments of a single entity.

*/

typedef struct worldSector_s {
	int		axis;		// -1 = leaf node
	float	dist;
	struct worldSector_s	*children[2];
	svEntity_t	*entities;
} worldSector_t;

#define	AREA_DEPTH	4
#define	AREA_NODES	64

worldSector_t	sv_worldSectors[AREA_NODES];
int			sv_numworldSectors;

void SV_SectorList_f( void ) {
	int				i, c;
	worldSector_t	*sec;
	svEntity_t		*ent;

	for ( i = 0 ; i < AREA_NODES ; i++ ) {
		sec = &sv_worldSectors[i];

		c = 0;
		for ( ent = sec->entities ; ent ; ent = ent->nextEntityInWorldSector ) {
			c++;
		}
		Com_Printf( "sector %i: %i entities\n", i, c );
	}
}

// Builds a uniformly subdivided tree for the given world size
static worldSector_t *SV_CreateworldSector( int depth, vector3 *mins, vector3 *maxs ) {
	worldSector_t	*anode;
	vector3		size;
	vector3		mins1, maxs1, mins2, maxs2;

	anode = &sv_worldSectors[sv_numworldSectors];
	sv_numworldSectors++;

	if (depth == AREA_DEPTH) {
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;
		return anode;
	}
	
	VectorSubtract (maxs, mins, &size);
	if (size.x > size.y) {
		anode->axis = 0;
	} else {
		anode->axis = 1;
	}

	anode->dist = 0.5f * (maxs->data[anode->axis] + mins->data[anode->axis]);
	VectorCopy (mins, &mins1);	
	VectorCopy (mins, &mins2);	
	VectorCopy (maxs, &maxs1);	
	VectorCopy (maxs, &maxs2);	
	
	maxs1.data[anode->axis] = mins2.data[anode->axis] = anode->dist;
	
	anode->children[0] = SV_CreateworldSector (depth+1, &mins2, &maxs2);
	anode->children[1] = SV_CreateworldSector (depth+1, &mins1, &maxs1);

	return anode;
}

void SV_ClearWorld( void ) {
	clipHandle_t	h;
	vector3			mins, maxs;

	memset( sv_worldSectors, 0, sizeof(sv_worldSectors) );
	sv_numworldSectors = 0;

	// get world map bounds
	h = CM_InlineModel( 0 );
	CM_ModelBounds( h, &mins, &maxs );
	SV_CreateworldSector( 0, &mins, &maxs );
}

void SV_UnlinkEntity( sharedEntity_t *gEnt ) {
	svEntity_t		*ent;
	svEntity_t		*scan;
	worldSector_t	*ws;

	ent = SV_SvEntityForGentity( gEnt );

	gEnt->r.linked = qfalse;

	ws = ent->worldSector;
	if ( !ws ) {
		return;		// not linked in anywhere
	}
	ent->worldSector = NULL;

	if ( ws->entities == ent ) {
		ws->entities = ent->nextEntityInWorldSector;
		return;
	}

	for ( scan = ws->entities ; scan ; scan = scan->nextEntityInWorldSector ) {
		if ( scan->nextEntityInWorldSector == ent ) {
			scan->nextEntityInWorldSector = ent->nextEntityInWorldSector;
			return;
		}
	}

	Com_Printf( "WARNING: SV_UnlinkEntity: not found in worldSector\n" );
}

#define MAX_TOTAL_ENT_LEAFS		128
void SV_LinkEntity( sharedEntity_t *gEnt ) {
	worldSector_t	*node;
	int num_leafs, lastLeaf, leafs[MAX_TOTAL_ENT_LEAFS];
	int cluster, area;
	int i, j, k;
	vector3 *origin, *angles;
	svEntity_t *ent;

	ent = SV_SvEntityForGentity( gEnt );

	if ( ent->worldSector ) {
		SV_UnlinkEntity( gEnt );	// unlink from old position
	}

	// encode the size into the entityState_t for client prediction
	if ( gEnt->r.bmodel ) {
		gEnt->s.solid = SOLID_BMODEL;		// a solid_box will never create this value
	} else if ( gEnt->r.contents & ( CONTENTS_SOLID | CONTENTS_BODY ) ) {
		// assume that x/y are equal and symetric
		i = (int)gEnt->r.maxs.x;
		if (i<1)
			i = 1;
		if (i>255)
			i = 255;

		// z is not symetric
		j = (int)(-gEnt->r.mins.z);
		if (j<1)
			j = 1;
		if (j>255)
			j = 255;

		// and z maxs can be negative...
		k = (int)(gEnt->r.maxs.z+32);
		if (k<1)
			k = 1;
		if (k>255)
			k = 255;

		gEnt->s.solid = (k<<16) | (j<<8) | i;
	} else {
		gEnt->s.solid = 0;
	}

	// get the position
	origin = &gEnt->r.currentOrigin;
	angles = &gEnt->r.currentAngles;

	// set the abs box
	if ( gEnt->r.bmodel && (angles->x || angles->y || angles->z) ) {
		// expand for rotation
		float		max;
		int			i;

		max = RadiusFromBounds( &gEnt->r.mins, &gEnt->r.maxs );
		for (i=0 ; i<3 ; i++) {
			gEnt->r.absmin.data[i] = origin->data[i] - max;
			gEnt->r.absmax.data[i] = origin->data[i] + max;
		}
	} else {
		// normal
		VectorAdd (origin, &gEnt->r.mins, &gEnt->r.absmin);	
		VectorAdd (origin, &gEnt->r.maxs, &gEnt->r.absmax);
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	gEnt->r.absmin.x -= 1;
	gEnt->r.absmin.y -= 1;
	gEnt->r.absmin.z -= 1;
	gEnt->r.absmax.x += 1;
	gEnt->r.absmax.y += 1;
	gEnt->r.absmax.z += 1;

	// link to PVS leafs
	ent->numClusters = 0;
	ent->lastCluster = 0;
	ent->areanum = -1;
	ent->areanum2 = -1;

	//get all leafs, including solids
	num_leafs = CM_BoxLeafnums( &gEnt->r.absmin, &gEnt->r.absmax, leafs, MAX_TOTAL_ENT_LEAFS, &lastLeaf );

	// if none of the leafs were inside the map, the
	// entity is outside the world and can be considered unlinked
	if ( !num_leafs ) {
		return;
	}

	// set areas, even from clusters that don't fit in the entity array
	for (i=0 ; i<num_leafs ; i++) {
		area = CM_LeafArea (leafs[i]);
		if (area != -1) {
			// doors may legally straggle two areas,
			// but nothing should evern need more than that
			if (ent->areanum != -1 && ent->areanum != area) {
				if (ent->areanum2 != -1 && ent->areanum2 != area && sv.state == SS_LOADING) {
					Com_DPrintf ("Object %i touching 3 areas at %f %f %f\n",
					gEnt->s.number,
					gEnt->r.absmin.x, gEnt->r.absmin.y, gEnt->r.absmin.z);
				}
				ent->areanum2 = area;
			} else {
				ent->areanum = area;
			}
		}
	}

	// store as many explicit clusters as we can
	ent->numClusters = 0;
	for (i=0 ; i < num_leafs ; i++) {
		cluster = CM_LeafCluster( leafs[i] );
		if ( cluster != -1 ) {
			ent->clusternums[ent->numClusters++] = cluster;
			if ( ent->numClusters == MAX_ENT_CLUSTERS ) {
				break;
			}
		}
	}

	// store off a last cluster if we need to
	if ( i != num_leafs ) {
		ent->lastCluster = CM_LeafCluster( lastLeaf );
	}

	gEnt->r.linkcount++;

	// find the first world sector node that the ent's box crosses
	node = sv_worldSectors;
	while (1)
	{
		if (node->axis == -1)
			break;

			 if ( gEnt->r.absmin.data[node->axis] > node->dist )	node = node->children[0];
		else if ( gEnt->r.absmax.data[node->axis] < node->dist )	node = node->children[1];
		else														break;		// crosses the node
	}
	
	// link it in
	ent->worldSector = node;
	ent->nextEntityInWorldSector = node->entities;
	node->entities = ent;

	gEnt->r.linked = qtrue;
}

/*

	AREA QUERY

	Fills in a list of all entities who's absmin / absmax intersects the given bounds.
	This does NOT mean that they actually touch in the case of bmodels.

*/

typedef struct areaParms_s {
	const vector3	*mins;
	const vector3	*maxs;
	int				*list;
	int				count, maxcount;
} areaParms_t;

static void SV_AreaEntities_r( worldSector_t *node, areaParms_t *ap ) {
	svEntity_t	*check, *next;
	sharedEntity_t *gcheck;

	for ( check = node->entities  ; check ; check = next ) {
		next = check->nextEntityInWorldSector;

		gcheck = SV_GEntityForSvEntity( check );

		if ( gcheck->r.absmin.x > ap->maxs->x ||
			 gcheck->r.absmin.y > ap->maxs->y ||
			 gcheck->r.absmin.z > ap->maxs->z ||
			 gcheck->r.absmax.x < ap->mins->x ||
			 gcheck->r.absmax.y < ap->mins->y ||
			 gcheck->r.absmax.z < ap->mins->z ) {
			continue;
		}

		if ( ap->count == ap->maxcount ) {
			Com_Printf ("SV_AreaEntities: MAXCOUNT\n");
			return;
		}

		ap->list[ap->count] = check - sv.svEntities;
		ap->count++;
	}
	
	if (node->axis == -1) {
		return;		// terminal node
	}

	// recurse down both sides
	if ( ap->maxs->data[node->axis] > node->dist ) {
		SV_AreaEntities_r ( node->children[0], ap );
	}
	if ( ap->mins->data[node->axis] < node->dist ) {
		SV_AreaEntities_r ( node->children[1], ap );
	}
}

int SV_AreaEntities( const vector3 *mins, const vector3 *maxs, int *entityList, int maxcount ) {
	areaParms_t		ap;

	ap.mins = mins;
	ap.maxs = maxs;
	ap.list = entityList;
	ap.count = 0;
	ap.maxcount = maxcount;

	SV_AreaEntities_r( sv_worldSectors, &ap );

	return ap.count;
}

typedef struct moveclip_s {
	vector3			boxmins, boxmaxs;// enclose the test object along entire move
	const vector3	*mins;
	const vector3	*maxs;	// size of the moving object
	const vector3	*start;
	vector3			end;
	trace_t			trace;
	int				passEntityNum;
	int				contentmask;
	int				capsule;
} moveclip_t;

void SV_ClipToEntity( trace_t *trace, const vector3 *start, const vector3 *mins, const vector3 *maxs, const vector3 *end, int entityNum, int contentmask, int capsule ) {
	sharedEntity_t	*touch;
	clipHandle_t	clipHandle;
	vector3			*origin, *angles;

	touch = SV_GentityNum( entityNum );

	memset(trace, 0, sizeof(trace_t));

	// if it doesn't have any brushes of a type we
	// are looking for, ignore it
	if ( ! ( contentmask & touch->r.contents ) ) {
		trace->fraction = 1.0;
		return;
	}

	// might intersect, so do an exact clip
	clipHandle = SV_ClipHandleForEntity (touch);

	origin = &touch->r.currentOrigin;
	angles = &touch->r.currentAngles;

	if ( !touch->r.bmodel ) {
		angles = &vec3_origin;	// boxes don't rotate
	}

	CM_TransformedBoxTrace ( trace, start, end, (vector3 *)mins, (vector3 *)maxs, clipHandle, contentmask, origin, angles, capsule);

	if ( trace->fraction < 1 ) {
		trace->entityNum = touch->s.number;
	}
}

static void SV_ClipMoveToEntities( moveclip_t *clip ) {
	int			i, num;
	int			touchlist[MAX_GENTITIES];
	sharedEntity_t *touch;
	int			passOwnerNum;
	trace_t		trace;
	clipHandle_t	clipHandle;
	vector3		*origin, *angles;

	num = SV_AreaEntities( &clip->boxmins, &clip->boxmaxs, touchlist, MAX_GENTITIES);

	if ( clip->passEntityNum != ENTITYNUM_NONE ) {
		passOwnerNum = ( SV_GentityNum( clip->passEntityNum ) )->r.ownerNum;
		if ( passOwnerNum == ENTITYNUM_NONE ) {
			passOwnerNum = -1;
		}
	} else {
		passOwnerNum = -1;
	}

	for ( i=0 ; i<num ; i++ ) {
		if ( clip->trace.allsolid ) {
			return;
		}
		touch = SV_GentityNum( touchlist[i] );

		// see if we should ignore this entity
		if ( clip->passEntityNum != ENTITYNUM_NONE ) {
			if ( touchlist[i] == clip->passEntityNum ) {
				continue;	// don't clip against the pass entity
			}
			//QTZTODO: Missiles colliding with eachother from https://github.com/dmead/jkaq3/commit/1d1e1fefb04fab8ceb42537702d946c1a75f4812
			if ( touch->r.ownerNum == clip->passEntityNum // don't clip against own missiles
				|| touch->r.ownerNum == passOwnerNum ) // don't clip against other missiles from our owner
				continue;
		}

		// if it doesn't have any brushes of a type we
		// are looking for, ignore it
		if ( ! ( clip->contentmask & touch->r.contents ) ) {
			continue;
		}

		// might intersect, so do an exact clip
		clipHandle = SV_ClipHandleForEntity (touch);

		origin = &touch->r.currentOrigin;
		angles = &touch->r.currentAngles;


		if ( !touch->r.bmodel ) {
			angles = &vec3_origin;	// boxes don't rotate
		}

		CM_TransformedBoxTrace( &trace, (vector3 *)clip->start, &clip->end, (vector3 *)clip->mins, (vector3 *)clip->maxs, clipHandle, clip->contentmask, origin, angles, clip->capsule );

		if ( trace.allsolid ) {
			clip->trace.allsolid = qtrue;
			trace.entityNum = touch->s.number;
		} else if ( trace.startsolid ) {
			clip->trace.startsolid = qtrue;
			trace.entityNum = touch->s.number;
		}

		if ( trace.fraction < clip->trace.fraction ) {
			qboolean	oldStart;

			// make sure we keep a startsolid from a previous trace
			oldStart = clip->trace.startsolid;

			trace.entityNum = touch->s.number;
			clip->trace = trace;
			clip->trace.startsolid |= oldStart;
		}
	}
}

// Moves the given mins/maxs volume through the world from start to end.
//	passEntityNum and entities owned by passEntityNum are explicitly not checked.
void SV_Trace( trace_t *results, const vector3 *start, const vector3 *mins, const vector3 *maxs, const vector3 *end, int passEntityNum, int contentmask, int capsule ) {
	moveclip_t	clip;
	int			i;

	if ( !mins )	mins = &vec3_origin;
	if ( !maxs )	maxs = &vec3_origin;

	memset ( &clip, 0, sizeof ( moveclip_t ) );

	// clip to world
	CM_BoxTrace( &clip.trace, start, end, mins, maxs, 0, contentmask, capsule );
	clip.trace.entityNum = clip.trace.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	if ( clip.trace.fraction == 0 ) {
		*results = clip.trace;
		return;		// blocked immediately by the world
	}

	clip.contentmask = contentmask;
	clip.start = start;
//	VectorCopy( clip.trace.endpos, clip.end );
	VectorCopy( end, &clip.end );
	clip.mins = mins;
	clip.maxs = maxs;
	clip.passEntityNum = passEntityNum;
	clip.capsule = capsule;

	// create the bounding box of the entire move
	// we can limit it to the part of the move not
	// already clipped off by the world, which can be
	// a significant savings for line of sight and shot traces
	for ( i=0 ; i<3 ; i++ ) {
		if ( end->data[i] > start->data[i] ) {
			clip.boxmins.data[i] = clip.start->data[i]	+ clip.mins->data[i] - 1;
			clip.boxmaxs.data[i] = clip.end.data[i]		+ clip.maxs->data[i] + 1;
		} else {
			clip.boxmins.data[i] = clip.end.data[i]		+ clip.mins->data[i] - 1;
			clip.boxmaxs.data[i] = clip.start->data[i]	+ clip.maxs->data[i] + 1;
		}
	}

	// clip to other solid entities
	SV_ClipMoveToEntities ( &clip );

	*results = clip.trace;
}

int SV_PointContents( const vector3 *p, int passEntityNum ) {
	int			touch[MAX_GENTITIES];
	sharedEntity_t *hit;
	int			i, num;
	int			contents, c2;
	clipHandle_t	clipHandle;
	vector3 *angles;

	// get base contents from world
	contents = CM_PointContents( p, 0 );

	// or in contents from all the other entities
	num = SV_AreaEntities( p, p, touch, MAX_GENTITIES );

	for ( i=0 ; i<num ; i++ ) {
		if ( touch[i] == passEntityNum ) {
			continue;
		}
		hit = SV_GentityNum( touch[i] );
		// might intersect, so do an exact clip
		clipHandle = SV_ClipHandleForEntity( hit );
		angles = &hit->r.currentAngles;
		if ( !hit->r.bmodel ) {
			angles = &vec3_origin;	// boxes don't rotate
		}

		c2 = CM_TransformedPointContents (p, clipHandle, &hit->r.currentOrigin, angles);

		contents |= c2;
	}

	return contents;
}


