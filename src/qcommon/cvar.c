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
// cvar.c -- dynamic variable tracking

#include "q_shared.h"
#include "qcommon.h"

cvar_t		*cvar_vars = NULL;
cvar_t		*cvar_cheats;
int			cvar_modifiedFlags;

#define	MAX_CVARS	2048 //1024
cvar_t		cvar_indexes[MAX_CVARS];
int			cvar_numIndexes;

#define FILE_HASH_SIZE		256
static	cvar_t	*hashTable[FILE_HASH_SIZE];

static long generateHashValue( const char *fname ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = (char)tolower(fname[i]);
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
	return hash;
}

static qboolean Cvar_ValidateString( const char *s ) {
	if ( !s ) {
		return qfalse;
	}
	if ( strchr( s, '\\' ) ) {
		return qfalse;
	}
	if ( strchr( s, '\"' ) ) {
		return qfalse;
	}
	if ( strchr( s, ';' ) ) {
		return qfalse;
	}
	return qtrue;
}

static cvar_t *Cvar_FindVar( const char *name ) {
	cvar_t	*var;
	long hash;

	hash = generateHashValue(name);
	
	for (var=hashTable[hash] ; var ; var=var->hashNext) {
		if (!Q_stricmp(name, var->name)) {
			return var;
		}
	}

	return NULL;
}

float Cvar_VariableValue( const char *name ) {
	cvar_t	*var;
	
	var = Cvar_FindVar (name);
	if (!var)
		return 0;
	return var->value;
}

int Cvar_VariableIntegerValue( const char *name ) {
	cvar_t	*var;
	
	var = Cvar_FindVar (name);
	if (!var)
		return 0;
	return var->integer;
}

char *Cvar_VariableString( const char *name ) {
	cvar_t *var;
	
	var = Cvar_FindVar (name);
	if (!var)
		return "";
	return var->string;
}

void Cvar_VariableStringBuffer( const char *name, char *buffer, size_t bufsize ) {
	cvar_t *var;
	
	var = Cvar_FindVar (name);
	if (!var) {
		*buffer = 0;
	}
	else {
		Q_strncpyz( buffer, var->string, bufsize );
	}
}

int Cvar_Flags(const char *name) {
	cvar_t *var;
	
	if(!(var = Cvar_FindVar(name)))
		return CVAR_NONEXISTENT;
	else
	{
		if(var->modified)
			return var->flags | CVAR_MODIFIED;
		else
			return var->flags;
	}
}

void Cvar_CommandCompletion(void (*callback)(const char *s)) {
	cvar_t		*cvar;
	
	for(cvar = cvar_vars; cvar; cvar = cvar->next)
	{
		if(cvar->name)
			callback(cvar->name);
	}
}

static const char *Cvar_Validate( cvar_t *var,
    const char *value, qboolean warn )
{
	static char s[ MAX_CVAR_VALUE_STRING ];
	float valuef;
	qboolean changed = qfalse;

	if( !var->validate )
		return value;

	if( !value )
		return value;

	if( Q_isanumber( value ) )
	{
		valuef = (float)atof( value );

		if( var->integral )
		{
			if( !Q_isintegral( valuef ) )
			{
				if( warn )
					Com_Printf( "WARNING: cvar '%s' must be integral", var->name );

				valuef = valuef;
				changed = qtrue;
			}
		}
	}
	else
	{
		if( warn )
			Com_Printf( "WARNING: cvar '%s' must be numeric", var->name );

		valuef = (float)atof( var->resetString );
		changed = qtrue;
	}

	if( valuef < var->min )
	{
		if( warn )
		{
			if( changed )
				Com_Printf( " and is" );
			else
				Com_Printf( "WARNING: cvar '%s'", var->name );

			if( Q_isintegral( var->min ) )
				Com_Printf( " out of range (min %d)", (int)var->min );
			else
				Com_Printf( " out of range (min %f)", var->min );
		}

		valuef = var->min;
		changed = qtrue;
	}
	else if( valuef > var->max )
	{
		if( warn )
		{
			if( changed )
				Com_Printf( " and is" );
			else
				Com_Printf( "WARNING: cvar '%s'", var->name );

			if( Q_isintegral( var->max ) )
				Com_Printf( " out of range (max %d)", (int)var->max );
			else
				Com_Printf( " out of range (max %f)", var->max );
		}

		valuef = var->max;
		changed = qtrue;
	}

	if( changed )
	{
		if( Q_isintegral( valuef ) )
		{
			Com_sprintf( s, sizeof( s ), "%d", (int)valuef );

			if( warn )
				Com_Printf( ", setting to %d\n", (int)valuef );
		}
		else
		{
			Com_sprintf( s, sizeof( s ), "%f", valuef );

			if( warn )
				Com_Printf( ", setting to %f\n", valuef );
		}

		return s;
	}
	else
		return value;
}

// If the variable already exists, the value will not be set unless CVAR_ROM
//	The flags will be or'ed in if the variable exists.
//	You can pass NULL for the description.
cvar_t *Cvar_Get( const char *name, const char *value, int flags, const char *description, void (*update)( void ) ) {
	cvar_t *var = NULL;
	long hash = 0;
	int index = 0;

	if ( !name || !value )
		Com_Error( ERR_FATAL, "Cvar_Get: NULL parameter" );

	if ( !Cvar_ValidateString( name ) ) {
		Com_Printf( "invalid cvar name string: %s\n", name );
		name = "BADNAME";
	}

	if ( (var=Cvar_FindVar( name )) ) {
		value = Cvar_Validate( var, value, qfalse );

		// if the C code is now specifying a variable that the user already set a value for, take the new value as the reset value
		if ( var->flags & CVAR_USER_CREATED ) {
			var->flags &= ~CVAR_USER_CREATED;
			Z_Free( var->resetString );
			var->resetString = CopyString( value );

			if ( description ) {
				if ( var->description )
					Z_Free( var->description );
				var->description = CopyString( description );
			}

			if ( update ) {
				var->update = update;
			}

			if ( flags & CVAR_ROM ) {
				// this variable was set by the user, so force it to value given by the engine.

				if ( var->latchedString )
					Z_Free( var->latchedString );
				
				var->latchedString = CopyString( value );
			}
		}

		// Make sure servers cannot mark engine-added variables as SERVER_CREATED
		if ( var->flags & CVAR_SERVER_CREATED ) {
			if ( !(flags & CVAR_SERVER_CREATED) )
				var->flags &= ~CVAR_SERVER_CREATED;
		}
		else {
			if ( flags & CVAR_SERVER_CREATED )
				flags &= ~CVAR_SERVER_CREATED;
		}
		
		var->flags |= flags;

		// only allow one non-empty reset string without a warning
		if ( !var->resetString[0] ) {
			// we don't have a reset string yet
			Z_Free( var->resetString );
			var->resetString = CopyString( value );
		}
		else if ( value[0] && strcmp( var->resetString, value ) )
			Com_DPrintf( "Warning: cvar \"%s\" given initial values: \"%s\" and \"%s\"\n", name, var->resetString, value );

		// if we have a latched string, take that value now
		if ( var->latchedString ) {
			char *s = var->latchedString;
			var->latchedString = NULL;	// otherwise cvar_set2 would free it
			Cvar_Set2( name, s, qtrue );
			Z_Free( s );
		}

		// ZOID--needs to be set so that cvars the game sets as SERVERINFO get sent to clients
		cvar_modifiedFlags |= flags;

		return var;
	}

	//
	// allocate a new cvar
	//

	// find a free cvar
	//QTZTODO: store head index
	for ( index=0; index<MAX_CVARS; index++ ) {
		if ( !cvar_indexes[index].name )
			break;
	}

	if ( index >= MAX_CVARS ) {
		if ( !com_errorEntered )
			Com_Error( ERR_FATAL, "Error: Too many cvars, cannot create a new one!" );

		return NULL;
	}
	
	var = &cvar_indexes[index];
	
	if ( index >= cvar_numIndexes )
		cvar_numIndexes = index + 1;
		
	var->name = CopyString( name );
	var->string = CopyString( value );
	//QTZTODO: tokens
	if ( description )
		var->description = CopyString( description );
	var->update = update;
	var->modified = qtrue;
	var->modificationCount = 1;
	var->value = (float)atof( var->string );
	var->integer = atoi( var->string );
	var->boolean = (qboolean)!!( var->integer );
	var->resetString = CopyString( value );
	var->validate = qfalse;

	// link the variable in
	var->next = cvar_vars;
	if ( cvar_vars )
		cvar_vars->prev = var;

	var->prev = NULL;
	cvar_vars = var;

	var->flags = flags;
	// note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
	cvar_modifiedFlags |= var->flags;

	hash = generateHashValue( name );
	var->hashIndex = hash;

	var->hashNext = hashTable[hash];
	if ( hashTable[hash] )
		hashTable[hash]->hashPrev = var;

	var->hashPrev = NULL;
	hashTable[hash] = var;

	return var;
}

// Prints the value, default, latched string and description of the given variable
void Cvar_Print( cvar_t *v ) {
	Com_Printf( S_COLOR_GREY"Cvar "S_COLOR_WHITE"%s = "S_COLOR_GREY"\""S_COLOR_WHITE"%s"S_COLOR_GREY"\""S_COLOR_WHITE, v->name, v->string );

	if ( !(v->flags & CVAR_ROM) ) {
		if ( Q_stricmp( v->string, v->resetString ) )
			Com_Printf( ", "S_COLOR_WHITE"default = "S_COLOR_GREY"\""S_COLOR_WHITE"%s"S_COLOR_GREY"\""S_COLOR_WHITE, v->resetString );
	}

	Com_Printf( "\n" );

	if ( v->latchedString )
		Com_Printf( "     latched = "S_COLOR_GREY"\""S_COLOR_WHITE"%s"S_COLOR_GREY"\"\n", v->latchedString );

	if ( v->description )
		Com_Printf( "     "S_COLOR_GREY"%s\n", v->description );
}

cvar_t *Cvar_Set2( const char *name, const char *value, qboolean force ) {
	cvar_t	*var;

//	Com_DPrintf( "Cvar_Set2: %s %s\n", name, value );

	if ( !Cvar_ValidateString( name ) ) {
		Com_Printf("invalid cvar name string: %s\n", name );
		name = "BADNAME";
	}

#if 0	// FIXME
	if ( value && !Cvar_ValidateString( value ) ) {
		Com_Printf("invalid cvar value string: %s\n", value );
		var_value = "BADVALUE";
	}
#endif

	var = Cvar_FindVar (name);
	if (!var) {
		if ( !value ) {
			return NULL;
		}
		// create it
		if ( !force ) {
			return Cvar_Get( name, value, CVAR_USER_CREATED, NULL, NULL );
		} else {
			return Cvar_Get (name, value, CVAR_NONE, NULL, NULL );
		}
	}

	if (!value ) {
		value = var->resetString;
	}

	value = Cvar_Validate(var, value, qtrue);

	if((var->flags & CVAR_LATCH) && var->latchedString)
	{
		if(!strcmp(value, var->string))
		{
			Z_Free(var->latchedString);
			var->latchedString = NULL;
			return var;
		}

		if(!strcmp(value, var->latchedString))
			return var;
	}
	else if(!strcmp(value, var->string))
		return var;

	// note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
	cvar_modifiedFlags |= var->flags;

	if (!force)
	{
		if (var->flags & CVAR_ROM)
		{
			Com_Printf ("%s is read only.\n", name);
			return var;
		}

		if (var->flags & CVAR_INIT)
		{
			Com_Printf ("%s is write protected.\n", name);
			return var;
		}

		if (var->flags & CVAR_LATCH)
		{
			if (var->latchedString)
			{
				if (strcmp(value, var->latchedString) == 0)
					return var;
				Z_Free (var->latchedString);
			}
			else
			{
				if (strcmp(value, var->string) == 0)
					return var;
			}

			Com_Printf ("%s will be changed upon restarting.\n", name);
			var->latchedString = CopyString(value);
			var->modified = qtrue;
			var->modificationCount++;
			return var;
		}

		if ( (var->flags & CVAR_CHEAT) && !cvar_cheats->integer )
		{
			Com_Printf ("%s is cheat protected.\n", name);
			return var;
		}

	}
	else
	{
		if (var->latchedString)
		{
			Z_Free (var->latchedString);
			var->latchedString = NULL;
		}
	}

	if (!strcmp(value, var->string))
		return var;		// not changed

	var->modified = qtrue;
	var->modificationCount++;
	
	Z_Free (var->string);	// free the old value string
	
	var->string = CopyString(value);
	//QTZTODO: tokens
	var->value = (float)atof (var->string);
	var->integer = atoi (var->string);
	var->boolean = (qboolean)!!(var->integer);
	if ( var->update )
		var->update();

	return var;
}

void Cvar_Set( const char *name, const char *value) {
	Cvar_Set2 (name, value, qtrue);
}

void Cvar_SetSafe( const char *name, const char *value ) {
	int flags = Cvar_Flags( name );

	if((flags != CVAR_NONEXISTENT) && (flags & CVAR_PROTECTED))
	{
		if( value )
			Com_Error( ERR_DROP, "Restricted source tried to set "
				"\"%s\" to \"%s\"", name, value );
		else
			Com_Error( ERR_DROP, "Restricted source tried to "
				"modify \"%s\"", name );
		return;
	}
	Cvar_Set( name, value );
}

void Cvar_SetLatched( const char *name, const char *value) {
	Cvar_Set2 (name, value, qfalse);
}

void Cvar_SetValue( const char *name, float value) {
	char	val[32];

	if ( value == (int)value ) {
		Com_sprintf (val, sizeof(val), "%i",(int)value);
	} else {
		Com_sprintf (val, sizeof(val), "%f",value);
	}
	Cvar_Set (name, val);
}

void Cvar_SetValueSafe( const char *name, float value ) {
	char val[32];

	if( Q_isintegral( value ) )
		Com_sprintf( val, sizeof(val), "%i", (int)value );
	else
		Com_sprintf( val, sizeof(val), "%f", value );
	Cvar_SetSafe( name, val );
}

void Cvar_Reset( const char *name ) {
	Cvar_Set2( name, NULL, qfalse );
}

void Cvar_ForceReset(const char *name) {
	Cvar_Set2(name, NULL, qtrue);
}

// Any testing variables will be reset to the safe values
void Cvar_SetCheatState( void ) {
	cvar_t	*var;

	// set all default vars to the safe value
	for(var = cvar_vars; var ; var = var->next)
	{
		if(var->flags & CVAR_CHEAT)
		{
			// the CVAR_LATCHED|CVAR_CHEAT vars might escape the reset here 
			// because of a different var->latchedString
			if (var->latchedString)
			{
				Z_Free(var->latchedString);
				var->latchedString = NULL;
			}
			if (strcmp(var->resetString,var->string))
				Cvar_Set(var->name, var->resetString);
		}
	}
}

// Handles variable inspection and changing from the console
qboolean Cvar_Command( void ) {
	cvar_t	*v;
	const char *args = Cmd_Args();

	// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v) {
		return qfalse;
	}

	// perform a variable print or set
	if ( Cmd_Argc() == 1 ) {
		Cvar_Print( v );
		return qtrue;
	}

	// set the value if forcing isn't required
	if ( args[0] == '!' )
		Cvar_Set2( v->name, va( "%i", !(v->integer) ), qfalse );
	else
		Cvar_Set2 (v->name, args, qfalse);
	return qtrue;
}

// Prints the contents of a cvar  (preferred over Cvar_Command where cvar names and commands conflict)
void Cvar_Print_f( void ) {
	const char *name;
	cvar_t *cv;
	
	if(Cmd_Argc() != 2)
	{
		Com_Printf ("usage: print <variable>\n");
		return;
	}

	name = Cmd_Argv(1);

	cv = Cvar_FindVar(name);
	
	if(cv)
		Cvar_Print(cv);
	else
		Com_Printf ("Cvar %s does not exist.\n", name);
}

// Toggles a cvar for easy single key binding, optionally through a list of given values
void Cvar_Toggle_f( void ) {
	int		i, c = Cmd_Argc();
	char		*curval;

	if(c < 2) {
		Com_Printf("usage: toggle <variable> [value1, value2, ...]\n");
		return;
	}

	if(c == 2) {
		Cvar_Set2(Cmd_Argv(1), va("%d", 
			!Cvar_VariableValue(Cmd_Argv(1))), 
			qfalse);
		return;
	}

	if(c == 3) {
		Com_Printf("toggle: nothing to toggle to\n");
		return;
	}

	curval = Cvar_VariableString(Cmd_Argv(1));

	// don't bother checking the last arg for a match since the desired
	// behaviour is the same as no match (set to the first argument)
	for(i = 2; i + 1 < c; i++) {
		if(strcmp(curval, Cmd_Argv(i)) == 0) {
			Cvar_Set2(Cmd_Argv(1), Cmd_Argv(i + 1), qfalse);
			return;
		}
	}

	// fallback
	Cvar_Set2(Cmd_Argv(1), Cmd_Argv(2), qfalse);
}

// Allows setting and defining of arbitrary cvars from console, even if they weren't declared in C code.
void Cvar_Set_f( void ) {
	int		c;
	const char	*cmd;
	cvar_t	*v;

	c = Cmd_Argc();
	cmd = Cmd_Argv(0);

	if ( c < 2 ) {
		Com_Printf ("usage: %s <variable> <value>\n", cmd);
		return;
	}
	if ( c == 2 ) {
		Cvar_Print_f();
		return;
	}

	v = Cvar_Set2 (Cmd_Argv(1), Cmd_ArgsFrom(2), qfalse);
	if( !v ) {
		return;
	}
	switch( cmd[3] ) {
		case 'a':
			if( !( v->flags & CVAR_ARCHIVE ) ) {
				v->flags |= CVAR_ARCHIVE;
				cvar_modifiedFlags |= CVAR_ARCHIVE;
			}
			break;
		case 'u':
			if( !( v->flags & CVAR_USERINFO ) ) {
				v->flags |= CVAR_USERINFO;
				cvar_modifiedFlags |= CVAR_USERINFO;
			}
			break;
		case 's':
			if( !( v->flags & CVAR_SERVERINFO ) ) {
				v->flags |= CVAR_SERVERINFO;
				cvar_modifiedFlags |= CVAR_SERVERINFO;
			}
			break;
	}
}

void Cvar_Reset_f( void ) {
	if ( Cmd_Argc() != 2 ) {
		Com_Printf ("usage: reset <variable>\n");
		return;
	}
	Cvar_Reset( Cmd_Argv( 1 ) );
}

//#define DEBUG_CVAR_SORT

typedef struct cvarSerialise_s {
	char *name, *value;
} cvarSerialise_t;

static int cvarSort( const void *cvar1, const void *cvar2 ) {
	return strcmp( ((cvarSerialise_t *)cvar1)->name, ((cvarSerialise_t *)cvar2)->name );
}

// Appends lines containing "seta variable value" for all variables with the archive flag set to qtrue.
void Cvar_WriteVariables( fileHandle_t f ) {
	cvar_t *var;
	char buffer[MAX_STRING_CHARS] = {0};
	int i=0, writeCount=0, totalCount=0, size=0;
	cvarSerialise_t *list = NULL, *listPtr = NULL;

	for ( var=cvar_vars; var; var=var->next ) {
		char *value = var->latchedString ? var->latchedString : var->string;
		if ( var->name && (var->flags & CVAR_ARCHIVE) && (Q_stricmp( var->resetString, value ) || (var->flags&CVAR_USER_CREATED)) ) {
			if ( strlen( var->name ) + strlen( var->latchedString ? var->latchedString : var->string ) + 9 >= sizeof( buffer ) )
				continue;
			writeCount++;
		}
		totalCount++;
	}

	// allocate the working space for sorting
	size = sizeof( cvarSerialise_t ) * writeCount;
	list = (cvarSerialise_t *)Z_Malloc( size );
	memset( list, 0, size );

	for ( var=cvar_vars, listPtr=list; var; var=var->next ) {
		char *value = var->latchedString ? var->latchedString : var->string;
		if ( var->name && (var->flags & CVAR_ARCHIVE) && (Q_stricmp( var->resetString, value ) || (var->flags&CVAR_USER_CREATED)) ) {
			if ( strlen( var->name ) + strlen( value ) + 9 >= sizeof( buffer ) ) {
				Com_Printf( S_COLOR_YELLOW "WARNING: value of variable \"%s\" too long to write to file\n", var->name );
				continue;
			}
			listPtr->name = var->name;
			listPtr->value = value;
			listPtr++;
		}
	}

	qsort( list, writeCount, sizeof( cvarSerialise_t ), cvarSort );

	for ( i=0, listPtr=list; i<writeCount; i++, listPtr++ ) {
		Com_sprintf( buffer, sizeof( buffer ), "seta %s \"%s\"\n", listPtr->name, listPtr->value );
		FS_Write( buffer, (int)strlen( buffer ), f );
	}

	Com_DPrintf( "Wrote %i/%i cvars to file\n", writeCount, totalCount );

	Z_Free( list );
	list = listPtr = NULL;
}

void Cvar_List_f( void ) {
	cvar_t *var;
	int i;
	const char *match = NULL;

	if ( Cmd_Argc() > 1 )
		match = Cmd_Argv( 1 );

	for ( var=cvar_vars, i=0; var;
		var=var->next,
		i++ )
	{
		if ( !var->name || (match && !Com_Filter( match, var->name, qfalse )) )
			continue;

		if (var->flags & CVAR_SERVERINFO)	Com_Printf( "S" );	else Com_Printf( " " );
		if (var->flags & CVAR_SYSTEMINFO)	Com_Printf( "s" );	else Com_Printf( " " );
		if (var->flags & CVAR_USERINFO)		Com_Printf( "U" );	else Com_Printf( " " );
		if (var->flags & CVAR_ROM)			Com_Printf( "R" );	else Com_Printf( " " );
		if (var->flags & CVAR_INIT)			Com_Printf( "I" );	else Com_Printf( " " );
		if (var->flags & CVAR_ARCHIVE)		Com_Printf( "A" );	else Com_Printf( " " );
		if (var->flags & CVAR_LATCH)		Com_Printf( "L" );	else Com_Printf( " " );
		if (var->flags & CVAR_CHEAT)		Com_Printf( "C" );	else Com_Printf( " " );
		if (var->flags & CVAR_USER_CREATED)	Com_Printf( "?" );	else Com_Printf( " " );

		Com_Printf( S_COLOR_WHITE" %s = "S_COLOR_GREY"\""S_COLOR_WHITE"%s"S_COLOR_GREY"\""S_COLOR_WHITE, var->name, var->string );
		if ( var->latchedString )
			Com_Printf( ", latched = "S_COLOR_GREY"\""S_COLOR_WHITE"%s"S_COLOR_GREY"\""S_COLOR_WHITE, var->latchedString );
		Com_Printf( "\n" );
	}

	Com_Printf( "\n%i total cvars\n", i );
	if ( i != cvar_numIndexes )
		Com_Printf( "%i cvar indexes\n", cvar_numIndexes );
}

void Cvar_ListChanged_f( void ) {
	cvar_t *var;
	int i;

	for ( var=cvar_vars, i=0; var; var=var->next, i++ ) {
		char *value = var->latchedString ? var->latchedString : var->string;
		if ( !var->name || !var->modificationCount || !strcmp( value, var->resetString ) )
			continue;

		Com_Printf( S_COLOR_GREY"Cvar "
			S_COLOR_WHITE"%s = "S_COLOR_GREY"\""S_COLOR_WHITE"%s"S_COLOR_GREY"\""S_COLOR_WHITE", "
			S_COLOR_WHITE"default = "S_COLOR_GREY"\""S_COLOR_WHITE"%s"S_COLOR_GREY"\""S_COLOR_WHITE"\n",
			var->name, value, var->resetString );
	}
}

// Unsets a cvar
cvar_t *Cvar_Unset(cvar_t *cv) {
	cvar_t *next = cv->next;

	if ( cv->name )				Z_Free( cv->name );
	if ( cv->string )			Z_Free( cv->string );
	if ( cv->latchedString )	Z_Free( cv->latchedString );
	if ( cv->resetString )		Z_Free( cv->resetString );
	if ( cv->description )		Z_Free( cv->description );

	if(cv->prev)
		cv->prev->next = cv->next;
	else
		cvar_vars = cv->next;
	if(cv->next)
		cv->next->prev = cv->prev;

	if(cv->hashPrev)
		cv->hashPrev->hashNext = cv->hashNext;
	else
		hashTable[cv->hashIndex] = cv->hashNext;
	if(cv->hashNext)
		cv->hashNext->hashPrev = cv->hashPrev;

	memset(cv, '\0', sizeof(*cv));
	
	return next;
}

// Unsets a userdefined cvar
void Cvar_Unset_f( void ) {
	cvar_t *cv;
	
	if(Cmd_Argc() != 2)
	{
		Com_Printf("Usage: %s <varname>\n", Cmd_Argv(0));
		return;
	}
	
	cv = Cvar_FindVar(Cmd_Argv(1));

	if(!cv)
		return;
	
	if(cv->flags & CVAR_USER_CREATED)
		Cvar_Unset(cv);
	else
		Com_Printf("Error: %s: Variable %s is not user created.\n", Cmd_Argv(0), cv->name);
}



// Resets all cvars to their hardcoded values and removes userdefined variables and variables added via the VMs if requested.
void Cvar_Restart( void ) {
	cvar_t *curvar = cvar_vars;

	while ( curvar ) {
		if ( (curvar->flags & CVAR_USER_CREATED) ) {
			// throw out any variables the user created
			curvar = Cvar_Unset( curvar );
			continue;
		}
		
		if ( !(curvar->flags & (CVAR_ROM|CVAR_INIT|CVAR_NORESTART)) ) {
			// Just reset the rest to their default values.
			Cvar_Set2( curvar->name, curvar->resetString, qfalse );
		}
		
		curvar = curvar->next;
	}
}

char *Cvar_InfoString( int bit ) {
	static char	info[MAX_INFO_STRING];
	cvar_t *var;

	info[0] = '\0';

	for ( var=cvar_vars; var; var = var->next ) {
		if ( var->name && (var->flags & bit) )
			Info_SetValueForKey( info, var->name, var->string );
	}

	return info;
}

// handles large info strings ( CS_SYSTEMINFO )
char *Cvar_InfoString_Big(int bit) {
	static char	info[BIG_INFO_STRING];
	cvar_t	*var;

	info[0] = 0;

	for (var = cvar_vars; var; var = var->next)
	{
		if(var->name && (var->flags & bit))
			Info_SetValueForKey_Big (info, var->name, var->string);
	}
	return info;
}

void Cvar_InfoStringBuffer( int bit, char* buff, size_t buffsize ) {
	Q_strncpyz(buff,Cvar_InfoString(bit),buffsize);
}

void Cvar_CheckRange( cvar_t *var, float min, float max, qboolean integral ) {
	var->validate = qtrue;
	var->min = min;
	var->max = max;
	var->integral = integral;

	// Force an initial range check
	Cvar_Set( var->name, var->string );
}

void Cvar_CompleteCvarName( char *args, int argNum ) {
	if( argNum == 2 ) {
		// Skip "<cmd> "
		char *p = Com_SkipTokens( args, 1, " " );

		if ( p > args )
			Field_CompleteCommand( p, qfalse, qtrue );
	}
}

// Reads in all archived cvars
void Cvar_Init( void ) {
	memset( cvar_indexes, 0, sizeof( cvar_indexes ) );
	memset( hashTable, 0, sizeof( hashTable ) );

	cvar_cheats = Cvar_Get( "sv_cheats", "1", CVAR_SYSTEMINFO, "Indicates whether cheats are enabled or not", NULL );

	Cmd_AddCommand( "print", Cvar_Print_f, NULL );
	Cmd_AddCommand( "toggle", Cvar_Toggle_f, Cvar_CompleteCvarName );
	Cmd_AddCommand( "set", Cvar_Set_f, Cvar_CompleteCvarName );
	Cmd_AddCommand( "sets", Cvar_Set_f, Cvar_CompleteCvarName );
	Cmd_AddCommand( "setu", Cvar_Set_f, Cvar_CompleteCvarName );
	Cmd_AddCommand( "seta", Cvar_Set_f, Cvar_CompleteCvarName );
	Cmd_AddCommand( "reset", Cvar_Reset_f, Cvar_CompleteCvarName );
	Cmd_AddCommand( "unset", Cvar_Unset_f, Cvar_CompleteCvarName );

	Cmd_AddCommand( "cvarlist", Cvar_List_f, NULL );
	Cmd_AddCommand( "cvar_modified", Cvar_ListChanged_f, NULL );
	Cmd_AddCommand( "cvar_restart", Cvar_Restart, NULL );
}
