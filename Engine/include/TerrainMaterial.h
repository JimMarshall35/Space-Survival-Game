#include <glm.hpp>

struct APP_API TerrainMaterial
{
	glm::vec3 Colour;
	float AmbientStrength;
	float SpecularStrength;
	float DiffuseStrength;
	float Shinyness;
};