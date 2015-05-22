#ifndef GAME_H
#define GAME_H

#include <memory>
#include <vector>

const int ticks_in_a_second = 120;

class Point
{
	int x_;
	int y_;
public:
	Point(int x, int y) : x_(x), y_(y) {}

	int x() { return x_; }
	int y() { return y_; }
};

class Vector
{
	int x_;
	int y_;
public:
	Vector(int x, int y) : x_(x), y_(y) {}

	int x() { return x_; }
	int y() { return y_; }
};

Point translate(Point source, Vector displacement);

class PlayerEntity
{

};

enum class Entity
{

};

class BombermanLevel
{
	int width;
	std::vector<Entity> entities;
};

class BombermanGame
{
	BombermanLevel current_level;
	unsigned long long ticks;
public:
	static bool refresh(const BombermanGame& source, BombermanGame& target);
};


#endif