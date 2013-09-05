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
// ui_qmenu.c
//

#define RCOLUMN_OFFSET			( BIGCHAR_WIDTH )
#define LCOLUMN_OFFSET			(-BIGCHAR_WIDTH )

#define SLIDER_RANGE			10
#define	MAX_EDIT_LINE			256

#define MTYPE_NULL				0
#define MTYPE_SLIDER			1	
#define MTYPE_ACTION			2
#define MTYPE_SPINCONTROL		3
#define MTYPE_FIELD				4
#define MTYPE_RADIOBUTTON		5
#define MTYPE_BITMAP			6	
#define MTYPE_TEXT				7
#define MTYPE_SCROLLLIST		8
#define MTYPE_PTEXT				9
#define MTYPE_BTEXT				10

#define QMF_BLINK				0x00000001
#define QMF_SMALLFONT			0x00000002
#define QMF_LEFT_JUSTIFY		0x00000004
#define QMF_CENTER_JUSTIFY		0x00000008
#define QMF_RIGHT_JUSTIFY		0x00000010
#define QMF_NUMBERSONLY			0x00000020	// edit field is only numbers
#define QMF_HIGHLIGHT			0x00000040
#define QMF_HIGHLIGHT_IF_FOCUS	0x00000080	// steady focus
#define QMF_PULSEIFFOCUS		0x00000100	// pulse if focus
#define QMF_HASMOUSEFOCUS		0x00000200
#define QMF_NOONOFFTEXT			0x00000400
#define QMF_MOUSEONLY			0x00000800	// only mouse input allowed
#define QMF_HIDDEN				0x00001000	// skips drawing
#define QMF_GRAYED				0x00002000	// grays and disables
#define QMF_INACTIVE			0x00004000	// disables any input
#define QMF_NODEFAULTINIT		0x00008000	// skip default initialization
#define QMF_OWNERDRAW			0x00010000
#define QMF_PULSE				0x00020000
#define QMF_LOWERCASE			0x00040000	// edit field is all lower case
#define QMF_UPPERCASE			0x00080000	// edit field is all upper case
#define QMF_SILENT				0x00100000

// callback notifications
#define QM_GOTFOCUS				1
#define QM_LOSTFOCUS			2
#define QM_ACTIVATED			3

typedef struct _tag_menuframework
{
	int	cursor;
	int cursor_prev;

	int	nitems;
	void *items[MAX_MENUITEMS];

	void (*draw) (void);
	sfxHandle_t (*key) (int key);

	qboolean	wrapAround;
	qboolean	fullscreen;
	qboolean	showlogo;
} menuframework_s;

typedef struct menucommon_s {
	int type;
	const char *name;
	int	id;
	int x, y;
	int left;
	int	top;
	int	right;
	int	bottom;
	menuframework_s *parent;
	int menuPosition;
	unsigned flags;

	void (*callback)( void *self, int event );
	void (*statusbar)( void *self );
	void (*ownerdraw)( void *self );
} menucommon_t;

typedef struct mfield_s {
	int		cursor;
	int		scroll;
	int		widthInChars;
	char	buffer[MAX_EDIT_LINE];
	int		maxchars;
} mfield_t;

typedef struct menufield_t {
	menucommon_t	generic;
	mfield_t		field;
} menufield_t;

typedef struct menuslider_s {
	menucommon_t generic;

	float minvalue;
	float maxvalue;
	float curvalue;

	float range;
} menuslider_t;

typedef struct menulist_s {
	menucommon_t generic;

	int	oldvalue;
	int curvalue;
	int	numitems;
	int	top;
		
	const char **itemnames;

	int width;
	int height;
	int	columns;
	int	seperation;
} menulist_t;

typedef struct menuaction_s {
	menucommon_t generic;
} menuaction_t;

typedef struct menuradiobutton_s {
	menucommon_t generic;
	int curvalue;
} menuradiobutton_t;

typedef struct menubitmap_s {
	menucommon_t	generic;
	char*			focuspic;	
	char*			errorpic;
	qhandle_t		shader;
	qhandle_t		focusshader;
	int				width;
	int				height;
	float*			focuscolor;
} menubitmap_t;

typedef struct menutext_s {
	menucommon_t	generic;
	char*			string;
	int				style;
	float*			color;
} menutext_t;

extern void			Menu_Cache( void );
extern void			Menu_Focus( menucommon_t *m );
extern void			Menu_AddItem( menuframework_s *menu, void *item );
extern void			Menu_AdjustCursor( menuframework_s *menu, int dir );
extern void			Menu_Draw( menuframework_s *menu );
extern void			*Menu_ItemAtCursor( menuframework_s *m );
extern sfxHandle_t	Menu_ActivateItem( menuframework_s *s, menucommon_t *item );
extern void			Menu_SetCursor( menuframework_s *s, int cursor );
extern void			Menu_SetCursorToItem( menuframework_s *m, void* ptr );
extern sfxHandle_t	Menu_DefaultKey( menuframework_s *s, int key );
extern void			Bitmap_Init( menubitmap_t *b );
extern void			Bitmap_Draw( menubitmap_t *b );
extern void			ScrollList_Draw( menulist_t *l );
extern sfxHandle_t	ScrollList_Key( menulist_t *l, int key );
extern sfxHandle_t	menu_in_sound;
extern sfxHandle_t	menu_move_sound;
extern sfxHandle_t	menu_out_sound;
extern sfxHandle_t	menu_buzz_sound;
extern sfxHandle_t	menu_null_sound;
extern sfxHandle_t	weaponChangeSound;
extern vector4		menu_text_color;
extern vector4		menu_grayed_color;
extern vector4		menu_dark_color;
extern vector4		menu_highlight_color;
extern vector4		menu_red_color;
extern vector4		menu_black_color;
extern vector4		menu_dim_color;
extern vector4		color_black;
extern vector4		color_white;
extern vector4		color_yellow;
extern vector4		color_blue;
extern vector4		color_orange;
extern vector4		color_red;
extern vector4		color_dim;
extern vector4		name_color;
extern vector4		list_color;
extern vector4		listbar_color;
extern vector4		text_color_disabled; 
extern vector4		text_color_normal;
extern vector4		text_color_highlight;

extern char	*ui_medalNames[];
extern char	*ui_medalPicNames[];
extern char	*ui_medalSounds[];

//
// ui_mfield.c
//
extern void			MField_Clear( mfield_t *edit );
extern void			MField_KeyDownEvent( mfield_t *edit, int key );
extern void			MField_CharEvent( mfield_t *edit, int ch );
extern void			MField_Draw( mfield_t *edit, int x, int y, int style, vector4 *color );
extern void			MenuField_Init( menufield_t *m );
extern void			MenuField_Draw( menufield_t *f );
extern sfxHandle_t	MenuField_Key( menufield_t *m, int *key );

//
// ui_main.c
//
void UI_Report( void );
void UI_Load( void );
void UI_LoadMenus(const char *menuFile, qboolean reset);
void UI_SetActiveMenu( uiMenuCommand_t menu );
int UI_AdjustTimeByGame(int time);
void UI_ShowPostGame(qboolean newHigh);
void UI_LoadArenas(void);

//
// ui_menu.c
//
extern void MainMenu_Cache( void );
extern void UI_MainMenu(void);
extern void UI_RegisterCvars( void );

//
// ui_credits.c
//
extern void UI_CreditMenu( void );

//
// ui_ingame.c
//
extern void InGame_Cache( void );
extern void UI_InGameMenu(void);

//
// ui_confirm.c
//
extern void ConfirmMenu_Cache( void );
extern void UI_ConfirmMenu( const char *question, void (*draw)( void ), void (*action)( qboolean result ) );

//
// ui_setup.c
//
extern void UI_SetupMenu_Cache( void );
extern void UI_SetupMenu(void);

//
// ui_team.c
//
extern void UI_TeamMainMenu( void );
extern void TeamMain_Cache( void );

//
// ui_connect.c
//
extern void UI_DrawConnectScreen( qboolean overlay );

//
// ui_controls2.c
//
extern void UI_ControlsMenu( void );
extern void Controls_Cache( void );

//
// ui_demo2.c
//
extern void UI_DemosMenu( void );
extern void Demos_Cache( void );

//
// ui_cinematics.c
//
extern void UI_CinematicsMenu( void );
extern void UI_CinematicsMenu_f( void );
extern void UI_CinematicsMenu_Cache( void );

//
// ui_mods.c
//
extern void UI_ModsMenu( void );
extern void UI_ModsMenu_Cache( void );

//
// ui_playermodel.c
//
extern void UI_PlayerModelMenu( void );
extern void PlayerModel_Cache( void );

//
// ui_playersettings.c
//
extern void UI_PlayerSettingsMenu( void );
extern void PlayerSettings_Cache( void );

//
// ui_preferences.c
//
extern void UI_PreferencesMenu( void );
extern void Preferences_Cache( void );

//
// ui_specifyleague.c
//
extern void UI_SpecifyLeagueMenu( void );
extern void SpecifyLeague_Cache( void );

//
// ui_specifyserver.c
//
extern void UI_SpecifyServerMenu( void );
extern void SpecifyServer_Cache( void );

//
// ui_servers2.c
//
#define MAX_FAVORITESERVERS 16

extern void UI_ArenaServersMenu( void );
extern void ArenaServers_Cache( void );

//
// ui_startserver.c
//
extern void UI_StartServerMenu( qboolean multiplayer );
extern void StartServer_Cache( void );
extern void ServerOptions_Cache( void );
extern void UI_BotSelectMenu( char *bot );
extern void UI_BotSelectMenu_Cache( void );

//
// ui_serverinfo.c
//
extern void UI_ServerInfoMenu( void );
extern void ServerInfo_Cache( void );

//
// ui_video.c
//
extern void UI_GraphicsOptionsMenu( void );
extern void GraphicsOptions_Cache( void );
extern void DriverInfo_Cache( void );

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
// this is only used in the old ui, the new ui has its own version
typedef struct uiStatic_s {
	int					frametime;
	int					realtime;
	int					cursorx;
	int					cursory;
	glconfig_t 	glconfig;
	qboolean		debug;
	qhandle_t		whiteShader;
	qhandle_t		menuBackShader;
	qhandle_t		menuBackNoLogoShader;
	qhandle_t		charset;
	qhandle_t		charsetProp;
	qhandle_t		charsetPropGlow;
	qhandle_t		charsetPropB;
	qhandle_t		cursor;
	qhandle_t		rb_on;
	qhandle_t		rb_off;
	float				scale;
	float				bias;
	qboolean		demoversion;
	qboolean		firstdraw;
} uiStatic_t;


// new ui stuff
#define UI_NUMFX 7
#define MAX_HEADS 256
#define MAX_HEADNAME  32
#define MAX_MAPS 512
#define MAX_SPMAPS 16
#define PLAYERS_PER_TEAM 5
#define MAX_PINGREQUESTS		32
#define MAX_ADDRESSLENGTH		64
#define MAX_HOSTNAMELENGTH		22
#define MAX_MAPNAMELENGTH		16
#define MAX_STATUSLENGTH		64
#define MAX_LISTBOXWIDTH		59
#define UI_FONT_THRESHOLD		0.1
#define MAX_DISPLAY_SERVERS		2048
#define MAX_SERVERSTATUS_LINES	128
#define MAX_SERVERSTATUS_TEXT	2048 //1024
#define MAX_FOUNDPLAYER_SERVERS	16
#define GAMES_ALL			0
#define GAMES_FFA			1
#define GAMES_TEAMPLAY		2
#define GAMES_TOURNEY		3
#define GAMES_CTF			4
#define MAX_MODS 64
#define MAX_DEMOS 2048 //512
#define MAX_MOVIES 256
#define MAX_PLAYERMODELS 1024 //256


typedef struct characterInfo_s {
	const char	*name;
	const char	*imageName;
	qhandle_t	headImage;
	const char	*base;
	qboolean	active;
	int			reference;
} characterInfo_t;

typedef struct aliasInfo_s {
	const char	*name;
	const char	*ai;
	const char	*action;
} aliasInfo_t;

typedef struct gameTypeInfo_s {
	const char	*gameType;
	int			gtEnum;
} gameTypeInfo_t;

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
	const char *description;
	const char *basedir;
} serverFilter_t;

typedef struct pinglist_s {
	char	adrstr[MAX_ADDRESSLENGTH];
	int		start;
} pinglist_t;


typedef struct serverStatus_s {
	pinglist_t pingList[MAX_PINGREQUESTS];
	int		numqueriedservers;
	int		currentping;
	int		nextpingtime;
	int		maxservers;
	int		refreshtime;
	int		numServers;
	int		sortKey;
	int		sortDir;
	int		lastCount;
	qboolean refreshActive;
	int		currentServer;
	int		displayServers[MAX_DISPLAY_SERVERS];
	int		numDisplayServers;
	int		numPlayersOnServers;
	int		nextDisplayRefresh;
	int		nextSortTime;
	qhandle_t currentServerPreview;
	int		currentServerCinematic;
	int		motdLen;
	int		motdWidth;
	int		motdPaintX;
	int		motdPaintX2;
	int		motdOffset;
	int		motdTime;
	char	motd[MAX_STRING_CHARS];
} serverStatus_t;


typedef struct pendingServer_s {
	char		adrstr[MAX_ADDRESSLENGTH];
	char		name[MAX_ADDRESSLENGTH];
	int			startTime;
	int			serverNum;
	qboolean	valid;
} pendingServer_t;

typedef struct pendingServerStatus_s {
	int num;
	pendingServer_t server[MAX_SERVERSTATUSREQUESTS];
} pendingServerStatus_t;

typedef struct serverStatusInfo_s {
	char address[MAX_ADDRESSLENGTH];
	char *lines[MAX_SERVERSTATUS_LINES][4];
	char text[MAX_SERVERSTATUS_TEXT];
	char pings[MAX_CLIENTS * 3];
	int numLines;
} serverStatusInfo_t;

typedef struct modInfo_s {
	const char *modName;
	const char *modDescr;
} modInfo_t;


typedef struct uiInfo_s {
	displayContextDef_t		uiDC;
	int						newHighScoreTime;
	int						newBestTime;
	int						showPostGameTime;
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
	char					teamNames[MAX_CLIENTS][MAX_NETNAME];
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
	int						startPostGameTime;
	sfxHandle_t				newHighScoreSound;

	int						q3HeadCount;
	char					q3HeadNames[MAX_PLAYERMODELS][64];
	qhandle_t				q3HeadIcons[MAX_PLAYERMODELS];
	int						q3SelectedHead;

	qboolean				inGameLoad;
} uiInfo_t;

extern uiInfo_t uiInfo;

extern void			UI_Init( qboolean inGameLoad );
extern void			UI_Shutdown( void );
extern void			UI_KeyEvent( int key, qboolean down );
extern void			UI_MouseEvent( int dx, int dy );
extern void			UI_Refresh( int realtime );
extern qboolean		UI_ConsoleCommand( int realTime );
extern float		UI_ClampCvar( float min, float max, float value );
extern void			UI_DrawNamedPic( float x, float y, float width, float height, const char *picname );
extern void			UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ); 
extern void			UI_FillRect( float x, float y, float width, float height, const vector4 *color );
extern void			UI_DrawRect( float x, float y, float width, float height, const vector4 *color );
extern void     UI_DrawTopBottom(float x, float y, float w, float h);
extern void     UI_DrawSides(float x, float y, float w, float h);
extern void			UI_UpdateScreen( void );
extern void			UI_SetColor( const vector4 *rgba );
extern void			UI_LerpColor(vector4 *a, vector4 *b, vector4 *c, float t);
extern void			UI_DrawBannerString( int x, int y, const char* str, int style, vector4 *color );
extern float		UI_ProportionalSizeScale( int style );
extern void			UI_DrawProportionalString( int x, int y, const char* str, int style, vector4 *color );
extern int			UI_ProportionalStringWidth( const char* str );
extern void			UI_DrawString( int x, int y, const char* str, int style, vector4 *color );
extern void			UI_DrawChar( int x, int y, int ch, int style, vector4 *color );
extern qboolean 	UI_CursorInRect (int x, int y, int width, int height);
extern void			UI_AdjustFrom640( float *x, float *y, float *w, float *h );
extern void			UI_DrawTextBox (int x, int y, int width, int lines);
extern qboolean		UI_IsFullscreen( void );
extern void			UI_SetActiveMenu( uiMenuCommand_t menu );
extern void			UI_PushMenu ( menuframework_s *menu );
extern void			UI_PopMenu (void);
extern void			UI_ForceMenuOff (void);
extern char			*UI_Argv( int arg );
extern char			*UI_Cvar_VariableString( const char *name );
extern void			UI_Refresh( int time );
extern void			UI_StartDemoLoop( void );
extern qboolean		m_entersound;
extern uiStatic_t	uis;

//
// ui_gameinfo.c
//
typedef enum awardType_e {
	AWARD_ACCURACY=0,
	AWARD_IMPRESSIVE,
	AWARD_EXCELLENT,
	AWARD_GAUNTLET,
	AWARD_FRAGS,
	AWARD_PERFECT
} awardType_t;

const char *UI_GetArenaInfoByNumber( int num );
const char *UI_GetArenaInfoByMap( const char *map );
const char *UI_GetSpecialArenaInfo( const char *tag );
int UI_GetNumArenas( void );

char *UI_GetBotInfoByNumber( int num );
char *UI_GetBotInfoByName( const char *name );
int UI_GetNumBots( void );
void UI_LoadBots( void );
char *UI_GetBotNameByNumber( int num );

int  UI_GetCurrentGame( void );
void UI_LogAwardData( int award, int data );
int UI_GetAwardLevel( int award );

void UI_SPUnlock_f( void );
void UI_SPUnlockMedals_f( void );

void UI_InitGameinfo( void );

// new ui 

#define ASSET_BACKGROUND "uiBackground"

// for tracking sp game info in Team Arena
typedef struct postGameInfo_s {
	int score;
	int redScore;
	int blueScore;
	int perfects;
	int accuracy;
	int impressives;
	int excellents;
	int defends;
	int assists;
	int gauntlets;
	int	captures;
	int time;
	int timeBonus;
	int shutoutBonus;
	int skillBonus;
	int baseScore;
} postGameInfo_t;

extern uiImport_t *trap;
