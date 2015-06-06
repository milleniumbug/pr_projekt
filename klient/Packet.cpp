#include "Packet.h"


Packet::Packet()
{
	Data = NULL;
	Length = 0;
	AllocDataSize = 0;
	RecvResult = -1000;
	ReadIndex = 0;
}

Packet::~Packet()
{
	/*delete[] Data;
	Data = NULL;*/
}

void Packet::AllocData(int size)
{
	if (Data != NULL)
	{
		byte* oldData = Data;
		Data = new byte[size];
		memcpy(Data, oldData, Length);
		AllocDataSize = size;
		delete[] oldData;
	}
	else
	{
		AllocDataSize = size;
		Data = new byte[size];
	}
}

void Packet::DeleteData()
{
	delete[] Data;
	Data = NULL;
	AllocDataSize = 0;
	Length = 0;
}

void Packet::AddByte(byte b)
{
	if (Length == AllocDataSize)
		AllocData(Length + 1);
	memcpy(&Data[Length++], &b, 1);
}

void Packet::AddShort(short s)
{
	if (Length + 2 >= AllocDataSize)
		AllocData(Length + 2);
	short s2 = _byteswap_ushort(s);
	memcpy(&Data[Length], &s2, 2);
	Length += 2;
}

void Packet::AddInt(int i)
{
	if (Length + 4 >= AllocDataSize)
		AllocData(Length + 4);
	int i2 = _byteswap_ulong(i);
	memcpy(&Data[Length], &i2, 4);
	Length += 4;
}

void Packet::AddLongLong(long long l)
{
	if (Length + 8 >= AllocDataSize)
		AllocData(Length + 8);
	long long l2 = _byteswap_uint64(l);
	memcpy(&Data[Length], &l2, 8);
	Length += 8;
}

byte Packet::ReadByte()
{
	return Data[ReadIndex++];
}

short Packet::ReadShort()
{
	short s = 0;
	memcpy(&s, Data + ReadIndex, 2);
	ReadIndex += 2;
	return _byteswap_ushort(s);
}

int Packet::ReadInt()
{
	int i = 0;
	memcpy(&i, Data + ReadIndex, 4);
	ReadIndex += 4;
	return _byteswap_ulong(i);
}

long long Packet::ReadLongLong()
{
	long long l = 0;
	memcpy(&l, Data + ReadIndex, 8);
	ReadIndex += 8;
	return _byteswap_uint64(l);
}