#include <iostream>
#include <matplotlibcpp.h>
#include <algorithm>
#include <random>
#include <zip.h>
#include <enumerate.h>

// NEAT
#include "neat.h"
#include "network.h"
#include "population.h"
#include "organism.h"
#include "genome.h"
#include "species.h"

namespace plt = matplotlibcpp;
using namespace std;
using namespace fpp;

const int WorldSizeX = 20;
const int WorldSizeY = 20;

enum CellType { EmptyCell = 0, FoodCell, HarmCell,  };
CellType World[WorldSizeX][WorldSizeY] = {};

struct Individual
{
	float Life = 100;
	int PosX = WorldSizeX / 2;
	int PosY = WorldSizeY / 2;

	void Step()
	{
		PosX++;

		PosX = PosX % WorldSizeX;
		PosY = PosY % WorldSizeY;
	}
};

int main()
{
	plt::ion();

	// Need to separate them for the plot
	vector<int> food_x(10);
	vector<int> food_y(10);
	vector<int> harm_x(10);
	vector<int> harm_y(10);
	
	mt19937 rng(42);
	for (auto& x : food_x)
		x = uniform_int_distribution<>(0, WorldSizeX-1)(rng);
	for (auto& y : food_y)
		y = uniform_int_distribution<>(0, WorldSizeY-1)(rng);
	for (auto& x : harm_x)
		x = uniform_int_distribution<>(0, WorldSizeX-1)(rng);
	for (auto& y : harm_y)
		y = uniform_int_distribution<>(0, WorldSizeY-1)(rng);

	for (auto [x, y] : zip(food_x, food_y))
		World[x][y] = FoodCell;
	for (auto [x, y] : zip(harm_x, harm_x))
		World[x][y] = HarmCell;

	// Simulate and plot positions
	Individual org;
	while (true)
	{
		org.Step();

		plt::clf();

		plt::plot(food_x, food_y, "bo");
		plt::plot(harm_x, harm_y, "ro");
		plt::plot({ (float)org.PosX }, { (float)org.PosY }, "xk");

		plt::xlim(0, WorldSizeX-1);
		plt::ylim(0, WorldSizeY-1);

		plt::pause(0.01);
	}

	// TODO : CLEAR EVERYTHING HERE!

	return 0;
}