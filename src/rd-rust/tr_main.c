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
// tr_main.c -- main control flow for each frame

#include "tr_local.h"

#include <string.h> // memcpy

trGlobals_t		tr;

static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};


refimport_t	*ri;

// entities that will have procedurally generated surfaces will just
// point at this for their sorting surface
surfaceType_t	entitySurface = SF_ENTITY;

// Returns CULL_IN, CULL_CLIP, or CULL_OUT
int R_CullLocalBox (vector3 bounds[2]) {
	int		i, j;
	vector3	transformed[8];
	float	dists[8];
	vector3	v;
	cplane_t	*frust;
	int			anyBack;
	int			front, back;

	if ( r_nocull->integer ) {
		return CULL_CLIP;
	}

	// transform into world space
	for (i = 0 ; i < 8 ; i++) {
		v.x = bounds[ i    &1].x;
		v.y = bounds[(i>>1)&1].y;
		v.z = bounds[(i>>2)&1].z;

		VectorCopy( &tr.or.origin, &transformed[i] );
		VectorMA( &transformed[i], v.x, &tr.or.axis[0], &transformed[i] );
		VectorMA( &transformed[i], v.y, &tr.or.axis[1], &transformed[i] );
		VectorMA( &transformed[i], v.z, &tr.or.axis[2], &transformed[i] );
	}

	// check against frustum planes
	anyBack = 0;
	for (i = 0 ; i < 4 ; i++) {
		frust = &tr.viewParms.frustum[i];

		front = back = 0;
		for (j = 0 ; j < 8 ; j++) {
			dists[j] = DotProduct(&transformed[j], &frust->normal);
			if ( dists[j] > frust->dist ) {
				front = 1;
				if ( back ) {
					break;		// a point is in front
				}
			} else {
				back = 1;
			}
		}
		if ( !front ) {
			// all points were behind one of the planes
			return CULL_OUT;
		}
		anyBack |= back;
	}

	if ( !anyBack ) {
		return CULL_IN;		// completely inside frustum
	}

	return CULL_CLIP;		// partially clipped
}

int R_CullLocalPointAndRadius( vector3 *pt, float radius ) {
	vector3 transformed;

	R_LocalPointToWorld( pt, &transformed );

	return R_CullPointAndRadius( &transformed, radius );
}

int R_CullPointAndRadius( vector3 *pt, float radius ) {
	int		i;
	float	dist;
	cplane_t	*frust;
	qboolean mightBeClipped = qfalse;

	if ( r_nocull->integer ) {
		return CULL_CLIP;
	}

	// check against frustum planes
	for (i = 0 ; i < 4 ; i++) 
	{
		frust = &tr.viewParms.frustum[i];

		dist = DotProduct( pt, &frust->normal) - frust->dist;
		if ( dist < -radius )
		{
			return CULL_OUT;
		}
		else if ( dist <= radius ) 
		{
			mightBeClipped = qtrue;
		}
	}

	if ( mightBeClipped )
	{
		return CULL_CLIP;
	}

	return CULL_IN;		// completely inside frustum
}

void R_LocalNormalToWorld (vector3 *local, vector3 *world) {
	world->x = local->x * tr.or.axis[0].x + local->y * tr.or.axis[1].x + local->z * tr.or.axis[2].x;
	world->y = local->x * tr.or.axis[0].y + local->y * tr.or.axis[1].y + local->z * tr.or.axis[2].y;
	world->z = local->x * tr.or.axis[0].z + local->y * tr.or.axis[1].z + local->z * tr.or.axis[2].z;
}

void R_LocalPointToWorld (vector3 *local, vector3 *world) {
	world->x = local->x * tr.or.axis[0].x + local->y * tr.or.axis[1].x + local->z * tr.or.axis[2].x + tr.or.origin.x;
	world->y = local->x * tr.or.axis[0].y + local->y * tr.or.axis[1].y + local->z * tr.or.axis[2].y + tr.or.origin.y;
	world->z = local->x * tr.or.axis[0].z + local->y * tr.or.axis[1].z + local->z * tr.or.axis[2].z + tr.or.origin.z;
}

void R_WorldToLocal (vector3 *world, vector3 *local) {
	local->x = DotProduct(world, &tr.or.axis[0]);
	local->y = DotProduct(world, &tr.or.axis[1]);
	local->z = DotProduct(world, &tr.or.axis[2]);
}

void R_TransformModelToClip( const vector3 *src, const float *modelMatrix, const float *projectionMatrix, vector4 *eye, vector4 *dst ) {
	int i;

	for ( i = 0 ; i < 4 ; i++ ) {
		eye->data[i] = 
			src->x * modelMatrix[ i + 0 * 4 ] +
			src->y * modelMatrix[ i + 1 * 4 ] +
			src->z * modelMatrix[ i + 2 * 4 ] +
				 1 * modelMatrix[ i + 3 * 4 ];
	}

	for ( i = 0 ; i < 4 ; i++ ) {
		dst->data[i] = 
			eye->x * projectionMatrix[ i + 0 * 4 ] +
			eye->y * projectionMatrix[ i + 1 * 4 ] +
			eye->z * projectionMatrix[ i + 2 * 4 ] +
			eye->w * projectionMatrix[ i + 3 * 4 ];
	}
}

void R_TransformClipToWindow( const vector4 *clip, const viewParms_t *view, vector4 *normalized, vector4 *window ) {
	normalized->x = ( clip->x / clip->w );
	normalized->y = ( clip->y / clip->w );
	normalized->z = ( clip->z + clip->w ) / ( 2 * clip->w );

	window->x = 0.5f * ( 1.0f + normalized->x ) * view->viewportWidth;
	window->y = 0.5f * ( 1.0f + normalized->y ) * view->viewportHeight;
	window->z = normalized->z;

	window->x = window->x + 0.5f;
	window->y = window->y + 0.5f;
}

void myGlMultMatrix( const float *a, const float *b, float *out ) {
	int i, j;

	for ( i=0; i<4; i++ ) {
		for ( j=0; j<4; j++ ) {
			out[i*4 + j] =	a[i*4 + 0] * b[0*4 + j] +
							a[i*4 + 1] * b[1*4 + j] +
							a[i*4 + 2] * b[2*4 + j] +
							a[i*4 + 3] * b[3*4 + j];
		}
	}
}

// Generates an orientation for an entity and viewParms
//	Does NOT produce any GL calls
//	Called by both the front end and the back end
void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms,
					   orientationr_t *or ) {
	float	glMatrix[16];
	vector3	delta;
	float	axisLength;

	if ( ent->e.reType != RT_MODEL ) {
		*or = viewParms->world;
		return;
	}

	VectorCopy( &ent->e.origin, &or->origin );

	VectorCopy( &ent->e.axis[0], &or->axis[0] );
	VectorCopy( &ent->e.axis[1], &or->axis[1] );
	VectorCopy( &ent->e.axis[2], &or->axis[2] );

	glMatrix[ 0] = or->axis[0].x;
	glMatrix[ 4] = or->axis[1].x;
	glMatrix[ 8] = or->axis[2].x;
	glMatrix[12] = or->origin.x;

	glMatrix[ 1] = or->axis[0].y;
	glMatrix[ 5] = or->axis[1].y;
	glMatrix[ 9] = or->axis[2].y;
	glMatrix[13] = or->origin.y;

	glMatrix[ 2] = or->axis[0].z;
	glMatrix[ 6] = or->axis[1].z;
	glMatrix[10] = or->axis[2].z;
	glMatrix[14] = or->origin.z;

	glMatrix[ 3] = 0;
	glMatrix[ 7] = 0;
	glMatrix[11] = 0;
	glMatrix[15] = 1;

	myGlMultMatrix( glMatrix, viewParms->world.modelMatrix, or->modelMatrix );

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	VectorSubtract( &viewParms->or.origin, &or->origin, &delta );

	// compensate for scale in the axes if necessary
	if ( ent->e.nonNormalizedAxes ) {
		axisLength = VectorLength( &ent->e.axis[0] );
		if ( !axisLength ) {
			axisLength = 0;
		} else {
			axisLength = 1.0f / axisLength;
		}
	} else {
		axisLength = 1.0f;
	}

	or->viewOrigin.x = DotProduct( &delta, &or->axis[0] ) * axisLength;
	or->viewOrigin.y = DotProduct( &delta, &or->axis[1] ) * axisLength;
	or->viewOrigin.z = DotProduct( &delta, &or->axis[2] ) * axisLength;
}

// Sets up the modelview matrix for a given viewParm
void R_RotateForViewer( void )  {
	float	viewerMatrix[16];
	vector3	origin;

	memset (&tr.or, 0, sizeof(tr.or));
	tr.or.axis[0].x = 1;
	tr.or.axis[1].y = 1;
	tr.or.axis[2].z = 1;
	VectorCopy (&tr.viewParms.or.origin, &tr.or.viewOrigin);

	// transform by the camera placement
	VectorCopy( &tr.viewParms.or.origin, &origin );

	viewerMatrix[ 0] = tr.viewParms.or.axis[0].x;
	viewerMatrix[ 4] = tr.viewParms.or.axis[0].y;
	viewerMatrix[ 8] = tr.viewParms.or.axis[0].z;
	viewerMatrix[12] = -origin.x * viewerMatrix[0] + -origin.y * viewerMatrix[4] + -origin.z * viewerMatrix[8];

	viewerMatrix[ 1] = tr.viewParms.or.axis[1].x;
	viewerMatrix[ 5] = tr.viewParms.or.axis[1].y;
	viewerMatrix[ 9] = tr.viewParms.or.axis[1].z;
	viewerMatrix[13] = -origin.x * viewerMatrix[1] + -origin.y * viewerMatrix[5] + -origin.z * viewerMatrix[9];

	viewerMatrix[ 2] = tr.viewParms.or.axis[2].x;
	viewerMatrix[ 6] = tr.viewParms.or.axis[2].y;
	viewerMatrix[10] = tr.viewParms.or.axis[2].z;
	viewerMatrix[14] = -origin.x * viewerMatrix[2] + -origin.y * viewerMatrix[6] + -origin.z * viewerMatrix[10];

	viewerMatrix[ 3] = 0;
	viewerMatrix[ 7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	myGlMultMatrix( viewerMatrix, s_flipMatrix, tr.or.modelMatrix );

	tr.viewParms.world = tr.or;

}

static void R_SetFarClip( void ) {
	float	farthestCornerDistance = 0;
	int		i;

	// if not rendering the world (icons, menus, etc)
	// set a 2k far clip plane
	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		tr.viewParms.zFar = 2048;
		return;
	}

	//
	// set far clipping planes dynamically
	//
	farthestCornerDistance = 0;
	for ( i = 0; i < 8; i++ )
	{
		vector3 v;
		vector3 vecTo;
		float distance;

		if ( i & 1 )	v.x = tr.viewParms.visBounds[0].x;
		else			v.x = tr.viewParms.visBounds[1].x;
		if ( i & 2 )	v.y = tr.viewParms.visBounds[0].y;
		else			v.y = tr.viewParms.visBounds[1].y;
		if ( i & 4 )	v.z = tr.viewParms.visBounds[0].z;
		else			v.z = tr.viewParms.visBounds[1].z;

		VectorSubtract( &v, &tr.viewParms.or.origin, &vecTo );

		distance = vecTo.x * vecTo.x + vecTo.y * vecTo.y + vecTo.z * vecTo.z;

		if ( distance > farthestCornerDistance )
			farthestCornerDistance = distance;
	}
	tr.viewParms.zFar = sqrtf( farthestCornerDistance );
}

// Set up the culling frustum planes for the current view using the results we got from computing the first two rows of the projection matrix.
void R_SetupFrustum (viewParms_t *dest, float xmin, float xmax, float ymax, float zProj, float stereoSep) {
	vector3 ofsorigin;
	float oppleg, adjleg, length;
	int i;
	
	if(stereoSep == 0 && xmin == -xmax)
	{
		// symmetric case can be simplified
		VectorCopy(&dest->or.origin, &ofsorigin);

		length = sqrtf(xmax * xmax + zProj * zProj);
		oppleg = xmax / length;
		adjleg = zProj / length;

		VectorScale(&dest->or.axis[0], oppleg, &dest->frustum[0].normal);
		VectorMA(&dest->frustum[0].normal, adjleg, &dest->or.axis[1], &dest->frustum[0].normal);

		VectorScale(&dest->or.axis[0], oppleg, &dest->frustum[1].normal);
		VectorMA(&dest->frustum[1].normal, -adjleg, &dest->or.axis[1], &dest->frustum[1].normal);
	}
	else
	{
		// In stereo rendering, due to the modification of the projection matrix, dest->or.origin is not the
		// actual origin that we're rendering so offset the tip of the view pyramid.
		VectorMA(&dest->or.origin, stereoSep, &dest->or.axis[1], &ofsorigin);
	
		oppleg = xmax + stereoSep;
		length = sqrtf(oppleg * oppleg + zProj * zProj);
		VectorScale(&dest->or.axis[0], oppleg / length, &dest->frustum[0].normal);
		VectorMA(&dest->frustum[0].normal, zProj / length, &dest->or.axis[1], &dest->frustum[0].normal);

		oppleg = xmin + stereoSep;
		length = sqrtf(oppleg * oppleg + zProj * zProj);
		VectorScale(&dest->or.axis[0], -oppleg / length, &dest->frustum[1].normal);
		VectorMA(&dest->frustum[1].normal, -zProj / length, &dest->or.axis[1], &dest->frustum[1].normal);
	}

	length = sqrtf(ymax * ymax + zProj * zProj);
	oppleg = ymax / length;
	adjleg = zProj / length;

	VectorScale(&dest->or.axis[0], oppleg, &dest->frustum[2].normal);
	VectorMA(&dest->frustum[2].normal, adjleg, &dest->or.axis[2], &dest->frustum[2].normal);

	VectorScale(&dest->or.axis[0], oppleg, &dest->frustum[3].normal);
	VectorMA(&dest->frustum[3].normal, -adjleg, &dest->or.axis[2], &dest->frustum[3].normal);
	
	for (i=0 ; i<4 ; i++) {
		dest->frustum[i].type = PLANE_NON_AXIAL;
		dest->frustum[i].dist = DotProduct (&ofsorigin, &dest->frustum[i].normal);
		SetPlaneSignbits( &dest->frustum[i] );
	}
}

void R_SetupProjection(viewParms_t *dest, float zProj, qboolean computeFrustum) {
	float	xmin, xmax, ymin, ymax;
	float	width, height, stereoSep = r_stereoSeparation->value;

	/*
	 * offset the view origin of the viewer for stereo rendering 
	 * by setting the projection matrix appropriately.
	 */

	if(stereoSep != 0)
	{
		if(dest->stereoFrame == STEREO_LEFT)
			stereoSep = zProj / stereoSep;
		else if(dest->stereoFrame == STEREO_RIGHT)
			stereoSep = zProj / -stereoSep;
		else
			stereoSep = 0;
	}

	ymax = zProj * tanf(dest->fovY * M_PI / 360.0f);
	ymin = -ymax;

	xmax = zProj * tanf(dest->fovX * M_PI / 360.0f);
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;
	
	dest->projectionMatrix[ 0] = 2 * zProj / width;
	dest->projectionMatrix[ 4] = 0;
	dest->projectionMatrix[ 8] = (xmax + xmin + 2 * stereoSep) / width;
	dest->projectionMatrix[12] = 2 * zProj * stereoSep / width;

	dest->projectionMatrix[ 1] = 0;
	dest->projectionMatrix[ 5] = 2 * zProj / height;
	dest->projectionMatrix[ 9] = ( ymax + ymin ) / height;	// normally 0
	dest->projectionMatrix[13] = 0;

	dest->projectionMatrix[ 3] = 0;
	dest->projectionMatrix[ 7] = 0;
	dest->projectionMatrix[11] = -1;
	dest->projectionMatrix[15] = 0;
	
	// Now that we have all the data for the projection matrix we can also setup the view frustum.
	if(computeFrustum)
		R_SetupFrustum(dest, xmin, xmax, ymax, zProj, stereoSep);
}

// Sets the z-component transformation part in the projection matrix
void R_SetupProjectionZ(viewParms_t *dest) {
	float zNear, zFar, depth;
	
	zNear	= r_znear->value;
	zFar	= dest->zFar;	
	depth	= zFar - zNear;

	dest->projectionMatrix[ 2] = 0;
	dest->projectionMatrix[ 6] = 0;
	dest->projectionMatrix[10] = -( zFar + zNear ) / depth;
	dest->projectionMatrix[14] = -2 * zFar * zNear / depth;
}

void R_MirrorPoint (vector3 *in, orientation_t *surface, orientation_t *camera, vector3 *out) {
	int		i;
	vector3	local;
	vector3	transformed;
	float	d;

	VectorSubtract( in, &surface->origin, &local );

	VectorClear( &transformed );
	for ( i = 0 ; i < 3 ; i++ ) {
		d = DotProduct(&local, &surface->axis[i]);
		VectorMA( &transformed, d, &camera->axis[i], &transformed );
	}

	VectorAdd( &transformed, &camera->origin, out );
}

void R_MirrorVector (vector3 *in, orientation_t *surface, orientation_t *camera, vector3 *out) {
	int		i;
	float	d;

	VectorClear( out );
	for ( i = 0 ; i < 3 ; i++ ) {
		d = DotProduct(in, &surface->axis[i]);
		VectorMA( out, d, &camera->axis[i], out );
	}
}

void R_PlaneForSurface (surfaceType_t *surfType, cplane_t *plane) {
	srfTriangles_t	*tri;
	srfPoly_t		*poly;
	drawVert_t		*v1, *v2, *v3;
	vector4			plane4;

	if (!surfType) {
		memset (plane, 0, sizeof(*plane));
		plane->normal.x = 1;
		return;
	}
	switch (*surfType) {
	case SF_FACE:
		*plane = ((srfSurfaceFace_t *)surfType)->plane;
		return;
	case SF_TRIANGLES:
		tri = (srfTriangles_t *)surfType;
		v1 = tri->verts + tri->indexes[0];
		v2 = tri->verts + tri->indexes[1];
		v3 = tri->verts + tri->indexes[2];
		PlaneFromPoints( &plane4, &v1->xyz, &v2->xyz, &v3->xyz );
		VectorCopy( (vector3 *)&plane4, &plane->normal ); 
		plane->dist = plane4.w;
		return;
	case SF_POLY:
		poly = (srfPoly_t *)surfType;
		PlaneFromPoints( &plane4, &poly->verts[0].xyz, &poly->verts[1].xyz, &poly->verts[2].xyz );
		VectorCopy( (vector3 *)&plane4, &plane->normal ); 
		plane->dist = plane4.w;
		return;
	default:
		memset (plane, 0, sizeof(*plane));
		plane->normal.x = 1;
		return;
	}
}

// entityNum is the entity that the portal surface is a part of, which may be moving and rotating.
//	Returns qtrue if it should be mirrored
qboolean R_GetPortalOrientations( drawSurf_t *drawSurf, int entityNum, 
							 orientation_t *surface, orientation_t *camera,
							 vector3 *pvsOrigin, qboolean *mirror ) {
	int			i;
	cplane_t	originalPlane, plane;
	trRefEntity_t	*e;
	float		d;
	vector3		transformed;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != REFENTITYNUM_WORLD ) {
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.or );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( &originalPlane.normal, &plane.normal );
		plane.dist = originalPlane.dist + DotProduct( &plane.normal, &tr.or.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( &originalPlane.normal, &tr.or.origin );
	} else {
		plane = originalPlane;
	}

	VectorCopy( &plane.normal, &surface->axis[0] );
	PerpendicularVector( &surface->axis[1], &surface->axis[0] );
	CrossProduct( &surface->axis[0], &surface->axis[1], &surface->axis[2] );

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0 ; i < tr.refdef.num_entities ; i++ ) {
		e = &tr.refdef.entities[i];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		d = DotProduct( &e->e.origin, &originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64) {
			continue;
		}

		// get the pvsOrigin from the entity
		VectorCopy( &e->e.oldorigin, pvsOrigin );

		// if the entity is just a mirror, don't use as a camera point
		if ( VectorCompare( &e->e.oldorigin, &e->e.origin ) ) {
			VectorScale( &plane.normal, plane.dist, &surface->origin );
			VectorCopy( &surface->origin, &camera->origin );
			VectorSubtract( &vec3_origin, &surface->axis[0], &camera->axis[0] );
			VectorCopy( &surface->axis[1], &camera->axis[1] );
			VectorCopy( &surface->axis[2], &camera->axis[2] );

			*mirror = qtrue;
			return qtrue;
		}

		// project the origin onto the surface plane to get
		// an origin point we can rotate around
		d = DotProduct( &e->e.origin, &plane.normal ) - plane.dist;
		VectorMA( &e->e.origin, -d, &surface->axis[0], &surface->origin );
			
		// now get the camera origin and orientation
		VectorCopy( &e->e.oldorigin, &camera->origin );
		AxisCopy( e->e.axis, camera->axis );
		VectorSubtract( &vec3_origin, &camera->axis[0], &camera->axis[0] );
		VectorSubtract( &vec3_origin, &camera->axis[1], &camera->axis[1] );

		// optionally rotate
		if ( e->e.oldframe ) {
			// if a speed is specified
			if ( e->e.frame ) {
				// continuous rotate
				d = (tr.refdef.time/1000.0f) * e->e.frame;
				VectorCopy( &camera->axis[1], &transformed );
				RotatePointAroundVector( &camera->axis[1], &camera->axis[0], &transformed, d );
				CrossProduct( &camera->axis[0], &camera->axis[1], &camera->axis[2] );
			} else {
				// bobbing rotate, with skinNum being the rotation offset
				d = sinf( tr.refdef.time * 0.003f );
				d = e->e.skinNum + d * 4;
				VectorCopy( &camera->axis[1], &transformed );
				RotatePointAroundVector( &camera->axis[1], &camera->axis[0], &transformed, d );
				CrossProduct( &camera->axis[0], &camera->axis[1], &camera->axis[2] );
			}
		}
		else if ( e->e.skinNum ) {
			d = (float)e->e.skinNum;
			VectorCopy( &camera->axis[1], &transformed );
			RotatePointAroundVector( &camera->axis[1], &camera->axis[0], &transformed, d );
			CrossProduct( &camera->axis[0], &camera->axis[1], &camera->axis[2] );
		}
		*mirror = qfalse;
		return qtrue;
	}

	// if we didn't locate a portal entity, don't render anything.
	// We don't want to just treat it as a mirror, because without a
	// portal entity the server won't have communicated a proper entity set
	// in the snapshot

	// unfortunately, with local movement prediction it is easily possible
	// to see a surface before the server has communicated the matching
	// portal surface entity, so we don't want to print anything here...

	//ri->Printf( PRINT_ALL, "Portal surface without a portal entity\n" );

	return qfalse;
}

static qboolean IsMirror( const drawSurf_t *drawSurf, int entityNum ) {
	int			i;
	cplane_t	originalPlane, plane;
	trRefEntity_t	*e;
	float		d;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != REFENTITYNUM_WORLD )
	{
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.or );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( &originalPlane.normal, &plane.normal );
		plane.dist = originalPlane.dist + DotProduct( &plane.normal, &tr.or.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( &originalPlane.normal, &tr.or.origin );
	} 
	else 
	{
		plane = originalPlane;
	}

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0 ; i < tr.refdef.num_entities ; i++ ) 
	{
		e = &tr.refdef.entities[i];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		d = DotProduct( &e->e.origin, &originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64) {
			continue;
		}

		// if the entity is just a mirror, don't use as a camera point
		if ( VectorCompare( &e->e.oldorigin, &e->e.origin ) ) 
			return qtrue;

		return qfalse;
	}
	return qfalse;
}

// Determines if a surface is completely offscreen.
static qboolean SurfIsOffscreen( const drawSurf_t *drawSurf, vector4 clipDest[128] ) {
	float shortest = 100000000;
	int entityNum;
	int numTriangles;
	shader_t *shader;
	int		fogNum;
	int dlighted;
	vector4 clip, eye;
	int i;
	unsigned int pointOr = 0;
	unsigned int pointAnd = (unsigned int)~0;

	R_RotateForViewer();

	R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted );
	RB_BeginSurface( shader, fogNum );
	rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );

	assert( tess.numVertexes < 128 );

	for ( i = 0; i < tess.numVertexes; i++ )
	{
		int j;
		unsigned int pointFlags = 0;

		R_TransformModelToClip( &tess.xyz[i], tr.or.modelMatrix, tr.viewParms.projectionMatrix, &eye, &clip );

		for ( j=0; j<3; j++ ) {
				 if ( clip.data[j] >=  clip.w )	pointFlags |= (1 << (j*2+0));
			else if ( clip.data[j] <= -clip.w )	pointFlags |= (1 << (j*2+1));
		}
		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if ( pointAnd )
		return qtrue;

	// determine if this surface is backfaced and also determine the distance
	// to the nearest vertex so we can cull based on portal range.  Culling
	// based on vertex distance isn't 100% correct (we should be checking for
	// range to the surface), but it's good enough for the types of portals
	// we have in the game right now.
	numTriangles = tess.numIndexes / 3;

	for ( i = 0; i < tess.numIndexes; i += 3 )
	{
		vector3 normal;
		float len;

		VectorSubtract( &tess.xyz[tess.indexes[i]], &tr.viewParms.or.origin, &normal );

		len = VectorLengthSquared( &normal );			// lose the sqrt
		if ( len < shortest )
			shortest = len;

		if ( DotProduct( &normal, &tess.normal[tess.indexes[i]] ) >= 0 )
			numTriangles--;
	}
	if ( !numTriangles )
		return qtrue;

	// mirrors can early out at this point, since we don't do a fade over distance
	// with them (although we could)
	if ( IsMirror( drawSurf, entityNum ) )
		return qfalse;

	if ( shortest > (tess.shader->portalRange*tess.shader->portalRange) )
		return qtrue;

	return qfalse;
}

// Returns qtrue if another view has been rendered
qboolean R_MirrorViewBySurface (drawSurf_t *drawSurf, int entityNum) {
	vector4			clipDest[128];
	viewParms_t		newParms;
	viewParms_t		oldParms;
	orientation_t	surface, camera;

	// don't recursively mirror
	if (tr.viewParms.isPortal) {
		ri->Printf( PRINT_DEVELOPER, "WARNING: recursive mirror/portal found\n" );
		return qfalse;
	}

	if ( r_noportals->integer || (r_fastsky->integer == 1) ) {
		return qfalse;
	}

	// trivially reject portal/mirror
	if ( SurfIsOffscreen( drawSurf, clipDest ) ) {
		return qfalse;
	}

	// save old viewParms so we can return to it after the mirror view
	oldParms = tr.viewParms;

	newParms = tr.viewParms;
	newParms.isPortal = qtrue;
	if ( !R_GetPortalOrientations( drawSurf, entityNum, &surface, &camera, 
		&newParms.pvsOrigin, &newParms.isMirror ) ) {
		return qfalse;		// bad portal, no portalentity
	}

	R_MirrorPoint (&oldParms.or.origin, &surface, &camera, &newParms.or.origin );

	VectorSubtract( &vec3_origin, &camera.axis[0], &newParms.portalPlane.normal );
	newParms.portalPlane.dist = DotProduct( &camera.origin, &newParms.portalPlane.normal );
	
	R_MirrorVector (&oldParms.or.axis[0], &surface, &camera, &newParms.or.axis[0]);
	R_MirrorVector (&oldParms.or.axis[1], &surface, &camera, &newParms.or.axis[1]);
	R_MirrorVector (&oldParms.or.axis[2], &surface, &camera, &newParms.or.axis[2]);

	// OPTIMIZE: restrict the viewport on the mirrored view

	// render the mirror view
	R_RenderView (&newParms);

	tr.viewParms = oldParms;

	return qtrue;
}

// See if a sprite is inside a fog volume
//QtZ: Patched with https://github.com/dmead/jkaq3/commit/6a552ec589ed20f1f768ccb0af587aa09f3d3f77
int R_SpriteFogNum( trRefEntity_t *ent ) {
	int				i, j;
	fog_t			*fog;
	float			radius;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

#if 0
	if ( ent->e.reType == RT_ORIENTED_QUAD )
		radius = ent->e.data.sprite.radius;
	else
#endif
		radius = ent->e.radius;

	for ( i=1; i<tr.world->numfogs; i++ )
	{
		fog = &tr.world->fogs[i];
		for ( j=0; j<3; j++ ) {
			if ( ent->e.origin.data[j] - radius >= fog->bounds[1].data[j] ||
				 ent->e.origin.data[j] + radius <= fog->bounds[0].data[j] )
				break;
		}
		if ( j == 3 )
			return i;
	}

	return 0;
}

/*

	DRAWSURF SORTING

*/

static QINLINE void R_Radix( int byte, int size, drawSurf_t *source, drawSurf_t *dest ) {
  int           count[ 256 ] = { 0 };
  int           index[ 256 ];
  int           i;
  unsigned char *sortKey = NULL;
  unsigned char *end = NULL;

  sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
  end = sortKey + ( size * sizeof( drawSurf_t ) );
  for( ; sortKey < end; sortKey += sizeof( drawSurf_t ) )
    ++count[ *sortKey ];

  index[ 0 ] = 0;

  for( i = 1; i < 256; ++i )
    index[ i ] = index[ i - 1 ] + count[ i - 1 ];

  sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
  for( i = 0; i < size; ++i, sortKey += sizeof( drawSurf_t ) )
    dest[ index[ *sortKey ]++ ] = source[ i ];
}

// Radix sort with 4 byte size buckets
static void R_RadixSort( drawSurf_t *source, int size ) {
  static drawSurf_t scratch[ MAX_DRAWSURFS ];
#ifdef Q3_LITTLE_ENDIAN
  R_Radix( 0, size, source, scratch );
  R_Radix( 1, size, scratch, source );
  R_Radix( 2, size, source, scratch );
  R_Radix( 3, size, scratch, source );
#else
  R_Radix( 3, size, source, scratch );
  R_Radix( 2, size, scratch, source );
  R_Radix( 1, size, source, scratch );
  R_Radix( 0, size, scratch, source );
#endif //Q3_LITTLE_ENDIAN
}

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, 
				   int fogIndex, int dlightMap ) {
	int			index;

	// instead of checking for overflow, we just mask the index
	// so it wraps around
	index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;
	// the sort data is packed into a single 32 bit value so it can be
	// compared quickly during the qsorting process
	tr.refdef.drawSurfs[index].sort = (shader->sortedIndex << QSORT_SHADERNUM_SHIFT) 
		| tr.shiftedEntityNum | ( fogIndex << QSORT_FOGNUM_SHIFT ) | (int)dlightMap;
	tr.refdef.drawSurfs[index].surface = surface;
	tr.refdef.numDrawSurfs++;
}

void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader, 
					 int *fogNum, int *dlightMap ) {
	*fogNum = ( sort >> QSORT_FOGNUM_SHIFT ) & 31;
	*shader = tr.sortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1) ];
	*entityNum = ( sort >> QSORT_REFENTITYNUM_SHIFT ) & REFENTITYNUM_MASK;
	*dlightMap = sort & 3;
}

void R_SortDrawSurfs( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	shader_t		*shader;
	int				fogNum;
	int				entityNum;
	int				dlighted;
	int				i;

	// it is possible for some views to not have any surfaces
	if ( numDrawSurfs < 1 ) {
		// we still need to add it for hyperspace cases
		R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
		return;
	}

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	if ( numDrawSurfs > MAX_DRAWSURFS ) {
		numDrawSurfs = MAX_DRAWSURFS;
	}

	// sort the drawsurfs by sort type, then orientation, then shader
	R_RadixSort( drawSurfs, numDrawSurfs );

	// check for any pass through drawing, which
	// may cause another view to be rendered first
	for ( i = 0 ; i < numDrawSurfs ; i++ ) {
		R_DecomposeSort( (drawSurfs+i)->sort, &entityNum, &shader, &fogNum, &dlighted );

		if ( shader->sort > SS_PORTAL ) {
			break;
		}

		// no shader should ever have this sort type
		if ( shader->sort == SS_BAD ) {
			ri->Error (ERR_DROP, "Shader '%s'with sort == SS_BAD", shader->name );
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if ( R_MirrorViewBySurface( (drawSurfs+i), entityNum) ) {
			// this is a debug option to see exactly what is being mirrored
			if ( r_portalOnly->integer ) {
				return;
			}
			break;		// only one mirror view at a time
		}
	}

	R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
}

void R_AddEntitySurfaces( void ) {
	trRefEntity_t	*ent;
	shader_t		*shader;

	if ( !r_drawentities->integer ) {
		return;
	}

	for ( tr.currentEntityNum = 0; 
	      tr.currentEntityNum < tr.refdef.num_entities; 
		  tr.currentEntityNum++ ) {
		ent = tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];

		ent->needDlights = qfalse;

		// preshift the value we are going to OR into the drawsurf sort
		tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

		//
		// the weapon model must be handled special --
		// we don't want the hacked weapon position showing in 
		// mirrors, because the true body position will already be drawn
		//
		if ( (ent->e.renderfx & RF_FIRST_PERSON) && tr.viewParms.isPortal) {
			continue;
		}

		// simple generated models, like sprites and beams, are not culled
		switch ( ent->e.reType ) {
		case RT_PORTALSURFACE:
			break;		// don't draw anything
		case RT_SPRITE:
		//QtZ: From JA/EF
		case RT_LINE:
#if 0
		case RT_ORIENTEDLINE:
		case RT_CYLINDER:
		case RT_ENT_CHAIN:
#endif
		case RT_POLY:
		case RT_ORIENTED_QUAD:
		//~QtZ
		case RT_BEAM:
		case RT_ELECTRICITY:
			// self blood sprites, talk balloons, etc should not be drawn in the primary
			// view.  We can't just do this check for all entities, because md3
			// entities may still want to cast shadows from them
			if ( (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal) {
				continue;
			}
			shader = R_GetShaderByHandle( ent->e.customShader );
			R_AddDrawSurf( &entitySurface, shader, R_SpriteFogNum( ent ), 0 );
			break;

		case RT_MODEL:
			// we must set up parts of tr.or for model culling
			R_RotateForEntity( ent, &tr.viewParms, &tr.or );

			tr.currentModel = R_GetModelByHandle( ent->e.hModel );
			if (!tr.currentModel) {
				R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0 );
			} else {
				switch ( tr.currentModel->type ) {
				case MOD_MESH:
					R_AddMD3Surfaces( ent );
					break;
				case MOD_MD4:
					R_AddAnimSurfaces( ent );
					break;
#ifdef RAVENMD4
				case MOD_MDR:
					R_MDRAddAnimSurfaces( ent );
					break;
#endif
				case MOD_IQM:
					R_AddIQMSurfaces( ent );
					break;
				case MOD_BRUSH:
					R_AddBrushModelSurfaces( ent );
					break;
				case MOD_BAD:		// null model axis
					if ( (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal) {
						break;
					}
					R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0 );
					break;
				default:
					ri->Error( ERR_DROP, "R_AddEntitySurfaces: Bad modeltype" );
					break;
				}
			}
			break;
		default:
			ri->Error( ERR_DROP, "R_AddEntitySurfaces: Bad reType" );
		}
	}

}

void R_GenerateDrawSurfs( void ) {
	R_AddWorldSurfaces();

	R_AddPolygonSurfaces();

	// set the projection matrix with the minimum zfar
	// now that we have the world bounded
	// this needs to be done before entities are
	// added, because they use the projection
	// matrix for lod calculation

	// dynamically compute far clip plane distance
	R_SetFarClip();

	// we know the size of the clipping volume. Now set the rest of the projection matrix.
	R_SetupProjectionZ (&tr.viewParms);

	R_AddEntitySurfaces();
}

void R_DebugPolygon( int color, int numPoints, vector3 *points ) {
	int		i;

	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	// draw solid shade

	qglColor3f( (float)(color&1), (float)((color>>1)&1), (float)((color>>2)&1) );
	qglBegin( GL_POLYGON );
	for ( i = 0 ; i < numPoints ; i++ ) {
		qglVertex3fv( (GLfloat *)(points + i * 3) );
	}
	qglEnd();

	// draw wireframe outline
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	qglDepthRange( 0, 0 );
	qglColor3f( 1, 1, 1 );
	qglBegin( GL_POLYGON );
	for ( i = 0 ; i < numPoints ; i++ ) {
		qglVertex3fv( (GLfloat *)(points + i * 3) );
	}
	qglEnd();
	qglDepthRange( 0, 1 );
}

// Visualization aid for movement clipping debugging
void R_DebugGraphics( void ) {
	if ( !r_debugSurface->integer ) {
		return;
	}

	// the render thread can't make callbacks to the main thread
	R_SyncRenderThread();

	GL_Bind( tr.whiteImage);
	GL_Cull( CT_FRONT_SIDED );
	ri->CM_DrawDebugSurface( R_DebugPolygon );
}


// A view may be either the actual camera view, or a mirror / remote location
void R_RenderView (viewParms_t *parms) {
	int		firstDrawSurf;

	if ( parms->viewportWidth <= 0 || parms->viewportHeight <= 0 ) {
		return;
	}

	tr.viewCount++;

	tr.viewParms = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount = tr.frameCount;

	firstDrawSurf = tr.refdef.numDrawSurfs;

	tr.viewCount++;

	// set viewParms.world
	R_RotateForViewer();

	R_SetupProjection(&tr.viewParms, r_zproj->value, qtrue);

	R_GenerateDrawSurfs();

	R_SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf );

	// draw main system development information (surface outlines, etc)
	R_DebugGraphics();
}



