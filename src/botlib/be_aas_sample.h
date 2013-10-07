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

/*****************************************************************************
 * name:		be_aas_sample.h
 *
 * desc:		AAS
 *
 * $Archive: /source/code/botlib/be_aas_sample.h $
 *
 *****************************************************************************/

#ifdef AASINTERN
void AAS_InitAASLinkHeap( void );
void AAS_InitAASLinkedEntities( void );
void AAS_FreeAASLinkHeap( void );
void AAS_FreeAASLinkedEntities( void );
aas_face_t *AAS_AreaGroundFace(int areanum, vector3 *point);
aas_face_t *AAS_TraceEndFace(aas_trace_t *trace);
aas_plane_t *AAS_PlaneFromNum(int planenum);
aas_link_t *AAS_AASLinkEntity(vector3 *absmins, vector3 *absmaxs, int entnum);
aas_link_t *AAS_LinkEntityClientBBox(vector3 *absmins, vector3 *absmaxs, int entnum, int presencetype);
qboolean AAS_PointInsideFace(int facenum, vector3 *point, float epsilon);
qboolean AAS_InsideFace(aas_face_t *face, vector3 *pnormal, vector3 *point, float epsilon);
void AAS_UnlinkFromAreas(aas_link_t *areas);
#endif //AASINTERN

//returns the mins and maxs of the bounding box for the given presence type
void AAS_PresenceTypeBoundingBox(int presencetype, vector3 *mins, vector3 *maxs);
//returns the cluster the area is in (negative portal number if the area is a portal)
int AAS_AreaCluster(int areanum);
//returns the presence type(s) of the area
int AAS_AreaPresenceType(int areanum);
//returns the presence type(s) at the given point
int AAS_PointPresenceType(vector3 *point);
//returns the result of the trace of a client bbox
aas_trace_t AAS_TraceClientBBox(vector3 *start, vector3 *end, int presencetype, int passent);
//stores the areas the trace went through and returns the number of passed areas
int AAS_TraceAreas(vector3 *start, vector3 *end, int *areas, vector3 *points, int maxareas);
//returns the areas the bounding box is in
int AAS_BBoxAreas(vector3 *absmins, vector3 *absmaxs, int *areas, int maxareas);
//return area information
int AAS_AreaInfo( int areanum, aas_areainfo_t *info );
//returns the area the point is in
int AAS_PointAreaNum(vector3 *point);
//
int AAS_PointReachabilityAreaIndex( vector3 *point );
//returns the plane the given face is in
void AAS_FacePlane(int facenum, vector3 *normal, float *dist);

