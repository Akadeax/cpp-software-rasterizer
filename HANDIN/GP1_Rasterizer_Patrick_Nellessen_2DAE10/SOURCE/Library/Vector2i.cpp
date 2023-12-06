#include "Vector2i.h"

#include "Vector2.h"
#include "Vector3.h"

dae::Vector2i::Vector2i(int x, int y) : x(x), y(y) {}

bool dae::Vector2i::operator==(const Vector2i& v) const
{
	return v.x == x && v.y == y;
}

dae::Vector2 dae::Vector2i::ToVector2() const
{
	return Vector2(x, y);
}

dae::Vector3 dae::Vector2i::ToVector3() const
{

}
