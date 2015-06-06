#include <array>
#include <algorithm>
#include <iterator>
#include "precomp.hpp"
#include "game.hpp"

bool operator==(const Point& lhs, const Point& rhs)
{
	return lhs.x() == rhs.x() && lhs.y() == rhs.y();
}

bool operator!=(const Point& lhs, const Point& rhs)
{
	return !(lhs == rhs);
}

bool BombermanGame::refresh()
{
	++ticks;

	for(int i = 0; i < height(); ++i)
	{
		for(int j = 0; j < width(); ++j)
		{
			auto point = Point(i, j);
			dispatch(functions(
			[&](Bomb& bomb)
			{
				--bomb.remaining_time_until_explosion;
				if(bomb.remaining_time_until_explosion == 0)
					bomb.trigger_explosion(*this, point);
			},
			[](Default){}), (*this)[point]);
		}
	}
	for(auto& player : players)
		player.refresh(*this);
}

Point BombermanGame::translate(Point source, Vector displacement) const
{
	Point new_pos = ::translate(source, displacement);
	if(new_pos.x() < 0)
		new_pos = Point(0, new_pos.y());
	if(new_pos.y() < 0)
		new_pos = Point(new_pos.x(), 0);
	if(new_pos.x() >= width())
		new_pos = Point(width()-1, new_pos.y());
	if(new_pos.y() >= height())
		new_pos = Point(new_pos.x(), height()-1);
	return new_pos;
}

Point translate(Point source, Vector displacement)
{
	return Point(source.x() + displacement.x(), source.y() + displacement.y());
}

void Bomb::trigger_explosion(BombermanGame& world, Point position)
{
	int radius = explosion_radius-1;
	std::vector<Point> neighbour_positions;
	std::fill_n(std::back_inserter(neighbour_positions), 4, position);
	for(int i = 0; i < radius; ++i)
	{
		for(auto& player : world.players)
			if(player.position() == position)
				player.hurt();
		
		using namespace std::placeholders;
		std::transform(
			neighbour_positions.begin(),
			neighbour_positions.end(),
			directions.begin(),
			neighbour_positions.begin(),
			std::bind(&BombermanGame::translate, &world, _1, _2));
		for(auto x : neighbour_positions)
		{
			dispatch(functions(
			[&](Bomb& bomb)
			{
				bomb.remaining_time_until_explosion = 1;
			},
			[&](DestructibleWall& wall)
			{
				world[x] = Entity();
			},
			[](Default){}), world[x]);
			for(auto& player : world.players)
				if(player.position() == x)
					player.hurt();
		}
	}
	world[position] = Entity();
}

Vector direction_from_int(int dir)
{
	if(dir == -1)
		return up;
	if(dir == 1)
		return down;
	if(dir == -4)
		return left;
	if(dir == 4)
		return right;
	assert(false);
	return Vector(0, 0);
}

void Player::refresh(BombermanGame& world)
{
	if(is_hurt_)
		return;

	if(direction_ != 0)
		--time_to_stop_;

	if(time_set_bomb_ > 0)
		--time_set_bomb_;

	if(direction_ != 0 && time_to_stop_ <= 0)
	{
		Point next_pos = world.translate(position_, direction_from_int(direction_));
		bool is_passable;
		dispatch([&](auto x){ is_passable = decltype(x)::is_passable::value; }, world[next_pos]);
		if(is_passable)
			position_ = next_pos;
		time_to_stop_ = next_move;
	}

	if(czy_klasc_bombe_)
	{
		dispatch(functions(
		[&](EmptySpace& sp)
		{
			world[position_] = Bomb(ticks_in_a_second * 5, 2, this);
			czy_klasc_bombe_ = false;
		},
		[](Default){}), world[position_]);
	}
}

void Player::hurt()
{
	is_hurt_ = true;
}

void Player::ustaw_bombe()
{
	czy_klasc_bombe_ = true;
}