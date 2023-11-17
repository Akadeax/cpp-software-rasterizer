#pragma once

namespace dae
{
	struct Vector2;
	struct Vector3;

	struct Vector2i
	{
		int x{};
		int y{};

		Vector2i() = default;
		Vector2i(int x, int y);

		bool operator==(const Vector2i& v) const;

		Vector2 ToVector2() const;
	};
}
