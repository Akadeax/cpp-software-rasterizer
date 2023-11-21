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
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	const std::vector meshesWorld
	{
		Mesh{
			std::vector<Vertex>{
			{ { -3.f,  3.f, -2.f } },
			{ { 0.f,  3.f, -2.f } },
			{ { 3.f,  3.f, -2.f } },
			{ { -3.f,  0.f, -2.f } },
			{ { 0.f,  0.f, -2.f } },
			{ { 3.f,  0.f, -2.f } },
			{ { -3.f,  -3.f, -2.f } },
			{ { 0.f,  -3.f, -2.f } },
			{ { 3.f,  -3.f, -2.f } },
			},
			//std::vector<uint32_t>{
			//	3,0,1, 1,4,3, 4,1,2,
			//	2,5,4, 6,3,4, 4,7,6,
			//	7,4,5, 5,8,7,
			//},
			//PrimitiveTopology::TriangleList
			std::vector<uint32_t>{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5,
			},
			PrimitiveTopology::TriangleStrip
		}
	};


	const size_t pixelCount{ static_cast<size_t>(m_Width) * static_cast<size_t>(m_Height) };
	float* depthBuffer{ new float[pixelCount] };
	std::fill_n(depthBuffer, pixelCount, FLT_MAX);

	// Clear screen
	SDL_FillRect(m_pBackBuffer, nullptr, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));


	for (const Mesh& mesh : meshesWorld)
	{
		std::vector<Vertex> meshVerticesScreen{};
		WorldToScreen(mesh.vertices, meshVerticesScreen);

		switch (mesh.primitiveTopology)
		{
		case PrimitiveTopology::TriangleList:
			for (size_t i{ 0 }; i < mesh.indices.size(); i += 3)
			{
				RenderTri(
					meshVerticesScreen[mesh.indices[i + 0]],
					meshVerticesScreen[mesh.indices[i + 1]],
					meshVerticesScreen[mesh.indices[i + 2]],
					depthBuffer
				);
			}
			break;

		case PrimitiveTopology::TriangleStrip:
			bool clockwise{ true };
			for (size_t i{ 0 }; i < mesh.indices.size() - 2; ++i)
			{
				if (clockwise)
				{
					RenderTri(
						meshVerticesScreen[mesh.indices[i + 0]],
						meshVerticesScreen[mesh.indices[i + 1]],
						meshVerticesScreen[mesh.indices[i + 2]],
						depthBuffer
					);
				}
				else
				{
					RenderTri(
						meshVerticesScreen[mesh.indices[i + 2]],
						meshVerticesScreen[mesh.indices[i + 1]],
						meshVerticesScreen[mesh.indices[i + 0]],
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

void Renderer::WorldToScreen(const std::vector<Vertex>& inVertices, std::vector<Vertex>& outVertices) const
{
	const float aspectRatio{ static_cast<float>(m_Width) / static_cast<float>(m_Height) };

	outVertices.reserve(inVertices.size());
	for (size_t i{ 0 }; i < inVertices.size(); ++i)
	{
		const Vector3 view{ m_Camera.viewMatrix.TransformPoint(inVertices[i].position) };

		// Perspective Divide
		float projectedX{ view.x / view.z };
		float projectedY{ view.y / view.z };

		projectedX = projectedX / (aspectRatio * m_Camera.fov);
		projectedY = projectedY / m_Camera.fov;

		const Vector3 projected{ projectedX, projectedY, view.z };

		outVertices.emplace_back(Vertex{ NdcToScreen(projected), inVertices[i].color });
	}
}

Vector3 Renderer::NdcToScreen(Vector3 ndc) const
{
	return {
		(ndc.x + 1.f) / 2.f * static_cast<float>(m_Width),
		(1.f - ndc.y) / 2.f * static_cast<float>(m_Height),
		ndc.z
	};
}

void Renderer::RenderTri(const Vertex& v0, const Vertex& v1, const Vertex& v2, float* depthBuffer) const
{
	if (v0.position.z < 0 ||
		v1.position.z < 0 ||
		v2.position.z < 0)
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

				const float hitDepth{
					res.w0 * v0.position.z +
					res.w1 * v1.position.z +
					res.w2 * v2.position.z
				};

				// Depth test
				if (depthBuffer[pixelIndex] < hitDepth) continue;

				depthBuffer[pixelIndex] = hitDepth;


				finalColor = {
					res.w0 * v0.color +
					res.w1 * v1.color +
					res.w2 * v2.color
				};


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

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
