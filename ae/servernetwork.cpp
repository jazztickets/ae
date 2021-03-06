/******************************************************************************
* Copyright (c) 2021 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#include <ae/servernetwork.h>
#include <ae/peer.h>
#include <ae/buffer.h>
#include <enet/enet.h>
#include <stdexcept>

namespace ae {

// Constructor
_ServerNetwork::_ServerNetwork(std::size_t MaxPeers, uint16_t Port) {
	ENetAddress Address;
	Address.host = ENET_HOST_ANY;
	Address.port = Port;

	// Create listener connection
	Connection = enet_host_create(&Address, MaxPeers, 0, 0, 0);
}

// Destructor
_ServerNetwork::~_ServerNetwork() {
	ClearPeers();
}

// Create ping socket
void _ServerNetwork::CreatePingSocket(uint16_t Port) {

	// Set ping socket options
	enet_socket_set_option(PingSocket, ENET_SOCKOPT_REUSEADDR, 1);
	enet_socket_set_option(PingSocket, ENET_SOCKOPT_NONBLOCK, 1);

	// Bind socket
	ENetAddress Address;
	Address.host = ENET_HOST_ANY;
	Address.port = Port;
	enet_socket_bind(PingSocket, &Address);
}

// Get port server is listening on
uint16_t _ServerNetwork::GetListenPort() {
	if(!Connection)
		return 0;

	return Connection->address.port;
}

// Get max number of peers
std::size_t _ServerNetwork::GetMaxPeers() {
	if(!Connection)
		return 0;

	return Connection->peerCount;
}

// Delete a peer and remove from the list
void _ServerNetwork::DeletePeer(_Peer *Peer) {

	// Delete peer
	for(auto Iterator = Peers.begin(); Iterator != Peers.end(); ++Iterator) {
		if((*Iterator) == Peer) {
			Peers.erase(Iterator);
			delete Peer;
			break;
		}
	}
}

// Clear the peer list out
void _ServerNetwork::ClearPeers() {

	// Delete peers
	for(auto &Peer : Peers)
		delete Peer;

	Peers.clear();
}

// Disconnect a single peer
void _ServerNetwork::DisconnectPeer(const _Peer *Peer, int Data) {
	if(!Peer || !Peer->ENetPeer)
		return;

	enet_peer_disconnect(Peer->ENetPeer, Data);
}

// Disconnect all peers
void _ServerNetwork::DisconnectAll(int Data) {

	// Disconnect all connected peers
	for(auto &Peer : Peers)
		enet_peer_disconnect(Peer->ENetPeer, Data);
}

// Create a _NetworkEvent from an enet event
void _ServerNetwork::CreateEvent(_NetworkEvent &Event, double EventTime, ENetEvent &EEvent) {
	Event.Time = EventTime;
	Event.Type = _NetworkEvent::EventType(EEvent.type-1);
	Event.EventData = (int)EEvent.data;
	if(EEvent.peer->data)
		Event.Peer = (_Peer *)EEvent.peer->data;
}

// Handle the event internally
void _ServerNetwork::HandleEvent(_NetworkEvent &Event, ENetEvent &EEvent) {

	// Add peer
	switch(Event.Type) {
		case _NetworkEvent::CONNECT: {

			// Create peer
			Event.Peer = new _Peer(EEvent.peer);

			// Set peer in enet's peer
			EEvent.peer->data = Event.Peer;
			Peers.push_back(Event.Peer);
		} break;
		case _NetworkEvent::DISCONNECT:
		break;
		case _NetworkEvent::PACKET: {
			Event.Data = new _Buffer((char *)EEvent.packet->data, EEvent.packet->dataLength);
			enet_packet_destroy(EEvent.packet);
		} break;
	}
}

// Send a packet
void _ServerNetwork::SendPacket(const _Buffer &Buffer, const _Peer *Peer, SendType Type, uint8_t Channel) {
	if(!Peer->ENetPeer)
		return;

	// Create enet packet
	ENetPacket *EPacket = enet_packet_create(Buffer.GetData(), Buffer.GetCurrentSize(), Type);

	// Send packet
	if(enet_peer_send(Peer->ENetPeer, Channel, EPacket) != 0)
		enet_packet_destroy(EPacket);
}

// Send a packet to all peers
void _ServerNetwork::BroadcastPacket(const _Buffer &Buffer, _Peer *ExceptionPeer, SendType Type, uint8_t Channel) {

	for(auto &Peer : Peers) {
		if(Peer != ExceptionPeer && Peer->Object)
			SendPacket(Buffer, Peer, Type, Channel);
	}
}

}
