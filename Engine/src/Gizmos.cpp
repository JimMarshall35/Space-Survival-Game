#include "Gizmos.h"
#include <sstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ext.hpp>
#include <iostream>

Gizmos* Gizmos::sm_singleton = nullptr;

Gizmos::Gizmos(unsigned int maxLines, unsigned int maxTris)
	: m_maxLines(maxLines),
	m_maxTris(maxTris),
	m_lineCount(0),
	m_triCount(0),
	m_lines(new GizmoLine[maxLines]),
	m_tris(new GizmoTri[maxTris])
{
	int success = GL_FALSE;
	//\==============================================================================================
	//\ Create our Vertex Shader from a char array
	const char* vsSource = "#version 150\n \
							in vec4 Position; \
							in vec4 Colour; \
							out vec4 vColour; \
							uniform mat4 ProjectionView; \
							void main() { vColour = Colour; gl_Position = ProjectionView * Position; }";

	m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(m_vertexShader, 1, (const char**)&vsSource, 0);
	glCompileShader(m_vertexShader);
	//Test our shader for succesful compiliation
	TestShaderStatus(m_vertexShader, GL_COMPILE_STATUS, success, "Vertex");

	const char* fsSource = "#version 150\n \
							in vec4 vColour; \
							out vec4 FragColor; \
							void main()	{ FragColor = vColour; }";

	m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(m_fragmentShader, 1, (const char**)&fsSource, 0);
	glCompileShader(m_fragmentShader);
	TestShaderStatus(m_fragmentShader, GL_COMPILE_STATUS, success, "Fragment");


	m_programID = glCreateProgram();
	glAttachShader(m_programID, m_vertexShader);
	glAttachShader(m_programID, m_fragmentShader);
	glBindAttribLocation(m_programID, 0, "Position");
	glBindAttribLocation(m_programID, 1, "Colour");
	glLinkProgram(m_programID);
	TestShaderStatus(m_fragmentShader, GL_LINK_STATUS, success, nullptr);

	// create VBOs
	glGenBuffers(1, &m_lineVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
	glBufferData(GL_ARRAY_BUFFER, m_maxLines * sizeof(GizmoLine), m_lines, GL_DYNAMIC_DRAW);


	glGenBuffers(1, &m_triVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_triVBO);
	glBufferData(GL_ARRAY_BUFFER, m_maxTris * sizeof(GizmoTri), m_tris, GL_DYNAMIC_DRAW);


	glGenVertexArrays(1, &m_lineVAO);
	glBindVertexArray(m_lineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GizmoVertex), 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, sizeof(GizmoVertex), ((char*)0) + 16);

	glGenVertexArrays(1, &m_triVAO);
	glBindVertexArray(m_triVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_triVBO);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GizmoVertex), 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, sizeof(GizmoVertex), ((char*)0) + 16);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Gizmos::~Gizmos()
{
	delete[] m_lines;
	delete[] m_tris;
	glDeleteBuffers(1, &m_lineVBO);
	glDeleteBuffers(1, &m_triVBO);
	glDeleteVertexArrays(1, &m_lineVAO);
	glDeleteVertexArrays(1, &m_triVAO);
	glDeleteProgram(m_programID);
	glDeleteShader(m_fragmentShader);
	glDeleteShader(m_vertexShader);
}

void Gizmos::Create(unsigned int maxLines /* = 16384 */, unsigned int maxTris /* = 16384 */)
{
	if (sm_singleton == nullptr)
		sm_singleton = new Gizmos(maxLines, maxTris);
}

void Gizmos::Destroy()
{
	delete sm_singleton;
	sm_singleton = nullptr;
}

void Gizmos::Clear()
{
	sm_singleton->m_lineCount = 0;
	sm_singleton->m_triCount = 0;
}

// Adds 3 unit-length lines (red,green,blue) representing the 3 axis of a transform, 
// at the transform's translation. Optional scale available.
void Gizmos::AddTransform(const glm::mat4& transform, float fScale /* = 1.0f */)
{
	glm::vec4 vXAxis = transform[3] + transform[0] * fScale;
	glm::vec4 vYAxis = transform[3] + transform[1] * fScale;
	glm::vec4 vZAxis = transform[3] + transform[2] * fScale;

	glm::vec4 vRed(1, 0, 0, 1);
	glm::vec4 vGreen(0, 1, 0, 1);
	glm::vec4 vBlue(0, 0, 1, 1);
	
	AddLine(transform[3].xyz, vXAxis.xyz, vRed, vRed);
	AddLine(transform[3].xyz, vYAxis.xyz, vGreen, vGreen);
	AddLine(transform[3].xyz, vZAxis.xyz, vBlue, vBlue);
}



void Gizmos::AddBox(const glm::vec3& center,
	const glm::vec3& vDimensions,
	const bool& filled,
	const glm::vec4& fillColour /*= glm::vec4( 1.f, 1.f, 1.f, 1.f)*/,
	const glm::mat4& transform /* = mat4::identity */)
{

	glm::vec3 vX(vDimensions.x/2.0f, 0, 0);
	glm::vec3 vY(0, vDimensions.y/2.0f, 0);
	glm::vec3 vZ(0, 0, vDimensions.z/2.0f);

	vX = (transform * glm::vec4(vX, 0)).xyz;
	vY = (transform * glm::vec4(vY, 0)).xyz;
	vZ = (transform * glm::vec4(vZ, 0)).xyz;


	glm::vec3 vVerts[8];

	// top verts
	vVerts[0] = center - vX - vZ - vY;
	vVerts[1] = center - vX + vZ - vY;
	vVerts[2] = center + vX + vZ - vY;
	vVerts[3] = center + vX - vZ - vY;

	// bottom verts
	vVerts[4] = center - vX - vZ + vY;
	vVerts[5] = center - vX + vZ + vY;
	vVerts[6] = center + vX + vZ + vY;
	vVerts[7] = center + vX - vZ + vY;

	glm::vec4 vWhite(1, 1, 1, 1);

	AddLine(vVerts[0], vVerts[1], fillColour, fillColour);
	AddLine(vVerts[1], vVerts[2], fillColour, fillColour);
	AddLine(vVerts[2], vVerts[3], fillColour, fillColour);
	AddLine(vVerts[3], vVerts[0], fillColour, fillColour);

	AddLine(vVerts[4], vVerts[5], fillColour, fillColour);
	AddLine(vVerts[5], vVerts[6], fillColour, fillColour);
	AddLine(vVerts[6], vVerts[7], fillColour, fillColour);
	AddLine(vVerts[7], vVerts[4], fillColour, fillColour);

	AddLine(vVerts[0], vVerts[4], fillColour, fillColour);
	AddLine(vVerts[1], vVerts[5], fillColour, fillColour);
	AddLine(vVerts[2], vVerts[6], fillColour, fillColour);
	AddLine(vVerts[3], vVerts[7], fillColour, fillColour);


	if (filled)
	{
		// top
		AddTri(vVerts[2], vVerts[1], vVerts[0], fillColour);
		AddTri(vVerts[3], vVerts[2], vVerts[0], fillColour);

		// bottom
		AddTri(vVerts[5], vVerts[6], vVerts[4], fillColour);
		AddTri(vVerts[6], vVerts[7], vVerts[4], fillColour);

		// front
		AddTri(vVerts[4], vVerts[3], vVerts[0], fillColour);
		AddTri(vVerts[7], vVerts[3], vVerts[4], fillColour);

		// back
		AddTri(vVerts[1], vVerts[2], vVerts[5], fillColour);
		AddTri(vVerts[2], vVerts[6], vVerts[5], fillColour);

		// left
		AddTri(vVerts[0], vVerts[1], vVerts[4], fillColour);
		AddTri(vVerts[1], vVerts[5], vVerts[4], fillColour);

		// right
		AddTri(vVerts[2], vVerts[3], vVerts[7], fillColour);
		AddTri(vVerts[6], vVerts[2], vVerts[7], fillColour);
	}
}

void Gizmos::AddCylinder(const glm::vec3& center, float radius,
	float fHalfLength, unsigned int segments,
	const bool& filled, const glm::vec4& fillColour /*= glm::vec4(1.f, 1.f, 1.f, 1.f)*/,
	const glm::mat4& transform /* = mat4::identity */)
{
	glm::vec4 vWhite(1, 1, 1, 1);

	float fSegmentSize = (2 * glm::pi<float>()) / segments;

	for (unsigned int i = 0; i < segments; ++i)
	{
		glm::vec3 v0top(0, fHalfLength, 0);
		glm::vec3 v1top(sinf(i * fSegmentSize) * radius, fHalfLength, cosf(i * fSegmentSize) * radius);
		glm::vec3 v2top(sinf((i + 1) * fSegmentSize) * radius, fHalfLength, cosf((i + 1) * fSegmentSize) * radius);
		glm::vec3 v0bottom(0, -fHalfLength, 0);
		glm::vec3 v1bottom(sinf(i * fSegmentSize) * radius, -fHalfLength, cosf(i * fSegmentSize) * radius);
		glm::vec3 v2bottom(sinf((i + 1) * fSegmentSize) * radius, -fHalfLength, cosf((i + 1) * fSegmentSize) * radius);

		v0top = (transform * glm::vec4(v0top, 0)).xyz;
		v1top = (transform * glm::vec4(v1top, 0)).xyz;
		v2top = (transform * glm::vec4(v2top, 0)).xyz;
		v0bottom = (transform * glm::vec4(v0bottom, 0)).xyz;
		v1bottom = (transform * glm::vec4(v1bottom, 0)).xyz;
		v2bottom = (transform * glm::vec4(v2bottom, 0)).xyz;


		// triangles
		if (filled)
		{
			AddTri(center + v0top, center + v1top, center + v2top, fillColour);
			AddTri(center + v0bottom, center + v2bottom, center + v1bottom, fillColour);
			AddTri(center + v2top, center + v1top, center + v1bottom, fillColour);
			AddTri(center + v1bottom, center + v2bottom, center + v2top, fillColour);
		}
		// lines
		AddLine(center + v1top, center + v2top, vWhite, vWhite);
		AddLine(center + v1top, center + v1bottom, vWhite, vWhite);
		AddLine(center + v1bottom, center + v2bottom, vWhite, vWhite);
	}
}

void Gizmos::AddCircle(const glm::vec3& center, float radius, unsigned int segments,
	const bool& filled, const glm::vec4& Colour /*= glm::vec4(1.f, 1.f, 1.f, 1.f*/,
	const glm::mat4& transform /* = mat4::identity */)
{
	//calculate the inner angle for each segment
	float fAngle = (2 * glm::pi<float>()) / segments;
	//We can start our first edge vector at (0,0,radius) as sin(0) = 0, cos(0) = 1
	glm::vec4 v3Edge1(0, 0, radius, 0);
	for (unsigned int i = 0; i < segments; ++i)
	{

		glm::vec4 v3Edge2(sinf((i + 1) * fAngle) * radius, 0, cosf((i + 1) * fAngle) * radius, 0);

		v3Edge1 = transform * v3Edge1;
		v3Edge2 = transform * v3Edge2;

		if (filled)
		{
			AddTri(center, v3Edge1.xyz, center + v3Edge2.xyz, Colour);
			AddTri(center + v3Edge2.xyz, center + v3Edge1.xyz, center, Colour);
		}
		else
		{
			// line
			AddLine(center + v3Edge1.xyz, center + v3Edge2.xyz, Colour);
		}
		v3Edge1 = v3Edge2;
	}
}

void Gizmos::AddSphere(const glm::vec3& center, int rows, int columns, float radius, const glm::vec4& fillColour,
	const glm::mat4* transform /*= nullptr*/, float longMin /*= 0.f*/, float longMax /*= 360*/,
	float latMin /*= -90*/, float latMax /*= 90*/)
{
	//Invert these first as the multiply is slightly quicker
	float invColumns = 1.0f / float(columns);
	float invRows = 1.0f / float(rows);

	float DEG2RAD = glm::pi<float>() / 180;

	//Lets put everything in radians first
	float latitiudinalRange = (latMax - latMin) * DEG2RAD;
	float longitudinalRange = (longMax - longMin) * DEG2RAD;
	// for each row of the mesh
	glm::vec3* v4Array = new glm::vec3[rows * columns + columns];

	for (int row = 0; row <= rows; ++row)
	{
		// y ordinates this may be a little confusing but here we are navigating around the xAxis in GL
		float ratioAroundXAxis = float(row) * invRows;
		float radiansAboutXAxis = ratioAroundXAxis * latitiudinalRange + (latMin * DEG2RAD);
		float y = radius * sin(radiansAboutXAxis);
		float z = radius * cos(radiansAboutXAxis);

		for (int col = 0; col <= columns; ++col)
		{
			float ratioAroundYAxis = float(col) * invColumns;
			float theta = ratioAroundYAxis * longitudinalRange + (longMin * DEG2RAD);
			glm::vec3 v4Point(-z * sinf(theta), y, -z * cosf(theta));

			if (transform != nullptr)
			{
				v4Point = (*transform * glm::vec4(v4Point, 0)).xyz;
			}

			int index = row * columns + (col % columns);
			v4Array[index] = v4Point;
		}
	}

	for (int face = 0; face < (rows) * (columns); ++face)
	{
		int iNextFace = face + 1;

		if (iNextFace % columns == 0)
		{
			iNextFace = iNextFace - (columns);
		}

		AddLine(center + v4Array[face], center + v4Array[face + columns], glm::vec4(1.f, 1.f, 1.f, 1.f), glm::vec4(1.f, 1.f, 1.f, 1.f));

		if (face % columns == 0 && longitudinalRange < (glm::pi<float>() * 2))
		{
			continue;
		}
		AddLine(center + v4Array[iNextFace + columns], center + v4Array[face + columns], glm::vec4(1.f, 1.f, 1.f, 1.f), glm::vec4(1.f, 1.f, 1.f, 1.f));

		AddTri(center + v4Array[iNextFace + columns], center + v4Array[face], center + v4Array[iNextFace], fillColour);
		AddTri(center + v4Array[iNextFace + columns], center + v4Array[face + columns], center + v4Array[face], fillColour);
	}

	delete[] v4Array;
}


void Gizmos::AddLine(const glm::vec3& rv0, const glm::vec3& rv1, const glm::vec4& colour)
{
	AddLine(rv0, rv1, colour, colour);
}

void Gizmos::AddLine(const glm::vec3& rv0, const glm::vec3& rv1, const glm::vec4& colour0, const glm::vec4& colour1)
{
	if (sm_singleton != nullptr)
	{
		if (sm_singleton->m_lineCount < sm_singleton->m_maxLines)
		{
			sm_singleton->m_lines[sm_singleton->m_lineCount].v0.position = glm::vec4(rv0, 1);
			sm_singleton->m_lines[sm_singleton->m_lineCount].v0.colour = colour0;
			sm_singleton->m_lines[sm_singleton->m_lineCount].v1.position = glm::vec4(rv1, 1);
			sm_singleton->m_lines[sm_singleton->m_lineCount].v1.colour = colour1;

			sm_singleton->m_lineCount++;
		}
		else
		{
			std::cout << "m_maxLines exceeded. m_maxLines: " << sm_singleton->m_maxLines << " m_lineCount: "<< sm_singleton->m_lineCount <<"\n";
		}
	}
	
}

void Gizmos::AddTri(const glm::vec3& rv0, const glm::vec3& rv1, const glm::vec3& rv2, const glm::vec4& colour)
{
	if (sm_singleton != nullptr)
	{
		if (sm_singleton->m_triCount < sm_singleton->m_maxTris)
		{
			sm_singleton->m_tris[sm_singleton->m_triCount].v0.position = glm::vec4(rv0, 1);
			sm_singleton->m_tris[sm_singleton->m_triCount].v1.position = glm::vec4(rv1, 1);
			sm_singleton->m_tris[sm_singleton->m_triCount].v2.position = glm::vec4(rv2, 1);
			sm_singleton->m_tris[sm_singleton->m_triCount].v0.colour = colour;
			sm_singleton->m_tris[sm_singleton->m_triCount].v1.colour = colour;
			sm_singleton->m_tris[sm_singleton->m_triCount].v2.colour = colour;

			sm_singleton->m_triCount++;
		}
	}
}

void Gizmos::Draw(const glm::mat4& projectionView)
{
	if (sm_singleton != nullptr &&
		(sm_singleton->m_lineCount > 0 || sm_singleton->m_triCount > 0))
	{
		glUseProgram(sm_singleton->m_programID);

		unsigned int projectionViewUniform = glGetUniformLocation(sm_singleton->m_programID, "ProjectionView");
		glUniformMatrix4fv(projectionViewUniform, 1, false, glm::value_ptr(projectionView));

		if (sm_singleton->m_lineCount > 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, sm_singleton->m_lineVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sm_singleton->m_lineCount * sizeof(GizmoLine), sm_singleton->m_lines);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(sm_singleton->m_lineVAO);
			glDrawArrays(GL_LINES, 0, sm_singleton->m_lineCount * 2);
		}

		if (sm_singleton->m_triCount > 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, sm_singleton->m_triVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sm_singleton->m_triCount * sizeof(GizmoTri), sm_singleton->m_tris);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(sm_singleton->m_triVAO);
			glDrawArrays(GL_TRIANGLES, 0, sm_singleton->m_triCount * 3);
		}

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glUseProgram(0);

	}
}

void Gizmos::TestShaderStatus(const unsigned int& uiShaderId, unsigned int uiStatus, int& iSuccessFlag, const char* shaderType)
{
	if (uiStatus == GL_LINK_STATUS)
	{
		glGetProgramiv(uiShaderId, uiStatus, &iSuccessFlag);
	}
	else
	{
		glGetShaderiv(uiShaderId, uiStatus, &iSuccessFlag);
	}

	if (iSuccessFlag == GL_FALSE)
	{
		int infoLogLength = 0;
		glGetShaderiv(uiShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar* log = (GLchar*)malloc(infoLogLength);
		glGetProgramInfoLog(uiShaderId, infoLogLength, &infoLogLength, log);
		std::ostringstream ssError;
		if (uiStatus == GL_LINK_STATUS)
		{
			ssError << "Error: Failed to  link shader!\n" << log;
		}
		else
		{
			ssError << "Error: Failed to compile " << shaderType << "shader!\n" << log;
		}
		free(log);
	}
}