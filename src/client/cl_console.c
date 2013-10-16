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
// console.c

#include "client.h"


#define	DEFAULT_CONSOLE_WIDTH (128)
int g_console_field_width = DEFAULT_CONSOLE_WIDTH;


#define	NUM_CON_TIMES 4

#define		CON_TEXTSIZE	32768
static struct console_s {
	qboolean	initialized;

	short	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int 	linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	float	xadjust;		// for wide aspect screens

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0f to 1.0f lines of console to display

	int		vislines;		// in scanlines

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines
	vector4	color;
} con;

cvar_t		*con_conspeed;
cvar_t		*con_notifytime;

void Con_ToggleConsole_f( void ) {
	// Can't toggle the console when it's the only thing available
	if ( clc.state == CA_DISCONNECTED && Key_GetCatcher( ) == KEYCATCH_CONSOLE ) {
		return;
	}

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify();
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_CONSOLE );
}

void Con_Clear_f( void ) {
	int		i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}

	Con_Bottom();		// go to end
}

// Save the console contents out to a file
void Con_Dump_f( void ) {
	int		l, x, i;
	short	*line;
	fileHandle_t	f;
	char	buffer[1024];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Com_Printf ("Dumped console text to %s.\n", Cmd_Argv(1) );

	f = FS_FOpenFileWrite( Cmd_Argv( 1 ) );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for (x=0 ; x<con.linewidth ; x++)
			if ((line[x] & 0xff) != ' ')
				break;
		if (x != con.linewidth)
			break;
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for ( ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for(i=0; i<con.linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for (x=con.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		strcat( buffer, "\n" );
		FS_Write(buffer, (int)strlen(buffer), f);
	}

	FS_FCloseFile( f );
}

void Con_ClearNotify( void ) {
	int		i;
	
	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		con.times[i] = 0;
	}
}

// If the line width has changed, reformat the buffer.
void Con_CheckResize( void ) {
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	short	tbuf[CON_TEXTSIZE];

	width = ((int)cls.glconfig.vidWidth / SMALLCHAR_WIDTH) - 2;

	if (width == con.linewidth)
		return;

	if ( width < 1 && !cls.glconfig.vidWidth ) {// video hasn't been initialized yet
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		for(i=0; i<CON_TEXTSIZE; i++)
			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}
	else {
		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;
	
		if (con.linewidth < numchars)
			numchars = con.linewidth;

		memcpy (tbuf, con.text, CON_TEXTSIZE * sizeof(short));
		for(i=0; i<CON_TEXTSIZE; i++)
			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';


		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}

void Cmd_CompleteTxtName( char *args, int argNum ) {
	if ( argNum == 2 )
		Field_CompleteFilename( "", "txt", qfalse, qtrue );
}

void Con_Init( void ) {
	int		i;

	con_notifytime = Cvar_Get( "con_notifytime", "3", 0, "How long messages appear in the notify area", NULL );
	con_conspeed = Cvar_Get( "scr_conspeed", "3", 0, "Speed at which the console window opens and closes", NULL );

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;
	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &historyEditLines[i] );
		historyEditLines[i].widthInChars = g_console_field_width;
	}
	CL_LoadConsoleHistory( );

	Cmd_AddCommand( "toggleconsole", Con_ToggleConsole_f, NULL );
	Cmd_AddCommand( "clear", Con_Clear_f, NULL );
	Cmd_AddCommand( "condump", Con_Dump_f, Cmd_CompleteTxtName );
}

void Con_Shutdown( void ) {
	Cmd_RemoveCommand( "toggleconsole" );
	Cmd_RemoveCommand( "clear" );
	Cmd_RemoveCommand( "condump" );
}

void Con_Linefeed (qboolean skipnotify) {
	int		i;

	// mark time for transparent overlay
	if (con.current >= 0)
	{
    if (skipnotify)
		  con.times[con.current % NUM_CON_TIMES] = 0;
    else
		  con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}

	con.x = 0;
	if (con.display == con.current)
		con.display++;
	con.current++;
	for(i=0; i<con.linewidth; i++)
		con.text[(con.current%con.totallines)*con.linewidth+i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
}

// Handles cursor positioning, line wrapping, etc
//	All console printing must go through this in order to be logged to disk
//	If no console is visible, the text will appear at the top of the game window
void CL_ConsolePrint( char *txt ) {
	int		y, l;
	unsigned char	c;
	char color;
	qboolean skipnotify = qfalse;		// NERVE - SMF
	int prev;							// NERVE - SMF

	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( txt, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		txt += 12;
	}
	
	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}
	
	if (!con.initialized) {
		VectorSet4( &con.color, 1.0f, 0.788f, 0.054f, 0.8f );
		con.linewidth = -1;
		Con_CheckResize();
		con.initialized = qtrue;
	}

	color = ColorIndex(COLOR_WHITE);

	while ( (c = *((unsigned char *) txt)) != 0 ) {
		if ( Q_IsColorString( txt ) ) {
			color = ColorIndex( *(txt+1) );
			txt += 2;
			continue;
		}

		// count word length
		for (l=0 ; l< con.linewidth ; l++) {
			if ( txt[l] <= ' ') {
				break;
			}

		}

		// word wrap
		if (l != con.linewidth && (con.x + l >= con.linewidth) ) {
			Con_Linefeed(skipnotify);

		}

		txt++;

		switch (c)
		{
		case '\n':
			Con_Linefeed (skipnotify);
			break;
		case '\r':
			con.x = 0;
			break;
		default:	// display character and advance
			y = con.current % con.totallines;
			con.text[y*con.linewidth+con.x] = (color << 8) | c;
			con.x++;
			if(con.x >= con.linewidth)
				Con_Linefeed(skipnotify);
			break;
		}
	}


	// mark time for transparent overlay
	if (con.current >= 0) {
		// NERVE - SMF
		if ( skipnotify ) {
			prev = con.current % NUM_CON_TIMES - 1;
			if ( prev < 0 )
				prev = NUM_CON_TIMES - 1;
			con.times[prev] = 0;
		}
		else
		// -NERVE - SMF
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}
}


/*

	DRAWING

*/


// Draw the editline after a ] prompt
void Con_DrawInput( void ) {
	int		y;

	if ( clc.state != CA_DISCONNECTED && !(Key_GetCatcher( ) & KEYCATCH_CONSOLE ) ) {
		return;
	}

	y = con.vislines - ( SMALLCHAR_HEIGHT * 2 );

	re->SetColor( &con.color );

	SCR_DrawSmallChar( (int)con.xadjust + 1 * SMALLCHAR_WIDTH, y, CONSOLE_PROMPT_CHAR );

	Field_Draw( &g_consoleField, (int)con.xadjust + 2 * SMALLCHAR_WIDTH, y, (int)SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, qtrue, qtrue, &con.color );
}

// Draws the last few lines of output transparently over the game top
void Con_DrawNotify( void ) {
	int		x, v, i, time, currentColor;
	short	*text;

	currentColor = 7;
	re->SetColor( &g_color_table[currentColor] );

	v = 0;
	for (i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++)
	{
		if (i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = con.text + (i % con.totallines)*con.linewidth;

		if (cl.snap.ps.pm_type != PM_INTERMISSION && Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
			continue;
		}

		for (x = 0 ; x < con.linewidth ; x++) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}
			if ( ( (text[x]>>8)&Q_COLOR_BITS ) != currentColor ) {
				currentColor = (text[x]>>8)&Q_COLOR_BITS;
				re->SetColor( &g_color_table[currentColor] );
			}
			SCR_DrawSmallChar( cl_conXOffset->integer + (int)con.xadjust + (x+1)*SMALLCHAR_WIDTH, v, text[x] & 0xff );
		}

		v += SMALLCHAR_HEIGHT;
	}

	re->SetColor( NULL );

	if (Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME) )
		return;
}

static void Con_DrawScrollbar( float scrollbarLength, float scrollbarX, float scrollbarY, int rows ) {
	vector4 color;
	const float scrollbarWidth = 3;
	const float scrollHandleLength		= con.totallines ? scrollbarLength * MIN( 1.0f, (float)rows / (float)con.totallines ) : 0.0f;
	const float scrollbarLengthPerLine	= (scrollbarLength - scrollHandleLength) / (float)(con.totallines - rows);
	const int relativeScrollLineIndex	= con.current - con.totallines + MIN( rows, con.totallines );
	const float scrollHandlePosition	= scrollbarLengthPerLine * (con.display - relativeScrollLineIndex);

	// scrollbar
	VectorCopy4( QCOLOR(COLOR_GREY), &color );
	SCR_FillRect( scrollbarX, scrollbarY, scrollbarWidth, scrollbarLength, &color );

	// handle
	if ( scrollHandlePosition >= 0 && scrollHandleLength > 0 ) {
		VectorCopy4( QCOLOR(COLOR_WHITE), &color );
		SCR_FillRect( scrollbarX, scrollbarY + scrollHandlePosition, scrollbarWidth, scrollHandleLength, &color );
	}
	else if ( con.totallines ) {
		// this happens when line appending gets us over the top position in a roll-lock situation
		VectorCopy4( QCOLOR(COLOR_CYAN), &color );
		SCR_FillRect( scrollbarX, scrollbarY, scrollbarWidth, scrollHandleLength, &color );
	}
}

// Draws the console with the solid background
void Con_DrawSolidConsole( float frac ) {
	int				i, x, y;
	int				rows;
	short			*text;
	int				row;
	int				yoffset = (int)(cls.glconfig.vidHeight * frac), lines;
//	qhandle_t		conShader;
	int				currentColor;
	const vector4	*color = &g_color_table[ColorIndex(COLOR_ORANGE)], shadowColour = { 0.4f, 0.4f, 0.4f, 0.5f };
	char			version[] = CLIENT_WINDOW_TITLE;

	if (yoffset <= 0)
		return;

	if (yoffset > cls.glconfig.vidHeight )
		yoffset = cls.glconfig.vidHeight;

	lines = yoffset / SMALLCHAR_HEIGHT;

	// on wide screens, we will center the text
	con.xadjust = 0;
	SCR_AdjustFrom640( &con.xadjust, NULL, NULL, NULL );

	// draw the background
	y = (int)(frac * SCREEN_HEIGHT);
	if ( y < 1 )
		y = 0;
	else
		SCR_DrawPic( 0.0f, 0.0f, SCREEN_WIDTH, (float)y, cls.consoleShader );

	SCR_FillRect( 0.0f, (float)y, SCREEN_WIDTH, 3.0f, &shadowColour );
	SCR_FillRect( 0.0f, (float)y+1, SCREEN_WIDTH, 1.0f, color );


	// draw the version number
	i = (int)strlen( version );

	re->SetColor( &shadowColour );
	for ( x=0; x<i; x++ )
		SCR_DrawSmallChar2( (int)((cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH)+1), (int)(yoffset - SMALLCHAR_HEIGHT*1.3f)+1, version[x], 1.0f );
	re->SetColor( color );
	for ( x=0; x<i; x++ )
		SCR_DrawSmallChar2( (int)(cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH), (int)(yoffset - SMALLCHAR_HEIGHT*1.3f), version[x], 1.0f );


	// draw the text
	con.vislines = yoffset;
	rows = (yoffset-SMALLCHAR_WIDTH)/SMALLCHAR_WIDTH;		// rows of text to draw

	y = yoffset - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if (con.display != con.current)
	{// draw arrows to show the buffer is backscrolled
		re->SetColor( color );
		for (x=0 ; x<con.linewidth ; x+=4)
			SCR_DrawSmallChar( (int)(con.xadjust + (x+1)*SMALLCHAR_WIDTH), y, '^' );
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}
	
	row = con.display;

	if ( con.x == 0 ) {
		row--;
	}

	currentColor = 7;
	re->SetColor( &g_color_table[currentColor] );

	for (i=0 ; i<rows ; i++, y -= SMALLCHAR_HEIGHT, row--)
	{
		if (row < 0)
			break;
		if (con.current - row >= con.totallines) {
			// past scrollback wrap point
			continue;	
		}

		text = con.text + (row % con.totallines)*con.linewidth;

		for (x=0 ; x<con.linewidth ; x++) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}

			if ( ( (text[x]>>8)&Q_COLOR_BITS ) != currentColor ) {
				currentColor = (text[x]>>8)&Q_COLOR_BITS;
				re->SetColor( &g_color_table[currentColor] );
			}
			SCR_DrawSmallChar( (int)con.xadjust + (x+1)*SMALLCHAR_WIDTH, y, text[x] & 0xff );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput();
	
	// draw the scrollbar
	Con_DrawScrollbar( (frac * SCREEN_HEIGHT) - 5 - SMALLCHAR_HEIGHT, SCREEN_WIDTH - 5, 3, rows );

	re->SetColor( NULL );
}

void Con_DrawConsole( void ) {
	// check for console width changes from a vid mode change
	Con_CheckResize();

	// if disconnected, render console full screen
	if ( clc.state == CA_DISCONNECTED ) {
		if ( !( Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			Con_DrawSolidConsole( 1.0f );
			return;
		}
	}

	if ( con.displayFrac ) {
		Con_DrawSolidConsole( con.displayFrac );
	} else {
		// draw notify lines
		if ( clc.state == CA_ACTIVE ) {
			Con_DrawNotify();
		}
	}
}

// Scroll it up or down
void Con_RunConsole( void ) {
	// decide on the destination height of the console
	if ( Key_GetCatcher( ) & KEYCATCH_CONSOLE )
		con.finalFrac = 0.5f;		// half screen
	else
		con.finalFrac = 0;				// none visible
	
	// scroll towards the destination height
	if (con.finalFrac < con.displayFrac)
	{
		con.displayFrac -= con_conspeed->value*cls.realFrametime*0.001f;
		if (con.finalFrac > con.displayFrac)
			con.displayFrac = con.finalFrac;

	}
	else if (con.finalFrac > con.displayFrac)
	{
		con.displayFrac += con_conspeed->value*cls.realFrametime*0.001f;
		if (con.finalFrac < con.displayFrac)
			con.displayFrac = con.finalFrac;
	}

}


void Con_PageUp( void ) {
	con.display -= 2;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown( void ) {
	con.display += 2;
	if (con.display > con.current) {
		con.display = con.current;
	}
}

void Con_Top( void ) {
	con.display = con.totallines;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom( void ) {
	con.display = con.current;
}


void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &g_consoleField );
	Con_ClearNotify();
	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CONSOLE );
	con.finalFrac = 0;				// none visible
	con.displayFrac = 0;
}
