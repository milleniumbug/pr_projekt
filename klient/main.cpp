#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <Windows.h>

using namespace std;

string mainPath;

SDL_Renderer* ren = NULL;
bool doExit = false;
TTF_Font* font = NULL;
SDL_Color red;
SDL_Color colorKey = {255, 255, 0, 255};
int ticks = 0;
int framesCount = 0;
int tempFramesCount = 0;

int windowW = 768, windowH = 768;

class Block
{
	public:
		int type;
		int x;
		int y;

	Block()
	{
		type = 0;
		x = 0;
		y = 0;
	}

	Block(int Type, int X, int Y)
	{
		type = Type;
		x = X;
		y = Y;
	}
};

Block map[12][12];
SDL_Texture** blockTextures;

void generateMap()
{
	srand(GetTickCount());
	for (int i = 0; i < 12; i++)
		for (int j = 0; j < 12; j++)
			map[i][j] = Block(rand() % 2, i, j);
}

string getDirectory()
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
	SDL_Surface* bmp = SDL_LoadBMP(path.c_str());
	SDL_SetColorKey(bmp, SDL_TRUE, SDL_MapRGB(bmp->format, 255, 0, 255));
	if (bmp == NULL)
		return NULL;

	SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, bmp);
	SDL_FreeSurface(bmp);

	if (tex == NULL)
		return NULL;

	return tex;
}

void renderTexture(SDL_Texture* texture, int x, int y, int w, int h)
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

void renderText(std::string message, SDL_Color color, int x, int y)
{
	SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, message.c_str(), color);
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

void drawFPS()
{
	tempFramesCount++;
	if (GetTickCount() - ticks >= 1000)
	{
		framesCount = tempFramesCount;
		tempFramesCount = 0;
		ticks = GetTickCount();
	}
	renderText("FPS: " + to_string(framesCount), red, 0, 0);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		return 1;
	}

	mainPath = getDirectory();
	if (mainPath == "error")
		return 1;
	SDL_Window* win = SDL_CreateWindow("Hello World!", 100, 100, windowW, windowH, SDL_WINDOW_SHOWN);
	if (win == NULL)
	{
		SDL_Quit();
		return 1;
	}
	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	if (ren == nullptr)
	{
		SDL_DestroyWindow(win);
		SDL_Quit();
		return 1;
	}

	if (TTF_Init() != 0)
	{
		SDL_Quit();
		return 1;
	}

	font = TTF_OpenFont((mainPath + "gfx\\arial.ttf").c_str(), 12);
	if (font == NULL)
	{
		SDL_Quit();
		return 1;
	}

	red.a = 255;
	red.r = 255;
	red.b = 0;
	red.g = 0;
	generateMap();

	ticks = GetTickCount();

	SDL_Texture* blockTexture = LoadTexture(mainPath + "gfx\\block.bmp");
	SDL_Event e;
	while (!doExit)
	{
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
				doExit = true;
			if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_a)
					doExit = true;
			}
		}
		//Render the scene
		SDL_RenderClear(ren);
		SDL_Rect fillRect = {windowH, windowW, 0, 0}; 
		SDL_SetRenderDrawColor(ren, 0, 200, 0, 255); 
		SDL_RenderFillRect(ren, &fillRect);
		for (int i = 0; i < 12; i++)
		{
			for (int j = 0; j < 12; j++)
			{
				if (map[i][j].type != 0)
					renderTexture(blockTexture,i*64, j*64, 64, 64);
			}
		}
		//renderTexture(blockTexture, 0, 0, 64, 64);
		drawFPS();
		SDL_RenderPresent(ren);
	}
	TTF_CloseFont(font);
	SDL_DestroyTexture(blockTexture);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}