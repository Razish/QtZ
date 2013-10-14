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
// tr_sky.c
#include "tr_local.h"

#define SKY_SUBDIVISIONS		8
#define HALF_SKY_SUBDIVISIONS	(SKY_SUBDIVISIONS/2)

static float s_cloudTexCoords[6][SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1][2];
static float s_cloudTexP[6][SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1];

/*

	POLYGON TO BOX SIDE PROJECTION

*/

static vector3 sky_clip[6] = {
	{  1,  1,  0 },
	{  1, -1,  0 },
	{  0, -1,  1 },
	{  0,  1,  1 },
	{  1,  0,  1 },
	{ -1,  0,  1 } 
};

static float	sky_mins[2][6], sky_maxs[2][6];
static float	sky_min, sky_max;

static void AddSkyPolygon (int nump, vector3 *vecs)  {
	int		i,j;
	vector3	v, av;
	float	s, t, dv;
	int		axis;
	vector3	*vp;
	// s = [0]/[2], t = [1]/[2]
	static int	vec_to_st[6][3] =
	{
		{-2,3,1},
		{2,3,-1},

		{1,3,2},
		{-1,3,-2},

		{-2,-1,3},
		{-2,1,-3}

	//	{-1,2,3},
	//	{1,2,-3}
	};

	// decide which face it maps to
	VectorCopy (&vec3_origin, &v);
	for (i=0, vp=vecs ; i<nump ; i++, vp++)
	{
		VectorAdd (vp, &v, &v);
	}
	av.x = fabsf(v.x);
	av.y = fabsf(v.y);
	av.z = fabsf(v.z);
	if (av.x > av.y && av.x > av.z) {
		if ( v.x < 0 )	axis = 1;
		else			axis = 0;
	}
	else if (av.y > av.z && av.y > av.x) {
		if ( v.y < 0 )	axis = 3;
		else			axis = 2;
	}
	else {
		if ( v.z < 0 )	axis = 5;
		else			axis = 4;
	}

	// project new texture coords
	for (i=0 ; i<nump ; i++, vecs++)
	{
		j = vec_to_st[axis][2];
		if ( j > 0 )	dv =  vecs->data[j - 1];
		else			dv = -vecs->data[-j - 1];
		if ( dv < 0.001f ) continue; // don't divide by zero

		j = vec_to_st[axis][0];
		if ( j < 0 )	s = -vecs->data[-j -1] / dv;
		else			s =  vecs->data[j-1] / dv;

		j = vec_to_st[axis][1];
		if ( j < 0 )	t = -vecs->data[-j -1] / dv;
		else			t =  vecs->data[j-1] / dv;

		if ( s < sky_mins[0][axis] )	sky_mins[0][axis] = s;
		if ( t < sky_mins[1][axis] )	sky_mins[1][axis] = t;
		if ( s > sky_maxs[0][axis] )	sky_maxs[0][axis] = s;
		if ( t > sky_maxs[1][axis] )	sky_maxs[1][axis] = t;
	}
}

#define	ON_EPSILON		0.1f			// point on plane side epsilon
#define	MAX_CLIP_VERTS	64

static void ClipSkyPolygon (int nump, vector3 *vecs, int stage)  {
	vector3	*norm, *v;
	qboolean	front, back;
	float	d, e;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	vector3	newv[2][MAX_CLIP_VERTS];
	int		newc[2];
	int		i, j;

	if (nump > MAX_CLIP_VERTS-2)
		ri->Error (ERR_DROP, "ClipSkyPolygon: MAX_CLIP_VERTS");
	if (stage == 6)
	{	// fully clipped, so draw it
		AddSkyPolygon (nump, vecs);
		return;
	}

	front = back = qfalse;
	norm = &sky_clip[stage];
	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		d = DotProduct (v, norm);
		if (d > ON_EPSILON) {
			front = qtrue;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON) {
			back = qtrue;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back)
	{	// not clipped
		ClipSkyPolygon (nump, vecs, stage+1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (vecs, (vecs+(i*3)) );
	newc[0] = newc[1] = 0;

	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy (v, &newv[0][newc[0]]);
			newc[0]++;
			break;
		case SIDE_BACK:
			VectorCopy (v, &newv[1][newc[1]]);
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy (v, &newv[0][newc[0]]);
			newc[0]++;
			VectorCopy (v, &newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{
			e = v->data[j] + d*(v->data[j+3] - v->data[j]);
			newv[0][newc[0]].data[j] = e;
			newv[1][newc[1]].data[j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon (newc[0], &newv[0][0], stage+1);
	ClipSkyPolygon (newc[1], &newv[1][0], stage+1);
}

static void ClearSkyBox( void ) {
	int		i;

	for (i=0 ; i<6 ; i++) {
		sky_mins[0][i] = sky_mins[1][i] = 9999;
		sky_maxs[0][i] = sky_maxs[1][i] = -9999;
	}
}

void RB_ClipSkyPolygons( shaderCommands_t *input ) {
	vector3		p[5];	// need one extra point for clipping
	int			i, j;

	ClearSkyBox();

	for ( i = 0; i < input->numIndexes; i += 3 )
	{
		for (j = 0 ; j < 3 ; j++) 
		{
			VectorSubtract( &input->xyz[input->indexes[i+j]],
							&backEnd.viewParms.or.origin, 
							&p[j] );
		}
		ClipSkyPolygon( 3, &p[0], 0 );
	}
}

/*

	CLOUD VERTEX GENERATION

*/

// Parms: s, t range from -1 to 1
static void MakeSkyVec( float s, float t, int axis, float outSt[2], vector3 *outXYZ ) {
	// 1 = s, 2 = t, 3 = 2048
	static int	st_to_vec[6][3] =
	{
		{3,-1,2},
		{-3,1,2},

		{1,3,2},
		{-1,-3,2},

		{-2,-1,3},		// 0 degrees yaw, look straight up
		{2,-1,-3}		// look straight down
	};

	vector3		b;
	int			j, k;
	float	boxSize;

	boxSize = backEnd.viewParms.zFar / 1.75f;		// div sqrt(3)
	b.x = s*boxSize;
	b.y = t*boxSize;
	b.z = boxSize;

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if ( k < 0 )	outXYZ->data[j] = -b.data[-k - 1];
		else			outXYZ->data[j] =  b.data[ k - 1];
	}

	// avoid bilerp seam
	s = (s+1)*0.5f;
	t = (t+1)*0.5f;
		 if ( s < sky_min )	s = sky_min;
	else if ( s > sky_max )	s = sky_max;
		 if ( t < sky_min )	t = sky_min;
	else if ( t > sky_max )	t = sky_max;

	t = 1.0f - t;


	if ( outSt ) {
		outSt[0] = s;
		outSt[1] = t;
	}
}

static int	sky_texorder[6] = {0,2,1,3,4,5};
static vector3	s_skyPoints[SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1];
static float	s_skyTexCoords[SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1][2];

static void DrawSkySide( struct image_s *image, const int mins[2], const int maxs[2] ) {
	int s, t;

	GL_Bind( image );

	for ( t = mins[1]+HALF_SKY_SUBDIVISIONS; t < maxs[1]+HALF_SKY_SUBDIVISIONS; t++ )
	{
		qglBegin( GL_TRIANGLE_STRIP );

		for ( s = mins[0]+HALF_SKY_SUBDIVISIONS; s <= maxs[0]+HALF_SKY_SUBDIVISIONS; s++ )
		{
			qglTexCoord2fv( s_skyTexCoords[t][s] );
			qglVertex3fv( (GLfloat *)&s_skyPoints[t][s] );

			qglTexCoord2fv( s_skyTexCoords[t+1][s] );
			qglVertex3fv( (GLfloat *)&s_skyPoints[t+1][s] );
		}

		qglEnd();
	}
}

static void DrawSkyBox( shader_t *shader ) {
	int		i;

	sky_min = 0;
	sky_max = 1;

	memset( s_skyTexCoords, 0, sizeof( s_skyTexCoords ) );

	for (i=0 ; i<6 ; i++)
	{
		int sky_mins_subd[2], sky_maxs_subd[2];
		int s, t;

		sky_mins[0][i] = floorf( sky_mins[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_mins[1][i] = floorf( sky_mins[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[0][i] = ceilf( sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[1][i] = ceilf( sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;

		if ( ( sky_mins[0][i] >= sky_maxs[0][i] ) ||
			 ( sky_mins[1][i] >= sky_maxs[1][i] ) )
		{
			continue;
		}

		sky_mins_subd[0] = (int)(sky_mins[0][i] * HALF_SKY_SUBDIVISIONS);
		sky_mins_subd[1] = (int)(sky_mins[1][i] * HALF_SKY_SUBDIVISIONS);
		sky_maxs_subd[0] = (int)(sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS);
		sky_maxs_subd[1] = (int)(sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS);
			 if ( sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS )	sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_mins_subd[0] >  HALF_SKY_SUBDIVISIONS )	sky_mins_subd[0] =  HALF_SKY_SUBDIVISIONS;
			 if ( sky_mins_subd[1] < -HALF_SKY_SUBDIVISIONS )	sky_mins_subd[1] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_mins_subd[1] >  HALF_SKY_SUBDIVISIONS )	sky_mins_subd[1] =  HALF_SKY_SUBDIVISIONS;
			 if ( sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS )	sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_maxs_subd[0] >  HALF_SKY_SUBDIVISIONS )	sky_maxs_subd[0] =  HALF_SKY_SUBDIVISIONS;
			 if ( sky_maxs_subd[1] < -HALF_SKY_SUBDIVISIONS )	sky_maxs_subd[1] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_maxs_subd[1] >  HALF_SKY_SUBDIVISIONS )	sky_maxs_subd[1] =  HALF_SKY_SUBDIVISIONS;

		//
		// iterate through the subdivisions
		//
		for ( t = sky_mins_subd[1]+HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1]+HALF_SKY_SUBDIVISIONS; t++ )
		{
			for ( s = sky_mins_subd[0]+HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0]+HALF_SKY_SUBDIVISIONS; s++ )
			{
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							i, 
							s_skyTexCoords[t][s], 
							&s_skyPoints[t][s] );
			}
		}

		DrawSkySide( shader->sky.outerbox[sky_texorder[i]],
			         sky_mins_subd,
					 sky_maxs_subd );
	}

}

static void FillCloudySkySide( const int mins[2], const int maxs[2], qboolean addIndexes ) {
	int s, t;
	int vertexStart = tess.numVertexes;
	int tHeight, sWidth;

	tHeight = maxs[1] - mins[1] + 1;
	sWidth = maxs[0] - mins[0] + 1;

	for ( t = mins[1]+HALF_SKY_SUBDIVISIONS; t <= maxs[1]+HALF_SKY_SUBDIVISIONS; t++ )
	{
		for ( s = mins[0]+HALF_SKY_SUBDIVISIONS; s <= maxs[0]+HALF_SKY_SUBDIVISIONS; s++ )
		{
			VectorAdd( &s_skyPoints[t][s], &backEnd.viewParms.or.origin, &tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0].x = s_skyTexCoords[t][s][0];
			tess.texCoords[tess.numVertexes][0].y = s_skyTexCoords[t][s][1];

			tess.numVertexes++;

			if ( tess.numVertexes >= SHADER_MAX_VERTEXES )
				ri->Error( ERR_DROP, "SHADER_MAX_VERTEXES hit in FillCloudySkySide()" );
		}
	}

	// only add indexes for one pass, otherwise it would draw multiple times for each pass
	if ( addIndexes ) {
		for ( t = 0; t < tHeight-1; t++ ) {	
			for ( s = 0; s < sWidth-1; s++ ) {
				tess.indexes[tess.numIndexes++] = vertexStart + s + t * ( sWidth );
				tess.indexes[tess.numIndexes++] = vertexStart + s + ( t + 1 ) * ( sWidth );
				tess.indexes[tess.numIndexes++] = vertexStart + s + 1 + t * ( sWidth );

				tess.indexes[tess.numIndexes++] = vertexStart + s + ( t + 1 ) * ( sWidth );
				tess.indexes[tess.numIndexes++] = vertexStart + s + 1 + ( t + 1 ) * ( sWidth );
				tess.indexes[tess.numIndexes++] = vertexStart + s + 1 + t * ( sWidth );
			}
		}
	}
}

static void FillCloudBox( const shader_t *shader, int stage ) {
	int i;

	for ( i =0; i < 6; i++ )
	{
		int sky_mins_subd[2], sky_maxs_subd[2];
		int s, t;
		float MIN_T;

		if ( 1 ) // FIXME? shader->sky.fullClouds )
		{
			MIN_T = -HALF_SKY_SUBDIVISIONS;

			// still don't want to draw the bottom, even if fullClouds
			if ( i == 5 )
				continue;
		}
		else
		{
			switch( i )
			{
			case 0:
			case 1:
			case 2:
			case 3:
				MIN_T = -1;
				break;
			case 5:
				// don't draw clouds beneath you
				continue;
			case 4:		// top
			default:
				MIN_T = -HALF_SKY_SUBDIVISIONS;
				break;
			}
		}

		sky_mins[0][i] = floorf( sky_mins[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_mins[1][i] = floorf( sky_mins[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[0][i] = ceilf( sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[1][i] = ceilf( sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;

		if ( ( sky_mins[0][i] >= sky_maxs[0][i] ) ||
			 ( sky_mins[1][i] >= sky_maxs[1][i] ) )
		{
			continue;
		}

		sky_mins_subd[0] = ri->ftol(sky_mins[0][i] * HALF_SKY_SUBDIVISIONS);
		sky_mins_subd[1] = ri->ftol(sky_mins[1][i] * HALF_SKY_SUBDIVISIONS);
		sky_maxs_subd[0] = ri->ftol(sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS);
		sky_maxs_subd[1] = ri->ftol(sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS);

			 if ( sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS )		sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_mins_subd[0] >  HALF_SKY_SUBDIVISIONS )		sky_mins_subd[0] =  HALF_SKY_SUBDIVISIONS;
			 if ( sky_mins_subd[1] <  (int)MIN_T )					sky_mins_subd[1] =  (int)MIN_T;
		else if ( sky_mins_subd[1] >  HALF_SKY_SUBDIVISIONS )		sky_mins_subd[1] =  HALF_SKY_SUBDIVISIONS;

			 if ( sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS )		sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_maxs_subd[0] >  HALF_SKY_SUBDIVISIONS )		sky_maxs_subd[0] =  HALF_SKY_SUBDIVISIONS;
			 if ( sky_maxs_subd[1] <  (int)MIN_T )					sky_maxs_subd[1] =  (int)MIN_T;
		else if ( sky_maxs_subd[1] >  HALF_SKY_SUBDIVISIONS )		sky_maxs_subd[1] =  HALF_SKY_SUBDIVISIONS;

		//
		// iterate through the subdivisions
		//
		for ( t = sky_mins_subd[1]+HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1]+HALF_SKY_SUBDIVISIONS; t++ )
		{
			for ( s = sky_mins_subd[0]+HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0]+HALF_SKY_SUBDIVISIONS; s++ )
			{
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							i, 
							NULL,
							&s_skyPoints[t][s] );

				s_skyTexCoords[t][s][0] = s_cloudTexCoords[i][t][s][0];
				s_skyTexCoords[t][s][1] = s_cloudTexCoords[i][t][s][1];
			}
		}

		// only add indexes for first stage
		FillCloudySkySide( sky_mins_subd, sky_maxs_subd, ( stage == 0 ) );
	}
}

void R_BuildCloudData( shaderCommands_t *input ) {
	int			i;
	shader_t	*shader;

	shader = input->shader;

	assert( shader->isSky );

	sky_min = 1.0f / 256.0f;		// FIXME: not correct?
	sky_max = 255.0f / 256.0f;

	// set up for drawing
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	if ( shader->sky.cloudHeight )
	{
		for ( i = 0; i < MAX_SHADER_STAGES; i++ )
		{
			if ( !tess.xstages[i] )
				break;
			FillCloudBox( shader, i );
		}
	}
}

#define SQR( a ) ((a)*(a))

// Called when a sky shader is parsed
void R_InitSkyTexCoords( float heightCloud ) {
	int i, s, t;
	float radiusWorld = 4096;
	float p;
	float sRad, tRad;
	vector3 skyVec;
	vector3 v;

	// init zfar so MakeSkyVec works even though
	// a world hasn't been bounded
	backEnd.viewParms.zFar = 1024;

	for ( i = 0; i < 6; i++ )
	{
		for ( t = 0; t <= SKY_SUBDIVISIONS; t++ )
		{
			for ( s = 0; s <= SKY_SUBDIVISIONS; s++ )
			{
				// compute vector from view origin to sky side integral point
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							i, 
							NULL,
							&skyVec );

				// compute parametric value 'p' that intersects with cloud layer
				p = ( 1.0f / ( 2 * DotProduct( &skyVec, &skyVec ) ) ) *
					( -2 * skyVec.z * radiusWorld + 
					2 * sqrtf( SQR( skyVec.z ) * SQR( radiusWorld ) + 
					2 * SQR( skyVec.x ) * radiusWorld * heightCloud +
					SQR( skyVec.x ) * SQR( heightCloud ) + 
					2 * SQR( skyVec.y ) * radiusWorld * heightCloud +
					SQR( skyVec.y ) * SQR( heightCloud ) + 
					2 * SQR( skyVec.z ) * radiusWorld * heightCloud +
					SQR( skyVec.z ) * SQR( heightCloud ) ) );

				s_cloudTexP[i][t][s] = p;

				// compute intersection point based on p
				VectorScale( &skyVec, p, &v );
				v.z += radiusWorld;

				// compute vector from world origin to intersection point 'v'
				VectorNormalize( &v );

				sRad = Q_acos( v.x );
				tRad = Q_acos( v.y );

				s_cloudTexCoords[i][t][s][0] = sRad;
				s_cloudTexCoords[i][t][s][1] = tRad;
			}
		}
	}
}

void RB_DrawSun( float scale, shader_t *shader ) {
	float		size;
	float		dist;
	vector3		origin, vec1, vec2;
	byte		sunColor[4] = { 255, 255, 255, 255 };

	if ( !backEnd.skyRenderedThisView ) {
		return;
	}

	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	qglTranslatef (backEnd.viewParms.or.origin.x, backEnd.viewParms.or.origin.y, backEnd.viewParms.or.origin.z);

	dist = 	backEnd.viewParms.zFar / 1.75f;		// div sqrt(3)
	size = dist * scale;

	VectorScale( &tr.sunDirection, dist, &origin );
	PerpendicularVector( &vec1, &tr.sunDirection );
	CrossProduct( &tr.sunDirection, &vec1, &vec2 );

	VectorScale( &vec1, size, &vec1 );
	VectorScale( &vec2, size, &vec2 );

	// farthest depth range
	qglDepthRange( 1.0, 1.0 );

	RB_BeginSurface( shader, 0 );
	RB_AddQuadStamp( &origin, &vec1, &vec2, sunColor );
	RB_EndSurface();

	// back to normal depth range
	qglDepthRange( 0.0, 1.0 );
}

// All of the visible sky triangles are in tess
//	Other things could be stuck in here, like birds in the sky, etc
void RB_StageIteratorSky( void ) {
	if ( r_fastsky->integer ) {
		return;
	}

	// go through all the polygons and project them onto
	// the sky box to see which blocks on each side need
	// to be drawn
	RB_ClipSkyPolygons( &tess );

	// r_showsky will let all the sky blocks be drawn in
	// front of everything to allow developers to see how
	// much sky is getting sucked in
	if ( r_showsky->integer )
		qglDepthRange( 0.0, 0.0 );
	else
		qglDepthRange( 1.0, 1.0 );

	// draw the outer skybox
	if ( tess.shader->sky.outerbox[0] && tess.shader->sky.outerbox[0] != tr.defaultImage ) {
		qglColor3f( tr.identityLight, tr.identityLight, tr.identityLight );
		
		qglPushMatrix();
		GL_State( 0 );
		qglTranslatef (backEnd.viewParms.or.origin.x, backEnd.viewParms.or.origin.y, backEnd.viewParms.or.origin.z);

		DrawSkyBox( tess.shader );

		qglPopMatrix();
	}

	// generate the vertexes for all the clouds, which will be drawn
	// by the generic shader routine
	R_BuildCloudData( &tess );

	RB_StageIteratorGeneric();

	// draw the inner skybox


	// back to normal depth range
	qglDepthRange( 0.0, 1.0 );

	// note that sky was drawn so we will draw a sun later
	backEnd.skyRenderedThisView = qtrue;
}

