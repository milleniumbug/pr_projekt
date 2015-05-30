#ifndef GAME_H
#define GAME_H

#include <memory>
#include <vector>
#include "serialization.hpp"

const int ticks_in_a_second = 120;

class Point
{
	int x_;
	int y_;
public:
	Point(int x, int y) : x_(x), y_(y) {}

	int x() const { return x_; }
	int y() const { return y_; }
};

class Vector
{
	int x_;
	int y_;
public:
	Vector(int x, int y) : x_(x), y_(y) {}

	int x() const { return x_; }
	int y() const { return y_; }
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
private:
	int width_;
	std::vector<Entity> entities_;
public:
	int width() const { return width_; }
	int height() const { return entities_.size() / width_; }

	Entity& operator[](Point p)
	{
		return entities_[p.y() * width_ + p.x()];
	}

	const Entity& operator[](Point p) const
	{
		return entities_[p.y() * width_ + p.x()];
	}
};

class BombermanGame
{
	BombermanLevel current_level;
	unsigned long long ticks;
public:
	static bool refresh(const BombermanGame& source, BombermanGame& target);
};


#endif