#pragma once

#include <Windows.h>

class Packet
{
	public:

	Packet();
	~Packet();

	byte* Data;
	int Length;
	int AllocDataSize;

	int RecvResult;

	void AllocData(int size);
	void DeleteData();

	void AddByte(byte b);
	void AddShort(short s);
	void AddInt(int i);
	void AddLongLong(long long l);

	void RemoveAt(int index, int size);

	byte ReadByte();
	short ReadShort();
	int ReadInt();
	long long ReadLongLong();

	int ReadIndex;
};