#include "pch.h"
#include "../Library/src/Maths.h"
using namespace dae;

TEST(Library, LinearAlgebraTests)
{
	EXPECT_EQ(Vector3::Cross(Vector3::UnitX, Vector3::UnitY), Vector3::UnitZ);
}
