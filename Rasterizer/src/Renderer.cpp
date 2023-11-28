//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"

#include <iostream>

#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = static_cast<uint32_t*>(m_pBackBuffer->pixels);

	//Initialize Camera
	m_Camera.Initialize(
		60.f,
		{ .0f,5.f,-30.f },
		static_cast<float>(m_Width) / static_cast<float>(m_Height)
	);

	Mesh tuktuk{};

	Utils::ParseOBJ("Resources/tuktuk.obj", tuktuk.vertices, tuktuk.indices);

	m_SceneMeshes = {
		tuktuk
		//Mesh{
		//	std::vector<Vertex>{
		//	{ { -3.f,  3.f, -2.f }, {}, { 0.f, 0.f } },
		//	{ { 0.f,  3.f, -2.f }, {}, { 0.5f, 0.f } },
		//	{ { 3.f,  3.f, -2.f }, {}, { 1.f, 0.f } },
		//	{ { -3.f,  0.f, -2.f }, {}, { 0.f, 0.5f } },
		//	{ { 0.f,  0.f, -2.f }, {}, { 0.5f, 0.5f } },
		//	{ { 3.f,  0.f, -2.f }, {}, { 1.f, 0.5f } },
		//	{ { -3.f,  -3.f, -2.f }, {}, { 0.f, 1.f } },
		//	{ { 0.f,  -3.f, -2.f }, {}, { 0.5f, 1.f } },
		//	{ { 3.f,  -3.f, -2.f }, {}, { 1.f, 1.f } },
		//	},
		//	//std::vector<uint32_t>{
		//	//	3,0,1, 1,4,3, 4,1,2,
		//	//	2,5,4, 6,3,4, 4,7,6,
		//	//	7,4,5, 5,8,7,
		//	//},
		//	//PrimitiveTopology::TriangleList
		//	std::vector<uint32_t>{
		//		3,0,4,1,5,2,
		//		2,6,
		//		6,3,7,4,8,5,
		//	},
		//	PrimitiveTopology::TriangleStrip,
		//	0
		//}
	};
}

Renderer::~Renderer()
{
	for (const Material& mat : m_Materials)
	{
		delete mat.pTexture;
	}
}

void Renderer::Update(const Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	const size_t pixelCount{ static_cast<size_t>(m_Width) * static_cast<size_t>(m_Height) };
	float* depthBuffer{ new float[pixelCount] };
	std::fill_n(depthBuffer, pixelCount, FLT_MAX);

	// Clear screen
	SDL_FillRect(m_pBackBuffer, nullptr, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	AddMaterial("Resources/tuktuk.png");

	for (Mesh& mesh : m_SceneMeshes)
	{
		WorldToScreen(mesh);

		switch (mesh.primitiveTopology)
		{
		case PrimitiveTopology::TriangleList:
			for (size_t i{ 0 }; i < mesh.indices.size(); i += 3)
			{
				RenderScreenTri(
					mesh.verticesOut[mesh.indices[i + 0]],
					mesh.verticesOut[mesh.indices[i + 1]],
					mesh.verticesOut[mesh.indices[i + 2]],
					m_Materials[mesh.materialId],
					depthBuffer
				);
			}
			break;

		case PrimitiveTopology::TriangleStrip:

			bool clockwise{ true };
			for (size_t i{ 0 }; i < mesh.indices.size() - 2; ++i)
			{
				Vertex_Out& v0{ mesh.verticesOut[mesh.indices[i + 0]] };
				Vertex_Out& v1{ mesh.verticesOut[mesh.indices[i + 1]] };
				Vertex_Out& v2{ mesh.verticesOut[mesh.indices[i + 2]] };

				if (
					v0.position == v1.position ||
					v1.position == v2.position ||
					v0.position == v2.position)
				{
					continue;
				}

				if (clockwise)
				{
					RenderScreenTri(
						v0, v1, v2, m_Materials[mesh.materialId],
						depthBuffer
					);
				}
				else
				{
					RenderScreenTri(
						v2, v1, v0, m_Materials[mesh.materialId],
						depthBuffer
					);
				}

				clockwise = !clockwise;
			}
			break;
		}
	}

	delete[] depthBuffer;

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, nullptr, m_pFrontBuffer, nullptr);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::WorldToScreen(Mesh& mesh) const
{
	mesh.verticesOut.clear();
	mesh.verticesOut.reserve(mesh.vertices.size());

	for (size_t i{ 0 }; i < mesh.vertices.size(); ++i)
	{
		// Mesh > World > View > Clipping
		const Matrix worldViewProjection{ mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };
		Vector4 transformed{ worldViewProjection.TransformPoint({ mesh.vertices[i].position, 1 }) };

		// Perspective Divide
		transformed.x /= transformed.w;
		transformed.y /= transformed.w;
		transformed.z /= transformed.w;

		// Do Clipping > Image and insert back
		mesh.verticesOut.emplace_back(Vertex_Out{ NdcToScreen(transformed), mesh.vertices[i].color, mesh.vertices[i].uv });
	}
}

Vector4 Renderer::NdcToScreen(Vector4 ndc) const
{
	return {
		(ndc.x + 1.f) / 2.f * static_cast<float>(m_Width),
		(1.f - ndc.y) / 2.f * static_cast<float>(m_Height),
		ndc.z,
		ndc.w
	};
}

void Renderer::CycleRenderMode()
{
	m_RenderMode = static_cast<RenderMode>((static_cast<int>(m_RenderMode) + 1) % (static_cast<int>(RenderMode::depth) + 1));
}

void Renderer::RenderScreenTri(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2, const Material& mat, float* depthBuffer) const
{
	if (v0.position.z < 0 || v0.position.z > 1 ||
	    v1.position.z < 0 || v1.position.z > 1 ||
	    v2.position.z < 0 || v2.position.z > 1)
	{
		return;
	}


	const GeometryUtils::ScreenBoundingBox bound{ GeometryUtils::GetScreenBoundingBox(
		v0.position, v1.position, v2.position,
		m_Width, m_Height
	) };
	for (int px{ bound.topLeft.x }; px < bound.bottomRight.x; ++px)
	{
		for (int py{ bound.topLeft.y }; py < bound.bottomRight.y; ++py)
		{
			const Vector2 screenPos{ static_cast<float>(px) + 0.5f, static_cast<float>(py) + 0.5f };
			ColorRGB finalColor{ -1, -1, -1 };

			const GeometryUtils::TriResult res{
			GeometryUtils::HitTest_ScreenTriangle(
				screenPos, v0, v1, v2
			) };

			if (res.hit)
			{
				const int pixelIndex{ px + py * m_Width };

				// position.z holds view depth for all vertices
				const float hitDepth{
					1.f / 
					(
						(1.f / v0.position.w * res.w0) +
						(1.f / v1.position.w * res.w1) +
						(1.f / v2.position.w * res.w2)
					)
				};


				// Depth test
				if (depthBuffer[pixelIndex] < hitDepth) continue;

				depthBuffer[pixelIndex] = hitDepth;

				if (m_RenderMode == RenderMode::depth)
				{
					const float nonLinearDepth{
						1.f /
						(
							(1.f / v0.position.z * res.w0) +
							(1.f / v1.position.z * res.w1) +
							(1.f / v2.position.z * res.w2)
						)
					};

					float colorDepth{ Remap(nonLinearDepth, 0.85f, 1.f, 0.f, 1.f) };
					finalColor = colors::White * std::clamp(colorDepth, 0.f, 1.f);
				}
				else if (m_RenderMode == RenderMode::standard)
				{
					const Vector2 uvCoords{
						(
							(v0.uv / v0.position.w * res.w0) +
							(v1.uv / v1.position.w * res.w1) +
							(v2.uv / v2.position.w * res.w2)
						) * hitDepth
					};

					finalColor = mat.pTexture->Sample(uvCoords);
				}

				//Update Color in Buffer
				finalColor.MaxToOne();

				m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255)
				);
			}
		}
	}
}

// Returns MaterialID
size_t Renderer::AddMaterial(const std::string& texturePath)
{
	m_Materials.push_back(Material{ Texture::LoadFromFile(texturePath) });
	return m_Materials.size() - 1;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
