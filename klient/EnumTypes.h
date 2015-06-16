#pragma once

#include <Windows.h>

enum PacketType : byte
{
	ClientJoinGame = 0x01,
	ServerAccept = 0x02,
	ClientRequestGameState = 0x03,
	ServerSendGameState = 0x04,
	ClientRequestServerInfo = 0x05,
	ServerSendServerInfo = 0x06,
	ClientKeepAlive = 0x07,
	ClientDisconnect = 0x08,
	ClientPlayerAction = 0x09,
	ServerUnknownPacket = 0x71,
	ServerIncompatibleVersion = 0x72,
	ServerGameAlreadyStarted = 0x73,
	ServerIsFull = 0x74
};

enum GameState : byte
{
	Lobby = 0x01,
	Running = 0x02
};

enum BlockType : byte
{
	Empty = 0,
	Destructible = 1,
	Imdestructible = 2,
	Bomb = 3
};