#include "TerrainApplication.h"

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>
#include <cmath>
#include <iostream>
#include <vector>





// Helper structures. Declared here only for this exercise
struct Vector2
{
    Vector2() : Vector2(0.f, 0.f) {}
    Vector2(float x, float y) : x(x), y(y) {}
    float x, y;
};

struct Vector3
{
    Vector3() : Vector3(0.f,0.f,0.f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    float x, y, z;

    Vector3 Normalize() const
    {
        float length = std::sqrt(1 + x * x + y * y + z * z);
        return Vector3(x / length, y / length, z / length);
    }
};

struct Vertex {
    Vector3 position;
    Vector2 texCoords;
    Vector3 color;
    Vector3 normal;
};


TerrainApplication::TerrainApplication()
    : Application(1024, 1024, "Terrain demo"), m_gridX(128), m_gridY(128), m_shaderProgram(0)
{
}

void TerrainApplication::Initialize()
{
    Application::Initialize();

    // Build shaders and store in m_shaderProgram
    BuildShaders();

    glEnable(GL_DEPTH_TEST);

    std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;



    vertices.reserve((m_gridX+1) * (m_gridY+1));
    indices.reserve((m_gridX+1) * (m_gridY+1) * 6);

    for (unsigned int j = 0u; j < m_gridY + 1u; j++) {
        for (unsigned int i = 0u; i < m_gridX + 1u; i++) {
            Vector3 position;
            Vector2 texCoord;
            Vector3 color;

            float x = i * (1.0f / m_gridX) - 0.5f;
            float y = j * (1.0 / m_gridY) - 0.5f;
            float z = 0.0f;
            float octaves = 6.0f;
            float lacunarity = 2.3f;
            float gain = 0.5f;
            z = stb_perlin_fbm_noise3(x, y, z, lacunarity, gain, octaves);

            position = Vector3(x, y, z);
            texCoord = Vector2(static_cast<float>(i), static_cast<float>(j));

            Vector3 water = Vector3(0.0f, 0.2f, 1.0f);
            Vector3 soil = Vector3(0.8f, 0.5f, 0.4f);
            Vector3 rock = Vector3(0.5f, 0.5f, 0.5f);
            Vector3 snow = Vector3(1.0f, 1.0f, 1.0f);

            if (z > 0.5f) {
                color = snow;
            }
            else if (z > 0.0f) {
                color = rock;
            }
            else if (z > -0.5f) {
                color = soil;
            }
            else {
                color = water;
            }

            vertices.emplace_back(position, texCoord, color, Vector3(0.f,0.f,0.f));
        }
    }

    for (unsigned int j = 1u; j < m_gridY; j++) {
        for (unsigned int i = 1u; i < m_gridX; i++) {
            unsigned int ihere = j * m_gridX + i;
            unsigned int iup = ihere + m_gridX;
            unsigned int idown = ihere - m_gridX;
            unsigned int ileft = ihere - 1u;
            unsigned int iright = ihere + 1u;

            auto here = vertices[ihere];
            auto up = vertices[iup];
            auto down = vertices[idown];
            auto left = vertices[ileft];
            auto right = vertices[iright];


            float dx = (right.position.z - left.position.z) / (right.position.x - left.position.x);
            float dy = (up.position.z - down.position.z) / (up.position.x - down.position.x);

            vertices[ihere].normal = Vector3(dx, dy, 1.0f).Normalize();
           
        }
    }

	for (unsigned int j = 0u; j < m_gridY; j++) {
		for (unsigned int i = 0u; i < m_gridX; i++) {

            unsigned int botLeft = i + ((m_gridX+1u) * j);
            unsigned int botRight = botLeft + 1u;
            unsigned int topRight = botRight + (m_gridX+1u);
            unsigned int topLeft = botLeft + (m_gridX+1u);

            indices.emplace_back(botLeft);
            indices.emplace_back(topLeft);
            indices.emplace_back(topRight);

            indices.emplace_back(botLeft);
            indices.emplace_back(topRight);
            indices.emplace_back(botRight);
        }
    }

    vao.Bind();
    vbo.Bind();


    VertexAttribute position(Data::Type::Float, 3);
    VertexAttribute textureCoordinate(Data::Type::Float, 2);
    VertexAttribute color(Data::Type::Float, 3);
    VertexAttribute normal(Data::Type::Float, 3);

    vbo.AllocateData<Vertex>(vertices);

    ebo.Bind();
    ebo.AllocateData<unsigned int>(indices);

    vao.SetAttribute(0, position, 0, sizeof(Vertex));
    vao.SetAttribute(1, textureCoordinate, position.GetSize(), sizeof(Vertex));
    vao.SetAttribute(2, color, position.GetSize() + textureCoordinate.GetSize(), sizeof(Vertex));
    vao.SetAttribute(3, normal, position.GetSize() + textureCoordinate.GetSize() + color.GetSize(), sizeof(Vertex));

    vao.Unbind();
    vbo.Unbind();
    ebo.Unbind();
}

void TerrainApplication::Update()
{
    Application::Update();

    UpdateOutputMode();
}

void TerrainApplication::Render()
{
    Application::Render();

    // Clear color and depth
    GetDevice().Clear(true, Color(0.0f, 0.0f, 0.0f, 1.0f), true, 1.0f);

    // Set shader to be used
    glUseProgram(m_shaderProgram);
    vao.Bind();
    ebo.Bind();

    // Enable wireframes
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Draw indices
    glDrawElements(GL_TRIANGLES, (m_gridX+1)*(m_gridY+1)*6, GL_UNSIGNED_INT, 0);
}

void TerrainApplication::Cleanup()
{
    Application::Cleanup();
}

void TerrainApplication::BuildShaders()
{
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "layout (location = 2) in vec3 aColor;\n"
        "layout (location = 3) in vec3 aNormal;\n"
        "uniform mat4 Matrix = mat4(1);\n"
        "out vec2 texCoord;\n"
        "out vec3 color;\n"
        "out vec3 normal;\n"
        "void main()\n"
        "{\n"
        "   texCoord = aTexCoord;\n"
        "   color = aColor;\n"
        "   normal = aNormal;\n"
        "   gl_Position = Matrix * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "uniform int Mode = 0;\n"
        "in vec2 texCoord;\n"
        "in vec3 color;\n"
        "in vec3 normal;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   switch (Mode)\n"
        "   {\n"
        "   default:\n"
        "   case 0:\n"
        "       FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
        "       break;\n"
        "   case 1:\n"
        "       FragColor = vec4(fract(texCoord), 0.0f, 1.0f);\n"
        "       break;\n"
        "   case 2:\n"
        "       FragColor = vec4(color, 1.0f);\n"
        "       break;\n"
        "   case 3:\n"
        "       FragColor = vec4(normalize(normal), 1.0f);\n"
        "       break;\n"
        "   case 4:\n"
        "       FragColor = vec4(color * max(dot(normalize(normal), normalize(vec3(1,0,1))), 0.2f), 1.0f);\n"
        "       break;\n"
        "   }\n"
        "}\n\0";


    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    m_shaderProgram = shaderProgram;
}

void TerrainApplication::UpdateOutputMode()
{
    for (int i = 0; i <= 4; ++i)
    {
        if (GetMainWindow().IsKeyPressed(GLFW_KEY_0 + i))
        {
            int modeLocation = glGetUniformLocation(m_shaderProgram, "Mode");
            glUseProgram(m_shaderProgram);
            glUniform1i(modeLocation, i);
            break;
        }
    }
    if (GetMainWindow().IsKeyPressed(GLFW_KEY_TAB))
    {
        const float projMatrix[16] = { 0, -1.294f, -0.721f, -0.707f, 1.83f, 0, 0, 0, 0, 1.294f, -0.721f, -0.707f, 0, 0, 1.24f, 1.414f };
        int matrixLocation = glGetUniformLocation(m_shaderProgram, "Matrix");
        glUseProgram(m_shaderProgram);
        glUniformMatrix4fv(matrixLocation, 1, false, projMatrix);
    }
}
