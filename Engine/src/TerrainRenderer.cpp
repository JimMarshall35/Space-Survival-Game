#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include "TerrainRenderer.h"
#include "ITerrainPolygonizer.h"
#include "ITerrainOctreeNode.h"
#include "OctreeTypes.h"
#include "TerrainLight.h"
#include "TerrainMaterial.h"
#include "Gizmos.h"

static const char* gTerrainShaderCodeVert =
"#version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"layout(location = 1) in vec3 aNormal;\n"

"out vec3 FragPos;\n"
"out vec3 FragWorldPos;\n"
"out vec3 Normal;\n"
"out vec3 LightPos;\n"
"out vec3 WorldSpaceNormal;\n"

"uniform vec3 lightPos;\n" // we now define the uniform in the vertex shader and pass the 'view space' lightpos to the fragment shader. lightPos is currently in world space.

"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"

"void main()\n"
"{""\n"
    "gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "FragPos = vec3(view * model * vec4(aPos, 1.0));\n"
    "FragWorldPos = vec3(model * vec4(aPos, 1.0));\n"
    "Normal = aNormal;\n"//"mat3(transpose(inverse(view * model))) * aNormal;\n"
    "WorldSpaceNormal = vec3(model * vec4(aNormal, 0.0));\n"

    "LightPos = vec3(view * vec4(lightPos, 1.0)); // Transform world-space light position to view-space light position\n"
"}\n";

static const char* gTerrainShaderCodeFrag =
"#version 330 core\n"
"out vec4 FragColor;\n"

"in vec3 FragPos;\n"
"in vec3 FragWorldPos;\n"
"in vec3 Normal;\n"
"in vec3 LightPos;\n" // extra in variable, since we need the light position in view space we calculate this in the vertex shader
"in vec3 WorldSpaceNormal;\n"

"uniform vec3 lightColor;\n"
"uniform vec3 objectColor;\n"
"uniform float ambientStrength;\n"
"uniform float specularStrength;\n"
"uniform float diffuseStrength;\n"
"uniform float shininess;\n"
//"layout(binding = 0) uniform sampler2D RockTexture;"
//"layout(binding = 1) uniform sampler2D MossTexture;"
//
//"const float GRASS_ROCK_BOUNDARY = -0.9;"
//"const float ROCK_GRASS_FADE = 0.2;"
//"const float TRIPLANAR_BLEND_SHARPNESS = 4.0;"
//
//"vec3 LookupTextureTriplanar(in sampler2D tex) {"
//    "float textureScale = 5.0;"
//    "vec2 yUV = FragWorldPos.xz / textureScale;"
//    "vec2 xUV = FragWorldPos.zy / textureScale;"
//    "vec2 zUV = FragWorldPos.xy / textureScale;"
//
//    "vec3 yDiff = vec3(texture(tex, yUV));"
//    "vec3 xDiff = vec3(texture(tex, xUV));"
//    "vec3 zDiff = vec3(texture(tex, zUV));"
//
//    "vec3 blendWeights = vec3("
//        "pow(abs(WorldSpaceNormal.x), TRIPLANAR_BLEND_SHARPNESS),"
//        "pow(abs(WorldSpaceNormal.y), TRIPLANAR_BLEND_SHARPNESS),"
//        "pow(abs(WorldSpaceNormal.z), TRIPLANAR_BLEND_SHARPNESS)"
//    ");"
//    "blendWeights /= (blendWeights.x + blendWeights.y + blendWeights.z);"
//
//    "return xDiff * blendWeights.x + yDiff * blendWeights.y + zDiff * blendWeights.z;"
//"}"

"void main()\n"
"{\n"
    "// ambient""\n"
    "//float ambientStrength = 0.2;""\n"
    "vec3 ambient = ambientStrength * lightColor;\n"
    //"vec3 grassPixel = LookupTextureTriplanar(MossTexture);"
    //"vec3 rockPixel = LookupTextureTriplanar(RockTexture);"
    "vec3 pixel_colour = objectColor;\n"//(FragWorldPos.y < GRASS_ROCK_BOUNDARY) ? grassPixel : mix(grassPixel, rockPixel, clamp(((FragWorldPos.y - GRASS_ROCK_BOUNDARY) / ROCK_GRASS_FADE), 0.0, 1.0));"

    "// diffuse ""\n"
    "vec3 norm = normalize(Normal);\n"
    "vec3 lightDir = normalize(LightPos - FragPos);\n"
    "float diff = max(dot(norm, lightDir), 0.0);\n"
    "vec3 diffuse = diffuseStrength * diff * lightColor;\n"

    "// specular""\n"
    "//float specularStrength = 0.2;\n"
    "vec3 viewDir = normalize(-FragPos);\n" // the viewer is always at (0,0,0) in view-space, so viewDir is (0,0,0) - Position => -Position
    "vec3 reflectDir = reflect(-lightDir, norm);\n"
    "float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);\n"
    "vec3 specular = specularStrength * spec * lightColor;\n"

    "vec3 result = ( diffuse + specular) * pixel_colour;\n"
    "FragColor = vec4(result, 1.0);\n"
"}\n";

TerrainRenderer::TerrainRenderer(const TerrainRendererConfigData& config)
	:MemoryBudget(config.MemoryBudget),
    //TerrainShader(gTerrainShaderCodeVert, gTerrainShaderCodeFrag),
    FreeingStrategy(config.FreeingStrategy),
    FreeingData(config.FreeingData)
{
    TerrainShader.LoadFromString(gTerrainShaderCodeVert, gTerrainShaderCodeFrag);
}

void TerrainRenderer::UploadNewlyPolygonizedToGPU(PolygonizeWorkerThreadData* data)
{
    size_t allocationSize = (data->OutputtedVertices * sizeof(TerrainVertex)) + (data->OutputtedIndices * sizeof(u32));
    if (CurrentTerrainGPUAllocation + allocationSize > MemoryBudget)
    {
        FreeChunksToFit(allocationSize);
    }
    TerrainChunkMesh& mesh = data->Node->GetTerrainChunkMeshMutable();
    glGenBuffers(2, mesh.Buffers);
    glGenVertexArrays(1, &mesh.VAO);
    mesh.IndiciesToDraw = data->OutputtedIndices;
    u32 VBO = mesh.Buffers[(u32)TerrainChunkMeshBuffer::VBO];
    u32 EBO = mesh.Buffers[(u32)TerrainChunkMeshBuffer::EBO];

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, data->OutputtedVertices * sizeof(TerrainVertex), data->Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, Position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, Normal));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data->OutputtedIndices * sizeof(u32), data->Indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}

void TerrainRenderer::FreeChunksToFit(u32 attemptedAllocation)
{
    switch (FreeingStrategy)
    {
    case GPUMemoryFreeingStrategy::OvershootFixedAmount:
        FreeChunksToFit_UntilFixedMemoryAmount(attemptedAllocation);
        break;
    case GPUMemoryFreeingStrategy::FreeUntilLastRenderedAFixedTimeAgo:
        FreeChunksToFit_UntilFixedTimeAgoRendered(attemptedAllocation);
        break;
    }
    

}

void TerrainRenderer::FreeChunksToFit_UntilFixedMemoryAmount(u32 attemptedAllocation)
{
    std::vector<LastRenderedPair> vec;
    for (const LastRenderedPair& pair : LastRendered)
    {
        vec.push_back(pair);
    }
    std::sort(vec.begin(), vec.end(), [](const LastRenderedPair& a, const LastRenderedPair& b) -> bool
    {
        return a.second.LastRendered > b.second.LastRendered;
    });

    for (const LastRenderedPair& pair : vec)
    {
        ITerrainOctreeNode* node = pair.first;
        const TerrainNodeRendererData& renderData = pair.second;

        const TerrainChunkMesh& mesh = node->GetTerrainChunkMesh();
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(2, mesh.Buffers);

        LastRendered.erase(node);

        CurrentTerrainGPUAllocation -= renderData.SizeOfAllocation;

        if (CurrentTerrainGPUAllocation < (MemoryBudget - attemptedAllocation - std::get<u32>(FreeingData)))
        {
            return;
        }
    }
}

void TerrainRenderer::FreeChunksToFit_UntilFixedTimeAgoRendered(u32 attemptedAllocation)
{
    TerrainTimePoint now = std::chrono::system_clock::now();
    TerrainTimePoint cutoff = now - std::get<TerrainDuration>(FreeingData);
    std::vector<LastRenderedPair> vec;
    for (const LastRenderedPair& pair : LastRendered)
    {
        vec.push_back(pair);
    }
    std::sort(vec.begin(), vec.end(), [](const LastRenderedPair& a, const LastRenderedPair& b) -> bool
    {
        return a.second.LastRendered > b.second.LastRendered;
    });

    for (const LastRenderedPair& pair : vec)
    {
        ITerrainOctreeNode* node = pair.first;
        const TerrainNodeRendererData& renderData = pair.second;

        const TerrainChunkMesh& mesh = node->GetTerrainChunkMesh();
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(2, mesh.Buffers);

        LastRendered.erase(node);

        CurrentTerrainGPUAllocation -= renderData.SizeOfAllocation;

        if (pair.second.LastRendered < cutoff)
        {
            return;
        }
    }
}

void TerrainRenderer::SetTerrainMaterial(const TerrainMaterial& material)
{
    TerrainShader.use();
    TerrainShader.setVec3("objectColor", material.Colour);
    TerrainShader.setFloat("ambientStrength", material.AmbientStrength);
    TerrainShader.setFloat("diffuseStrength", material.DiffuseStrength);

    TerrainShader.setFloat("specularStrength", material.SpecularStrength);
    TerrainShader.setFloat("shininess", material.Shinyness);
}

void TerrainRenderer::SetTerrainLight(const TerrainLight& light)
{
    TerrainShader.use();
    TerrainShader.setVec3("LightPos", light.LightPosition);
    TerrainShader.setVec3("lightColor", light.LightColor);
}

void TerrainRenderer::RenderTerrainNodes(
    const std::vector<ITerrainOctreeNode*>& nodes,
    const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
{
    TerrainTimePoint now = std::chrono::system_clock::now();
    for (ITerrainOctreeNode* node : nodes)
    {
        LastRendered[node].LastRendered = now;
        const TerrainChunkMesh& mesh = node->GetTerrainChunkMesh();

        u32 ebo = mesh.GetEBO();
        TerrainShader.use();
        TerrainShader.setMat4("model", model);
        TerrainShader.setMat4("view", view);
        TerrainShader.setMat4("projection", projection);
        glBindVertexArray(mesh.VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.GetEBO());
        glDrawElements(GL_TRIANGLES, mesh.IndiciesToDraw, GL_UNSIGNED_INT, 0);
        const auto& bl = node->GetBottomLeftCorner();
        const auto size = node->GetSizeInVoxels();
        glm::vec3 parentCenter = {
            bl.x + (size / 2),
            bl.y + (size / 2),
            bl.z + (size / 2)
        };
        Gizmos::AddBox(
            parentCenter,
            { size, size, size }, // dimensions
            false,
            { 1.0,1.0,1.0,1.0 }
        );
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
