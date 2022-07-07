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

#include <algorithm>
#include <memory>

#include <steam/isteamnetworkingutils.h>

#include "quakedef.h"
#include "CGNSNetSystem.h"

void SV_NewClient(qsocket_t* newClient);

static void DebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg)
{
	Con_Printf("%s\n", pszMsg);
}

CGNSNetSystem::CGNSNetSystem()
{
	s_pCallbackInstance = this;

	SteamNetworkingErrMsg error;
	if (!GameNetworkingSockets_Init(nullptr, error))
	{
		Sys_Error("Error initializing GameNetworkingSockets: %s\n", error);
	}

	m_GNSInterface = SteamNetworkingSockets();

	SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Msg, &DebugOutput);
}

CGNSNetSystem::~CGNSNetSystem()
{
	if (m_GNSInterface)
	{
		for (auto& conn : m_Connections)
		{
			m_GNSInterface->CloseConnection(conn->connection, k_ESteamNetConnectionEnd_App_Generic, "Shutting down", false);
		}

		m_Connections.clear();

		CloseSocket();
	}

	GameNetworkingSockets_Kill();

	s_pCallbackInstance = nullptr;
}

void CGNSNetSystem::SetPort(std::uint16_t port)
{
	m_Port = port;

	if ((m_GNSSocket != k_HSteamListenSocket_Invalid))
	{
		// force a change to the new port
		CloseSocket();
		Listen();
	}
}

void CGNSNetSystem::Listen()
{
	if ((m_GNSSocket != k_HSteamListenSocket_Invalid))
	{
		return;
	}

	SteamNetworkingIPAddr addr;
	addr.Clear();
	addr.m_port = m_Port;

	SteamNetworkingConfigValue_t opt;
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)ServerSteamNetConnectionStatusChangedCallback);
	m_GNSSocket = m_GNSInterface->CreateListenSocketIP(addr, 1, &opt);

	static_assert(NET_NAMELEN >= SteamNetworkingIPAddr::k_cchMaxString);

	addr.ToString(my_tcpip_address, sizeof(my_tcpip_address), false);
}

qsocket_t* CGNSNetSystem::Connect(const char* host)
{
	SetNetTime();

	if (!host)
		return nullptr;

	SteamNetworkingIPAddr addr;

	if (0 == strcmp(host, "local"))
	{
		addr.SetIPv6LocalHost(GetPort());
	}
	else
	{
		if (!addr.ParseString(host))
		{
			return nullptr;
		}
	}

	SteamNetworkingConfigValue_t opt;
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)ClientSteamNetConnectionStatusChangedCallback);
	auto connection = m_GNSInterface->ConnectByIPAddress(addr, 1, &opt);

	if (connection == k_HSteamNetConnection_Invalid)
	{
		return nullptr;
	}

	auto socket = std::make_unique<qsocket_t>();

	socket->connection = connection;
	socket->connecttime = net_time;
	socket->address = host;

	m_Connections.push_back(std::move(socket));

	return m_Connections.back().get();
}

bool CGNSNetSystem::CanSendMessage(qsocket_t* conn)
{
	if (!conn)
		return false;

	if (!IsValidConnection(conn))
		return false;

	SetNetTime();

	const bool r = true;// sfunc.CanSendMessage(sock); //TODO: figure out if we need to throttle packets
	return r;
}

int CGNSNetSystem::GetMessage(qsocket_t* conn)
{
	if (!conn)
		return -1;

	if (!IsValidConnection(conn))
	{
		Con_Printf("NET_GetMessage: disconnected socket\n");
		return -1;
	}

	SetNetTime();

	SteamNetworkingMessage_t* message = nullptr;
	int ret = m_GNSInterface->ReceiveMessagesOnConnection(conn->connection, &message, 1);

	if (ret > 0)
	{
		SZ_Clear(&net_message);
		SZ_Write(&net_message, message->GetData(), message->GetSize());

		ret = (message->m_nFlags & k_nSteamNetworkingSend_Reliable) != 0 ? 1 : 2;

		if (ret == 1)
			messagesReceived++;
		else if (ret == 2)
			unreliableMessagesReceived++;
	}

	if (message)
	{
		message->Release();
	}

	return ret;
}

int CGNSNetSystem::SendMessage(qsocket_t* conn, sizebuf_t* data)
{
	if (!conn)
		return -1;

	if (!IsValidConnection(conn))
	{
		Con_Printf("NET_SendMessage: disconnected socket\n");
		return -1;
	}

	SetNetTime();
	const int r = m_GNSInterface->SendMessageToConnection(
		conn->connection,
		data->data,
		data->cursize,
		k_nSteamNetworkingSend_Reliable | k_nSteamNetworkingSend_NoNagle, nullptr) == k_EResultOK ? 1 : -1;

	if (r == 1)
		messagesSent++;

	return r;
}

int CGNSNetSystem::SendUnreliableMessage(qsocket_t* conn, sizebuf_t* data)
{
	if (!conn)
		return -1;

	if (!IsValidConnection(conn))
	{
		Con_Printf("NET_SendMessage: disconnected socket\n");
		return -1;
	}

	SetNetTime();
	const int r = m_GNSInterface->SendMessageToConnection(
		conn->connection,
		data->data,
		data->cursize,
		k_nSteamNetworkingSend_Unreliable | k_nSteamNetworkingSend_NoNagle, nullptr) == k_EResultOK ? 1 : -1;

	if (r == 1)
		unreliableMessagesSent++;

	return r;
}

int CGNSNetSystem::SendToAll(sizebuf_t* data, int blocktime)
{
	int			i;
	int			count = 0;
	bool		state1[MAX_SCOREBOARD];
	bool		state2[MAX_SCOREBOARD];

	for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
	{
		if (!host_client->netconnection)
			continue;
		if (host_client->active)
		{
			SendMessage(host_client->netconnection, data);
			state1[i] = true;
			state2[i] = true;
		}
		else
		{
			state1[i] = true;
			state2[i] = true;
		}
	}

	const double start = Sys_FloatTime();
	while (count)
	{
		count = 0;
		for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		{
			if (!state1[i])
			{
				if (CanSendMessage(host_client->netconnection))
				{
					state1[i] = true;
					SendMessage(host_client->netconnection, data);
				}
				else
				{
					GetMessage(host_client->netconnection);
				}
				count++;
				continue;
			}

			if (!state2[i])
			{
				if (CanSendMessage(host_client->netconnection))
				{
					state2[i] = true;
				}
				else
				{
					GetMessage(host_client->netconnection);
				}
				count++;
				continue;
			}
		}
		if ((Sys_FloatTime() - start) > blocktime)
			break;
	}
	return count;
}

void CGNSNetSystem::Close(qsocket_t* conn)
{
	if (!conn || conn->connection == k_HSteamNetConnection_Invalid)
		return;

	SetNetTime();

	m_GNSInterface->CloseConnection(conn->connection, k_ESteamNetConnectionEnd_App_Generic, "Closing connection", false);

	m_Connections.erase(std::remove_if(m_Connections.begin(), m_Connections.end(), [&](const auto& other)
		{
			return other.get() == conn;
		}), m_Connections.end());
}

void CGNSNetSystem::RunFrame()
{
	m_GNSInterface->RunCallbacks();
}

bool CGNSNetSystem::IsValidConnection(qsocket_t* conn) const
{
	return conn && m_GNSInterface->GetConnectionInfo(conn->connection, nullptr);
}

void CGNSNetSystem::CloseSocket()
{
	if (m_GNSSocket != k_HSteamListenSocket_Invalid)
	{
		m_GNSInterface->CloseListenSocket(m_GNSSocket);
		m_GNSSocket = k_HSteamListenSocket_Invalid;
	}
}

void CGNSNetSystem::OnServerSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	switch (pInfo->m_info.m_eState)
	{
	case k_ESteamNetworkingConnectionState_None:
		// NOTE: We will get callbacks here when we destroy connections. You can ignore these.
		break;

	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		// Ignore if they were not previously connected.  (If they disconnected
		// before we accepted the connection.)
		if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
		{
			// Locate the client.  Note that it should have been found, because this
			// is the only codepath where we remove clients (except on shutdown),
			// and connection change callbacks are dispatched in queue order.
			auto connection = [&]() -> qsocket_t*
			{
				for (auto& connection : m_Connections)
				{
					if (connection->connection == pInfo->m_hConn)
					{
						return connection.get();
					}
				}

				return nullptr;
			}();

			// Select appropriate log messages
			const char* pszDebugLogAction;
			if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
			{
				pszDebugLogAction = "problem detected locally";
			}
			else
			{
				// Note that here we could check the reason code to see if
				// it was a "usual" connection or an "unusual" one.
				pszDebugLogAction = "closed by peer";
			}

			// Spew something to our own log.  Note that because we put their nick
			// as the connection description, it will show up, along with their
			// transport-specific data (e.g. their IP address)
			Con_Printf("Connection %s %s, reason %d: %s\n",
				pInfo->m_info.m_szConnectionDescription,
				pszDebugLogAction,
				pInfo->m_info.m_eEndReason,
				pInfo->m_info.m_szEndDebug
			);

			Close(connection);
			break;
		}

		// Clean up the connection.  This is important!
		// The connection is "closed" in the network sense, but
		// it has not been destroyed.  We must close it on our end, too
		// to finish up.  The reason information do not matter in this case,
		// and we cannot linger because it's already closed on the other end,
		// so we just pass 0's.
		m_GNSInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
		break;
	}

	case k_ESteamNetworkingConnectionState_Connecting:
	{
		Con_Printf("Connection request from %s", pInfo->m_info.m_szConnectionDescription);

		// A client is attempting to connect
		// Try to accept the connection.
		if (m_GNSInterface->AcceptConnection(pInfo->m_hConn) != k_EResultOK)
		{
			// This could fail.  If the remote host tried to connect, but then
			// disconnected, the connection may already be half closed.  Just
			// destroy whatever we have on our side.
			m_GNSInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
			Con_Printf("Can't accept connection. (It was already closed?)\n");
			break;
		}

		auto connection = std::make_unique<qsocket_t>();

		connection->connection = pInfo->m_hConn;
		connection->connecttime = net_time;

		connection->address.resize(SteamNetworkingIPAddr::k_cchMaxString);
		pInfo->m_info.m_addrRemote.ToString(connection->address.data(), connection->address.size(), true);

		m_Connections.push_back(std::move(connection));

		SV_NewClient(m_Connections.back().get());
		break;
	}

	case k_ESteamNetworkingConnectionState_Connected:
		// We will get a callback immediately after accepting the connection.
		// Since we are the server, we can ignore this, it's not news to us.
		break;

	default:
		// Silences -Wswitch
		break;
	}
}

void CGNSNetSystem::OnClientSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	switch (pInfo->m_info.m_eState)
	{
	case k_ESteamNetworkingConnectionState_None:
		// NOTE: We will get callbacks here when we destroy connections. You can ignore these.
		break;

	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		// Print an appropriate message
		if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
		{
			// Note: we could distinguish between a timeout, a rejected connection,
			// or some other transport problem.
			Con_Printf("We sought the remote host, yet our efforts were met with defeat. (%s)\n", pInfo->m_info.m_szEndDebug);
		}
		else if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
		{
			Con_Printf("Alas, troubles beset us; we have lost contact with the host. (%s)\n", pInfo->m_info.m_szEndDebug);
		}
		else
		{
			// NOTE: We could check the reason code for a normal disconnection
			Con_Printf("The host hath bidden us farewell. (%s)\n", pInfo->m_info.m_szEndDebug);
		}

		// Clean up the connection.  This is important!
		// The connection is "closed" in the network sense, but
		// it has not been destroyed.  We must close it on our end, too
		// to finish up.  The reason information do not matter in this case,
		// and we cannot linger because it's already closed on the other end,
		// so we just pass 0's.
		m_GNSInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
		//m_hConnection = k_HSteamNetConnection_Invalid;
		break;
	}

	case k_ESteamNetworkingConnectionState_Connecting:
		// We will get this callback when we start connecting.
		// We can ignore this.
		break;

	case k_ESteamNetworkingConnectionState_Connected:
		Con_DPrintf("Connected to server OK");
		break;

	default:
		// Silences -Wswitch
		break;
	}
}
