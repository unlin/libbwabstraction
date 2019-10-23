#ifndef STANDARDSHADER_H
#define STANDARDSHADER_H

#include <GL/glew.h>
#include <vector>

enum StandardVertexAttributeLocation
{
    ATTRIB_POSITION = 0,
    ATTRIB_NORMAL = 1,
    ATTRIB_TEXCOORD = 2,
    ATTRIB_COLOR = 3,
    ATTRIB_INDEX = 4
};

class StandardVertexAttribute
{

public:

    StandardVertexAttribute();

    /// Creates a full-screen quad with texcoords for full-screen image passes.
    static StandardVertexAttribute CreateQuadAttribute();

    void Create();
    void Destroy();
    void Bind();
    void Reset();
    void BufferData(std::vector<float> &positions,
                    std::vector<float> &normals,
                    std::vector<float> &texcoords,
                    std::vector<float> &colors,
                    std::vector<unsigned int> &indices);
    void Color3f(float r, float g, float b);
    inline GLsizei VertexCount() { return vertexCount; }
    inline GLsizei IndexCount() { return indexCount; }

private:

    GLuint vao;
    GLuint buffers[5];
    GLsizei vertexCount;
    GLsizei indexCount;

};

class StandardShader
{

public:

    void CreateComputeFromFile(const char *compShader);
    void CreateCompute(const char *compShader);
    void CreateFromFile(const char *vertShader, const char *fragShader, const char *geoShader = NULL);
    void Create(const char *vertShader, const char *fragShader, const char *geoShader = NULL);
    void Destroy();
    void Bind();
    void Draw(GLenum mode, StandardVertexAttribute &attribute);

    GLuint program;

private:

    char* LoadSrcFromFile(const char *file);

};

#endif // STANDARDSHADER_H

#ifdef STANDARDSHADER_IMPLEMENTION

#include <cstdio>

StandardVertexAttribute::StandardVertexAttribute()
{
    memset(this, 0, sizeof(StandardVertexAttribute));
}

StandardVertexAttribute StandardVertexAttribute::CreateQuadAttribute()
{
    const float QUAD_VERTEX[12] =
    {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };
    const float QUAD_TEXCOORD[8] =
    {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    const unsigned int QUAD_INDEX[6] =
    {
        0, 1, 2,
        2, 3, 0
    };
    StandardVertexAttribute attribute;
    attribute.Create();
    std::vector<float> positions(QUAD_VERTEX, QUAD_VERTEX + 12);
    std::vector<float> empty;
    std::vector<float> texcoords(QUAD_TEXCOORD, QUAD_TEXCOORD + 8);
    std::vector<unsigned int> indices(QUAD_INDEX, QUAD_INDEX + 6);
    attribute.BufferData(positions, empty, texcoords, empty, indices);
    return attribute;
}

void StandardVertexAttribute::Create()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(5, buffers);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[ATTRIB_POSITION]);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[ATTRIB_NORMAL]);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[ATTRIB_TEXCOORD]);
    glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[ATTRIB_COLOR]);
    glVertexAttribPointer(ATTRIB_COLOR, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[ATTRIB_INDEX]);
}

void StandardVertexAttribute::Destroy()
{
    if(vao != 0)
    {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(5, buffers);
    }
}

void StandardVertexAttribute::Bind()
{
    glBindVertexArray(vao);
}

void StandardVertexAttribute::Reset()
{
    std::vector<float> empty;
    std::vector<unsigned int> emptyIdx;
    BufferData(empty, empty, empty, empty, emptyIdx);
}

void StandardVertexAttribute::BufferData(std::vector<float> &positions,
                                         std::vector<float> &normals,
                                         std::vector<float> &texcoords,
                                         std::vector<float> &colors,
                                         std::vector<unsigned int> &indices)
{
    glBindVertexArray(vao);

    vertexCount = static_cast<GLsizei>(positions.size() / 3);
    indexCount = static_cast<GLsizei>(indices.size());

    if(positions.size() != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buffers[ATTRIB_POSITION]);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(ATTRIB_POSITION);
    }
    else
    {
        glDisableVertexAttribArray(ATTRIB_POSITION);
    }
    if(normals.size() != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buffers[ATTRIB_NORMAL]);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(ATTRIB_NORMAL);
    }
    else
    {
        glDisableVertexAttribArray(ATTRIB_NORMAL);
    }
    if(texcoords.size() != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buffers[ATTRIB_TEXCOORD]);
        glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(float), texcoords.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(ATTRIB_TEXCOORD);
    }
    else
    {
        glDisableVertexAttribArray(ATTRIB_TEXCOORD);
    }
    if(colors.size() != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buffers[ATTRIB_COLOR]);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(ATTRIB_COLOR);
    }
    else
    {
        glDisableVertexAttribArray(ATTRIB_COLOR);
    }
    if(indices.size() != 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[ATTRIB_INDEX]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    }
    else
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}

void StandardVertexAttribute::Color3f(float r, float g, float b)
{
    glBindVertexArray(vao);
    glVertexAttrib3f(ATTRIB_COLOR, r, g, b);
}

char* StandardShader::LoadSrcFromFile(const char *file)
{
    FILE* f = fopen(file, "r");
    if (f == NULL)
    {
        throw std::invalid_argument("Cannot open shader file. LoadSrcFromFile() failed.");
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    char* src = new char[sz + 1];
    src[sz] = '\0';
    fseek(f, 0, SEEK_SET);
    fread(src, sz, 1, f);
    fclose(f);
    return src;
}

void StandardShader::CreateCompute(const char *compShaderSrc)
{
    program = glCreateProgram();

    GLuint compShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compShader, 1, &compShaderSrc, 0);
    glCompileShader(compShader);
    glPrintShaderLog(compShader);

    glAttachShader(program, compShader);
    glLinkProgram(program);
    glPrintProgramLog(program);

    glDeleteShader(compShader);
}

void StandardShader::CreateComputeFromFile(const char *compShaderFile)
{
    char *compShaderSrc = LoadSrcFromFile(compShaderFile);
    CreateCompute(compShaderSrc);
    delete[] compShaderSrc;
}

void StandardShader::CreateFromFile(const char *vertShaderFile, const char *fragShaderFile, const char *geoShaderFile)
{
    char *vertShaderSrc = LoadSrcFromFile(vertShaderFile);
    char *fragShaderSrc = LoadSrcFromFile(fragShaderFile);
    char *geoShaderSrc = geoShaderFile ? LoadSrcFromFile(geoShaderFile) : NULL;
    Create(vertShaderSrc, fragShaderSrc, geoShaderSrc);
    delete[] vertShaderSrc;
    delete[] fragShaderSrc;
    if(geoShaderSrc)
    {
        delete[] geoShaderSrc;
    }
}

void StandardShader::Create(const char *vertShaderSrc, const char *fragShaderSrc, const char *geoShaderSrc)
{
    program = glCreateProgram();

    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertShaderSrc, 0);
    glCompileShader(vertShader);
    glPrintShaderLog(vertShader);

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragShaderSrc, 0);
    glCompileShader(fragShader);
    glPrintShaderLog(fragShader);

    GLuint geoShader;
    if(geoShaderSrc)
    {
        geoShader = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geoShader, 1, &geoShaderSrc, 0);
        glCompileShader(geoShader);
        glPrintShaderLog(geoShader);

        glAttachShader(program, geoShader);
    }

    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glPrintProgramLog(program);
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    if(geoShaderSrc)
    {
        glDeleteShader(geoShader);
    }
}

void StandardShader::Destroy()
{
    glDeleteProgram(program);
}

void StandardShader::Bind()
{
    glUseProgram(program);
}

void StandardShader::Draw(GLenum mode, StandardVertexAttribute &attribute)
{
    Bind();
    attribute.Bind();

    if(attribute.IndexCount() == 0)
    {
        if(attribute.VertexCount() != 0)
        {
            glDrawArrays(mode, 0, attribute.VertexCount());
        }
    }
    else
    {
        glDrawElements(mode, attribute.IndexCount(), GL_UNSIGNED_INT, 0);
    }
}

#endif // STANDARDSHADER_IMPLEMENT
