#ifndef NETUTILS_HPP
#define NETUTILS_HPP

class IPv4Address
{
private:
	unsigned char addr[4];

public:
	IPv4Address(int octet_a, int octet_b, int octet_c, int octet_d);
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
	char* send(char* data_begin, char* data_end, IPv4Address destination);
	char* receive(char* data_begin, char* data_end, IPv4Address& source);
};

#endif