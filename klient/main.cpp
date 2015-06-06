#include "main.h"

#include <string>
#include <Windows.h>
#include <SDL2_rotozoom.h>
#include "TexArray.h"
#include "Loader.h"
#include "Player.h"
#include "Renderer.h"
#include "Connection.h"


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
int mapSize = 13;
int versionNumber = 1;

//magia visuala, mam zalinkowan¹ bibliotekê we w³aœciwoœciach projektu, ale ten jej nie widzi ;D
//a ta linijka magicznie dzia³a o.o
#pragma comment(lib, "..\\SDL2_gfx-1.0.1\\lib\\SDL2_gfx.lib")
#pragma comment(lib,"ws2_32.lib")

SDL_Texture* texBlock;
SDL_Texture* arrowRight;
Player* p1 = NULL;
Player* p2 = NULL;
Player* p3 = NULL;
Player* p4 = NULL;
Player* controlledPlayer = NULL;

int tileSize = 64;
int windowW = mapSize * tileSize, windowH = mapSize * tileSize;
int playerSize = 48;

enum class PacketType : byte
{
	ClientJoinGame = 0x01,
	ServerAccept = 0x02,
	ClientRequestGameState = 0x03,
	ServerSendGameState = 0x04,
	ClientRequestServerInfo = 0x05,
	ServerSendServerInfo = 0x06,
	ClientKeepAlive = 0x07,
	ClientDisconnect = 0x08,
	ClientPlayerAction = 0x09,
	ServerUnknownPacket = 0x71,
	ServerIncompatibleVersion = 0x72,
	ServerGameAlreadyStarted = 0x73,
	ServerIsFull = 0x74
};

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

Block** map;
SDL_Texture** blockTextures;

void generateMap()
{
	srand(GetTickCount());
	map = new Block*[mapSize];
	for (int i = 0; i < mapSize; i++)
	{
		map[i] = new Block[mapSize];
		for (int j = 0; j < mapSize; j++)
			map[i][j] = Block(rand() % 2, tileSize, i, j);
	}

	//piêkne linijki - wyczyszczenie rogów mapy, tak aby by³o miejsce gdzie postawiæ bombê
	map[0][0] = Block(0, tileSize, 0, 0);
	map[1][0] = Block(0, tileSize, 1, 0);
	map[0][1] = Block(0, tileSize, 0, 1);

	map[mapSize - 1][0] = Block(0, tileSize, mapSize - 1, 0);
	map[mapSize - 1][1] = Block(0, tileSize, mapSize - 1, 1);
	map[mapSize - 2][0] = Block(0, tileSize, mapSize - 2, 0);

	map[0][mapSize - 1] = Block(0, tileSize, 0, mapSize - 1);
	map[1][mapSize - 1] = Block(0, tileSize, 1, mapSize - 1);
	map[0][mapSize - 2] = Block(0, tileSize, 0, mapSize - 2);

	map[mapSize - 1][mapSize - 1] = Block(0, tileSize, mapSize - 1, mapSize - 1);
	map[mapSize - 1][mapSize - 2] = Block(0, tileSize, mapSize - 1, mapSize - 2);
	map[mapSize - 2][mapSize - 1] = Block(0, tileSize, mapSize - 2, mapSize - 1);
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

bool canBeIP(SDL_Keycode k)
{
	if ((k >= SDLK_a && k <= SDLK_z) || (k >= SDLK_0 && k <= SDLK_9) || (k == SDLK_PERIOD) || (k == SDLK_SEMICOLON) || (k == SDLK_BACKSPACE))
		return true;
	return false;
}

//Menu
//IP:PORT textfield
//Info bout server
//Connect
//Get Info
//Exit

Connection connection;

int selectedEntry = 0;
string infoString = "";
string ipPortString = "192.168.56.101:60000";

void GetServerInfo()
{
	infoString = "";
	string myIpPortString = ipPortString;
	string ip = "";
	int port = -1;

	char* pch;
	pch = strtok((char*)myIpPortString.c_str(), ":");
	int i = 0;
	while (pch != NULL)
	{
		if (i == 0)
		{
			ip = pch;
			i++;
			pch = strtok(NULL, ":");
		}
		else if (i == 1)
		{
			port = strtol(pch, NULL, 10);
			break;
		}
	}
	if (port == -1)
		port = 60000;

	connection.Connect(ip, port);
	Packet p;
	p.AllocData(5);
	p.AddInt(versionNumber);
	p.AddByte(5);
	connection.Send(p);
	p.DeleteData();
	Packet r = connection.Recv();
	if (r.RecvResult > 0)
	{
		int serverVersion = r.ReadInt();
		if (serverVersion != versionNumber)
		{
			infoString = "Wrong version number: " + to_string(serverVersion);
			r.DeleteData();
			return;
		}
		byte packetType = r.ReadByte();
		if (packetType != (byte)PacketType::ServerSendServerInfo)
		{
			r.DeleteData();
			infoString = "Broken server (?)";
			return;
		}
		byte connectedPlayers = r.ReadByte();
		byte maxPlayers = r.ReadByte();
		byte gameState = r.ReadByte();
		short xSize = r.ReadShort();
		short ySize = r.ReadShort();
		infoString.append("Server status: ");
		infoString.append(to_string(connectedPlayers));
		infoString.append("/");
		infoString.append(to_string(maxPlayers));
		if (gameState == 1)
			infoString.append(" Waiting for players");
		else
			infoString.append(" Game in progress");
		infoString.append(" MapSize: ");
		infoString.append(to_string(xSize));
		infoString.append(", ");
		infoString.append(to_string(ySize));
	}
	else
	{
		infoString = "Dupa nie polaczenie";
	}
	r.DeleteData();
}

void RenderMenu()
{
	Renderer::RenderText("Info :" + infoString, font, white, 100, 80);
	Renderer::RenderText("IP:PORT = " + ipPortString, font, white, 100, 100);
	Renderer::RenderText("Connect", font, white, 100, 120);
	Renderer::RenderText("Get Info", font, white, 100, 140);
	Renderer::RenderText("Exit", font, white, 100, 160);
	Renderer::RenderTexture(arrowRight, 60, 90 + selectedEntry*20, 32, 32);
}

void HandleMenuKeyboard(SDL_Event e)
{
	if (e.type == SDL_KEYUP)
	{
		if (e.key.keysym.sym == SDLK_UP)
			selectedEntry--;
		if (e.key.keysym.sym == SDLK_DOWN)
			selectedEntry++;
		if (selectedEntry < 0)
			selectedEntry = 0;
		if (selectedEntry > 3)
			selectedEntry = 3;

		if (canBeIP(e.key.keysym.sym) && selectedEntry == 0)
		{
			if (e.key.keysym.sym != SDLK_BACKSPACE && e.key.keysym.sym != SDLK_SEMICOLON)
				ipPortString.push_back(e.key.keysym.sym);
			
			else if (e.key.keysym.sym == SDLK_SEMICOLON && (e.key.keysym.mod & KMOD_LSHIFT || e.key.keysym.mod & KMOD_RSHIFT))
				ipPortString.push_back(':');
			else if (ipPortString.size() > 0 && e.key.keysym.sym == SDLK_BACKSPACE)
				ipPortString.pop_back();

		}

		if (e.key.keysym.sym == SDLK_RETURN)
		{
			switch (selectedEntry)
			{
				case 0:	break;
				case 1: /*TODO: Connect*/ break;
				case 2: CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)GetServerInfo, NULL, NULL, NULL); break;
				case 3: doExit = true; break;
			}
		}
	}
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
	POINT TopLeft = {playerX, playerY}, TopRight = {playerX + playerSize, playerY}, BottomLeft = {playerX, playerY + playerSize}, BottomRight {playerX + playerSize, playerY + playerSize};
	if (TopLeft.x > b.x * tileSize && TopLeft.x < b.x * tileSize + tileSize && TopLeft.y > b.y * tileSize && TopLeft.y < b.y * tileSize + tileSize)
		return true;
	if (TopRight.x > b.x * tileSize && TopRight.x < b.x * tileSize + tileSize && TopRight.y > b.y * tileSize && TopRight.y < b.y * tileSize + tileSize)
		return true;
	if (BottomLeft.x > b.x * tileSize && BottomLeft.x < b.x * tileSize + tileSize && BottomLeft.y > b.y * tileSize && BottomLeft.y < b.y * tileSize + tileSize)
		return true;
	if (BottomRight.x > b.x * tileSize && BottomRight.x < b.x * tileSize + tileSize && BottomRight.y > b.y * tileSize && BottomRight.y < b.y * tileSize + tileSize)
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
			if (X < 0 || X > mapSize - 1 || Y < 0 || Y > mapSize - 1)
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

void recvThread()
{
	while (true)
	{
		Packet pucket = connection.Recv();
		if (pucket.RecvResult >= 0)
			MessageBoxA(0, "Odbiooor kurwa", "Lal", MB_OK);
	}
}

void sendThread()
{
}

void initConnection()
{
	if (connection.State != 0)
	{
		MessageBoxA(0, "Cos sie wyjebao", "Lol", MB_OK);
		return;
	}
	int result = connection.Connect("192.168.56.101", 60000);
	if (result != 0)
	{
		MessageBoxA(0, "Rzadka kupa a nie connection", "Lel", MB_OK);
		return;
	}
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)recvThread, NULL, NULL, NULL);
	Packet p;
	p.AllocData(5);
	p.AddInt(1);
	p.AddByte(5);
	int r = connection.Send(p);
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
	arrowRight = Loader::LoadTexture(mainPath + "gfx\\arrow.bmp");
	p1 = new Player(0, 1, 0, 0, 48, mainPath + "gfx\\player1.bmp");
	p2 = new Player(1, 3, 12, 0, 48, mainPath + "gfx\\player2.bmp");
	p3 = new Player(2, 1, 0, 12, 48, mainPath + "gfx\\player3.bmp");
	p4 = new Player(3, 3, 12, 12, 48, mainPath + "gfx\\player4.bmp");
	controlledPlayer = p1;

	HANDLE moveThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)moveThread, NULL, NULL, NULL);
	//CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)initConnection, NULL, NULL, NULL);

	SDL_Event e;
	while (!doExit)
	{
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
				doExit = true;
			if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
				HandleMenuKeyboard(e);
		}
		//Render the scene
		SDL_RenderClear(ren);
		SDL_Rect fillRect = {windowH, windowW, 0, 0}; 
		SDL_SetRenderDrawColor(ren, 0, 200, 0, 255); 
		SDL_RenderFillRect(ren, &fillRect);
		RenderMenu();
		/*for (int i = 0; i < mapSize; i++)
		{
			for (int j = 0; j < mapSize; j++)
			{
				if (map[i][j].type != 0)
					Renderer::RenderTexture(texBlock, i*tileSize, j * tileSize, tileSize, tileSize);
			}
		}
		p1->Render(); p2->Render(); p3->Render(); p4->Render();
		drawFPS();
		Renderer::RenderText("X: " + to_string(controlledPlayer->X) + " Y: " + to_string(controlledPlayer->Y), font, white, 10, 70);
		Renderer::RenderText("X: " + to_string(mapX) + " Y: " + to_string(mapY), font, red, 10, 100);*/
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