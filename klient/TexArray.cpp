#include "TexArray.h"
#include "main.h"
#include <SDL2_rotozoom.h>

TexArray::TexArray(string path)
{
	SDL_Surface* surfaceUp = SDL_LoadBMP(path.c_str());
	SDL_SetColorKey(surfaceUp, SDL_TRUE, SDL_MapRGB(surfaceUp->format, 255, 0, 255));
	SDL_Surface* surfaceDown = rotateSurface90Degrees(surfaceUp, 2);
	SDL_SetColorKey(surfaceDown, SDL_TRUE, SDL_MapRGB(surfaceDown->format, 255, 0, 255));
	SDL_Surface* surfaceRight = rotateSurface90Degrees(surfaceUp, 1);
	SDL_SetColorKey(surfaceRight, SDL_TRUE, SDL_MapRGB(surfaceRight->format, 255, 0, 255));
	SDL_Surface* surfaceLeft = rotateSurface90Degrees(surfaceUp, 3);
	SDL_SetColorKey(surfaceLeft, SDL_TRUE, SDL_MapRGB(surfaceLeft->format, 255, 0, 255));

	up = SDL_CreateTextureFromSurface(ren, surfaceUp);
	down = SDL_CreateTextureFromSurface(ren, surfaceDown);
	left = SDL_CreateTextureFromSurface(ren, surfaceLeft);
	right = SDL_CreateTextureFromSurface(ren, surfaceRight);
	current = up;

	//nie ogarniam czemu to powoduje access violation ^^
	/*SDL_FreeSurface(surfaceUp);
	SDL_FreeSurface(surfaceDown);
	SDL_FreeSurface(surfaceLeft);
	SDL_FreeSurface(surfaceRight);*/
}

TexArray::~TexArray()
{
	SDL_DestroyTexture(up);
	SDL_DestroyTexture(down);
	SDL_DestroyTexture(left);
	SDL_DestroyTexture(right);
}