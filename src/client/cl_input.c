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
// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

unsigned	frame_msec;
int			old_com_frameTime;

/*
	KEY BUTTONS

	Continuous button event tracking is complicated by the fact that two different input sources (say, mouse button 1 and the control key)
	can both press the same button, but the button should only be released when both of the pressing key have been released.

	When a key event issues a button command (+forward, +attack, etc), it appends its key number as argv(1)
	so it can be matched up with the release.

	argv(2) will be set to the time the event happened, which allows exact control even at low framerates
	when the down and up events may both get qued at the same time.

*/


kbutton_t	in_forward, in_back, in_left, in_right;
kbutton_t	in_up, in_down;
kbutton_t	in_lookup, in_lookdown, in_lookleft, in_lookright;
kbutton_t	in_strafe, in_speed;
qboolean	in_mlooking;

kbutton_t	in_buttons[16];

#ifdef USE_VOIP
kbutton_t	in_voiprecord;
#endif


void IN_MLookDown( void ) {
	in_mlooking = qtrue;
}

void IN_MLookUp( void ) {
	in_mlooking = qfalse;
	if ( !cl_freelook->integer ) {
		IN_CenterView();
	}
}

void IN_KeyDown( kbutton_t *b ) {
	int		k;
	const char	*c;
	
	c = Cmd_Argv(1);
	if ( c[0] ) {
		k = atoi(c);
	} else {
		k = -1;		// typed manually at the console for continuous down
	}

	if ( k == b->down[0] || k == b->down[1] ) {
		return;		// repeating key
	}
	
	if ( !b->down[0] ) {
		b->down[0] = k;
	} else if ( !b->down[1] ) {
		b->down[1] = k;
	} else {
		Com_Printf ("Three keys down for a button!\n");
		return;
	}
	
	if ( b->active ) {
		return;		// still down
	}

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	b->downtime = atoi(c);

	b->active = qtrue;
	b->wasPressed = qtrue;
}

void IN_KeyUp( kbutton_t *b ) {
	int		k;
	const char	*c;
	unsigned	uptime;

	c = Cmd_Argv(1);
	if ( c[0] ) {
		k = atoi(c);
	} else {
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->active = qfalse;
		return;
	}

	if ( b->down[0] == k ) {
		b->down[0] = 0;
	} else if ( b->down[1] == k ) {
		b->down[1] = 0;
	} else {
		return;		// key up without coresponding down (menu pass through)
	}
	if ( b->down[0] || b->down[1] ) {
		return;		// some other key is still holding it down
	}

	b->active = qfalse;

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	uptime = atoi(c);
	if ( uptime ) {
		b->msec += uptime - b->downtime;
	} else {
		b->msec += frame_msec / 2;
	}

	b->active = qfalse;
}

// Returns the fraction of the frame that the key was down
float CL_KeyState( kbutton_t *key ) {
	float		val;
	int			msec;

	msec = key->msec;
	key->msec = 0;

	if ( key->active ) {
		// still down
		if ( !key->downtime ) {
			msec = com_frameTime;
		} else {
			msec += com_frameTime - key->downtime;
		}
		key->downtime = com_frameTime;
	}

#if 1
	if ( in_mouseDebug->integer == 4 ) //msec )
		Com_Printf ("%i msec ", msec);
#endif

	val = (float)msec / frame_msec;
	if ( val < 0.0f )	val = 0.0f;
	if ( val > 1.0f )	val = 1.0f;

	return val;
}

cvar_t	*cl_yawspeed;
cvar_t	*cl_pitchspeed;

cvar_t	*cl_run;

cvar_t	*cl_anglespeedkey;

// Moves the local angle positions
void CL_AdjustAngles( void ) {
	float	speed, keystateRight=1.0f, keystateLeft=1.0f, keystateLookUp=1.0f, keystateLookDown=1.0f;
	
	if ( in_speed.active )
		speed = 0.001f * cls.frametime * cl_anglespeedkey->value;
	else
		speed = 0.001f * cls.frametime;

	keystateRight		= CL_KeyState( &in_lookright );
	keystateLeft		= CL_KeyState( &in_lookleft );
	keystateLookUp		= CL_KeyState( &in_lookup );
	keystateLookDown	= CL_KeyState( &in_lookdown );

	if ( in_mouseDebug->integer == 3 )
		Com_Printf( "CL_AdjustAngles: r/l/u/d %f/%f/%f/%f\n", keystateRight, keystateLeft, keystateLookUp, keystateLookDown );

	if ( in_mouseDebug->integer == 3 )
		Com_Printf( "CL_AdjustAngles: cl.viewangles (%.2f %.2f) -> ", cl.viewangles.yaw, cl.viewangles.pitch );

	if ( !in_strafe.active ) {
		cl.viewangles.yaw -= speed * cl_yawspeed->value * keystateRight;
		cl.viewangles.yaw += speed * cl_yawspeed->value * keystateLeft;
	}

	cl.viewangles.pitch -= speed * cl_pitchspeed->value * keystateLookUp;
	cl.viewangles.pitch += speed * cl_pitchspeed->value * keystateLookDown;

	if ( in_mouseDebug->integer == 3 )
		Com_Printf( "(%.2f %.2f)\n", cl.viewangles.yaw, cl.viewangles.pitch );

}

// Sets the usercmd_t based on key states
void CL_KeyMove( usercmd_t *cmd ) {
	int		movespeed;
	int		forward, side, up;

	//
	// adjust for speed key / running
	// the walking flag is to keep animations consistant
	// even during acceleration and develeration
	//
	if ( in_speed.active ^ cl_run->integer ) {
		movespeed = 127;
		cmd->buttons &= ~BUTTON_WALKING;
	} else {
		cmd->buttons |= BUTTON_WALKING;
		movespeed = 64;
	}

	forward = 0;
	side = 0;
	up = 0;
	if ( in_strafe.active ) {
		side += (int)(movespeed * CL_KeyState( &in_lookright ));
		side -= (int)(movespeed * CL_KeyState( &in_lookleft ));
	}

	side += (int)(movespeed * CL_KeyState( &in_right ));
	side -= (int)(movespeed * CL_KeyState( &in_left ));


	up += (int)(movespeed * CL_KeyState( &in_up ));
	up -= (int)(movespeed * CL_KeyState( &in_down ));

	forward += (int)(movespeed * CL_KeyState( &in_forward ));
	forward -= (int)(movespeed * CL_KeyState( &in_back ));

	cmd->forwardmove = ClampChar( forward );
	cmd->rightmove = ClampChar( side );
	cmd->upmove = ClampChar( up );
}

void CL_MouseEvent( int dx, int dy, int time ) {
	if ( Key_GetCatcher( ) & KEYCATCH_UI ) {
		ui->MouseEvent( dx, dy );
	} else if (Key_GetCatcher( ) & KEYCATCH_CGAME) {
		cgame->MouseEvent( dx, dy );
	} else {
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
}

// Joystick values stay set until changed
void CL_JoystickEvent( int axis, int value, int time ) {
	if ( axis < 0 || axis >= MAX_JOYSTICK_AXIS ) {
		Com_Error( ERR_DROP, "CL_JoystickEvent: bad axis %i", axis );
	}
	cl.joystickAxis[axis] = value;
}

void CL_JoystickMove( usercmd_t *cmd ) {
	float	anglespeed;

	if ( !(in_speed.active ^ cl_run->integer) ) {
		cmd->buttons |= BUTTON_WALKING;
	}

	if ( in_speed.active ) {
		anglespeed = 0.001f * cls.frametime * cl_anglespeedkey->value;
	} else {
		anglespeed = 0.001f * cls.frametime;
	}

	if ( !in_strafe.active ) {
		cl.viewangles.yaw += anglespeed * j_yaw->value * cl.joystickAxis[j_yaw_axis->integer];
		cmd->rightmove = ClampChar( cmd->rightmove + (int) (j_side->value * cl.joystickAxis[j_side_axis->integer]) );
	} else {
		cl.viewangles.yaw += anglespeed * j_side->value * cl.joystickAxis[j_side_axis->integer];
		cmd->rightmove = ClampChar( cmd->rightmove + (int) (j_yaw->value * cl.joystickAxis[j_yaw_axis->integer]) );
	}

	if ( in_mlooking ) {
		cl.viewangles.pitch += anglespeed * j_forward->value * cl.joystickAxis[j_forward_axis->integer];
		cmd->forwardmove = ClampChar( cmd->forwardmove + (int) (j_pitch->value * cl.joystickAxis[j_pitch_axis->integer]) );
	} else {
		cl.viewangles.pitch += anglespeed * j_pitch->value * cl.joystickAxis[j_pitch_axis->integer];
		cmd->forwardmove = ClampChar( cmd->forwardmove + (int) (j_forward->value * cl.joystickAxis[j_forward_axis->integer]) );
	}

	cmd->upmove = ClampChar( cmd->upmove + (int) (j_up->value * cl.joystickAxis[j_up_axis->integer]) );
}

void CL_MouseMove(usercmd_t *cmd) {
	float mx, my;

	// allow mouse smoothing
	if (m_filter->boolean) {
		mx = (cl.mouseDx[0] + cl.mouseDx[1]) * 0.5f;
		my = (cl.mouseDy[0] + cl.mouseDy[1]) * 0.5f;
	}
	else {
		mx = (float)cl.mouseDx[cl.mouseIndex];
		my = (float)cl.mouseDy[cl.mouseIndex];
	}
	
	cl.mouseIndex ^= 1;
	cl.mouseDx[cl.mouseIndex] = 0;
	cl.mouseDy[cl.mouseIndex] = 0;

	if (mx == 0.0f && my == 0.0f)
		return;
	
	if (m_accel->value > 0.0f)
	{
		if(m_accelStyle->integer == 0)
		{
			float accelSensitivity;
			float rate;
			
			rate = sqrtf(mx*mx + my*my) / (float) frame_msec;

			accelSensitivity = 1.0f + rate * m_accel->value;
			mx *= accelSensitivity;
			my *= accelSensitivity;
			
			if(cl_showMouseRate->integer)
				Com_Printf("rate: %f, accelSensitivity: %f\n", rate, accelSensitivity);
		}
		else
		{
			float rate[2];
			float power[2];

			// sensitivity remains pretty much unchanged at low speeds
			// cl_mouseAccel is a power value to how the acceleration is shaped
			// cl_mouseAccelOffset is the rate for which the acceleration will have doubled the non accelerated amplification
			// NOTE: decouple the config cvars for independent acceleration setup along X and Y?

			rate[0] = fabsf(mx) / (float) frame_msec;
			rate[1] = fabsf(my) / (float) frame_msec;
			power[0] = powf(rate[0] / m_accelOffset->value, m_accel->value);
			power[1] = powf(rate[1] / m_accelOffset->value, m_accel->value);

			mx = (mx + ((mx < 0) ? -power[0] : power[0]) * m_accelOffset->value);
			my = (my + ((my < 0) ? -power[1] : power[1]) * m_accelOffset->value);

			if(cl_showMouseRate->integer)
				Com_Printf("ratex: %f, ratey: %f, powx: %f, powy: %f\n", rate[0], rate[1], power[0], power[1]);
		}
	}

	// ingame FOV
	mx *= cl.cgameSensitivity;
	my *= cl.cgameSensitivity;

	if ( in_mouseDebug->integer > 2 )
		Com_Printf( "CL_MouseMove: mx my (%.2f, %.2f)\n", mx, my );

	if ( in_mouseDebug->integer == 3 )
		Com_Printf( "CL_MouseMove: cl.viewangles (%.2f %.2f) -> ", cl.viewangles.yaw, cl.viewangles.pitch );

	// add mouse X/Y movement to cmd
	if(in_strafe.active)
		cmd->rightmove = ClampChar( cmd->rightmove + (int)(m_side->value * mx) );
	else
		cl.viewangles.yaw -= mx * m_sensitivity->value;

	if ((in_mlooking || cl_freelook->integer) && !in_strafe.active)
		cl.viewangles.pitch += my * m_sensitivity->value;
	else
		cmd->forwardmove = ClampChar( cmd->forwardmove - (int)(m_forward->value * my) );

	if ( in_mouseDebug->integer == 3 )
		Com_Printf( "(%.2f %.2f)\n", cl.viewangles.yaw, cl.viewangles.pitch );
}

void CL_CmdButtons( usercmd_t *cmd ) {
	int		i;

	//
	// figure button bits
	// send a button bit even if the key was pressed and released in
	// less than a frame
	//	
	for (i = 0 ; i < 15 ; i++) {
		if ( in_buttons[i].active || in_buttons[i].wasPressed ) {
			cmd->buttons |= 1 << i;
		}
		in_buttons[i].wasPressed = qfalse;
	}

	if ( Key_GetCatcher( ) ) {
		cmd->buttons |= BUTTON_TALK;
	}

	// allow the game to know if any key at all is
	// currently pressed, even if it isn't bound to anything
	if ( anykeydown && Key_GetCatcher( ) == 0 ) {
		cmd->buttons |= BUTTON_ANY;
	}
}

void CL_FinishMove( usercmd_t *cmd ) {
	int		i;

	// copy the state that the cgame is currently sending
	cmd->weapon = (byte)cl.cgameUserCmdValue;

	// send the current server time so the amount of movement
	// can be determined without allowing cheating
	cmd->serverTime = cl.serverTime;

	for (i=0 ; i<3 ; i++) {
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles.data[i]);
	}
}

usercmd_t CL_CreateCmd( void ) {
	usercmd_t	cmd;
	vector3		oldAngles;

	VectorCopy( &cl.viewangles, &oldAngles );

	// keyboard angle adjustment
	CL_AdjustAngles();
	
	memset( &cmd, 0, sizeof( cmd ) );

	CL_CmdButtons( &cmd );

	// get basic movement from keyboard
	CL_KeyMove( &cmd );

	// get basic movement from mouse
	CL_MouseMove( &cmd );

	// get basic movement from joystick
	CL_JoystickMove( &cmd );

	// check to make sure the angles haven't wrapped
	if ( cl.viewangles.pitch - oldAngles.pitch > 90 ) {
		cl.viewangles.pitch = oldAngles.pitch + 90;
	} else if ( oldAngles.pitch - cl.viewangles.pitch > 90 ) {
		cl.viewangles.pitch = oldAngles.pitch - 90;
	} 

	// store out the final values
	CL_FinishMove( &cmd );

	// draw debug graphs of turning for mouse testing
	if ( cl_debugMove->integer ) {
		if ( cl_debugMove->integer == 1 ) {
			SCR_DebugGraph( fabsf(cl.viewangles.yaw - oldAngles.yaw) );
		}
		if ( cl_debugMove->integer == 2 ) {
			SCR_DebugGraph( fabsf(cl.viewangles.pitch - oldAngles.pitch) );
		}
	}

	return cmd;
}

// Create a new usercmd_t structure for this frame
void CL_CreateNewCommands( void ) {
	// no need to create usercmds until we have a gamestate
	if ( clc.state < CA_PRIMED )
		return;

	frame_msec = com_frameTime - old_com_frameTime;
	old_com_frameTime = com_frameTime;

		 if ( frame_msec > 200 )	frame_msec = 200;
	else if ( frame_msec < 1 )		frame_msec = 1;

	cl.cmds[++cl.cmdNumber & CMD_MASK] = CL_CreateCmd();
}

// Returns qfalse if we are over the maxpackets limit and should choke back the bandwidth a bit by not sending a packet this frame.
//	All the commands will still get delivered in the next packet, but saving a header and getting more
//	delta compression will reduce total bandwidth.
qboolean CL_ReadyToSendPacket( void ) {
	int		oldPacketNum;
	int		delta;

	// don't send anything if playing back a demo
	if ( clc.demoplaying ) {
		return qfalse;
	}

	// If we are downloading, we send no less than 50ms between packets
	if ( *clc.downloadTempName &&
		cls.realtime - clc.lastPacketSentTime < 50 ) {
		return qfalse;
	}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if ( clc.state != CA_ACTIVE && 
		clc.state != CA_PRIMED && 
		!*clc.downloadTempName &&
		cls.realtime - clc.lastPacketSentTime < 1000 ) {
		return qfalse;
	}

	// send every frame for loopbacks
	if ( clc.netchan.remoteAddress.type == NA_LOOPBACK ) {
		return qtrue;
	}

	// send every frame for LAN
	if ( cl_lanForcePackets->integer && Sys_IsLANAddress( clc.netchan.remoteAddress ) ) {
		return qtrue;
	}

	// check for exceeding cl_maxpackets
	if ( cl_maxpackets->integer < 15 ) {
		Cvar_Set( "cl_maxpackets", "15" );
	} else if ( cl_maxpackets->integer > 125 ) {
		Cvar_Set( "cl_maxpackets", "125" );
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1) & PACKET_MASK;
	delta = cls.realtime -  cl.outPackets[ oldPacketNum ].p_realtime;
	if ( delta < 1000 / cl_maxpackets->integer ) {
		// the accumulated commands will go out in the next packet
		return qfalse;
	}

	return qtrue;
}

// Create and send the command packet to the server including both the reliable commands and the usercmds
//	During normal gameplay, a client packet will contain something like:
//		4	sequence number
//		2	qport
//		4	serverid
//		4	acknowledged sequence number
//		4	clc.serverCommandSequence
//		<optional reliable commands>
//		1	clc_move or clc_moveNoDelta
//		1	command count
//		<count * usercmds>
void CL_WritePacket( void ) {
	msg_t		buf;
	byte		data[MAX_MSGLEN];
	int			i, j;
	usercmd_t	*cmd, *oldcmd;
	usercmd_t	nullcmd;
	int			packetNum;
	int			oldPacketNum;
	int			count, key;

	// don't send anything if playing back a demo
	if ( clc.demoplaying ) {
		return;
	}

	memset( &nullcmd, 0, sizeof(nullcmd) );
	oldcmd = &nullcmd;

	MSG_Init( &buf, data, sizeof(data) );

	MSG_Bitstream( &buf );
	// write the current serverId so the server
	// can tell if this is from the current gameState
	MSG_WriteLong( &buf, cl.serverId );

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	MSG_WriteLong( &buf, clc.serverMessageSequence );

	// write the last reliable message we received
	MSG_WriteLong( &buf, clc.serverCommandSequence );

	// write any unacknowledged clientCommands
	for ( i = clc.reliableAcknowledge + 1 ; i <= clc.reliableSequence ; i++ ) {
		MSG_WriteByte( &buf, clc_clientCommand );
		MSG_WriteLong( &buf, i );
		MSG_WriteString( &buf, clc.reliableCommands[ i & (MAX_RELIABLE_COMMANDS-1) ] );
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if ( cl_packetdup->integer < 0 ) {
		Cvar_Set( "cl_packetdup", "0" );
	} else if ( cl_packetdup->integer > 5 ) {
		Cvar_Set( "cl_packetdup", "5" );
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1 - cl_packetdup->integer) & PACKET_MASK;
	count = cl.cmdNumber - cl.outPackets[ oldPacketNum ].p_cmdNumber;
	if ( count > MAX_PACKET_USERCMDS ) {
		count = MAX_PACKET_USERCMDS;
		Com_Printf("MAX_PACKET_USERCMDS\n");
	}

#ifdef USE_VOIP
	if (clc.voipOutgoingDataSize > 0)
	{
		if((clc.voipFlags & VOIP_SPATIAL) || Com_IsVoipTarget(clc.voipTargets, sizeof(clc.voipTargets), -1))
		{
			MSG_WriteByte (&buf, clc_voip);
			MSG_WriteByte (&buf, clc.voipOutgoingGeneration);
			MSG_WriteLong (&buf, clc.voipOutgoingSequence);
			MSG_WriteByte (&buf, clc.voipOutgoingDataFrames);
			MSG_WriteData (&buf, clc.voipTargets, sizeof(clc.voipTargets));
			MSG_WriteByte(&buf, clc.voipFlags);
			MSG_WriteShort (&buf, clc.voipOutgoingDataSize);
			MSG_WriteData (&buf, clc.voipOutgoingData, clc.voipOutgoingDataSize);

			// If we're recording a demo, we have to fake a server packet with
			//  this VoIP data so it gets to disk; the server doesn't send it
			//  back to us, and we might as well eliminate concerns about dropped
			//  and misordered packets here.
			if(clc.demorecording && !clc.demowaiting)
			{
				const int voipSize = clc.voipOutgoingDataSize;
				msg_t fakemsg;
				byte fakedata[MAX_MSGLEN];
				MSG_Init (&fakemsg, fakedata, sizeof (fakedata));
				MSG_Bitstream (&fakemsg);
				MSG_WriteLong (&fakemsg, clc.reliableAcknowledge);
				MSG_WriteByte (&fakemsg, svc_voip);
				MSG_WriteShort (&fakemsg, clc.clientNum);
				MSG_WriteByte (&fakemsg, clc.voipOutgoingGeneration);
				MSG_WriteLong (&fakemsg, clc.voipOutgoingSequence);
				MSG_WriteByte (&fakemsg, clc.voipOutgoingDataFrames);
				MSG_WriteShort (&fakemsg, clc.voipOutgoingDataSize );
				MSG_WriteBits (&fakemsg, clc.voipFlags, VOIP_FLAGCNT);
				MSG_WriteData (&fakemsg, clc.voipOutgoingData, voipSize);
				MSG_WriteByte (&fakemsg, svc_EOF);
				CL_WriteDemoMessage (&fakemsg, 0);
			}

			clc.voipOutgoingSequence += clc.voipOutgoingDataFrames;
			clc.voipOutgoingDataSize = 0;
			clc.voipOutgoingDataFrames = 0;
		}
		else
		{
			// We have data, but no targets. Silently discard all data
			clc.voipOutgoingDataSize = 0;
			clc.voipOutgoingDataFrames = 0;
		}
	}
#endif

	if ( count >= 1 ) {
		if ( cl_showSend->integer ) {
			Com_Printf( "(%i)", count );
		}

		// begin a client move command
		if ( cl_nodelta->integer || !cl.snap.valid || clc.demowaiting
			|| clc.serverMessageSequence != cl.snap.messageNum ) {
			MSG_WriteByte (&buf, clc_moveNoDelta);
		} else {
			MSG_WriteByte (&buf, clc_move);
		}

		// write the command count
		MSG_WriteByte( &buf, count );

		// use the checksum feed in the key
		key = clc.checksumFeed;
		// also use the message acknowledge
		key ^= clc.serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= MSG_HashKey(clc.serverCommands[ clc.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1) ], 32);

		// write all the commands, including the predicted command
		for ( i = 0 ; i < count ; i++ ) {
			j = (cl.cmdNumber - count + i + 1) & CMD_MASK;
			cmd = &cl.cmds[j];
			MSG_WriteDeltaUsercmdKey (&buf, key, oldcmd, cmd);
			oldcmd = cmd;
		}
	}

	//
	// deliver the message
	//
	packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
	cl.outPackets[ packetNum ].p_realtime = cls.realtime;
	cl.outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
	cl.outPackets[ packetNum ].p_cmdNumber = cl.cmdNumber;
	clc.lastPacketSentTime = cls.realtime;

	if ( cl_showSend->integer ) {
		Com_Printf( "%i ", buf.cursize );
	}

	CL_Netchan_Transmit (&clc.netchan, &buf);	
}

// Called every frame to builds and sends a command packet to the server.
void CL_SendCmd( void ) {
	// don't send any message if not connected
	if ( clc.state < CA_CONNECTED ) {
		return;
	}

	// don't send commands if paused
	if ( com_sv_running->integer && sv_paused->integer && cl_paused->integer ) {
		return;
	}

	// we create commands even if a demo is playing,
	CL_CreateNewCommands();

	// don't send a packet if the last packet was sent too recently
	if ( !CL_ReadyToSendPacket() ) {
		if ( cl_showSend->integer ) {
			Com_Printf( ". " );
		}
		return;
	}

	CL_WritePacket();
}

void IN_UpDown( void )			{ IN_KeyDown( &in_up ); }
void IN_UpUp( void )			{ IN_KeyUp( &in_up ); }
void IN_DownDown( void )		{ IN_KeyDown( &in_down ); }
void IN_DownUp( void )			{ IN_KeyUp( &in_down ); }
void IN_LeftDown( void )		{ IN_KeyDown( &in_left ); }
void IN_LeftUp( void )			{ IN_KeyUp( &in_left ); }
void IN_RightDown( void )		{ IN_KeyDown( &in_right ); }
void IN_RightUp( void )			{ IN_KeyUp( &in_right ); }
void IN_ForwardDown( void )		{ IN_KeyDown( &in_forward ); }
void IN_ForwardUp( void )		{ IN_KeyUp( &in_forward ); }
void IN_BackDown( void )		{ IN_KeyDown( &in_back ); }
void IN_BackUp( void )			{ IN_KeyUp( &in_back ); }
void IN_LookLeftDown( void )	{ IN_KeyDown( &in_lookleft ); }
void IN_LookLeftUp( void )		{ IN_KeyUp( &in_lookleft ); }
void IN_LookRightDown( void )	{ IN_KeyDown( &in_lookright ); }
void IN_LookRightUp( void )		{ IN_KeyUp( &in_lookright ); }
void IN_CenterView( void )		{ cl.viewangles.pitch = -SHORT2ANGLE( cl.snap.ps.delta_angles.pitch ); }
void IN_LookupDown( void )		{ IN_KeyDown( &in_lookup ); }
void IN_LookupUp( void )		{ IN_KeyUp( &in_lookup ); }
void IN_LookdownDown( void )	{ IN_KeyDown( &in_lookdown ); }
void IN_LookdownUp( void )		{ IN_KeyUp( &in_lookdown ); }
void IN_SpeedDown( void )		{ IN_KeyDown( &in_speed ); }
void IN_SpeedUp( void )			{ IN_KeyUp( &in_speed ); }
void IN_StrafeDown( void )		{ IN_KeyDown( &in_strafe ); }
void IN_StrafeUp( void )		{ IN_KeyUp( &in_strafe ); }
#ifdef USE_VOIP
void IN_VoipRecordDown( void )	{ IN_KeyDown( &in_voiprecord ); Cvar_Set( "cl_voipSend", "1" ); }
void IN_VoipRecordUp( void )	{ IN_KeyUp( &in_voiprecord ); Cvar_Set( "cl_voipSend", "0" ); }
#endif

#define INBUTTON_LIST( x ) \
	static QINLINE void IN_ButtonDown##x( void ) { IN_KeyDown( &in_buttons[x] ); } \
	static QINLINE void IN_ButtonUp##x( void ) { IN_KeyUp( &in_buttons[x] ); }

INBUTTON_LIST(  0 )
INBUTTON_LIST(  1 )
INBUTTON_LIST(  2 )
INBUTTON_LIST(  3 )
INBUTTON_LIST(  4 )
INBUTTON_LIST(  5 )
INBUTTON_LIST(  6 )
INBUTTON_LIST(  7 )
INBUTTON_LIST(  8 )
INBUTTON_LIST(  9 )
INBUTTON_LIST( 10 )
INBUTTON_LIST( 11 )
INBUTTON_LIST( 12 )
INBUTTON_LIST( 13 )
INBUTTON_LIST( 14 )
INBUTTON_LIST( 15 )

#define AddButton( name, down, up ) { Cmd_AddCommand( "+"name, down, NULL ); Cmd_AddCommand( "-"name, up, NULL ); }
#define RemoveButton( name ) { Cmd_RemoveCommand( "+"name ); Cmd_RemoveCommand( "-"name ); }

void CL_InitInput( void ) {
	Cmd_AddCommand( "centerview", IN_CenterView, NULL );

	// vertical movement
	AddButton( "moveup",	IN_UpDown, IN_UpUp );
	AddButton( "jump",		IN_UpDown, IN_UpUp );
	AddButton( "movedown",	IN_DownDown, IN_DownUp );
	AddButton( "crouch",	IN_DownDown, IN_DownUp );

	// directional movement
	AddButton( "forward",	IN_ForwardDown, IN_ForwardUp );
	AddButton( "back",		IN_BackDown, IN_BackUp );
	AddButton( "left",		IN_LeftDown, IN_LeftUp );
	AddButton( "right",		IN_RightDown, IN_RightUp );

	// modifiers
	AddButton( "speed",		IN_SpeedDown, IN_SpeedUp );

	// view
	AddButton( "lookleft",	IN_LookLeftDown, IN_LookLeftUp );
	AddButton( "lookright",	IN_LookRightDown, IN_LookRightUp );
	AddButton( "lookup",	IN_LookupDown, IN_LookupUp );
	AddButton( "lookdown",	IN_LookdownDown, IN_LookdownUp );
	AddButton( "strafe",	IN_StrafeDown, IN_StrafeUp );
	AddButton( "mlook",		IN_MLookDown, IN_MLookUp );
#ifdef USE_VOIP
	AddButton( "voiprecord", IN_VoipRecordDown, IN_VoipRecordUp );
#endif

	// networked buttons
	AddButton( "attack",	IN_ButtonDown0, IN_ButtonUp0 );
	AddButton( "button0",	IN_ButtonDown0, IN_ButtonUp0 );
	AddButton( "altattack", IN_ButtonDown1, IN_ButtonUp1 );
	AddButton( "button1",	IN_ButtonDown1, IN_ButtonUp1 );
	AddButton( "button2",	IN_ButtonDown2, IN_ButtonUp2 );
	AddButton( "button3",	IN_ButtonDown3, IN_ButtonUp3 );
	AddButton( "button4",	IN_ButtonDown4, IN_ButtonUp4 );
	AddButton( "button5",	IN_ButtonDown5, IN_ButtonUp5 );
	AddButton( "button6",	IN_ButtonDown6, IN_ButtonUp6 );
	AddButton( "button7",	IN_ButtonDown7, IN_ButtonUp7 );
	AddButton( "button8",	IN_ButtonDown8, IN_ButtonUp8 );
	AddButton( "button9",	IN_ButtonDown9, IN_ButtonUp9 );
	AddButton( "button10",	IN_ButtonDown10, IN_ButtonUp10 );
	AddButton( "button11",	IN_ButtonDown11, IN_ButtonUp11 );
	AddButton( "button12",	IN_ButtonDown12, IN_ButtonUp12 );
	AddButton( "button13",	IN_ButtonDown13, IN_ButtonUp13 );
	AddButton( "button14",	IN_ButtonDown14, IN_ButtonUp14 );

	cl_nodelta = Cvar_Get( "cl_nodelta", "0", 0, "Disable delta encoding of network packets", NULL );
	cl_debugMove = Cvar_Get( "cl_debugMove", "0", 0, "Show graph for mouse movement", NULL );
}

void CL_ShutdownInput( void ) {
	Cmd_RemoveCommand( "centerview" );

	RemoveButton( "+moveup" );
	RemoveButton( "+movedown" );
	RemoveButton( "+left" );
	RemoveButton( "+right" );
	RemoveButton( "+forward" );
	RemoveButton( "+back" );
	RemoveButton( "+lookup" );
	RemoveButton( "+lookdown" );
	RemoveButton( "+strafe" );
	RemoveButton( "+lookleft" );
	RemoveButton( "+lookright" );
	RemoveButton( "+speed" );
	RemoveButton( "+attack" );
	RemoveButton( "+button0" );
	RemoveButton( "+button1" );
	RemoveButton( "+button2" );
	RemoveButton( "+button3" );
	RemoveButton( "+button4" );
	RemoveButton( "+button5" );
	RemoveButton( "+button6" );
	RemoveButton( "+button7" );
	RemoveButton( "+button8" );
	RemoveButton( "+button9" );
	RemoveButton( "+button10" );
	RemoveButton( "+button11" );
	RemoveButton( "+button12" );
	RemoveButton( "+button13" );
	RemoveButton( "+button14" );
	RemoveButton( "+mlook" );

#ifdef USE_VOIP
	RemoveButton( "voiprecord" );
#endif
}
