#pragma once

#include "Packet.h"
#include <string>

using namespace std;

class Connection
{
	public:
	Connection();
	~Connection();

	void SetRecvTimeout(int miliseconds);
	int SetIP(string ip, int port);
	void Disconnect();
	int Send(Packet p);
	Packet Recv();

	SOCKET Socket;

	//0 - OK, 1 - Failed initialization
	int State;

	private:
	struct sockaddr_in si_other;
	int s, slen = sizeof(si_other);
	char buf[512];
	char message[512];
	WSADATA wsa;
};