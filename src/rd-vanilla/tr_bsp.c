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
// tr_map.c

#include "tr_local.h"

/*

Loads and prepares a map file for scene rendering.

A single entry point:

void RE_LoadWorldMap( const char *name );

*/

static	world_t		s_worldData;
static	byte		*fileBase;

int			c_subdivisions;
int			c_gridVerts;

static void HSVtoRGB( float h, float s, float v, float rgb[3] ) {
	int i;
	float f;
	float p, q, t;

	h *= 5;

	i = (int)floorf( h );
	f = h - i;

	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch ( i )
	{
	case 0:
		rgb[0] = v;
		rgb[1] = t;
		rgb[2] = p;
		break;
	case 1:
		rgb[0] = q;
		rgb[1] = v;
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = v;
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = v;
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = v;
		break;
	case 5:
		rgb[0] = v;
		rgb[1] = p;
		rgb[2] = q;
		break;
	}
}

static	void R_ColorShiftLightingBytes( byte in[4], byte out[4] ) {
	int		shift, r, g, b;

	// shift the color data based on overbright range
	shift = r_mapOverBrightBits->integer - tr.overbrightBits;

	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;
	
	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 ) {
		int		max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[0] = (byte)r;
	out[1] = (byte)g;
	out[2] = (byte)b;
	out[3] = in[3];
}

#define	LIGHTMAP_SIZE	128
static	void R_LoadLightmaps( lump_t *l ) {
	byte		*buf, *buf_p;
	int			len;
	byte		image[LIGHTMAP_SIZE*LIGHTMAP_SIZE*4];
	int			i, j;
	float maxIntensity = 0;
	double sumIntensity = 0;

	len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	// we are about to upload textures
	R_SyncRenderThread();

	// create all the lightmaps
	tr.numLightmaps = len / (LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3);
	if ( tr.numLightmaps == 1 ) {
		//FIXME: HACK: maps with only one lightmap turn up fullbright for some reason.
		//this avoids this, but isn't the correct solution.
		tr.numLightmaps++;
	}

	// if we are in r_vertexLight mode, we don't need the lightmaps at all
	if ( r_vertexLight->integer ) {
		return;
	}

	tr.lightmaps = ri->Hunk_Alloc( tr.numLightmaps * sizeof(image_t *), PREF_LOW );
	for ( i = 0 ; i < tr.numLightmaps ; i++ ) {
		// expand the 24 bit on-disk to 32 bit
		buf_p = buf + i * LIGHTMAP_SIZE*LIGHTMAP_SIZE * 3;

		if ( r_lightmap->integer == 2 )
		{	// color code by intensity as development tool	(FIXME: check range)
			for ( j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ )
			{
				float r = buf_p[j*3+0];
				float g = buf_p[j*3+1];
				float b = buf_p[j*3+2];
				float intensity;
				float out[3] = {0.0f, 0.0f, 0.0f};

				intensity = 0.33f * r + 0.685f * g + 0.063f * b;

				if ( intensity > 255 )
					intensity = 1.0f;
				else
					intensity /= 255.0f;

				if ( intensity > maxIntensity )
					maxIntensity = intensity;

				HSVtoRGB( intensity, 1.00f, 0.50f, out );

				image[j*4+0] = (byte)(out[0] * 255);
				image[j*4+1] = (byte)(out[1] * 255);
				image[j*4+2] = (byte)(out[2] * 255);
				image[j*4+3] = 255;

				sumIntensity += intensity;
			}
		} else {
			for ( j = 0 ; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ ) {
				R_ColorShiftLightingBytes( &buf_p[j*3], &image[j*4] );
				image[j*4+3] = 255;
			}
		}
		tr.lightmaps[i] = R_CreateImage( va("*lightmap%d",i), image, 
			LIGHTMAP_SIZE, LIGHTMAP_SIZE, qfalse, qfalse, GL_CLAMP_TO_EDGE );
	}

	if ( r_lightmap->integer == 2 )	{
		ri->Printf( PRINT_ALL, "Brightest lightmap value: %d\n", ( int ) ( maxIntensity * 255 ) );
	}
}


// This is called by the clipmodel subsystem so we can share the 1.8 megs of space in big maps...
void		RE_SetWorldVisData( const byte *vis ) {
	tr.externalVisData = vis;
}

static	void R_LoadVisibility( lump_t *l ) {
	int		len;
	byte	*buf;

	len = ( s_worldData.numClusters + 63 ) & ~63;
	s_worldData.novis = ri->Hunk_Alloc( len, PREF_LOW );
	memset( s_worldData.novis, 0xff, len );

    len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	s_worldData.numClusters = LittleLong( ((int *)buf)[0] );
	s_worldData.clusterBytes = LittleLong( ((int *)buf)[1] );

	// CM_Load should have given us the vis data to share, so
	// we don't need to allocate another copy
	if ( tr.externalVisData ) {
		s_worldData.vis = tr.externalVisData;
	} else {
		byte	*dest;

		dest = ri->Hunk_Alloc( len - 8, PREF_LOW );
		memcpy( dest, buf + 8, len - 8 );
		s_worldData.vis = dest;
	}
}

static shader_t *ShaderForShaderNum( int shaderNum, int lightmapNum ) {
	shader_t	*shader;
	dshader_t	*dsh;

	int _shaderNum = LittleLong( shaderNum );
	if ( _shaderNum < 0 || _shaderNum >= s_worldData.numShaders ) {
		ri->Error( ERR_DROP, "ShaderForShaderNum: bad num %i", _shaderNum );
	}
	dsh = &s_worldData.shaders[ _shaderNum ];

	if ( r_vertexLight->integer ) {
		lightmapNum = LIGHTMAP_BY_VERTEX;
	}

	if ( r_fullbright->integer ) {
		lightmapNum = LIGHTMAP_WHITEIMAGE;
	}

	shader = R_FindShader( dsh->shader, lightmapNum, qtrue );

	// if the shader had errors, just use default shader
	if ( shader->defaultShader ) {
		return tr.defaultShader;
	}

	return shader;
}

static void ParseFace( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes  ) {
	int			i, j;
	srfSurfaceFace_t	*cv;
	int			numPoints, numIndexes;
	int			lightmapNum;
	size_t		sfaceSize, ofsIndexes;

	lightmapNum = LittleLong( ds->lightmapNum );

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numPoints = LittleLong( ds->numVerts );
	if (numPoints > MAX_FACE_POINTS) {
		ri->Printf( PRINT_WARNING, "WARNING: MAX_FACE_POINTS exceeded: %i\n", numPoints);
    numPoints = MAX_FACE_POINTS;
    surf->shader = tr.defaultShader;
	}

	numIndexes = LittleLong( ds->numIndexes );

	// create the srfSurfaceFace_t
	sfaceSize = offsetof( srfSurfaceFace_t, points[numPoints] );
	ofsIndexes = sfaceSize;
	sfaceSize += sizeof( int ) * numIndexes;

	cv = ri->Hunk_Alloc( sfaceSize, PREF_LOW );
	cv->surfaceType = SF_FACE;
	cv->numPoints = numPoints;
	cv->numIndices = numIndexes;
	cv->ofsIndices = ofsIndexes;

	verts += LittleLong( ds->firstVert );
	for ( i = 0 ; i < numPoints ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			cv->points[i][j] = LittleFloat( verts[i].xyz.data[j] );
		}
		for ( j = 0 ; j < 2 ; j++ ) {
			cv->points[i][3+j] = LittleFloat( verts[i].st[j] );
			cv->points[i][5+j] = LittleFloat( verts[i].lightmap[j] );
		}
		R_ColorShiftLightingBytes( verts[i].color, (byte *)&cv->points[i][7] );
	}

	indexes += LittleLong( ds->firstIndex );
	for ( i = 0 ; i < numIndexes ; i++ ) {
		((int *)((byte *)cv + cv->ofsIndices ))[i] = LittleLong( indexes[ i ] );
	}

	// take the plane information from the lightmap vector
	for ( i = 0 ; i < 3 ; i++ ) {
		cv->plane.normal.data[i] = LittleFloat( ds->lightmapVecs[2].data[i] );
	}
	cv->plane.dist = DotProduct( (vector3 *)&cv->points[0], &cv->plane.normal );
	SetPlaneSignbits( &cv->plane );
	cv->plane.type = (byte)PlaneTypeForNormal( &cv->plane.normal );

	surf->data = (surfaceType_t *)cv;
}

static void ParseMesh ( dsurface_t *ds, drawVert_t *verts, msurface_t *surf ) {
	srfGridMesh_t	*grid;
	int				i, j;
	int				width, height, numPoints;
	drawVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE];
	int				lightmapNum;
	vector3			bounds[2];
	vector3			tmpVec;
	static surfaceType_t	skipData = SF_SKIP;

	lightmapNum = LittleLong( ds->lightmapNum );

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW ) {
		surf->data = &skipData;
		return;
	}

	width = LittleLong( ds->patchWidth );
	height = LittleLong( ds->patchHeight );

	verts += LittleLong( ds->firstVert );
	numPoints = width * height;
	for ( i = 0 ; i < numPoints ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			points[i].xyz.data[j] = LittleFloat( verts[i].xyz.data[j] );
			points[i].normal.data[j] = LittleFloat( verts[i].normal.data[j] );
		}
		for ( j = 0 ; j < 2 ; j++ ) {
			points[i].st[j] = LittleFloat( verts[i].st[j] );
			points[i].lightmap[j] = LittleFloat( verts[i].lightmap[j] );
		}
		R_ColorShiftLightingBytes( verts[i].color, points[i].color );
	}

	// pre-tesseleate
	grid = R_SubdividePatchToGrid( width, height, points );
	surf->data = (surfaceType_t *)grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0].data[i] = LittleFloat( ds->lightmapVecs[0].data[i] );
		bounds[1].data[i] = LittleFloat( ds->lightmapVecs[1].data[i] );
	}
	VectorAdd( &bounds[0], &bounds[1], &bounds[1] );
	VectorScale( &bounds[1], 0.5f, &grid->lodOrigin );
	VectorSubtract( &bounds[0], &grid->lodOrigin, &tmpVec );
	grid->lodRadius = VectorLength( &tmpVec );
}

static void ParseTriSurf( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes ) {
	srfTriangles_t	*tri;
	int				i, j;
	int				numVerts, numIndexes;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong( ds->numVerts );
	numIndexes = LittleLong( ds->numIndexes );

	tri = ri->Hunk_Alloc( sizeof( *tri ) + numVerts * sizeof( tri->verts[0] ) 
		+ numIndexes * sizeof( tri->indexes[0] ), PREF_LOW );
	tri->surfaceType = SF_TRIANGLES;
	tri->numVerts = numVerts;
	tri->numIndexes = numIndexes;
	tri->verts = (drawVert_t *)(tri + 1);
	tri->indexes = (int *)(tri->verts + tri->numVerts );

	surf->data = (surfaceType_t *)tri;

	// copy vertexes
	ClearBounds( &tri->bounds[0], &tri->bounds[1] );
	verts += LittleLong( ds->firstVert );
	for ( i = 0 ; i < numVerts ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			tri->verts[i].xyz.data[j] = LittleFloat( verts[i].xyz.data[j] );
			tri->verts[i].normal.data[j] = LittleFloat( verts[i].normal.data[j] );
		}
		AddPointToBounds( &tri->verts[i].xyz, &tri->bounds[0], &tri->bounds[1] );
		for ( j = 0 ; j < 2 ; j++ ) {
			tri->verts[i].st[j] = LittleFloat( verts[i].st[j] );
			tri->verts[i].lightmap[j] = LittleFloat( verts[i].lightmap[j] );
		}

		R_ColorShiftLightingBytes( verts[i].color, tri->verts[i].color );
	}

	// copy indexes
	indexes += LittleLong( ds->firstIndex );
	for ( i = 0 ; i < numIndexes ; i++ ) {
		tri->indexes[i] = LittleLong( indexes[i] );
		if ( tri->indexes[i] < 0 || tri->indexes[i] >= numVerts ) {
			ri->Error( ERR_DROP, "Bad index in triangle surface" );
		}
	}
}

static void ParseFlare( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes ) {
	srfFlare_t		*flare;
	int				i;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	flare = ri->Hunk_Alloc( sizeof( *flare ), PREF_LOW );
	flare->surfaceType = SF_FLARE;

	surf->data = (surfaceType_t *)flare;

	for ( i = 0 ; i < 3 ; i++ ) {
		flare->origin.data[i] = LittleFloat( ds->lightmapOrigin.data[i] );
		flare->color.data[i] = LittleFloat( ds->lightmapVecs[0].data[i] );
		flare->normal.data[i] = LittleFloat( ds->lightmapVecs[2].data[i] );
	}
}


// returns true if there are grid points merged on a width edge
int R_MergedWidthPoints(srfGridMesh_t *grid, int offset) {
	int i, j;

	for (i = 1; i < grid->width-1; i++) {
		for (j = i + 1; j < grid->width-1; j++) {
			if ( fabsf(grid->verts[i + offset].xyz.x - grid->verts[j + offset].xyz.x) > .1f) continue;
			if ( fabsf(grid->verts[i + offset].xyz.y - grid->verts[j + offset].xyz.y) > .1f) continue;
			if ( fabsf(grid->verts[i + offset].xyz.z - grid->verts[j + offset].xyz.z) > .1f) continue;
			return qtrue;
		}
	}
	return qfalse;
}

// returns true if there are grid points merged on a height edge
int R_MergedHeightPoints(srfGridMesh_t *grid, int offset) {
	int i, j;

	for (i = 1; i < grid->height-1; i++) {
		for (j = i + 1; j < grid->height-1; j++) {
			if ( fabsf(grid->verts[grid->width * i + offset].xyz.x - grid->verts[grid->width * j + offset].xyz.x) > .1f) continue;
			if ( fabsf(grid->verts[grid->width * i + offset].xyz.y - grid->verts[grid->width * j + offset].xyz.y) > .1f) continue;
			if ( fabsf(grid->verts[grid->width * i + offset].xyz.z - grid->verts[grid->width * j + offset].xyz.z) > .1f) continue;
			return qtrue;
		}
	}
	return qfalse;
}

// NOTE: never sync LoD through grid edges with merged points!
//	FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
void R_FixSharedVertexLodError_r( int start, srfGridMesh_t *grid1 ) {
	int j, k, l, m, n, offset1, offset2, touch;
	srfGridMesh_t *grid2;

	for ( j = start; j < s_worldData.numsurfaces; j++ ) {
		//
		grid2 = (srfGridMesh_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) continue;
		// if the LOD errors are already fixed for this patch
		if ( grid2->lodFixed == 2 ) continue;
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) continue;
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin.x != grid2->lodOrigin.x ) continue;
		if ( grid1->lodOrigin.y != grid2->lodOrigin.y ) continue;
		if ( grid1->lodOrigin.z != grid2->lodOrigin.z ) continue;
		//
		touch = qfalse;
		for (n = 0; n < 2; n++) {
			//
			if (n) offset1 = (grid1->height-1) * grid1->width;
			else offset1 = 0;
			if (R_MergedWidthPoints(grid1, offset1)) continue;
			for (k = 1; k < grid1->width-1; k++) {
				for (m = 0; m < 2; m++) {

					if (m) offset2 = (grid2->height-1) * grid2->width;
					else offset2 = 0;
					if (R_MergedWidthPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->width-1; l++) {
					//
						if ( fabsf(grid1->verts[k + offset1].xyz.x - grid2->verts[l + offset2].xyz.x) > .1f) continue;
						if ( fabsf(grid1->verts[k + offset1].xyz.y - grid2->verts[l + offset2].xyz.y) > .1f) continue;
						if ( fabsf(grid1->verts[k + offset1].xyz.z - grid2->verts[l + offset2].xyz.z) > .1f) continue;
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->widthLodError[k];
						touch = qtrue;
					}
				}
				for (m = 0; m < 2; m++) {

					if (m) offset2 = grid2->width-1;
					else offset2 = 0;
					if (R_MergedHeightPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->height-1; l++) {
					//
						if ( fabsf(grid1->verts[k + offset1].xyz.x - grid2->verts[grid2->width * l + offset2].xyz.x) > .1f) continue;
						if ( fabsf(grid1->verts[k + offset1].xyz.y - grid2->verts[grid2->width * l + offset2].xyz.y) > .1f) continue;
						if ( fabsf(grid1->verts[k + offset1].xyz.z - grid2->verts[grid2->width * l + offset2].xyz.z) > .1f) continue;
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->widthLodError[k];
						touch = qtrue;
					}
				}
			}
		}
		for (n = 0; n < 2; n++) {
			//
			if (n) offset1 = grid1->width-1;
			else offset1 = 0;
			if (R_MergedHeightPoints(grid1, offset1)) continue;
			for (k = 1; k < grid1->height-1; k++) {
				for (m = 0; m < 2; m++) {

					if (m) offset2 = (grid2->height-1) * grid2->width;
					else offset2 = 0;
					if (R_MergedWidthPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->width-1; l++) {
					//
						if ( fabsf(grid1->verts[grid1->width * k + offset1].xyz.x - grid2->verts[l + offset2].xyz.x) > .1f) continue;
						if ( fabsf(grid1->verts[grid1->width * k + offset1].xyz.y - grid2->verts[l + offset2].xyz.y) > .1f) continue;
						if ( fabsf(grid1->verts[grid1->width * k + offset1].xyz.z - grid2->verts[l + offset2].xyz.z) > .1f) continue;
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->heightLodError[k];
						touch = qtrue;
					}
				}
				for (m = 0; m < 2; m++) {

					if (m) offset2 = grid2->width-1;
					else offset2 = 0;
					if (R_MergedHeightPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->height-1; l++) {
					//
						if ( fabsf(grid1->verts[grid1->width * k + offset1].xyz.x - grid2->verts[grid2->width * l + offset2].xyz.x) > .1f) continue;
						if ( fabsf(grid1->verts[grid1->width * k + offset1].xyz.y - grid2->verts[grid2->width * l + offset2].xyz.y) > .1f) continue;
						if ( fabsf(grid1->verts[grid1->width * k + offset1].xyz.z - grid2->verts[grid2->width * l + offset2].xyz.z) > .1f) continue;
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->heightLodError[k];
						touch = qtrue;
					}
				}
			}
		}
		if (touch) {
			grid2->lodFixed = 2;
			R_FixSharedVertexLodError_r ( start, grid2 );
			//NOTE: this would be correct but makes things really slow
			//grid2->lodFixed = 1;
		}
	}
}

// This function assumes that all patches in one group are nicely stitched together for the highest LoD.
//	If this is not the case this function will still do its job but won't fix the highest LoD cracks.
void R_FixSharedVertexLodError( void ) {
	int i;
	srfGridMesh_t *grid1;

	for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
		//
		grid1 = (srfGridMesh_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if ( grid1->surfaceType != SF_GRID )
			continue;
		//
		if ( grid1->lodFixed )
			continue;
		//
		grid1->lodFixed = 2;
		// recursively fix other patches in the same LOD group
		R_FixSharedVertexLodError_r( i + 1, grid1);
	}
}

int R_StitchPatches( int grid1num, int grid2num ) {
	vector3 *v1, *v2;
	srfGridMesh_t *grid1, *grid2;
	int k, l, m, n, offset1, offset2, row, column;

	grid1 = (srfGridMesh_t *) s_worldData.surfaces[grid1num].data;
	grid2 = (srfGridMesh_t *) s_worldData.surfaces[grid2num].data;
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = (grid1->height-1) * grid1->width;
		else offset1 = 0;
		if (R_MergedWidthPoints(grid1, offset1))
			continue;
		for (k = 0; k < grid1->width-2; k += 2) {

			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = &grid1->verts[k + offset1].xyz;
					v2 = &grid2->verts[l + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;

					v1 = &grid1->verts[k + 2 + offset1].xyz;
					v2 = &grid2->verts[l + 1 + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;
					//
					v1 = &grid2->verts[l + offset2].xyz;
					v2 = &grid2->verts[l + 1 + offset2].xyz;
					if ( fabsf(v1->x - v2->x) < .01f &&
						 fabsf(v1->y - v2->y) < .01f &&
						 fabsf(v1->z - v2->z) < .01f)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row, &grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
					//
					v1 = &grid1->verts[k + offset1].xyz;
					v2 = &grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;

					v1 = &grid1->verts[k + 2 + offset1].xyz;
					v2 = &grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;
					//
					v1 = &grid2->verts[grid2->width * l + offset2].xyz;
					v2 = &grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabsf(v1->x - v2->x) < .01f &&
						 fabsf(v1->y - v2->y) < .01f &&
						 fabsf(v1->z - v2->z) < .01f)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column, &grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = grid1->width-1;
		else offset1 = 0;
		if (R_MergedHeightPoints(grid1, offset1))
			continue;
		for (k = 0; k < grid1->height-2; k += 2) {
			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = &grid1->verts[grid1->width * k + offset1].xyz;
					v2 = &grid2->verts[l + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;

					v1 = &grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = &grid2->verts[l + 1 + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;
					//
					v1 = &grid2->verts[l + offset2].xyz;
					v2 = &grid2->verts[(l + 1) + offset2].xyz;
					if ( fabsf(v1->x - v2->x) < .01f &&
						 fabsf(v1->y - v2->y) < .01f &&
						 fabsf(v1->z - v2->z) < .01f)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row, &grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = &grid1->verts[grid1->width * k + offset1].xyz;
					v2 = &grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;

					v1 = &grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = &grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;
					//
					v1 = &grid2->verts[grid2->width * l + offset2].xyz;
					v2 = &grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabsf(v1->x - v2->x) < .01f &&
						 fabsf(v1->y - v2->y) < .01f &&
						 fabsf(v1->z - v2->z) < .01f)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column, &grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = (grid1->height-1) * grid1->width;
		else offset1 = 0;
		if (R_MergedWidthPoints(grid1, offset1))
			continue;
		for (k = grid1->width-1; k > 1; k -= 2) {

			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = &grid1->verts[k + offset1].xyz;
					v2 = &grid2->verts[l + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;

					v1 = &grid1->verts[k - 2 + offset1].xyz;
					v2 = &grid2->verts[l + 1 + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;
					//
					v1 = &grid2->verts[l + offset2].xyz;
					v2 = &grid2->verts[(l + 1) + offset2].xyz;
					if ( fabsf(v1->x - v2->x) < .01f &&
						 fabsf(v1->y - v2->y) < .01f &&
						 fabsf(v1->z - v2->z) < .01f)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row, &grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = &grid1->verts[k + offset1].xyz;
					v2 = &grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;

					v1 = &grid1->verts[k - 2 + offset1].xyz;
					v2 = &grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;
					//
					v1 = &grid2->verts[grid2->width * l + offset2].xyz;
					v2 = &grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1->x - v2->x) < .01f &&
						 fabs(v1->y - v2->y) < .01f &&
						 fabs(v1->z - v2->z) < .01f)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column, &grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k+1]);
					if (!grid2)
						break;
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = grid1->width-1;
		else offset1 = 0;
		if (R_MergedHeightPoints(grid1, offset1))
			continue;
		for (k = grid1->height-1; k > 1; k -= 2) {
			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = &grid1->verts[grid1->width * k + offset1].xyz;
					v2 = &grid2->verts[l + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;

					v1 = &grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = &grid2->verts[l + 1 + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;
					//
					v1 = &grid2->verts[l + offset2].xyz;
					v2 = &grid2->verts[(l + 1) + offset2].xyz;
					if ( fabsf(v1->x - v2->x) < .01f &&
						 fabsf(v1->y - v2->y) < .01f &&
						 fabsf(v1->z - v2->z) < .01f)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row, &grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = &grid1->verts[grid1->width * k + offset1].xyz;
					v2 = &grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;

					v1 = &grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = &grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabsf(v1->x - v2->x) > .1f)	continue;
					if ( fabsf(v1->y - v2->y) > .1f)	continue;
					if ( fabsf(v1->z - v2->z) > .1f)	continue;
					//
					v1 = &grid2->verts[grid2->width * l + offset2].xyz;
					v2 = &grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabsf(v1->x - v2->x) < .01f &&
						 fabsf(v1->y - v2->y) < .01f &&
						 fabsf(v1->z - v2->z) < .01f)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column, &grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

// This function will try to stitch patches in the same LoD group together for the highest LoD.
//	Only single missing vertice cracks will be fixed.
//	Vertices will be joined at the patch side a crack is first found, at the other side of the patch (on the
//	same row or column) the vertices will not be joined and cracks might still appear at that side.
int R_TryStitchingPatch( int grid1num ) {
	int j, numstitches;
	srfGridMesh_t *grid1, *grid2;

	numstitches = 0;
	grid1 = (srfGridMesh_t *) s_worldData.surfaces[grid1num].data;
	for ( j = 0; j < s_worldData.numsurfaces; j++ ) {
		//
		grid2 = (srfGridMesh_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) continue;
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) continue;
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin.x != grid2->lodOrigin.x ) continue;
		if ( grid1->lodOrigin.y != grid2->lodOrigin.y ) continue;
		if ( grid1->lodOrigin.z != grid2->lodOrigin.z ) continue;
		//
		while (R_StitchPatches(grid1num, j))
		{
			numstitches++;
		}
	}
	return numstitches;
}

void R_StitchAllPatches( void ) {
	int i, stitched, numstitches;
	srfGridMesh_t *grid1;

	numstitches = 0;
	do
	{
		stitched = qfalse;
		for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
			//
			grid1 = (srfGridMesh_t *) s_worldData.surfaces[i].data;
			// if this surface is not a grid
			if ( grid1->surfaceType != SF_GRID )
				continue;
			//
			if ( grid1->lodStitched )
				continue;
			//
			grid1->lodStitched = qtrue;
			stitched = qtrue;
			//
			numstitches += R_TryStitchingPatch( i );
		}
	}
	while (stitched);
	ri->Printf( PRINT_DEVELOPER, "stitched %d LoD cracks\n", numstitches );
}

void R_MovePatchSurfacesToHunk( void ) {
	int i, size;
	srfGridMesh_t *grid, *hunkgrid;

	for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
		//
		grid = (srfGridMesh_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if ( grid->surfaceType != SF_GRID )
			continue;
		//
		size = (grid->width * grid->height - 1) * sizeof( drawVert_t ) + sizeof( *grid );
		hunkgrid = ri->Hunk_Alloc( size, PREF_LOW );
		memcpy(hunkgrid, grid, size);

		hunkgrid->widthLodError = ri->Hunk_Alloc( grid->width * 4, PREF_LOW );
		memcpy( hunkgrid->widthLodError, grid->widthLodError, grid->width * 4 );

		hunkgrid->heightLodError = ri->Hunk_Alloc( grid->height * 4, PREF_LOW );
		memcpy( hunkgrid->heightLodError, grid->heightLodError, grid->height * 4 );

		R_FreeSurfaceGridMesh( grid );

		s_worldData.surfaces[i].data = (void *) hunkgrid;
	}
}

static	void R_LoadSurfaces( lump_t *surfs, lump_t *verts, lump_t *indexLump ) {
	dsurface_t	*in;
	msurface_t	*out;
	drawVert_t	*dv;
	int			*indexes;
	int			count;
	int			numFaces, numMeshes, numTriSurfs, numFlares;
	int			i;

	numFaces = 0;
	numMeshes = 0;
	numTriSurfs = 0;
	numFlares = 0;

	in = (void *)(fileBase + surfs->fileofs);
	if (surfs->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = surfs->filelen / sizeof(*in);

	dv = (void *)(fileBase + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	indexes = (void *)(fileBase + indexLump->fileofs);
	if ( indexLump->filelen % sizeof(*indexes))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	out = ri->Hunk_Alloc ( count * sizeof(*out), PREF_LOW );	

	s_worldData.surfaces = out;
	s_worldData.numsurfaces = count;

	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		switch ( LittleLong( in->surfaceType ) ) {
		case MST_PATCH:
			ParseMesh ( in, dv, out );
			numMeshes++;
			break;
		case MST_TRIANGLE_SOUP:
			ParseTriSurf( in, dv, out, indexes );
			numTriSurfs++;
			break;
		case MST_PLANAR:
			ParseFace( in, dv, out, indexes );
			numFaces++;
			break;
		case MST_FLARE:
			ParseFlare( in, dv, out, indexes );
			numFlares++;
			break;
		default:
			ri->Error( ERR_DROP, "Bad surfaceType" );
		}
	}

#ifdef PATCH_STITCHING
	R_StitchAllPatches();
#endif

	R_FixSharedVertexLodError();

#ifdef PATCH_STITCHING
	R_MovePatchSurfacesToHunk();
#endif

	ri->Printf( PRINT_ALL, "...loaded %d faces, %i meshes, %i trisurfs, %i flares\n", 
		numFaces, numMeshes, numTriSurfs, numFlares );
}


static	void R_LoadSubmodels( lump_t *l ) {
	dmodel_t	*in;
	bmodel_t	*out;
	int			i, j, count;

	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);

	s_worldData.bmodels = out = ri->Hunk_Alloc( count * sizeof(*out), PREF_LOW );

	for ( i=0 ; i<count ; i++, in++, out++ ) {
		model_t *model;

		model = R_AllocModel();

		assert( model != NULL );			// this should never happen
		if ( model == NULL ) {
			ri->Error(ERR_DROP, "R_LoadSubmodels: R_AllocModel() failed");
		}

		model->type = MOD_BRUSH;
		model->bmodel = out;
		Com_sprintf( model->name, sizeof( model->name ), "*%d", i );

		for (j=0 ; j<3 ; j++) {
			out->bounds[0].data[j] = LittleFloat (in->mins.data[j]);
			out->bounds[1].data[j] = LittleFloat (in->maxs.data[j]);
		}

		out->firstSurface = s_worldData.surfaces + LittleLong( in->firstSurface );
		out->numSurfaces = LittleLong( in->numSurfaces );
	}
}

static	void R_SetParent (mnode_t *node, mnode_t *parent) {
	node->parent = parent;
	if (node->contents != -1)
		return;
	R_SetParent (node->children[0], node);
	R_SetParent (node->children[1], node);
}

static	void R_LoadNodesAndLeafs (lump_t *nodeLump, lump_t *leafLump) {
	int			i, j, p;
	dnode_t		*in;
	dleaf_t		*inLeaf;
	mnode_t 	*out;
	int			numNodes, numLeafs;

	in = (void *)(fileBase + nodeLump->fileofs);
	if (nodeLump->filelen % sizeof(dnode_t) ||
		leafLump->filelen % sizeof(dleaf_t) ) {
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	numNodes = nodeLump->filelen / sizeof(dnode_t);
	numLeafs = leafLump->filelen / sizeof(dleaf_t);

	out = ri->Hunk_Alloc ( (numNodes + numLeafs) * sizeof(*out), PREF_LOW);	

	s_worldData.nodes = out;
	s_worldData.numnodes = numNodes + numLeafs;
	s_worldData.numDecisionNodes = numNodes;

	// load nodes
	for ( i=0 ; i<numNodes; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins.data[j] = (float)LittleLong (in->mins.data[j]);
			out->maxs.data[j] = (float)LittleLong (in->maxs.data[j]);
		}
	
		p = LittleLong(in->planeNum);
		out->plane = s_worldData.planes + p;

		out->contents = CONTENTS_NODE;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = s_worldData.nodes + p;
			else
				out->children[j] = s_worldData.nodes + numNodes + (-1 - p);
		}
	}
	
	// load leafs
	inLeaf = (void *)(fileBase + leafLump->fileofs);
	for ( i=0 ; i<numLeafs ; i++, inLeaf++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins.data[j] = (float)LittleLong (inLeaf->mins.data[j]);
			out->maxs.data[j] = (float)LittleLong (inLeaf->maxs.data[j]);
		}

		out->cluster = LittleLong(inLeaf->cluster);
		out->area = LittleLong(inLeaf->area);

		if ( out->cluster >= s_worldData.numClusters ) {
			s_worldData.numClusters = out->cluster + 1;
		}

		out->firstmarksurface = s_worldData.marksurfaces +
			LittleLong(inLeaf->firstLeafSurface);
		out->nummarksurfaces = LittleLong(inLeaf->numLeafSurfaces);
	}	

	// chain decendants
	R_SetParent (s_worldData.nodes, NULL);
}

static	void R_LoadShaders( lump_t *l ) {	
	int		i, count;
	dshader_t	*in, *out;
	
	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);
	out = ri->Hunk_Alloc ( count*sizeof(*out), PREF_LOW );

	s_worldData.shaders = out;
	s_worldData.numShaders = count;

	memcpy( out, in, count*sizeof(*out) );

	for ( i=0 ; i<count ; i++ ) {
		out[i].surfaceFlags = LittleLong( out[i].surfaceFlags );
		out[i].contentFlags = LittleLong( out[i].contentFlags );
	}
}

static	void R_LoadMarksurfaces (lump_t *l) {	
	int		i, j, count;
	int		*in;
	msurface_t **out;
	
	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);
	out = ri->Hunk_Alloc ( count*sizeof(*out), PREF_LOW);	

	s_worldData.marksurfaces = out;
	s_worldData.nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleLong(in[i]);
		out[i] = s_worldData.surfaces + j;
	}
}

static	void R_LoadPlanes( lump_t *l ) {
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);
	out = ri->Hunk_Alloc ( count*2*sizeof(*out), PREF_LOW);	
	
	s_worldData.planes = out;
	s_worldData.numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++) {
		bits = 0;
		for (j=0 ; j<3 ; j++) {
			out->normal.data[j] = LittleFloat (in->normal.data[j]);
			if (out->normal.data[j] < 0) {
				bits |= 1<<j;
			}
		}

		out->dist = LittleFloat (in->dist);
		out->type = (byte)PlaneTypeForNormal( &out->normal );
		out->signbits = (byte)bits;
	}
}

static	void R_LoadFogs( lump_t *l, lump_t *brushesLump, lump_t *sidesLump ) {
	int			i;
	fog_t		*out;
	dfog_t		*fogs;
	dbrush_t 	*brushes, *brush;
	dbrushside_t	*sides;
	int			count, brushesCount, sidesCount;
	int			sideNum;
	int			planeNum;
	shader_t	*shader;
	float		d;
	int			firstSide;

	fogs = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*fogs)) {
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	count = l->filelen / sizeof(*fogs);

	// create fog strucutres for them
	s_worldData.numfogs = count + 1;
	s_worldData.fogs = ri->Hunk_Alloc ( s_worldData.numfogs*sizeof(*out), PREF_LOW);
	out = s_worldData.fogs + 1;

	if ( !count ) {
		return;
	}

	brushes = (void *)(fileBase + brushesLump->fileofs);
	if (brushesLump->filelen % sizeof(*brushes)) {
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	brushesCount = brushesLump->filelen / sizeof(*brushes);

	sides = (void *)(fileBase + sidesLump->fileofs);
	if (sidesLump->filelen % sizeof(*sides)) {
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	sidesCount = sidesLump->filelen / sizeof(*sides);

	for ( i=0 ; i<count ; i++, fogs++) {
		out->originalBrushNumber = LittleLong( fogs->brushNum );

		if ( out->originalBrushNumber >= brushesCount ) {
			ri->Error( ERR_DROP, "fog brushNumber out of range" );
		}
		brush = brushes + out->originalBrushNumber;

		firstSide = LittleLong( brush->firstSide );

			if ( firstSide > sidesCount - 6 ) {
			ri->Error( ERR_DROP, "fog brush sideNumber out of range" );
		}

		// brushes are always sorted with the axial sides first
		sideNum = firstSide + 0;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0].x = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 1;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1].x = s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 2;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0].y = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 3;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1].y = s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 4;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0].z = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 5;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1].z = s_worldData.planes[ planeNum ].dist;

		// get information from the shader for fog parameters
		shader = R_FindShader( fogs->shader, LIGHTMAP_NONE, qtrue );

		out->parms = shader->fogParms;

		out->colorInt = ColorBytes4 ( shader->fogParms.color.r * tr.identityLight, 
			                          shader->fogParms.color.g * tr.identityLight, 
			                          shader->fogParms.color.b * tr.identityLight, 1.0f );

		d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		out->tcScale = 1.0f / ( d * 8 );

		// set the gradient vector
		sideNum = LittleLong( fogs->visibleSide );

		if ( sideNum == -1 ) {
			out->hasSurface = qfalse;
		} else {
			out->hasSurface = qtrue;
			planeNum = LittleLong( sides[ firstSide + sideNum ].planeNum );
			VectorSubtract( &vec3_origin, &s_worldData.planes[ planeNum ].normal, (vector3 *)&out->surface );
			out->surface.w = -s_worldData.planes[ planeNum ].dist;
		}

		out++;
	}

}

void R_LoadLightGrid( lump_t *l ) {
	int		i;
	vector3	maxs;
	int		numGridPoints;
	world_t	*w;
	vector3 *wMins, *wMaxs;

	w = &s_worldData;

	w->lightGridInverseSize.x = 1.0f / w->lightGridSize.x;
	w->lightGridInverseSize.y = 1.0f / w->lightGridSize.y;
	w->lightGridInverseSize.z = 1.0f / w->lightGridSize.z;

	wMins = &w->bmodels[0].bounds[0];
	wMaxs = &w->bmodels[0].bounds[1];

	for ( i = 0 ; i < 3 ; i++ ) {
		w->lightGridOrigin.data[i] = w->lightGridSize.data[i] * ceilf( wMins->data[i] / w->lightGridSize.data[i] );
		maxs.data[i] = w->lightGridSize.data[i] * floorf( wMaxs->data[i] / w->lightGridSize.data[i] );
		w->lightGridBounds.data[i] = (int)((maxs.data[i] - w->lightGridOrigin.data[i])/w->lightGridSize.data[i]) + 1;
	}

	numGridPoints = w->lightGridBounds.x * w->lightGridBounds.y * w->lightGridBounds.z;

	if ( l->filelen != numGridPoints * 8 ) {
		ri->Printf( PRINT_WARNING, "WARNING: light grid mismatch\n" );
		w->lightGridData = NULL;
		return;
	}

	w->lightGridData = ri->Hunk_Alloc( l->filelen, PREF_LOW );
	memcpy( w->lightGridData, (void *)(fileBase + l->fileofs), l->filelen );

	// deal with overbright bits
	for ( i = 0 ; i < numGridPoints ; i++ ) {
		R_ColorShiftLightingBytes( &w->lightGridData[i*8], &w->lightGridData[i*8] );
		R_ColorShiftLightingBytes( &w->lightGridData[i*8+3], &w->lightGridData[i*8+3] );
	}
}

void R_LoadEntities( lump_t *l ) {
	const char *p;
	char *token;
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];
	world_t	*w;

	w = &s_worldData;
	w->lightGridSize.x = 64;
	w->lightGridSize.y = 64;
	w->lightGridSize.z = 128;

	p = (const char *)(fileBase + l->fileofs);

	// store for reference by the cgame
	w->entityString = (char *)ri->Hunk_Alloc( l->filelen + 1, PREF_LOW );
	Q_strncpyz( w->entityString, p, l->filelen+1 );
	w->entityParsePoint = w->entityString;

	token = COM_ParseExt( &p, qtrue );
	if (!*token || *token != '{') {
		return;
	}

	// only parse the world spawn
	while ( 1 ) {	
		// parse key
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz(keyname, token, sizeof(keyname));

		// parse value
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz(value, token, sizeof(value));

		// check for a different grid size
		if ( !Q_stricmp( keyname, "gridsize" ) )
		{
			if ( sscanf( value, "%f %f %f", &w->lightGridSize.x, &w->lightGridSize.y, &w->lightGridSize.z ) != 3 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: invalid argument count for gridsize '%s'", value );
				VectorSet( &w->lightGridSize, 64.0f, 64.0f, 128.0f );
			}
			continue;
		}
	}
}

qboolean R_GetEntityToken( char *buffer, size_t size ) {
	const char	*s;

	s = COM_Parse( &s_worldData.entityParsePoint );
	Q_strncpyz( buffer, s, size );
	if ( !s_worldData.entityParsePoint || !s[0] ) {
		s_worldData.entityParsePoint = s_worldData.entityString;
		return qfalse;
	} else {
		return qtrue;
	}
}

// Called directly from cgame
void RE_LoadWorldMap( const char *name ) {
	int			i;
	dheader_t	*header;
	union {
		byte *b;
		void *v;
	} buffer;
	byte		*startMarker;

	if ( tr.worldMapLoaded ) {
		ri->Error( ERR_DROP, "ERROR: attempted to redundantly load world map" );
	}

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection.x = 0.45f;
	tr.sunDirection.y = 0.3f;
	tr.sunDirection.z = 0.9f;

	VectorNormalize( &tr.sunDirection );

	tr.worldMapLoaded = qtrue;

	// load it
    ri->FS_ReadFile( name, &buffer.v );
	if ( !buffer.b ) {
		ri->Error (ERR_DROP, "RE_LoadWorldMap: %s not found", name);
	}

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	memset( &s_worldData, 0, sizeof( s_worldData ) );
	Q_strncpyz( s_worldData.name, name, sizeof( s_worldData.name ) );

	Q_strncpyz( s_worldData.baseName, COM_SkipPath( s_worldData.name ), sizeof( s_worldData.name ) );
	COM_StripExtension(s_worldData.baseName, s_worldData.baseName, sizeof(s_worldData.baseName));

	startMarker = ri->Hunk_Alloc(0, PREF_LOW);
	c_gridVerts = 0;

	header = (dheader_t *)buffer.b;
	fileBase = (byte *)header;

	i = LittleLong (header->version);
	if ( i != BSP_VERSION ) {
		ri->Error (ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (%i should be %i)", 
			name, i, BSP_VERSION);
	}

	// swap all the lumps
	for (i=0 ; i<sizeof(dheader_t)/4 ; i++) {
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);
	}

	// load into heap
	R_LoadShaders( &header->lumps[LUMP_SHADERS] );
	R_LoadLightmaps( &header->lumps[LUMP_LIGHTMAPS] );
	R_LoadPlanes (&header->lumps[LUMP_PLANES]);
	R_LoadFogs( &header->lumps[LUMP_FOGS], &header->lumps[LUMP_BRUSHES], &header->lumps[LUMP_BRUSHSIDES] );
	R_LoadSurfaces( &header->lumps[LUMP_SURFACES], &header->lumps[LUMP_DRAWVERTS], &header->lumps[LUMP_DRAWINDEXES] );
	R_LoadMarksurfaces (&header->lumps[LUMP_LEAFSURFACES]);
	R_LoadNodesAndLeafs (&header->lumps[LUMP_NODES], &header->lumps[LUMP_LEAFS]);
	R_LoadSubmodels (&header->lumps[LUMP_MODELS]);
	R_LoadVisibility( &header->lumps[LUMP_VISIBILITY] );
	R_LoadEntities( &header->lumps[LUMP_ENTITIES] );
	R_LoadLightGrid( &header->lumps[LUMP_LIGHTGRID] );

	s_worldData.dataSize = (byte *)ri->Hunk_Alloc(0, PREF_LOW) - startMarker;

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

    ri->FS_FreeFile( buffer.v );
}

