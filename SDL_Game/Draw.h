#pragma once
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "Enums.h"
#include <utility>
#include <vector>
#include <unordered_map> 
#include <tuple>
#include <math.h>       /* sin */

#define PI 3.14159265

#define INIT_WINDOW_WIDTH 1500
#define INIT_WINDOW_HEIGHT 825

template <class T>
T min_constrain(T x, T min) {
	T result = x < min ? min : x;
	return result;
}

template <class T>
T max_constrain(T x, T max) {
	T result = x > max ? max : x;
	return result;
}

template <class T>
T constrain(T x, T min, T max) {
	T result = min_constrain(x, min);
	result = max_constrain(result, max);
	return result;
}

class Draw {
private:
	SDL_Window* pWindow;
	SDL_Renderer* pRenderer;

	SDL_Texture* pTextureGrass;
	SDL_Texture* pTextureStone;

	SDL_Texture* pTextureTanks;
	SDL_Texture* pTextureShot;

	SDL_Texture* pTextureHeart;
	SDL_Texture* pTextureText;

	SDL_Texture* pTextureMainMenu;
	SDL_Texture* pTextureHPkit;
	SDL_Texture* pTextureLandmine;

	UINT cameraX, cameraY;

	std::unordered_map<char, std::pair<int, int>> textLocation;
	std::vector<std::tuple<std::string, int, int, int, bool>> textQueue;

	void loadSprites() {
		SDL_Surface* grass = SDL_LoadBMP("grass.bmp");
		pTextureGrass = SDL_CreateTextureFromSurface(pRenderer, grass);
		SDL_FreeSurface(grass);

		SDL_Surface* stone = SDL_LoadBMP("stones.bmp");
		SDL_SetColorKey(stone, SDL_TRUE, SDL_MapRGB(stone->format, 0, 0, 0));
		pTextureStone = SDL_CreateTextureFromSurface(pRenderer, stone);
		SDL_FreeSurface(stone);

		SDL_Surface* tanks = SDL_LoadBMP("tanks.bmp");
		SDL_SetColorKey(tanks, SDL_TRUE, SDL_MapRGB(tanks->format, 255, 255, 255));
		pTextureTanks = SDL_CreateTextureFromSurface(pRenderer, tanks);
		SDL_FreeSurface(tanks);

		SDL_Surface* shot = SDL_LoadBMP("shot.bmp");
		SDL_SetColorKey(shot, SDL_TRUE, SDL_MapRGB(shot->format, 0, 255, 0));
		pTextureShot = SDL_CreateTextureFromSurface(pRenderer, shot);
		SDL_FreeSurface(shot);

		SDL_Surface* heart = SDL_LoadBMP("heart.bmp");
		SDL_SetColorKey(heart, SDL_TRUE, SDL_MapRGB(heart->format, 0, 255, 0));
		pTextureHeart = SDL_CreateTextureFromSurface(pRenderer, heart);
		SDL_FreeSurface(heart);

		SDL_Surface* text = SDL_LoadBMP("text.bmp");
		SDL_SetColorKey(text, SDL_TRUE, SDL_MapRGB(text->format, 0, 255, 0));
		pTextureText = SDL_CreateTextureFromSurface(pRenderer, text);
		SDL_FreeSurface(text);

		SDL_Surface* mm = SDL_LoadBMP("mainmenu.bmp");
		pTextureMainMenu = SDL_CreateTextureFromSurface(pRenderer, mm);
		SDL_FreeSurface(mm);

		SDL_Surface* hpkit = SDL_LoadBMP("healthkit.bmp");
		SDL_SetColorKey(hpkit, SDL_TRUE, SDL_MapRGB(hpkit->format, 0, 0, 0));
		pTextureHPkit = SDL_CreateTextureFromSurface(pRenderer, hpkit);
		SDL_FreeSurface(hpkit);

		SDL_Surface* lm = SDL_LoadBMP("landmine.bmp");
		SDL_SetColorKey(lm, SDL_TRUE, SDL_MapRGB(lm->format, 0, 0, 0));
		pTextureLandmine = SDL_CreateTextureFromSurface(pRenderer, lm);
		SDL_FreeSurface(lm);
	}

	void wrapBG(std::vector<std::pair<int, int>> stoneLoc, std::vector<std::pair<int, int>> stoneSize, Color myTank, std::vector<std::tuple<Color, int, float, float, float, float>> tankInfo, int mapW, int mapH) {
		UINT myX = 0, myY = 0;
		UINT avgX = 0, avgY = 0;
		UINT numLiveTank = 0;

		for (std::tuple<Color, int, int, int, float, float> tank : tankInfo) {
			if (std::get<1>(tank) != 0) {
				UINT x = (UINT)std::get<2>(tank);
				UINT y = (UINT)std::get<3>(tank);
				if (std::get<0>(tank) == myTank) {
					myX = x;
					myY = y;
				}
				avgX += x;
				avgY += y;
				numLiveTank++;
			}
			else {
				myX = mapW / 2;
				myY = mapH / 2;
			}
		}
		
		if (numLiveTank < 1)
			numLiveTank = 1;

		avgX = (float)avgX / numLiveTank;
		avgY = (float)avgY / numLiveTank;

		avgX = constrain((int)myX - (int)avgX, -250, 250) + 710;
		avgY = constrain((int)myY - (int)avgY, -150, 150) + 425;

		cameraX = cameraX - constrain((int)(cameraX - (myX - avgX)), -10, 10);
		cameraY = cameraY - constrain((int)(cameraY - (myY - avgY)), -10, 10);

		UINT x = 0, y = 0, dx = 250, dy = 250;
		for (; x < mapW; x += dx) {
			for (y = 0; y < mapH; y += dy) {
				SDL_Rect srcRect = { 0, 0, 250, 250 };
				SDL_Rect destRect = { x - cameraX, y - cameraY, 250, 250 };
				SDL_RenderCopy(pRenderer, pTextureGrass, &srcRect, &destRect);
			}
		}

		for (UINT i = 0; i < stoneLoc.size(); i++) {
			SDL_Rect srcRect = { 256 * (i % 4), 0, 256, 256 }; //5 Textures to choose from
			SDL_Rect destRect = { stoneLoc[i].first - cameraX, stoneLoc[i].second - cameraY, stoneSize[i].first, stoneSize[i].second };
			SDL_RenderCopy(pRenderer, pTextureStone, &srcRect, &destRect);
		}
	}

	//Tank ID, hp , x, y, tank yaw, turret yaw
	void drawTanks(std::vector<std::tuple<Color, int, float, float, float, float>> tankInfo, int maxHP) {
		for (std::tuple < Color, int, float, float, float, float > tank : tankInfo) {
			int dmg = 0;
			if (std::get<1>(tank) < maxHP / 2)
				dmg = 1;
			if (std::get<1>(tank) == 0)
				dmg = 2;

			//tank
			float bodyAngle = fmod(std::get<4>(tank) + 90, 360);
			SDL_Rect srcRectBody = { 44 + dmg * 84, std::get<0>(tank) * 104, 80, 100 }; //4 pixel gap
			SDL_Rect destRectBody = { (UINT)std::get<2>(tank) - cameraX, (UINT)std::get<3>(tank) - cameraY, 80, 100 };
			SDL_Point bodyCenter = { 40, 50 };

			SDL_RenderCopyEx(pRenderer, pTextureTanks, &srcRectBody, &destRectBody, bodyAngle, &bodyCenter, SDL_FLIP_NONE);

			if (dmg != 2) {
				//turret
				float turretAngle = fmod(std::get<5>(tank) + 90, 360);
				SDL_Rect srcRectTurret = { 0, std::get<0>(tank) * 104, 40, 100 };
				SDL_Rect destRectTurret = { 20 + (UINT)std::get<2>(tank) - cameraX, (UINT)std::get<3>(tank) - cameraY, 40, 100 };
				SDL_Point turretCenter = { 20, 50 };

				SDL_RenderCopyEx(pRenderer, pTextureTanks, &srcRectTurret, &destRectTurret, turretAngle, &turretCenter, SDL_FLIP_NONE);
			}
		}
	}

	//x, y, animation #(0-4), angle (not used here)
	void drawShots(std::vector<std::tuple<float, float, int, float>> shots) {
		for (std::tuple<int, int, int, float> shot : shots) {
			SDL_Rect srcRect = { std::get<2>(shot) * 8, 0, 8, 8 };
			SDL_Rect destRect = { std::get<0>(shot) - cameraX, std::get<1>(shot) - cameraY, 8, 8 };
			SDL_RenderCopy(pRenderer, pTextureShot, &srcRect, &destRect);
		}
	}

	void drawHearts(std::vector<std::tuple<Color, int, float, float, float, float>> tankInfo, Color myTank) {
		SDL_Rect srcRect = { 0, 0, 800, 800 };
		int hp = 0;
		for (auto tank : tankInfo) {
			if (std::get<0>(tank) == myTank) {
				hp = std::get<1>(tank);
			}
		}

		for (int i = 0; i < hp; i++) {
			SDL_Rect destRect = { 20 + i * 30, 0, 70, 70 };
			SDL_RenderCopy(pRenderer, pTextureHeart, &srcRect, &destRect);
		}
	}

	void drawFieldItems(std::vector<std::pair<int, int>> hpkits, std::vector<std::pair<int, int>> landmines) {
		for (auto hp : hpkits) {
			SDL_Rect srcRect = { 0, 0, 128, 118 };
			SDL_Rect destRect = { hp.first - 32 - cameraX, hp.second - 32 - cameraY, 64, 64 };
			SDL_RenderCopy(pRenderer, pTextureHPkit, &srcRect, &destRect);
		}

		for (auto lm : landmines) {
			SDL_Rect srcRect = { 0, 0, 210, 190 };
			SDL_Rect destRect = { lm.first - 32 - cameraX, lm.second - 32 - cameraY, 64, 64 };
			SDL_RenderCopy(pRenderer, pTextureLandmine, &srcRect, &destRect);
		}
	}

	//default size of letter is 20x20, so adjust int size to scale
	void drawText() {
		for (std::tuple<std::string, int, int, int, bool> job : textQueue) {
			int currentX = std::get<1>(job);

			for (char const& c : std::get<0>(job)) {
				char upc = toupper(c);
		
				SDL_Rect srcRect = { 20 * textLocation[upc].first, 20 * textLocation[upc].second, 20, 20 };
				int size = std::get<3>(job);
				SDL_Rect destRect;
				if (std::get<4>(job)) {
					destRect = { currentX - (int)cameraX, std::get<2>(job) - (int)cameraY, size, size };
				}
				else {
					destRect = { currentX, std::get<2>(job), size, size };
				}
				currentX += size;

				SDL_RenderCopy(pRenderer, pTextureText, &srcRect, &destRect);
			}
		}
		textQueue.clear();
	}
public:
	Draw() {
		textLocation['A'] = std::make_pair(3, 2);
		textLocation['B'] = std::make_pair(4, 2);
		textLocation['C'] = std::make_pair(5, 2);
		textLocation['D'] = std::make_pair(6, 2);
		textLocation['E'] = std::make_pair(7, 2);
		textLocation['F'] = std::make_pair(8, 2);
		textLocation['G'] = std::make_pair(9, 2);
		textLocation['H'] = std::make_pair(10, 2);
		textLocation['I'] = std::make_pair(11, 2);
		textLocation['J'] = std::make_pair(12, 2);
		textLocation['K'] = std::make_pair(13, 2);
		textLocation['L'] = std::make_pair(14, 2);
		textLocation['M'] = std::make_pair(0, 3);
		textLocation['N'] = std::make_pair(1, 3);
		textLocation['O'] = std::make_pair(2, 3);
		textLocation['P'] = std::make_pair(3, 3);
		textLocation['Q'] = std::make_pair(4, 3);
		textLocation['R'] = std::make_pair(5, 3);
		textLocation['S'] = std::make_pair(6, 3);
		textLocation['T'] = std::make_pair(7, 3);
		textLocation['U'] = std::make_pair(8, 3);
		textLocation['V'] = std::make_pair(9, 3);
		textLocation['W'] = std::make_pair(10, 3);
		textLocation['X'] = std::make_pair(11, 3);
		textLocation['Y'] = std::make_pair(12, 3);
		textLocation['Z'] = std::make_pair(13, 3);


		textLocation['0'] = std::make_pair(1, 1);
		textLocation['1'] = std::make_pair(2, 1);
		textLocation['2'] = std::make_pair(3, 1);
		textLocation['3'] = std::make_pair(4, 1);
		textLocation['4'] = std::make_pair(5, 1);
		textLocation['5'] = std::make_pair(6, 1);
		textLocation['6'] = std::make_pair(7, 1);
		textLocation['7'] = std::make_pair(8, 1);
		textLocation['8'] = std::make_pair(9, 1);
		textLocation['9'] = std::make_pair(10, 1);

		textLocation[' '] = std::make_pair(0, 0);
		textLocation['!'] = std::make_pair(1, 0);
		textLocation['"'] = std::make_pair(2, 0);
		textLocation['#'] = std::make_pair(3, 0);
		textLocation['$'] = std::make_pair(4, 0);
		textLocation['%'] = std::make_pair(5, 0);
		textLocation['&'] = std::make_pair(6, 0);
		textLocation['\''] = std::make_pair(7, 0);
		textLocation['('] = std::make_pair(8, 0);
		textLocation[')'] = std::make_pair(9, 0);
		textLocation['*'] = std::make_pair(10, 0);
		textLocation['+'] = std::make_pair(11, 0);
		textLocation[','] = std::make_pair(12, 0);
		textLocation['-'] = std::make_pair(13, 0);
		textLocation['.'] = std::make_pair(14, 0);
		textLocation['/'] = std::make_pair(0, 0);

	}

	void setup() {
		SDL_Init(SDL_INIT_VIDEO);

		pWindow = SDL_CreateWindow("SDL Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
		pRenderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

		cameraX = 710;
		cameraY = 425;

		loadSprites();
	}

	void queueText(std::string text, int x, int y, int size) {
		textQueue.push_back(std::make_tuple(text, x, y, size, false));
	}

	void queueText(std::string text, int x, int y, int size, bool followCam) {
		textQueue.push_back(std::make_tuple(text, x, y, size, followCam));
	}

	void updateMM(bool isServer) {
		SDL_RenderClear(pRenderer);
		SDL_Rect srcRect = { 0, 0, 1920, 1080 };
		SDL_Rect destRect = { 0, 0, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT };
		SDL_RenderCopy(pRenderer, pTextureMainMenu, &srcRect, &destRect);

		queueText("Just Tanks", 50, 300, 100);
		queueText("Daniel Pham", 80, 400, 50);
		queueText("Press any button to start!", 500, 700, 20);
		if (isServer) {
			queueText("Serving game", 600, 600, 30);
		}

		drawText();
		SDL_RenderPresent(pRenderer);
	}

	void updateGS(std::string name, std::string IP) {
		SDL_RenderClear(pRenderer);
		SDL_Rect srcRect = { 0, 0, 1920, 1080 };
		SDL_Rect destRect = { 0, 0, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT };
		SDL_RenderCopy(pRenderer, pTextureMainMenu, &srcRect, &destRect);

		queueText("What is your Name?", 50, 50, 70);
		queueText(name, 50, 150, 50);

		

		drawText();
		SDL_RenderPresent(pRenderer);
	}

	void winScreen() {
		SDL_RenderClear(pRenderer);
		SDL_Rect srcRect = { 0, 0, 1920, 1080 };
		SDL_Rect destRect = { 0, 0, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT };
		SDL_RenderCopy(pRenderer, pTextureMainMenu, &srcRect, &destRect);

		queueText("You Win!", 100, 100, 100);

		drawText();
		SDL_RenderPresent(pRenderer);
	}
	
	void loseScreen() {
		SDL_RenderClear(pRenderer);
		SDL_Rect srcRect = { 0, 0, 1920, 1080 };
		SDL_Rect destRect = { 0, 0, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT };
		SDL_RenderCopy(pRenderer, pTextureMainMenu, &srcRect, &destRect);

		queueText("You Lose!", 100, 100, 100);

		drawText();
		SDL_RenderPresent(pRenderer);
	}

	void update(std::vector<std::pair<int, int>> stoneLoc, std::vector<std::pair<int, int>> stoneSize, 
		int mapW, int mapH, 
		Color myTank, std::vector<std::tuple<Color, int, float, float, float, float>> tankInfo, std::vector<std::tuple<float, float, int, float>> shots, 
		int maxHP, std::string fps, 
		std::vector<std::pair<int, int>> hpkits, std::vector<std::pair<int, int>> landmines) {

		SDL_RenderClear(pRenderer);

		wrapBG(stoneLoc, stoneSize, myTank, tankInfo, mapW, mapH);
		drawFieldItems(hpkits, landmines);
		drawTanks(tankInfo, maxHP);
		drawShots(shots);
		drawHearts(tankInfo, myTank);
		
		queueText("FPS: " + fps, 20, INIT_WINDOW_HEIGHT - 20, 20);

		drawText();
		SDL_RenderPresent(pRenderer);
	}

	void cleanup() {
		SDL_DestroyTexture(pTextureGrass);
		SDL_DestroyTexture(pTextureStone);
		SDL_DestroyTexture(pTextureTanks);
		SDL_DestroyTexture(pTextureShot);
		SDL_DestroyTexture(pTextureHeart);
		SDL_DestroyTexture(pTextureText);

		SDL_DestroyTexture(pTextureMainMenu);
		SDL_DestroyTexture(pTextureHPkit);
		SDL_DestroyTexture(pTextureLandmine);

		SDL_DestroyRenderer(pRenderer);
		SDL_DestroyWindow(pWindow);
		SDL_Quit();
	}
};