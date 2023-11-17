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

	const std::vector<Vertex> verticesWorld
	{
		{ { 0.f,   4.f, 2.f }, { 1.f, 0.f, 0.f } },
		{ { 3.f,  -2.f, 2.f }, { 0.f, 1.f, 0.f } },
		{ { -3.f, -2.f, 2.f }, { 0.f, 0.f, 1.f } },
		{ { 0.f,   4.f, -22.f }, { 1.f, 0.f, 0.f } },
		{ { 3.f,  -2.f, -22.f }, { 1.f, 1.f, 0.f } },
		{ { -3.f, -2.f, -22.f }, { 0.f, 0.f, 1.f } },
	};

	std::vector<Vertex> verticesScreen{};
	WorldToScreen(verticesWorld, verticesScreen);

	const size_t pixelCount{ static_cast<size_t>(m_Width) * static_cast<size_t>(m_Height) };
	float* depthBuffer{ new float[pixelCount] };
	std::fill_n(depthBuffer, pixelCount, FLT_MAX);

	// Clear screen
	SDL_FillRect(m_pBackBuffer, nullptr, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	for (size_t triIndex{ 0 }; triIndex < verticesScreen.size(); triIndex += 3)
	{
		if (verticesScreen[triIndex + 0].position.z < 0 ||
			verticesScreen[triIndex + 1].position.z < 0 ||
			verticesScreen[triIndex + 2].position.z < 0)
		{
			continue;
		}

#pragma region calculate bounding box
		const Vector3& v0Pos{ verticesScreen[triIndex + 0].position };
		const Vector3& v1Pos{ verticesScreen[triIndex + 1].position };
		const Vector3& v2Pos{ verticesScreen[triIndex + 2].position };

		// smallest x and y of all vertices
		Vector2i boundingBoxTopLeft{ Vector2i(
			static_cast<int>(std::min(v0Pos.x, std::min(v1Pos.x, v2Pos.x))),
			static_cast<int>(std::min(v0Pos.y, std::min(v1Pos.y, v2Pos.y)))
		) };

		// largest x and y of all vertices
		Vector2i boundingBoxBottomRight{ Vector2i(
			static_cast<int>(std::max(v0Pos.x, std::max(v1Pos.x, v2Pos.x))),
			static_cast<int>(std::max(v0Pos.y, std::max(v1Pos.y, v2Pos.y)))
		) };

		boundingBoxTopLeft.x = std::max(boundingBoxTopLeft.x, 0);
		boundingBoxTopLeft.x = std::min(boundingBoxTopLeft.x, m_Width - 1);
		boundingBoxTopLeft.y = std::max(boundingBoxTopLeft.y, 0);
		boundingBoxTopLeft.y = std::min(boundingBoxTopLeft.y, m_Height - 1);

		boundingBoxBottomRight.x = std::max(boundingBoxBottomRight.x, 0);
		boundingBoxBottomRight.x = std::min(boundingBoxBottomRight.x, m_Width - 1);
		boundingBoxBottomRight.y = std::max(boundingBoxBottomRight.y, 0);
		boundingBoxBottomRight.y = std::min(boundingBoxBottomRight.y, m_Height - 1);
#pragma endregion

		//RENDER LOGIC
		for (int px{ boundingBoxTopLeft.x }; px < boundingBoxBottomRight.x; ++px)
		{
			for (int py{ boundingBoxTopLeft.y }; py < boundingBoxBottomRight.y; ++py)
			{
				const Vector2 screenPos{ static_cast<float>(px) + 0.5f, static_cast<float>(py) + 0.5f };
				ColorRGB finalColor{ -1, -1, -1 };

				const GeometryUtils::TriResult res{
				GeometryUtils::HitTest_ScreenTriangle(
					screenPos,
					verticesScreen[triIndex + 0],
					verticesScreen[triIndex + 1],
					verticesScreen[triIndex + 2]
				)};

				if (res.hit)
				{
					const int pixelIndex{ px + py * m_Width };

					Vertex& v0{ verticesScreen[triIndex + 0] };
					Vertex& v1{ verticesScreen[triIndex + 1] };
					Vertex& v2{ verticesScreen[triIndex + 2] };


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

	delete[] depthBuffer;

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
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

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
