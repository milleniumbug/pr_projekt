#include "TexArray.h"
#include <SDL.h>
#include <SDL2_rotozoom.h>

TexArray::TexArray(string path, SDL_Renderer* ren)
{
	SDL_Surface* surfaceUp = SDL_LoadBMP(path.c_str());
	SDL_Surface* surfaceDown = rotateSurface90Degrees(surfaceUp, 2);
	SDL_Surface* surfaceRight = rotateSurface90Degrees(surfaceUp, 1);
	SDL_Surface* surfaceLeft = rotateSurface90Degrees(surfaceUp, 3);

	up = SDL_CreateTextureFromSurface(ren, surfaceUp);
	down = SDL_CreateTextureFromSurface(ren, surfaceDown);
	left = SDL_CreateTextureFromSurface(ren, surfaceLeft);
	right = SDL_CreateTextureFromSurface(ren, surfaceRight);
	current = up;

	SDL_FreeSurface(surfaceUp);
	SDL_FreeSurface(surfaceDown);
	SDL_FreeSurface(surfaceLeft);
	SDL_FreeSurface(surfaceRight);
	this->ren = ren;
}

TexArray::~TexArray()
{
	SDL_DestroyTexture(up);
	SDL_DestroyTexture(down);
	SDL_DestroyTexture(left);
	SDL_DestroyTexture(right);
}