#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>
#include <iostream>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		SDL_Surface* surface{ IMG_Load(path.c_str()) };
		if (surface == nullptr)
		{
			throw TextureLoadFailedException();
		}
		
		return new Texture(surface);
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		if (uv.x <= 0) return {};
		const int pixelX{ static_cast<int>(static_cast<float>(m_pSurface->w) * uv.x) };
		const int pixelY{ static_cast<int>(static_cast<float>(m_pSurface->h) * uv.y)};

		const int pixelIndex{ m_pSurface->w * pixelY + pixelX };
		const uint32_t pixel{ m_pSurfacePixels[pixelIndex] };
		
		uint8_t r{};
		uint8_t g{};
		uint8_t b{};
		SDL_GetRGB(pixel, m_pSurface->format, &r, &g, &b);

		return { ColorRGB{
			static_cast<float>(r) / 255.f,
			static_cast<float>(g) / 255.f,
			static_cast<float>(b) / 255.f
		} };
	}
}