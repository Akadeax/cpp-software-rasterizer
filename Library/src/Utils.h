#pragma once
#include <cassert>
#include <fstream>
#include "Maths.h"
#include "DataTypes.h"

//#define DISABLE_OBJ

namespace dae
{
	namespace GeometryUtils
	{
		struct TriResult
		{
			bool hit{ false };
			float w0{ 1 }, w1{ 1 }, w2{ 1 };
		};

		TriResult HitTest_ScreenTriangle(Vector2 screenPos, const Vertex_Out& v0,  const Vertex_Out& v1, const Vertex_Out& v2)
		{
			const float triArea{ std::abs(Vector2::Cross(v1.position - v0.position, v2.position - v0.position)) };

			const float w0{ std::abs(Vector2::Cross(screenPos - v1.position.ToVector2(), (v2.position - v1.position).ToVector2())) };
			const float w1{ std::abs(Vector2::Cross(screenPos - v2.position.ToVector2(), (v0.position - v2.position).ToVector2())) };
			const float w2{ std::abs(Vector2::Cross(screenPos - v0.position.ToVector2(), (v1.position - v0.position).ToVector2())) };

			const float totalArea{ w0 + w1 + w2 };
			return { AreEqual(totalArea, triArea, 1.f), w0 / totalArea, w1 / totalArea, w2 / totalArea };
		}

		struct ScreenBoundingBox
		{
			Vector2i topLeft{};
			Vector2i bottomRight{};
		};

		inline ScreenBoundingBox GetScreenBoundingBox(Vector3 v0, Vector3 v1, Vector3 v2, int width, int height)
		{
			// smallest x and y of all vertices
			Vector2i boundingBoxTopLeft{ Vector2i(
				static_cast<int>(std::min(v0.x, std::min(v1.x, v2.x))),
				static_cast<int>(std::min(v0.y, std::min(v1.y, v2.y)))
			) };

			// largest x and y of all vertices
			Vector2i boundingBoxBottomRight{ Vector2i(
				static_cast<int>(std::max(v0.x, std::max(v1.x, v2.x))),
				static_cast<int>(std::max(v0.y, std::max(v1.y, v2.y)))
			) };

			boundingBoxTopLeft.x = std::max(boundingBoxTopLeft.x, 0);
			boundingBoxTopLeft.x = std::min(boundingBoxTopLeft.x, width - 1);
			boundingBoxTopLeft.y = std::max(boundingBoxTopLeft.y, 0);
			boundingBoxTopLeft.y = std::min(boundingBoxTopLeft.y, height - 1);

			// Offset by 1 in all directions to make sure there's no gaps
			boundingBoxTopLeft.x -= 1;
			boundingBoxTopLeft.y -= 1;
			boundingBoxBottomRight.x += 1;
			boundingBoxBottomRight.y += 1;

			boundingBoxTopLeft.x = std::max(boundingBoxTopLeft.x, 0);
			boundingBoxBottomRight.x = std::min(boundingBoxBottomRight.x, width - 1);
			boundingBoxTopLeft.y = std::max(boundingBoxTopLeft.y, 0);
			boundingBoxBottomRight.y = std::min(boundingBoxBottomRight.y, height - 1);

			return { boundingBoxTopLeft, boundingBoxBottomRight };
		}

		inline bool CheckRange(float x, float y, float z, int xMax, int yMax, float zMax)
		{
			return x >= 0 && x <= xMax && y >= 0 && y <= yMax && z >= 0 && z <= zMax;
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, bool flipAxisAndWinding = true)
		{
#ifdef DISABLE_OBJ

			//TODO: Enable the code below after uncommenting all the vertex attributes of DataTypes::Vertex
			// >> Comment/Remove '#define DISABLE_OBJ'
			assert(false && "OBJ PARSER not enabled! Check the comments in Utils::ParseOBJ");

#else

			std::ifstream file(filename);
			if (!file)
				return false;

			std::vector<Vector3> positions{};
			std::vector<Vector3> normals{};
			std::vector<Vector2> UVs{};

			vertices.clear();
			indices.clear();

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;

					positions.emplace_back(x, y, z);
				}
				else if (sCommand == "vt")
				{
					// Vertex TexCoord
					float u, v;
					file >> u >> v;
					UVs.emplace_back(u, 1 - v);
				}
				else if (sCommand == "vn")
				{
					// Vertex Normal
					float x, y, z;
					file >> x >> y >> z;

					normals.emplace_back(x, y, z);
				}
				else if (sCommand == "f")
				{
					//if a face is read:
					//construct the 3 vertices, add them to the vertex array
					//add three indices to the index array
					//add the material index as attibute to the attribute array
					//
					// Faces or triangles
					Vertex vertex{};
					size_t iPosition, iTexCoord, iNormal;

					uint32_t tempIndices[3];
					for (size_t iFace = 0; iFace < 3; iFace++)
					{
						// OBJ format uses 1-based arrays
						file >> iPosition;
						vertex.position = positions[iPosition - 1];

						if ('/' == file.peek())//is next in buffer ==  '/' ?
						{
							file.ignore();//read and ignore one element ('/')

							if ('/' != file.peek())
							{
								// Optional texture coordinate
								file >> iTexCoord;
								vertex.uv = UVs[iTexCoord - 1];
							}

							if ('/' == file.peek())
							{
								file.ignore();

								// Optional vertex normal
								file >> iNormal;
								vertex.normal = normals[iNormal - 1];
							}
						}

						vertices.push_back(vertex);
						tempIndices[iFace] = uint32_t(vertices.size()) - 1;
						//indices.push_back(uint32_t(vertices.size()) - 1);
					}

					indices.push_back(tempIndices[0]);
					if (flipAxisAndWinding) 
					{
						indices.push_back(tempIndices[2]);
						indices.push_back(tempIndices[1]);
					}
					else
					{
						indices.push_back(tempIndices[1]);
						indices.push_back(tempIndices[2]);
					}
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');
			}

			//Cheap Tangent Calculations
			for (uint32_t i = 0; i < indices.size(); i += 3)
			{
				uint32_t index0 = indices[i];
				uint32_t index1 = indices[size_t(i) + 1];
				uint32_t index2 = indices[size_t(i) + 2];

				const Vector3& p0 = vertices[index0].position;
				const Vector3& p1 = vertices[index1].position;
				const Vector3& p2 = vertices[index2].position;
				const Vector2& uv0 = vertices[index0].uv;
				const Vector2& uv1 = vertices[index1].uv;
				const Vector2& uv2 = vertices[index2].uv;

				const Vector3 edge0 = p1 - p0;
				const Vector3 edge1 = p2 - p0;
				const Vector2 diffX = Vector2(uv1.x - uv0.x, uv2.x - uv0.x);
				const Vector2 diffY = Vector2(uv1.y - uv0.y, uv2.y - uv0.y);
				float r = 1.f / Vector2::Cross(diffX, diffY);

				Vector3 tangent = (edge0 * diffY.y - edge1 * diffY.x) * r;
				vertices[index0].tangent += tangent;
				vertices[index1].tangent += tangent;
				vertices[index2].tangent += tangent;
			}

			//Fix the tangents per vertex now because we accumulated
			for (auto& v : vertices)
			{
				v.tangent = Vector3::Reject(v.tangent, v.normal).Normalized();

				if(flipAxisAndWinding)
				{
					v.position.z *= -1.f;
					v.normal.z *= -1.f;
					v.tangent.z *= -1.f;
				}

			}

			return true;
#endif
		}
#pragma warning(pop)
	}
}