#pragma once

#include "main.h"

namespace Renderer
{
	void RenderTexture(SDL_Texture* texture, int x, int y, int w, int h);
	void RenderText(string message, TTF_Font* font, SDL_Color color, int x, int y);
}