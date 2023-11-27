#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render() const;

		bool SaveBufferToImage() const;

		void WorldToScreen(Mesh& mesh) const;

		Vector4 NdcToScreen(Vector4 ndc) const;

	private:
		std::vector<Vertex> m_WorldVertices{};

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		const Texture* m_pTex{};

		void RenderScreenTri(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2, float* depthBuffer) const;
	};
}
