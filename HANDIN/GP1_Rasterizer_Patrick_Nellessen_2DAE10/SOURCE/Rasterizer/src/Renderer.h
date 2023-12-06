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
		enum class RenderMode
		{
			standard, depth
		};

		enum class ShadingMode
		{
			combined,
			observedArea,
			diffuse,
			specular
		};

		explicit Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void WorldToScreen(Mesh& mesh) const;

		Vector4 NdcToScreen(Vector4 ndc) const;

		void CycleRenderMode();
		void CycleRotationMode();
		void CycleShadingMode();
		void CycleNormalMode();

	private:
		std::vector<Mesh> m_SceneMeshes{};
		std::vector<Material> m_Materials{};

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		RenderMode m_RenderMode{};
		ShadingMode m_ShadingMode{};
		bool m_Rotating{ true };
		bool m_UsingNormalMap{ true };

		float m_CurrentRotation{ 0.f };

		void RenderScreenTri(
			const Vertex_Out& v0,
			const Vertex_Out& v1,
			const Vertex_Out& v2,
			const Material& mat,
			float* depthBuffer
		) const;

		size_t AddMaterial(
			const std::string& diffuse = "",
			const std::string& normal = "",
			const std::string& specular = "",
			const std::string& gloss = ""
		);

		ColorRGB Shade(const Vertex_Out& vertex, const Material& material) const;


	};
}
