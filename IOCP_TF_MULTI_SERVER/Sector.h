#pragma once
#include <Windows.h>
constexpr int SECTOR_WIDTH = 64 * 2;
constexpr int SECTOR_HEIGHT = 64 * 2;
constexpr int SECTOR_MAX_X = 6400 / SECTOR_WIDTH;
constexpr int SECTOR_MAX_Y = 6400 / SECTOR_HEIGHT;
struct Sector
{
	int x;
	int y;
	SRWLOCK _SecLock;
};

struct SectorAround
{
	int sectorCount;
	Sector aroundSector[9];
};

