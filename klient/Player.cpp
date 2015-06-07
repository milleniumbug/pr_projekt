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
	switch (Direction)
	{
		case 0:	Textures->current = Textures->up; break;
		case 1:	Textures->current = Textures->right; break;
		case 2:	Textures->current = Textures->down;	break;
		case 3: Textures->current = Textures->left; break;
	}
	Renderer::RenderTexture(Textures->current, X, Y, Size, Size);
}