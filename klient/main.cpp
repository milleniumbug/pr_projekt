#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <Windows.h>
#include <SDL2_rotozoom.h>
#include "TexArray.h"


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

//magia visuala, mam zalinkowan¹ bibliotekê we w³aœciwoœciach projektu, ale ten jej nie widzi ;D
//a ta linijka magicznie dzia³a o.o
#pragma comment(lib, "..\\SDL2_gfx-1.0.1\\lib\\SDL2_gfx.lib")

SDL_Texture* texBlock;
TexArray* texPlayer1;
TexArray* texPlayer2;
TexArray* texPlayer3;
TexArray* texPlayer4;

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

	//piêkne linijki - wyczyszczenie rogów mapy, tak aby by³o miejsce gdzie postawiæ bombê
	map[0][0] = Block(0, 0, 0);
	map[0][1] = Block(0, 0, 1);
	map[1][0] = Block(0, 1, 0);
	map[11][0] = Block(0, 11, 0);
	map[11][1] = Block(0, 11, 1);
	map[10][0] = Block(0, 10, 0);
	map[11][10] = Block(0, 11, 0);
	map[11][11] = Block(0, 11, 1);
	map[10][0] = Block(0, 10, 0);
	map[0][10] = Block(0, 11, 0);
	map[0][11] = Block(0, 11, 1);
	map[1][11] = Block(0, 10, 0);
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
	SetWindowTextA(GetActiveWindow(), ("FPS: " + to_string(framesCount)).c_str());
	//renderText("FPS: " + to_string(framesCount), red, 0, 0);
}

int x = 0;
int y = 0;
int xMoveDir = 0;
int yMoveDir = 0;
double multiplyFactor = 1.0f;

bool leftPressed = false;
bool rightPressed = false;
bool upPressed = false;
bool downPressed = false;


void HandleKeyboard(SDL_Event e)
{
	if (e.type == SDL_KEYDOWN)
	{
		if (e.key.keysym.sym == SDLK_UP)
			upPressed = true;
		if (e.key.keysym.sym == SDLK_DOWN)
			downPressed = true;
		if (e.key.keysym.sym == SDLK_LEFT)
			leftPressed = true;
		if (e.key.keysym.sym == SDLK_RIGHT)
			rightPressed = true;
		if (e.key.keysym.sym == SDLK_LSHIFT)
			multiplyFactor = 0.5f;
	}
	else // if (e.type == SDL_KEYUP)
	{
		if (e.key.keysym.sym == SDLK_UP)
			upPressed = false;
		else if (e.key.keysym.sym == SDLK_DOWN)
			downPressed = false;
		else if (e.key.keysym.sym == SDLK_LEFT)
			leftPressed = false;
		else if (e.key.keysym.sym == SDLK_RIGHT)
			rightPressed = false;
		if (e.key.keysym.sym == SDLK_ESCAPE)
			doExit = true;
		if (e.key.keysym.sym == SDLK_LSHIFT)
			multiplyFactor = 1.0f;
	}
}

int xDiff = 0;
int yDiff = 0;
//async * 2ms
void moveThread()
{
	while (true)
	{
		xDiff = 0;
		yDiff = 0;
		if (leftPressed)
			xDiff -= 2 * multiplyFactor;
		if (rightPressed)
			xDiff += 2 * multiplyFactor;
		if (upPressed)
			yDiff -= 2 * multiplyFactor;
		if (downPressed)
			yDiff += 2 * multiplyFactor;
		
		y += yDiff;
		x += xDiff;

		if (yDiff < 0)
			texPlayer1->current = texPlayer1->up;
		else if (yDiff > 0)
			texPlayer1->current = texPlayer1->down;
		if (xDiff < 0)
			texPlayer1->current = texPlayer1->left;
		else if (xDiff > 0)
			texPlayer1->current = texPlayer1->right;

		if (x < 0)
			x = 0;
		if (y < 0)
			y = 0;
		if (y > windowH - 64)
			y = windowH - 64;
		if (x > windowW - 64)
			x = windowW - 64;

		Sleep(4);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		return 1;

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

	texBlock = LoadTexture(mainPath + "gfx\\block.bmp");
	texPlayer1 = new TexArray(mainPath + "gfx\\player1.bmp", ren);
	texPlayer2 = new TexArray(mainPath + "gfx\\player2.bmp", ren);
	texPlayer3 = new TexArray(mainPath + "gfx\\player3.bmp", ren);
	texPlayer4 = new TexArray(mainPath + "gfx\\player4.bmp", ren);

	HANDLE moveThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)moveThread, NULL, NULL, NULL);

	SDL_Event e;
	while (!doExit)
	{
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
				doExit = true;
			if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
				HandleKeyboard(e);
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
					renderTexture(texBlock, i*64,  j*64, 64, 64);
			}
		}
		renderTexture(texPlayer1->current, x, y, 64, 64);
		//renderTexture(blockTexture, 0, 0, 64, 64);
		drawFPS();
		SDL_RenderPresent(ren);
		//Sleep(33);
	}

	TerminateThread(moveThreadHandle, 0);
	TTF_CloseFont(font);
	SDL_DestroyTexture(texBlock);
	
	delete texPlayer1;
	delete texPlayer2;
	delete texPlayer3;
	delete texPlayer4;
	
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}