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
// tr_shade_calc.c

#include "tr_local.h"
#if defined(idppc_altivec) && !defined(MACOS_X)
#include <altivec.h>
#endif


#define	WAVEVALUE( table, base, amplitude, phase, freq )  ((base) + table[ ri->ftol( ( ( (phase) + tess.shaderTime * (freq) ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * (amplitude))

static float *TableForFunc( genFunc_t func ) 
{
	switch ( func )
	{
	case GF_SIN:
		return tr.sinTable;
	case GF_TRIANGLE:
		return tr.triangleTable;
	case GF_SQUARE:
		return tr.squareTable;
	case GF_SAWTOOTH:
		return tr.sawToothTable;
	case GF_INVERSE_SAWTOOTH:
		return tr.inverseSawToothTable;
	case GF_NONE:
	default:
		break;
	}

	ri->Error( ERR_DROP, "TableForFunc called with invalid function '%d' in shader '%s'", func, tess.shader->name );
	return NULL;
}

// Evaluates a given waveForm_t, referencing backEnd.refdef.time directly
static float EvalWaveForm( const waveForm_t *wf ) 
{
	float	*table;

	table = TableForFunc( wf->func );

	return WAVEVALUE( table, wf->base, wf->amplitude, wf->phase, wf->frequency );
}

static float EvalWaveFormClamped( const waveForm_t *wf )
{
	float glow  = EvalWaveForm( wf );

	if ( glow < 0 )
	{
		return 0;
	}

	if ( glow > 1 )
	{
		return 1;
	}

	return glow;
}

void RB_CalcStretchTexCoords( const waveForm_t *wf, float *st )
{
	float p;
	texModInfo_t tmi;

	p = 1.0f / EvalWaveForm( wf );

	tmi.matrix[0][0] = p;
	tmi.matrix[1][0] = 0;
	tmi.translate[0] = 0.5f - 0.5f * p;

	tmi.matrix[0][1] = 0;
	tmi.matrix[1][1] = p;
	tmi.translate[1] = 0.5f - 0.5f * p;

	RB_CalcTransformTexCoords( &tmi, st );
}

/*

	DEFORMATIONS

*/

void RB_CalcDeformVertexes( deformStage_t *ds )
{
	int i;
	vector3	offset;
	float	scale;
	vector3	*xyz = &tess.xyz[0], *normal = &tess.normal[0];
	float	*table;

	if ( ds->deformationWave.frequency == 0 )
	{
		scale = EvalWaveForm( &ds->deformationWave );

		for ( i = 0; i < tess.numVertexes; i++, xyz++, normal++ )
		{
			VectorScale( normal, scale, &offset );
			
			xyz->x += offset.x;
			xyz->y += offset.y;
			xyz->z += offset.z;
		}
	}
	else
	{
		table = TableForFunc( ds->deformationWave.func );

		for ( i = 0; i < tess.numVertexes; i++, xyz++, normal++ )
		{
			float off = ( xyz->x + xyz->y + xyz->z ) * ds->deformationSpread;

			scale = WAVEVALUE( table, ds->deformationWave.base, 
				ds->deformationWave.amplitude,
				ds->deformationWave.phase + off,
				ds->deformationWave.frequency );

			VectorScale( normal, scale, &offset );
			
			xyz->x += offset.x;
			xyz->y += offset.y;
			xyz->z += offset.z;
		}
	}
}

// Wiggle the normals for wavy environment mapping
void RB_CalcDeformNormals( deformStage_t *ds ) {
	int i;
	float	scale;
	vector3	*xyz = &tess.xyz[0], *normal = &tess.normal[0];

	for ( i = 0; i < tess.numVertexes; i++, xyz++, normal++ ) {
		scale = 0.98f;
		scale = R_NoiseGet4f( 000 + xyz->x * scale, xyz->y * scale, xyz->z * scale, tess.shaderTime * ds->deformationWave.frequency );
		normal->x += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f( 100 + xyz->x * scale, xyz->y * scale, xyz->z * scale, tess.shaderTime * ds->deformationWave.frequency );
		normal->y += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f( 200 + xyz->z * scale, xyz->y * scale, xyz->z * scale, tess.shaderTime * ds->deformationWave.frequency );
		normal->z += ds->deformationWave.amplitude * scale;

		VectorNormalizeFast( normal );
	}
}

void RB_CalcBulgeVertexes( deformStage_t *ds ) {
	int i;
	const vector2 *st = NULL;
	vector3 *xyz = NULL, *normal = NULL;
	float now;

	now = backEnd.refdef.time * ds->bulgeSpeed * 0.001f;

	for ( i = 0; i < tess.numVertexes; i++ ) {
		int		off;
		float scale;
		st = &tess.texCoords[i][0];
		xyz = &tess.xyz[i];
		normal = &tess.normal[i];


		off = (int)((float)( FUNCTABLE_SIZE / (M_PI*2) ) * ( st[0].x * ds->bulgeWidth + now ));

		scale = tr.sinTable[ off & FUNCTABLE_MASK ] * ds->bulgeHeight;
			
		xyz->x += normal->x * scale;
		xyz->y += normal->y * scale;
		xyz->z += normal->z * scale;
	}
}


// A deformation that can move an entire surface along a wave path
void RB_CalcMoveVertexes( deformStage_t *ds ) {
	int			i;
	vector3		*xyz;
	float		*table;
	float		scale;
	vector3		offset;

	table = TableForFunc( ds->deformationWave.func );

	scale = WAVEVALUE( table, ds->deformationWave.base, 
		ds->deformationWave.amplitude,
		ds->deformationWave.phase,
		ds->deformationWave.frequency );

	VectorScale( &ds->moveVector, scale, &offset );

	xyz = &tess.xyz[0];
	for ( i = 0; i < tess.numVertexes; i++, xyz++ ) {
		VectorAdd( xyz, &offset, xyz );
	}
}


// Change a polygon into a bunch of text polygons
void DeformText( const char *text ) {
	int		i;
	vector3	origin, width, height;
	int		len;
	int		ch;
	byte	color[4];
	float	bottom, top;
	vector3	mid;

	height.x = 0;
	height.y = 0;
	height.z = -1;
	CrossProduct( &tess.normal[0], &height, &width );

	// find the midpoint of the box
	VectorClear( &mid );
	bottom = 999999;
	top = -999999;
	for ( i = 0 ; i < 4 ; i++ ) {
		VectorAdd( &tess.xyz[i], &mid, &mid );
		if ( tess.xyz[i].z < bottom ) {
			bottom = tess.xyz[i].z;
		}
		if ( tess.xyz[i].z > top ) {
			top = tess.xyz[i].z;
		}
	}
	VectorScale( &mid, 0.25f, &origin );

	// determine the individual character size
	height.x = 0;
	height.y = 0;
	height.z = ( top - bottom ) * 0.5f;

	VectorScale( &width, height.z * -0.75f, &width );

	// determine the starting position
	len = strlen( text );
	VectorMA( &origin, (float)(len-1), &width, &origin );

	// clear the shader indexes
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	color[0] = color[1] = color[2] = color[3] = 255;

	// draw each character
	for ( i = 0 ; i < len ; i++ ) {
		ch = text[i];
		ch &= 255;

		if ( ch != ' ' ) {
			int		row, col;
			float	frow, fcol, size;

			row = ch>>4;
			col = ch&15;

			frow = row*0.0625f;
			fcol = col*0.0625f;
			size = 0.0625f;

			RB_AddQuadStampExt( &origin, &width, &height, color, fcol, frow, fcol + size, frow + size );
		}
		VectorMA( &origin, -2, &width, &origin );
	}
}

static void GlobalVectorToLocal( const vector3 *in, vector3 *out ) {
	out->x = DotProduct( in, &backEnd.or.axis[0] );
	out->y = DotProduct( in, &backEnd.or.axis[1] );
	out->z = DotProduct( in, &backEnd.or.axis[2] );
}

// Assuming all the triangles for this shader are independant quads, rebuild them as forward facing sprites
static void AutospriteDeform( void ) {
	int		i;
	int		oldVerts;
	vector3	*xyz;
	vector3	mid, delta;
	float	radius;
	vector3	left, up;
	vector3	leftDir, upDir;

	if ( tess.numVertexes & 3 )
		ri->Printf( PRINT_WARNING, "Autosprite shader %s had odd vertex count\n", tess.shader->name );
	if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 )
		ri->Printf( PRINT_WARNING, "Autosprite shader %s had odd index count\n", tess.shader->name );

	oldVerts = tess.numVertexes;
	tess.numVertexes = 0;
	tess.numIndexes = 0;

	if ( backEnd.currentEntity != &tr.worldEntity ) {
		GlobalVectorToLocal( &backEnd.viewParms.or.axis[1], &leftDir );
		GlobalVectorToLocal( &backEnd.viewParms.or.axis[2], &upDir );
	} else {
		VectorCopy( &backEnd.viewParms.or.axis[1], &leftDir );
		VectorCopy( &backEnd.viewParms.or.axis[2], &upDir );
	}

	for ( i = 0 ; i < oldVerts ; i+=4 ) {
		// find the midpoint
		xyz = &tess.xyz[i];

		mid.x = 0.25f * (xyz[0].x + xyz[1].x + xyz[2].x + xyz[3].x);
		mid.y = 0.25f * (xyz[0].y + xyz[1].y + xyz[2].y + xyz[3].y);
		mid.z = 0.25f * (xyz[0].z + xyz[1].z + xyz[2].z + xyz[3].z);

		VectorSubtract( xyz, &mid, &delta );
		radius = VectorLength( &delta ) * 0.707f;		// / sqrt(2)

		VectorScale( &leftDir, radius, &left );
		VectorScale( &upDir, radius, &up );

		if ( backEnd.viewParms.isMirror ) {
			VectorSubtract( &vec3_origin, &left, &left );
		}

		// compensate for scale in the axes if necessary
		if ( backEnd.currentEntity->e.nonNormalizedAxes ) {
			float axisLength;
			axisLength = VectorLength( &backEnd.currentEntity->e.axis[0] );
			if ( !axisLength ) {
				axisLength = 0;
			} else {
				axisLength = 1.0f / axisLength;
			}
			VectorScale(&left, axisLength, &left);
			VectorScale(&up, axisLength, &up);
		}

		RB_AddQuadStamp( &mid, &left, &up, tess.vertexColors[i] );
	}
}

// Autosprite2 will pivot a rectangular quad along the center of its long axis
int edgeVerts[6][2] = {
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 1, 2 },
	{ 1, 3 },
	{ 2, 3 }
};

static void Autosprite2Deform( void ) {
	int		i, j, k;
	int		indexes;
	vector3	*xyz;
	vector3	forward;

	if ( tess.numVertexes & 3 ) {
		ri->Printf( PRINT_WARNING, "Autosprite2 shader %s had odd vertex count", tess.shader->name );
	}
	if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 ) {
		ri->Printf( PRINT_WARNING, "Autosprite2 shader %s had odd index count", tess.shader->name );
	}

	if ( backEnd.currentEntity != &tr.worldEntity ) {
		GlobalVectorToLocal( &backEnd.viewParms.or.axis[0], &forward );
	} else {
		VectorCopy( &backEnd.viewParms.or.axis[0], &forward );
	}

	// this is a lot of work for two triangles...
	// we could precalculate a lot of it is an issue, but it would mess up
	// the shader abstraction
	for ( i = 0, indexes = 0 ; i < tess.numVertexes ; i+=4, indexes+=6 ) {
		float	lengths[2];
		int		nums[2];
		vector3	mid[2];
		vector3	major, minor;
		vector3	*v1, *v2;

		// find the midpoint
		xyz = &tess.xyz[i];

		// identify the two shortest edges
		nums[0] = nums[1] = 0;
		lengths[0] = lengths[1] = 999999;

		for ( j = 0 ; j < 6 ; j++ ) {
			float	l;
			vector3	temp;

			v1 = xyz + 4 * edgeVerts[j][0];
			v2 = xyz + 4 * edgeVerts[j][1];

			VectorSubtract( v1, v2, &temp );
			
			l = DotProduct( &temp, &temp );
			if ( l < lengths[0] ) {
				nums[1] = nums[0];
				lengths[1] = lengths[0];
				nums[0] = j;
				lengths[0] = l;
			} else if ( l < lengths[1] ) {
				nums[1] = j;
				lengths[1] = l;
			}
		}

		for ( j = 0 ; j < 2 ; j++ ) {
			v1 = xyz + 4 * edgeVerts[nums[j]][0];
			v2 = xyz + 4 * edgeVerts[nums[j]][1];

			mid[j].x = 0.5f * (v1->x + v2->x);
			mid[j].y = 0.5f * (v1->y + v2->y);
			mid[j].z = 0.5f * (v1->z + v2->z);
		}

		// find the vector of the major axis
		VectorSubtract( &mid[1], &mid[0], &major );

		// cross this with the view direction to get minor axis
		CrossProduct( &major, &forward, &minor );
		VectorNormalize( &minor );
		
		// re-project the points
		for ( j = 0 ; j < 2 ; j++ ) {
			float	l;

			v1 = xyz + 4 * edgeVerts[nums[j]][0];
			v2 = xyz + 4 * edgeVerts[nums[j]][1];

			l = 0.5f * sqrtf( lengths[j] );
			
			// we need to see which direction this edge
			// is used to determine direction of projection
			for ( k = 0 ; k < 5 ; k++ ) {
				if ( tess.indexes[ indexes + k ] == i + edgeVerts[nums[j]][0]
					&& tess.indexes[ indexes + k + 1 ] == i + edgeVerts[nums[j]][1] ) {
					break;
				}
			}

			if ( k == 5 ) {
				VectorMA( &mid[j],  l, &minor, v1 );
				VectorMA( &mid[j], -l, &minor, v2 );
			} else {
				VectorMA( &mid[j], -l, &minor, v1 );
				VectorMA( &mid[j],  l, &minor, v2 );
			}
		}
	}
}

void RB_DeformTessGeometry( void ) {
	int		i;
	deformStage_t	*ds;

	for ( i = 0 ; i < tess.shader->numDeforms ; i++ ) {
		ds = &tess.shader->deforms[ i ];

		switch ( ds->deformation ) {
        case DEFORM_NONE:
            break;
		case DEFORM_NORMALS:
			RB_CalcDeformNormals( ds );
			break;
		case DEFORM_WAVE:
			RB_CalcDeformVertexes( ds );
			break;
		case DEFORM_BULGE:
			RB_CalcBulgeVertexes( ds );
			break;
		case DEFORM_MOVE:
			RB_CalcMoveVertexes( ds );
			break;
		case DEFORM_PROJECTION_SHADOW:
			RB_ProjectionShadowDeform();
			break;
		case DEFORM_AUTOSPRITE:
			AutospriteDeform();
			break;
		case DEFORM_AUTOSPRITE2:
			Autosprite2Deform();
			break;
		case DEFORM_TEXT0:
		case DEFORM_TEXT1:
		case DEFORM_TEXT2:
		case DEFORM_TEXT3:
		case DEFORM_TEXT4:
		case DEFORM_TEXT5:
		case DEFORM_TEXT6:
		case DEFORM_TEXT7:
			DeformText( backEnd.refdef.text[ds->deformation - DEFORM_TEXT0] );
			break;
		}
	}
}

/*

	COLORS

*/

void RB_CalcColorFromEntity( unsigned char *dstColors )
{
	int	i;
	int *pColors = ( int * ) dstColors;
	int c;

	if ( !backEnd.currentEntity )
		return;

	c = * ( int * ) backEnd.currentEntity->e.shaderRGBA;

	for ( i = 0; i < tess.numVertexes; i++, pColors++ )
	{
		*pColors = c;
	}
}

void RB_CalcColorFromOneMinusEntity( unsigned char *dstColors )
{
	int	i;
	int *pColors = ( int * ) dstColors;
	unsigned char invModulate[4];
	int c;

	if ( !backEnd.currentEntity )
		return;

	invModulate[0] = 255 - backEnd.currentEntity->e.shaderRGBA[0];
	invModulate[1] = 255 - backEnd.currentEntity->e.shaderRGBA[1];
	invModulate[2] = 255 - backEnd.currentEntity->e.shaderRGBA[2];
	invModulate[3] = 255 - backEnd.currentEntity->e.shaderRGBA[3];	// this trashes alpha, but the AGEN block fixes it

	c = * ( int * ) invModulate;

	for ( i = 0; i < tess.numVertexes; i++, pColors++ )
	{
		*pColors = c;
	}
}

void RB_CalcAlphaFromEntity( unsigned char *dstColors )
{
	int	i;

	if ( !backEnd.currentEntity )
		return;

	dstColors += 3;

	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 )
	{
		*dstColors = backEnd.currentEntity->e.shaderRGBA[3];
	}
}

void RB_CalcAlphaFromOneMinusEntity( unsigned char *dstColors )
{
	int	i;

	if ( !backEnd.currentEntity )
		return;

	dstColors += 3;

	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 )
	{
		*dstColors = 0xff - backEnd.currentEntity->e.shaderRGBA[3];
	}
}

void RB_CalcWaveColor( const waveForm_t *wf, unsigned char *dstColors )
{
	int i;
	int v;
	float glow;
	int *colors = ( int * ) dstColors;
	byte	color[4];


  if ( wf->func == GF_NOISE ) {
		glow = wf->base + R_NoiseGet4f( 0, 0, 0, ( tess.shaderTime + wf->phase ) * wf->frequency ) * wf->amplitude;
	} else {
		glow = EvalWaveForm( wf ) * tr.identityLight;
	}
	
	if ( glow < 0 ) {
		glow = 0;
	}
	else if ( glow > 1 ) {
		glow = 1;
	}

	v = ri->ftol(255 * glow);
	color[0] = color[1] = color[2] = v;
	color[3] = 255;
	v = *(int *)color;
	
	for ( i = 0; i < tess.numVertexes; i++, colors++ ) {
		*colors = v;
	}
}

void RB_CalcWaveAlpha( const waveForm_t *wf, unsigned char *dstColors )
{
	int i;
	int v;
	float glow;

	glow = EvalWaveFormClamped( wf );

	v = (int)(255 * glow);

	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 )
	{
		dstColors[3] = v;
	}
}

void RB_CalcModulateColorsByFog( unsigned char *colors ) {
	int		i;
	float	texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords[0] );

	for ( i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0f - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[0] = (byte)(colors[0] * f);
		colors[1] = (byte)(colors[1] * f);
		colors[2] = (byte)(colors[2] * f);
	}
}

void RB_CalcModulateAlphasByFog( unsigned char *colors ) {
	int		i;
	float	texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords[0] );

	for ( i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0f - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[3] = (byte)(colors[3] * f);
	}
}

void RB_CalcModulateRGBAsByFog( unsigned char *colors ) {
	int		i;
	float	texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords[0] );

	for ( i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0f - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[0] = (byte)(colors[0] * f);
		colors[1] = (byte)(colors[1] * f);
		colors[2] = (byte)(colors[2] * f);
		colors[3] = (byte)(colors[3] * f);
	}
}


/*
====================================================================

TEX COORDS

====================================================================
*/

// To do the clipped fog plane really correctly, we should use projected textures, but I don't trust the
//	drivers and it doesn't fit our shader data.
void RB_CalcFogTexCoords( float *st ) {
	int			i;
	vector3		*v;
	float		s, t;
	float		eyeT;
	qboolean	eyeOutside;
	fog_t		*fog;
	vector3		local;
	vector4		fogDistanceVector, fogDepthVector = {0, 0, 0, 0};

	fog = tr.world->fogs + tess.fogNum;

	// all fogging distance is based on world Z units
	VectorSubtract( &backEnd.or.origin, &backEnd.viewParms.or.origin, &local );
	fogDistanceVector.x = -backEnd.or.modelMatrix[2];
	fogDistanceVector.y = -backEnd.or.modelMatrix[6];
	fogDistanceVector.z = -backEnd.or.modelMatrix[10];
	fogDistanceVector.w = DotProduct( &local, &backEnd.viewParms.or.axis[0] );

	// scale the fog vectors based on the fog's thickness
	fogDistanceVector.x *= fog->tcScale;
	fogDistanceVector.y *= fog->tcScale;
	fogDistanceVector.z *= fog->tcScale;
	fogDistanceVector.w *= fog->tcScale;

	// rotate the gradient vector for this orientation
	if ( fog->hasSurface ) {
		fogDepthVector.x =  fog->surface.x * backEnd.or.axis[0].x + fog->surface.y * backEnd.or.axis[0].y + fog->surface.z * backEnd.or.axis[0].z;
		fogDepthVector.y =  fog->surface.x * backEnd.or.axis[1].x + fog->surface.y * backEnd.or.axis[1].y + fog->surface.z * backEnd.or.axis[1].z;
		fogDepthVector.z =  fog->surface.x * backEnd.or.axis[2].x + fog->surface.y * backEnd.or.axis[2].y + fog->surface.z * backEnd.or.axis[2].z;
		fogDepthVector.w = -fog->surface.w + DotProduct( &backEnd.or.origin, (vector3 *)&fog->surface );

		eyeT = DotProduct( &backEnd.or.viewOrigin, (vector3 *)&fogDepthVector ) + fogDepthVector.w;
	} else {
		eyeT = 1;	// non-surface fog always has eye inside
	}

	// see if the viewpoint is outside
	// this is needed for clipping distance even for constant fog

	if ( eyeT < 0 ) {
		eyeOutside = qtrue;
	} else {
		eyeOutside = qfalse;
	}

	fogDistanceVector.w += 1.0/512;

	// calculate density for each point
	for (i = 0, v = &tess.xyz[0] ; i < tess.numVertexes ; i++, v++) {
		// calculate the length in fog
		s = DotProduct( v, (vector3 *)&fogDistanceVector ) + fogDistanceVector.w;
		t = DotProduct( v, (vector3 *)&fogDepthVector ) + fogDepthVector.w;

		// partially clipped fogs use the T axis		
		if ( eyeOutside ) {
			if ( t < 1.0f ) {
				t = 1.0f/32;	// point is outside, so no fogging
			} else {
				t = 1.0f/32 + 30.0f/32 * t / ( t - eyeT );	// cut the distance at the fog plane
			}
		} else {
			if ( t < 0 ) {
				t = 1.0f/32;	// point is outside, so no fogging
			} else {
				t = 31.0f/32;
			}
		}

		st[0] = s;
		st[1] = t;
		st += 2;
	}
}

void RB_CalcEnvironmentTexCoords( float *st ) 
{
	int			i;
	vector3		*v, *normal;
	vector3		viewer, reflected;
	float		d;

	v = &tess.xyz[0];
	normal = &tess.normal[0];

	for (i = 0 ; i < tess.numVertexes ; i++, v++, normal++, st += 2 ) 
	{
		VectorSubtract (&backEnd.or.viewOrigin, v, &viewer);
		VectorNormalizeFast (&viewer);

		d = DotProduct (normal, &viewer);

		reflected.x = normal->x*2*d - viewer.x;
		reflected.y = normal->y*2*d - viewer.y;
		reflected.z = normal->z*2*d - viewer.z;

		st[0] = 0.5f + reflected.x * 0.5f;
		st[1] = 0.5f - reflected.y * 0.5f;
	}
}

void RB_CalcTurbulentTexCoords( const waveForm_t *wf, float *st )
{
	int i;
	float now;

	now = ( wf->phase + tess.shaderTime * wf->frequency );

	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		float s = st[0];
		float t = st[1];

		st[0] = s + tr.sinTable[ ( ( int ) ( ( ( tess.xyz[i].x + tess.xyz[i].z )* 1.0/128 * 0.125 + now ) * FUNCTABLE_SIZE ) ) & ( FUNCTABLE_MASK ) ] * wf->amplitude;
		st[1] = t + tr.sinTable[ ( ( int ) ( ( tess.xyz[i].y * 1.0/128 * 0.125 + now ) * FUNCTABLE_SIZE ) ) & ( FUNCTABLE_MASK ) ] * wf->amplitude;
	}
}

void RB_CalcScaleTexCoords( const float scale[2], float *st )
{
	int i;

	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		st[0] *= scale[0];
		st[1] *= scale[1];
	}
}

void RB_CalcScrollTexCoords( const float scrollSpeed[2], float *st )
{
	int i;
	float timeScale = tess.shaderTime;
	float adjustedScrollS, adjustedScrollT;

	adjustedScrollS = scrollSpeed[0] * timeScale;
	adjustedScrollT = scrollSpeed[1] * timeScale;

	// clamp so coordinates don't continuously get larger, causing problems
	// with hardware limits
	adjustedScrollS = adjustedScrollS - floorf( adjustedScrollS );
	adjustedScrollT = adjustedScrollT - floorf( adjustedScrollT );

	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		st[0] += adjustedScrollS;
		st[1] += adjustedScrollT;
	}
}

void RB_CalcTransformTexCoords( const texModInfo_t *tmi, float *st  )
{
	int i;

	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		float s = st[0];
		float t = st[1];

		st[0] = s * tmi->matrix[0][0] + t * tmi->matrix[1][0] + tmi->translate[0];
		st[1] = s * tmi->matrix[0][1] + t * tmi->matrix[1][1] + tmi->translate[1];
	}
}

void RB_CalcRotateTexCoords( float degsPerSecond, float *st )
{
	float timeScale = tess.shaderTime;
	float degs;
	int index;
	float sinValue, cosValue;
	texModInfo_t tmi;

	degs = -degsPerSecond * timeScale;
	index = (int)(degs * ( FUNCTABLE_SIZE / 360.0f ));

	sinValue = tr.sinTable[ index & FUNCTABLE_MASK ];
	cosValue = tr.sinTable[ ( index + FUNCTABLE_SIZE / 4 ) & FUNCTABLE_MASK ];

	tmi.matrix[0][0] = cosValue;
	tmi.matrix[1][0] = -sinValue;
	tmi.translate[0] = 0.5f - 0.5f * cosValue + 0.5f * sinValue;

	tmi.matrix[0][1] = sinValue;
	tmi.matrix[1][1] = cosValue;
	tmi.translate[1] = 0.5f - 0.5f * sinValue - 0.5f * cosValue;

	RB_CalcTransformTexCoords( &tmi, st );
}


vector3 lightOrigin = { -960, 1980, 96 };		// FIXME: track dynamically

// Calculates specular coefficient and places it in the alpha channel
void RB_CalcSpecularAlpha( unsigned char *alphas ) {
	int			i;
	vector3		*v, *normal;
	vector3		viewer,  reflected;
	float		l, d;
	int			b;
	vector3		lightDir;
	int			numVertexes;

	v = &tess.xyz[0];
	normal = &tess.normal[0];

	alphas += 3;

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v++, normal++, alphas += 4) {
		float ilength;

		VectorSubtract( &lightOrigin, v, &lightDir );
//		ilength = Q_rsqrt( DotProduct( lightDir, lightDir ) );
		VectorNormalizeFast( &lightDir );

		// calculate the specular color
		d = DotProduct (normal, &lightDir);
//		d *= ilength;

		// we don't optimize for the d < 0 case since this tends to
		// cause visual artifacts such as faceted "snapping"
		reflected.x = normal->x*2*d - lightDir.x;
		reflected.y = normal->y*2*d - lightDir.y;
		reflected.z = normal->z*2*d - lightDir.z;

		VectorSubtract (&backEnd.or.viewOrigin, v, &viewer);
		ilength = Q_rsqrt( DotProduct( &viewer, &viewer ) );
		l = DotProduct (&reflected, &viewer);
		l *= ilength;

		if (l < 0) {
			b = 0;
		} else {
			l = l*l;
			l = l*l;
			b = (int)(l * 255);
			if (b > 255) {
				b = 255;
			}
		}

		*alphas = b;
	}
}

#if defined(idppc_altivec)
// The basic vertex lighting calc
static void RB_CalcDiffuseColor_altivec( unsigned char *colors )
{
	int				i;
	float			*v, *normal;
	trRefEntity_t	*ent;
	int				ambientLightInt;
	vector3			lightDir;
	int				numVertexes;
	vector unsigned char vSel = VECCONST_UINT8(0x00, 0x00, 0x00, 0xff,
                                               0x00, 0x00, 0x00, 0xff,
                                               0x00, 0x00, 0x00, 0xff,
                                               0x00, 0x00, 0x00, 0xff);
	vector float ambientLightVec;
	vector float directedLightVec;
	vector float lightDirVec;
	vector float normalVec0, normalVec1;
	vector float incomingVec0, incomingVec1, incomingVec2;
	vector float zero, jVec;
	vector signed int jVecInt;
	vector signed short jVecShort;
	vector unsigned char jVecChar, normalPerm;
	ent = backEnd.currentEntity;
	ambientLightInt = ent->ambientLightInt;
	// A lot of this could be simplified if we made sure
	// entities light info was 16-byte aligned.
	jVecChar = vec_lvsl(0, ent->ambientLight);
	ambientLightVec = vec_ld(0, (vector float *)ent->ambientLight);
	jVec = vec_ld(11, (vector float *)ent->ambientLight);
	ambientLightVec = vec_perm(ambientLightVec,jVec,jVecChar);

	jVecChar = vec_lvsl(0, ent->directedLight);
	directedLightVec = vec_ld(0,(vector float *)ent->directedLight);
	jVec = vec_ld(11,(vector float *)ent->directedLight);
	directedLightVec = vec_perm(directedLightVec,jVec,jVecChar);	 

	jVecChar = vec_lvsl(0, ent->lightDir);
	lightDirVec = vec_ld(0,(vector float *)ent->lightDir);
	jVec = vec_ld(11,(vector float *)ent->lightDir);
	lightDirVec = vec_perm(lightDirVec,jVec,jVecChar);	 

	zero = (vector float)vec_splat_s8(0);
	VectorCopy( ent->lightDir, lightDir );

	v = tess.xyz[0];
	normal = tess.normal[0];

	normalPerm = vec_lvsl(0,normal);
	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4) {
		normalVec0 = vec_ld(0,(vector float *)normal);
		normalVec1 = vec_ld(11,(vector float *)normal);
		normalVec0 = vec_perm(normalVec0,normalVec1,normalPerm);
		incomingVec0 = vec_madd(normalVec0, lightDirVec, zero);
		incomingVec1 = vec_sld(incomingVec0,incomingVec0,4);
		incomingVec2 = vec_add(incomingVec0,incomingVec1);
		incomingVec1 = vec_sld(incomingVec1,incomingVec1,4);
		incomingVec2 = vec_add(incomingVec2,incomingVec1);
		incomingVec0 = vec_splat(incomingVec2,0);
		incomingVec0 = vec_max(incomingVec0,zero);
		normalPerm = vec_lvsl(12,normal);
		jVec = vec_madd(incomingVec0, directedLightVec, ambientLightVec);
		jVecInt = vec_cts(jVec,0);	// RGBx
		jVecShort = vec_pack(jVecInt,jVecInt);		// RGBxRGBx
		jVecChar = vec_packsu(jVecShort,jVecShort);	// RGBxRGBxRGBxRGBx
		jVecChar = vec_sel(jVecChar,vSel,vSel);		// RGBARGBARGBARGBA replace alpha with 255
		vec_ste((vector unsigned int)jVecChar,0,(unsigned int *)&colors[i*4]);	// store color
	}
}
#endif

static void RB_CalcDiffuseColor_scalar( unsigned char *colors )
{
	int				i, j;
	vector3			*v, *normal;
	float			incoming;
	trRefEntity_t	*ent;
	int				ambientLightInt;
	vector3			ambientLight, lightDir, directedLight;
	int				numVertexes;

	ent = backEnd.currentEntity;
	ambientLightInt = ent->ambientLightInt;
	VectorCopy( &ent->ambientLight, &ambientLight );
	VectorCopy( &ent->directedLight, &directedLight );
	VectorCopy( &ent->lightDir, &lightDir );

	v = &tess.xyz[0];
	normal = &tess.normal[0];

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v++, normal++) {
		incoming = DotProduct (normal, &lightDir);
		if ( incoming <= 0 ) {
			*(int *)&colors[i*4] = ambientLightInt;
			continue;
		} 

		j = ri->ftol(ambientLight.r + incoming * directedLight.r);
		if ( j > 255 ) j = 255;
		colors[i*4+0] = j;

		j = ri->ftol(ambientLight.g + incoming * directedLight.g);
		if ( j > 255 ) j = 255;
		colors[i*4+1] = j;

		j = ri->ftol(ambientLight.b + incoming * directedLight.b);
		if ( j > 255 ) j = 255;
		colors[i*4+2] = j;

		colors[i*4+3] = 255;
	}
}

void RB_CalcDiffuseColor( unsigned char *colors )
{
#if defined(idppc_altivec)
	if (com_altivec->integer) {
		// must be in a seperate function or G3 systems will crash.
		RB_CalcDiffuseColor_altivec( colors );
		return;
	}
#endif
	RB_CalcDiffuseColor_scalar( colors );
}

