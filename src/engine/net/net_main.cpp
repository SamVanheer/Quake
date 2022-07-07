/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_main.c

#include "quakedef.h"
#include "CGNSNetSystem.h"

int			DEFAULTnet_hostport = 26000;

char		my_tcpip_address[NET_NAMELEN];

sizebuf_t		net_message;
int				net_activeconnections = 0;

int messagesSent = 0;
int messagesReceived = 0;
int unreliableMessagesSent = 0;
int unreliableMessagesReceived = 0;

cvar_t	hostname = {"hostname", "UNNAMED"};

#ifdef IDGODS
cvar_t	idgods = {"idgods", "0"};
#endif

double net_time;

void SetNetTime()
{
	net_time = Sys_FloatTime();
}

static void MaxPlayers_f(void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf("\"maxplayers\" is \"%u\"\n", svs.maxclients);
		return;
	}

	if (sv.active)
	{
		Con_Printf("maxplayers can not be changed while a server is running.\n");
		return;
	}

	int n = Q_atoi(Cmd_Argv(1));
	if (n < 1)
		n = 1;
	if (n > svs.maxclientslimit)
	{
		n = svs.maxclientslimit;
		Con_Printf("\"maxplayers\" set to \"%u\"\n", n);
	}

	svs.maxclients = n;
	if (n == 1)
		Cvar_Set("deathmatch", "0");
	else
		Cvar_Set("deathmatch", "1");
}


static void NET_Port_f(void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf("\"port\" is \"%u\"\n", g_Networking->GetPort());
		return;
	}

	const int n = Q_atoi(Cmd_Argv(1));
	if (n < 1 || n > 65534)
	{
		Con_Printf("Bad value, must be between 1 and 65534\n");
		return;
	}

	DEFAULTnet_hostport = n;
	g_Networking->SetPort(n);
}

//=============================================================================

/*
====================
NET_Init
====================
*/

void NET_Init(void)
{
	g_Networking = std::make_unique<CGNSNetSystem>();

	int i = COM_CheckParm("-port");
	if (!i)
		i = COM_CheckParm("-udpport");

	if (i)
	{
		if (i < com_argc - 1)
			DEFAULTnet_hostport = Q_atoi(com_argv[i + 1]);
		else
			Sys_Error("NET_Init: you must specify a number after -port");
	}

	g_Networking->SetPort(DEFAULTnet_hostport);

	if (COM_CheckParm("-listen") || cls.state == ca_dedicated)
	{
		g_Networking->Listen();
	}

	SetNetTime();

	// allocate space for network message buffer
	SZ_Alloc(&net_message, NET_MAXMESSAGE);

	Cvar_RegisterVariable(&hostname);
#ifdef IDGODS
	Cvar_RegisterVariable(&idgods);
#endif

	Cmd_AddCommand("maxplayers", MaxPlayers_f);
	Cmd_AddCommand("port", NET_Port_f);

	// initialize all the drivers
	g_Networking->Listen();

	if (*my_tcpip_address)
		Con_DPrintf("TCP/IP address %s\n", my_tcpip_address);
}

/*
====================
NET_Shutdown
====================
*/

void		NET_Shutdown(void)
{
	SetNetTime();

	g_Networking.reset();
}
