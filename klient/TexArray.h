#pragma once

#include <string>
#include <SDL.h>

using namespace std;

class TexArray
{
	public:
	TexArray(string path, SDL_Renderer* ren);

	~TexArray();

	SDL_Texture* up;
	SDL_Texture* down;
	SDL_Texture* left;
	SDL_Texture* right;
	SDL_Texture* current;

	SDL_Renderer* ren;
};