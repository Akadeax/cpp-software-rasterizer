#include "pch.h"
#include "../Library/src/Maths.h"
using namespace dae;

TEST(Library, LinearAlgebraTests)
{
	EXPECT_EQ(Vector3::Cross(Vector3::UnitX, Vector3::UnitY), Vector3::UnitZ);

	float remap{ Remap(0.5f, 0, 1, 0.5f, 1.f) };
	EXPECT_TRUE(AreEqual(remap, 0.75f));
}
