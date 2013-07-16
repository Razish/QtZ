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

typedef struct winding_s {
	int		numpoints;
	vector3	p[4];		// variable sized
} winding_t;

#define	MAX_POINTS_ON_WINDING	64

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2
#define	SIDE_CROSS	3

#define	CLIP_EPSILON	0.1f

#define MAX_MAP_BOUNDS			65535

// you can define on_epsilon in the makefile as tighter
#ifndef	ON_EPSILON
#define	ON_EPSILON	0.1f
#endif

winding_t	*AllocWinding			( int points );
number		 WindingArea			( winding_t *w );
void		 WindingCenter			( winding_t *w, vector3 *center );
void		 ClipWindingEpsilon		( winding_t *in, vector3 *normal, number dist, number epsilon, winding_t **front, winding_t **back );
winding_t	*ChopWinding			( winding_t *in, vector3 *normal, number dist );
winding_t	*CopyWinding			( winding_t *w );
winding_t	*ReverseWinding			( winding_t *w );
winding_t	*BaseWindingForPlane	( vector3 *normal, number dist);
void		 CheckWinding			( winding_t *w );
void		 WindingPlane			( winding_t *w, vector3 *normal, number *dist);
void		 RemoveColinearPoints	( winding_t *w );
int			 WindingOnPlaneSide		( winding_t *w, vector3 *normal, number dist);
void		 FreeWinding			( winding_t *w );
void		 WindingBounds			( winding_t *w, vector3 *mins, vector3 *maxs);
void		 AddWindingToConvexHull	( winding_t *w, winding_t **hull, vector3 *normal );
void		 ChopWindingInPlace		( winding_t **w, vector3 *normal, number dist, number epsilon);
