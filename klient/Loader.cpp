#include "Loader.h"
#include "main.h"

namespace Loader
{
	string GetDirectory()
	{
		string toRet = "";
		char* basePath = SDL_GetBasePath();
		if (basePath)
		{
			toRet = basePath;
			SDL_free(basePath);
		}
		else
			toRet = "error";
		return toRet;
	}

	SDL_Texture* LoadTexture(string path)
	{
		if (path.size() > 2) //Nice Windows Code ;D
		{
			if (path[1] != ':')
				path = GetDirectory() + path;
		}
		SDL_Surface* bmp = SDL_LoadBMP(path.c_str());
		/*bmp->format->Amask = 0xFF000000;
		bmp->format->Ashift = 24;*/
		SDL_SetColorKey(bmp, SDL_TRUE, SDL_MapRGB(bmp->format, 255, 0, 255));
		if (bmp == NULL)
			return NULL;

		SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, bmp);
		SDL_FreeSurface(bmp);

		if (tex == NULL)
			return NULL;

		return tex;
	}
}