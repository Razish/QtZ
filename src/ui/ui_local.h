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
#pragma once

#include "../qcommon/q_shared.h"
#include "../renderer/tr_types.h"
#include "ui_public.h"
#include "../client/keycodes.h"
#include "../game/bg_public.h"
#include "ui_shared.h"

#define XCVAR_PROTO
	#include "ui_xcvar.h"
#undef XCVAR_PROTO

//
// ui_main.c
//
void UI_Report( void );
void UI_Load( void );
void UI_LoadMenus( const char *menuFile, qboolean reset );
void UI_LoadArenas( void );

//
// ui_players.c
//

//FIXME ripped from cg_local.h
typedef struct lerpFrame_s {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	int			animationNumber;	// may include ANIM_TOGGLEBIT
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact
} lerpFrame_t;

typedef struct playerInfo_s {
	// model info
	qhandle_t		legsModel, torsoModel, headModel;
	qhandle_t		legsSkin, torsoSkin, headSkin;
	lerpFrame_t		legs, torso;

	animation_t		animations[MAX_TOTALANIMATIONS];

	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	vector3			flashDlightColor;
	int				muzzleFlashTime;

	// currently in use drawing parms
	vector3			viewAngles;
	vector3			moveAngles;
	weapon_t		currentWeapon;
	int				legsAnim;
	int				torsoAnim;

	// animation vars
	weapon_t		weapon;
	weapon_t		lastWeapon;
	weapon_t		pendingWeapon;
	int				weaponTimer;
	int				pendingLegsAnim;
	int				torsoAnimationTimer;

	int				pendingTorsoAnim;
	int				legsAnimationTimer;

	qboolean		chat;
	qboolean		newModel;

	qboolean		barrelSpinning;
	float			barrelAngle;
	int				barrelTime;

	int				realWeapon;
} playerInfo_t;

void UI_DrawPlayer( float x, float y, float w, float h, playerInfo_t *pi, int time );
void UI_PlayerInfo_SetModel( playerInfo_t *pi, const char *model, const char *headmodel, char *teamName );
void UI_PlayerInfo_SetInfo( playerInfo_t *pi, int legsAnim, int torsoAnim, vector3 *viewAngles, vector3 *moveAngles, weapon_t weaponNum, qboolean chat );
qboolean UI_RegisterClientModelname( playerInfo_t *pi, const char *modelSkinName , const char *headName, const char *teamName);

//
// ui_atoms.c
//

// new ui stuff
#define MAX_HEADS				256
#define MAX_MAPS				256	// match g_bot.c
#define PLAYERS_PER_TEAM		5
#define MAX_PINGREQUESTS		32
#define MAX_ADDRESSLENGTH		64
#define MAX_HOSTNAMELENGTH		22
#define MAX_MAPNAMELENGTH		16
#define MAX_DISPLAY_SERVERS		2048
#define MAX_SERVERSTATUS_LINES	128
#define MAX_SERVERSTATUS_TEXT	2048
#define MAX_FOUNDPLAYER_SERVERS	16
#define MAX_MODS				64
#define MAX_DEMOS				2048
#define MAX_MOVIES				256
#define MAX_PLAYERMODELS		256

typedef struct characterInfo_s {
	const char	*name;
	const char	*imageName;
	qhandle_t	headImage;
	const char	*base;
	qboolean	active;
	int			reference;
} characterInfo_t;

typedef struct mapInfo_s {
	const char	*mapName;
	const char	*mapLoadName;
	const char	*imageName;
	const char	*opponentName;
	int			teamMembers;
	int			typeBits;
	int			cinematic;
	int			timeToBeat[GT_NUM_GAMETYPES];
	qhandle_t	levelShot;
	qboolean	active;
} mapInfo_t;

typedef struct serverFilter_s {
	const char	*description;
	const char	*basedir;
} serverFilter_t;

typedef struct pinglist_s {
	char	adrstr[MAX_ADDRESSLENGTH];
	int		start;
} pinglist_t;

typedef struct serverStatus_s {
	pinglist_t	pingList[MAX_PINGREQUESTS];
	int			numqueriedservers;
	int			currentping;
	int			nextpingtime;
	int			maxservers;
	int			refreshtime;
	int			numServers;
	int			sortKey;
	int			sortDir;
	int			lastCount;
	qboolean	refreshActive;
	int			currentServer;
	int			displayServers[MAX_DISPLAY_SERVERS];
	int			numDisplayServers;
	int			numPlayersOnServers;
	int			nextDisplayRefresh;
	int			nextSortTime;
	qhandle_t	currentServerPreview;
	int			currentServerCinematic;
	size_t		motdLen;
	int			motdWidth;
	float		motdPaintX;
	int			motdPaintX2;
	int			motdOffset;
	int			motdTime;
	char		motd[MAX_CVAR_VALUE_STRING];
} serverStatus_t;

typedef struct pendingServer_s {
	char		adrstr[MAX_ADDRESSLENGTH];
	char		name[MAX_ADDRESSLENGTH];
	int			startTime;
	int			serverNum;
	qboolean	valid;
} pendingServer_t;

typedef struct pendingServerStatus_s {
	int				num;
	pendingServer_t	server[MAX_SERVERSTATUSREQUESTS];
} pendingServerStatus_t;

typedef struct serverStatusInfo_s {
	char	address[MAX_ADDRESSLENGTH];
	char	*lines[MAX_SERVERSTATUS_LINES][4];
	char	text[MAX_SERVERSTATUS_TEXT];
	char	pings[MAX_CLIENTS*3];
	int		numLines;
} serverStatusInfo_t;

typedef struct modInfo_s {
	const char	*modName;
	const char	*modDescr;
} modInfo_t;

typedef struct uiInfo_s {
	displayContextDef_t		uiDC;
	int						newHighScoreTime;
	int						newBestTime;
	qboolean				newHighScore;
	qboolean				demoAvailable;
	qboolean				soundHighScore;
	
	int						characterCount;
	int						botIndex;
	characterInfo_t			characterList[MAX_HEADS];

	int						redBlue;
	int						playerCount;
	int						myTeamCount;
	int						teamIndex;
	int						playerRefresh;
	int						playerIndex;
	int						playerNumber; 
	char					playerNames[MAX_CLIENTS][MAX_NETNAME];
	char					teamNames[MAX_CLIENTS][MAX_TEAMNAME];
	int						teamClientNums[MAX_CLIENTS];

	int						mapCount;
	mapInfo_t				mapList[MAX_MAPS];

	int						skillIndex;

	modInfo_t				modList[MAX_MODS];
	int						modCount;
	int						modIndex;

	char					demoList[MAX_DEMOS][MAX_QPATH];
	int						demoCount;
	int						demoIndex;
	int						loadedDemos;

	const char				*movieList[MAX_MOVIES];
	int						movieCount;
	int						movieIndex;
	int						previewMovie;

	serverStatus_t			serverStatus;

	// for the showing the status of a server
	char					serverStatusAddress[MAX_ADDRESSLENGTH];
	serverStatusInfo_t		serverStatusInfo;
	int						nextServerStatusRefresh;

	// to retrieve the status of server to find a player
	pendingServerStatus_t	pendingServerStatus;
	char					findPlayerName[MAX_STRING_CHARS];
	char					foundPlayerServerAddresses[MAX_FOUNDPLAYER_SERVERS][MAX_ADDRESSLENGTH];
	char					foundPlayerServerNames[MAX_FOUNDPLAYER_SERVERS][MAX_ADDRESSLENGTH];
	int						currentFoundPlayerServer;
	int						numFoundPlayerServers;
	int						nextFindPlayerRefresh;

	int						currentCrosshair;
	sfxHandle_t				newHighScoreSound;

	int						q3HeadCount;
	char					q3HeadNames[MAX_PLAYERMODELS][64];
	qhandle_t				q3HeadIcons[MAX_PLAYERMODELS];
	int						q3SelectedHead;

	qboolean				inGameLoad;
} uiInfo_t;
extern uiInfo_t uiInfo;

// ui_atoms.c
qboolean		UI_ConsoleCommand( int realTime );
void			UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader );
void			UI_FillRect( float x, float y, float width, float height, const vector4 *color );
void			UI_SetColor( const vector4 *rgba );
void			UI_AdjustFrom640( float *x, float *y, float *w, float *h );
char			*UI_Cvar_VariableString( const char *name );

int UI_GetNumBots( void );
void UI_LoadBots( void );
char *UI_GetBotNameByNumber( int num );

extern uiImport_t *trap;
