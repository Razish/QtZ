// sv_subs.c - common function replacements for modular server

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sv_local.h"

// qcommon/q_shared.c
void QDECL Com_Printf( const char *msg, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	svi.Print( "%s", text );
}

// ???
void QDECL Com_DPrintf( const char *msg, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

//	svi.Print( "%s", text );
}

// qcommon/q_shared.c
void QDECL Com_Error( int level, const char *error, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, error );
	Q_vsnprintf( text, sizeof( text ), error, argptr );
	va_end( argptr );

	svi.Error( level, "%s", text );
}

// sys/sys_main.c
void QDECL NET_Init( void ) {
	svi.NET_Init();
}

int QDECL Sys_Milliseconds( void ) {
	return svi.Sys_Milliseconds();
}

void *VM_ArgPtr( intptr_t intValue ) {
	return svi.VM_ArgPtr( intValue );
}

char *CopyString( const char *in ) {
	return svi.CopyString( in );
}
