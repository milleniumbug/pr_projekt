#include "main.h"

#include <string>
#include <Windows.h>
#include <SDL2_rotozoom.h>
#include "TexArray.h"
#include "Loader.h"
#include "Player.h"
#include "Renderer.h"


using namespace std;
string mainPath;

SDL_Renderer* ren = NULL;
TTF_Font* font = NULL;
bool doExit = false;
SDL_Color red;
SDL_Color white = {255, 255, 255, 255};
SDL_Color colorKey = {255, 255, 0, 255};
int ticks = 0;
int framesCount = 0;
int tempFramesCount = 0;

//magia visuala, mam zalinkowan¹ bibliotekê we w³aœciwoœciach projektu, ale ten jej nie widzi ;D
//a ta linijka magicznie dzia³a o.o
#pragma comment(lib, "..\\SDL2_gfx-1.0.1\\lib\\SDL2_gfx.lib")

SDL_Texture* texBlock;
Player* p1 = NULL;
Player* p2 = NULL;
Player* p3 = NULL;
Player* p4 = NULL;
Player* controlledPlayer = NULL;

int windowW = 768, windowH = 768;
int playerSize = 48;
int tileSize = 64;

class Block
{
	public:
		int type;
		int size;
		int x;
		int y;

	Block()
	{
		type = 0;
		size = 0;
		x = 0;
		y = 0;
	}

	Block(int Type, int Size, int X, int Y)
	{
		type = Type;
		size = Size;
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
			map[i][j] = Block(rand() % 2, 64, i, j);

	//piêkne linijki - wyczyszczenie rogów mapy, tak aby by³o miejsce gdzie postawiæ bombê
	map[0][0] = Block(0, 64, 0, 0);
	map[0][1] = Block(0, 64, 0, 1);
	map[1][0] = Block(0, 64, 1, 0);
	map[11][0] = Block(0, 64, 11, 0);
	map[11][1] = Block(0, 64, 11, 1);
	map[10][0] = Block(0, 64, 10, 0);
	map[11][10] = Block(0, 64, 11, 0);
	map[11][11] = Block(0, 64, 11, 1);
	map[10][0] = Block(0, 64, 10, 0);
	map[0][10] = Block(0, 64, 11, 0);
	map[0][11] = Block(0, 64, 11, 1);
	map[1][11] = Block(0, 64, 10, 0);
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

int xMoveDir = 0;
int yMoveDir = 0;
double multiplyFactor = 1.0f;

bool leftPressed = false;
bool rightPressed = false;
bool upPressed = false;
bool downPressed = false;

void nextPlayer()
{
	if (controlledPlayer == p1)
		controlledPlayer = p2;
	else if (controlledPlayer == p2)
		controlledPlayer = p3;
	else if (controlledPlayer == p3)
		controlledPlayer = p4;
	else if (controlledPlayer == p4)
		controlledPlayer = p1;
}


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
		if (e.key.keysym.sym == SDLK_TAB)
			nextPlayer();
	}
}

int xDiff = 0;
int yDiff = 0;

int mapX = 0;
int mapY = 0;

bool collidesWithBlock(int playerX, int playerY, Block b)
{
	POINT TopLeft = {playerX, playerY}, TopRight = {playerX + 48, playerY}, BottomLeft = {playerX, playerY + 48}, BottomRight {playerX + 48, playerY + 48};
	if (TopLeft.x > b.x * 64 && TopLeft.x < b.x * 64 + 64 && TopLeft.y > b.y * 64 && TopLeft.y < b.y * 64 + 64)
		return true;
	if (TopRight.x > b.x * 64 && TopRight.x < b.x * 64 + 64 && TopRight.y > b.y * 64 && TopRight.y < b.y * 64 + 64)
		return true;
	if (BottomLeft.x > b.x * 64 && BottomLeft.x < b.x * 64 + 64 && BottomLeft.y > b.y * 64 && BottomLeft.y < b.y * 64 + 64)
		return true;
	if (BottomRight.x > b.x * 64 && BottomRight.x < b.x * 64 + 64 && BottomRight.y > b.y * 64 && BottomRight.y < b.y * 64 + 64)
		return true;
	return false;
}

//0 - no collision, 1 - collision on X, 2 - collision on Y, 3 - collision on X and Y
int collides()
{
	mapX = controlledPlayer->MapX(xDiff);
	mapY = controlledPlayer->MapY(yDiff);

	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			/*if (i == 0 && j == 0)
				continue;*/
			int X = mapX + i;
			int Y = mapY + j;
			if (X < 0 || X > 11 || Y < 0 || Y > 11)
				continue;
			Block b = map[X][Y];
			if (b.type != 0)
			{
				if (collidesWithBlock(controlledPlayer->X + xDiff, controlledPlayer->Y + yDiff, b))
					return true;
			}
		}
	}

	return 0;
}

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

		if (yDiff != 0 || xDiff != 0)
		{
			if (collides())
			{
				xDiff = 0;
				yDiff = 0;
			}
		}
		
		controlledPlayer->Y += yDiff;
		controlledPlayer->X += xDiff;

		if (yDiff < 0)
			controlledPlayer->Direction = 0;
		else if (yDiff > 0)
			controlledPlayer->Direction = 2;
		if (xDiff < 0)
			controlledPlayer->Direction = 3;
		else if (xDiff > 0)
			controlledPlayer->Direction = 1;

		if (controlledPlayer->X < 0)
			controlledPlayer->X = 0;
		if (controlledPlayer->Y < 0)
			controlledPlayer->Y = 0;
		if (controlledPlayer->Y > windowH - playerSize)
			controlledPlayer->Y = windowH - playerSize;
		if (controlledPlayer->X > windowW - playerSize)
			controlledPlayer->X = windowW - playerSize;

		Sleep(4);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		return 1;

	mainPath = Loader::GetDirectory();
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

	texBlock = Loader::LoadTexture(mainPath + "gfx\\block.bmp");
	p1 = new Player(0, 1, 0, 0, 48, mainPath + "gfx\\player1.bmp");
	p2 = new Player(1, 3, 11, 0, 48, mainPath + "gfx\\player2.bmp");
	p3 = new Player(2, 1, 0, 11, 48, mainPath + "gfx\\player3.bmp");
	p4 = new Player(3, 3, 11, 11, 48, mainPath + "gfx\\player4.bmp");
	controlledPlayer = p1;

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
					Renderer::RenderTexture(texBlock, i*tileSize, j * tileSize, tileSize, tileSize);
			}
		}
		p1->Render(); p2->Render(); p3->Render(); p4->Render();
		drawFPS();
		Renderer::RenderText("X: " + to_string(controlledPlayer->X) + " Y: " + to_string(controlledPlayer->Y), font, white, 10, 70);
		Renderer::RenderText("X: " + to_string(mapX) + " Y: " + to_string(mapY), font, red, 10, 100);
		SDL_RenderPresent(ren);
		//Sleep(33);
	}

	TerminateThread(moveThreadHandle, 0);
	TTF_CloseFont(font);
	SDL_DestroyTexture(texBlock);
	
	delete p1;
	delete p2;
	delete p3;
	delete p4;
	
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}