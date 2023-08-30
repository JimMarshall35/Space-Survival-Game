#ifndef __GIZMOS_H_
#define __GIZMOS_H_

#include <glm.hpp>

class Gizmos
{
public:

	static void		Create(unsigned int maxLines = 16384*4, unsigned int maxTris = 16384*4);
	static void		Destroy();

	// removes all Gizmos
	static void		Clear();

	// draws current Gizmo buffers, either using a combined (projection * view) matrix, or separate matrices
	static void		Draw(const glm::mat4& projectionView);
	static void		Draw(const glm::mat4& view, const glm::mat4& projection);

	// Adds a single debug line
	static void		AddLine(const glm::vec3& rv0, const glm::vec3& rv1,
		const glm::vec4& colour);

	// Adds a single debug line
	static void		AddLine(const glm::vec3& rv0, const glm::vec3& rv1,
		const glm::vec4& colour0, const glm::vec4& colour1);

	// Adds a triangle.
	static void		AddTri(const glm::vec3& rv0, const glm::vec3& rv1, const glm::vec3& rv2,
		const glm::vec4& colour);

	// Adds 3 unit-length lines (red,green,blue) representing the 3 axis of a transform, 
	// at the transform's translation. Optional scale available.
	static void		AddTransform(const glm::mat4& transform, float fScale = 1.0f);

	// Adds an Axis-Aligned Bounding-Box with optional transform for rotation.
	static void		AddBox(const glm::vec3& center, const glm::vec3& dimensions,
		const bool& filled, const glm::vec4& fillColour = glm::vec4(1.f, 1.f, 1.f, 1.f),
		const glm::mat4& transform = glm::mat4(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f));

	// Adds a cylinder aligned to the Y-axis with optional transform for rotation.
	static void		AddCylinder(const glm::vec3& center, float radius,
		float fHalfLength, unsigned int segments,
		const bool& filled, const glm::vec4& fillColour = glm::vec4(1.f, 1.f, 1.f, 1.f),
		const glm::mat4& transform = glm::mat4(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f));

	static void		AddCircle(const glm::vec3& center, float radius, unsigned int segments,
		const bool& filled, const glm::vec4& Colour = glm::vec4(1.f, 1.f, 1.f, 1.f),
		const glm::mat4& transform = glm::mat4(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f));

	// Adds a Sphere at a given position, with a given number of rows, and columns, radius and a max and min long and latitude
	static void		AddSphere(const glm::vec3& center, int rows, int columns, float radius, const glm::vec4& fillColour,
		const glm::mat4* transform = nullptr, float longMin = 0.f, float longMax = 360,
		float latMin = -90, float latMax = 90);

private:

	Gizmos(unsigned int maxLines, unsigned int maxTris);
	~Gizmos();

	void TestShaderStatus(const unsigned int& uiShaderId, unsigned int uiStatus, int& iSuccessFlag, const char* shaderType);
	struct GizmoVertex
	{
		glm::vec4 position;
		glm::vec4 colour;
	};

	struct GizmoLine
	{
		GizmoVertex v0;
		GizmoVertex v1;
	};

	struct GizmoTri
	{
		GizmoVertex v0;
		GizmoVertex v1;
		GizmoVertex v2;
	};

	unsigned int	m_programID;
	unsigned int	m_vertexShader;
	unsigned int	m_fragmentShader;

	// line data
	unsigned int	m_maxLines;
	unsigned int	m_lineCount;
	GizmoLine* m_lines;

	unsigned int	m_lineVAO;
	unsigned int 	m_lineVBO;

	// triangle data
	unsigned int	m_maxTris;
	unsigned int	m_triCount;
	GizmoTri* m_tris;

	unsigned int	m_triVAO;
	unsigned int 	m_triVBO;

	static Gizmos* sm_singleton;
};

inline void Gizmos::Draw(const glm::mat4& view, const glm::mat4& projection)
{
	Draw(projection * view);
}

#endif // __GIZMOS_H_
