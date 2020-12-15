#include "Server.h"
#include "Client.h"
#include "Draw.h"
#include "Enums.h"
#include "Map_1.h"

#include <stdio.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <stdlib.h>     /* srand, rand */
#include <time.h> 

int main(int argc, char** argv) {
	srand(time(NULL));
	
	printf("Client or Server?: ");
	std::string choice;
	std::cin >> choice;

	SOCKET ConnectSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	bool isServer = false;

	std::string address = "localhost";
	switch (choice[0]) {
	case 'c':
	case 'C':
		printf("Please enter server address\n");
		printf("For example > fruit.qc.to\n");
		printf(">> ");
		std::cin >> address;
		ConnectSocket = setupClient(address);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("Error connecting to host!\n");
			exit(0);
			break;
		}
		printf("Connected to game server!\n");
		break;
	case 's':
	case 'S':
		isServer = true;
		ClientSocket = setupServer("0.0.0.0", 25566);
		printf("Opponent connected!\n");
		break;
	default:
		printf("c or s please");
		exit(0);
		break;
	}
	
	//SDL_SetMainReady();
	Draw GameWindow;
	GameWindow.setup();

	//main menu
	bool finished = false;

main_menu_label:
	bool inMM = true;
	while (inMM && !finished) {
		GameWindow.updateMM(isServer);
		SDL_Event e = { };
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				finished = true;
			}
			else if (e.type == SDL_KEYDOWN) {
				auto keyCode = e.key.keysym.sym;
				switch (keyCode) {
				default:
					inMM = false;
					break;
				}
			}
		}
	}
	//main game loop

	Map_1 Map;

	float x = 300, y = 300;
	float b_angle = 0.0, t_angle = 0.0; //tank sprite points north at 90, but 0 is right
	const float maxSpeed = 5;
	const float acceleration = 0.05;
	float currentSpeed = 0;
	const int maxHP = 5;
	int myHP = 5;
	Color myTank;
	std::vector<std::tuple<Color, int, float, float, float, float>> tankInfo;

	if (isServer) {
		myTank = Color::YELLOW;
	}
	else {
		myTank = Color::GREEN;
		x = 300;
		y = 600;
	}
	tankInfo.push_back(std::make_tuple(myTank, myHP, x, y, b_angle, t_angle));

	int tankHitboxRadius = 35;
	char shot = 0;

	const float b_rotationSpeed = 2;
	const float t_rotationSpeed = 2; //degrees per frame

	std::chrono::steady_clock::time_point lastShotTime = std::chrono::steady_clock::now();
	const long shootCooldown = 1500; //milliseconds
	const int shotSpeed = 10;

	long diff = 0; //To count fps

	//commands
	int dir[7] = { 0, 0, 0, 0, 0, 0, 0};

	//x, y, animation #(0-4), angle (not used here)
	std::vector<std::tuple<float, float, int, float>> shots;

	int buffer = 3; //network buffer

	//lets run at 60 fps
	while (!finished) {
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		SDL_Event e = { };
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				finished = true;
			}
			else if (e.type == SDL_KEYDOWN) {
				auto keyCode = e.key.keysym.sym;
				switch (keyCode) {
				case SDLK_ESCAPE:
					finished = true;
					break;
				case SDLK_w:
					dir[Command::FORWARD] = 1;
					break;
				case SDLK_s:
					dir[Command::BACK] = 1;
					break;
				case SDLK_a:
					dir[Command::B_LEFT] = 1;
					break;
				case SDLK_d:
					dir[Command::B_RIGHT] = 1;
					break;
				case SDLK_j:
					dir[Command::T_LEFT] = 1;
					break;
				case SDLK_k:
					dir[Command::T_RIGHT] = 1;
					break;
				case SDLK_SPACE:
					dir[Command::SHOOT] = 1;
					break;
				default:
					break;
				}
			}
			else if (e.type == SDL_KEYUP) {
				auto keyCode = e.key.keysym.sym;
				switch (keyCode) {
				case SDLK_w:
					dir[Command::FORWARD] = 0;
					break;
				case SDLK_s:
					dir[Command::BACK] = 0;
					break;
				case SDLK_a:
					dir[Command::B_LEFT] = 0;
					break;
				case SDLK_d:
					dir[Command::B_RIGHT] = 0;
					break;
				case SDLK_j:
					dir[Command::T_LEFT] = 0;
					break;
				case SDLK_k:
					dir[Command::T_RIGHT] = 0;
					break;
				case SDLK_SPACE:
					dir[Command::SHOOT] = 0;
					break;
				default:
					break;
				}
			}
		}

		shot = 0;

		if (myHP > 0) {
			if (dir[Command::FORWARD]) {
				if (currentSpeed < 0) {
					currentSpeed += 2.5 * acceleration;
				}
				else {
					currentSpeed += acceleration;
				}
			}
			else if (dir[Command::BACK]) {
				if (currentSpeed > 0) {
					currentSpeed -= 2.5 * acceleration;
				}
				else {
					currentSpeed -= acceleration;
				}
			}
			else {
				currentSpeed -= currentSpeed * 0.03;
			}


			if (dir[Command::B_LEFT]) { b_angle -= b_rotationSpeed; t_angle -= b_rotationSpeed; }
			if (dir[Command::B_RIGHT]) { b_angle += b_rotationSpeed; t_angle += b_rotationSpeed; }
			if (dir[Command::T_LEFT]) { t_angle -= t_rotationSpeed; }
			if (dir[Command::T_RIGHT]) { t_angle += t_rotationSpeed; }

			b_angle = fmod(b_angle, 360);
			t_angle = fmod(t_angle, 360);

			currentSpeed = constrain(currentSpeed, -1 * maxSpeed, maxSpeed);

			float x_factor = cos(b_angle * PI / 180.0);
			float y_factor = sin(b_angle * PI / 180.0);

			/*
			std::vector<std::pair<int, int>> tankHitbox;

			//assuming tank is pointing to the right
			tankHitbox.push_back(std::make_pair(x + 30 + 40 * x_factor, y + 45 + 35 * y_factor)); //front
			tankHitbox.push_back(std::make_pair(x + 30 - 40 * x_factor, y + 45 - 35 * y_factor)); //back
			tankHitbox.push_back(std::make_pair(x + 30 + 40 * y_factor, y + 45 - 35 * x_factor)); //L
			tankHitbox.push_back(std::make_pair(x + 30 - 40 * y_factor, y + 45 + 35 * x_factor)); //R

			tankHitbox.push_back(std::make_pair(x + 30 + 40 * x_factor + 40 * y_factor, y + 45 + 35 * y_factor - 35 * x_factor)); //front left
			tankHitbox.push_back(std::make_pair(x + 30 - 40 * x_factor + 40 * y_factor, y + 45 - 35 * y_factor - 35 * x_factor)); //back left
			tankHitbox.push_back(std::make_pair(x + 30 + 40 * x_factor - 40 * y_factor, y + 45 + 35 * y_factor + 35 * x_factor)); //front right
			tankHitbox.push_back(std::make_pair(x + 30 - 40 * x_factor - 40 * y_factor, y + 45 - 35 * y_factor + 35 * x_factor)); //back right
			*/

			/*for (auto p : tankHitbox) {
				GameWindow.queueText("o", (int)p.first, (int)p.second, 30, true);
			}*/

			int tankCenterX = x + 40 - 10 * x_factor;
			int tankCenterY = y + 50 - 10 * y_factor;

			/*for (int i = tankCenterX - tankHitboxRadius; i < tankCenterX + tankHitboxRadius ; i++) {
				for (int j = tankCenterY - tankHitboxRadius; j < tankCenterY + tankHitboxRadius ; j++) {
					if (pow(i - tankCenterX, 2) + pow(j - tankCenterY, 2) <= pow(tankHitboxRadius, 2)) {
						GameWindow.queueText("o", i, j, 1, true);
					}
				}
			}*/



			int cleanx = 0;
			int cleany = 0;
			int totalxy = 0;
			int numHits = 0;

			std::vector<std::pair<int, int>> otherTankCenters;
			for (auto tank : tankInfo) {
				//printf("%i %i %f %f %f %f\n", std::get<0>(tank), std::get<1>(tank), std::get<2>(tank), std::get<3>(tank), std::get<4>(tank), std::get<5>(tank));
				if (std::get<0>(tank) != myTank) {
					float ob_angle = std::get<4>(tank);
					float ox_factor = cos(ob_angle * PI / 180.0);
					float oy_factor = sin(ob_angle * PI / 180.0);
					int otankCenterX = std::get<2>(tank) + 40 - 10 * ox_factor;
					int otankCenterY = std::get<3>(tank) + 50 - 10 * oy_factor;
					otherTankCenters.push_back(std::make_pair(otankCenterX, otankCenterY));
				}
			}

			for (int i = tankCenterX - tankHitboxRadius; i < tankCenterX + tankHitboxRadius; i++) {
				for (int j = tankCenterY - tankHitboxRadius; j < tankCenterY + tankHitboxRadius; j++) {
					if (pow(i - tankCenterX, 2) + pow(j - tankCenterY, 2) <= pow(tankHitboxRadius, 2)) {
						if (Map.collide(i, j)) {
							cleanx += i;
							cleany += j;
							totalxy++;
							numHits++;
							//printf("collide %i, %i\n", i, j);
						}
						else {
							for (std::pair<int, int> otank : otherTankCenters) {
								if (pow(otank.first - i, 2) + pow(otank.second - j, 2) <= pow(tankHitboxRadius, 2)) {
									cleanx += i;
									cleany += j;
									totalxy++;
									numHits++;
								}
							}
						}
					}
				}
			}

			int cavgX = 0;
			int cavgY = 0;
			if (totalxy > 0) {
				cavgX = cleanx / totalxy;
				cavgY = cleany / totalxy;
			}

			float nextMoveX = currentSpeed * x_factor;
			float nextMoveY = currentSpeed * y_factor;
			if (numHits > 0) {
				//printf("%i, %i, %i, %i\n", cavgX, cavgY, tankCenterX, tankCenterY);
				if (cavgX <= tankCenterX) {
					if (nextMoveX >= 0) {
						x += nextMoveX;
					}
				}

				if (cavgX >= tankCenterX) {
					if (nextMoveX <= 0) {
						x += nextMoveX;
					}
				}

				if (cavgY <= tankCenterY) {
					if (nextMoveY >= 0) {
						y += nextMoveY;
					}
				}

				if (cavgY >= tankCenterY) {
					if (nextMoveY <= 0) {
						y += nextMoveY;
					}
				}
			}
			else {
				x += nextMoveX;
				y += nextMoveY;
			}


			x = constrain(x, 0.0f, (float)Map.getW());
			y = constrain(y, 0.0f, (float)Map.getH());

			//shoot
			if (dir[Command::SHOOT]) {
				std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
				if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastShotTime).count() > shootCooldown) {
					shot = 1;
					lastShotTime = std::chrono::steady_clock::now();
				}
			}
		}

		//tankInfo.clear();

		if (!isServer) {
			// color int
			// hp int
			// x float
			// y float
			// body rotation float
			// turret rotation float
			// shot? char
			// 4 + 4 + 4 + 4 + 4 + 4 + 1 = 25

			char sendbuf[25];
			memcpy_s(&sendbuf[0], 4, (char*)&myTank, 4);
			memcpy_s(&sendbuf[4], 4, (char*)&myHP, 4);
			memcpy_s(&sendbuf[8], 4, (char*)&x, 4);
			memcpy_s(&sendbuf[12], 4, (char*)&y, 4);
			memcpy_s(&sendbuf[16], 4, (char*)&b_angle, 4);
			memcpy_s(&sendbuf[20], 4, (char*)&t_angle, 4);
			memcpy_s(&sendbuf[24], 1, (char*)&shot, 1);

			int sumSent = 0;
			while (sumSent < 25) {
				int iResult = send(ConnectSocket, &sendbuf[sumSent], 25 - sumSent, 0);
				if (iResult == SOCKET_ERROR) {
					printf("client tank send failed with error: %d\n", WSAGetLastError());
					closesocket(ConnectSocket);
					WSACleanup();
					finished = true;
				}
				sumSent += iResult;
			}

			//printf("tank sent\n");
		}

		if (isServer && buffer == 0) {
			Color r_Color;
			int r_HP;
			float r_X;
			float r_Y;
			float r_B_angle;
			float r_T_angle;
			char r_shot;

			int sumRecv = 0;
			char recvbuf[25];
			while (sumRecv < 25) {
				int iResult = recv(ClientSocket, &recvbuf[sumRecv], 25 - sumRecv, 0);

				if (iResult > 0) {
					sumRecv += iResult;
				}
				else if (iResult == 0)
				{
					printf("Connection closing...\n");
					finished = true;
					break;
				} else {
					printf("recv tank failed with error: %d\n", WSAGetLastError());
					closesocket(ClientSocket);
					WSACleanup();
					finished = true;
					break;
				}
			}

			memcpy_s((char*)&r_Color, sizeof(Color), &recvbuf[0], 4);
			memcpy_s((char*)&r_HP, sizeof(int), &recvbuf[4], 4);
			memcpy_s((char*)&r_X, sizeof(float), &recvbuf[8], 4);
			memcpy_s((char*)&r_Y, sizeof(float), &recvbuf[12], 4);
			memcpy_s((char*)&r_B_angle, sizeof(float), &recvbuf[16], 4);
			memcpy_s((char*)&r_T_angle, sizeof(float), &recvbuf[20], 4);
			memcpy_s((char*)&r_shot, sizeof(char), &recvbuf[24], 1);

			float r_sx_factor = cos(r_T_angle * PI / 180.0);
			float r_sy_factor = sin(r_T_angle * PI / 180.0);

			int r_turretCenterX = r_X + 35;
			int r_turretCenterY = r_Y + 45;

			if (r_shot) {
				shots.push_back(std::make_tuple(r_turretCenterX + 60 * r_sx_factor, r_turretCenterY + 60 * r_sy_factor, 0, r_T_angle));
			}
			//std::tuple<Color, int, float, float, float, float> r_tank = std::make_tuple(r_Color, r_HP, r_X, r_Y, r_B_angle, r_T_angle);

			std::vector<std::tuple<Color, int, float, float, float, float>> updatedTanks;

			bool inTankInfo = false;
			for (auto tank : tankInfo) {
				if (std::get<0>(tank) == r_Color) {
					inTankInfo = true;
					std::tuple<Color, int, float, float, float, float> temp = std::make_tuple(r_Color, std::get<1>(tank), r_X, r_Y, r_B_angle, r_T_angle);
					updatedTanks.push_back(temp);
				}
				else {
					updatedTanks.push_back(tank);
				}
			}
			if (!inTankInfo)
				updatedTanks.push_back(std::make_tuple(r_Color, r_HP, r_X, r_Y, r_B_angle, r_T_angle));
			tankInfo = updatedTanks;
			//tankInfo.push_back(r_tank);

			//printf("tank received\n");
		}

		if (isServer) {
			//tankInfo.push_back(std::make_tuple(myTank, myHP, x, y, b_angle, t_angle));

			if (shot) {
				//x, y, animation #(0-4), angle
				float sx_factor = cos(t_angle * PI / 180.0);
				float sy_factor = sin(t_angle * PI / 180.0);

				int turretCenterX = x + 35;
				int turretCenterY = y + 45;
				shots.push_back(std::make_tuple(turretCenterX + 60 * sx_factor, turretCenterY + 60 * sy_factor, 0, t_angle));
			}

			//update each shot
			std::vector<std::tuple<float, float, int, float>> updated_shots;
			for (auto shot : shots) {
				float s_angle = std::get<3>(shot);
				float sx_factor = cos(s_angle * PI / 180.0);
				float sy_factor = sin(s_angle * PI / 180.0);

				float newX = std::get<0>(shot) + shotSpeed * sx_factor;
				float newY = std::get<1>(shot) + shotSpeed * sy_factor;
				int newAnim = (std::get<2>(shot) + 1) % 5;

				if (!Map.collide(newX, newY)) {
					updated_shots.push_back(std::make_tuple(newX, newY, newAnim, s_angle));
				}
			}

			shots = updated_shots;

			//calculate hits
			updated_shots.clear();

			std::vector<std::tuple<Color, int, float, float, float, float>> updatedtankInfo;
			std::vector<std::pair<int, int>> hitLMs;
			std::vector<std::pair<int, int>> hitHPs;
			for (auto tank : tankInfo) {
				float ob_angle = std::get<4>(tank);
				float ox_factor = cos(ob_angle * PI / 180.0);
				float oy_factor = sin(ob_angle * PI / 180.0);
				float otankCenterX = std::get<2>(tank) + 40 - 10 * ox_factor;
				float otankCenterY = std::get<3>(tank) + 50 - 10 * oy_factor;
				int hp = std::get<1>(tank);
				for (auto shot : shots) {
					if (pow(otankCenterX - std::get<0>(shot), 2) + pow(otankCenterY - std::get<1>(shot), 2) <= pow(tankHitboxRadius, 2)) {
						hp = min_constrain(hp - 1, 0);
					}
					else {
						updated_shots.push_back(shot);
					}
				}

				for (auto landmine : Map.getLandmines()) {
					if (pow(otankCenterX - landmine.first, 2) + pow(otankCenterY - landmine.second, 2) <= pow(tankHitboxRadius, 2)) {
						hp = min_constrain(hp - 1, 0);
						hitLMs.push_back(std::make_pair(landmine.first, landmine.second));
					}
				}

				for (auto health : Map.getHealthKits()) {
					if (pow(otankCenterX - health.first, 2) + pow(otankCenterY - health.second, 2) <= pow(tankHitboxRadius, 2)) {
						hp = max_constrain(hp + 1, maxHP);
						hitHPs.push_back(std::make_pair(health.first, health.second));
					}
				}

				if (std::get<0>(tank) == myTank) {
					myHP = hp;
					updatedtankInfo.push_back(std::make_tuple(std::get<0>(tank), hp, x, y, b_angle, t_angle));
				}
				else {
					updatedtankInfo.push_back(std::make_tuple(std::get<0>(tank), hp, std::get<2>(tank), std::get<3>(tank), std::get<4>(tank), std::get<5>(tank)));
				}
				shots = updated_shots;
				updated_shots.clear();
			}

			tankInfo = updatedtankInfo;
			//printf("server processing done\n");

			//send updated shots to client
			//shot x float 4
			//shot y float 4
			//8 bytes per shot
			{
				int bytestosend = 2 * sizeof(float) * shots.size();

				char sendbuf[sizeof(int)];
				memcpy_s(&sendbuf[0], sizeof(int), (char*)&bytestosend, sizeof(int));

				int sumSent = 0;
				while (sumSent < sizeof(int)) {
					int iResult = send(ClientSocket, &sendbuf[sumSent], sizeof(int) - sumSent, 0);
					if (iResult == SOCKET_ERROR) {
						printf("send shots info failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					sumSent += iResult;
				}

				//printf("sent shot info %i\n", bytestosend);

				if (bytestosend != 0) {
					char* sendbufShots = new char[bytestosend]();

					int i = 0;
					for (auto shot : shots) {
						memcpy_s(&sendbufShots[i], sizeof(float), (char*)&std::get<0>(shot), sizeof(float));
						i += sizeof(float);
						memcpy_s(&sendbufShots[i], sizeof(float), (char*)&std::get<1>(shot), sizeof(float));
						i += sizeof(float);
					}

					sumSent = 0;
					while (sumSent < bytestosend) {
						int iResult = send(ClientSocket, &sendbufShots[sumSent], bytestosend - sumSent, 0);
						if (iResult == SOCKET_ERROR) {
							printf("send shots failed with error: %d\n", WSAGetLastError());
							closesocket(ClientSocket);
							WSACleanup();
							return 1;
						}
						sumSent += iResult;
					}

					delete[] sendbufShots;

					//printf("sent shots\n");
				}
			}

			//send tank info
			//was 25 earlier, -1 shot
			{
				int bytestosend = 24 * tankInfo.size();
				char sendbuf[sizeof(int)];
				memcpy_s(&sendbuf[0], sizeof(int), (char*)&bytestosend, sizeof(int));

				int sumSent = 0;
				while (sumSent < sizeof(int)) {
					int iResult = send(ClientSocket, &sendbuf[sumSent], sizeof(int) - sumSent, 0);
					if (iResult == SOCKET_ERROR) {
						printf("send tank info failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					sumSent += iResult;
				}

				//printf("sent tanks info %i\n", bytestosend);

				if (bytestosend != 0) {
					char* sendbufTanks = new char[bytestosend]();

					int i = 0;
					for (auto tanks : tankInfo) {
						memcpy_s(&sendbufTanks[i], sizeof(Color), (char*)&std::get<0>(tanks), sizeof(Color));
						i += sizeof(Color);
						memcpy_s(&sendbufTanks[i], sizeof(int), (char*)&std::get<1>(tanks), sizeof(int));
						i += sizeof(int);
						memcpy_s(&sendbufTanks[i], sizeof(float), (char*)&std::get<2>(tanks), sizeof(float));
						i += sizeof(float);
						memcpy_s(&sendbufTanks[i], sizeof(float), (char*)&std::get<3>(tanks), sizeof(float));
						i += sizeof(float);
						memcpy_s(&sendbufTanks[i], sizeof(float), (char*)&std::get<4>(tanks), sizeof(float));
						i += sizeof(float);
						memcpy_s(&sendbufTanks[i], sizeof(float), (char*)&std::get<5>(tanks), sizeof(float));
						i += sizeof(float);
					}

					sumSent = 0;
					while (sumSent < bytestosend) {
						int iResult = send(ClientSocket, &sendbufTanks[sumSent], bytestosend - sumSent, 0);
						if (iResult == SOCKET_ERROR) {
							printf("send tank list failed with error: %d\n", WSAGetLastError());
							closesocket(ClientSocket);
							WSACleanup();
							return 1;
						}
						sumSent += iResult;
					}

					delete[] sendbufTanks;

					//printf("sent tanks\n");
				}
			}
			//send if hit landmine
			{
				//2 ints = 8
				int bytestosend = 2 * sizeof(int) * hitLMs.size();
				char sendbuf[sizeof(int)];
				memcpy_s(&sendbuf[0], sizeof(int), (char*)&bytestosend, sizeof(int));

				int sumSent = 0;
				while (sumSent < sizeof(int)) {
					int iResult = send(ClientSocket, &sendbuf[sumSent], sizeof(int) - sumSent, 0);
					if (iResult == SOCKET_ERROR) {
						printf("send LM info failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					sumSent += iResult;
				}

				if (bytestosend != 0) {
					char* sendbufLM = new char[bytestosend]();

					int i = 0;
					for (auto LM : hitLMs) {
						Map.hitLandmine(LM.first, LM.second);
						memcpy_s(&sendbufLM[i], sizeof(int), (char*)&LM.first, sizeof(int));
						i += sizeof(int);
						memcpy_s(&sendbufLM[i], sizeof(int), (char*)&LM.second, sizeof(int));
						i += sizeof(int);
					}

					sumSent = 0;
					while (sumSent < bytestosend) {
						int iResult = send(ClientSocket, &sendbufLM[sumSent], bytestosend - sumSent, 0);
						if (iResult == SOCKET_ERROR) {
							printf("send LM list failed with error: %d\n", WSAGetLastError());
							closesocket(ClientSocket);
							WSACleanup();
							return 1;
						}
						sumSent += iResult;
					}

					delete[] sendbufLM;
				}
			}
			//send if hit health
			{
				//2 ints = 8
				int bytestosend = 2 * sizeof(int) * hitHPs.size();
				char sendbuf[sizeof(int)];
				memcpy_s(&sendbuf[0], sizeof(int), (char*)&bytestosend, sizeof(int));

				int sumSent = 0;
				while (sumSent < sizeof(int)) {
					int iResult = send(ClientSocket, &sendbuf[sumSent], sizeof(int) - sumSent, 0);
					if (iResult == SOCKET_ERROR) {
						printf("send hp kit info failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					sumSent += iResult;
				}

				if (bytestosend != 0) {
					char* sendbufHP = new char[bytestosend]();

					int i = 0;
					for (auto HP : hitHPs) {
						Map.hitHealth(HP.first, HP.second);
						memcpy_s(&sendbufHP[i], sizeof(int), (char*)&HP.first, sizeof(int));
						i += sizeof(int);
						memcpy_s(&sendbufHP[i], sizeof(int), (char*)&HP.second, sizeof(int));
						i += sizeof(int);
					}

					sumSent = 0;
					while (sumSent < bytestosend) {
						int iResult = send(ClientSocket, &sendbufHP[sumSent], bytestosend - sumSent, 0);
						if (iResult == SOCKET_ERROR) {
							printf("send hp kit list failed with error: %d\n", WSAGetLastError());
							closesocket(ClientSocket);
							WSACleanup();
							return 1;
						}
						sumSent += iResult;
					}

					delete[] sendbufHP;
				}
			}
		}
		else if (buffer == 0) {
			//client
			//get shots
			{
				int bytestorecv;
				int sumRecv = 0;
				char recvbuf[sizeof(int)];
				while (sumRecv < sizeof(int)) {
					int iResult = recv(ConnectSocket, &recvbuf[sumRecv], sizeof(int) - sumRecv, 0);

					if (iResult > 0) {
						sumRecv += iResult;
					}
					else if (iResult == 0) {
						printf("Connection closing...\n");
						finished = true;
						break;
					}
					else {
						printf("recv shots info failed with error: %d\n", WSAGetLastError());
						closesocket(ConnectSocket);
						WSACleanup();
						finished = true;
						break;
					}
				}

				memcpy_s((char*)&bytestorecv, sizeof(int), &recvbuf[0], sizeof(int));

				//printf("got shot info %i\n", bytestorecv);
				shots.clear();
				if (bytestorecv != 0) {
					char* recvbufShots = new char[bytestorecv];

					int sumRecv = 0;
					while (sumRecv < bytestorecv) {
						int iResult = recv(ConnectSocket, &recvbufShots[sumRecv], bytestorecv - sumRecv, 0);

						if (iResult > 0) {
							sumRecv += iResult;
						}
						else if (iResult == 0)
						{
							printf("Connection closing...\n");
							finished = true;
							break;
						}
						else {
							printf("recv shots failed with error: %d\n", WSAGetLastError());
							closesocket(ConnectSocket);
							WSACleanup();
							finished = true;
							break;
						}
					}

					for (int i = 0; i < bytestorecv;) {
						float sx, sy;
						memcpy_s((char*)&sx, sizeof(float), &recvbufShots[i], sizeof(float));
						i += sizeof(float);
						memcpy_s((char*)&sy, sizeof(float), &recvbufShots[i], sizeof(float));
						i += sizeof(float);
						int anim = rand() % 5;
						float angle = 0;

						shots.push_back(std::make_tuple(sx, sy, anim, angle));
					}

					delete[] recvbufShots;

					//printf("got shots\n");
				}
			}

			//get tanks
			{
				int bytestorecv;
				int sumRecv = 0;
				char recvbuf[sizeof(int)];
				while (sumRecv < sizeof(int)) {
					int iResult = recv(ConnectSocket, &recvbuf[sumRecv], sizeof(int) - sumRecv, 0);

					if (iResult > 0) {
						sumRecv += iResult;
					}
					else if (iResult == 0)
					{
						printf("Connection closing...\n");
						finished = true;
						break;
					}
					else {
						printf("recv tank info failed with error: %d\n", WSAGetLastError());
						closesocket(ConnectSocket);
						WSACleanup();
						finished = true;
						break;
					}
				}

				memcpy_s((char*)&bytestorecv, sizeof(int), &recvbuf[0], sizeof(int));

				//printf("recv tank info %i\n", bytestorecv);
				if (bytestorecv != 0) {
					char* recvbufTanks = new char[bytestorecv];

					int sumRecv = 0;
					while (sumRecv < bytestorecv) {
						int iResult = recv(ConnectSocket, &recvbufTanks[sumRecv], bytestorecv - sumRecv, 0);

						if (iResult > 0) {
							sumRecv += iResult;
						}
						else if (iResult == 0)
						{
							printf("Connection closing...\n");
							finished = true;
							break;
						}
						else {
							printf("recv tank list failed with error: %d\n", WSAGetLastError());
							closesocket(ConnectSocket);
							WSACleanup();
							finished = true;
							break;
						}
					}

					tankInfo.clear();
					for (int i = 0; i < bytestorecv;) {
						Color r_c;
						int hp;
						float rx, ry, ba, ta;
						memcpy_s((char*)&r_c, sizeof(Color), &recvbufTanks[i], sizeof(Color));
						i += sizeof(Color);
						memcpy_s((char*)&hp, sizeof(int), &recvbufTanks[i], sizeof(int));
						i += sizeof(int);
						memcpy_s((char*)&rx, sizeof(float), &recvbufTanks[i], sizeof(float));
						i += sizeof(float);
						memcpy_s((char*)&ry, sizeof(float), &recvbufTanks[i], sizeof(float));
						i += sizeof(float);
						memcpy_s((char*)&ba, sizeof(float), &recvbufTanks[i], sizeof(float));
						i += sizeof(float);
						memcpy_s((char*)&ta, sizeof(float), &recvbufTanks[i], sizeof(float));
						i += sizeof(float);

						tankInfo.push_back(std::make_tuple(r_c, hp, rx, ry, ba, ta));
					}

					for (auto tank : tankInfo) {
						if (myTank == std::get<0>(tank)) {
							myHP = std::get<1>(tank);
						}
					}

					delete[] recvbufTanks;

					//printf("got tanks\n");
				}
			}

			//get landmines
			{
				int bytestorecv;
				int sumRecv = 0;
				char recvbuf[sizeof(int)];
				while (sumRecv < sizeof(int)) {
					int iResult = recv(ConnectSocket, &recvbuf[sumRecv], sizeof(int) - sumRecv, 0);

					if (iResult > 0) {
						sumRecv += iResult;
					}
					else if (iResult == 0)
					{
						printf("Connection closing...\n");
						finished = true;
						break;
					}
					else {
						printf("recv LM info failed with error: %d\n", WSAGetLastError());
						closesocket(ConnectSocket);
						WSACleanup();
						finished = true;
						break;
					}
				}

				memcpy_s((char*)&bytestorecv, sizeof(int), &recvbuf[0], sizeof(int));

				if (bytestorecv != 0) {
					char* recvbufLM = new char[bytestorecv];

					int sumRecv = 0;
					while (sumRecv < bytestorecv) {
						int iResult = recv(ConnectSocket, &recvbufLM[sumRecv], bytestorecv - sumRecv, 0);

						if (iResult > 0) {
							sumRecv += iResult;
						}
						else if (iResult == 0)
						{
							printf("Connection closing...\n");
							finished = true;
							break;
						}
						else {
							printf("recv LM list failed with error: %d\n", WSAGetLastError());
							closesocket(ConnectSocket);
							WSACleanup();
							finished = true;
							break;
						}
					}

					for (int i = 0; i < bytestorecv;) {
						int l_x, l_y;
						memcpy_s((char*)&l_x, sizeof(int), &recvbufLM[i], sizeof(int));
						i += sizeof(int);
						memcpy_s((char*)&l_y, sizeof(int), &recvbufLM[i], sizeof(int));
						i += sizeof(int);

						Map.hitLandmine(l_x, l_y);
					}

					delete[] recvbufLM;
				}
			}

			//get health
			{
				int bytestorecv;
				int sumRecv = 0;
				char recvbuf[sizeof(int)];
				while (sumRecv < sizeof(int)) {
					int iResult = recv(ConnectSocket, &recvbuf[sumRecv], sizeof(int) - sumRecv, 0);

					if (iResult > 0) {
						sumRecv += iResult;
					}
					else if (iResult == 0)
					{
						printf("Connection closing...\n");
						finished = true;
						break;
					}
					else {
						printf("recv hp kit info failed with error: %d\n", WSAGetLastError());
						closesocket(ConnectSocket);
						WSACleanup();
						finished = true;
						break;
					}
				}

				memcpy_s((char*)&bytestorecv, sizeof(int), &recvbuf[0], sizeof(int));

				if (bytestorecv != 0) {
					char* recvbufHP = new char[bytestorecv];

					int sumRecv = 0;
					while (sumRecv < bytestorecv) {
						int iResult = recv(ConnectSocket, &recvbufHP[sumRecv], bytestorecv - sumRecv, 0);

						if (iResult > 0) {
							sumRecv += iResult;
						}
						else if (iResult == 0)
						{
							printf("Connection closing...\n");
							finished = true;
							break;
						}
						else {
							printf("recv hp kit list failed with error: %d\n", WSAGetLastError());
							closesocket(ConnectSocket);
							WSACleanup();
							finished = true;
							break;
						}
					}

					for (int i = 0; i < bytestorecv;) {
						int l_x, l_y;
						memcpy_s((char*)&l_x, sizeof(int), &recvbufHP[i], sizeof(int));
						i += sizeof(int);
						memcpy_s((char*)&l_y, sizeof(int), &recvbufHP[i], sizeof(int));
						i += sizeof(int);

						Map.hitHealth(l_x, l_y);
					}

					delete[] recvbufHP;
				}
			}
		}
		
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		diff = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
		int framecap = (int)(1000000.0 / 60.0);
		if (diff < framecap) {
			//60 fps in microseconds
			std::this_thread::sleep_for(std::chrono::microseconds(framecap - diff - 1000));
			//somehow there's a 1 ms delay on this func
		}
		end = std::chrono::steady_clock::now();
		diff = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();

		int fps = constrain((int)(1000000.0 / diff), 0, 999);
		std::string fps_string = std::to_string(fps);
		GameWindow.update(Map.getLoc(), Map.getSize(), Map.getW(), Map.getH(), myTank, tankInfo, shots, maxHP, fps_string, Map.getHealthKits(), Map.getLandmines());

		if (buffer == 0) {
			int remainHP = 0;
			for (auto tank : tankInfo) {
				if (myTank != std::get<0>(tank)) {
					remainHP += std::get<1>(tank);
				}
			}
			if (myHP == 0 || remainHP == 0) {
				finished = true;
			}
		}
		else {
			buffer--;
		}
	}

	if (myHP == 0) {
		GameWindow.loseScreen();
	}
	else {
		GameWindow.winScreen();
	}

	inMM = true;
	while (inMM) {
		SDL_Event e = { };
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				finished = true;
			}
			else if (e.type == SDL_KEYDOWN) {
				auto keyCode = e.key.keysym.sym;
				switch (keyCode) {
				case SDLK_ESCAPE:
					inMM = false;
					break;
				default:
					break;
				}
			}
		}
	}

	if (isServer) {
		closeServer(ClientSocket);
	}
	else {
		// shutdown the connection since no more data will be sent
		int iResult = shutdown(ConnectSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
		}
		else {
			closeClient(ConnectSocket);
		}
	}
	Map.cleanup();
	GameWindow.cleanup();
	return 0;
}