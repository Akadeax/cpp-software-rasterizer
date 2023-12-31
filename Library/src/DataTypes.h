#pragma once
#include "Maths.h"
#include "Texture.h"
#include "vector"

namespace dae
{
	struct Vertex
	{
		Vector3 position{};
		ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{}; //W4
		Vector3 tangent{}; //W4
		Vector3 viewDirection{}; //W4
	};

	struct Vertex_Out
	{
		Vector4 position{};
		ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
	};

	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};

	struct Mesh
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleList };

		size_t materialId{};

		std::vector<Vertex_Out> verticesOut{};
		Matrix worldMatrix{};
	};

	class Material
	{
	public:
		Texture* pDiffuse;
		Texture* pNormal;
		Texture* pSpecular;
		Texture* pGloss;
	};
}
