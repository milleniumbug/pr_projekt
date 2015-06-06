#pragma once

#include <string>
#include <SDL.h>

using namespace std;

namespace Loader
{
	string GetDirectory();
	SDL_Texture* LoadTexture(string path);
}