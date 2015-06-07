#ifndef GAME_H
#define GAME_H

#include <memory>
#include <array>
#include <type_traits>
#include <vector>
#include "serialization.hpp"
#include "variant.hpp"

const int ticks_in_a_second = 120;
const int seconds_in_a_minute = 60;
const int milliseconds_in_a_second = 1000;

class Point
{
	int x_;
	int y_;
public:
	Point(int x, int y) : x_(x), y_(y) {}

	int x() const { return x_; }
	int y() const { return y_; }
};

bool operator==(const Point& lhs, const Point& rhs);
bool operator!=(const Point& lhs, const Point& rhs);

class Vector
{
	int x_;
	int y_;
public:
	Vector(int x, int y) : x_(x), y_(y) {}

	int x() const { return x_; }
	int y() const { return y_; }
};

static const Vector up(0, -1);
static const Vector down(0, 1);
static const Vector left(-1, 0);
static const Vector right(1, 0);

static const std::array<Vector, 4> directions = { up, down, left, right };

Point translate(Point source, Vector displacement);

class BombermanGame;

class Player
{
private:
	int direction_;
	int time_to_stop_;
	int time_set_bomb_;
	Point position_;
	bool is_hurt_;
	bool czy_klasc_bombe_;
	static const int next_move = ticks_in_a_second;
public:
	void set_next_input(int dir)
	{
		direction_ = dir;
		time_to_stop_ = next_move;
	}

	Point position() const
	{
		return position_;
	}

	bool is_hurt() const
	{
		return is_hurt_;
	}

	void refresh(BombermanGame& world);
	void hurt();

	Player(Point position) :
		direction_(0),
		time_to_stop_(0),
		time_set_bomb_(0),
		position_(position),
		is_hurt_(false),
		czy_klasc_bombe_(false)
	{

	}

	void ustaw_bombe();
};

struct DestructibleWall
{
	typedef std::false_type is_passable;
};

struct NondestructibleWall
{
	typedef std::false_type is_passable;
};

struct EmptySpace
{
	typedef std::true_type is_passable;
};

class BombermanGame;

struct Bomb
{
	int remaining_time_until_explosion;
	int explosion_radius;
	Player* owner;

	void trigger_explosion(BombermanGame& world, Point position);

	typedef std::true_type is_passable;

	Bomb(int remaining_time_until_explosion, int explosion_radius, Player* owner) :
		remaining_time_until_explosion(remaining_time_until_explosion),
		explosion_radius(explosion_radius),
		owner(owner)
	{

	}
};

typedef Variant<EmptySpace, DestructibleWall, NondestructibleWall, Bomb> Entity;

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

	BombermanLevel(int width, int height) :
		width_(width)
	{
		entities_.resize(width*height);
	}

	auto begin() { return entities_.begin(); }
	auto end() { return entities_.end(); }
	auto begin() const { return entities_.begin(); }
	auto end() const { return entities_.end(); }
	auto cbegin() const { return entities_.cbegin(); }
	auto cend() const { return entities_.cend(); }
};

class BombermanGame
{
	unsigned long long ticks;
	unsigned long long round_time;
public:
	bool refresh();

	int width() const { return current_level.width(); }
	int height() const { return current_level.height(); }

	Entity& operator[](Point p)
	{
		return current_level[p];
	}

	const Entity& operator[](Point p) const
	{
		return current_level[p];
	}

	int remaining_time()
	{
		unsigned long long remaining_time = ticks < round_time ? round_time - ticks : 0;
		return static_cast<int>(remaining_time * milliseconds_in_a_second / ticks_in_a_second);
	}

	Point translate(Point source, Vector displacement) const;

	BombermanGame(BombermanLevel level, unsigned long long round_time) :
		ticks(0),
		round_time(round_time),
		current_level(std::move(level))
	{
		
	}

	BombermanLevel current_level;
	std::vector<Player> players;

	static const int min_graczy = 2;
	static const int max_graczy = 4;
};

template<typename T, typename U, typename V>
bool in_range(T x, U min, V max)
{
	return x >= min && x <= max;
};

template<typename OutputIterator>
void serialize_to(OutputIterator output, BombermanGame& gamestate)
{
	serialize_to(output, static_cast<uint32_t>(gamestate.remaining_time()));
	for(auto& x : gamestate.current_level)
	{
		char deserialized_entity;
		dispatch(functions(
		[&](const EmptySpace&){ deserialized_entity = 0; },
		[&](const DestructibleWall&){ deserialized_entity = 1; },
		[&](const NondestructibleWall&){ deserialized_entity = 2; },
		[&](const Bomb&){ deserialized_entity = 3; }), x);
		*output++ = deserialized_entity;
	}
	for(auto& player : gamestate.players)
	{
		Point pos = player.position();
		assert(in_range(pos.x(), std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max()));
		assert(in_range(pos.y(), std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max()));
		serialize_to(output, static_cast<uint16_t>(pos.x()));
		serialize_to(output, static_cast<uint16_t>(pos.y()));
		// TODO: na razie żadnych bonusów nie ma
		serialize_to(output, static_cast<uint32_t>(0));
	}
}

#endif