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
// net.h -- quake's interface to the networking layer

#pragma once

#include <memory>
#include <string>

#include <steam/steamnetworkingsockets.h>

#define	NET_NAMELEN			64

#define NET_MAXMESSAGE		8192

struct qsocket_t
{
	/**
	*	@brief If this is a real connection (i.e. not a VCR playback) this will be valid
	*/
	HSteamNetConnection connection = k_HSteamNetConnection_Invalid;

	double connecttime;

	std::string address;
};

struct INetSystem
{
	virtual ~INetSystem() = default;

	virtual std::uint16_t GetPort() const = 0;

	virtual void SetPort(std::uint16_t port) = 0;

	virtual void Listen() = 0;

	/**
	*	@brief called by client to connect to a host.  Returns -1 if not able to
	*/
	virtual qsocket_t* Connect(const char* host) = 0;

	/**
	*	@brief Returns true or false if the given qsocket can currently accept a message to be transmitted.
	*/
	virtual bool CanSendMessage(qsocket_t* conn) = 0;

	/**
	*	@brief returns data in net_message sizebuf
	*	@return 0 if no data is waiting
	*			1 if a message was received
	*			2 if an unreliable message was received
	*			-1 if the connection died
	*/
	virtual int GetMessage(qsocket_t* conn) = 0;
	
	/**
	*	@return 0 if the message connot be delivered reliably, but the connection is still considered valid
	*			1 if the message was sent properly
	*			-1 if the connection died
	*/
	virtual int SendMessage(qsocket_t* conn, sizebuf_t* data) = 0;

	virtual int SendUnreliableMessage(qsocket_t* conn, sizebuf_t* data) = 0;

	/**
	*	@brief This is a reliable *blocking* send to all attached clients.
	*/
	virtual int SendToAll(sizebuf_t* data, int blocktime) = 0;

	/**
	*	@brief if a dead connection is returned by a get or send function, this function should be called when it is convenient
	*	@details Server calls when a client is kicked off for a game related misbehavior
	*	like an illegal protocal conversation. Client calls when disconnecting from a server.
	*	A netcon_t number will not be reused until this function is called for it
	*/
	virtual void Close(qsocket_t* conn) = 0;

	virtual void RunFrame() = 0;
};

extern int			DEFAULTnet_hostport;

extern cvar_t		hostname;

extern int		messagesSent;
extern int		messagesReceived;
extern int		unreliableMessagesSent;
extern int		unreliableMessagesReceived;

inline std::unique_ptr<INetSystem> g_Networking;

void SetNetTime();

//============================================================================
//
// public network functions
//
//============================================================================

extern	double		net_time;
extern	sizebuf_t	net_message;
extern	int			net_activeconnections;

void		NET_Init(void);
void		NET_Shutdown(void);

extern	char		my_tcpip_address[NET_NAMELEN];
