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

//match template contexts
#define MTCONTEXT_MISC					2
#define MTCONTEXT_INITIALTEAMCHAT		4
#define MTCONTEXT_TIME					8
#define MTCONTEXT_TEAMMATE				16
#define MTCONTEXT_ADDRESSEE				32
#define MTCONTEXT_PATROLKEYAREA			64
#define MTCONTEXT_REPLYCHAT				128
#define MTCONTEXT_CTF					256

//message types
typedef enum botMsgType_e {
	MSG_NONE = 0,
	MSG_ENTERGAME,				//enter game message
	MSG_HELP,					//help someone
	MSG_ACCOMPANY,				//accompany someone
	MSG_DEFENDKEYAREA,			//defend a key area
	MSG_RUSHBASE,				//everyone rush to base
	MSG_GETFLAG,				//get the enemy flag
	MSG_WAIT,					//wait for someone
	MSG_WHATAREYOUDOING,		//what are you doing?
	MSG_JOINSUBTEAM,			//join a sub-team
	MSG_LEAVESUBTEAM,			//leave a sub-team
	MSG_CREATENEWFORMATION,		//create a new formation
	MSG_FORMATIONPOSITION,		//tell someone his/her position in a formation
	MSG_FORMATIONSPACE,			//set the formation intervening space
	MSG_DOFORMATION,			//form a known formation
	MSG_DISMISS,				//dismiss commanded team mates
	MSG_CAMP,					//camp somewhere
	MSG_CHECKPOINT,				//remember a check point
	MSG_PATROL,					//patrol between certain keypoints
	MSG_LEADTHEWAY,				//lead the way
	MSG_GETITEM,				//get an item
	MSG_KILL,					//kill someone
	MSG_WHEREAREYOU,			//where is someone
	MSG_RETURNFLAG,				//return the flag
	MSG_WHATISMYCOMMAND,		//ask the team leader what to do
	MSG_WHICHTEAM,				//ask which team a bot is in
	MSG_TASKPREFERENCE,			//tell your teamplay task preference
	MSG_ATTACKENEMYBASE,		//attack the enemy base
	MSG_SUICIDE,				//order to suicide

	MSG_ME = 100,
	MSG_EVERYONE,
	MSG_MULTIPLENAMES,
	MSG_NAME,
	MSG_PATROLKEYAREA,
	MSG_MINUTES,
	MSG_SECONDS,
	MSG_FOREVER,
	MSG_FORALONGTIME,
	MSG_FORAWHILE,

	MSG_CHATALL = 200,
	MSG_CHATTEAM,
	MSG_CHATTELL,

	MSG_CTF = 300,				//ctf message
} botMsgType_t;

//command sub types
#define ST_SOMEWHERE		0x00001
#define ST_NEARITEM			0x00002
#define ST_ADDRESSED		0x00004
#define ST_METER			0x00008
#define ST_FEET				0x00010
#define ST_TIME				0x00020
#define ST_HERE				0x00040
#define ST_THERE			0x00080
#define ST_I				0x00100
#define ST_MORE				0x00200
#define ST_BACK				0x00400
#define ST_REVERSE			0x00800
#define ST_SOMEONE			0x01000
#define ST_GOTFLAG			0x02000
#define ST_CAPTUREDFLAG		0x04000
#define ST_RETURNEDFLAG		0x08000
#define ST_TEAM				0x10000
#define ST_TROJANGOTFLAG	0x20000

//ctf task preferences
#define ST_DEFENDER			0x0001
#define ST_ATTACKER			0x0002
#define ST_ROAMER			0x0004

//word replacement variables
#define THE_ENEMY						7
#define THE_TEAM						7

//team message variables
#define NETNAME							0
#define PLACE							1
#define FLAG							1
#define MESSAGE							2
#define ADDRESSEE						2
#define ITEM							3
#define TEAMMATE						4
#define TEAMNAME						4
#define ENEMY							4
#define KEYAREA							5
#define FORMATION						5
#define POSITION						5
#define NUMBER							5
#define TIME							6
#define NAME							6
#define MORE							6


