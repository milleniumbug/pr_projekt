#include "Connection.h"
#include <winsock.h>

Connection::Connection()
{
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		State = WSAGetLastError();
		return;
	}
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		State = WSAGetLastError();
		return;
	}

	SetRecvTimeout(5000);
	
	State = 0;
}

Connection::~Connection()
{
	closesocket(Socket);
	WSACleanup();
}

void Connection::SetRecvTimeout(int miliseconds) 
{ //TIMEOUT na RECVFROM, wg MSDN to dzia³a od Win 8.1, ale na moim 7 te¿ hula :D
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&miliseconds, 4);
}

int Connection::SetIP(string ip, int port)
{
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	si_other.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
	return 0;
}

void Connection::Disconnect()
{
	closesocket(Socket);
	WSACleanup();
}

int Connection::Send(Packet p)
{
	return sendto(s, (const char*)p.Data, p.Length, 0, (sockaddr*)&si_other, slen);
}

Packet Connection::Recv()
{
	Packet p;
	p.AllocData(512);
	int result = recvfrom(s, (char*)p.Data, p.AllocDataSize, 0, (sockaddr*)&si_other, &slen);
	p.RecvResult = result;
	return p;
}