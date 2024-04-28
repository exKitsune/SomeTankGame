#pragma once
#include <vector>

//view size 1500 x 750
//map size should be 3000x1500
class Map_1 {
private:
	std::vector<std::pair<int, int>> stoneLoc;
	std::vector<std::pair<int, int>> stoneSize;
	std::vector<std::pair<int, int>> healthKits;
	std::vector<std::pair<int, int>> landmines;
	int max_x = 3000, max_y = 1500;
	int* hitbox = nullptr;

	void addHitbox(int startX, int startY, int sizeX, int sizeY) {
		for (int x = startX; x < startX + sizeX; x++) {
			for (int y = startY; y < startY + sizeY; y++) {
				hitbox[x * max_y + y] = 1;
			}
		}
	}
public:
	Map_1() {
		hitbox = new int[max_x * max_y]();
		stoneLoc.push_back(std::make_pair(622, 300));
		stoneSize.push_back(std::make_pair(256, 256));

		for (int i = 622; i < 622 + 256; i++) {
			for (int j = 300; j < 300 + 256; j++) {
				hitbox[i*max_y + j] = 1;
			}
		}

		stoneLoc.push_back(std::make_pair(1700, 300));
		stoneSize.push_back(std::make_pair(256, 256));
		addHitbox(1700, 300, 256, 256);

		stoneLoc.push_back(std::make_pair(1400, 700));
		stoneSize.push_back(std::make_pair(256, 256));
		addHitbox(1400, 700, 256, 256);

		stoneLoc.push_back(std::make_pair(2400, 500));
		stoneSize.push_back(std::make_pair(256, 256));
		addHitbox(2400, 500, 256, 256);


		for (int i = 0; i < 3000; i += 500) {
			for (int j = 0; j < 1500; j += 1300) {
				stoneLoc.push_back(std::make_pair(i, j));
				stoneSize.push_back(std::make_pair(500, 200));

				int startX = i;
				int startY = j;
				int sizeX = 500;
				int sizeY = 200;
				for (int x = startX; x < startX + sizeX; x++) {
					for (int y = startY; y < startY + sizeY; y++) {
						hitbox[x * max_y + y] = 1;
					}
				}
			}
		}

		for (int i = 0; i < 3000; i += 2800) {
			for (int j = 0; j < 1500; j += 500) {
				stoneLoc.push_back(std::make_pair(i, j));
				stoneSize.push_back(std::make_pair(200, 500));

				int startX = i;
				int startY = j;
				int sizeX = 200;
				int sizeY = 500;
				for (int x = startX; x < startX + sizeX; x++) {
					for (int y = startY; y < startY + sizeY; y++) {
						hitbox[x * max_y + y] = 1;
					}
				}
			}
		}

		landmines.push_back(std::make_pair(1500, 1000));
		landmines.push_back(std::make_pair(700, 700));
		landmines.push_back(std::make_pair(300, 1100));
		landmines.push_back(std::make_pair(2000, 600));

		healthKits.push_back(std::make_pair(400, 500));
		healthKits.push_back(std::make_pair(1250, 600));
		healthKits.push_back(std::make_pair(900, 250));
		healthKits.push_back(std::make_pair(2400, 900));
	}

	void cleanup() {
		delete[] hitbox;
	}

	std::vector<std::pair<int, int>> getLoc() {
		return stoneLoc;
	}

	std::vector<std::pair<int, int>> getSize() {
		return stoneSize;
	}

	int getW() {
		return max_x;
	}

	int getH() {
		return max_y;
	}

	std::vector<std::pair<int, int>> getLandmines() {
		return landmines;
	}

	std::vector<std::pair<int, int>> getHealthKits() {
		return healthKits;
	}

	void hitLandmine(int x, int y) {
		std::vector<std::pair<int, int>> updated;
		for (std::pair<int, int> mine : landmines) {
			if (mine.first == x && mine.second == y) {
				continue;
			}
			else {
				updated.push_back(mine);
			}
		}

		landmines = updated;
	}

	void hitHealth(int x, int y) {
		std::vector<std::pair<int, int>> updated;
		for (std::pair<int, int> hp : healthKits) {
			if (hp.first == x && hp.second == y) {
				continue;
			}
			else {
				updated.push_back(hp);
			}
		}

		healthKits = updated;
	}

	bool collide(int x, int y) {
		int index = x * max_y + y;
		if (index < 0 || index > max_x * max_y) {
			return true;
		}
		return hitbox[index] == 1;
	}
};

