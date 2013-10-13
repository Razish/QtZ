
#include "cg_local.h"
#include "../../build/qtz/ui/menudef.h"
#include "../client/keycodes.h"

#define MAX_CHATBOX_ENTRIES (512)
#define MAX_CHATBOX_IDENTIFIER_SIZE (32)

#define CHATBOX_POS_X (8)
#define CHATBOX_POS_Y (SCREEN_HEIGHT*0.66666f)
#define CHATBOX_LINE_HEIGHT (16)
#define CHATBOX_FONT_SCALE (0.33333f)

#define CHAT_MESSAGE_LENGTH MAX_SAY_TEXT

// chatbox
typedef struct chatEntry_s {
	char		message[CHAT_MESSAGE_LENGTH];
	qboolean	isUsed;
	int			time;
} chatEntry_t;

static struct chatBox_s {
	chatEntry_t		chatBuffer[MAX_CHATBOX_ENTRIES]; // A logical message may be fragmented into multiple chatEntry_t structures, based on line width.
													// This is to allow scrolling cleanly but has the down-side of line splitting only being computed once
	int				numActiveLines; // totaly amount of lines used
	int				scrollAmount; // how many lines we've scrolled upwards
} chatbox;

// chatbox input history
typedef struct chatHistory_s {
	struct chatHistory_s *next, *prev;

	char message[MAX_EDIT_LINE];
} chatHistory_t;
static chatHistory_t *chatHistory = NULL;
static chatHistory_t *currentHistory = NULL;

// chatbox input
field_t		chatField;
qboolean	chat_team;
qboolean	chatActive;

static chatHistory_t *CreateChatHistoryObject( const char *message ) {
	chatHistory_t *ch = (chatHistory_t *)malloc( sizeof( chatHistory_t ) );
	memset( ch, 0, sizeof( chatHistory_t ) );
	Q_strncpyz( ch->message, message, sizeof( ch->message ) );
	return ch;
}

static chatHistory_t *GetNewChatHistory( const char *message ) {
	chatHistory_t *ch = NULL, *prev = chatHistory;

	ch = CreateChatHistoryObject( message );
	if ( prev )
		chatHistory = prev->prev = ch; // Attach it to the start of the list
	else
		chatHistory = ch;

	ch->next = prev;
	return ch;
}

void CG_ChatboxOutgoing( void ) {
	// remove the key catcher
	CG_ChatboxEscape();
	
	// commit the current line to history
	GetNewChatHistory( chatField.buffer );

	// intercept team messages, replace teambinds like #h
	if ( chat_team ) {
		CG_HandleTeamBinds( chatField.buffer, sizeof( chatField.buffer ) );
		trap->SendClientCommand( va( "say_team %s", chatField.buffer ) );
		return;
	}

	// send as regular message
	trap->SendClientCommand( va( "say %s", chatField.buffer ) );
}

// This function is called recursively when a logical message has to be split into multiple lines
void CG_ChatboxAdd( const char *message, qboolean multiLine ) {
	chatEntry_t *chat = &chatbox.chatBuffer[MAX_CHATBOX_ENTRIES-1];
	size_t strLength = 0;
	int i = 0;
	float accumLength = 0.0f;

	chatbox.numActiveLines++;

	//Stop scrolling up if we've already scrolled, similar to console behaviour
	if ( chatbox.scrollAmount < 0 )
		chatbox.scrollAmount = MAX( chatbox.scrollAmount - 1, chatbox.numActiveLines >= cg_chatboxLineCount->integer ? ((MIN(chatbox.numActiveLines, MAX_CHATBOX_ENTRIES)-cg_chatboxLineCount->integer)*-1) : 0 ); //cb->scrollAmount--;

	for ( i=0, strLength=strlen( message );
		 i<strLength && i<CHAT_MESSAGE_LENGTH;
		 i++ )
	{
		char *p = (char*)&message[i];
		char buf[1];

		buf[0] = *p;

		if ( !Q_IsColorString( p ) && (i > 0 && !Q_IsColorString( p-1 )) )
			accumLength += CG_Text_Width( buf, CHATBOX_FONT_SCALE, -1 );

		if ( accumLength > SCREEN_WIDTH && (i>0 && !Q_IsColorString( p-1 )) ) {
			char lastColor = '2';
			int j = i;
			int savedOffset = i;
			char tempMessage[CHAT_MESSAGE_LENGTH];

			//Attempt to back-track, find a space (' ') within X characters or pixels
			//RAZTODO: Another method using character width? Meh
			while ( message[i] != ' ' )
			{
				if ( i <= 0 || i < savedOffset-16 )
				{
					i = j = savedOffset;
					break;
				}
				i--;
				j--;
			}

			memmove( &chatbox.chatBuffer[0], &chatbox.chatBuffer[1], sizeof( chatbox.chatBuffer ) - sizeof( chatEntry_t ) );
			memset( chat, 0, sizeof( chatEntry_t ) ); //Clear the last element, ready for writing
			Q_strncpyz( chat->message, message, i+1 );
			chat->time = cg.time + cg_chatboxMsgTime->integer*1000;

			chat->isUsed = qtrue;
			for ( j=i; j>=0; j-- )
			{
				if ( message[j] == '^' && message[j+1] >= '0' && message[j+1] <= '9' )
				{
					lastColor = message[j+1];
					break;
				}
			}
			Com_sprintf( tempMessage, sizeof( tempMessage ), "^%c%s", lastColor, (const char *)(message + i) );

			//Recursively insert until we don't have to split the message
			CG_ChatboxAdd( tempMessage, qtrue );
			return;
		}
	}

	memmove( &chatbox.chatBuffer[0], &chatbox.chatBuffer[1], sizeof( chatbox.chatBuffer ) - sizeof( chatEntry_t ) );
	memset( chat, 0, sizeof( chatEntry_t ) ); //Clear the last element, ready for writing
	Q_strncpyz( chat->message, message, i+1 );

	chat->time = cg.time + cg_chatboxMsgTime->integer*1000;
	chat->isUsed = qtrue;
}

void CG_DrawChatbox( void ) {
	int i = MAX_CHATBOX_ENTRIES-MIN( cg_chatboxLineCount->integer, chatbox.numActiveLines );
	int numLines = 0;
	int done = 0;
	chatEntry_t *last = NULL;

	if ( CG_ChatboxActive() ) {
		char *pre = "Say: ";
		if ( chat_team )
			pre = "Team: ";
		CG_Text_Paint( CHATBOX_POS_X, CHATBOX_POS_Y + (CHATBOX_LINE_HEIGHT*cg_chatboxLineCount->integer), CHATBOX_FONT_SCALE, (vector4 *)&g_color_table[ColorIndex(COLOR_WHITE)], va( chat_team ? "Team: %s" : "Say: %s", chatField.buffer ), 0, 0, ITEM_TEXTSTYLE_SHADOWED );
	}

	if ( cg.scoreBoardShowing && !(cg.snap && cg.snap->ps.pm_type == PM_INTERMISSION) )
		return;

	//i is the ideal index. Now offset for scrolling
	i += chatbox.scrollAmount;

	if ( chatbox.numActiveLines == 0 )
		return;

	if ( chatbox.scrollAmount < 0 && CG_ChatboxActive() )
		CG_Text_Paint( CHATBOX_POS_X, CHATBOX_POS_Y - (CHATBOX_LINE_HEIGHT), CHATBOX_FONT_SCALE, (vector4 *)&g_color_table[ColorIndex(COLOR_WHITE)], va( "^3Scrolled lines: ^5%i\n", chatbox.scrollAmount*-1 ), 0, 0, ITEM_TEXTSTYLE_SHADOWED );

	for ( done = 0; done<cg_chatboxLineCount->integer && i<MAX_CHATBOX_ENTRIES; i++, done++ )
	{
		chatEntry_t *chat = &chatbox.chatBuffer[i];
		if ( chat->isUsed )
		{
			last = chat;
			if ( chat->time >= cg.time-cg_chatboxMsgTime->integer || (chatbox.scrollAmount && CG_ChatboxActive()) || CG_ChatboxActive() ) {
				CG_Text_Paint( CHATBOX_POS_X, CHATBOX_POS_Y + (CHATBOX_LINE_HEIGHT * numLines), CHATBOX_FONT_SCALE, (vector4 *)&g_color_table[ColorIndex(COLOR_WHITE)], chat->message, 0, 0, ITEM_TEXTSTYLE_SHADOWED );
				numLines++;
			}
		}
	}

//	if ( last->isUsed && last->time < cg.time-cg_chatBox.integer && !currentChatbox->scrollAmount && cg_chatBox.integer > 1 )
//		CG_Text_Paint( cg_hudChatX.value, cg_hudChatY.value + (cg_chatLH.value * numLines), cg_hudChatS.value, colorWhite, va( "%s%s", (cg_chatTimeStamp.integer ? last->timeStamp : ""), last->message ), 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, JP_GetChatboxFont() );
}

void CG_ChatboxScroll( int direction ) {
	int scrollAmount = chatbox.scrollAmount;
	int numActiveLines = chatbox.numActiveLines;

	if ( direction == 0 )
	{//down
		chatbox.scrollAmount = MIN( scrollAmount + 1, 0 );
	}
	else
	{//up
		chatbox.scrollAmount = MAX( scrollAmount - 1, numActiveLines >= cg_chatboxLineCount->integer ? ((MIN(numActiveLines,MAX_CHATBOX_ENTRIES)-cg_chatboxLineCount->integer)*-1) : 0 );
	}
}

void CG_ChatboxTabComplete( void ) {
	if ( cg_chatboxCompletion->integer ) {
		int i=0, match = -1, numMatches = 0;
		char currWord[MAX_INFO_STRING] = { 0 };
		char matches[MAX_CLIENTS][MAX_NETNAME] = { {0} }; // because cgs.clientinfo[i].name uses MAX_QPATH...wtf...
		char *p = &chatField.buffer[0];//[chatField->cursor];

		p = &chatField.buffer[chatField.cursor];
		//find current word
		while ( p > &chatField.buffer[0] && *(p-1) != ' ' )
			p--;

		if ( !*p )
			return;

		Q_strncpyz( currWord, p, sizeof( currWord ) );
		Q_CleanColorStr( currWord );
		Q_strlwr( currWord );

		for ( i=0; i<cgs.maxclients; i++ ) {
			if ( cgs.clientinfo[i].infoValid ) {
				char name[MAX_QPATH/*MAX_NETNAME*/] = { 0 }; // because cgs.clientinfo[i].name uses MAX_QPATH...wtf...

				Q_strncpyz( name, cgs.clientinfo[i].name, sizeof( name ) );
				Q_CleanColorStr( name );
				Q_strlwr( name );
				if ( strstr( name, currWord ) )
				{
					match = i;
					Q_strncpyz( matches[numMatches++], cgs.clientinfo[i].name, sizeof( matches[0] ) );
				}
			}
		}

		if ( numMatches == 1 ) {
			size_t oldCursor = chatField.cursor;
			ptrdiff_t delta = &chatField.buffer[oldCursor] - p;
			const char *str = va( "%s ^2", cgs.clientinfo[match].name );
			size_t drawLen, len;

			Q_strncpyz( p, str, sizeof( chatField.buffer ) - (p - &chatField.buffer[0]) );
			chatField.cursor = oldCursor - delta + strlen( str );

			//make sure cursor is visible
			drawLen = chatField.widthInChars - 1;
			len = strlen( chatField.buffer );
			if ( chatField.scroll + drawLen > len )
			{
				chatField.scroll = (int)(len - drawLen);
				if ( chatField.scroll < 0 )
					chatField.scroll = 0;
			}
		}
		else if ( numMatches > 1 )
		{
			CG_ChatboxAdd( va( "Several matches found for '%s':", currWord ), qfalse );
			for ( i=0; i</*min( 3, numMatches )*/numMatches; i++ )
				CG_ChatboxAdd( va( "^2- ^7%s", matches[i] ), qfalse );
		//	if ( numMatches > 3 )
		//		CG_ChatboxAdd( "^2- ^7[...] truncated", qfalse, "normal" );
			trap->S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
		}
	}
}

void CG_ChatboxHistoryUp( void ) {
//	Com_Printf( "History up\n" );
	if ( currentHistory )
	{
		if ( currentHistory->next )
			currentHistory = currentHistory->next;
	}
	else if ( chatHistory )
		currentHistory = chatHistory;
	else
		return;

	Q_strncpyz( chatField.buffer, currentHistory->message, sizeof( chatField.buffer ) );
	chatField.cursor = strlen( chatField.buffer );
}

void CG_ChatboxHistoryDn( void ) {
//	Com_Printf( "History down\n" );
	if ( currentHistory )
		currentHistory = currentHistory->prev;
	if ( currentHistory )
	{
		Q_strncpyz( chatField.buffer, currentHistory->message, sizeof( chatField.buffer ) );
		chatField.cursor = strlen( chatField.buffer );
	}
	else
		Field_Clear( &chatField );
}

void CG_ChatboxClear( void ) {
	if ( !trap->Key_IsDown( K_CTRL ) )
		return;

	chatbox.numActiveLines = 0;
	memset( chatbox.chatBuffer, 0, sizeof( chatbox.chatBuffer ) );
	chatbox.scrollAmount = 0;
	//TODO: Clear chat history?
}

void CG_ChatboxEscape( void ) {
	chatActive = qfalse;
	trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_CGAME );
}

void CG_MessageModeAll( void ) {
	chatActive = qtrue;
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	trap->Key_SetCatcher( trap->Key_GetCatcher() ^ KEYCATCH_CGAME );
}

void CG_MessageModeTeam( void ) {
	chatActive = qtrue;
	chat_team = qtrue;
	Field_Clear( &chatField );
	chatField.widthInChars = 25;
	trap->Key_SetCatcher( trap->Key_GetCatcher() ^ KEYCATCH_CGAME );
}

void CG_ChatboxChar( char key ) {
	trap->Field_CharEvent( &chatField, key );
}

qboolean CG_ChatboxActive( void ) {
	return chatActive && (trap->Key_GetCatcher() & KEYCATCH_CGAME);
}
