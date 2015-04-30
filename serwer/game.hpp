#ifndef GAME_H
#define GAME_H

#include <memory>
#include <vector>

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

Point translate(Point source, Vector displacement)
{
	return Point(source.x() + displacement.x(), source.y() + displacement.y());
}

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
};

bool refresh(const BombermanGame& source, BombermanGame& target);

#endif