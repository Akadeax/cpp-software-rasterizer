#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		float cameraRotationSpeed{ 0.02f };
		float cameraTranslationSpeed{ 5.f };

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		// World to Camera transformation
		void CalculateViewMatrix()
		{
			const Matrix rotation{ Matrix::CreateRotation(totalPitch, totalYaw, 0.f) };

			forward = rotation.TransformVector(Vector3::UnitZ);
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			invViewMatrix = Matrix(right, up, forward, origin);
			viewMatrix = invViewMatrix.Inverse();

			// Maybe implement as:
			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W3

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime{ pTimer->GetElapsed() };

			// Keyboard Input
			const uint8_t* pKeyboardState{ SDL_GetKeyboardState(nullptr) };

			// Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState{ SDL_GetRelativeMouseState(&mouseX, &mouseY) };

			const bool leftMouse{ (mouseState & SDL_BUTTON(1)) != 0 };
			const bool rightMouse{ (mouseState & SDL_BUTTON(3)) != 0 };

			// == Translation ==
			// WASD
			if (pKeyboardState[SDL_SCANCODE_W])
			{
				origin += forward * cameraTranslationSpeed * deltaTime;
			}
			else if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= forward * cameraTranslationSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += right * cameraTranslationSpeed * deltaTime;
			}
			else if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin -= right * cameraTranslationSpeed * deltaTime;
			}

			// LMB + MouseY
			if (leftMouse && !rightMouse && mouseY != 0)
			{
				origin -= forward * mouseY * cameraTranslationSpeed * deltaTime;
			}
			// LMB + RMB + MouseY
			if (leftMouse && rightMouse && mouseY != 0)
			{
				origin += Vector3::UnitY * mouseY * cameraTranslationSpeed * deltaTime;
			}

			// Rotation
			if (leftMouse && !rightMouse && mouseX != 0)
			{
				totalYaw += cameraRotationSpeed * mouseX;
			}
			else if (!leftMouse && rightMouse)
			{
				totalYaw += cameraRotationSpeed * mouseX;
				totalPitch -= cameraRotationSpeed * mouseY;
			}

			if (pKeyboardState[SDL_SCANCODE_SPACE])
			{
				totalYaw += cameraRotationSpeed * deltaTime;
			}

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
