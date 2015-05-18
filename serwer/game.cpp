#include "precomp.hpp"
#include "game.hpp"

bool BombermanGame::refresh(const BombermanGame& source, BombermanGame& target)
{
	target.ticks = source.ticks+1;

}

Point translate(Point source, Vector displacement)
{
	return Point(source.x() + displacement.x(), source.y() + displacement.y());
}