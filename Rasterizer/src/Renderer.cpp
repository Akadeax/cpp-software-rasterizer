//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
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
	};

	std::vector<Vertex> verticesScreen{};
	WorldToScreen(verticesWorld, verticesScreen);


	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			const Vector2 screenPos{ static_cast<float>(px) + 0.5f, static_cast<float>(py) + 0.5f };
			ColorRGB finalColor{};

			if (verticesScreen.size() >= 3)
			{
				const GeometryUtils::TriResult res{
				GeometryUtils::HitTest_ScreenTriangle(screenPos,
				verticesScreen[0], verticesScreen[1], verticesScreen[2])
				};

				if (res.hit)
				{
					finalColor = {
						res.w0 * verticesScreen[0].color +
						res.w1 * verticesScreen[1].color +
						res.w2 * verticesScreen[2].color
					};
				}
			}


			

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255)
			);
		}
	}

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

		if (view.z < 0.f) continue;

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
		0
	};
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
