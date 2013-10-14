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
 * name:		l_libvar.c
 *
 * desc:		bot library variables
 *
 * $Archive: /MissionPack/code/botlib/l_libvar.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_memory.h"
#include "l_libvar.h"

//list with library variables
libvar_t *libvarlist = NULL;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float LibVarStringValue(char *string) {
	int dotfound = 0;
	float value = 0;

	while(*string)
	{
		if (*string < '0' || *string > '9')
		{
			if (dotfound || *string != '.')
			{
				return 0;
			}
			else
			{
				dotfound = 10;
				string++;
			}
		}
		if (dotfound)
		{
			value = value + (float) (*string - '0') / (float) dotfound;
			dotfound *= 10;
		}
		else
		{
			value = value * 10.0f + (float) (*string - '0');
		}
		string++;
	}
	return value;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
libvar_t *LibVarAlloc(char *name) {
	libvar_t *v;

	v = (libvar_t *) GetMemory(sizeof(libvar_t));
	memset(v, 0, sizeof(libvar_t));
	v->name = (char *) GetMemory(strlen(name)+1);
	strcpy(v->name, name);
	//add the variable in the list
	v->next = libvarlist;
	libvarlist = v;
	return v;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void LibVarDeAlloc(libvar_t *v) {
	if (v->string) FreeMemory(v->string);
	FreeMemory(v->name);
	FreeMemory(v);
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void LibVarDeAllocAll( void ) {
	libvar_t *v;

	for (v = libvarlist; v; v = libvarlist)
	{
		libvarlist = libvarlist->next;
		LibVarDeAlloc(v);
	}
	libvarlist = NULL;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
libvar_t *LibVarGet(char *name) {
	libvar_t *v;

	for (v = libvarlist; v; v = v->next)
	{
		if (!Q_stricmp(v->name, name))
		{
			return v;
		}
	}
	return NULL;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
char *LibVarGetString(char *name) {
	libvar_t *v;

	v = LibVarGet(name);
	if (v)
	{
		return v->string;
	}
	else
	{
		return "";
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float LibVarGetValue(char *name) {
	libvar_t *v;

	v = LibVarGet(name);
	if (v)
	{
		return v->value;
	}
	else
	{
		return 0;
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
libvar_t *LibVar(char *name, char *value) {
	libvar_t *v;
	v = LibVarGet(name);
	if (v) return v;
	//create new variable
	v = LibVarAlloc(name);
	//variable string
	v->string = (char *) GetMemory(strlen(value) + 1);
	strcpy(v->string, value);
	//the value
	v->value = LibVarStringValue(v->string);
	//variable is modified
	v->modified = qtrue;
	//
	return v;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
char *LibVarString(char *name, char *value) {
	libvar_t *v;

	v = LibVar(name, value);
	return v->string;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float LibVarValue(char *name, char *value) {
	libvar_t *v;

	v = LibVar(name, value);
	return v->value;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void LibVarSet(char *name, char *value) {
	libvar_t *v;

	v = LibVarGet(name);
	if (v)
	{
		FreeMemory(v->string);
	}
	else
	{
		v = LibVarAlloc(name);
	}
	//variable string
	v->string = (char *) GetMemory(strlen(value) + 1);
	strcpy(v->string, value);
	//the value
	v->value = LibVarStringValue(v->string);
	//variable is modified
	v->modified = qtrue;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean LibVarChanged(char *name) {
	libvar_t *v;

	v = LibVarGet(name);
	if (v)
	{
		return v->modified;
	}
	else
	{
		return qfalse;
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void LibVarSetNotModified(char *name) {
	libvar_t *v;

	v = LibVarGet(name);
	if (v)
	{
		v->modified = qfalse;
	}
}
