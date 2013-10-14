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

#include "cm_local.h"
#include "cm_patch.h"

/*

This file does not reference any globals, and has these entry points:

void CM_ClearLevelPatches( void );
struct patchCollide_s	*CM_GeneratePatchCollide( int width, int height, const vector3 *points );
void CM_TraceThroughPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
qboolean CM_PositionTestInPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
void CM_DrawDebugSurface( void (*drawPoly)(int color, int numPoints, vector3 *points) );


WARNING: this may misbehave with meshes that have rows or columns that only
degenerate a few triangles.  Completely degenerate rows and columns are handled
properly.
*/

int	c_totalPatchBlocks;
int	c_totalPatchSurfaces;
int	c_totalPatchEdges;

static const patchCollide_t	*debugPatchCollide;
static const facet_t		*debugFacet;
static qboolean		debugBlock;
static vector3		debugBlockPoints[4];

void CM_ClearLevelPatches( void ) {
	debugPatchCollide = NULL;
	debugFacet = NULL;
}

static int CM_SignbitsForNormal( vector3 *normal ) {
	int	bits, j;

	bits = 0;
	for (j=0 ; j<3 ; j++) {
		if ( normal->data[j] < 0 ) {
			bits |= 1<<j;
		}
	}
	return bits;
}

// Returns false if the triangle is degenrate.
//	The normal will point out of the clock for clockwise ordered points
static qboolean CM_PlaneFromPoints( vector4 *plane, vector3 *a, vector3 *b, vector3 *c ) {
	vector3	d1, d2;

	VectorSubtract( b, a, &d1 );
	VectorSubtract( c, a, &d2 );
	CrossProduct( &d2, &d1, (vector3 *)plane );
	if ( VectorNormalize( (vector3 *)plane ) == 0 ) {
		return qfalse;
	}

	plane->w = DotProduct( a, (vector3 *)plane );
	return qtrue;
}


/*

	GRID SUBDIVISION

*/

// Returns true if the given quadratic curve is not flat enough for our collision detection purposes
static qboolean	CM_NeedsSubdivision( vector3 *a, vector3 *b, vector3 *c ) {
	vector3		cmid;
	vector3		lmid;
	vector3		delta;
	float		dist;
	int			i;

	// calculate the linear midpoint
	for ( i = 0 ; i < 3 ; i++ ) {
		lmid.data[i] = 0.5f*(a->data[i] + c->data[i]);
	}

	// calculate the exact curve midpoint
	for ( i = 0 ; i < 3 ; i++ ) {
		cmid.data[i] = 0.5f * ( 0.5f*(a->data[i] + b->data[i]) + 0.5f*(b->data[i] + c->data[i]) );
	}

	// see if the curve is far enough away from the linear mid
	VectorSubtract( &cmid, &lmid, &delta );
	dist = VectorLength( &delta );
	
	return !!(dist >= SUBDIVIDE_DISTANCE);
}

// a, b, and c are control points.
//	the subdivided sequence will be: a, out1, out2, out3, c
static void CM_Subdivide( vector3 *a, vector3 *b, vector3 *c, vector3 *out1, vector3 *out2, vector3 *out3 ) {
	int		i;

	for ( i = 0 ; i < 3 ; i++ ) {
		out1->data[i] = 0.5f * (a->data[i] + b->data[i]);
		out3->data[i] = 0.5f * (b->data[i] + c->data[i]);
		out2->data[i] = 0.5f * (out1->data[i] + out3->data[i]);
	}
}

// Swaps the rows and columns in place
static void CM_TransposeGrid( cGrid_t *grid ) {
	int			i, j, l;
	vector3		temp;
	qboolean	tempWrap;

	if ( grid->width > grid->height ) {
		for ( i = 0 ; i < grid->height ; i++ ) {
			for ( j = i + 1 ; j < grid->width ; j++ ) {
				if ( j < grid->height ) {
					// swap the value
					VectorCopy( &grid->points[i][j], &temp );
					VectorCopy( &grid->points[j][i], &grid->points[i][j] );
					VectorCopy( &temp, &grid->points[j][i] );
				} else {
					// just copy
					VectorCopy( &grid->points[j][i], &grid->points[i][j] );
				}
			}
		}
	} else {
		for ( i = 0 ; i < grid->width ; i++ ) {
			for ( j = i + 1 ; j < grid->height ; j++ ) {
				if ( j < grid->width ) {
					// swap the value
					VectorCopy( &grid->points[j][i], &temp );
					VectorCopy( &grid->points[i][j], &grid->points[j][i] );
					VectorCopy( &temp, &grid->points[i][j] );
				} else {
					// just copy
					VectorCopy( &grid->points[i][j], &grid->points[j][i] );
				}
			}
		}
	}

	l = grid->width;
	grid->width = grid->height;
	grid->height = l;

	tempWrap = grid->wrapWidth;
	grid->wrapWidth = grid->wrapHeight;
	grid->wrapHeight = tempWrap;
}

// If the left and right columns are exactly equal, set grid->wrapWidth qtrue
static void CM_SetGridWrapWidth( cGrid_t *grid ) {
	int		i, j;
	float	d;

	for ( i = 0 ; i < grid->height ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			d = grid->points[0][i].data[j] - grid->points[grid->width-1][i].data[j];
			if ( d < -WRAP_POINT_EPSILON || d > WRAP_POINT_EPSILON ) {
				break;
			}
		}
		if ( j != 3 ) {
			break;
		}
	}
	if ( i == grid->height ) {
		grid->wrapWidth = qtrue;
	} else {
		grid->wrapWidth = qfalse;
	}
}

// Adds columns as necessary to the grid until all the aproximating points are within SUBDIVIDE_DISTANCE from the true curve
static void CM_SubdivideGridColumns( cGrid_t *grid ) {
	int		i, j, k;

	for ( i = 0 ; i < grid->width - 2 ;  ) {
		// grid->points[i][x] is an interpolating control point
		// grid->points[i+1][x] is an aproximating control point
		// grid->points[i+2][x] is an interpolating control point

		//
		// first see if we can collapse the aproximating collumn away
		//
		for ( j = 0 ; j < grid->height ; j++ ) {
			if ( CM_NeedsSubdivision( &grid->points[i][j], &grid->points[i+1][j], &grid->points[i+2][j] ) ) {
				break;
			}
		}
		if ( j == grid->height ) {
			// all of the points were close enough to the linear midpoints
			// that we can collapse the entire column away
			for ( j = 0 ; j < grid->height ; j++ ) {
				// remove the column
				for ( k = i + 2 ; k < grid->width ; k++ ) {
					VectorCopy( &grid->points[k][j], &grid->points[k-1][j] );
				}
			}

			grid->width--;

			// go to the next curve segment
			i++;
			continue;
		}

		//
		// we need to subdivide the curve
		//
		for ( j = 0 ; j < grid->height ; j++ ) {
			vector3	prev, mid, next;

			// save the control points now
			VectorCopy( &grid->points[i][j], &prev );
			VectorCopy( &grid->points[i+1][j], &mid );
			VectorCopy( &grid->points[i+2][j], &next );

			// make room for two additional columns in the grid
			// columns i+1 will be replaced, column i+2 will become i+4
			// i+1, i+2, and i+3 will be generated
			for ( k = grid->width - 1 ; k > i + 1 ; k-- ) {
				VectorCopy( &grid->points[k][j], &grid->points[k+2][j] );
			}

			// generate the subdivided points
			CM_Subdivide( &prev, &mid, &next, &grid->points[i+1][j], &grid->points[i+2][j], &grid->points[i+3][j] );
		}

		grid->width += 2;

		// the new aproximating point at i+1 may need to be removed
		// or subdivided farther, so don't advance i
	}
}

#define	POINT_EPSILON	0.1f
static qboolean CM_ComparePoints( vector3 *a, vector3 *b ) {
	float d;

	d = a->x - b->x;
	if ( d < -POINT_EPSILON || d > POINT_EPSILON )
		return qfalse;

	d = a->y - b->y;
	if ( d < -POINT_EPSILON || d > POINT_EPSILON )
		return qfalse;

	d = a->z - b->z;
	if ( d < -POINT_EPSILON || d > POINT_EPSILON )
		return qfalse;

	return qtrue;
}

// If there are any identical columns, remove them
static void CM_RemoveDegenerateColumns( cGrid_t *grid ) {
	int		i, j, k;

	for ( i = 0 ; i < grid->width - 1 ; i++ ) {
		for ( j = 0 ; j < grid->height ; j++ ) {
			if ( !CM_ComparePoints( &grid->points[i][j], &grid->points[i+1][j] ) ) {
				break;
			}
		}

		if ( j != grid->height ) {
			continue;	// not degenerate
		}

		for ( j = 0 ; j < grid->height ; j++ ) {
			// remove the column
			for ( k = i + 2 ; k < grid->width ; k++ ) {
				VectorCopy( &grid->points[k][j], &grid->points[k-1][j] );
			}
		}
		grid->width--;

		// check against the next column
		i--;
	}
}

/*

	PATCH COLLIDE GENERATION

*/

static	int				numPlanes;
static	patchPlane_t	planes[MAX_PATCH_PLANES];

static	int				numFacets;
static	facet_t			facets[MAX_PATCH_PLANES]; //maybe MAX_FACETS ??

#define	NORMAL_EPSILON	0.0001f
#define	DIST_EPSILON	0.02f

int CM_PlaneEqual(patchPlane_t *p, vector4 *plane, qboolean *flipped) {
	vector4 invplane;

	if ( fabsf( p->plane.x - plane->x ) < NORMAL_EPSILON &&
		 fabsf( p->plane.y - plane->y ) < NORMAL_EPSILON &&
		 fabsf( p->plane.z - plane->z ) < NORMAL_EPSILON &&
		 fabsf( p->plane.w - plane->w ) < DIST_EPSILON )
	{
		*flipped = qfalse;
		return qtrue;
	}

	VectorNegate((vector3 *)&plane, (vector3 *)&invplane);
	invplane.w = -plane->w;

	if ( fabsf( p->plane.x - invplane.x ) < NORMAL_EPSILON &&
		 fabsf( p->plane.y - invplane.y ) < NORMAL_EPSILON &&
		 fabsf( p->plane.z - invplane.z ) < NORMAL_EPSILON &&
		 fabsf( p->plane.w - invplane.w ) < DIST_EPSILON )
	{
		*flipped = qtrue;
		return qtrue;
	}

	return qfalse;
}

void CM_SnapVector(vector3 *normal) {
	int		i;

	for (i=0 ; i<3 ; i++)
	{
		if ( fabsf(normal->data[i] - 1) < NORMAL_EPSILON )
		{
			VectorClear (normal);
			normal->data[i] = 1;
			break;
		}
		if ( fabsf(normal->data[i] - -1) < NORMAL_EPSILON )
		{
			VectorClear (normal);
			normal->data[i] = -1;
			break;
		}
	}
}

int CM_FindPlane2(vector4 *plane, qboolean *flipped) {
	int i;

	// see if the points are close enough to an existing plane
	for ( i = 0 ; i < numPlanes ; i++ ) {
		if (CM_PlaneEqual(&planes[i], plane, flipped)) return i;
	}

	// add a new plane
	if ( numPlanes == MAX_PATCH_PLANES ) {
		Com_Error( ERR_DROP, "MAX_PATCH_PLANES" );
	}

	VectorCopy4( plane, &planes[numPlanes].plane );
	planes[numPlanes].signbits = CM_SignbitsForNormal( (vector3 *)plane );

	numPlanes++;

	*flipped = qfalse;

	return numPlanes-1;
}

static int CM_FindPlane( vector3 *p1, vector3 *p2, vector3 *p3 ) {
	vector4 plane;
	int		i;
	float	d;

	if ( !CM_PlaneFromPoints( &plane, p1, p2, p3 ) )
		return -1;

	// see if the points are close enough to an existing plane
	for ( i = 0 ; i < numPlanes ; i++ ) {
		if ( DotProduct( (vector3 *)&plane, (vector3 *)&planes[i].plane ) < 0 ) {
			continue;	// allow backwards planes?
		}

		d = DotProduct( p1, (vector3 *)&planes[i].plane ) - planes[i].plane.w;
		if ( d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON )
			continue;

		d = DotProduct( p2, (vector3 *)&planes[i].plane ) - planes[i].plane.w;
		if ( d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON )
			continue;

		d = DotProduct( p3, (vector3 *)&planes[i].plane ) - planes[i].plane.w;
		if ( d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON )
			continue;

		// found it
		return i;
	}

	// add a new plane
	if ( numPlanes == MAX_PATCH_PLANES ) {
		Com_Error( ERR_DROP, "MAX_PATCH_PLANES" );
	}

	VectorCopy4( &plane, &planes[numPlanes].plane );
	planes[numPlanes].signbits = CM_SignbitsForNormal( (vector3 *)&plane );

	return numPlanes++; // post increment
}

static int CM_PointOnPlaneSide( vector3 *p, int planeNum ) {
	vector4	*plane;
	float	d;

	if ( planeNum == -1 ) {
		return SIDE_ON;
	}
	plane = &planes[ planeNum ].plane;

	d = DotProduct( p, (vector3 *)plane ) - plane->w;

	if ( d > PLANE_TRI_EPSILON ) {
		return SIDE_FRONT;
	}

	if ( d < -PLANE_TRI_EPSILON ) {
		return SIDE_BACK;
	}

	return SIDE_ON;
}

static int	CM_GridPlane( int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], int i, int j, int tri ) {
	int		p;

	p = gridPlanes[i][j][tri];
	if ( p != -1 )
		return p;

	p = gridPlanes[i][j][!tri];
	if ( p != -1 )
		return p;

	// should never happen
	Com_Printf( "WARNING: CM_GridPlane unresolvable\n" );
	return -1;
}

static int CM_EdgePlaneNum( cGrid_t *grid, int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], int i, int j, int k ) {
	vector3	*p1, *p2;
	vector3		up;
	int			p;

	switch ( k ) {
	case 0:	// top border
		p1 = &grid->points[i][j];
		p2 = &grid->points[i+1][j];
		p = CM_GridPlane( gridPlanes, i, j, 0 );
		VectorMA( p1, 4, (vector3 *)&planes[ p ].plane, &up );
		return CM_FindPlane( p1, p2, &up );

	case 2:	// bottom border
		p1 = &grid->points[i][j+1];
		p2 = &grid->points[i+1][j+1];
		p = CM_GridPlane( gridPlanes, i, j, 1 );
		VectorMA( p1, 4, (vector3 *)&planes[ p ].plane, &up );
		return CM_FindPlane( p2, p1, &up );

	case 3: // left border
		p1 = &grid->points[i][j];
		p2 = &grid->points[i][j+1];
		p = CM_GridPlane( gridPlanes, i, j, 1 );
		VectorMA( p1, 4, (vector3 *)&planes[ p ].plane, &up );
		return CM_FindPlane( p2, p1, &up );

	case 1:	// right border
		p1 = &grid->points[i+1][j];
		p2 = &grid->points[i+1][j+1];
		p = CM_GridPlane( gridPlanes, i, j, 0 );
		VectorMA( p1, 4, (vector3 *)&planes[ p ].plane, &up );
		return CM_FindPlane( p1, p2, &up );

	case 4:	// diagonal out of triangle 0
		p1 = &grid->points[i+1][j+1];
		p2 = &grid->points[i][j];
		p = CM_GridPlane( gridPlanes, i, j, 0 );
		VectorMA( p1, 4, (vector3 *)&planes[ p ].plane, &up );
		return CM_FindPlane( p1, p2, &up );

	case 5:	// diagonal out of triangle 1
		p1 = &grid->points[i][j];
		p2 = &grid->points[i+1][j+1];
		p = CM_GridPlane( gridPlanes, i, j, 1 );
		VectorMA( p1, 4, (vector3 *)&planes[ p ].plane, &up );
		return CM_FindPlane( p1, p2, &up );

	}

	Com_Error( ERR_DROP, "CM_EdgePlaneNum: bad k" );
	return -1;
}

static void CM_SetBorderInward( facet_t *facet, cGrid_t *grid, int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2],
						  int i, int j, int which ) {
	int		k, l;
	vector3	*points[4];
	int		numPoints;

	switch ( which ) {
	case -1:
		points[0] = &grid->points[i][j];
		points[1] = &grid->points[i+1][j];
		points[2] = &grid->points[i+1][j+1];
		points[3] = &grid->points[i][j+1];
		numPoints = 4;
		break;
	case 0:
		points[0] = &grid->points[i][j];
		points[1] = &grid->points[i+1][j];
		points[2] = &grid->points[i+1][j+1];
		numPoints = 3;
		break;
	case 1:
		points[0] = &grid->points[i+1][j+1];
		points[1] = &grid->points[i][j+1];
		points[2] = &grid->points[i][j];
		numPoints = 3;
		break;
	default:
		Com_Error( ERR_FATAL, "CM_SetBorderInward: bad parameter" );
		numPoints = 0;
		break;
	}

	for ( k = 0 ; k < facet->numBorders ; k++ ) {
		int		front, back;

		front = 0;
		back = 0;

		for ( l = 0 ; l < numPoints ; l++ ) {
			int		side;

			side = CM_PointOnPlaneSide( points[l], facet->borderPlanes[k] );
			if ( side == SIDE_FRONT ) {
				front++;
			} if ( side == SIDE_BACK ) {
				back++;
			}
		}

		if ( front && !back ) {
			facet->borderInward[k] = qtrue;
		} else if ( back && !front ) {
			facet->borderInward[k] = qfalse;
		} else if ( !front && !back ) {
			// flat side border
			facet->borderPlanes[k] = -1;
		} else {
			// bisecting side border
			Com_DPrintf( "WARNING: CM_SetBorderInward: mixed plane sides\n" );
			facet->borderInward[k] = qfalse;
			if ( !debugBlock ) {
				debugBlock = qtrue;
				VectorCopy( &grid->points[i][j], &debugBlockPoints[0] );
				VectorCopy( &grid->points[i+1][j], &debugBlockPoints[1] );
				VectorCopy( &grid->points[i+1][j+1], &debugBlockPoints[2] );
				VectorCopy( &grid->points[i][j+1], &debugBlockPoints[3] );
			}
		}
	}
}

// If the facet isn't bounded by its borders, we screwed up.
static qboolean CM_ValidateFacet( facet_t *facet ) {
	vector4		plane;
	int			j;
	winding_t	*w;
	vector3		bounds[2];

	if ( facet->surfacePlane == -1 ) {
		return qfalse;
	}

	VectorCopy4( &planes[ facet->surfacePlane ].plane, &plane );
	w = BaseWindingForPlane( (vector3 *)&plane,  plane.w );
	for ( j = 0 ; j < facet->numBorders && w ; j++ ) {
		if ( facet->borderPlanes[j] == -1 ) {
			FreeWinding( w );
			return qfalse;
		}
		VectorCopy4( &planes[ facet->borderPlanes[j] ].plane, &plane );
		if ( !facet->borderInward[j] ) {
			VectorSubtract( &vec3_origin, (vector3 *)&plane, (vector3 *)&plane );
			plane.w = -plane.w;
		}
		ChopWindingInPlace( &w, (vector3 *)&plane, plane.w, 0.1f );
	}

	if ( !w ) {
		return qfalse;		// winding was completely chopped away
	}

	// see if the facet is unreasonably large
	WindingBounds( w, &bounds[0], &bounds[1] );
	FreeWinding( w );
	
	for ( j = 0 ; j < 3 ; j++ ) {
		if ( bounds[1].data[j] - bounds[0].data[j] > MAX_MAP_BOUNDS )
			return qfalse;		// we must be missing a plane
		if ( bounds[0].data[j] >=  MAX_MAP_BOUNDS )		return qfalse;
		if ( bounds[1].data[j] <= -MAX_MAP_BOUNDS )		return qfalse;
	}
	return qtrue;		// winding is fine
}

void CM_AddFacetBevels( facet_t *facet ) {

	int i, j, k, l;
	int axis, dir, order;
	qboolean flipped;
	vector4 plane, newplane;
	float d;
	winding_t *w, *w2;
	vector3 mins, maxs, vec, vec2;

	VectorCopy4( &planes[ facet->surfacePlane ].plane, &plane );

	w = BaseWindingForPlane( (vector3 *)&plane,  plane.w );
	for ( j = 0 ; j < facet->numBorders && w ; j++ ) {
		if (facet->borderPlanes[j] == facet->surfacePlane) continue;
		VectorCopy4( &planes[ facet->borderPlanes[j] ].plane, &plane );

		if ( !facet->borderInward[j] ) {
			VectorSubtract( &vec3_origin, (vector3 *)&plane, (vector3 *)&plane );
			plane.w = -plane.w;
		}

		ChopWindingInPlace( &w, (vector3 *)&plane, plane.a, 0.1f );
	}
	if ( !w ) {
		return;
	}

	WindingBounds(w, &mins, &maxs);

	// add the axial planes
	order = 0;
	for ( axis = 0 ; axis < 3 ; axis++ )
	{
		for ( dir = -1 ; dir <= 1 ; dir += 2, order++ )
		{
			VectorClear((vector3 *)&plane);
			plane.data[axis] = (float)dir;
			if (dir == 1)	plane.w =  maxs.data[axis];
			else			plane.w = -mins.data[axis];
			//if it's the surface plane
			if (CM_PlaneEqual(&planes[facet->surfacePlane], &plane, &flipped)) {
				continue;
			}
			// see if the plane is allready present
			for ( i = 0 ; i < facet->numBorders ; i++ ) {
				if (CM_PlaneEqual(&planes[facet->borderPlanes[i]], &plane, &flipped))
					break;
			}

			if ( i == facet->numBorders ) {
				if (facet->numBorders > 4 + 6 + 16) Com_Printf("ERROR: too many bevels\n");
				facet->borderPlanes[facet->numBorders] = CM_FindPlane2(&plane, &flipped);
				facet->borderNoAdjust[facet->numBorders] = qfalse;
				facet->borderInward[facet->numBorders] = flipped;
				facet->numBorders++;
			}
		}
	}
	//
	// add the edge bevels
	//
	// test the non-axial plane edges
	for ( j = 0 ; j < w->numpoints ; j++ )
	{
		k = (j+1)%w->numpoints;
		VectorSubtract (&w->p[j], &w->p[k], &vec);
		//if it's a degenerate edge
		if (VectorNormalize (&vec) < 0.5f)
			continue;
		CM_SnapVector(&vec);
		for ( k = 0; k < 3 ; k++ )
			if ( vec.data[k] == -1 || vec.data[k] == 1 )
				break;	// axial
		if ( k < 3 )
			continue;	// only test non-axial edges

		// try the six possible slanted axials from this edge
		for ( axis = 0 ; axis < 3 ; axis++ )
		{
			for ( dir = -1 ; dir <= 1 ; dir += 2 )
			{
				// construct a plane
				VectorClear (&vec2);
				vec2.data[axis] = (float)dir;
				CrossProduct (&vec, &vec2, (vector3 *)&plane);
				if (VectorNormalize ((vector3 *)&plane) < 0.5f)
					continue;
				plane.w = DotProduct (&w->p[j], (vector3 *)&plane);

				// if all the points of the facet winding are
				// behind this plane, it is a proper edge bevel
				for ( l = 0 ; l < w->numpoints ; l++ )
				{
					d = DotProduct (&w->p[l], (vector3 *)&plane) - plane.w;
					if (d > POINT_EPSILON)
						break;	// point in front
				}
				if ( l < w->numpoints )
					continue;

				//if it's the surface plane
				if (CM_PlaneEqual(&planes[facet->surfacePlane], &plane, &flipped)) {
					continue;
				}
				// see if the plane is allready present
				for ( i = 0 ; i < facet->numBorders ; i++ ) {
					if (CM_PlaneEqual(&planes[facet->borderPlanes[i]], &plane, &flipped)) {
							break;
					}
				}

				if ( i == facet->numBorders ) {
					if (facet->numBorders > 4 + 6 + 16) Com_Printf("ERROR: too many bevels\n");
					facet->borderPlanes[facet->numBorders] = CM_FindPlane2(&plane, &flipped);

					for ( k = 0 ; k < facet->numBorders ; k++ ) {
						if (facet->borderPlanes[facet->numBorders] ==
							facet->borderPlanes[k]) Com_Printf("WARNING: bevel plane already used\n");
					}

					facet->borderNoAdjust[facet->numBorders] = qfalse;
					facet->borderInward[facet->numBorders] = flipped;
					//
					w2 = CopyWinding(w);
					VectorCopy4(&planes[facet->borderPlanes[facet->numBorders]].plane, &newplane);
					if (!facet->borderInward[facet->numBorders])
					{
						VectorNegate((vector3 *)&newplane, (vector3 *)&newplane);
						newplane.w = -newplane.w;
					} //end if
					ChopWindingInPlace( &w2, (vector3 *)&newplane, newplane.w, 0.1f );
					if (!w2) {
						Com_DPrintf("WARNING: CM_AddFacetBevels... invalid bevel\n");
						continue;
					}
					else {
						FreeWinding(w2);
					}
					//
					facet->numBorders++;
					//already got a bevel
//					break;
				}
			}
		}
	}
	FreeWinding( w );

#ifndef BSPC
	//add opposite plane
	facet->borderPlanes[facet->numBorders] = facet->surfacePlane;
	facet->borderNoAdjust[facet->numBorders] = 0;
	facet->borderInward[facet->numBorders] = qtrue;
	facet->numBorders++;
#endif //BSPC

}

typedef enum edgeName_e {
	EN_TOP=0,
	EN_RIGHT,
	EN_BOTTOM,
	EN_LEFT
} edgeName_t;

static void CM_PatchCollideFromGrid( cGrid_t *grid, patchCollide_t *pf ) {
	int				i, j;
	vector3			*p1, *p2, *p3;
	int				gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2];
	facet_t			*facet;
	int				borders[4];
	qboolean		noAdjust[4];

	numPlanes = 0;
	numFacets = 0;

	// find the planes for each triangle of the grid
	for ( i = 0 ; i < grid->width - 1 ; i++ ) {
		for ( j = 0 ; j < grid->height - 1 ; j++ ) {
			p1 = &grid->points[i][j];
			p2 = &grid->points[i+1][j];
			p3 = &grid->points[i+1][j+1];
			gridPlanes[i][j][0] = CM_FindPlane( p1, p2, p3 );

			p1 = &grid->points[i+1][j+1];
			p2 = &grid->points[i][j+1];
			p3 = &grid->points[i][j];
			gridPlanes[i][j][1] = CM_FindPlane( p1, p2, p3 );
		}
	}

	// create the borders for each facet
	for ( i = 0 ; i < grid->width - 1 ; i++ ) {
		for ( j = 0 ; j < grid->height - 1 ; j++ ) {
			 
			borders[EN_TOP] = -1;
			if ( j > 0 ) {
				borders[EN_TOP] = gridPlanes[i][j-1][1];
			} else if ( grid->wrapHeight ) {
				borders[EN_TOP] = gridPlanes[i][grid->height-2][1];
			} 
			noAdjust[EN_TOP] = ( borders[EN_TOP] == gridPlanes[i][j][0] );
			if ( borders[EN_TOP] == -1 || noAdjust[EN_TOP] ) {
				borders[EN_TOP] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 0 );
			}

			borders[EN_BOTTOM] = -1;
			if ( j < grid->height - 2 ) {
				borders[EN_BOTTOM] = gridPlanes[i][j+1][0];
			} else if ( grid->wrapHeight ) {
				borders[EN_BOTTOM] = gridPlanes[i][0][0];
			}
			noAdjust[EN_BOTTOM] = ( borders[EN_BOTTOM] == gridPlanes[i][j][1] );
			if ( borders[EN_BOTTOM] == -1 || noAdjust[EN_BOTTOM] ) {
				borders[EN_BOTTOM] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 2 );
			}

			borders[EN_LEFT] = -1;
			if ( i > 0 ) {
				borders[EN_LEFT] = gridPlanes[i-1][j][0];
			} else if ( grid->wrapWidth ) {
				borders[EN_LEFT] = gridPlanes[grid->width-2][j][0];
			}
			noAdjust[EN_LEFT] = ( borders[EN_LEFT] == gridPlanes[i][j][1] );
			if ( borders[EN_LEFT] == -1 || noAdjust[EN_LEFT] ) {
				borders[EN_LEFT] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 3 );
			}

			borders[EN_RIGHT] = -1;
			if ( i < grid->width - 2 ) {
				borders[EN_RIGHT] = gridPlanes[i+1][j][1];
			} else if ( grid->wrapWidth ) {
				borders[EN_RIGHT] = gridPlanes[0][j][1];
			}
			noAdjust[EN_RIGHT] = ( borders[EN_RIGHT] == gridPlanes[i][j][0] );
			if ( borders[EN_RIGHT] == -1 || noAdjust[EN_RIGHT] ) {
				borders[EN_RIGHT] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 1 );
			}

			if ( numFacets == MAX_FACETS ) {
				Com_Error( ERR_DROP, "MAX_FACETS" );
			}
			facet = &facets[numFacets];
			memset( facet, 0, sizeof( *facet ) );

			if ( gridPlanes[i][j][0] == gridPlanes[i][j][1] ) {
				if ( gridPlanes[i][j][0] == -1 ) {
					continue;		// degenrate
				}
				facet->surfacePlane = gridPlanes[i][j][0];
				facet->numBorders = 4;
				facet->borderPlanes[0] = borders[EN_TOP];
				facet->borderNoAdjust[0] = noAdjust[EN_TOP];
				facet->borderPlanes[1] = borders[EN_RIGHT];
				facet->borderNoAdjust[1] = noAdjust[EN_RIGHT];
				facet->borderPlanes[2] = borders[EN_BOTTOM];
				facet->borderNoAdjust[2] = noAdjust[EN_BOTTOM];
				facet->borderPlanes[3] = borders[EN_LEFT];
				facet->borderNoAdjust[3] = noAdjust[EN_LEFT];
				CM_SetBorderInward( facet, grid, gridPlanes, i, j, -1 );
				if ( CM_ValidateFacet( facet ) ) {
					CM_AddFacetBevels( facet );
					numFacets++;
				}
			} else {
				// two seperate triangles
				facet->surfacePlane = gridPlanes[i][j][0];
				facet->numBorders = 3;
				facet->borderPlanes[0] = borders[EN_TOP];
				facet->borderNoAdjust[0] = noAdjust[EN_TOP];
				facet->borderPlanes[1] = borders[EN_RIGHT];
				facet->borderNoAdjust[1] = noAdjust[EN_RIGHT];
				facet->borderPlanes[2] = gridPlanes[i][j][1];
				if ( facet->borderPlanes[2] == -1 ) {
					facet->borderPlanes[2] = borders[EN_BOTTOM];
					if ( facet->borderPlanes[2] == -1 ) {
						facet->borderPlanes[2] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 4 );
					}
				}
 				CM_SetBorderInward( facet, grid, gridPlanes, i, j, 0 );
				if ( CM_ValidateFacet( facet ) ) {
					CM_AddFacetBevels( facet );
					numFacets++;
				}

				if ( numFacets == MAX_FACETS ) {
					Com_Error( ERR_DROP, "MAX_FACETS" );
				}
				facet = &facets[numFacets];
				memset( facet, 0, sizeof( *facet ) );

				facet->surfacePlane = gridPlanes[i][j][1];
				facet->numBorders = 3;
				facet->borderPlanes[0] = borders[EN_BOTTOM];
				facet->borderNoAdjust[0] = noAdjust[EN_BOTTOM];
				facet->borderPlanes[1] = borders[EN_LEFT];
				facet->borderNoAdjust[1] = noAdjust[EN_LEFT];
				facet->borderPlanes[2] = gridPlanes[i][j][0];
				if ( facet->borderPlanes[2] == -1 ) {
					facet->borderPlanes[2] = borders[EN_TOP];
					if ( facet->borderPlanes[2] == -1 ) {
						facet->borderPlanes[2] = CM_EdgePlaneNum( grid, gridPlanes, i, j, 5 );
					}
				}
				CM_SetBorderInward( facet, grid, gridPlanes, i, j, 1 );
				if ( CM_ValidateFacet( facet ) ) {
					CM_AddFacetBevels( facet );
					numFacets++;
				}
			}
		}
	}

	// copy the results out
	pf->numPlanes = numPlanes;
	pf->numFacets = numFacets;
	pf->facets = (facet_t *)Hunk_Alloc( numFacets * sizeof( *pf->facets ), PREF_HIGH );
	memcpy( pf->facets, facets, numFacets * sizeof( *pf->facets ) );
	pf->planes = (patchPlane_t *)Hunk_Alloc( numPlanes * sizeof( *pf->planes ), PREF_HIGH );
	memcpy( pf->planes, planes, numPlanes * sizeof( *pf->planes ) );
}


// Creates an internal structure that will be used to perform collision detection with a patch mesh.
//	Points is packed as concatenated rows.
struct patchCollide_s	*CM_GeneratePatchCollide( int width, int height, vector3 *points ) {
	patchCollide_t	*pf;
	cGrid_t			grid;
	int				i, j;

	if ( width <= 2 || height <= 2 || !points ) {
		Com_Error( ERR_DROP, "CM_GeneratePatchFacets: bad parameters: (%i, %i, %p)",
			width, height, (void *)points );
	}

	if ( !(width & 1) || !(height & 1) ) {
		Com_Error( ERR_DROP, "CM_GeneratePatchFacets: even sizes are invalid for quadratic meshes" );
	}

	if ( width > MAX_GRID_SIZE || height > MAX_GRID_SIZE ) {
		Com_Error( ERR_DROP, "CM_GeneratePatchFacets: source is > MAX_GRID_SIZE" );
	}

	// build a grid
	grid.width = width;
	grid.height = height;
	grid.wrapWidth = qfalse;
	grid.wrapHeight = qfalse;
	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height ; j++ ) {
			VectorCopy( &points[j*width + i], &grid.points[i][j] );
		}
	}

	// subdivide the grid
	CM_SetGridWrapWidth( &grid );
	CM_SubdivideGridColumns( &grid );
	CM_RemoveDegenerateColumns( &grid );

	CM_TransposeGrid( &grid );

	CM_SetGridWrapWidth( &grid );
	CM_SubdivideGridColumns( &grid );
	CM_RemoveDegenerateColumns( &grid );

	// we now have a grid of points exactly on the curve
	// the approximate surface defined by these points will be
	// collided against
	pf = (patchCollide_t *)Hunk_Alloc( sizeof( *pf ), PREF_HIGH );
	ClearBounds( &pf->bounds[0], &pf->bounds[1] );
	for ( i = 0 ; i < grid.width ; i++ ) {
		for ( j = 0 ; j < grid.height ; j++ ) {
			AddPointToBounds( &grid.points[i][j], &pf->bounds[0], &pf->bounds[1] );
		}
	}

	c_totalPatchBlocks += ( grid.width - 1 ) * ( grid.height - 1 );

	// generate a bsp tree for the surface
	CM_PatchCollideFromGrid( &grid, pf );

	// expand by one unit for epsilon purposes
	pf->bounds[0].x -= 1;
	pf->bounds[0].y -= 1;
	pf->bounds[0].z -= 1;

	pf->bounds[1].x += 1;
	pf->bounds[1].y += 1;
	pf->bounds[1].z += 1;

	return pf;
}

/*

	TRACE TESTING

*/

// special case for point traces because the patch collide "brushes" have no volume
void CM_TracePointThroughPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc ) {
	qboolean	frontFacing[MAX_PATCH_PLANES];
	float		intersection[MAX_PATCH_PLANES];
	float		intersect;
	const patchPlane_t	*planes;
	const facet_t	*facet;
	int			i, j, k;
	float		offset;
	float		d1, d2;
#ifndef BSPC
	static cvar_t *cv;
#endif //BSPC

#ifndef BSPC
	if ( !cm_playerCurveClip->integer || !tw->isPoint ) {
		return;
	}
#endif

	// determine the trace's relationship to all planes
	planes = pc->planes;
	for ( i = 0 ; i < pc->numPlanes ; i++, planes++ ) {
		offset = DotProduct( &tw->offsets[ planes->signbits ], (vector3 *)&planes->plane );
		d1 = DotProduct( &tw->start, (vector3 *)&planes->plane ) - planes->plane.w + offset;
		d2 = DotProduct( &tw->end, (vector3 *)&planes->plane ) - planes->plane.w + offset;
		if ( d1 <= 0 ) {
			frontFacing[i] = qfalse;
		} else {
			frontFacing[i] = qtrue;
		}
		if ( d1 == d2 ) {
			intersection[i] = 99999;
		} else {
			intersection[i] = d1 / ( d1 - d2 );
			if ( intersection[i] <= 0 ) {
				intersection[i] = 99999;
			}
		}
	}


	// see if any of the surface planes are intersected
	facet = pc->facets;
	for ( i = 0 ; i < pc->numFacets ; i++, facet++ ) {
		if ( !frontFacing[facet->surfacePlane] ) {
			continue;
		}
		intersect = intersection[facet->surfacePlane];
		if ( intersect < 0 ) {
			continue;		// surface is behind the starting point
		}
		if ( intersect > tw->trace.fraction ) {
			continue;		// already hit something closer
		}
		for ( j = 0 ; j < facet->numBorders ; j++ ) {
			k = facet->borderPlanes[j];
			if ( frontFacing[k] ^ facet->borderInward[j] ) {
				if ( intersection[k] > intersect ) {
					break;
				}
			} else {
				if ( intersection[k] < intersect ) {
					break;
				}
			}
		}
		if ( j == facet->numBorders ) {
			// we hit this facet
#ifndef BSPC
			if (!cv) {
				cv = Cvar_Get( "r_debugSurfaceUpdate", "1", 0, NULL, NULL );
			}
			if (cv->integer) {
				debugPatchCollide = pc;
				debugFacet = facet;
			}
#endif //BSPC
			planes = &pc->planes[facet->surfacePlane];

			// calculate intersection with a slight pushoff
			offset = DotProduct( &tw->offsets[ planes->signbits ], (vector3 *)&planes->plane );
			d1 = DotProduct( &tw->start, (vector3 *)&planes->plane ) - planes->plane.w + offset;
			d2 = DotProduct( &tw->end, (vector3 *)&planes->plane ) - planes->plane.w + offset;
			tw->trace.fraction = ( d1 - SURFACE_CLIP_EPSILON ) / ( d1 - d2 );

			if ( tw->trace.fraction < 0 ) {
				tw->trace.fraction = 0;
			}

			VectorCopy( (vector3 *)&planes->plane,  &tw->trace.plane.normal );
			tw->trace.plane.dist = planes->plane.w;
		}
	}
}

int CM_CheckFacetPlane(vector4 *plane, vector3 *start, vector3 *end, float *enterFrac, float *leaveFrac, int *hit) {
	float d1, d2, f;

	*hit = qfalse;

	d1 = DotProduct( start, (vector3 *)plane ) - plane->w;
	d2 = DotProduct( end, (vector3 *)plane ) - plane->w;

	// if completely in front of face, no intersection with the entire facet
	if (d1 > 0 && ( d2 >= SURFACE_CLIP_EPSILON || d2 >= d1 )  ) {
		return qfalse;
	}

	// if it doesn't cross the plane, the plane isn't relevent
	if (d1 <= 0 && d2 <= 0 ) {
		return qtrue;
	}

	// crosses face
	if (d1 > d2) {	// enter
		f = (d1-SURFACE_CLIP_EPSILON) / (d1-d2);
		if ( f < 0 ) {
			f = 0;
		}
		//always favor previous plane hits and thus also the surface plane hit
		if (f > *enterFrac) {
			*enterFrac = f;
			*hit = qtrue;
		}
	} else {	// leave
		f = (d1+SURFACE_CLIP_EPSILON) / (d1-d2);
		if ( f > 1 ) {
			f = 1;
		}
		if (f < *leaveFrac) {
			*leaveFrac = f;
		}
	}
	return qtrue;
}

void CM_TraceThroughPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc ) {
	int i, j, hit, hitnum;
	float offset, enterFrac, leaveFrac, t;
	patchPlane_t *planes;
	facet_t	*facet;
	vector4 plane = {0, 0, 0, 0}, bestplane = {0, 0, 0, 0};
	vector3 startp, endp;
#ifndef BSPC
	static cvar_t *cv;
#endif //BSPC

	if ( !CM_BoundsIntersect( &tw->bounds[0], &tw->bounds[1], &pc->bounds[0], &pc->bounds[1] ) ) {
		return;
	}

	if (tw->isPoint) {
		CM_TracePointThroughPatchCollide( tw, pc );
		return;
	}

	facet = pc->facets;
	for ( i = 0 ; i < pc->numFacets ; i++, facet++ ) {
		enterFrac = -1.0f;
		leaveFrac = 1.0f;
		hitnum = -1;
		//
		planes = &pc->planes[ facet->surfacePlane ];
		VectorCopy((vector3 *)&planes->plane, (vector3 *)&plane);
		plane.w = planes->plane.w;
		if ( tw->sphere.use ) {
			// adjust the plane distance appropriately for radius
			plane.w += tw->sphere.radius;

			// find the closest point on the capsule to the plane
			t = DotProduct( (vector3 *)&plane, &tw->sphere.offset );
			if ( t > 0.0f ) {
				VectorSubtract( &tw->start, &tw->sphere.offset, &startp );
				VectorSubtract( &tw->end, &tw->sphere.offset, &endp );
			}
			else {
				VectorAdd( &tw->start, &tw->sphere.offset, &startp );
				VectorAdd( &tw->end, &tw->sphere.offset, &endp );
			}
		}
		else {
			offset = DotProduct( &tw->offsets[ planes->signbits ], (vector3 *)&plane);
			plane.w -= offset;
			VectorCopy( &tw->start, &startp );
			VectorCopy( &tw->end, &endp );
		}

		if (!CM_CheckFacetPlane(&plane, &startp, &endp, &enterFrac, &leaveFrac, &hit)) {
			continue;
		}
		if (hit) {
			VectorCopy4(&plane, &bestplane);
		}

		for ( j = 0; j < facet->numBorders; j++ ) {
			planes = &pc->planes[ facet->borderPlanes[j] ];
			if (facet->borderInward[j]) {
				VectorNegate((vector3 *)&planes->plane, (vector3 *)&plane);
				plane.w = -planes->plane.w;
			}
			else {
				VectorCopy((vector3 *)&planes->plane, (vector3 *)&plane);
				plane.w = planes->plane.w;
			}
			if ( tw->sphere.use ) {
				// adjust the plane distance appropriately for radius
				plane.w += tw->sphere.radius;

				// find the closest point on the capsule to the plane
				t = DotProduct( (vector3 *)&plane, &tw->sphere.offset );
				if ( t > 0.0f ) {
					VectorSubtract( &tw->start, &tw->sphere.offset, &startp );
					VectorSubtract( &tw->end, &tw->sphere.offset, &endp );
				}
				else {
					VectorAdd( &tw->start, &tw->sphere.offset, &startp );
					VectorAdd( &tw->end, &tw->sphere.offset, &endp );
				}
			}
			else {
				// NOTE: this works even though the plane might be flipped because the bbox is centered
				offset = DotProduct( &tw->offsets[ planes->signbits ], (vector3 *)&plane);
				plane.z += fabsf(offset);
				VectorCopy( &tw->start, &startp );
				VectorCopy( &tw->end, &endp );
			}

			if (!CM_CheckFacetPlane(&plane, &startp, &endp, &enterFrac, &leaveFrac, &hit)) {
				break;
			}
			if (hit) {
				hitnum = j;
				VectorCopy4(&plane, &bestplane);
			}
		}
		if (j < facet->numBorders) continue;
		//never clip against the back side
		if (hitnum == facet->numBorders - 1) continue;

		if (enterFrac < leaveFrac && enterFrac >= 0) {
			if (enterFrac < tw->trace.fraction) {
				if (enterFrac < 0) {
					enterFrac = 0;
				}
#ifndef BSPC
				if (!cv) {
					cv = Cvar_Get( "r_debugSurfaceUpdate", "1", 0, NULL, NULL );
				}
				if (cv && cv->integer) {
					debugPatchCollide = pc;
					debugFacet = facet;
				}
#endif //BSPC

				tw->trace.fraction = enterFrac;
				VectorCopy( (vector3 *)&bestplane, &tw->trace.plane.normal );
				tw->trace.plane.dist = bestplane.w;
			}
		}
	}
}


/*

	POSITION TEST

*/

qboolean CM_PositionTestInPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc ) {
	int i, j;
	float offset, t;
	patchPlane_t *planes;
	facet_t	*facet;
	vector4 plane;
	vector3 startp;

	if (tw->isPoint) {
		return qfalse;
	}
	//
	facet = pc->facets;
	for ( i = 0 ; i < pc->numFacets ; i++, facet++ ) {
		planes = &pc->planes[ facet->surfacePlane ];
		VectorCopy((vector3 *)&planes->plane, (vector3 *)&plane);
		plane.w = planes->plane.w;
		if ( tw->sphere.use ) {
			// adjust the plane distance appropriately for radius
			plane.z += tw->sphere.radius;

			// find the closest point on the capsule to the plane
			t = DotProduct( (vector3 *)&plane, &tw->sphere.offset );
			if ( t > 0 ) {
				VectorSubtract( &tw->start, &tw->sphere.offset, &startp );
			}
			else {
				VectorAdd( &tw->start, &tw->sphere.offset, &startp );
			}
		}
		else {
			offset = DotProduct( &tw->offsets[ planes->signbits ], (vector3 *)&plane);
			plane.w -= offset;
			VectorCopy( &tw->start, &startp );
		}

		if ( DotProduct( (vector3 *)&plane, &startp ) - plane.w > 0.0f ) {
			continue;
		}

		for ( j = 0; j < facet->numBorders; j++ ) {
			planes = &pc->planes[ facet->borderPlanes[j] ];
			if (facet->borderInward[j]) {
				VectorNegate((vector3 *)&planes->plane, (vector3 *)&plane);
				plane.w = -planes->plane.w;
			}
			else {
				VectorCopy((vector3 *)&planes->plane, (vector3 *)&plane);
				plane.w = planes->plane.w;
			}
			if ( tw->sphere.use ) {
				// adjust the plane distance appropriately for radius
				plane.w += tw->sphere.radius;

				// find the closest point on the capsule to the plane
				t = DotProduct( (vector3 *)&plane, &tw->sphere.offset );
				if ( t > 0.0f ) {
					VectorSubtract( &tw->start, &tw->sphere.offset, &startp );
				}
				else {
					VectorAdd( &tw->start, &tw->sphere.offset, &startp );
				}
			}
			else {
				// NOTE: this works even though the plane might be flipped because the bbox is centered
				offset = DotProduct( &tw->offsets[ planes->signbits ], (vector3 *)&plane);
				plane.w += fabsf(offset);
				VectorCopy( &tw->start, &startp );
			}

			if ( DotProduct( (vector3 *)&plane, &startp ) - plane.w > 0.0f ) {
				break;
			}
		}
		if (j < facet->numBorders) {
			continue;
		}
		// inside this patch facet
		return qtrue;
	}
	return qfalse;
}

/*

	DEBUGGING

*/


// Called from the renderer
void CM_DrawDebugSurface( void (*drawPoly)(int color, int numPoints, vector3 *points) ) {
	static cvar_t	*cv;
#ifndef BSPC
	static cvar_t	*cv2;
#endif
	const patchCollide_t	*pc;
	facet_t			*facet;
	winding_t		*w;
	int				i, j, k, n;
	int				curplanenum, planenum, curinward, inward;
	vector4			plane;
	vector3 mins = {-15, -15, -28}, maxs = {15, 15, 28};
	//vector3 mins = {0, 0, 0}, maxs = {0, 0, 0};
	vector3 v1, v2;

#ifndef BSPC
	if ( !cv2 )
	{
		cv2 = Cvar_Get( "r_debugSurface", "0", 0, NULL, NULL );
	}

	if (cv2->integer != 1)
	{
		SV_BotDrawDebugPolygons(drawPoly, cv2->integer);
		return;
	}
#endif

	if ( !debugPatchCollide ) {
		return;
	}

#ifndef BSPC
	if ( !cv ) {
		cv = Cvar_Get( "cm_debugSize", "2", 0, NULL, NULL );
	}
#endif
	pc = debugPatchCollide;

	for ( i = 0, facet = pc->facets ; i < pc->numFacets ; i++, facet++ ) {

		for ( k = 0 ; k < facet->numBorders + 1; k++ ) {
			//
			if (k < facet->numBorders) {
				planenum = facet->borderPlanes[k];
				inward = facet->borderInward[k];
			}
			else {
				planenum = facet->surfacePlane;
				inward = qfalse;
				//continue;
			}

			VectorCopy4( &pc->planes[ planenum ].plane, &plane );

			//planenum = facet->surfacePlane;
			if ( inward ) {
				VectorSubtract( &vec3_origin, (vector3 *)&plane, (vector3 *)&plane );
				plane.w = -plane.w;
			}

			plane.w += cv->value;
			//*
			for (n = 0; n < 3; n++)
			{
				if (plane.data[n] > 0) v1.data[n] = maxs.data[n];
				else v1.data[n] = mins.data[n];
			} //end for
			VectorNegate((vector3 *)&plane, &v2);
			plane.w += fabsf(DotProduct(&v1, &v2));
			//*/

			w = BaseWindingForPlane( (vector3 *)&plane,  plane.w );
			for ( j = 0 ; j < facet->numBorders + 1 && w; j++ ) {
				//
				if (j < facet->numBorders) {
					curplanenum = facet->borderPlanes[j];
					curinward = facet->borderInward[j];
				}
				else {
					curplanenum = facet->surfacePlane;
					curinward = qfalse;
					//continue;
				}
				//
				if (curplanenum == planenum) continue;

				VectorCopy4( &pc->planes[ curplanenum ].plane, &plane );
				if ( !curinward ) {
					VectorSubtract( &vec3_origin, (vector3 *)&plane, (vector3 *)&plane );
					plane.w = -plane.w;
				}
			//	if ( !facet->borderNoAdjust[j] ) {
					plane.w -= cv->value;
			//	}
				for (n = 0; n < 3; n++)
				{
					if (plane.data[n] > 0) v1.data[n] = maxs.data[n];
					else v1.data[n] = mins.data[n];
				} //end for
				VectorNegate((vector3 *)&plane, &v2);
				plane.w -= fabsf(DotProduct(&v1, &v2));

				ChopWindingInPlace( &w, (vector3 *)&plane, plane.w, 0.1f );
			}
			if ( w ) {
				if ( facet == debugFacet ) {
					drawPoly( 4, w->numpoints, &w->p[0] );
					//Com_Printf("blue facet has %d border planes\n", facet->numBorders);
				} else {
					drawPoly( 1, w->numpoints, &w->p[0] );
				}
				FreeWinding( w );
			}
			else
				Com_Printf("winding chopped away by border planes\n");
		}
	}

	// draw the debug block
	{
		matrix3 v;

		VectorCopy( &debugBlockPoints[0], &v[0] );
		VectorCopy( &debugBlockPoints[1], &v[1] );
		VectorCopy( &debugBlockPoints[2], &v[2] );
		drawPoly( 2, 3, &v[0] );

		VectorCopy( &debugBlockPoints[2], &v[0] );
		VectorCopy( &debugBlockPoints[3], &v[1] );
		VectorCopy( &debugBlockPoints[0], &v[2] );
		drawPoly( 2, 3, &v[0] );
	}

#if 0
	vector3			v[4];

	v[0][0] = pc->bounds[1][0];
	v[0][1] = pc->bounds[1][1];
	v[0][2] = pc->bounds[1][2];

	v[1][0] = pc->bounds[1][0];
	v[1][1] = pc->bounds[0][1];
	v[1][2] = pc->bounds[1][2];

	v[2][0] = pc->bounds[0][0];
	v[2][1] = pc->bounds[0][1];
	v[2][2] = pc->bounds[1][2];

	v[3][0] = pc->bounds[0][0];
	v[3][1] = pc->bounds[1][1];
	v[3][2] = pc->bounds[1][2];

	drawPoly( 4, v[0] );
#endif
}
