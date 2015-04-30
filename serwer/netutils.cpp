#include <cstring>
#include <cerrno>
#include <cassert>
#include <utility>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

#include "netutils.hpp"

UDPSocket::UDPSocket(int port)
{
	fd_ = FileDescriptor(socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP));
	if(!fd_.valid())
	{
		// TODO: Add proper exceptions
		throw "FUCK";
	}
	std::memset(&sa_, 0, sizeof sa_);
	sa_.sin_family = AF_INET;
	sa_.sin_addr.s_addr = htonl(INADDR_ANY);
	sa_.sin_port = htons(port);
	if(bind(fd_,(struct sockaddr*)&sa_, sizeof(sa_)) == -1)
	{
		// TODO: Add proper exceptions
		throw "FUCK";
	}
}

char* UDPSocket::send(char* data_begin, char* data_end, IPv4Address destination)
{
	assert(false);

}

char* UDPSocket::receive(char* data_begin, char* data_end, IPv4Address& source)
{
	struct sockaddr_in sa;
	socklen_t fromlen = sizeof(sa);
	size_t distance = (data_end - data_begin);
	ssize_t ret = recvfrom(fd_.fd(), data_begin, distance, 0, (struct sockaddr*)&sa, &fromlen);
	if(ret == -1)
	{
		// TODO: Add proper exceptions
		throw "FUCK";
	}
	auto a = sa.sin_addr.s_addr;
	IPv4Address address((a >> 24)&0xFF, (a >> 16)&0xFF, (a >> 8)&0xFF, (a >> 0)&0xFF);
	source = address;
	return data_begin + ret;
}

FileDescriptor::~FileDescriptor()
{
	if(fd_ != -1)
		close(fd_);
}

FileDescriptor::FileDescriptor(FileDescriptor&& other)
{
	fd_ = other.fd_;
	other.fd_ = -1;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor other)
{
	std::swap(other.fd_, this->fd_);
	return *this;
}

FileDescriptor::FileDescriptor(int descriptor)
{
	fd_ = descriptor;
}

FileDescriptor::FileDescriptor()
{
	fd_ = -1;
}

IPv4Address::IPv4Address(int octet_a, int octet_b, int octet_c, int octet_d)
{
	addr[0] = octet_a;
	addr[1] = octet_b;
	addr[2] = octet_c;
	addr[3] = octet_d;
}