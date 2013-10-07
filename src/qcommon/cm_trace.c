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

// always use bbox vs. bbox collision and never capsule vs. bbox or vice versa
//#define ALWAYS_BBOX_VS_BBOX
// always use capsule vs. capsule collision and never capsule vs. bbox or vice versa
//#define ALWAYS_CAPSULE_VS_CAPSULE

//#define CAPSULE_DEBUG

/*

	BASIC MATH

*/

void RotatePoint(vector3 *point, const matrix3 matrix) {
	vector3 tvec;

	VectorCopy(point, &tvec);
	point->x = DotProduct(&matrix[0], &tvec);
	point->y = DotProduct(&matrix[1], &tvec);
	point->z = DotProduct(&matrix[2], &tvec);
}

void CreateRotationMatrix(const vector3 *angles, matrix3 matrix) {
	AngleVectors(angles, &matrix[0], &matrix[1], &matrix[2]);
	VectorInverse(&matrix[1]);
}

void CM_ProjectPointOntoVector( vector3 *point, vector3 *vStart, vector3 *vDir, vector3 *vProj )
{
	vector3 pVec;

	VectorSubtract( point, vStart, &pVec );
	// project onto the directional vector for this segment
	VectorMA( vStart, DotProduct( &pVec, vDir ), vDir, vProj );
}

float CM_DistanceFromLineSquared(vector3 *p, vector3 *lp1, vector3 *lp2, vector3 *dir) {
	vector3 proj, t;
	int j;

	CM_ProjectPointOntoVector(p, lp1, dir, &proj);
	for (j = 0; j < 3; j++) 
		if ((proj.data[j] > lp1->data[j] && proj.data[j] > lp2->data[j]) ||
			(proj.data[j] < lp1->data[j] && proj.data[j] < lp2->data[j]))
			break;
	if (j < 3) {
		if (fabs(proj.data[j] - lp1->data[j]) < fabsf(proj.data[j] - lp2->data[j]))
			VectorSubtract(p, lp1, &t);
		else
			VectorSubtract(p, lp2, &t);
		return VectorLengthSquared(&t);
	}
	VectorSubtract(p, &proj, &t);
	return VectorLengthSquared(&t);
}

float CM_VectorDistanceSquared(vector3 *p1, vector3 *p2) {
	vector3 dir;

	VectorSubtract(p2, p1, &dir);
	return VectorLengthSquared(&dir);
}

float SquareRootFloat(float number) {
	floatint_t t;
	float x, y;
	const float f = 1.5F;

	x = number * 0.5F;
	t.f  = number;
	t.i  = 0x5f3759df - ( t.i >> 1 );
	y  = t.f;
	y  = y * ( f - ( x * y * y ) );
	y  = y * ( f - ( x * y * y ) );
	return number * y;
}


/*

	POSITION TESTING

*/

void CM_TestBoxInBrush( traceWork_t *tw, cbrush_t *brush ) {
	int			i;
	cplane_t	*plane;
	float		dist;
	float		d1;
	cbrushside_t	*side;
	float		t;
	vector3		startp;

	if (!brush->numsides) {
		return;
	}

	// special test for axial
	if ( tw->bounds[0].x > brush->bounds[1].x ||
		 tw->bounds[0].y > brush->bounds[1].y ||
		 tw->bounds[0].z > brush->bounds[1].z ||
		 tw->bounds[1].x < brush->bounds[0].x ||
		 tw->bounds[1].y < brush->bounds[0].y ||
		 tw->bounds[1].z < brush->bounds[0].z ) {
		return;
	}

   if ( tw->sphere.use ) {
		// the first six planes are the axial planes, so we only
		// need to test the remainder
		for ( i = 6 ; i < brush->numsides ; i++ ) {
			side = brush->sides + i;
			plane = side->plane;

			// adjust the plane distance apropriately for radius
			dist = plane->dist + tw->sphere.radius;
			// find the closest point on the capsule to the plane
			t = DotProduct( &plane->normal, &tw->sphere.offset );
			if ( t > 0 )
			{
				VectorSubtract( &tw->start, &tw->sphere.offset, &startp );
			}
			else
			{
				VectorAdd( &tw->start, &tw->sphere.offset, &startp );
			}
			d1 = DotProduct( &startp, &plane->normal ) - dist;
			// if completely in front of face, no intersection
			if ( d1 > 0 ) {
				return;
			}
		}
	} else {
		// the first six planes are the axial planes, so we only
		// need to test the remainder
		for ( i = 6 ; i < brush->numsides ; i++ ) {
			side = brush->sides + i;
			plane = side->plane;

			// adjust the plane distance apropriately for mins/maxs
			dist = plane->dist - DotProduct( &tw->offsets[ plane->signbits ], &plane->normal );

			d1 = DotProduct( &tw->start, &plane->normal ) - dist;

			// if completely in front of face, no intersection
			if ( d1 > 0 ) {
				return;
			}
		}
	}

	// inside this brush
	tw->trace.startsolid = tw->trace.allsolid = qtrue;
	tw->trace.fraction = 0;
	tw->trace.contents = brush->contents;
}

void CM_TestInLeaf( traceWork_t *tw, cLeaf_t *leaf ) {
	int			k;
	int			brushnum;
	cbrush_t	*b;
	cPatch_t	*patch;

	// test box position against all brushes in the leaf
	for (k=0 ; k<leaf->numLeafBrushes ; k++) {
		brushnum = cm.leafbrushes[leaf->firstLeafBrush+k];
		b = &cm.brushes[brushnum];
		if (b->checkcount == cm.checkcount) {
			continue;	// already checked this brush in another leaf
		}
		b->checkcount = cm.checkcount;

		if ( !(b->contents & tw->contents)) {
			continue;
		}
		
		CM_TestBoxInBrush( tw, b );
		if ( tw->trace.allsolid ) {
			return;
		}
	}

	// test against all patches
#ifdef BSPC
	if (1) {
#else
	if ( !cm_noCurves->integer ) {
#endif //BSPC
		for ( k = 0 ; k < leaf->numLeafSurfaces ; k++ ) {
			patch = cm.surfaces[ cm.leafsurfaces[ leaf->firstLeafSurface + k ] ];
			if ( !patch ) {
				continue;
			}
			if ( patch->checkcount == cm.checkcount ) {
				continue;	// already checked this brush in another leaf
			}
			patch->checkcount = cm.checkcount;

			if ( !(patch->contents & tw->contents)) {
				continue;
			}
			
			if ( CM_PositionTestInPatchCollide( tw, patch->pc ) ) {
				tw->trace.startsolid = tw->trace.allsolid = qtrue;
				tw->trace.fraction = 0;
				tw->trace.contents = patch->contents;
				return;
			}
		}
	}
}

// capsule inside capsule check
void CM_TestCapsuleInCapsule( traceWork_t *tw, clipHandle_t model ) {
	int i;
	vector3 mins, maxs;
	vector3 top, bottom;
	vector3 p1, p2, tmp;
	vector3 offset, symetricSize[2];
	float radius, halfwidth, halfheight, offs, r;

	CM_ModelBounds(model, &mins, &maxs);

	VectorAdd(&tw->start, &tw->sphere.offset, &top);
	VectorSubtract(&tw->start, &tw->sphere.offset, &bottom);
	for ( i = 0 ; i < 3 ; i++ ) {
		offset.data[i] = ( mins.data[i] + maxs.data[i] ) * 0.5f;
		symetricSize[0].data[i] = mins.data[i] - offset.data[i];
		symetricSize[1].data[i] = maxs.data[i] - offset.data[i];
	}
	halfwidth = symetricSize[1].x;
	halfheight = symetricSize[1].z;
	radius = ( halfwidth > halfheight ) ? halfheight : halfwidth;
	offs = halfheight - radius;

	r = Square(tw->sphere.radius + radius);
	// check if any of the spheres overlap
	VectorCopy(&offset, &p1);
	p1.z += offs;
	VectorSubtract(&p1, &top, &tmp);
	if ( VectorLengthSquared(&tmp) < r ) {
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0;
	}
	VectorSubtract(&p1, &bottom, &tmp);
	if ( VectorLengthSquared(&tmp) < r ) {
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0;
	}
	VectorCopy(&offset, &p2);
	p2.z -= offs;
	VectorSubtract(&p2, &top, &tmp);
	if ( VectorLengthSquared(&tmp) < r ) {
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0;
	}
	VectorSubtract(&p2, &bottom, &tmp);
	if ( VectorLengthSquared(&tmp) < r ) {
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0;
	}
	// if between cylinder up and lower bounds
	if ( (top.z >= p1.z && top.z <= p2.z) ||
		(bottom.z >= p1.z && bottom.z <= p2.z) ) {
		// 2d coordinates
		top.z = p1.z = 0;
		// if the cylinders overlap
		VectorSubtract(&top, &p1, &tmp);
		if ( VectorLengthSquared(&tmp) < r ) {
			tw->trace.startsolid = tw->trace.allsolid = qtrue;
			tw->trace.fraction = 0;
		}
	}
}

// bounding box inside capsule check
void CM_TestBoundingBoxInCapsule( traceWork_t *tw, clipHandle_t model ) {
	vector3 mins, maxs, offset, size[2];
	clipHandle_t h;
	cmodel_t *cmod;
	int i;

	// mins maxs of the capsule
	CM_ModelBounds(model, &mins, &maxs);

	// offset for capsule center
	for ( i = 0 ; i < 3 ; i++ ) {
		offset.data[i] = ( mins.data[i] + maxs.data[i] ) * 0.5f;
		size[0].data[i] = mins.data[i] - offset.data[i];
		size[1].data[i] = maxs.data[i] - offset.data[i];
		tw->start.data[i] -= offset.data[i];
		tw->end.data[i] -= offset.data[i];
	}

	// replace the bounding box with the capsule
	tw->sphere.use = qtrue;
	tw->sphere.radius = ( size[1].x > size[1].z ) ? size[1].z : size[1].x;
	tw->sphere.halfheight = size[1].z;
	VectorSet( &tw->sphere.offset, 0, 0, size[1].z - tw->sphere.radius );

	// replace the capsule with the bounding box
	h = CM_TempBoxModel(&tw->size[0], &tw->size[1], qfalse);
	// calculate collision
	cmod = CM_ClipHandleToModel( h );
	CM_TestInLeaf( tw, &cmod->leaf );
}

#define	MAX_POSITION_LEAFS	1024
void CM_PositionTest( traceWork_t *tw ) {
	int		leafs[MAX_POSITION_LEAFS];
	int		i;
	leafList_t	ll;

	// identify the leafs we are touching
	VectorAdd( &tw->start, &tw->size[0], &ll.bounds[0] );
	VectorAdd( &tw->start, &tw->size[1], &ll.bounds[1] );

	for (i=0 ; i<3 ; i++) {
		ll.bounds[0].data[i] -= 1;
		ll.bounds[1].data[i] += 1;
	}

	ll.count = 0;
	ll.maxcount = MAX_POSITION_LEAFS;
	ll.list = leafs;
	ll.storeLeafs = CM_StoreLeafs;
	ll.lastLeaf = 0;
	ll.overflowed = qfalse;

	cm.checkcount++;

	CM_BoxLeafnums_r( &ll, 0 );


	cm.checkcount++;

	// test the contents of the leafs
	for (i=0 ; i < ll.count ; i++) {
		CM_TestInLeaf( tw, &cm.leafs[leafs[i]] );
		if ( tw->trace.allsolid ) {
			break;
		}
	}
}

/*

	TRACING

*/

void CM_TraceThroughPatch( traceWork_t *tw, cPatch_t *patch ) {
	float		oldFrac;

	c_patch_traces++;

	oldFrac = tw->trace.fraction;

	CM_TraceThroughPatchCollide( tw, patch->pc );

	if ( tw->trace.fraction < oldFrac ) {
		tw->trace.surfaceFlags = patch->surfaceFlags;
		tw->trace.contents = patch->contents;
	}
}

void CM_TraceThroughBrush( traceWork_t *tw, cbrush_t *brush ) {
	int			i;
	cplane_t	*plane, *clipplane;
	float		dist;
	float		enterFrac, leaveFrac;
	float		d1, d2;
	qboolean	getout, startout;
	float		f;
	cbrushside_t	*side, *leadside;
	float		t;
	vector3		startp;
	vector3		endp;

	enterFrac = -1.0;
	leaveFrac = 1.0;
	clipplane = NULL;

	if ( !brush->numsides ) {
		return;
	}

	c_brush_traces++;

	getout = qfalse;
	startout = qfalse;

	leadside = NULL;

	if ( tw->sphere.use ) {
		//
		// compare the trace against all planes of the brush
		// find the latest time the trace crosses a plane towards the interior
		// and the earliest time the trace crosses a plane towards the exterior
		//
		for (i = 0; i < brush->numsides; i++) {
			side = brush->sides + i;
			plane = side->plane;

			// adjust the plane distance apropriately for radius
			dist = plane->dist + tw->sphere.radius;

			// find the closest point on the capsule to the plane
			t = DotProduct( &plane->normal, &tw->sphere.offset );
			if ( t > 0 )
			{
				VectorSubtract( &tw->start, &tw->sphere.offset, &startp );
				VectorSubtract( &tw->end, &tw->sphere.offset, &endp );
			}
			else
			{
				VectorAdd( &tw->start, &tw->sphere.offset, &startp );
				VectorAdd( &tw->end, &tw->sphere.offset, &endp );
			}

			d1 = DotProduct( &startp, &plane->normal ) - dist;
			d2 = DotProduct( &endp, &plane->normal ) - dist;

			if (d2 > 0) {
				getout = qtrue;	// endpoint is not in solid
			}
			if (d1 > 0) {
				startout = qtrue;
			}

			// if completely in front of face, no intersection with the entire brush
			if (d1 > 0 && ( d2 >= SURFACE_CLIP_EPSILON || d2 >= d1 )  ) {
				return;
			}

			// if it doesn't cross the plane, the plane isn't relevent
			if (d1 <= 0 && d2 <= 0 ) {
				continue;
			}

			// crosses face
			if (d1 > d2) {	// enter
				f = (d1-SURFACE_CLIP_EPSILON) / (d1-d2);
				if ( f < 0 ) {
					f = 0;
				}
				if (f > enterFrac) {
					enterFrac = f;
					clipplane = plane;
					leadside = side;
				}
			} else {	// leave
				f = (d1+SURFACE_CLIP_EPSILON) / (d1-d2);
				if ( f > 1 ) {
					f = 1;
				}
				if (f < leaveFrac) {
					leaveFrac = f;
				}
			}
		}
	} else {
		//
		// compare the trace against all planes of the brush
		// find the latest time the trace crosses a plane towards the interior
		// and the earliest time the trace crosses a plane towards the exterior
		//
		for (i = 0; i < brush->numsides; i++) {
			side = brush->sides + i;
			plane = side->plane;

			// adjust the plane distance apropriately for mins/maxs
			dist = plane->dist - DotProduct( &tw->offsets[ plane->signbits ], &plane->normal );

			d1 = DotProduct( &tw->start, &plane->normal ) - dist;
			d2 = DotProduct( &tw->end, &plane->normal ) - dist;

			if (d2 > 0) {
				getout = qtrue;	// endpoint is not in solid
			}
			if (d1 > 0) {
				startout = qtrue;
			}

			// if completely in front of face, no intersection with the entire brush
			if (d1 > 0 && ( d2 >= SURFACE_CLIP_EPSILON || d2 >= d1 )  ) {
				return;
			}

			// if it doesn't cross the plane, the plane isn't relevent
			if (d1 <= 0 && d2 <= 0 ) {
				continue;
			}

			// crosses face
			if (d1 > d2) {	// enter
				f = (d1-SURFACE_CLIP_EPSILON) / (d1-d2);
				if ( f < 0 ) {
					f = 0;
				}
				if (f > enterFrac) {
					enterFrac = f;
					clipplane = plane;
					leadside = side;
				}
			} else {	// leave
				f = (d1+SURFACE_CLIP_EPSILON) / (d1-d2);
				if ( f > 1 ) {
					f = 1;
				}
				if (f < leaveFrac) {
					leaveFrac = f;
				}
			}
		}
	}

	//
	// all planes have been checked, and the trace was not
	// completely outside the brush
	//
	if (!startout) {	// original point was inside brush
		tw->trace.startsolid = qtrue;
		if (!getout) {
			tw->trace.allsolid = qtrue;
			tw->trace.fraction = 0;
			tw->trace.contents = brush->contents;
		}
		return;
	}
	
	if (enterFrac < leaveFrac) {
		if (enterFrac > -1 && enterFrac < tw->trace.fraction) {
			if (enterFrac < 0) {
				enterFrac = 0;
			}
			tw->trace.fraction = enterFrac;
			tw->trace.plane = *clipplane;
			tw->trace.surfaceFlags = leadside->surfaceFlags;
			tw->trace.contents = brush->contents;
		}
	}
}

void CM_TraceThroughLeaf( traceWork_t *tw, cLeaf_t *leaf ) {
	int			k;
	int			brushnum;
	cbrush_t	*b;
	cPatch_t	*patch;

	// trace line against all brushes in the leaf
	for ( k = 0 ; k < leaf->numLeafBrushes ; k++ ) {
		brushnum = cm.leafbrushes[leaf->firstLeafBrush+k];

		b = &cm.brushes[brushnum];
		if ( b->checkcount == cm.checkcount ) {
			continue;	// already checked this brush in another leaf
		}
		b->checkcount = cm.checkcount;

		if ( !(b->contents & tw->contents) ) {
			continue;
		}

		if ( !CM_BoundsIntersect( &tw->bounds[0], &tw->bounds[1], &b->bounds[0], &b->bounds[1] ) ) {
			continue;
		}

		CM_TraceThroughBrush( tw, b );
		if ( !tw->trace.fraction ) {
			return;
		}
	}

	// trace line against all patches in the leaf
#ifdef BSPC
	if (1) {
#else
	if ( !cm_noCurves->integer ) {
#endif
		for ( k = 0 ; k < leaf->numLeafSurfaces ; k++ ) {
			patch = cm.surfaces[ cm.leafsurfaces[ leaf->firstLeafSurface + k ] ];
			if ( !patch ) {
				continue;
			}
			if ( patch->checkcount == cm.checkcount ) {
				continue;	// already checked this patch in another leaf
			}
			patch->checkcount = cm.checkcount;

			if ( !(patch->contents & tw->contents) ) {
				continue;
			}
			
			CM_TraceThroughPatch( tw, patch );
			if ( !tw->trace.fraction ) {
				return;
			}
		}
	}
}

#define RADIUS_EPSILON		1.0f

// get the first intersection of the ray with the sphere
void CM_TraceThroughSphere( traceWork_t *tw, vector3 *origin, float radius, vector3 *start, vector3 *end ) {
	float l1, l2, length, scale, fraction;
	//float a;
	float b, c, d, sqrtd;
	vector3 v1, dir, intersection;

	// if inside the sphere
	VectorSubtract(start, origin, &dir);
	l1 = VectorLengthSquared(&dir);
	if (l1 < Square(radius)) {
		tw->trace.fraction = 0;
		tw->trace.startsolid = qtrue;
		// test for allsolid
		VectorSubtract(end, origin, &dir);
		l1 = VectorLengthSquared(&dir);
		if (l1 < Square(radius)) {
			tw->trace.allsolid = qtrue;
		}
		return;
	}
	//
	VectorSubtract(end, start, &dir);
	length = VectorNormalize(&dir);
	//
	l1 = CM_DistanceFromLineSquared(origin, start, end, &dir);
	VectorSubtract(end, origin, &v1);
	l2 = VectorLengthSquared(&v1);
	// if no intersection with the sphere and the end point is at least an epsilon away
	if (l1 >= Square(radius) && l2 > Square(radius+SURFACE_CLIP_EPSILON)) {
		return;
	}
	//
	//	| origin - (start + t * dir) | = radius
	//	a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	//	b = 2 * (dir[0] * (start[0] - origin[0]) + dir[1] * (start[1] - origin[1]) + dir[2] * (start[2] - origin[2]));
	//	c = (start[0] - origin[0])^2 + (start[1] - origin[1])^2 + (start[2] - origin[2])^2 - radius^2;
	//
	VectorSubtract(start, origin, &v1);
	// dir is normalized so a = 1
	//a = 1.0f;//dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
	b = 2.0f * (dir.x * v1.x + dir.y * v1.y + dir.z * v1.z);
	c = v1.x * v1.x + v1.y * v1.y + v1.z * v1.z - (radius+RADIUS_EPSILON) * (radius+RADIUS_EPSILON);

	d = b * b - 4.0f * c;// * a;
	if (d > 0) {
		sqrtd = SquareRootFloat(d);
		// = (- b + sqrtd) * 0.5f; // / (2.0f * a);
		fraction = (- b - sqrtd) * 0.5f; // / (2.0f * a);
		//
		if (fraction < 0) {
			fraction = 0;
		}
		else {
			fraction /= length;
		}
		if ( fraction < tw->trace.fraction ) {
			tw->trace.fraction = fraction;
			VectorSubtract(end, start, &dir);
			VectorMA(start, fraction, &dir, &intersection);
			VectorSubtract(&intersection, origin, &dir);
			#ifdef CAPSULE_DEBUG
				l2 = VectorLength(dir);
				if (l2 < radius) {
					int bah = 1;
				}
			#endif
			scale = 1 / (radius+RADIUS_EPSILON);
			VectorScale(&dir, scale, &dir);
			VectorCopy(&dir, &tw->trace.plane.normal);
			VectorAdd( &tw->modelOrigin, &intersection, &intersection);
			tw->trace.plane.dist = DotProduct(&tw->trace.plane.normal, &intersection);
			tw->trace.contents = CONTENTS_BODY;
		}
	}
	else if (d == 0) {
		//t1 = (- b ) / 2;
		// slide along the sphere
	}
	// no intersection at all
}

// get the first intersection of the ray with the cylinder
//	the cylinder extends halfheight above and below the origin
void CM_TraceThroughVerticalCylinder( traceWork_t *tw, vector3 *origin, float radius, float halfheight, vector3 *start, vector3 *end) {
	float length, scale, fraction, l1, l2;
	//float a;
	float b, c, d, sqrtd;
	vector3 v1, dir, start2d, end2d, org2d, intersection;

	// 2d coordinates
	VectorSet(&start2d, start->x, start->y, 0);
	VectorSet(&end2d, end->x, end->y, 0);
	VectorSet(&org2d, origin->x, origin->y, 0);
	// if between lower and upper cylinder bounds
	if (start->z <= origin->z + halfheight &&
		start->z >= origin->z - halfheight) {
		// if inside the cylinder
		VectorSubtract(&start2d, &org2d, &dir);
		l1 = VectorLengthSquared(&dir);
		if (l1 < Square(radius)) {
			tw->trace.fraction = 0;
			tw->trace.startsolid = qtrue;
			VectorSubtract(&end2d, &org2d, &dir);
			l1 = VectorLengthSquared(&dir);
			if (l1 < Square(radius)) {
				tw->trace.allsolid = qtrue;
			}
			return;
		}
	}
	//
	VectorSubtract(&end2d, &start2d, &dir);
	length = VectorNormalize(&dir);
	//
	l1 = CM_DistanceFromLineSquared(&org2d, &start2d, &end2d, &dir);
	VectorSubtract(&end2d, &org2d, &v1);
	l2 = VectorLengthSquared(&v1);
	// if no intersection with the cylinder and the end point is at least an epsilon away
	if (l1 >= Square(radius) && l2 > Square(radius+SURFACE_CLIP_EPSILON)) {
		return;
	}
	//
	//
	// (start[0] - origin[0] - t * dir[0]) ^ 2 + (start[1] - origin[1] - t * dir[1]) ^ 2 = radius ^ 2
	// (v1[0] + t * dir[0]) ^ 2 + (v1[1] + t * dir[1]) ^ 2 = radius ^ 2;
	// v1[0] ^ 2 + 2 * v1[0] * t * dir[0] + (t * dir[0]) ^ 2 +
	//						v1[1] ^ 2 + 2 * v1[1] * t * dir[1] + (t * dir[1]) ^ 2 = radius ^ 2
	// t ^ 2 * (dir[0] ^ 2 + dir[1] ^ 2) + t * (2 * v1[0] * dir[0] + 2 * v1[1] * dir[1]) +
	//						v1[0] ^ 2 + v1[1] ^ 2 - radius ^ 2 = 0
	//
	VectorSubtract(start, origin, &v1);
	// dir is normalized so we can use a = 1
	//a = 1.0f;// * (dir[0] * dir[0] + dir[1] * dir[1]);
	b = 2.0f * (v1.x * dir.x + v1.y * dir.y);
	c = v1.x * v1.x + v1.y * v1.y - (radius+RADIUS_EPSILON) * (radius+RADIUS_EPSILON);

	d = b * b - 4.0f * c;// * a;
	if (d > 0) {
		sqrtd = SquareRootFloat(d);
		// = (- b + sqrtd) * 0.5f;// / (2.0f * a);
		fraction = (- b - sqrtd) * 0.5f;// / (2.0f * a);
		//
		if (fraction < 0) {
			fraction = 0;
		}
		else {
			fraction /= length;
		}
		if ( fraction < tw->trace.fraction ) {
			VectorSubtract(end, start, &dir);
			VectorMA(start, fraction, &dir, &intersection);
			// if the intersection is between the cylinder lower and upper bound
			if (intersection.z <= origin->z + halfheight &&
				intersection.z >= origin->z - halfheight) {
				//
				tw->trace.fraction = fraction;
				VectorSubtract(&intersection, origin, &dir);
				dir.z = 0;
				#ifdef CAPSULE_DEBUG
					l2 = VectorLength(dir);
					if (l2 <= radius) {
						int bah = 1;
					}
				#endif
				scale = 1 / (radius+RADIUS_EPSILON);
				VectorScale(&dir, scale, &dir);
				VectorCopy(&dir, &tw->trace.plane.normal);
				VectorAdd( &tw->modelOrigin, &intersection, &intersection);
				tw->trace.plane.dist = DotProduct(&tw->trace.plane.normal, &intersection);
				tw->trace.contents = CONTENTS_BODY;
			}
		}
	}
	else if (d == 0) {
		//t[0] = (- b ) / 2 * a;
		// slide along the cylinder
	}
	// no intersection at all
}

// capsule vs. capsule collision (not rotated)
void CM_TraceCapsuleThroughCapsule( traceWork_t *tw, clipHandle_t model ) {
	int i;
	vector3 mins, maxs;
	vector3 top, bottom, starttop, startbottom, endtop, endbottom;
	vector3 offset, symetricSize[2];
	float radius, halfwidth, halfheight, offs, h;

	CM_ModelBounds(model, &mins, &maxs);
	// test trace bounds vs. capsule bounds
	if ( tw->bounds[0].x > maxs.x + RADIUS_EPSILON ||
		 tw->bounds[0].y > maxs.y + RADIUS_EPSILON ||
		 tw->bounds[0].z > maxs.z + RADIUS_EPSILON ||
		 tw->bounds[1].x < mins.x - RADIUS_EPSILON ||
		 tw->bounds[1].y < mins.y - RADIUS_EPSILON ||
		 tw->bounds[1].z < mins.z - RADIUS_EPSILON) {
		return;
	}
	// top origin and bottom origin of each sphere at start and end of trace
	VectorAdd(&tw->start, &tw->sphere.offset, &starttop);
	VectorSubtract(&tw->start, &tw->sphere.offset, &startbottom);
	VectorAdd(&tw->end, &tw->sphere.offset, &endtop);
	VectorSubtract(&tw->end, &tw->sphere.offset, &endbottom);

	// calculate top and bottom of the capsule spheres to collide with
	for ( i = 0 ; i < 3 ; i++ ) {
		offset.data[i] = ( mins.data[i] + maxs.data[i] ) * 0.5f;
		symetricSize[0].data[i] = mins.data[i] - offset.data[i];
		symetricSize[1].data[i] = maxs.data[i] - offset.data[i];
	}
	halfwidth = symetricSize[1].x;
	halfheight = symetricSize[1].z;
	radius = ( halfwidth > halfheight ) ? halfheight : halfwidth;
	offs = halfheight - radius;
	VectorCopy(&offset, &top);
	top.z += offs;
	VectorCopy(&offset, &bottom);
	bottom.z -= offs;
	// expand radius of spheres
	radius += tw->sphere.radius;
	// if there is horizontal movement
	if ( tw->start.x != tw->end.x || tw->start.y != tw->end.y ) {
		// height of the expanded cylinder is the height of both cylinders minus the radius of both spheres
		h = halfheight + tw->sphere.halfheight - radius;
		// if the cylinder has a height
		if ( h > 0 ) {
			// test for collisions between the cylinders
			CM_TraceThroughVerticalCylinder(tw, &offset, radius, h, &tw->start, &tw->end);
		}
	}
	// test for collision between the spheres
	CM_TraceThroughSphere(tw, &top, radius, &startbottom, &endbottom);
	CM_TraceThroughSphere(tw, &bottom, radius, &starttop, &endtop);
}

// bounding box vs. capsule collision
void CM_TraceBoundingBoxThroughCapsule( traceWork_t *tw, clipHandle_t model ) {
	vector3 mins, maxs, offset, size[2];
	clipHandle_t h;
	cmodel_t *cmod;
	int i;

	// mins maxs of the capsule
	CM_ModelBounds(model, &mins, &maxs);

	// offset for capsule center
	for ( i = 0 ; i < 3 ; i++ ) {
		offset.data[i] = ( mins.data[i] + maxs.data[i] ) * 0.5f;
		size[0].data[i] = mins.data[i] - offset.data[i];
		size[1].data[i] = maxs.data[i] - offset.data[i];
		tw->start.data[i] -= offset.data[i];
		tw->end.data[i] -= offset.data[i];
	}

	// replace the bounding box with the capsule
	tw->sphere.use = qtrue;
	tw->sphere.radius = ( size[1].x > size[1].z ) ? size[1].z: size[1].x;
	tw->sphere.halfheight = size[1].z;
	VectorSet( &tw->sphere.offset, 0, 0, size[1].z - tw->sphere.radius );

	// replace the capsule with the bounding box
	h = CM_TempBoxModel(&tw->size[0], &tw->size[1], qfalse);
	// calculate collision
	cmod = CM_ClipHandleToModel( h );
	CM_TraceThroughLeaf( tw, &cmod->leaf );
}

// Traverse all the contacted leafs from the start to the end position.
//	If the trace is a point, they will be exactly in order, but for larger trace volumes it is possible to hit
//	something in a later leaf with a smaller intercept fraction.
void CM_TraceThroughTree( traceWork_t *tw, int num, float p1f, float p2f, vector3 *p1, vector3 *p2) {
	cNode_t		*node;
	cplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	vector3		mid;
	int			side;
	float		midf;

	if (tw->trace.fraction <= p1f) {
		return;		// already hit something nearer
	}

	// if < 0, we are in a leaf node
	if (num < 0) {
		CM_TraceThroughLeaf( tw, &cm.leafs[-1-num] );
		return;
	}

	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//
	node = cm.nodes + num;
	plane = node->plane;

	// adjust the plane distance apropriately for mins/maxs
	if ( plane->type < 3 ) {
		t1 = p1->data[plane->type] - plane->dist;
		t2 = p2->data[plane->type] - plane->dist;
		offset = tw->extents.data[plane->type];
	} else {
		t1 = DotProduct (&plane->normal, p1) - plane->dist;
		t2 = DotProduct (&plane->normal, p2) - plane->dist;
		if ( tw->isPoint ) {
			offset = 0;
		} else {
			// this is silly
			offset = 2048;
		}
	}

	// see which sides we need to consider
	if ( t1 >= offset + 1 && t2 >= offset + 1 ) {
		CM_TraceThroughTree( tw, node->children[0], p1f, p2f, p1, p2 );
		return;
	}
	if ( t1 < -offset - 1 && t2 < -offset - 1 ) {
		CM_TraceThroughTree( tw, node->children[1], p1f, p2f, p1, p2 );
		return;
	}

	// put the crosspoint SURFACE_CLIP_EPSILON pixels on the near side
	if ( t1 < t2 ) {
		idist = 1.0f/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + SURFACE_CLIP_EPSILON)*idist;
		frac = (t1 - offset + SURFACE_CLIP_EPSILON)*idist;
	} else if (t1 > t2) {
		idist = 1.0f/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - SURFACE_CLIP_EPSILON)*idist;
		frac = (t1 + offset + SURFACE_CLIP_EPSILON)*idist;
	} else {
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if ( frac < 0 ) {
		frac = 0;
	}
	if ( frac > 1 ) {
		frac = 1;
	}
		
	midf = p1f + (p2f - p1f)*frac;
	VectorLerp( p1, frac, p2, &mid );
	CM_TraceThroughTree( tw, node->children[side], p1f, midf, p1, &mid );


	// go past the node
	if ( frac2 < 0 ) {
		frac2 = 0;
	}
	if ( frac2 > 1 ) {
		frac2 = 1;
	}
		
	midf = p1f + (p2f - p1f)*frac2;
	VectorLerp( p1, frac, p2, &mid );
	CM_TraceThroughTree( tw, node->children[side^1], midf, p2f, &mid, p2 );
}

void CM_Trace( trace_t *results, const vector3 *start, const vector3 *end, const vector3 *mins, const vector3 *maxs, clipHandle_t model, const vector3 *origin, int brushmask, int capsule, sphere_t *sphere ) {
	int			i;
	traceWork_t	tw;
	vector3		offset;
	cmodel_t	*cmod;

	cmod = CM_ClipHandleToModel( model );

	cm.checkcount++;		// for multi-check avoidance

	c_traces++;				// for statistics, may be zeroed

	// fill in a default trace
	memset( &tw, 0, sizeof(tw) );
	tw.trace.fraction = 1;	// assume it goes the entire distance until shown otherwise
	VectorCopy(origin, &tw.modelOrigin);

	if (!cm.numNodes) {
		*results = tw.trace;

		return;	// map not loaded, shouldn't happen
	}

	// allow NULL to be passed in for 0,0,0
	if ( !mins ) {
		mins = &vec3_origin;
	}
	if ( !maxs ) {
		maxs = &vec3_origin;
	}

	// set basic parms
	tw.contents = brushmask;

	// adjust so that mins and maxs are always symetric, which
	// avoids some complications with plane expanding of rotated
	// bmodels
	for ( i = 0 ; i < 3 ; i++ ) {
		offset.data[i] = ( mins->data[i] + maxs->data[i] ) * 0.5f;
		tw.size[0].data[i] = mins->data[i] - offset.data[i];
		tw.size[1].data[i] = maxs->data[i] - offset.data[i];
		tw.start.data[i] = start->data[i] + offset.data[i];
		tw.end.data[i] = end->data[i] + offset.data[i];
	}

	// if a sphere is already specified
	if ( sphere ) {
		tw.sphere = *sphere;
	}
	else {
		tw.sphere.use = capsule;
		tw.sphere.radius = ( tw.size[1].x > tw.size[1].z ) ? tw.size[1].z: tw.size[1].x;
		tw.sphere.halfheight = tw.size[1].z;
		VectorSet( &tw.sphere.offset, 0, 0, tw.size[1].z - tw.sphere.radius );
	}

	tw.maxOffset = tw.size[1].x + tw.size[1].y + tw.size[1].z;

	// tw.offsets[signbits] = vector to apropriate corner from origin
	tw.offsets[0].x = tw.size[0].x;
	tw.offsets[0].y = tw.size[0].y;
	tw.offsets[0].z = tw.size[0].z;
	tw.offsets[1].x = tw.size[1].x;
	tw.offsets[1].y = tw.size[0].y;
	tw.offsets[1].z = tw.size[0].z;
	tw.offsets[2].x = tw.size[0].x;
	tw.offsets[2].y = tw.size[1].y;
	tw.offsets[2].z = tw.size[0].z;
	tw.offsets[3].x = tw.size[1].x;
	tw.offsets[3].y = tw.size[1].y;
	tw.offsets[3].z = tw.size[0].z;
	tw.offsets[4].x = tw.size[0].x;
	tw.offsets[4].y = tw.size[0].y;
	tw.offsets[4].z = tw.size[1].z;
	tw.offsets[5].x = tw.size[1].x;
	tw.offsets[5].y = tw.size[0].y;
	tw.offsets[5].z = tw.size[1].z;
	tw.offsets[6].x = tw.size[0].x;
	tw.offsets[6].y = tw.size[1].y;
	tw.offsets[6].z = tw.size[1].z;
	tw.offsets[7].x = tw.size[1].x;
	tw.offsets[7].y = tw.size[1].y;
	tw.offsets[7].z = tw.size[1].z;

	//
	// calculate bounds
	//
	if ( tw.sphere.use ) {
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( tw.start.data[i] < tw.end.data[i] ) {
				tw.bounds[0].data[i] = tw.start.data[i]	- fabsf(tw.sphere.offset.data[i]) - tw.sphere.radius;
				tw.bounds[1].data[i] = tw.end.data[i]	+ fabsf(tw.sphere.offset.data[i]) + tw.sphere.radius;
			} else {
				tw.bounds[0].data[i] = tw.end.data[i]	- fabsf(tw.sphere.offset.data[i]) - tw.sphere.radius;
				tw.bounds[1].data[i] = tw.start.data[i]	+ fabsf(tw.sphere.offset.data[i]) + tw.sphere.radius;
			}
		}
	}
	else {
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( tw.start.data[i] < tw.end.data[i] ) {
				tw.bounds[0].data[i] = tw.start.data[i]	+ tw.size[0].data[i];
				tw.bounds[1].data[i] = tw.end.data[i]	+ tw.size[1].data[i];
			} else {
				tw.bounds[0].data[i] = tw.end.data[i]	+ tw.size[0].data[i];
				tw.bounds[1].data[i] = tw.start.data[i]	+ tw.size[1].data[i];
			}
		}
	}

	//
	// check for position test special case
	//
	if ( VectorCompare( start, end ) ) {
		if ( model ) {
#ifdef ALWAYS_BBOX_VS_BBOX // FIXME - compile time flag?
			if ( model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE) {
				tw.sphere.use = qfalse;
				CM_TestInLeaf( &tw, &cmod->leaf );
			}
			else
#elif defined(ALWAYS_CAPSULE_VS_CAPSULE)
			if ( model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE) {
				CM_TestCapsuleInCapsule( &tw, model );
			}
			else
#endif
			if ( model == CAPSULE_MODEL_HANDLE ) {
				if ( tw.sphere.use ) {
					CM_TestCapsuleInCapsule( &tw, model );
				}
				else {
					CM_TestBoundingBoxInCapsule( &tw, model );
				}
			}
			else {
				CM_TestInLeaf( &tw, &cmod->leaf );
			}
		} else {
			CM_PositionTest( &tw );
		}
	} else {
		//
		// check for point special case
		//
		if ( tw.size[0].x == 0 && tw.size[0].y == 0 && tw.size[0].z == 0 ) {
			tw.isPoint = qtrue;
			VectorClear( &tw.extents );
		} else {
			tw.isPoint = qfalse;
			tw.extents.x = tw.size[1].x;
			tw.extents.y = tw.size[1].y;
			tw.extents.z = tw.size[1].z;
		}

		//
		// general sweeping through world
		//
		if ( model ) {
#ifdef ALWAYS_BBOX_VS_BBOX
			if ( model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE) {
				tw.sphere.use = qfalse;
				CM_TraceThroughLeaf( &tw, &cmod->leaf );
			}
			else
#elif defined(ALWAYS_CAPSULE_VS_CAPSULE)
			if ( model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE) {
				CM_TraceCapsuleThroughCapsule( &tw, model );
			}
			else
#endif
			if ( model == CAPSULE_MODEL_HANDLE ) {
				if ( tw.sphere.use ) {
					CM_TraceCapsuleThroughCapsule( &tw, model );
				}
				else {
					CM_TraceBoundingBoxThroughCapsule( &tw, model );
				}
			}
			else {
				CM_TraceThroughLeaf( &tw, &cmod->leaf );
			}
		} else {
			CM_TraceThroughTree( &tw, 0, 0, 1, &tw.start, &tw.end );
		}
	}

	// generate endpos from the original, unmodified start/end
	if ( tw.trace.fraction == 1 ) {
		VectorCopy (end, &tw.trace.endpos);
	} else {
		for ( i=0 ; i<3 ; i++ ) {
			tw.trace.endpos.data[i] = start->data[i] + tw.trace.fraction * (end->data[i] - start->data[i]);
		}
	}

        // If allsolid is set (was entirely inside something solid), the plane is not valid.
        // If fraction == 1.0, we never hit anything, and thus the plane is not valid.
        // Otherwise, the normal on the plane should have unit length
        assert(tw.trace.allsolid ||
               tw.trace.fraction == 1.0 ||
               VectorLengthSquared(&tw.trace.plane.normal) > 0.9999);
	*results = tw.trace;
}

void CM_BoxTrace( trace_t *results, const vector3 *start, const vector3 *end, const vector3 *mins, const vector3 *maxs, clipHandle_t model, int brushmask, int capsule ) {
	CM_Trace( results, start, end, mins, maxs, model, &vec3_origin, brushmask, capsule, NULL );
}

// Handles offseting and rotation of the end points for moving and rotating entities
void CM_TransformedBoxTrace( trace_t *results, const vector3 *start, const vector3 *end, const vector3 *mins, const vector3 *maxs, clipHandle_t model, int brushmask, const vector3 *origin, const vector3 *angles, int capsule ) {
	trace_t		trace;
	vector3		start_l, end_l;
	qboolean	rotated;
	vector3		offset;
	vector3		symetricSize[2];
	matrix3		matrix, transpose;
	int			i;
	float		halfwidth;
	float		halfheight;
	float		t;
	sphere_t	sphere;

	if ( !mins ) {
		mins = &vec3_origin;
	}
	if ( !maxs ) {
		maxs = &vec3_origin;
	}

	// adjust so that mins and maxs are always symetric, which
	// avoids some complications with plane expanding of rotated
	// bmodels
	for ( i = 0 ; i < 3 ; i++ ) {
		offset.data[i] = ( mins->data[i] + maxs->data[i] ) * 0.5f;
		symetricSize[0].data[i] = mins->data[i] - offset.data[i];
		symetricSize[1].data[i] = maxs->data[i] - offset.data[i];
		start_l.data[i] = start->data[i] + offset.data[i];
		end_l.data[i] = end->data[i] + offset.data[i];
	}

	// subtract origin offset
	VectorSubtract( &start_l, origin, &start_l );
	VectorSubtract( &end_l, origin, &end_l );

	// rotate start and end into the models frame of reference
	if ( model != BOX_MODEL_HANDLE && 
		(angles->x || angles->y || angles->z) ) {
		rotated = qtrue;
	} else {
		rotated = qfalse;
	}

	halfwidth = symetricSize[1].x;
	halfheight = symetricSize[1].z;

	sphere.use = capsule;
	sphere.radius = ( halfwidth > halfheight ) ? halfheight : halfwidth;
	sphere.halfheight = halfheight;
	t = halfheight - sphere.radius;

	if (rotated) {
		// rotation on trace line (start-end) instead of rotating the bmodel
		// NOTE: This is still incorrect for bounding boxes because the actual bounding
		//		 box that is swept through the model is not rotated. We cannot rotate
		//		 the bounding box or the bmodel because that would make all the brush
		//		 bevels invalid.
		//		 However this is correct for capsules since a capsule itself is rotated too.
		CreateRotationMatrix(angles, matrix);
		RotatePoint(&start_l, matrix);
		RotatePoint(&end_l, matrix);
		// rotated sphere offset for capsule
		sphere.offset.x =  matrix[0].z * t;
		sphere.offset.y = -matrix[1].z * t;
		sphere.offset.z =  matrix[2].z * t;
	}
	else {
		VectorSet( &sphere.offset, 0, 0, t );
	}

	// sweep the box through the model
	CM_Trace( &trace, &start_l, &end_l, &symetricSize[0], &symetricSize[1], model, origin, brushmask, capsule, &sphere );

	// if the bmodel was rotated and there was a collision
	if ( rotated && trace.fraction != 1.0 ) {
		// rotation of bmodel collision plane
		MatrixTranspose(matrix, transpose);
		RotatePoint(&trace.plane.normal, transpose);
	}

	// re-calculate the end position of the trace because the trace.endpos
	// calculated by CM_Trace could be rotated and have an offset
	VectorLerp( start, trace.fraction, end, &trace.endpos );
	*results = trace;
}
