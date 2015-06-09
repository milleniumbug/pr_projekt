#include "Player.h"
#include "Renderer.h"

Player::Player(int id, int dir, int mapX, int mapY, int size, string bmpPath)
{
	ID = id;
	Direction = dir;
	X = mapX * TILE_SIZE;
	Y = mapY * TILE_SIZE;
	Size = size;
	Bonus = 0;
	WalkProgress = 0;
	Textures = new TexArray(bmpPath);
}

Player::~Player()
{
	delete Textures;
}

int Player::MapX(int offset)
{
	return (X + offset) / TILE_SIZE;
}

int Player::MapY(int offset)
{
	return (Y + offset) / TILE_SIZE;
}

void Player::Render()
{
	int xDiff = 0;
	int yDiff = 0;
	if (Direction != 0)
		int a = 0;
	switch (Direction)
	{
		case -1: Textures->current = Textures->up; yDiff = -(int)((float)TILE_SIZE * ((float)WalkProgress / 100.0f)); break;
		case 1: Textures->current = Textures->down; yDiff = (int)((float)TILE_SIZE * ((float)WalkProgress / 100.0f)); break;
		case -4: Textures->current = Textures->left; xDiff = -(int)((float)TILE_SIZE * ((float)WalkProgress / 100.0f)); break;
		case 4: Textures->current = Textures->right; xDiff = (int)((float)TILE_SIZE * ((float)WalkProgress / 100.0f)); break;
	}
	Renderer::RenderTexture(Textures->current, X + xDiff, Y + yDiff, Size, Size);
}