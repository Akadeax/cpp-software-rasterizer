//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"

#include <iostream>

#include "BRDFs.h"
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
		45.f,
		{ .0f,5.f,-64.f },
		static_cast<float>(m_Width) / static_cast<float>(m_Height)
	);

	Mesh vehicle{};
	vehicle.worldMatrix = Matrix::CreateTranslation(0, 0, 50.f);

	Utils::ParseOBJ("../_Resources/vehicle.obj", vehicle.vertices, vehicle.indices);

	m_SceneMeshes = {
		vehicle
	};

	vehicle.materialId = AddMaterial(
		"../_Resources/vehicle_diffuse.png",
		"../_Resources/vehicle_normal.png",
		"../_Resources/vehicle_specular.png",
		"../_Resources/vehicle_gloss.png"
	);
}

Renderer::~Renderer()
{
	for (const Material& mat : m_Materials)
	{
		delete mat.pDiffuse;
		delete mat.pNormal;
		delete mat.pSpecular;
		delete mat.pGloss;
	}
}

void Renderer::Update(const Timer* pTimer)
{
	m_Camera.Update(pTimer);

	if (m_Rotating) m_CurrentRotation += PI_DIV_4 * pTimer->GetElapsed();

	const Matrix rotation{ Matrix::CreateRotationY(m_CurrentRotation) };

	m_SceneMeshes[0].worldMatrix = rotation;
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
	if (mesh.verticesOut.size() != mesh.vertices.size())
	{
		mesh.verticesOut.clear();
		mesh.verticesOut.resize(mesh.vertices.size());
	}

	const Matrix worldViewProjection{ mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };

	for (size_t i{ 0 }; i < mesh.vertices.size(); ++i)
	{
		// Mesh > World > View > Clipping
		Vector4 transformed{ worldViewProjection.TransformPoint({ mesh.vertices[i].position, 1 }) };

		// Perspective Divide
		transformed.x /= transformed.w;
		transformed.y /= transformed.w;
		transformed.z /= transformed.w;

		// Do Clipping > Image and insert back
		const Vertex_Out out{
			NdcToScreen(transformed),
			mesh.vertices[i].color,
			mesh.vertices[i].uv,
			mesh.worldMatrix.TransformVector(mesh.vertices[i].normal).Normalized(),
			mesh.worldMatrix.TransformVector(mesh.vertices[i].tangent).Normalized(),
			(mesh.worldMatrix.TransformPoint(mesh.vertices[i].position) - m_Camera.origin).Normalized(),
		};

		mesh.verticesOut[i] = out;
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

void Renderer::CycleRotationMode()
{
	m_Rotating = !m_Rotating;
}

void Renderer::CycleShadingMode()
{
	m_ShadingMode = static_cast<ShadingMode>((static_cast<int>(m_ShadingMode) + 1) % (static_cast<int>(ShadingMode::specular) + 1));
}

void Renderer::CycleNormalMode()
{
	m_UsingNormalMap = !m_UsingNormalMap;
}

void Renderer::RenderScreenTri(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2, const Material& mat, float* depthBuffer) const
{
	if (!GeometryUtils::CheckRange(v0.position.x, v0.position.y, v0.position.z, m_Width, m_Height, 1) ||
		!GeometryUtils::CheckRange(v1.position.x, v1.position.y, v1.position.z, m_Width, m_Height, 1) || 
		!GeometryUtils::CheckRange(v2.position.x, v2.position.y, v2.position.z, m_Width, m_Height, 1))
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

				// position.z holds projected depth for all vertices
				// position.w holds view depth for all vertices
				const float viewDepth{
					1.f / 
					(
						(1.f / v0.position.w * res.w0) +
						(1.f / v1.position.w * res.w1) +
						(1.f / v2.position.w * res.w2)
					)
				};

				const float projectedDepth{
					1.f /
					(
						(1.f / v0.position.z * res.w0) +
						(1.f / v1.position.z * res.w1) +
						(1.f / v2.position.z * res.w2)
					)
				};


				// Depth test
				if (depthBuffer[pixelIndex] < viewDepth) continue;

				depthBuffer[pixelIndex] = viewDepth;

				if (m_RenderMode == RenderMode::depth)
				{
					const float remapMin{ 0.995f };
					const float remapMax{ 1.0f };
					const float depthColor{ (Clamp(projectedDepth, remapMin, remapMax) - remapMin) / (remapMax - remapMin) };
					finalColor = ColorRGB{ depthColor,depthColor,depthColor };
				}
				else
				{
#pragma region Interpolation
					const Vector4 interpolatedPosition{
						static_cast<float>(px),
						static_cast<float>(py),
						projectedDepth,
						viewDepth
					};

					const ColorRGB interpolatedColor{
						(
							(v0.color / v0.position.w * res.w0) +
							(v1.color / v1.position.w * res.w1) +
							(v2.color / v2.position.w * res.w2)
						) * viewDepth
					};

					const Vector2 interpolatedUV{
						(
							(v0.uv / v0.position.w * res.w0) +
							(v1.uv / v1.position.w * res.w1) +
							(v2.uv / v2.position.w * res.w2)
						) * viewDepth
					};
					const Vector3 interpolatedNormal{
						(
							(v0.normal / v0.position.w * res.w0) +
							(v1.normal / v1.position.w * res.w1) +
							(v2.normal / v2.position.w * res.w2)
						) * viewDepth
					};
					const Vector3 interpolatedTangent{
						(
							(v0.tangent / v0.position.w * res.w0) +
							(v1.tangent / v1.position.w * res.w1) +
							(v2.tangent / v2.position.w * res.w2)
						) * viewDepth
					};
					const Vector3 interpolatedViewDirection{
						(
							(v0.viewDirection / v0.position.w * res.w0) +
							(v1.viewDirection / v1.position.w * res.w1) +
							(v2.viewDirection / v2.position.w * res.w2)
						) * viewDepth
					};

#pragma endregion
					Vertex_Out interpolatedVertex{
						interpolatedPosition,
						interpolatedColor,
						interpolatedUV,
						interpolatedNormal.Normalized(),
						interpolatedTangent.Normalized(),
						interpolatedViewDirection.Normalized()
					};

					finalColor = Shade(interpolatedVertex, mat);
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

size_t Renderer::AddMaterial(const std::string& diffuse, const std::string& normal, const std::string& specular,
	const std::string& gloss)
{
	Texture* diffuseTexture{ nullptr };
	if (!diffuse.empty()) diffuseTexture = Texture::LoadFromFile(diffuse);

	Texture* normalTexture{ nullptr };
	if (!normal.empty()) normalTexture = Texture::LoadFromFile(normal);

	Texture* specularTexture{ nullptr };
	if (!specular.empty()) specularTexture = Texture::LoadFromFile(specular);

	Texture* glossTexture{ nullptr };
	if (!gloss.empty()) glossTexture = Texture::LoadFromFile(gloss);


	m_Materials.push_back(Material{ diffuseTexture, normalTexture, specularTexture, glossTexture });
	return m_Materials.size() - 1;
}


ColorRGB Renderer::Shade(const Vertex_Out& vertex, const Material& material) const
{
	const Vector3 lightDirection{ .577f, -.577f, .577f };

	// Normal
	const Vector3 binormal{ Vector3::Cross(vertex.normal, vertex.tangent) };
	const Matrix tangentSpaceAxis{ vertex.tangent, binormal, vertex.normal, Vector3::Zero };

	const ColorRGB normalMapSample{ material.pNormal->Sample(vertex.uv) };
	Vector3 normal{ normalMapSample.ToVector3() * 2.f - Vector3::One };
	normal = tangentSpaceAxis.TransformPoint(normal);
	normal.Normalize();

	if (!m_UsingNormalMap) normal = vertex.normal;

	const float observedArea{ Vector3::Dot(normal, -lightDirection) };
	if (observedArea <= 0)
	{
		return colors::Black;
	}

	constexpr float lightIntensity{ 7.f };
	constexpr float shininess{ 25.f };
	const ColorRGB ambient{ .03f, .03f, .03f };

	// Diffuse
	const ColorRGB diffuseMapSample{ material.pDiffuse->Sample(vertex.uv) };
	const ColorRGB diffuse{ BRDF::Lambert(1.f, diffuseMapSample)};

	const ColorRGB specularMapSample{ material.pSpecular->Sample(vertex.uv) };
	const ColorRGB glossinessMapSample{ material.pGloss->Sample(vertex.uv) };
	const ColorRGB glossiness{ glossinessMapSample * shininess };

	const ColorRGB specular{ BRDF::Phong(
		specularMapSample,
		glossiness,
		lightDirection,
		vertex.viewDirection,
		normal
	) };

	switch (m_ShadingMode)
	{
	case ShadingMode::combined:
		return ((colors::White * lightIntensity * diffuse) + ambient + specular) * observedArea;
	case ShadingMode::observedArea:
		return ColorRGB{ observedArea,observedArea,observedArea };
	case ShadingMode::diffuse:
		return colors::White * lightIntensity * diffuse * observedArea;
	case ShadingMode::specular:
		return specular;
	}

	return {};
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
