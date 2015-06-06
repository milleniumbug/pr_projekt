#include "Renderer.h"

namespace Renderer
{
	void RenderTexture(SDL_Texture* texture, int x, int y, int w, int h)
	{
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = w;
		rect.h = h;
		SDL_Rect rect2 = rect;
		rect2.x = x;
		rect2.y = y;
		SDL_RenderCopy(ren, texture, &rect, &rect2);
	}

	void RenderText(string message, TTF_Font* renderFont, SDL_Color color, int x, int y)
	{
		SDL_Surface* surfaceMessage = TTF_RenderText_Solid(renderFont, message.c_str(), color);
		SDL_Texture* Message = SDL_CreateTextureFromSurface(ren, surfaceMessage);
		SDL_FreeSurface(surfaceMessage);

		SDL_Rect Message_rect;
		int w, h;
		TTF_SizeText(font, message.c_str(), &w, &h);
		Message_rect.x = x;
		Message_rect.y = y;
		Message_rect.w = w;
		Message_rect.h = h;

		SDL_RenderCopy(ren, Message, NULL, &Message_rect);
		SDL_DestroyTexture(Message);
	}
}