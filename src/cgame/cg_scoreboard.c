#include "cg_local.h"

void DrawServerInformation( float fade ) {
	float fontScale = 0.33333f, y = 20.0f, lineHeight=14.0f;
	const char *tmp = NULL;
	vector4 colour = { 1.0f, 1.0f, 1.0f, 1.0f };
	colour.a = fade;

	tmp = cgs.server.hostname;
	CG_Text_Paint( SCREEN_WIDTH/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
	y += lineHeight;

	tmp = va( "%s (%s)", (char *)CG_ConfigString( CS_MESSAGE ), cgs.mapname );
	CG_Text_Paint( SCREEN_WIDTH/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
	y += lineHeight;
	tmp = GametypeStringForID( cgs.gametype );
	CG_Text_Paint( SCREEN_WIDTH/2.0f- CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
	y += lineHeight;

	switch ( cgs.gametype ) {
	case GT_BLOODBATH:
	case GT_DUEL:
		if ( cgs.timelimit && cgs.fraglimit )	tmp = va( "Until "S_COLOR_YELLOW"%i "S_COLOR_WHITE"frags or "S_COLOR_YELLOW"%i"S_COLOR_WHITE"/"S_COLOR_YELLOW"%i minutes", cgs.fraglimit, (cg.time-cgs.levelStartTime)/60000, cgs.timelimit );
		else if ( cgs.timelimit )				tmp = va( "Until "S_COLOR_YELLOW"%i"S_COLOR_WHITE"/"S_COLOR_YELLOW"%i "S_COLOR_WHITE"minutes", (cg.time-cgs.levelStartTime)/60000, cgs.timelimit );
		else if ( cgs.fraglimit )				tmp = va( "Until "S_COLOR_YELLOW"%i "S_COLOR_WHITE"frags", cgs.fraglimit );
		else									tmp = "Playing forever!";
		CG_Text_Paint( SCREEN_WIDTH/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
		y += lineHeight*2;
		break;
	case GT_FLAGS:
	case GT_TROJAN:
		if ( cgs.timelimit && cgs.capturelimit )	tmp = va( "Until "S_COLOR_YELLOW"%i"S_COLOR_WHITE"/"S_COLOR_YELLOW"%i "S_COLOR_WHITE"captures or "S_COLOR_YELLOW"%i"S_COLOR_WHITE"/"S_COLOR_YELLOW"%i minutes", MAX(cgs.scores1, cgs.scores2), cgs.capturelimit, (cg.time-cgs.levelStartTime)/60000, cgs.timelimit );
		else if ( cgs.timelimit )					tmp = va( "Until "S_COLOR_YELLOW"%i"S_COLOR_WHITE"/"S_COLOR_YELLOW"%i "S_COLOR_WHITE"minutes", (cg.time-cgs.levelStartTime)/60000, cgs.timelimit );
		else if ( cgs.capturelimit )				tmp = va( "Until "S_COLOR_YELLOW"%i"S_COLOR_WHITE"/"S_COLOR_YELLOW"%i "S_COLOR_WHITE"captures", MAX(cgs.scores1, cgs.scores2), cgs.capturelimit );
		else										tmp = "Playing forever!";
		CG_Text_Paint( SCREEN_WIDTH/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
		y += lineHeight*2;
		//FALL THROUGH TO GENERIC TEAM GAME INFO!
	case GT_TEAMBLOOD:
		if ( cgs.scores1 == cgs.scores2 ) {
			tmp = S_COLOR_YELLOW"Teams are tied";
			CG_Text_Paint( SCREEN_WIDTH/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
			y += lineHeight;
			tmp = va( S_COLOR_RED"%i "S_COLOR_WHITE"/ "S_COLOR_CYAN"%i", cgs.scores1, cgs.scores2 );
			CG_Text_Paint( SCREEN_WIDTH/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
			y += lineHeight;
		}
		else if ( cgs.scores1 > cgs.scores2 ) {
			tmp = S_COLOR_RED"Red "S_COLOR_WHITE"leads";
			CG_Text_Paint( SCREEN_WIDTH/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
			y += lineHeight;
			tmp = va( S_COLOR_RED"%i "S_COLOR_WHITE"/ "S_COLOR_CYAN"%i", cgs.scores1, cgs.scores2 );
			CG_Text_Paint( SCREEN_WIDTH/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
			y += lineHeight;
		}
		else {
			tmp = S_COLOR_CYAN"Blue "S_COLOR_WHITE"leads";
			CG_Text_Paint( SCREEN_WIDTH/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
			y += lineHeight;
			tmp = va( S_COLOR_CYAN"%i "S_COLOR_WHITE"/ "S_COLOR_RED"%i", cgs.scores2, cgs.scores1 );
			CG_Text_Paint( SCREEN_WIDTH/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
			y += lineHeight;
		}
		break;
	default:
		break;
	}
}





void DrawPlayerCount_Free( float fade ) {
	float fontScale = 0.33333f, y = 108.0f;
	const char *tmp = NULL;
	vector4 colour = { 1.0f, 1.0f, 1.0f, 1.0f };
	int i = 0, freeCount = 0, specCount = 0, botCount = 0;
	float width = SCREEN_WIDTH/2.0f, lineHeight = 14.0f;

	colour.a = fade;

	for ( i=0; i<cg.numScores; i++ ) {
		clientInfo_t *ci = &cgs.clientinfo[cg.scores[i].client];
		if ( ci->team == TEAM_FREE )
			freeCount++;
		else if ( ci->team == TEAM_SPECTATOR )
			specCount++;
		if ( ci->botSkill != -1 )
			botCount++;
	}

	if ( botCount )
		tmp = va( "%i players / %i bots", freeCount-botCount, botCount );
	else
		tmp = va( "%i players", freeCount );
	CG_Text_Paint( width/*/2.0f*/ - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
	tmp = va( "%i spectators", specCount );
	CG_Text_Paint( width/* + width/2.0f*/ - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + lineHeight*20, fontScale, &colour, tmp, 0.0f, 0, 0 );
}

void DrawPlayerCount_Team( float fade ) {
	float fontScale = 0.33333f, y = 108.0f;
	const char *tmp = NULL;
	vector4 colour = { 1.0f, 1.0f, 1.0f, 1.0f };
	int i = 0, redCount = 0, blueCount = 0, specCount = 0;
	int pingAccumRed = 0, pingAvgRed = 0, pingAccumBlue=0, pingAvgBlue=0;
	float width = SCREEN_WIDTH/2.0f, lineHeight=14.0f;

	colour.a = fade;

	for ( i=0; i<cg.numScores; i++ ) {
		if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_RED ) {
			pingAccumRed += cg.scores[i].ping;
			redCount++;
		}
		else if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_BLUE ) {
			blueCount++;
			pingAccumBlue += cg.scores[i].ping;
		}
		else if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_SPECTATOR )
			specCount++;
	}

	if ( redCount )
		pingAvgRed = pingAccumRed / redCount;
	if ( blueCount )
		pingAvgBlue = pingAccumBlue / blueCount;

	if ( cgs.scores1 >= cgs.scores2 ) {
		tmp = va( S_COLOR_RED"%2i players", redCount );
		CG_Text_Paint( width/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
		tmp = va( "%3i avg ping", pingAvgRed );
		CG_Text_Paint( width/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y+lineHeight, fontScale, &colour, tmp, 0.0f, 0, 0 );

		tmp = va( S_COLOR_CYAN"%2i players", blueCount );
		CG_Text_Paint( width + width/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
		tmp = va( "%3i avg ping", pingAvgBlue );
		CG_Text_Paint( width + width/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y+lineHeight, fontScale, &colour, tmp, 0.0f, 0, 0 );
	}
	else {
		tmp = va( S_COLOR_CYAN"%2i players", blueCount );
		CG_Text_Paint( width/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
		tmp = va( "%3i avg ping", pingAvgBlue );
		CG_Text_Paint( width/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y+lineHeight, fontScale, &colour, tmp, 0.0f, 0, 0 );

		tmp = va( S_COLOR_RED"%2i players", redCount );
		CG_Text_Paint( width + width/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y, fontScale, &colour, tmp, 0.0f, 0, 0 );
		tmp = va( "%3i avg ping", pingAvgRed );
		CG_Text_Paint( width + width/2.0f - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y+lineHeight, fontScale, &colour, tmp, 0.0f, 0, 0 );
	}

	tmp = va( "%2i spectators", specCount );
	CG_Text_Paint( width/* + width/2.0f*/ - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + lineHeight*20, fontScale, &colour, tmp, 0.0f, 0, 0 );
}

void DrawPlayerCount( float fade ) {
	switch ( cgs.gametype ) {
	case GT_BLOODBATH:
	case GT_DUEL:
		DrawPlayerCount_Free( fade );
		break;

	case GT_TEAMBLOOD:
	case GT_FLAGS:
	case GT_TROJAN:
		DrawPlayerCount_Team( fade );
		break;
	}
}

int PlayerCount( team_t team ) {
	int i=0, count=0;

	for ( i=0; i<cg.numScores; i++ ) {
		clientInfo_t *ci = &cgs.clientinfo[cg.scores[i].client];
		if ( ci->team == team )
			count++;
	}

	return count;
}





//Return value: number of players on team 'team'
int ListPlayers_FFA( float fade, float x, float y, float fontScale, float lineHeight, int startIndex, int playerCount ) {
	const char *tmp = NULL;
	vector4	white		= { 1.0f, 1.0f, 1.0f, 1.0f },
			background	= { 0.75f, 0.75f, 0.75f, 1.0f },
			blue		= { 0.6f, 0.6f, 1.0f, 1.0f };
	int i = 0, count = playerCount, column = 0;
	float endX = SCREEN_WIDTH/2.0f, columnOffset[] = { /*name*/80.0f, /*score*/170.0f, /*ping*/270.0f, /*time*/295.0f }, savedY=0.0f;

	white.a = fade;
	background.a = 0.6f*fade;
	blue.a = fade;

	if ( !count )
		return 0;

	trap->R_SetColor( &background );
		CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f*2 + 5.0f, lineHeight*1.5f, cgs.media.scoreboard.line );
	trap->R_SetColor( NULL );

	tmp = "Name";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "Score";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "Ping";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "Time";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
	y += lineHeight*1.5f;

	savedY = y;

	// First pass, background + borders
	for ( i=startIndex; i<startIndex+playerCount; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == TEAM_FREE ) {
			column = 0;

			// background
			if ( ci-cgs.clientinfo == cg.snap->ps.clientNum )
				trap->R_SetColor( &blue );
			else
				trap->R_SetColor( &background );

			CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f*2 + 5.0f, lineHeight, cgs.media.scoreboard.line );
			trap->R_SetColor( NULL );

			y += lineHeight;
		}
	}
	y = savedY;

	// Second pass, text + icons
	for ( i=startIndex; i<startIndex+playerCount; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == TEAM_FREE ) {
			vector4	pingColour	= { 1.0f, 1.0f, 1.0f, 1.0f },
					pingGood	= { 0.0f, 1.0f, 0.0f, 1.0f },
					pingBad		= { 1.0f, 0.0f, 0.0f, 1.0f };
			pingColour.a = pingGood.a = pingBad.a = fade;
			VectorLerp4( &pingGood, MIN( score->ping / 300.0f, 1.0f ), &pingBad, &pingColour );

			column = 0;

			if ( score->ping >= 999 || (cg_entities[score->client].currentState.eFlags&EF_CONNECTION) ) {
				trap->R_SetColor( &white );
					CG_DrawPic( x+5.0f, y+1.0f, lineHeight-2.0f, lineHeight-2.0f, cgs.media.connectionShader );
				trap->R_SetColor( NULL );
			}

			else if ( ci->botSkill != -1 ) {
				tmp = "BOT";
				CG_Text_Paint( x + 8.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
			}

			else if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1<<score->client) ) {
				tmp = "READY";
				CG_Text_Paint( x + 8.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
			}

			//Name
			tmp = ci->name;
			CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

			//Score
			if ( score->ping == -1 )
				tmp = "Connecting";
		//	else if ( team == TEAM_SPECTATOR )
		//		tmp = "Spectating"; //TODO: Name of client they're spectating? possible?
			else
				tmp = va( "%02i", score->score );

			CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

			if ( score->ping != -1 ) {
				int msec, secs, mins;

				//Ping
				if ( ci->botSkill != -1 ) {
					tmp = "--";
					CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
				}
				else {
					tmp = va( "%i", score->ping );
					CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &pingColour, tmp, 0.0f, 0, 0 );
				}

				//Time
				msec = cg.time - cgs.levelStartTime - score->time;
				secs = msec/1000;
				mins = secs/60;
				secs %= 60;
				msec %= 1000;

				tmp = va( "%i:%02i", mins, secs );
				CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
			}

			y += lineHeight;
		}
	}

	return count;
}

//Return value: number of players on team 'team'
int ListPlayers_TDM( float fade, float x, float y, float fontScale, float lineHeight, team_t team ) {
	const char *tmp = NULL;
	vector4	white		= { 1.0f, 1.0f, 1.0f, 1.0f },
			border		= { 0.6f, 0.6f, 0.6f, 1.0f },
			borderRed	= { 1.0f, 0.8f, 0.8f, 1.0f },
			borderBlue	= { 1.0f, 0.8f, 0.8f, 1.0f },
			blue		= { 0.6f, 0.6f, 1.0f, 1.0f },
			background	= { 0.75f, 0.75f, 0.75f, 1.0f },
			teamRed		= { 0.7f, 0.4f, 0.4f, 1.0f },
			teamBlue	= { 0.4f, 0.4f, 0.7f, 1.0f };
	vector4	*teamBackground	= &background,
			*teamBorder		= &border;
	int i = 0, count = 0, column = 0;
	float endX = SCREEN_WIDTH/2.0f, columnOffset[] = { /*name*/80.0f, /*score*/170.0f, /*capture*/195.0f, /*defend*/220.f, /*assist*/245.f, /*ping*/270.0f, /*time*/295.0f }, savedY=0.0f;

	white.a = border.a = borderRed.a = borderBlue.a = blue.a = fade;
	background.a = teamRed.a = teamBlue.a = 0.6f * fade;

	if ( team == TEAM_RED ) {
		teamBackground = &teamRed;
		teamBorder = &borderRed;
	}
	else if ( team == TEAM_BLUE ) {
		teamBackground = &teamBlue;
		teamBorder = &borderBlue;
	}

	for ( i=0; i<cg.numScores; i++ ) {
		if ( cgs.clientinfo[cg.scores[i].client].team == team )
			count++;
	}

	if ( !count )
		return 0;

	trap->R_SetColor( teamBackground );
		CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f*2 + 5.0f, lineHeight*1.5f, cgs.media.scoreboard.line );
	trap->R_SetColor( NULL );

	tmp = "Name";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "Score";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "Cap";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "Def";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "Asst";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "Ping";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "Time";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
	y += lineHeight*1.5f;

	savedY = y;

	// First pass, background + borders
	for ( i=0; i<cg.numScores; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == team ) {
			column = 0;

			// background
			if ( ci-cgs.clientinfo == cg.snap->ps.clientNum )
				trap->R_SetColor( &blue );
			else
				trap->R_SetColor( teamBackground );

			CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f*2 + 5.0f, lineHeight, cgs.media.scoreboard.line );
			trap->R_SetColor( NULL );

			y += lineHeight;
		}
	}
	y = savedY;

	// Second pass, text + icons
	for ( i=0; i<cg.numScores; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == team ) {
			vector4	pingColour	= { 1.0f, 1.0f, 1.0f, 1.0f },
					pingGood	= { 0.0f, 1.0f, 0.0f, 1.0f },
					pingBad		= { 1.0f, 0.0f, 0.0f, 1.0f };
			pingColour.a = pingGood.a = pingBad.a = fade;
			VectorLerp4( &pingGood, MIN( score->ping / 300.0f, 1.0f ), &pingBad, &pingColour );

			column = 0;

			if ( score->ping >= 999 || (cg_entities[score->client].currentState.eFlags&EF_CONNECTION) ) {
				trap->R_SetColor( &white );
					CG_DrawPic( x+5.0f, y+1.0f, lineHeight-2.0f, lineHeight-2.0f, cgs.media.connectionShader );
				trap->R_SetColor( NULL );
			}

			else if ( ci->botSkill != -1 ) {
				tmp = "BOT";
				CG_Text_Paint( x + 8.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
			}

			else if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1<<score->client) ) {
				tmp = "READY";
				CG_Text_Paint( x + 8.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
			}

			//Name
			tmp = ci->name;
			CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

			//Score
			tmp = va( "%4i", score->score );
			CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

			//Capture
			tmp = va( "%2i", score->captures );
			CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

			//Defend
			tmp = va( "%2i", score->defendCount );
			CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

			//Assist
			tmp = va( "%2i", score->assistCount );
			CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

			if ( score->ping != -1 ) {
				int msec, secs, mins;

				//Ping
				if ( ci->botSkill != -1 ) {
					tmp = "--";
					CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
				}
				else {
					tmp = va( "%i", score->ping );
					CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &pingColour, tmp, 0.0f, 0, 0 );
				}

				//Time
				msec = cg.time - cgs.levelStartTime - score->time;
				secs = msec/1000;
				mins = secs/60;
				secs %= 60;
				msec %= 1000;

				tmp = va( "%i:%02i", mins, secs );
				CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
			}

			y += lineHeight;
		}
	}

	return count;
}

//Return value: number of players on team 'team'
int ListPlayers_Flags( float fade, float x, float y, float fontScale, float lineHeight, team_t team ) {
	const char *tmp = NULL;
	vector4	white		= { 1.0f, 1.0f, 1.0f, 1.0f },
			border		= { 0.6f, 0.6f, 0.6f, 1.0f },
			borderRed	= { 1.0f, 0.8f, 0.8f, 1.0f },
			borderBlue	= { 1.0f, 0.8f, 0.8f, 1.0f },
			blue		= { 0.6f, 0.6f, 1.0f, 1.0f },
			background	= { 0.75f, 0.75f, 0.75f, 1.0f },
			teamRed		= { 0.7f, 0.4f, 0.4f, 1.0f },
			teamBlue	= { 0.4f, 0.4f, 0.7f, 1.0f };
	vector4	*teamBackground	= &background,
			*teamBorder		= &border;
	int i = 0, count = 0, column = 0;
	float endX = SCREEN_WIDTH/2.0f, columnOffset[] = { /*name*/80.0f, /*score*/170.0f, /*capture*/195.0f, /*defend*/220.f, /*assist*/245.f, /*ping*/270.0f, /*time*/295.0f }, savedY=0.0f;

	white.a = border.a = borderRed.a = borderBlue.a = blue.a = fade;
	background.a = teamRed.a = teamBlue.a = 0.6f*fade;

	if ( team == TEAM_RED ) {
		teamBackground = &teamRed;
		teamBorder = &borderRed;
	}
	else if ( team == TEAM_BLUE ) {
		teamBackground = &teamBlue;
		teamBorder = &borderBlue;
	}

	for ( i=0; i<cg.numScores; i++ ) {
		if ( cgs.clientinfo[cg.scores[i].client].team == team )
			count++;
	}

	if ( !count )
		return 0;

	trap->R_SetColor( teamBackground );
		CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f*2 + 5.0f, lineHeight*1.5f, cgs.media.scoreboard.line );
	trap->R_SetColor( NULL );

	tmp = "Name";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "S";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "C";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "D";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "A";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "P";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

	tmp = "T";
	CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
	y += lineHeight*1.5f;

	savedY = y;

	// First pass, background + borders
	for ( i=0; i<cg.numScores; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == team ) {
			column = 0;

			// background
			if ( ci-cgs.clientinfo == cg.snap->ps.clientNum )
				trap->R_SetColor( &blue );
			else
				trap->R_SetColor( teamBackground );

			CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f*2 + 5.0f, lineHeight, cgs.media.scoreboard.line );
			trap->R_SetColor( NULL );

			y += lineHeight;
		}
	}
	y = savedY;

	// Second pass, text + icons
	for ( i=0; i<cg.numScores; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == team ) {
			vector4	pingColour	= { 1.0f, 1.0f, 1.0f, 1.0f },
					pingGood	= { 0.0f, 1.0f, 0.0f, 1.0f },
					pingBad		= { 1.0f, 0.0f, 0.0f, 1.0f };
			pingColour.a = pingGood.a = pingBad.a = fade;
			VectorLerp4( &pingGood, MIN( score->ping / 300.0f, 1.0f ), &pingBad, &pingColour );

			column = 0;

			if ( ci->powerups & ((1<<PW_BLUEFLAG)|(1<<PW_REDFLAG)) ) {
				trap->R_SetColor( &white );
					CG_DrawPic( x+5.0f, y, lineHeight, lineHeight, cgs.media.flagShaders[FLAG_TAKEN] );
				trap->R_SetColor( NULL );
			}

			else if ( score->ping >= 999 || (cg_entities[score->client].currentState.eFlags&EF_CONNECTION) ) {
				trap->R_SetColor( &white );
					CG_DrawPic( x+5.0f, y+1.0f, lineHeight-2.0f, lineHeight-2.0f, cgs.media.connectionShader );
				trap->R_SetColor( NULL );
			}

			else if ( ci->botSkill != -1 ) {
				tmp = "BOT";
				CG_Text_Paint( x + 8.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
			}

			else if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1<<score->client) ) {
				tmp = "READY";
				CG_Text_Paint( x + 8.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
			}

			//Name
			tmp = ci->name;
			CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

			//Score
			tmp = va( "%i", score->score );
			CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

			//Capture
			tmp = va( "%i", score->captures );
			CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

			//Defend
			tmp = va( "%i", score->defendCount );
			CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

			//Assist
			tmp = va( "%i", score->assistCount );
			CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );

			if ( score->ping != -1 ) {
				int msec, secs, mins;

				//Ping
				if ( ci->botSkill != -1 ) {
					tmp = "--";
					CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
				}
				else {
					tmp = va( "%i", score->ping );
					CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &pingColour, tmp, 0.0f, 0, 0 );
				}

				//Time
				msec = cg.time - cgs.levelStartTime - score->time;
				secs = msec/1000;
				mins = secs/60;
				secs %= 60;
				msec %= 1000;

				tmp = va( "%i:%02i", mins, secs );
				CG_Text_Paint( x + columnOffset[column++] - CG_Text_Width( tmp, fontScale, 0 )/2.0f, y + (lineHeight/2.0f) + (CG_Text_Height( tmp, fontScale, 0 )/2.0f), fontScale, &white, tmp, 0.0f, 0, 0 );
			}

			y += lineHeight;
		}
	}

	return count;
}

//Will render a list of players on team 'team' at 'x','y'
//	will use relevant information based on gametype
int ListPlayers_Free( float fade, float x, float y, float fontScale, float lineHeight, int startIndex, int playerCount ) {
	switch ( cgs.gametype ) {
	case GT_BLOODBATH:
		return ListPlayers_FFA( fade, x, y, fontScale, lineHeight, startIndex, playerCount );
	case GT_DUEL:
		break;

	case GT_TEAMBLOOD:
	case GT_FLAGS:
	case GT_TROJAN:
		trap->Error( ERR_DROP, "Tried to use non-team scoreboard on team gametype" );
		break;
	}

	return -1;
}

//Will render a list of players on team 'team' at 'x','y'
//	will use relevant information based on gametype
int ListPlayers_Team( float fade, float x, float y, float fontScale, float lineHeight, team_t team ) {
	switch ( cgs.gametype ) {
	case GT_BLOODBATH:
	case GT_DUEL:
		trap->Error( ERR_DROP, "Tried to use team scoreboard on non-team gametype" );
		break;

	case GT_TEAMBLOOD:
		return ListPlayers_TDM( fade, x, y, fontScale, lineHeight, team );
		break;
	case GT_FLAGS:
	case GT_TROJAN:
		return ListPlayers_Flags( fade, x, y, fontScale, lineHeight, team );
		break;
	}

	return -1;
}

void DrawSpectators( float fade ) {
	float fontScale = 0.33333f, y = 128.0f, lineHeight=14.0f;
	vector4	white = { 1.0f, 1.0f, 1.0f, 1.0f };
	white.a = fade;

	CG_BuildSpectatorString();
	cg.scoreboard.spectatorWidth = CG_Text_Width( cg.scoreboard.spectatorList, fontScale, 0 );

	if ( cg.scoreboard.spectatorLen ) {
		float dt = (cg.time - cg.scoreboard.spectatorResetTime)*0.0625f;
		cg.scoreboard.spectatorX = SCREEN_WIDTH - (1.0f)*dt;

		if ( cg.scoreboard.spectatorX < 0 - cg.scoreboard.spectatorWidth ) {
			cg.scoreboard.spectatorX = SCREEN_WIDTH;
			cg.scoreboard.spectatorResetTime = cg.time;
		}
		CG_Text_Paint( cg.scoreboard.spectatorX, (y + lineHeight*20) - 3, fontScale, &white, cg.scoreboard.spectatorList, 0.0f, 0, 0 );
	}
}

void DrawPlayers_Free( float fade ) {
	float fontScale = 0.25f, x = 0, y = 128.0f, lineHeight=14.0f;
	int playerCount = PlayerCount( TEAM_FREE );

	ListPlayers_Free( fade, x,						y, fontScale, lineHeight, 0, (playerCount+1)/2 );	// dirty hack, 7 players means 4 on left, so +1 then /2 (8/2==4) will give a good number (4 left, 3 right)
																										//	similarly, 8 players means 4 on left, so +1 then /2 (9/2==4) will give a good number (4 left, 4 right)
	ListPlayers_Free( fade, x + SCREEN_WIDTH/2.0f,	y, fontScale, lineHeight, (playerCount+1)/2, playerCount/2 );

	DrawSpectators( fade );
}

void DrawPlayers_Team( float fade ) {
	float fontScale = 0.25f/*0.17f*/, x = 0, y = 128.0f, lineHeight=14.0f;

	if ( cgs.scores1 >= cgs.scores2 ) {
		ListPlayers_Team( fade, x,						y, fontScale, lineHeight, TEAM_RED );
		ListPlayers_Team( fade, x + SCREEN_WIDTH/2.0f,	y, fontScale, lineHeight, TEAM_BLUE );
	}
	else {
		ListPlayers_Team( fade, x,						y, fontScale, lineHeight, TEAM_BLUE );
		ListPlayers_Team( fade, x + SCREEN_WIDTH/2.0f,	y, fontScale, lineHeight, TEAM_RED );
	}

	DrawSpectators( fade );
}

//controller for drawing the 'players' section
//	will call the relevant functions based on gametype
void DrawPlayers( float fade ) {
	switch ( cgs.gametype ) {
	case GT_BLOODBATH:
	case GT_DUEL:
		DrawPlayers_Free( fade );
		break;

	case GT_TEAMBLOOD:
	case GT_FLAGS:
	case GT_TROJAN:
		DrawPlayers_Team( fade );
		break;
	}
}

//Scoreboard entry point
//	This will be called even if the scoreboard is not showing - and will return false if it has faded aka 'not showing'
//	It will return true if the scoreboard is showing
qboolean CG_DrawScoreboard( void ) {
	vector4 fadeWhite = { 1.0f, 1.0f, 1.0f, 1.0f };
	float fade = 1.0f;

	if ( cg.warmup && !cg.showScores )
		return qfalse;

	if ( !cg.showScores && cg.snap->ps.pm_type != PM_DEAD && cg.snap->ps.pm_type != PM_INTERMISSION ) {
		if ( CG_FadeColor2( &fadeWhite, cg.scoreFadeTime, 300 ) )
			return qfalse;

		fade = fadeWhite.a;
	}

	DrawServerInformation( fade );

	DrawPlayerCount( fade );
	DrawPlayers( fade );

	CG_LoadDeferredPlayers();
	return qtrue;
}
