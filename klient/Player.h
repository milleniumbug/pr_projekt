#pragma once

#include "TexArray.h"

class Player
{
	public:
	int ID; //0 - top left, 1 - top right, 2 - down left, 3 - down right
	int Direction; //0 - up, 1 - right, 2 - down, 3 - left
	int X;
	int Y;
	int Size;
	TexArray* Textures;

	Player(int id, int dir, int x, int y, int size, string bmpPath);
	~Player();

	int MapX(int offset = 0);
	int MapY(int offset = 0);
	void Render();
};