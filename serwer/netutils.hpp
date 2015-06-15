#ifndef NETUTILS_HPP
#define NETUTILS_HPP

#include <array>

class IPv4Address
{
private:
	std::array<unsigned char, 4> addr;

public:
	uint32_t as_uint();
	IPv4Address(int octet_a, int octet_b, int octet_c, int octet_d);
	IPv4Address();

	friend bool operator==(const IPv4Address& lhs, const IPv4Address& rhs)
	{
		return std::equal(lhs.addr.begin(), lhs.addr.end(), rhs.addr.begin());
	}

	friend bool operator!=(const IPv4Address& lhs, const IPv4Address& rhs)
	{
		return !(lhs == rhs);
	}
};

struct SocketError : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

class FileDescriptor
{
	int fd_;
public:
	~FileDescriptor();
	FileDescriptor(const FileDescriptor&) = delete;
	FileDescriptor(FileDescriptor&& other);
	FileDescriptor& operator=(FileDescriptor other);
	FileDescriptor(int descriptor);
	FileDescriptor();
	
	bool valid() { return fd() != -1; }
	int fd() { return fd_; }
	operator int() { return fd(); }
};

class UDPSocket
{
private:
	FileDescriptor fd_;
	struct sockaddr_in sa_;
public:
	UDPSocket(int port);
	
	int fd() { return fd_.fd(); }
	const char* send(const char* data_begin, const char* data_end, IPv4Address destination, int port);
	char* receive(char* data_begin, char* data_end, IPv4Address& source, int& port);
};

bool initialize_networking();

#endif