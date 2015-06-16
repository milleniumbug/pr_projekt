#include "main.h"

#include "EnumTypes.h"
#include <string>
#include <SDL2_rotozoom.h>
#include "TexArray.h"
#include "Loader.h"
#include "Player.h"
#include "Renderer.h"
#include "Connection.h"
#include <ctime>


using namespace std;
string mainPath;

SDL_Renderer* ren = NULL;
TTF_Font* font = NULL;
TTF_Font* bombTimeFont = NULL;
bool doExit = false;
SDL_Color red;
SDL_Color white = {255, 255, 255, 255};
SDL_Color colorKey = {255, 255, 0, 255};
SDL_Color blue = {0, 155, 255, 255};
SDL_Window* win = NULL;
int ticks = 0;
int framesCount = 0;
int tempFramesCount = 0;
int mapSize = 13;
int versionNumber = 1;
int tickrate = 20;
bool fpsLimiter = true;
int fpsLimit = 60;
int fpsLimitMiliseconds = 1000 / fpsLimit;

HANDLE moveThreadHandle = NULL;

void initConnection();

//magia visuala, mam zalinkowan¹ bibliotekê we w³aœciwoœciach projektu, ale ten jej nie widzi ;D
//a ta linijka magicznie dzia³a o.o
#pragma comment(lib, "..\\SDL2_gfx-1.0.1\\lib\\SDL2_gfx.lib")
#pragma comment(lib, "ws2_32.lib")

SDL_Texture* texBlock;
SDL_Texture* texGround;
SDL_Texture* texImmortal;
SDL_Texture* texBomba;
SDL_Texture* texBombaTransparent;
SDL_Texture* arrowRight;
SDL_Texture* texExplosionEffect;
Player* p1 = NULL;
Player* p2 = NULL;
Player* p3 = NULL;
Player* p4 = NULL;
Player* controlledPlayer = NULL;
int playerId = 0;

int tileSize = 64;
int windowW = mapSize * tileSize, windowH = mapSize * tileSize;
int playerSize = 48;

class Block
{
	public:
		int type;
		int size;
		int x;
		int y;
		int hitmarkerExpireTime;

	Block()
	{
		type = 0;
		size = 0;
		x = 0;
		y = 0;
		hitmarkerExpireTime = 0;
	}

	Block(int Type, int Size, int X, int Y)
	{
		type = Type;
		size = Size;
		x = X;
		y = Y;
		hitmarkerExpireTime = 0;
	}
};

Block** map;

void drawFPS()
{
	tempFramesCount++;
	if (GetTickCount() - ticks >= 1000)
	{
		framesCount = tempFramesCount;
		tempFramesCount = 0;
		ticks = GetTickCount();
	}
	
	SDL_SetWindowTitle(win, ("FPS: " + to_string(framesCount)).c_str());
}

int xMoveDir = 0;
int yMoveDir = 0;
double multiplyFactor = 1.0f;

bool leftPressed = false;
bool rightPressed = false;
bool upPressed = false;
bool downPressed = false;
bool spacePressed = false;

bool canBeIP(SDL_Keycode k)
{
	if ((k >= SDLK_a && k <= SDLK_z) || (k >= SDLK_0 && k <= SDLK_9) || (k == SDLK_PERIOD) || (k == SDLK_SEMICOLON) || (k == SDLK_BACKSPACE))
		return true;
	return false;
}

bool menu = true;
bool lobby = false;
//Menu
//IP:PORT textfield
//Info bout server
//Connect
//Get Info
//Exit

Connection connection;

int selectedEntry = 0;
string infoString = "";
string ipPortString = "127.0.0.1:60000";
string ip = "";
int port = -1;

void GetIPPort()
{
	string myIpPortString = ipPortString;
	ip = "";
	port = -1;
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
}

void GetServerInfo()
{
	infoString = "";
	GetIPPort();

	if (port == -1)
		port = 60000;

	connection.SetIP(ip, port);
	Packet p;
	p.AllocData(5);
	p.AddInt(versionNumber);
	p.AddByte(5);

	int result = connection.Send(p);

	p.DeleteData();

	Packet r = connection.Recv();
	if (r.RecvResult > 0 && r.RecvResult != 0xFFFFFFFF)
	{
		int serverVersion = r.ReadInt();
		if (serverVersion != versionNumber)
		{
			infoString = "Wrong version number: " + to_string(serverVersion);
			r.DeleteData();
			return;
		}
		byte packetType = r.ReadByte();
		if (packetType != PacketType::ServerSendServerInfo)
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
	Renderer::RenderTexture(texBomba, windowW / 2 - 150, windowH / 2 - 150, 300, 300);
	Renderer::RenderText("Info: " + infoString, font, white, 100, 80);
	Renderer::RenderText("IP:PORT = " + ipPortString, font, white, 100, 100);
	Renderer::RenderText("Connect", font, white, 100, 120);
	Renderer::RenderText("Get Info", font, white, 100, 140);
	Renderer::RenderText("Exit", font, white, 100, 160);
	Renderer::RenderTexture(arrowRight, 60, 95 + selectedEntry*20, 32, 32);
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
				case 1: lobby = true; CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)initConnection, NULL, NULL, NULL); menu = false; break;
				case 2: CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)GetServerInfo, NULL, NULL, NULL); break;
				case 3: doExit = true; break;
			}
		}
	}
}

bool sendGameStateRequest = false;
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
		if (e.key.keysym.sym == SDLK_SPACE)
			spacePressed = true;
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
		{
			connection.Disconnect();
			doExit = true;
		}
		if (e.key.keysym.sym == SDLK_LSHIFT)
			multiplyFactor = 1.0f;
		if (e.key.keysym.sym == SDLK_SPACE)
			spacePressed = false;
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
		
		int calcSum = 0;
		if (leftPressed)
			calcSum = -4;
		if (rightPressed)
			calcSum = 4;
		if (upPressed)
			calcSum = -1;
		if (downPressed)
			calcSum = 1;
		if (spacePressed)
			calcSum += 16;

		if (calcSum != 0)
		{
			Packet p;
			p.AllocData(6);
			p.AddInt(versionNumber);
			p.AddByte(PacketType::ClientPlayerAction);
			p.AddByte(calcSum);
			connection.Send(p);
			p.DeleteData();
		}

		Sleep(33);
	}
}
int timeLeft = 0;
bool connectionProblem = false;
void recvThread()
{
	while (true)
	{
		Packet recvPacket = connection.Recv();
		if (recvPacket.RecvResult >= 0 && recvPacket.RecvResult != 0xFFFFFFFF)
		{
			connectionProblem = false;
			int version = recvPacket.ReadInt();
			int packetType = recvPacket.ReadByte();
			if (packetType == PacketType::ServerSendGameState)
			{
				timeLeft = recvPacket.ReadInt();
				for (int i = 0; i < mapSize; i++)
				{
					for (int j = 0; j < mapSize; j++)
					{
						Block oldBlock = map[j][i];
						map[j][i] = Block(recvPacket.ReadByte(), tileSize, j, i);
						if (oldBlock.type == BlockType::Bomb && map[j][i].type != BlockType::Bomb)
						{
							map[j][i].hitmarkerExpireTime = time(NULL) + 2500;
						}
					}
				}
				short p1X = recvPacket.ReadShort();
				short p1Y = recvPacket.ReadShort();
				byte p1Progress = recvPacket.ReadByte();
				byte p1Dir = recvPacket.ReadByte();
				recvPacket.ReadByte();
				byte p1Alive = recvPacket.ReadByte();
				short p2X = recvPacket.ReadShort();
				short p2Y = recvPacket.ReadShort();
				byte p2Progress = recvPacket.ReadByte();
				byte p2Dir = recvPacket.ReadByte();
				recvPacket.ReadByte();
				byte p2Alive = recvPacket.ReadByte();
				short p3X = recvPacket.ReadShort();
				short p3Y = recvPacket.ReadShort();
				byte p3Progress = recvPacket.ReadByte();
				byte p3Dir = recvPacket.ReadByte();
				recvPacket.ReadByte();
				byte p3Alive = recvPacket.ReadByte();
				short p4X = recvPacket.ReadShort();
				short p4Y = recvPacket.ReadShort();
				byte p4Progress = recvPacket.ReadByte();
				byte p4Dir = recvPacket.ReadByte();
				recvPacket.ReadByte();
				byte p4Alive = recvPacket.ReadByte();
				p1->X = p1X * 64 + 8;
				p1->Y = p1Y * 64 + 8;
				p1->WalkProgress = p1Progress;
				p1->Direction = p1Dir;
				if (p1->Direction > 10)
					p1->Direction -= 256;
				p1->IsAlive = p1Alive;
				p2->X = p2X * 64 + 8;
				p2->Y = p2Y * 64 + 8;
				p2->WalkProgress = p2Progress;
				p2->Direction = p2Dir;
				if (p2->Direction > 10)
					p2->Direction -= 256;
				p2->IsAlive = p2Alive;
				p3->X = p3X * 64 + 8;
				p3->Y = p3Y * 64 + 8;
				p3->WalkProgress = p3Progress;
				p3->Direction = p3Dir;
				if (p3->Direction > 10)
					p3->Direction -= 256;
				p3->IsAlive = p3Alive;
				p4->X = p4X * 64 + 8;
				p4->Y = p4Y * 64 + 8;
				p4->WalkProgress = p4Progress;
				p4->Direction = p4Dir;
				if (p4->Direction > 10)
					p4->Direction -= 256;
				p4->IsAlive = p4Alive;
			}
			else
			{

			}
		}
		else if (recvPacket.RecvResult == 0xFFFFFFFF)
		{
			connectionProblem = true;
		}
	}
}

void sendThread()
{
	Packet gameStatePacket;
	gameStatePacket.AllocData(5);
	gameStatePacket.AddInt(versionNumber);
	gameStatePacket.AddByte(PacketType::ClientRequestGameState);

	while (true)
	{
		connection.Send(gameStatePacket);
		Sleep(1000 / tickrate);
	}

	gameStatePacket.DeleteData();
}

string lobbyMessage = "";
string lobbyGameState = "";

void RenderLobby()
{
	int textX = 0, textY = 0;
	int textLength = 0;
	int textHeight = 0;
	TTF_SizeText(font, (lobbyMessage + lobbyGameState).c_str(), &textLength, &textHeight);
	textX = (windowW / 2) - (textLength / 2);
	textY = (windowH / 2) - (textHeight / 2);
	Renderer::RenderText(lobbyMessage + lobbyGameState, font, red, textX, textY);
}

void initConnection()
{
	lobbyMessage = "Connecting...";
	lobbyGameState = "";

	GetIPPort();
	int result = connection.SetIP(ip, port);

	Packet p;
	p.AllocData(5);
	p.AddInt(versionNumber);
	p.AddByte(PacketType::ClientJoinGame);
	int r = connection.Send(p);
	p.DeleteData();
	int gameState = GameState::Lobby;
	p = connection.Recv();
	int gv = p.ReadInt();
	byte response = p.ReadByte();
	if (response != PacketType::ServerAccept)
	{
		if (p.RecvResult == 0xFFFFFFFF)
			lobbyMessage = "CLIENT: Connection failed.";
		else if (response == PacketType::ServerGameAlreadyStarted)
			lobbyMessage = "SERVER: Game already started.";
		else if (response == PacketType::ServerIncompatibleVersion)
			lobbyMessage = "SERVER: Incompatible version.";
		else if (response == PacketType::ServerIsFull)
			lobbyMessage = "SERVER: Server is full.";
		else if (response == PacketType::ServerUnknownPacket)
			lobbyMessage = "SERVER: Unknown packet. omG WTF lol";
		else
			lobbyMessage = "Server returned unknown packet, lol";
		Sleep(5000);
		menu = true;
		lobby = false;
		return;
	}
	lobbyMessage = "Waiting for players... ";
	int playersJoined = p.ReadByte();
	int playersNeeded = p.ReadByte();
	gameState = p.ReadByte();
	lobbyGameState = to_string(playersJoined) + "/" + to_string(playersNeeded);
	p.ReadInt();
	playerId = p.ReadByte();
	if (playerId == 1)
		controlledPlayer = p1;
	else if (playerId == 2)
		controlledPlayer = p2;
	else if (playerId == 3)
		controlledPlayer = p3;
	else if (playerId == 4)
		controlledPlayer = p4;
	p.DeleteData();
	Packet sendPacket;
	sendPacket.AllocData(5);
	sendPacket.AddInt(versionNumber);
	sendPacket.AddByte(PacketType::ClientRequestServerInfo);
	while (gameState != GameState::Running)
	{
		connection.Send(sendPacket);
		p = connection.Recv();
		int gv = p.ReadInt();
		byte packetType = p.ReadByte();
		int playersJoined = p.ReadByte();
		int playersNeeded = p.ReadByte();
		gameState = p.ReadByte();
		lobbyGameState = to_string(playersJoined) + "/" + to_string(playersNeeded);
		p.DeleteData();
		Sleep(50);
	}
	sendPacket.DeleteData();
	lobby = false;
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)recvThread, NULL, NULL, NULL);
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)sendThread, NULL, NULL, NULL);
	moveThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)moveThread, NULL, NULL, NULL);
}

int readType(int x, int y)
{
	if (x >= mapSize || x < 0 || y >= mapSize || y < 0)
		return -1;
	else
		return map[y][x].type;
}

void lobbyTimeout()
{
	Sleep(5000);
	lobby = false;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		return 1;

	mainPath = Loader::GetDirectory();
	if (mainPath == "error")
		return 1;
	
	win = SDL_CreateWindow(("Bomberman v" + to_string(versionNumber)).c_str(), 100, 100, windowW, windowH, SDL_WINDOW_SHOWN);
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

	font = TTF_OpenFont((mainPath + "gfx\\DroidSans.ttf").c_str(), 20);
	if (font == NULL)
	{
		SDL_Quit();
		return 1;
	}


	red.a = 255;
	red.r = 255;
	red.b = 0;
	red.g = 0;
	
	map = new Block*[mapSize];
	for (int i = 0; i < mapSize; i++)
		map[i] = new Block[mapSize];

	ticks = GetTickCount();

	texBlock = Loader::LoadTexture(mainPath + "gfx\\block.bmp");
	texImmortal = Loader::LoadTexture(mainPath + "gfx\\immortalblock.bmp");
	texGround = Loader::LoadTexture(mainPath + "gfx\\ground.bmp");
	texBomba = Loader::LoadTexture(mainPath + "gfx\\bomba.bmp");
	texBombaTransparent = Loader::LoadTexture(mainPath + "gfx\\bombx.bmp");
	arrowRight = Loader::LoadTexture(mainPath + "gfx\\arrow.bmp");
	texExplosionEffect = Loader::LoadTexture(mainPath + "gfx\\hitmarker4.bmp");
	p1 = new Player(0, 1, 0, 0, 48, mainPath + "gfx\\player1.bmp");
	p2 = new Player(1, 3, 12, 0, 48, mainPath + "gfx\\player2.bmp");
	p3 = new Player(2, 1, 0, 12, 48, mainPath + "gfx\\player3.bmp");
	p4 = new Player(3, 3, 12, 12, 48, mainPath + "gfx\\player4.bmp");
	controlledPlayer = p1;

	int currentFrameTime;
	SDL_Event e;
	while (!doExit)
	{
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
				doExit = true;
			if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
				if (menu)
					HandleMenuKeyboard(e);
				else
					HandleKeyboard(e);
		}
		currentFrameTime = SDL_GetTicks();
		SDL_SetRenderDrawColor(ren, 0, 200, 0, 255);
		SDL_RenderClear(ren);
		SDL_Rect fillRect = {windowH, windowW, 0, 0}; 
		SDL_RenderFillRect(ren, &fillRect);
		//Renderer::RenderTexture(texExplosionEffect, 200, 0, 64, 64);
		if (menu)
		{
			RenderMenu();
			SDL_RenderPresent(ren);
			continue;
		}
		if (lobby)
		{
			RenderLobby();
			SDL_RenderPresent(ren);
			continue;
		}
		for (int i = 0; i < mapSize; i++)
		{
			for (int j = 0; j < mapSize; j++)
			{
				if (map[i][j].type == 0)
					Renderer::RenderTexture(texGround, i * tileSize, j * tileSize, tileSize, tileSize);
				else if (map[i][j].type == 1)
					Renderer::RenderTexture(texBlock, i * tileSize, j * tileSize, tileSize, tileSize);
				else if (map[i][j].type == 2)
					Renderer::RenderTexture(texImmortal, i * tileSize, j * tileSize, tileSize, tileSize);
				else if (map[i][j].type == 3)
					Renderer::RenderTexture(texBombaTransparent, i * tileSize, j * tileSize, tileSize, tileSize);
			}
		}
		p1->Render(); p2->Render(); p3->Render(); p4->Render();
		for (int i = 0; i < mapSize; i++)
		{
			for (int j = 0; j < mapSize; j++)
			{
				if (map[i][j].hitmarkerExpireTime > time(NULL))
				{
					Renderer::RenderTexture(texExplosionEffect, i * tileSize, j * tileSize, tileSize, tileSize);
					if (readType(j, i - 1) != BlockType::Imdestructible)
						Renderer::RenderTexture(texExplosionEffect, (i - 1) * tileSize, j * tileSize, tileSize, tileSize);
					if (readType(j, i + 1) != BlockType::Imdestructible)
						Renderer::RenderTexture(texExplosionEffect, (i + 1) * tileSize, j * tileSize, tileSize, tileSize);
					if (readType(j - 1, i) != BlockType::Imdestructible)
						Renderer::RenderTexture(texExplosionEffect, i * tileSize, (j - 1) * tileSize, tileSize, tileSize);
					if (readType(j + 1, i) != BlockType::Imdestructible)
						Renderer::RenderTexture(texExplosionEffect, i * tileSize, (j + 1) * tileSize, tileSize, tileSize);
				}
			}
		}
		
		drawFPS();
		if (connectionProblem)
			Renderer::RenderText("Connection problem", font, red, 0, 0);
		Renderer::RenderText("Time left: " + to_string(timeLeft), font, white, 0, 50);
		Renderer::RenderText("Player ID: " + to_string(playerId), font, white, 0, 150);
		if (!controlledPlayer->IsAlive)
		{
			int width = windowW / 3;
			SDL_Rect r = {width, windowH / 2 - 50, width, 100};
			SDL_SetRenderDrawColor(ren, red.r, red.g, red.b, 100);
			SDL_RenderDrawRect(ren, &r);
			r = {width + 1, windowH / 2 - 49, width - 2, 98};
			SDL_SetRenderDrawColor(ren, blue.r, blue.g, blue.b, 100);
			SDL_RenderFillRect(ren, &r);
			Renderer::RenderText("You are dead.", font, red, width + 80, windowH / 2 - 20);
		}
		SDL_RenderPresent(ren);
		int currentSpeed = SDL_GetTicks() - currentFrameTime;
		if (fpsLimitMiliseconds > currentSpeed)
			SDL_Delay(fpsLimitMiliseconds - currentSpeed);
	}

	for (int i = 0; i < mapSize; i++)
		delete[] map[i];
	delete[] map;

	TerminateThread(moveThreadHandle, 0);
	TTF_CloseFont(font);
	SDL_DestroyTexture(texGround);
	SDL_DestroyTexture(texExplosionEffect);
	SDL_DestroyTexture(texBombaTransparent);
	SDL_DestroyTexture(texBomba);
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
