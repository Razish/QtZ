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
//
// q_math.c -- stateless support routines that are included in each code module

#include "q_shared.h"

vector3	vec3_origin = {0,0,0};
vector3	axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };


vector4 colorBlack		= { 0.0f,	0.0f,	0.0f,	1.0f };
vector4 colorRed		= { 1.0f,	0.0f,	0.0f,	1.0f };
vector4 colorGreen		= { 0.0f,	1.0f,	0.0f,	1.0f };
vector4 colorBlue		= { 0.0f,	0.0f,	1.0f,	1.0f };
vector4 colorYellow		= { 1.0f,	1.0f,	0.0f,	1.0f };
vector4 colorMagenta	= { 1.0f,	0.0f,	1.0f,	1.0f };
vector4 colorCyan		= { 0.0f,	1.0f,	1.0f,	1.0f };
vector4 colorWhite		= { 1.0f,	1.0f,	1.0f,	1.0f };
vector4 colorGrey		= { 0.5f,	0.5f,	0.5f,	1.0f };
vector4 colorOrange		= { 1.0f,	0.5f,	0.0f,	1.0f };
vector4 colorLtGrey		= { 0.75f,	0.75f,	0.75f,	1.0f };
vector4 colorMdGrey		= { 0.5f,	0.5f,	0.5f,	1.0f };
vector4 colorDkGrey		= { 0.25f,	0.25f,	0.25f,	1.0f };

vector4 g_color_table[10] = {
	{ 0.0f,		0.0f,	0.0f,	1.0f }, // black
	{ 1.0f,		0.0f,	0.0f,	1.0f }, // red
	{ 0.0f,		1.0f,	0.0f,	1.0f }, // green
	{ 1.0f,		0.75f,	0.125f,	1.0f }, // yellow
	{ 0.333f,	0.5f,	1.0f,	1.0f }, // blue
	{ 0.0f,		1.0f,	1.0f,	1.0f }, // cyan
	{ 1.0f,		0.0f,	1.0f,	1.0f }, // magenta
	{ 1.0f,		1.0f,	1.0f,	1.0f }, // white
	{ 0.5f,		0.5f,	0.5f,	1.0f }, // grey
	{ 1.0f,		0.5f,	0.0f,	1.0f }, // orange
};


vector3 bytedirs[NUMVERTEXNORMALS] = {
	{ -0.525731f,  0.000000f,  0.850651f }, { -0.442863f,  0.238856f,  0.864188f }, { -0.295242f,  0.000000f,  0.955423f },
	{ -0.309017f,  0.500000f,  0.809017f }, { -0.162460f,  0.262866f,  0.951056f }, {  0.000000f,  0.000000f,  1.000000f },
	{  0.000000f,  0.850651f,  0.525731f }, { -0.147621f,  0.716567f,  0.681718f }, {  0.147621f,  0.716567f,  0.681718f },
	{  0.000000f,  0.525731f,  0.850651f }, {  0.309017f,  0.500000f,  0.809017f }, {  0.525731f,  0.000000f,  0.850651f },
	{  0.295242f,  0.000000f,  0.955423f }, {  0.442863f,  0.238856f,  0.864188f }, {  0.162460f,  0.262866f,  0.951056f },
	{ -0.681718f,  0.147621f,  0.716567f }, { -0.809017f,  0.309017f,  0.500000f }, { -0.587785f,  0.425325f,  0.688191f },
	{ -0.850651f,  0.525731f,  0.000000f }, { -0.864188f,  0.442863f,  0.238856f }, { -0.716567f,  0.681718f,  0.147621f },
	{ -0.688191f,  0.587785f,  0.425325f }, { -0.500000f,  0.809017f,  0.309017f }, { -0.238856f,  0.864188f,  0.442863f },
	{ -0.425325f,  0.688191f,  0.587785f }, { -0.716567f,  0.681718f, -0.147621f }, { -0.500000f,  0.809017f, -0.309017f },
	{ -0.525731f,  0.850651f,  0.000000f }, {  0.000000f,  0.850651f, -0.525731f }, { -0.238856f,  0.864188f, -0.442863f },
	{  0.000000f,  0.955423f, -0.295242f }, { -0.262866f,  0.951056f, -0.162460f }, {  0.000000f,  1.000000f,  0.000000f },
	{  0.000000f,  0.955423f,  0.295242f }, { -0.262866f,  0.951056f,  0.162460f }, {  0.238856f,  0.864188f,  0.442863f },
	{  0.262866f,  0.951056f,  0.162460f }, {  0.500000f,  0.809017f,  0.309017f }, {  0.238856f,  0.864188f, -0.442863f },
	{  0.262866f,  0.951056f, -0.162460f }, {  0.500000f,  0.809017f, -0.309017f }, {  0.850651f,  0.525731f,  0.000000f },
	{  0.716567f,  0.681718f,  0.147621f }, {  0.716567f,  0.681718f, -0.147621f }, {  0.525731f,  0.850651f,  0.000000f },
	{  0.425325f,  0.688191f,  0.587785f }, {  0.864188f,  0.442863f,  0.238856f }, {  0.688191f,  0.587785f,  0.425325f },
	{  0.809017f,  0.309017f,  0.500000f }, {  0.681718f,  0.147621f,  0.716567f }, {  0.587785f,  0.425325f,  0.688191f },
	{  0.955423f,  0.295242f,  0.000000f }, {  1.000000f,  0.000000f,  0.000000f }, {  0.951056f,  0.162460f,  0.262866f },
	{  0.850651f, -0.525731f,  0.000000f }, {  0.955423f, -0.295242f,  0.000000f }, {  0.864188f, -0.442863f,  0.238856f },
	{  0.951056f, -0.162460f,  0.262866f }, {  0.809017f, -0.309017f,  0.500000f }, {  0.681718f, -0.147621f,  0.716567f },
	{  0.850651f,  0.000000f,  0.525731f }, {  0.864188f,  0.442863f, -0.238856f }, {  0.809017f,  0.309017f, -0.500000f },
	{  0.951056f,  0.162460f, -0.262866f }, {  0.525731f,  0.000000f, -0.850651f }, {  0.681718f,  0.147621f, -0.716567f },
	{  0.681718f, -0.147621f, -0.716567f }, {  0.850651f,  0.000000f, -0.525731f }, {  0.809017f, -0.309017f, -0.500000f },
	{  0.864188f, -0.442863f, -0.238856f }, {  0.951056f, -0.162460f, -0.262866f }, {  0.147621f,  0.716567f, -0.681718f },
	{  0.309017f,  0.500000f, -0.809017f }, {  0.425325f,  0.688191f, -0.587785f }, {  0.442863f,  0.238856f, -0.864188f },
	{  0.587785f,  0.425325f, -0.688191f }, {  0.688191f,  0.587785f, -0.425325f }, { -0.147621f,  0.716567f, -0.681718f },
	{ -0.309017f,  0.500000f, -0.809017f }, {  0.000000f,  0.525731f, -0.850651f }, { -0.525731f,  0.000000f, -0.850651f },
	{ -0.442863f,  0.238856f, -0.864188f }, { -0.295242f,  0.000000f, -0.955423f }, { -0.162460f,  0.262866f, -0.951056f },
	{  0.000000f,  0.000000f, -1.000000f }, {  0.295242f,  0.000000f, -0.955423f }, {  0.162460f,  0.262866f, -0.951056f },
	{ -0.442863f, -0.238856f, -0.864188f }, { -0.309017f, -0.500000f, -0.809017f }, { -0.162460f, -0.262866f, -0.951056f },
	{  0.000000f, -0.850651f, -0.525731f }, { -0.147621f, -0.716567f, -0.681718f }, {  0.147621f, -0.716567f, -0.681718f },
	{  0.000000f, -0.525731f, -0.850651f }, {  0.309017f, -0.500000f, -0.809017f }, {  0.442863f, -0.238856f, -0.864188f },
	{  0.162460f, -0.262866f, -0.951056f }, {  0.238856f, -0.864188f, -0.442863f }, {  0.500000f, -0.809017f, -0.309017f },
	{  0.425325f, -0.688191f, -0.587785f }, {  0.716567f, -0.681718f, -0.147621f }, {  0.688191f, -0.587785f, -0.425325f },
	{  0.587785f, -0.425325f, -0.688191f }, {  0.000000f, -0.955423f, -0.295242f }, {  0.000000f, -1.000000f,  0.000000f },
	{  0.262866f, -0.951056f, -0.162460f }, {  0.000000f, -0.850651f,  0.525731f }, {  0.000000f, -0.955423f,  0.295242f },
	{  0.238856f, -0.864188f,  0.442863f }, {  0.262866f, -0.951056f,  0.162460f }, {  0.500000f, -0.809017f,  0.309017f },
	{  0.716567f, -0.681718f,  0.147621f }, {  0.525731f, -0.850651f,  0.000000f }, { -0.238856f, -0.864188f, -0.442863f },
	{ -0.500000f, -0.809017f, -0.309017f }, { -0.262866f, -0.951056f, -0.162460f }, { -0.850651f, -0.525731f,  0.000000f },
	{ -0.716567f, -0.681718f, -0.147621f }, { -0.716567f, -0.681718f,  0.147621f }, { -0.525731f, -0.850651f,  0.000000f },
	{ -0.500000f, -0.809017f,  0.309017f }, { -0.238856f, -0.864188f,  0.442863f }, { -0.262866f, -0.951056f,  0.162460f },
	{ -0.864188f, -0.442863f,  0.238856f }, { -0.809017f, -0.309017f,  0.500000f }, { -0.688191f, -0.587785f,  0.425325f },
	{ -0.681718f, -0.147621f,  0.716567f }, { -0.442863f, -0.238856f,  0.864188f }, { -0.587785f, -0.425325f,  0.688191f },
	{ -0.309017f, -0.500000f,  0.809017f }, { -0.147621f, -0.716567f,  0.681718f }, { -0.425325f, -0.688191f,  0.587785f },
	{ -0.162460f, -0.262866f,  0.951056f }, {  0.442863f, -0.238856f,  0.864188f }, {  0.162460f, -0.262866f,  0.951056f },
	{  0.309017f, -0.500000f,  0.809017f }, {  0.147621f, -0.716567f,  0.681718f }, {  0.000000f, -0.525731f,  0.850651f },
	{  0.425325f, -0.688191f,  0.587785f }, {  0.587785f, -0.425325f,  0.688191f }, {  0.688191f, -0.587785f,  0.425325f },
	{ -0.955423f,  0.295242f,  0.000000f }, { -0.951056f,  0.162460f,  0.262866f }, { -1.000000f,  0.000000f,  0.000000f },
	{ -0.850651f,  0.000000f,  0.525731f }, { -0.955423f, -0.295242f,  0.000000f }, { -0.951056f, -0.162460f,  0.262866f },
	{ -0.864188f,  0.442863f, -0.238856f }, { -0.951056f,  0.162460f, -0.262866f }, { -0.809017f,  0.309017f, -0.500000f },
	{ -0.864188f, -0.442863f, -0.238856f }, { -0.951056f, -0.162460f, -0.262866f }, { -0.809017f, -0.309017f, -0.500000f },
	{ -0.681718f,  0.147621f, -0.716567f }, { -0.681718f, -0.147621f, -0.716567f }, { -0.850651f,  0.000000f, -0.525731f },
	{ -0.688191f,  0.587785f, -0.425325f }, { -0.587785f,  0.425325f, -0.688191f }, { -0.425325f,  0.688191f, -0.587785f },
	{ -0.425325f, -0.688191f, -0.587785f }, { -0.587785f, -0.425325f, -0.688191f }, { -0.688191f, -0.587785f, -0.425325f }
};

//==============================================================

int		Q_rand( int *seed ) {
	*seed = (69069 * *seed + 1);
	return *seed;
}

float	Q_random( int *seed ) {
	return ( Q_rand( seed ) & 0xffff ) / (float)0x10000;
}

float	Q_crandom( int *seed ) {
	return 2.0f * ( Q_random( seed ) - 0.5f );
}

//=======================================================

signed char ClampChar( int i ) {
	if ( i < -128 ) {
		return -128;
	}
	if ( i > 127 ) {
		return 127;
	}
	return i;
}

signed short ClampShort( int i ) {
	if ( i < -32768 ) {
		return -32768;
	}
	if ( i > 0x7fff ) {
		return 0x7fff;
	}
	return i;
}

// this isn't a real cheap function to call!
int DirToByte( vector3 *dir ) {
	int		i, best;
	float	d, bestd;

	if ( !dir ) {
		return 0;
	}

	bestd = 0;
	best = 0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		d = DotProduct (dir, &bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir( int b, vector3 *dir ) {
	if ( b < 0 || b >= NUMVERTEXNORMALS ) {
		VectorCopy( &vec3_origin, dir );
		return;
	}
	VectorCopy (&bytedirs[b], dir);
}


unsigned int ColorBytes3 (float r, float g, float b) {
	unsigned int i;

	( (byte *)&i )[0] = (byte)(r * 255);
	( (byte *)&i )[1] = (byte)(g * 255);
	( (byte *)&i )[2] = (byte)(b * 255);

	return i;
}

unsigned int ColorBytes4 (float r, float g, float b, float a) {
	unsigned int i;

	( (byte *)&i )[0] = (byte)(r * 255);
	( (byte *)&i )[1] = (byte)(g * 255);
	( (byte *)&i )[2] = (byte)(b * 255);
	( (byte *)&i )[3] = (byte)(a * 255);

	return i;
}

float NormalizeColor( const vector3 *in, vector3 *out ) {
	float	max;
	
	max = in->r;
	if ( in->g > max ) {
		max = in->g;
	}
	if ( in->b > max ) {
		max = in->b;
	}

	if ( !max ) {
		VectorClear( out );
	} else {
		out->r = in->r / max;
		out->g = in->g / max;
		out->b = in->b / max;
	}
	return max;
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints( vector4 *plane, const vector3 *a, const vector3 *b, const vector3 *c ) {
	vector3	d1, d2;

	VectorSubtract( b, a, &d1 );
	VectorSubtract( c, a, &d2 );
	CrossProduct( &d2, &d1, (vector3*)plane );
	if ( VectorNormalize( (vector3*)plane ) == 0 ) {
		return qfalse;
	}

	plane->w = DotProduct( a, (vector3*)plane );
	return qtrue;
}

/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
void RotatePointAroundVector( vector3 *dst, const vector3 *dir, const vector3 *point, float degrees ) {
	vector3 m[3], im[3], zrot[3], tmpmat[3], rot[3];
	vector3 vr, vup, vf;
	int	i;
	float	rad;

	vf.x = dir->x;
	vf.y = dir->y;
	vf.z = dir->z;

	PerpendicularVector( &vr, dir );
	CrossProduct( &vr, &vf, &vup );

	m[0].x = vr.x;
	m[1].x = vr.y;
	m[2].x = vr.z;

	m[0].y = vup.x;
	m[1].y = vup.y;
	m[2].y = vup.z;

	m[0].z = vf.x;
	m[1].z = vf.y;
	m[2].z = vf.z;

	memcpy( im, m, sizeof( im ) );

	im[0].y = m[1].x;
	im[0].z = m[2].x;
	im[1].x = m[0].y;
	im[1].z = m[2].y;
	im[2].x = m[0].z;
	im[2].y = m[1].z;

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0].x = zrot[1].y = zrot[2].z = 1.0f;

	rad = DEG2RAD( degrees );
	zrot[0].x =  cosf( rad );
	zrot[0].y =  sinf( rad );
	zrot[1].x = -sinf( rad );
	zrot[1].y =  cosf( rad );

	MatrixMultiply( m, zrot, tmpmat );
	MatrixMultiply( tmpmat, im, rot );

	for ( i=0; i<3; i++ )
		dst->data[i] = rot[i].x * point->x + rot[i].y * point->y + rot[i].z * point->z;
}

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection( vector3 axis[3], float yaw ) {

	// create an arbitrary axis[1] 
	PerpendicularVector( &axis[1], &axis[0] );

	// rotate it around axis[0] by yaw
	if ( yaw ) {
		vector3	temp;

		VectorCopy( &axis[1], &temp );
		RotatePointAroundVector( &axis[1], &axis[0], &temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( &axis[0], &axis[1], &axis[2] );
}



void vectoangles( const vector3 *value1, vector3 *angles ) {
	float	forward;
	float	yaw, pitch;
	
	if ( value1->y == 0 && value1->x == 0 ) {
		yaw = 0.0f;
		if ( value1->z > 0.0f ) {
			pitch = 90.0f;
		}
		else {
			pitch = 270.0f;
		}
	}
	else {
		if ( value1->x ) {
			yaw = ( atan2f( value1->y, value1->x ) * 180.0f / M_PI );
		}
		else if ( value1->y > 0.0f ) {
			yaw = 90.0f;
		}
		else {
			yaw = 270.0f;
		}
		if ( yaw < 0 ) {
			yaw += 360.0f;
		}

		forward = sqrtf( value1->x*value1->x + value1->y*value1->y );
		pitch = ( atan2f(value1->z, forward) * 180.0f / M_PI );
		if ( pitch < 0 ) {
			pitch += 360.0f;
		}
	}

	angles->pitch = -pitch;
	angles->yaw = yaw;
	angles->roll = 0.0f;
}


/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vector3 *angles, vector3 axis[3] ) {
	vector3	right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, &axis[0], &right, &axis[2] );
	VectorSubtract( &vec3_origin, &right, &axis[1] );
}

void AxisClear( vector3 axis[3] ) {
	axis[0].x = 1;
	axis[0].y = 0;
	axis[0].z = 0;

	axis[1].x = 0;
	axis[1].y = 1;
	axis[1].z = 0;

	axis[2].x = 0;
	axis[2].y = 0;
	axis[2].z = 1;
}

void AxisCopy( vector3 in[3], vector3 out[3] ) {
	VectorCopy( &in[0], &out[0] );
	VectorCopy( &in[1], &out[1] );
	VectorCopy( &in[2], &out[2] );
}

void ProjectPointOnPlane( vector3 *dst, const vector3 *p, const vector3 *normal )
{
	float d;
	vector3 n;
	float inv_denom;

	inv_denom =  DotProduct( normal, normal );
	assert( Q_fabs(inv_denom) != 0.0f ); // bk010122 - zero vectors get here
	inv_denom = 1.0f / inv_denom;

	d = DotProduct( normal, p ) * inv_denom;

	n.x = normal->x * inv_denom;
	n.y = normal->y * inv_denom;
	n.z = normal->z * inv_denom;

	dst->x = p->x - d * n.x;
	dst->y = p->y - d * n.y;
	dst->z = p->z - d * n.z;
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors( const vector3 *forward, vector3 *right, vector3 *up) {
	float		d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right->y = -forward->x;
	right->z = forward->y;
	right->x = forward->z;

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right);
	CrossProduct (right, forward, up);
}


QINLINE void VectorRotate( vector3 *in, vector3 matrix[3], vector3 *out ) {
	out->x = DotProduct( in, &matrix[0] );
	out->y = DotProduct( in, &matrix[1] );
	out->z = DotProduct( in, &matrix[2] );
}

//============================================================================

#ifndef idppc
/*
** float q_rsqrt( float number )
*/
float Q_rsqrt( float number )
{
	floatint_t t;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	t.f  = number;
	t.i  = 0x5f3759df - ( t.i >> 1 );               // what the fuck?
	y  = t.f;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

float Q_fabs( float f ) {
	floatint_t fi;
	fi.f = f;
	fi.i &= 0x7FFFFFFF;
	return fi.f;
}
#endif

//============================================================

/*
===============
LerpAngle

===============
*/
float LerpAngle (float from, float to, float frac) {
	float	a;

	if ( to - from > 180 ) {
		to -= 360;
	}
	if ( to - from < -180 ) {
		to += 360;
	}
	a = from + frac * (to - from);

	return a;
}


/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float	AngleSubtract( float a1, float a2 ) {
	float	a;

	a = a1 - a2;
	while ( a > 180 ) {
		a -= 360;
	}
	while ( a < -180 ) {
		a += 360;
	}
	return a;
}


void AnglesSubtract( vector3 *v1, vector3 *v2, vector3 *v3 ) {
	v3->x = AngleSubtract( v1->x, v2->x );
	v3->y = AngleSubtract( v1->y, v2->y );
	v3->z = AngleSubtract( v1->z, v2->z );
}


float AngleMod(float a) {
	a = (360.0f/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}


/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
float AngleNormalize360 ( float angle ) {
	return (360.0f/ 65536) * ((int)(angle * (65536 / 360.0f)) & 65535);
}


/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
float AngleNormalize180 ( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0 ) {
		angle -= 360.0;
	}
	return angle;
}


/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
float AngleDelta ( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}


//============================================================


/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits (cplane_t *out) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j=0 ; j<3 ; j++) {
		if (out->normal.data[j] < 0) {
			bits |= 1<<j;
		}
	}
	out->signbits = bits;
}


/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide(vector3 *emins, vector3 *emaxs, struct cplane_s *p)
{
	float	dist[2];
	int		sides, b, i;

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins->data[p->type])
			return 1;
		if (p->dist >= emaxs->data[p->type])
			return 2;
		return 3;
	}

	// general case
	dist[0] = dist[1] = 0;
	if (p->signbits < 8) // >= 8: default case is original code (dist[0]=dist[1]=0)
	{
		for (i=0 ; i<3 ; i++)
		{
			b = (p->signbits >> i) & 1;
			dist[ b] += p->normal.data[i]*emaxs->data[i];
			dist[!b] += p->normal.data[i]*emins->data[i];
		}
	}

	sides = 0;
	if (dist[0] >= p->dist)		sides  = 1;
	if (dist[1] <  p->dist)		sides |= 2;

	return sides;
}


/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( const vector3 *mins, const vector3 *maxs ) {
	int		i;
	vector3	corner;
	float	a, b;

	for (i=0 ; i<3 ; i++) {
		a = fabsf( mins->data[i] );
		b = fabsf( maxs->data[i] );
		corner.data[i] = a > b ? a : b;
	}

	return VectorLength (&corner);
}


void ClearBounds( vector3 *mins, vector3 *maxs ) {
	mins->x = mins->y = mins->z = 99999;
	maxs->x = maxs->y = maxs->z = -99999;
}

number DistanceHorizontal( const vector3 *p1, const vector3 *p2 ) {
	vector3	v;

	VectorSubtract( p2, p1, &v );
	return sqrtf( v.x*v.x + v.y*v.y );	//Leave off the z component
}

number DistanceHorizontalSquared( const vector3 *p1, const vector3 *p2 ) {
	vector3	v;

	VectorSubtract( p2, p1, &v );
	return v.x*v.x + v.y*v.y;	//Leave off the z component
}

void AddPointToBounds( const vector3 *v, vector3 *mins, vector3 *maxs ) {
	if ( v->x < mins->x )	mins->x = v->x;
	if ( v->x > maxs->x )	maxs->x = v->x;

	if ( v->y < mins->y )	mins->y = v->y;
	if ( v->y > maxs->y )	maxs->y = v->y;

	if ( v->z < mins->z )	mins->z = v->z;
	if ( v->z > maxs->z )	maxs->z = v->z;
}

qboolean BoundsIntersect( const vector3 *mins, const vector3 *maxs, const vector3 *mins2, const vector3 *maxs2 )
{
	if ( maxs->x < mins2->x || maxs->y < mins2->y || maxs->z < mins2->z ||
		 mins->x > maxs2->x || mins->y > maxs2->y || mins->z > maxs2->z )
		return qfalse;

	return qtrue;
}

qboolean BoundsIntersectSphere( const vector3 *mins, const vector3 *maxs, const vector3 *origin, number radius )
{
	if ( origin->x - radius > maxs->x ||
		 origin->x + radius < mins->x ||
		 origin->y - radius > maxs->y ||
		 origin->y + radius < mins->y ||
		 origin->z - radius > maxs->z ||
		 origin->z + radius < mins->z )
		return qfalse;

	return qtrue;
}

qboolean BoundsIntersectPoint( const vector3 *mins, const vector3 *maxs, const vector3 *origin )
{
	if ( origin->x > maxs->x ||
		 origin->x < mins->x ||
		 origin->y > maxs->y ||
		 origin->y < mins->y ||
		 origin->z > maxs->z ||
		 origin->z < mins->z )
		return qfalse;

	return qtrue;
}

QINLINE void VectorAdd( const vector3 *vec1, const vector3 *vec2, vector3 *vecOut ) {
#ifdef USE_SSE
	__asm {
		mov ecx, vec1
		movss xmm0, [ecx]
		movhps xmm0, [ecx+4]

		mov edx, vec2
		movss xmm1, [edx]
		movhps xmm1, [edx+4]

		addps xmm0, xmm1

		mov eax, vecOut
		movss [eax], xmm0
		movhps [eax+4], xmm0
	}
#else
	vecOut->x = vec1->x + vec2->x;
	vecOut->y = vec1->y + vec2->y;
	vecOut->z = vec1->z + vec2->z;
#endif
}

QINLINE void VectorSubtract( const vector3 *vec1, const vector3 *vec2, vector3 *vecOut ) {
#ifdef USE_SSE
	__asm {
		mov ecx, vec1
		movss xmm0, [ecx]
		movhps xmm0, [ecx+4]

		mov edx, vec2
		movss xmm1, [edx]
		movhps xmm1, [edx+4]

		subps xmm0, xmm1

		mov eax, vecOut
		movss [eax], xmm0
		movhps [eax+4], xmm0
	}
#else
	vecOut->x = vec1->x - vec2->x;
	vecOut->y = vec1->y - vec2->y;
	vecOut->z = vec1->z - vec2->z;
#endif
}

QINLINE void VectorNegate( const vector3 *vecIn, vector3 *vecOut ) {
	vecOut->x = -vecIn->x;
	vecOut->y = -vecIn->y;
	vecOut->z = -vecIn->z;
}

QINLINE void VectorScale( const vector3 *vecIn, number scale, vector3 *vecOut ) {
#ifdef USE_SSE
	__asm {
		movss xmm0, scale
		shufps xmm0, xmm0, 0x0

		mov edx, vecIn
		movss xmm1, [edx]
		movhps xmm1, [edx+4]

		mulps xmm0, xmm1

		mov eax, vecOut
		movss [eax], xmm0
		movhps [eax+4], xmm0
	}
#else
	vecOut->x = vecIn->x * scale;
	vecOut->y = vecIn->y * scale;
	vecOut->z = vecIn->z * scale;
#endif
}

QINLINE void VectorScale4( const vector4 *vecIn, number scale, vector4 *vecOut ) {
	vecOut->x = vecIn->x * scale;
	vecOut->y = vecIn->y * scale;
	vecOut->z = vecIn->z * scale;
	vecOut->w = vecIn->w * scale;
}

QINLINE void VectorScaleVector( const vector3 *vecIn, const vector3 *vecScale, vector3 *vecOut ) {
	vecOut->x = vecIn->x * vecScale->x;
	vecOut->y = vecIn->y * vecScale->y;
	vecOut->z = vecIn->z * vecScale->z;
}

QINLINE void VectorMA( const vector3 *vec1, number scale, const vector3 *vec2, vector3 *vecOut ) {
	vecOut->x = vec1->x + vec2->x*scale;
	vecOut->y = vec1->y + vec2->y*scale;
	vecOut->z = vec1->z + vec2->z*scale;
}

QINLINE void VectorLerp( const vector3 *vec1, number frac, const vector3 *vec2, vector3 *vecOut ) {
	vecOut->x = vec1->x + (vec2->x-vec1->x)*frac;
	vecOut->y = vec1->y + (vec2->y-vec1->y)*frac;
	vecOut->z = vec1->z + (vec2->z-vec1->z)*frac;
}

QINLINE void VectorLerp4( const vector4 *vec1, number frac, const vector4 *vec2, vector4 *vecOut ) {
	vecOut->x = vec1->x + (vec2->x-vec1->x)*frac;
	vecOut->y = vec1->y + (vec2->y-vec1->y)*frac;
	vecOut->z = vec1->z + (vec2->z-vec1->z)*frac;
	vecOut->w = vec1->w + (vec2->w-vec1->w)*frac;
}

QINLINE number VectorLength( const vector3 *vec ) {
#ifdef USE_SSE
	float res;

	__asm {
		mov edx, vec
		movss xmm1, [edx]
		movhps xmm1, [edx+4]

		movaps xmm2, xmm1

		mulps xmm1, xmm2

		movaps xmm0, xmm1

		shufps xmm0, xmm0, 0x32
		addps xmm1, xmm0

		shufps xmm0, xmm0, 0x32
		addps xmm1, xmm0

		sqrtss xmm1, xmm1
		movss [res], xmm1
	}

	return res;
#else
	return (number)sqrt( vec->x*vec->x + vec->y*vec->y + vec->z*vec->z );
#endif
}

QINLINE number VectorLengthSquared( const vector3 *vec ) {
#ifdef USE_SSE
	float res;

	__asm {
		mov edx, vec
		movss xmm1, [edx]
		movhps xmm1, [edx+4]

		movaps xmm2, xmm1

		mulps xmm1, xmm2

		movaps xmm0, xmm1

		shufps xmm0, xmm0, 0x32
		addps xmm1, xmm0

		shufps xmm0, xmm0, 0x32
		addps xmm1, xmm0

		movss [res], xmm1
	}

	return res;
#else
	return (vec->x*vec->x + vec->y*vec->y + vec->z*vec->z);
#endif
}

QINLINE number Distance( const vector3 *p1, const vector3 *p2 ) {
	vector3	v;

	VectorSubtract( p2, p1, &v );
	return VectorLength( &v );
}

QINLINE number DistanceSquared( const vector3 *p1, const vector3 *p2 ) {
	vector3	v;

	VectorSubtract( p2, p1, &v );
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation
QINLINE void VectorNormalizeFast( vector3 *vec )
{
	float ilength;

	ilength = Q_rsqrt( DotProduct( vec, vec ) );

	vec->x *= ilength;
	vec->y *= ilength;
	vec->z *= ilength;
}

QINLINE number VectorNormalize( vector3 *vec ) {
	float length, ilength;

	length = vec->x*vec->x + vec->y*vec->y + vec->z*vec->z;
	length = sqrtf( length );

	if ( length ) {
		ilength = 1/length;
		vec->x *= ilength;
		vec->y *= ilength;
		vec->z *= ilength;
	}

	return length;
}

QINLINE number VectorNormalize2( const vector3 *vec, vector3 *vecOut ) {
	float	length, ilength;

	length = vec->x*vec->x + vec->y*vec->y + vec->z*vec->z;
	length = sqrtf( length );

	if ( length ) {
		ilength = 1/length;
		vecOut->x = vec->x * ilength;
		vecOut->y = vec->y * ilength;
		vecOut->z = vec->z * ilength;
	}
	else
		VectorClear( vecOut );

	return length;
}

QINLINE void VectorCopy( const vector3 *vecIn, vector3 *vecOut ) {
	vecOut->x = vecIn->x;
	vecOut->y = vecIn->y;
	vecOut->z = vecIn->z;
}
QINLINE void IVectorCopy( const ivector3 *vecIn, ivector3 *vecOut ) {
	vecOut->x = vecIn->x;
	vecOut->y = vecIn->y;
	vecOut->z = vecIn->z;
}

QINLINE void VectorCopy4( const vector4 *vecIn, vector4 *vecOut ) {
	vecOut->x = vecIn->x;
	vecOut->y = vecIn->y;
	vecOut->z = vecIn->z;
	vecOut->w = vecIn->w;
}

QINLINE void VectorSet( vector3 *vec, number x, number y, number z ) {
	vec->x = x;
	vec->y = y;
	vec->z = z;
}

QINLINE void VectorSet4( vector4 *vec, number x, number y, number z, number w ) {
	vec->x = x;
	vec->y = y;
	vec->z = z;
	vec->w = w;
}

QINLINE void VectorClear( vector3 *vec ) {
	vec->x = vec->y = vec->z = 0.0f;
}

QINLINE void VectorClear4( vector4 *vec ) {
	vec->x = vec->y = vec->z = vec->w = 0.0f;
}

QINLINE void VectorInc( vector3 *vec ) {
	vec->x += 1.0f;
	vec->y += 1.0f;
	vec->z += 1.0f;
}

QINLINE void VectorDec( vector3 *vec ) {
	vec->x -= 1.0f;
	vec->y -= 1.0f;
	vec->z -= 1.0f;
}

QINLINE void VectorInverse( vector3 *vec ) {
	vec->x = -vec->x;
	vec->y = -vec->y;
	vec->z = -vec->z;
}

QINLINE void CrossProduct( const vector3 *vec1, const vector3 *vec2, vector3 *vecOut ) {
	vecOut->x = (vec1->y * vec2->z) - (vec1->z * vec2->y);
	vecOut->y = (vec1->z * vec2->x) - (vec1->x * vec2->z);
	vecOut->z = (vec1->x * vec2->y) - (vec1->y * vec2->x);
}

QINLINE number DotProduct( const vector3 *vec1, const vector3 *vec2 ) {
#ifdef USE_SSE
	float res;

	__asm {
		mov edx, vec1
		movss xmm1, [edx]
		movhps xmm1, [edx+4]

		mov edx, vec2
		movss xmm2, [edx]
		movhps xmm2, [edx+4]

		mulps xmm1, xmm2

		movaps xmm0, xmm1

		shufps xmm0, xmm0, 0x32
		addps xmm1, xmm0

		shufps xmm0, xmm0, 0x32
		addps xmm1, xmm0

		movss [res], xmm1
	}

	return res;
#else
	return vec1->x*vec2->x + vec1->y*vec2->y + vec1->z*vec2->z;
#endif
}

QINLINE qboolean VectorCompare( const vector3 *vec1, const vector3 *vec2 ) {
	if ( vec1->x == vec2->x &&
		 vec1->y == vec2->y &&
		 vec1->z == vec2->z )
		return qtrue;
	return qfalse;
}

#if _MSC_VER // windows

	#include <float.h>
	#pragma fenv_access( on )

	static QINLINE float roundfloat( float n ) {
		return (n < 0.0f) ? ceilf(n + 0.5f) : floorf(n + 0.5f);
	}

#else // linux, mac

	#include <fenv.h>

#endif

/*
======================
VectorSnap

Round a vector to integers for more efficient network transmission
======================
*/
QINLINE void VectorSnap( vector3 *v ) {
#if _MSC_VER
	unsigned int oldcontrol, newcontrol;

	_controlfp_s( &oldcontrol, 0, 0 );
	_controlfp_s( &newcontrol, _RC_NEAR, _MCW_RC );

	v->x = roundfloat( v->x );
	v->y = roundfloat( v->y );
	v->z = roundfloat( v->z );

	_controlfp_s( &newcontrol, oldcontrol, _MCW_RC );
#else // pure c99
	int oldround = fegetround();
	fesetround( FE_TONEAREST );

	v->x = nearbyint( v->x );
	v->y = nearbyint( v->y );
	v->z = nearbyint( v->z );

	fesetround( oldround );
#endif
}

/*
======================
VectorSnapTowards

Round a vector to integers for more efficient network transmission,
but make sure that it rounds towards a given point rather than blindly truncating.
This prevents it from truncating into a wall.
======================
*/
QINLINE void VectorSnapTowards( vector3 *v, vector3 *to ) {
	int i;

	for ( i=0; i<3; i++ ) {
		if ( to->data[i] <= v->data[i] )
			v->data[i] = floorf( v->data[i] );
		else
			v->data[i] = ceilf( v->data[i] );
	}
}

int Q_log2( int val ) {
	int answer;

	answer = 0;
	while ( ( val>>=1 ) != 0 ) {
		answer++;
	}
	return answer;
}

/*
================
MatrixMultiply
================
*/
void MatrixMultiply( vector3 in1[3], vector3 in2[3], vector3 out[3] ) {
	out[0].x = in1[0].x*in2[0].x + in1[0].y*in2[1].x + in1[0].z*in2[2].x;
	out[0].y = in1[0].x*in2[0].y + in1[0].y*in2[1].y + in1[0].z*in2[2].y;
	out[0].z = in1[0].x*in2[0].z + in1[0].y*in2[1].z + in1[0].z*in2[2].z;
	out[1].x = in1[1].x*in2[0].x + in1[1].y*in2[1].x + in1[1].z*in2[2].x;
	out[1].y = in1[1].x*in2[0].y + in1[1].y*in2[1].y + in1[1].z*in2[2].y;
	out[1].z = in1[1].x*in2[0].z + in1[1].y*in2[1].z + in1[1].z*in2[2].z;
	out[2].x = in1[2].x*in2[0].x + in1[2].y*in2[1].x + in1[2].z*in2[2].x;
	out[2].y = in1[2].x*in2[0].y + in1[2].y*in2[1].y + in1[2].z*in2[2].y;
	out[2].z = in1[2].x*in2[0].z + in1[2].y*in2[1].z + in1[2].z*in2[2].z;
}

void MatrixTranspose( vector3 matrix[3], vector3 transpose[3] ) {
	int i, j;
	for ( i=0; i<3; i++ ) {
		for ( j=0; j<3; j++ ) {
			transpose[i].data[j] = matrix[j].data[i];
		}
	}
}

void AngleVectors( const vector3 *angles, vector3 *forward, vector3 *right, vector3 *up) {
	float		angle;
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles->yaw * (M_PI*2 / 360);
	sy = sinf(angle);
	cy = cosf(angle);
	angle = angles->pitch * (M_PI*2 / 360);
	sp = sinf(angle);
	cp = cosf(angle);
	angle = angles->roll * (M_PI*2 / 360);
	sr = sinf(angle);
	cr = cosf(angle);

	if (forward)
	{
		forward->x = cp*cy;
		forward->y = cp*sy;
		forward->z = -sp;
	}
	if (right)
	{
		right->x = (-1*sr*sp*cy+-1*cr*-sy);
		right->y = (-1*sr*sp*sy+-1*cr*cy);
		right->z = -1*sr*cp;
	}
	if (up)
	{
		up->x = (cr*sp*cy+-sr*-sy);
		up->y = (cr*sp*sy+-sr*cy);
		up->z = cr*cp;
	}
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vector3 *dst, const vector3 *src )
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vector3 tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabsf( src->data[i] ) < minelem )
		{
			pos = i;
			minelem = fabsf( src->data[i] );
		}
	}
	tempvec.x = tempvec.y = tempvec.z = 0.0f;
	tempvec.data[pos] = 1.0f;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, &tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

/*
================
Q_isnan

Don't pass doubles to this
================
*/
int Q_isnan( float x )
{
	floatint_t fi;

	fi.f = x;
	fi.ui &= 0x7FFFFFFF;
	fi.ui = 0x7F800000 - fi.ui;

	return (int)( (unsigned int)fi.ui >> 31 );
}
//------------------------------------------------------------------------

/*
=====================
Q_acos

the msvc acos doesn't always return a value between -PI and PI:

int i;
i = 1065353246;
acos(*(float*) &i) == -1.#IND0

=====================
*/
float Q_acos(float c) {
	float angle;

	angle = acosf(c);

	if (angle > M_PI) {
		return (float)M_PI;
	}
	if (angle < -M_PI) {
		return (float)M_PI;
	}
	return angle;
}

//Raz: Added from Jedi Academy

// This is the VC libc version of rand() without multiple seeds per thread or 12 levels
// of subroutine calls.
// Both calls have been designed to minimise the inherent number of float <--> int 
// conversions and the additional math required to get the desired value.
// eg the typical tint = (rand() * 255) / 32768
// becomes tint = irand(0, 255)

static unsigned long	holdrand = 0x89abcdef;

void Rand_Init(int seed)
{
	holdrand = seed;
}

// Returns a float min <= x < max (exclusive; will get max - 0.00001; but never max)

float flrand(float min, float max)
{
	float	result;

	holdrand = (holdrand * 214013L) + 2531011L;
	result = (float)(holdrand >> 17);						// 0 - 32767 range
	result = ((result * (max - min)) / 32768.0F) + min;

	return(result);
}
float Q_flrand(float min, float max)
{
	return flrand(min,max);
}

// Returns an integer min <= x <= max (ie inclusive)

int irand(int min, int max)
{
	int		result;

	assert((max - min) < 32768);

	max++;
	holdrand = (holdrand * 214013L) + 2531011L;
	result = holdrand >> 17;
	result = ((result * (max - min)) >> 15) + min;
	return(result);
}

int Q_irand(int value1, int value2)
{
	return irand(value1, value2);
}

float Q_powf ( float x, int y )
{
	float r = x;
	for ( y--; y>0; y-- )
		r = r * r;
	return r;
}

void HSL2RGB( float h, float s, float l, float *r, float *g, float *b )
{
	double tr, tg, tb;
	double v;

	tr = l;
	tg = l;
	tb = l;
	v = (l <= 0.5f) ? (l * (1.0f + (l*2))) : (l + l - l * l);
    if (v > 0) {
		double m;
		double sv;
		int sextant;
		double fract, vsf, mid1, mid2;

		m = l + l - v;
		sv = (v - m ) / v;
		h *= 5.0f;
		sextant = (int)floorf(h);
		fract = h - sextant;
		vsf = v * sv * fract;
		mid1 = m + vsf;
		mid2 = v - vsf;
		switch (sextant)
		{
			case 0:
			default:
				tr = v;
				tg = mid1;
				tb = m;
				break;
			case 1:
				tr = mid2;
				tg = v;
				tb = m;
				break;
			case 2:
				tr = m;
				tg = v;
				tb = mid1;
				break;
			case 3:
				tr = m;
				tg = mid2;
				tb = v;
				break;
			case 4:
				tr = mid1;
				tg = m;
				tb = v;
				break;
			case 5:
				tr = v;
				tg = m;
				tb = mid2;
				break;
          }
    }
    *r = (float)tr;
    *g = (float)tg;
	*b = (float)tb;
}

void HSV2RGB( float h, float s, float v, float *r, float *g, float *b )
{
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
		*r = v;
		*g = t;
		*b = p;
		break;
	case 1:
		*r = q;
		*g = v;
		*b = p;
		break;
	case 2:
		*r = p;
		*g = v;
		*b = t;
		break;
	case 3:
		*r = p;
		*g = q;
		*b = v;
		break;
	case 4:
		*r = t;
		*g = p;
		*b = v;
		break;
	case 5:
		*r = v;
		*g = p;
		*b = q;
		break;
	}
}
