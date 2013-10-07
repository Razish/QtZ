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
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definately
// be a valid snapshot this frame

#include "cg_local.h"

#include "../../build/qtz/ui/menudef.h"

#define SCORE_OFFSET (13)

static void CG_ParseScores( void ) {
	int i, powerups;

	cg.numScores = atoi( trap->Cmd_Argv( 1 ) );
	if ( cg.numScores > MAX_CLIENTS )
		cg.numScores = MAX_CLIENTS;

	cg.teamScores[0] = atoi( trap->Cmd_Argv( 2 ) );
	cg.teamScores[1] = atoi( trap->Cmd_Argv( 3 ) );

	memset( cg.scores, 0, sizeof( cg.scores ) );

	for ( i=0; i<cg.numScores; i++ ) {
		cg.scores[i].client				= atoi( trap->Cmd_Argv( i*SCORE_OFFSET +  4 ) );
		cg.scores[i].score				= atoi( trap->Cmd_Argv( i*SCORE_OFFSET +  5 ) );
		cg.scores[i].ping				= atoi( trap->Cmd_Argv( i*SCORE_OFFSET +  6 ) );
		cg.scores[i].time				= atoi( trap->Cmd_Argv( i*SCORE_OFFSET +  7 ) );
		cg.scores[i].scoreFlags			= atoi( trap->Cmd_Argv( i*SCORE_OFFSET +  8 ) );
		powerups						= atoi( trap->Cmd_Argv( i*SCORE_OFFSET +  9 ) );
		cg.scores[i].accuracy			= atoi( trap->Cmd_Argv( i*SCORE_OFFSET + 10 ) );
		cg.scores[i].impressiveCount	= atoi( trap->Cmd_Argv( i*SCORE_OFFSET + 11 ) );
		cg.scores[i].excellentCount		= atoi( trap->Cmd_Argv( i*SCORE_OFFSET + 12 ) );
		cg.scores[i].defendCount		= atoi( trap->Cmd_Argv( i*SCORE_OFFSET + 13 ) );
		cg.scores[i].assistCount		= atoi( trap->Cmd_Argv( i*SCORE_OFFSET + 14 ) );
		cg.scores[i].perfect			= atoi( trap->Cmd_Argv( i*SCORE_OFFSET + 15 ) );
		cg.scores[i].captures			= atoi( trap->Cmd_Argv( i*SCORE_OFFSET + 16 ) );

		if ( cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS )
			cg.scores[i].client = 0;

		cgs.clientinfo[cg.scores[i].client].score = cg.scores[i].score;
		cgs.clientinfo[cg.scores[i].client].powerups = powerups;

		cg.scores[i].team = cgs.clientinfo[cg.scores[i].client].team;
	}

	CG_SetScoreSelection( NULL );

}

#define TEAMINFO_OFFSET (6)
static void CG_ParseTeamInfo( void ) {
	int		i;
	int		client;

	numSortedTeamPlayers = atoi( trap->Cmd_Argv( 1 ) );
	if( numSortedTeamPlayers < 0 || numSortedTeamPlayers > TEAM_MAXOVERLAY )
	{
		trap->Error( ERR_DROP, "CG_ParseTeamInfo: numSortedTeamPlayers out of range (%d)",
				numSortedTeamPlayers );
		return;
	}

	for ( i = 0 ; i < numSortedTeamPlayers ; i++ ) {
		client = atoi( trap->Cmd_Argv( i * 6 + 2 ) );
		if( client < 0 || client >= MAX_CLIENTS )
		{
		  trap->Error( ERR_DROP, "CG_ParseTeamInfo: bad client number: %d", client );
		  return;
		}

		sortedTeamPlayers[i] = client;

		cgs.clientinfo[ client ].location	= atoi( trap->Cmd_Argv( i*TEAMINFO_OFFSET + 3 ) );
		cgs.clientinfo[ client ].health		= atoi( trap->Cmd_Argv( i*TEAMINFO_OFFSET + 4 ) );
		cgs.clientinfo[ client ].armor		= atoi( trap->Cmd_Argv( i*TEAMINFO_OFFSET + 5 ) );
		cgs.clientinfo[ client ].curWeapon	= atoi( trap->Cmd_Argv( i*TEAMINFO_OFFSET + 6 ) );
		cgs.clientinfo[ client ].powerups	= atoi( trap->Cmd_Argv( i*TEAMINFO_OFFSET + 7 ) );
	}
}

// This is called explicitly when the gamestate is first received, and whenever the server updates any serverinfo flagged cvars
void CG_ParseServerinfo( void ) {
	const char	*info;
	char	*mapname;
	int i;

	info = CG_ConfigString( CS_SERVERINFO );
	cgs.gametype = atoi( Info_ValueForKey( info, "sv_gametype" ) );
	trap->Cvar_Set("sv_gametype", va("%i", cgs.gametype));
	cgs.dmflags = atoi( Info_ValueForKey( info, "dmflags" ) );
	cgs.capturelimit = atoi( Info_ValueForKey( info, "capturelimit" ) );

	//QtZ: So we can play the "1 minute warning" etc sounds if timelimit is changed during play
	i = atoi( Info_ValueForKey( info, "timelimit" ) );
	if ( cgs.timelimit != i )
		cg.timelimitWarnings &= ~(1|2);
	cgs.timelimit = i;

	// reset fraglimit warnings
	i = atoi( Info_ValueForKey( info, "fraglimit" ) );
	if ( cgs.fraglimit < i )
		cg.fraglimitWarnings &= ~(1|2|4);
	cgs.fraglimit = i;

	cgs.maxclients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cgs.mapname, sizeof( cgs.mapname ), "maps/%s.bsp", mapname );
	Q_strncpyz( cgs.redTeam, Info_ValueForKey( info, "g_redTeam" ), sizeof(cgs.redTeam) );
	trap->Cvar_Set("g_redTeam", cgs.redTeam);
	Q_strncpyz( cgs.blueTeam, Info_ValueForKey( info, "g_blueTeam" ), sizeof(cgs.blueTeam) );
	trap->Cvar_Set("g_blueTeam", cgs.blueTeam);

	//QtZ: Added
	Q_strncpyz( cgs.server.hostname, Info_ValueForKey( info, "sv_hostname" ), sizeof( cgs.server.hostname ) );

	// fix bogus vote strings
	Q_strncpyz( cgs.voteString, CG_ConfigString( CS_VOTE_STRING ), sizeof( cgs.voteString ) );

	trap->Cvar_Set( "ui_about_unlagged", va( "%i", atoi( Info_ValueForKey( info, "g_delagHitscan" ) ) ) );
	trap->Cvar_Set( "ui_about_svframetime", va( "%i", atoi( Info_ValueForKey( info, "sv_frametime" ) ) ) );
}

static void CG_ParseWarmup( void ) {
	const char	*info;
	int			warmup;

	info = CG_ConfigString( CS_WARMUP );

	warmup = atoi( info );
	cg.warmupCount = -1;

	if ( warmup == 0 && cg.warmup ) {
	} else if ( warmup > 0 && cg.warmup <= 0 ) {
		if ( cgs.gametype >= GT_TEAMBLOOD )
			trap->S_StartLocalSound( cgs.media.countPrepareTeamSound, CHAN_ANNOUNCER );
		else
			trap->S_StartLocalSound( cgs.media.countPrepareSound, CHAN_ANNOUNCER );
	}

	cg.warmup = warmup;
}

// Called on load to set the initial values from configstrings
void CG_SetConfigValues( void ) {
	const char *s;

	cgs.scores1 = atoi( CG_ConfigString( CS_SCORES1 ) );
	cgs.scores2 = atoi( CG_ConfigString( CS_SCORES2 ) );
	cgs.levelStartTime = atoi( CG_ConfigString( CS_LEVEL_START_TIME ) );
	if( cgs.gametype == GT_FLAGS ) {
		s = CG_ConfigString( CS_FLAGSTATUS );
		cgs.redflag = s[0] - '0';
		cgs.blueflag = s[1] - '0';
	}
	else if( cgs.gametype == GT_TROJAN ) {
		s = CG_ConfigString( CS_FLAGSTATUS );
		cgs.flagStatus = s[0] - '0';
	}
	cg.warmup = atoi( CG_ConfigString( CS_WARMUP ) );
}

static void CG_ConfigStringModified( void ) {
	const char	*str;
	int		num;

	num = atoi( trap->Cmd_Argv( 1 ) );

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap->GetGameState( &cgs.gameState );

	// look up the individual string that was modified
	str = CG_ConfigString( num );

	// do something with it if necessary
	if ( num == CS_MUSIC ) {
		CG_StartMusic();
	} else if ( num == CS_SERVERINFO ) {
		CG_ParseServerinfo();
	} else if ( num == CS_WARMUP ) {
		CG_ParseWarmup();
	} else if ( num == CS_SCORES1 ) {
		cgs.scores1 = atoi( str );
	} else if ( num == CS_SCORES2 ) {
		cgs.scores2 = atoi( str );
	} else if ( num == CS_LEVEL_START_TIME ) {
		cgs.levelStartTime = atoi( str );
	} else if ( num == CS_VOTE_TIME ) {
		cgs.voteTime = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_YES ) {
		cgs.voteYes = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_NO ) {
		cgs.voteNo = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_STRING ) {
		Q_strncpyz( cgs.voteString, str, sizeof( cgs.voteString ) );
		trap->S_StartLocalSound( cgs.media.voteNow, CHAN_ANNOUNCER );
	} else if ( num == CS_INTERMISSION ) {
		cg.intermissionStarted = atoi( str );
	} else if ( num >= CS_MODELS && num < CS_MODELS+MAX_MODELS ) {
		cgs.gameModels[ num-CS_MODELS ] = trap->R_RegisterModel( str );
	} else if ( num >= CS_SOUNDS && num < CS_SOUNDS+MAX_SOUNDS ) {
		if ( str[0] != '*' ) {	// player specific sounds don't register here
			cgs.gameSounds[ num-CS_SOUNDS] = trap->S_RegisterSound( str, qfalse );
		}
	} else if ( num >= CS_PLAYERS && num < CS_PLAYERS+MAX_CLIENTS ) {
		CG_NewClientInfo( num - CS_PLAYERS );
		CG_BuildSpectatorString();
	} else if ( num == CS_FLAGSTATUS ) {
		if( cgs.gametype == GT_FLAGS ) {
			// format is rb where its red/blue, 0 is at base, 1 is taken, 2 is dropped
			cgs.redflag = str[0] - '0';
			cgs.blueflag = str[1] - '0';
		}
		else if( cgs.gametype == GT_TROJAN ) {
			cgs.flagStatus = str[0] - '0';
		}
	}
}

static void CG_AddToTeamChat( const char *str ) {
	int len;
	char *p, *ls;
	int lastcolor;
	int chatHeight = TEAMCHAT_HEIGHT;

	if (chatHeight <= 0 || cg_teamChatTime->integer <= 0) {
		// team chat disabled, dump into normal chat
		cgs.teamChatPos = cgs.teamLastChatPos = 0;
		return;
	}

	len = 0;

	p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while (*str) {
		if (len > TEAMCHAT_WIDTH - 1) {
			if (ls) {
				str -= (p - ls);
				str++;
				p -= (p - ls);
			}
			*p = 0;

			cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;

			cgs.teamChatPos++;
			p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
			*p = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len = 0;
			ls = NULL;
		}

		if ( Q_IsColorString( str ) ) {
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if (*str == ' ') {
			ls = p;
		}
		*p++ = *str++;
		len++;
	}
	*p = 0;

	cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;
	cgs.teamChatPos++;

	if (cgs.teamChatPos - cgs.teamLastChatPos > chatHeight)
		cgs.teamLastChatPos = cgs.teamChatPos - chatHeight;
}

// The server has issued a map_restart, so the next snapshot is completely new and should not be interpolated to.
//	A tournament restart will clear everything, but doesn't require a reload of all the media
static void CG_MapRestart( void ) {
	CG_InitLocalEntities();
	CG_InitMarkPolys();
	CG_ClearParticles();

	// make sure the "3 frags left" warnings play again
	cg.fraglimitWarnings = 0;

	cg.timelimitWarnings = 0;
	cg.rewardTime = 0;
	cg.rewardStack = 0;
	cg.intermissionStarted = qfalse;

	cgs.voteTime = 0;

	cg.mapRestart = qtrue;

	CG_StartMusic();

	trap->S_ClearLoopingSounds(qtrue);

	// we really should clear more parts of cg here and stop sounds

	// play the "fight" sound if this is a restart without warmup
	if ( cg.warmup == 0 /* && cgs.gametype == GT_DUEL */) {
		trap->S_StartLocalSound( cgs.media.countFightSound, CHAN_ANNOUNCER );
		CG_CenterPrint( "FIGHT!", 120 );
	}
}

#define MAX_VOICEFILESIZE	16384
#define MAX_VOICEFILES		8
#define MAX_VOICECHATS		192
#define MAX_VOICESOUNDS		192
#define MAX_CHATSIZE		192
#define MAX_VOICEMODELS		256

typedef struct voiceChat_s
{
	char id[64];
	int numSounds;
	sfxHandle_t sounds[MAX_VOICESOUNDS];
	char chats[MAX_VOICESOUNDS][MAX_CHATSIZE];
} voiceChat_t;

typedef struct voiceChatList_s
{
	char name[64];
	int numVoiceChats;
	voiceChat_t voiceChats[MAX_VOICECHATS];
} voiceChatList_t;

typedef struct modelVoiceChat_s
{
	char model[64];
	int voiceChatNum;
} modelVoiceChat_t;

voiceChatList_t voiceChatLists[MAX_VOICEFILES];
modelVoiceChat_t modelVoiceChat[MAX_VOICEMODELS];

int CG_ParseVoiceChats( const char *filename, voiceChatList_t *voiceChatList, int maxVoiceChats ) {
	int	len, i;
	fileHandle_t f;
	char buf[MAX_VOICEFILESIZE];
	char **p, *ptr;
	char *token;
	voiceChat_t *voiceChats;
	sfxHandle_t sound;

	len = trap->FS_Open( filename, &f, FS_READ );
	if ( !f ) {
		trap->Print( S_COLOR_RED "voice chat file not found: %s\n", filename );
		return qfalse;
	}
	if ( len >= MAX_VOICEFILESIZE ) {
		trap->Print( S_COLOR_RED "voice chat file too large: %s is %i, max allowed is %i\n", filename, len, MAX_VOICEFILESIZE );
		trap->FS_Close( f );
		return qfalse;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );

	ptr = buf;
	p = &ptr;

	Com_sprintf(voiceChatList->name, sizeof(voiceChatList->name), "%s", filename);
	voiceChats = voiceChatList->voiceChats;
	for ( i = 0; i < maxVoiceChats; i++ ) {
		voiceChats[i].id[0] = 0;
	}
	token = COM_ParseExt(p, qtrue);
	if (!token || token[0] == 0) {
		return qtrue;
	}

	voiceChatList->numVoiceChats = 0;
	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);
		if (!token || token[0] == 0) {
			return qtrue;
		}
		Com_sprintf(voiceChats[voiceChatList->numVoiceChats].id, sizeof( voiceChats[voiceChatList->numVoiceChats].id ), "%s", token);
		token = COM_ParseExt(p, qtrue);
		if (Q_stricmp(token, "{")) {
			trap->Print( S_COLOR_RED "expected { found %s in voice chat file: %s\n", token, filename );
			return qfalse;
		}
		voiceChats[voiceChatList->numVoiceChats].numSounds = 0;
		while(1) {
			token = COM_ParseExt(p, qtrue);
			if (!token || token[0] == 0) {
				return qtrue;
			}
			if (!Q_stricmp(token, "}"))
				break;
			sound = trap->S_RegisterSound( token, qtrue );
			voiceChats[voiceChatList->numVoiceChats].sounds[voiceChats[voiceChatList->numVoiceChats].numSounds] = sound;
			token = COM_ParseExt(p, qtrue);
			if (!token || token[0] == 0) {
				return qtrue;
			}
			Com_sprintf(voiceChats[voiceChatList->numVoiceChats].chats[
							voiceChats[voiceChatList->numVoiceChats].numSounds], MAX_CHATSIZE, "%s", token);
			if (sound)
				voiceChats[voiceChatList->numVoiceChats].numSounds++;
			if (voiceChats[voiceChatList->numVoiceChats].numSounds >= MAX_VOICESOUNDS)
				break;
		}
		voiceChatList->numVoiceChats++;
		if (voiceChatList->numVoiceChats >= maxVoiceChats)
			return qtrue;
	}
	return qtrue;
}

void CG_LoadVoiceChats( void ) {
	int size;

	size = trap->MemoryRemaining();
	CG_ParseVoiceChats( "scripts/female1.voice", &voiceChatLists[0], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/female2.voice", &voiceChatLists[1], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/female3.voice", &voiceChatLists[2], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/male1.voice", &voiceChatLists[3], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/male2.voice", &voiceChatLists[4], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/male3.voice", &voiceChatLists[5], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/male4.voice", &voiceChatLists[6], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/male5.voice", &voiceChatLists[7], MAX_VOICECHATS );
	trap->Print("voice chat memory size = %d\n", size - trap->MemoryRemaining());
}

int CG_HeadModelVoiceChats( char *filename ) {
	int	len, i;
	fileHandle_t f;
	char buf[MAX_VOICEFILESIZE];
	char **p, *ptr;
	char *token;

	len = trap->FS_Open( filename, &f, FS_READ );
	if ( !f ) {
		//trap->Print( "voice chat file not found: %s\n", filename );
		return -1;
	}
	if ( len >= MAX_VOICEFILESIZE ) {
		trap->Print( S_COLOR_RED "voice chat file too large: %s is %i, max allowed is %i\n", filename, len, MAX_VOICEFILESIZE );
		trap->FS_Close( f );
		return -1;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );

	ptr = buf;
	p = &ptr;

	token = COM_ParseExt(p, qtrue);
	if (!token || token[0] == 0) {
		return -1;
	}

	for ( i = 0; i < MAX_VOICEFILES; i++ ) {
		if ( !Q_stricmp(token, voiceChatLists[i].name) ) {
			return i;
		}
	}

	//FIXME: maybe try to load the .voice file which name is stored in token?

	return -1;
}

int CG_GetVoiceChat( voiceChatList_t *voiceChatList, const char *id, sfxHandle_t *snd, char **chat) {
	int i, rnd;

	for ( i = 0; i < voiceChatList->numVoiceChats; i++ ) {
		if ( !Q_stricmp( id, voiceChatList->voiceChats[i].id ) ) {
			rnd = (int)(random() * voiceChatList->voiceChats[i].numSounds);
			*snd = voiceChatList->voiceChats[i].sounds[rnd];
			*chat = voiceChatList->voiceChats[i].chats[rnd];
			return qtrue;
		}
	}
	return qfalse;
}

voiceChatList_t *CG_VoiceChatListForClient( int clientNum ) {
	clientInfo_t *ci;
	int voiceChatNum, i, j, k;
	char filename[MAX_QPATH], modelName[MAX_QPATH];

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	for ( k = 0; k < 2; k++ ) {
		Com_sprintf( modelName, sizeof(modelName), "%s", ci->modelName );
		// find the voice file for the head model the client uses
		for ( i = 0; i < MAX_VOICEMODELS; i++ ) {
			if (!Q_stricmp(modelVoiceChat[i].model, modelName)) {
				break;
			}
		}
		if (i < MAX_VOICEMODELS) {
			return &voiceChatLists[modelVoiceChat[i].voiceChatNum];
		}
		// find a <headmodelname>.vc file
		for ( i = 0; i < MAX_VOICEMODELS; i++ ) {
			if (!strlen(modelVoiceChat[i].model)) {
				Com_sprintf(filename, sizeof(filename), "scripts/%s.vc", modelName);
				voiceChatNum = CG_HeadModelVoiceChats(filename);
				if (voiceChatNum == -1)
					break;
				Com_sprintf(modelVoiceChat[i].model, sizeof ( modelVoiceChat[i].model ),
							"%s", modelName);
				modelVoiceChat[i].voiceChatNum = voiceChatNum;
				return &voiceChatLists[modelVoiceChat[i].voiceChatNum];
			}
		}
	}
	// store this head model with voice chat for future reference
	for ( j = 0; j < MAX_VOICEMODELS; j++ ) {
		if (!strlen(modelVoiceChat[j].model)) {
			Com_sprintf(modelVoiceChat[j].model, sizeof ( modelVoiceChat[j].model ),
					"%s", modelName);
			modelVoiceChat[j].voiceChatNum = 0;
			break;
		}
	}
	// just return the first voice chat list
	return &voiceChatLists[0];
}

#define MAX_VOICECHATBUFFER		32

typedef struct bufferedVoiceChat_s
{
	int clientNum;
	sfxHandle_t snd;
	int voiceOnly;
	char cmd[MAX_SAY_TEXT];
	char message[MAX_SAY_TEXT];
} bufferedVoiceChat_t;

bufferedVoiceChat_t voiceChatBuffer[MAX_VOICECHATBUFFER];

void CG_PlayVoiceChat( bufferedVoiceChat_t *vchat ) {
	// if we are going into the intermission, don't start any voices
	if ( cg.intermissionStarted ) {
		return;
	}

	trap->S_StartLocalSound( vchat->snd, CHAN_VOICE);
	if (vchat->clientNum != cg.snap->ps.clientNum) {
		// see if this was an order
		CG_ShowResponseHead();
	}

	if (!vchat->voiceOnly) {
		CG_AddToTeamChat( vchat->message );
		trap->Print( "%s\n", vchat->message );
	}
	voiceChatBuffer[cg.voiceChatBufferOut].snd = 0;
}

void CG_PlayBufferedVoiceChats( void ) {
	if ( cg.voiceChatTime < cg.time ) {
		if (cg.voiceChatBufferOut != cg.voiceChatBufferIn && voiceChatBuffer[cg.voiceChatBufferOut].snd) {
			//
			CG_PlayVoiceChat(&voiceChatBuffer[cg.voiceChatBufferOut]);
			//
			cg.voiceChatBufferOut = (cg.voiceChatBufferOut + 1) % MAX_VOICECHATBUFFER;
			cg.voiceChatTime = cg.time + 1000;
		}
	}
}

void CG_AddBufferedVoiceChat( bufferedVoiceChat_t *vchat ) {
	// if we are going into the intermission, don't start any voices
	if ( cg.intermissionStarted ) {
		return;
	}

	memcpy(&voiceChatBuffer[cg.voiceChatBufferIn], vchat, sizeof(bufferedVoiceChat_t));
	cg.voiceChatBufferIn = (cg.voiceChatBufferIn + 1) % MAX_VOICECHATBUFFER;
	if (cg.voiceChatBufferIn == cg.voiceChatBufferOut) {
		CG_PlayVoiceChat( &voiceChatBuffer[cg.voiceChatBufferOut] );
		cg.voiceChatBufferOut++;
	}
}

void CG_VoiceChatLocal( int mode, qboolean voiceOnly, int clientNum, int color, const char *cmd ) {
	char *chat;
	voiceChatList_t *voiceChatList;
	clientInfo_t *ci;
	sfxHandle_t snd;
	bufferedVoiceChat_t vchat;

	// if we are going into the intermission, don't start any voices
	if ( cg.intermissionStarted ) {
		return;
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	voiceChatList = CG_VoiceChatListForClient( clientNum );

	if ( CG_GetVoiceChat( voiceChatList, cmd, &snd, &chat ) ) {
		//
		if ( mode == SAY_TEAM || !cg_teamChatsOnly->integer ) {
			vchat.clientNum = clientNum;
			vchat.snd = snd;
			vchat.voiceOnly = voiceOnly;
			Q_strncpyz(vchat.cmd, cmd, sizeof(vchat.cmd));
			if ( mode == SAY_TELL ) {
				Com_sprintf(vchat.message, sizeof(vchat.message), "[%s]: %c%c%s", ci->name, Q_COLOR_ESCAPE, color, chat);
			}
			else if ( mode == SAY_TEAM ) {
				Com_sprintf(vchat.message, sizeof(vchat.message), "(%s): %c%c%s", ci->name, Q_COLOR_ESCAPE, color, chat);
			}
			else {
				Com_sprintf(vchat.message, sizeof(vchat.message), "%s: %c%c%s", ci->name, Q_COLOR_ESCAPE, color, chat);
			}
			CG_AddBufferedVoiceChat(&vchat);
		}
	}
}

void CG_VoiceChat( int mode ) {
	const char *cmd;
	int clientNum, color;
	qboolean voiceOnly;

	voiceOnly = atoi(trap->Cmd_Argv(1));
	clientNum = atoi(trap->Cmd_Argv(2));
	color = atoi(trap->Cmd_Argv(3));
	cmd = trap->Cmd_Argv(4);

	if (cg_noTaunt->integer != 0) {
		if (!strcmp(cmd, VOICECHAT_KILLINSULT)  || !strcmp(cmd, VOICECHAT_TAUNT) ||
			!strcmp(cmd, VOICECHAT_DEATHINSULT) || !strcmp(cmd, VOICECHAT_PRAISE)) {
			return;
		}
	}

	CG_VoiceChatLocal( mode, voiceOnly, clientNum, color, cmd );
}

// The string has been tokenized and can be retrieved with Cmd_Argc() / Cmd_Argv()
static void CG_ServerCommand( void ) {
	const char	*cmd;

	cmd = trap->Cmd_Argv(0);

	if ( !cmd[0] ) {
		// server claimed the command
		return;
	}

	if ( !strcmp( cmd, "cp" ) ) {
		CG_CenterPrint( trap->Cmd_Argv(1), (int)(SCREEN_HEIGHT * 0.30f) );
		return;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CG_ConfigStringModified();
		return;
	}

	//RAZTODO: WTF, rewrite plz.
	if ( !strcmp( cmd, "print" ) ) {
		trap->Print( "%s", trap->Cmd_Argv(1) );
		cmd = trap->Cmd_Argv(1);			// yes, this is obviously a hack, but so is the way we hear about
									// votes passing or failing
		if ( !Q_stricmpn( cmd, "vote failed", 11 ) || !Q_stricmpn( cmd, "team vote failed", 16 )) {
			trap->S_StartLocalSound( cgs.media.voteFailed, CHAN_ANNOUNCER );
		} else if ( !Q_stricmpn( cmd, "vote passed", 11 ) || !Q_stricmpn( cmd, "team vote passed", 16 ) ) {
			trap->S_StartLocalSound( cgs.media.votePassed, CHAN_ANNOUNCER );
		}
		return;
	}

	if ( !strcmp( cmd, "chat" ) ) {
		if ( !cg_teamChatsOnly->integer ) {
			char text[MAX_SAY_TEXT] = {0};
			trap->S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
			Com_sprintf( text, sizeof( text ), "%s"S_COLOR_WHITE": %s", cgs.clientinfo[atoi( trap->Cmd_Argv(1) )].name, trap->Cmd_Argv(2) );
			CG_ChatboxAdd( text, qfalse );
			trap->Print( "[skipnotify]%s\n", text );
		}
		return;
	}

	if ( !strcmp( cmd, "tchat" ) ) {
		char text[MAX_SAY_TEXT] = {0};
		trap->S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
		Com_sprintf( text, sizeof( text ), "%s"S_COLOR_WHITE": %s", cgs.clientinfo[atoi( trap->Cmd_Argv(1) )].name, trap->Cmd_Argv(2) );
		CG_AddToTeamChat( text );
		CG_ChatboxAdd( text, qfalse );
		trap->Print( "[skipnotify]%s\n", text );
		return;
	}

	if ( !strcmp( cmd, "vchat" ) ) {
		CG_VoiceChat( SAY_ALL );
		return;
	}

	if ( !strcmp( cmd, "vtchat" ) ) {
		CG_VoiceChat( SAY_TEAM );
		return;
	}

	if ( !strcmp( cmd, "vtell" ) ) {
		CG_VoiceChat( SAY_TELL );
		return;
	}

	if ( !strcmp( cmd, "scores" ) ) {
		CG_ParseScores();
		return;
	}

	if ( !strcmp( cmd, "tinfo" ) ) {
		CG_ParseTeamInfo();
		return;
	}

	if ( !strcmp( cmd, "map_restart" ) ) {
		CG_MapRestart();
		return;
	}

	// loaddeferred can be both a servercmd and a consolecmd
	if ( !strcmp( cmd, "loaddefered" ) ) {	// FIXME: spelled wrong, but not changing for demo
		CG_LoadDeferredPlayers();
		return;
	}

	trap->Print( "Unknown client game command: %s\n", cmd );
}


// Execute all of the server commands that were received along with this this snapshot.
void CG_ExecuteNewServerCommands( int latestSequence ) {
	while ( cgs.serverCommandSequence < latestSequence ) {
		if ( trap->GetServerCommand( ++cgs.serverCommandSequence ) ) {
			CG_ServerCommand();
		}
	}
}
