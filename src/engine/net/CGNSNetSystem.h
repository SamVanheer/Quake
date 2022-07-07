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

#pragma once

#include <vector>

#include <steam/steamnetworkingsockets.h>

#include "net.h"

/**
*	@brief GameNetworkingSockets-based networking system.
*/
class CGNSNetSystem final : public INetSystem
{
public:
	CGNSNetSystem();
	~CGNSNetSystem() override;

	std::uint16_t GetPort() const override { return m_Port; }

	void SetPort(std::uint16_t port) override;

	void Listen() override;

	qsocket_t* Connect(const char* host) override;

	bool CanSendMessage(qsocket_t* conn) override;

	int GetMessage(qsocket_t* conn) override;

	int SendMessage(qsocket_t* conn, sizebuf_t* data) override;

	int SendUnreliableMessage(qsocket_t* conn, sizebuf_t* data) override;

	int SendToAll(sizebuf_t* data, int blocktime) override;

	void Close(qsocket_t* conn) override;

	void RunFrame() override;

private:
	bool IsValidConnection(qsocket_t* conn) const;

	void CloseSocket();

	void OnServerSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);
	void OnClientSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);

	static inline CGNSNetSystem* s_pCallbackInstance = nullptr;
	static void ServerSteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
	{
		s_pCallbackInstance->OnServerSteamNetConnectionStatusChanged(pInfo);
	}

	static void ClientSteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
	{
		s_pCallbackInstance->OnClientSteamNetConnectionStatusChanged(pInfo);
	}

private:
	ISteamNetworkingSockets* m_GNSInterface = nullptr;

	std::uint16_t m_Port = 0;

	HSteamListenSocket m_GNSSocket = k_HSteamListenSocket_Invalid;

	std::vector<std::unique_ptr<qsocket_t>> m_Connections;
};
