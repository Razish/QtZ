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

// this is only used for visualization tools in cm_ debug functions


#include "cm_local.h"


// counters are only bumped when running single threaded,
// because they are an awful coherence problem
int	c_active_windings;
int	c_peak_windings;
int	c_winding_allocs;
int	c_winding_points;

winding_t	*AllocWinding (int points) {
	winding_t	*w;
	int			s;

	c_winding_allocs++;
	c_winding_points += points;
	c_active_windings++;
	if (c_active_windings > c_peak_windings)
		c_peak_windings = c_active_windings;

	s = sizeof(number)*3*points + sizeof(int);
	w = Z_Malloc (s);
	memset (w, 0, s); 
	return w;
}

void FreeWinding (winding_t *w) {
	if (*(unsigned *)w == 0xdeaddead)
		Com_Error (ERR_FATAL, "FreeWinding: freed a freed winding");
	*(unsigned *)w = 0xdeaddead;

	c_active_windings--;
	Z_Free (w);
}

int	c_removed;

void	RemoveColinearPoints (winding_t *w) {
	int		i, j, k;
	vector3	v1, v2;
	int		nump;
	vector3	p[MAX_POINTS_ON_WINDING];

	nump = 0;
	for (i=0 ; i<w->numpoints ; i++)
	{
		j = (i+1)%w->numpoints;
		k = (i+w->numpoints-1)%w->numpoints;
		VectorSubtract (&w->p[j], &w->p[i], &v1);
		VectorSubtract (&w->p[i], &w->p[k], &v2);
		VectorNormalize2(&v1,&v1);
		VectorNormalize2(&v2,&v2);
		if (DotProduct(&v1, &v2) < 0.999f)
		{
			VectorCopy (&w->p[i], &p[nump]);
			nump++;
		}
	}

	if (nump == w->numpoints)
		return;

	c_removed += w->numpoints - nump;
	w->numpoints = nump;
	memcpy (w->p, p, nump*sizeof(p[0]));
}

void WindingPlane (winding_t *w, vector3 *normal, number *dist) {
	vector3	v1, v2;

	VectorSubtract (&w->p[1], &w->p[0], &v1);
	VectorSubtract (&w->p[2], &w->p[0], &v2);
	CrossProduct (&v2, &v1, normal);
	VectorNormalize2(normal, normal);
	*dist = DotProduct (&w->p[0], normal);

}

number WindingArea (winding_t *w) {
	int		i;
	vector3	d1, d2, cross;
	number	total;

	total = 0;
	for (i=2 ; i<w->numpoints ; i++)
	{
		VectorSubtract (&w->p[i-1], &w->p[0], &d1);
		VectorSubtract (&w->p[i], &w->p[0], &d2);
		CrossProduct (&d1, &d2, &cross);
		total += 0.5f * VectorLength ( &cross );
	}
	return total;
}

void	WindingBounds (winding_t *w, vector3 *mins, vector3 *maxs) {
	number	v;
	int		i,j;

	mins->x = mins->y = mins->z =  MAX_MAP_BOUNDS;
	maxs->x = maxs->y = maxs->z = -MAX_MAP_BOUNDS;

	for (i=0 ; i<w->numpoints ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			v = w->p[i].data[j];
			if (v < mins->data[j])	mins->data[j] = v;
			if (v > maxs->data[j])	maxs->data[j] = v;
		}
	}
}

void	WindingCenter (winding_t *w, vector3 *center) {
	int		i;
	float	scale;

	VectorCopy (&vec3_origin, center);
	for (i=0 ; i<w->numpoints ; i++)
		VectorAdd (&w->p[i], center, center);

	scale = 1.0f/w->numpoints;
	VectorScale (center, scale, center);
}

winding_t *BaseWindingForPlane (vector3 *normal, number dist) {
	int		i, x;
	number	max, v;
	vector3	org, vright, vup;
	winding_t	*w;
	
// find the major axis

	max = -MAX_MAP_BOUNDS;
	x = -1;
	for (i=0 ; i<3; i++)
	{
		v = fabsf(normal->data[i]);
		if (v > max)
		{
			x = i;
			max = v;
		}
	}
	if (x==-1)
		Com_Error (ERR_DROP, "BaseWindingForPlane: no axis found");
		
	VectorCopy (&vec3_origin, &vup);	
	switch (x)
	{
	case 0:
	case 1:
		vup.z = 1;
		break;		
	case 2:
		vup.x = 1;
		break;		
	}

	v = DotProduct (&vup, normal);
	VectorMA (&vup, -v, normal, &vup);
	VectorNormalize2(&vup, &vup);
		
	VectorScale (normal, dist, &org);
	
	CrossProduct (&vup, normal, &vright);
	
	VectorScale (&vup, MAX_MAP_BOUNDS, &vup);
	VectorScale (&vright, MAX_MAP_BOUNDS, &vright);

// project a really big	axis aligned box onto the plane
	w = AllocWinding (4);
	
	VectorSubtract (&org, &vright, &w->p[0]);
	VectorAdd (&w->p[0], &vup, &w->p[0]);
	
	VectorAdd (&org, &vright, &w->p[1]);
	VectorAdd (&w->p[1], &vup, &w->p[1]);
	
	VectorAdd (&org, &vright, &w->p[2]);
	VectorSubtract (&w->p[2], &vup, &w->p[2]);
	
	VectorSubtract (&org, &vright, &w->p[3]);
	VectorSubtract (&w->p[3], &vup, &w->p[3]);
	
	w->numpoints = 4;
	
	return w;	
}

winding_t	*CopyWinding (winding_t *w) {
	intptr_t	size;
	winding_t	*c;

	c = AllocWinding (w->numpoints);
	size = (intptr_t) &((winding_t *)0)->p[w->numpoints];
	memcpy (c, w, size);
	return c;
}

winding_t	*ReverseWinding (winding_t *w) {
	int			i;
	winding_t	*c;

	c = AllocWinding (w->numpoints);
	for (i=0 ; i<w->numpoints ; i++)
	{
		VectorCopy (&w->p[w->numpoints-1-i], &c->p[i]);
	}
	c->numpoints = w->numpoints;
	return c;
}


void	ClipWindingEpsilon (winding_t *in, vector3 *normal, number dist, number epsilon, winding_t **front, winding_t **back) {
	number	dists[MAX_POINTS_ON_WINDING+4];
	int		sides[MAX_POINTS_ON_WINDING+4];
	int		counts[3];
	static	number	dot;		// VC 4.2 optimizer bug if not static
	int		i, j;
	vector3	*p1, *p2, mid;
	winding_t	*f, *b;
	int		maxpts;
	
	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (&in->p[i], normal);
		dot -= dist;
		dists[i] = dot;
		if (dot > epsilon)
			sides[i] = SIDE_FRONT;
		else if (dot < -epsilon)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
	*front = *back = NULL;

	if (!counts[0])
	{
		*back = CopyWinding (in);
		return;
	}
	if (!counts[1])
	{
		*front = CopyWinding (in);
		return;
	}

	maxpts = in->numpoints+4;	// cant use counts[0]+2 because
								// of fp grouping errors

	*front = f = AllocWinding (maxpts);
	*back = b = AllocWinding (maxpts);
		
	for (i=0 ; i<in->numpoints ; i++)
	{
		p1 = &in->p[i];
		
		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, &f->p[f->numpoints]);
			f->numpoints++;
			VectorCopy (p1, &b->p[b->numpoints]);
			b->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, &f->p[f->numpoints]);
			f->numpoints++;
		}
		if (sides[i] == SIDE_BACK)
		{
			VectorCopy (p1, &b->p[b->numpoints]);
			b->numpoints++;
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
	// generate a split point
		p2 = &in->p[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
				 if ( normal->data[j] ==  1 )	mid.data[j] =  dist;
			else if ( normal->data[j] == -1 )	mid.data[j] = -dist;
			else								mid.data[j] = p1->data[j] + dot*(p2->data[j]-p1->data[j]);
		}
			
		VectorCopy (&mid, &f->p[f->numpoints]);
		f->numpoints++;
		VectorCopy (&mid, &b->p[b->numpoints]);
		b->numpoints++;
	}
	
	if (f->numpoints > maxpts || b->numpoints > maxpts)
		Com_Error (ERR_DROP, "ClipWinding: points exceeded estimate");
	if (f->numpoints > MAX_POINTS_ON_WINDING || b->numpoints > MAX_POINTS_ON_WINDING)
		Com_Error (ERR_DROP, "ClipWinding: MAX_POINTS_ON_WINDING");
}


void ChopWindingInPlace (winding_t **inout, vector3 *normal, number dist, number epsilon) {
	winding_t	*in;
	number	dists[MAX_POINTS_ON_WINDING+4];
	int		sides[MAX_POINTS_ON_WINDING+4];
	int		counts[3];
	static	number dot;		// VC 4.2 optimizer bug if not static
	int		i, j;
	vector3	*p1, *p2, mid;
	winding_t	*f;
	int		maxpts;

	in = *inout;
	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (&in->p[i], normal);
		dot -= dist;
		dists[i] = dot;
			 if ( dot >  epsilon )	sides[i] = SIDE_FRONT;
		else if ( dot < -epsilon )	sides[i] = SIDE_BACK;
		else						sides[i] = SIDE_ON;
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
	if (!counts[0])
	{
		FreeWinding (in);
		*inout = NULL;
		return;
	}
	if (!counts[1])
		return;		// inout stays the same

	maxpts = in->numpoints+4;	// cant use counts[0]+2 because
								// of fp grouping errors

	f = AllocWinding (maxpts);
		
	for (i=0 ; i<in->numpoints ; i++)
	{
		p1 = &in->p[i];
		
		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, &f->p[f->numpoints]);
			f->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, &f->p[f->numpoints]);
			f->numpoints++;
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
	// generate a split point
		p2 = &in->p[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
				 if ( normal->data[j] ==  1 )	mid.data[j] =  dist;
			else if ( normal->data[j] == -1 )	mid.data[j] = -dist;
			else								mid.data[j] = p1->data[j] + dot*(p2->data[j]-p1->data[j]);
		}
			
		VectorCopy (&mid, &f->p[f->numpoints]);
		f->numpoints++;
	}
	
	if (f->numpoints > maxpts)
		Com_Error (ERR_DROP, "ClipWinding: points exceeded estimate");
	if (f->numpoints > MAX_POINTS_ON_WINDING)
		Com_Error (ERR_DROP, "ClipWinding: MAX_POINTS_ON_WINDING");

	FreeWinding (in);
	*inout = f;
}

// Returns the fragment of in that is on the front side of the cliping plane.
//	The original is freed.
winding_t	*ChopWinding (winding_t *in, vector3 *normal, number dist) {
	winding_t	*f, *b;

	ClipWindingEpsilon (in, normal, dist, ON_EPSILON, &f, &b);
	FreeWinding (in);
	if (b)
		FreeWinding (b);
	return f;
}

void CheckWinding (winding_t *w) {
	int		i, j;
	vector3	*p1, *p2;
	number	d, edgedist;
	vector3	dir, edgenormal, facenormal;
	number	area;
	number	facedist;

	if (w->numpoints < 3)
		Com_Error (ERR_DROP, "CheckWinding: %i points",w->numpoints);
	
	area = WindingArea(w);
	if (area < 1)
		Com_Error (ERR_DROP, "CheckWinding: %f area", area);

	WindingPlane (w, &facenormal, &facedist);
	
	for (i=0 ; i<w->numpoints ; i++)
	{
		p1 = &w->p[i];

		for (j=0 ; j<3 ; j++)
			if (p1->data[j] > MAX_MAP_BOUNDS || p1->data[j] < -MAX_MAP_BOUNDS)
				Com_Error (ERR_DROP, "CheckFace: BUGUS_RANGE: %f",p1->data[j]);

		j = i+1 == w->numpoints ? 0 : i+1;
		
	// check the point is on the face plane
		d = DotProduct (p1, &facenormal) - facedist;
		if (d < -ON_EPSILON || d > ON_EPSILON)
			Com_Error (ERR_DROP, "CheckWinding: point off plane");
	
	// check the edge isnt degenerate
		p2 = &w->p[j];
		VectorSubtract (p2, p1, &dir);
		
		if (VectorLength (&dir) < ON_EPSILON)
			Com_Error (ERR_DROP, "CheckWinding: degenerate edge");
			
		CrossProduct (&facenormal, &dir, &edgenormal);
		VectorNormalize2 (&edgenormal, &edgenormal);
		edgedist = DotProduct (p1, &edgenormal);
		edgedist += ON_EPSILON;
		
	// all other points must be on front side
		for (j=0 ; j<w->numpoints ; j++)
		{
			if (j == i)
				continue;
			d = DotProduct (&w->p[j], &edgenormal);
			if (d > edgedist)
				Com_Error (ERR_DROP, "CheckWinding: non-convex");
		}
	}
}

int		WindingOnPlaneSide (winding_t *w, vector3 *normal, number dist) {
	qboolean	front, back;
	int			i;
	number		d;

	front = qfalse;
	back = qfalse;
	for (i=0 ; i<w->numpoints ; i++)
	{
		d = DotProduct (&w->p[i], normal) - dist;
		if (d < -ON_EPSILON)
		{
			if (front)
				return SIDE_CROSS;
			back = qtrue;
			continue;
		}
		if (d > ON_EPSILON)
		{
			if (back)
				return SIDE_CROSS;
			front = qtrue;
			continue;
		}
	}

	if (back)
		return SIDE_BACK;
	if (front)
		return SIDE_FRONT;
	return SIDE_ON;
}

#define	MAX_HULL_POINTS		128

// Both w and *hull are on the same plane
void	AddWindingToConvexHull( winding_t *w, winding_t **hull, vector3 *normal ) {
	int			i, j, k;
	vector3		*p, *copy;
	vector3		dir;
	float		d;
	int			numHullPoints, numNew;
	vector3		hullPoints[MAX_HULL_POINTS];
	vector3		newHullPoints[MAX_HULL_POINTS];
	vector3		hullDirs[MAX_HULL_POINTS];
	qboolean	hullSide[MAX_HULL_POINTS];
	qboolean	outside;

	if ( !*hull ) {
		*hull = CopyWinding( w );
		return;
	}

	numHullPoints = (*hull)->numpoints;
	memcpy( hullPoints, (*hull)->p, numHullPoints * sizeof(vector3) );

	for ( i = 0 ; i < w->numpoints ; i++ ) {
		p = &w->p[i];

		// calculate hull side vectors
		for ( j = 0 ; j < numHullPoints ; j++ ) {
			k = ( j + 1 ) % numHullPoints;

			VectorSubtract( &hullPoints[k], &hullPoints[j], &dir );
			VectorNormalize2( &dir, &dir );
			CrossProduct( normal, &dir, &hullDirs[j] );
		}

		outside = qfalse;
		for ( j = 0 ; j < numHullPoints ; j++ ) {
			VectorSubtract( p, &hullPoints[j], &dir );
			d = DotProduct( &dir, &hullDirs[j] );
			if ( d >= ON_EPSILON ) {
				outside = qtrue;
			}
			if ( d >= -ON_EPSILON ) {
				hullSide[j] = qtrue;
			} else {
				hullSide[j] = qfalse;
			}
		}

		// if the point is effectively inside, do nothing
		if ( !outside ) {
			continue;
		}

		// find the back side to front side transition
		for ( j = 0 ; j < numHullPoints ; j++ ) {
			if ( !hullSide[ j % numHullPoints ] && hullSide[ (j + 1) % numHullPoints ] ) {
				break;
			}
		}
		if ( j == numHullPoints ) {
			continue;
		}

		// insert the point here
		VectorCopy( p, &newHullPoints[0] );
		numNew = 1;

		// copy over all points that aren't double fronts
		j = (j+1)%numHullPoints;
		for ( k = 0 ; k < numHullPoints ; k++ ) {
			if ( hullSide[ (j+k) % numHullPoints ] && hullSide[ (j+k+1) % numHullPoints ] ) {
				continue;
			}
			copy = &hullPoints[ (j+k+1) % numHullPoints ];
			VectorCopy( copy, &newHullPoints[numNew] );
			numNew++;
		}

		numHullPoints = numNew;
		memcpy( hullPoints, newHullPoints, numHullPoints * sizeof(vector3) );
	}

	FreeWinding( *hull );
	w = AllocWinding( numHullPoints );
	w->numpoints = numHullPoints;
	*hull = w;
	memcpy( w->p, hullPoints, numHullPoints * sizeof(vector3) );
}


