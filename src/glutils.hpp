#ifndef GLUTILS_H_
#define GLUTILS_H_

#include <GL/glew.h>
#include <iostream>

void glPrintContextInfo(bool printExtension = false);
void glPrintError(void);
void glPrintFramebufferStatus(GLenum target);
void glPrintShaderLog(GLuint shader);
void glPrintProgramLog(GLuint program);
void glPrintComputeShaderInfo();
void glPrintUniformInfo();

#endif // GLUTILS_H_

#ifdef GLUTILS_IMPLEMENTATION

void glPrintContextInfo(bool printExtension)
{
    std::cout << "===== BEGIN CONTEXT INFO =====" << std::endl;
    std::cout << "GL_VENDOR = " << reinterpret_cast<const char*>(glGetString(GL_VENDOR)) << std::endl;
    std::cout << "GL_RENDERER = " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << std::endl;
    std::cout << "GL_VERSION = " << reinterpret_cast<const char*>(glGetString(GL_VERSION)) << std::endl;
    std::cout << "GL_SHADING_LANGUAGE_VERSION = " << reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)) << std::endl;
    if(printExtension)
    {
        GLint numExt;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
        std::cout << "GL_EXTENSIONS =" << std::endl;
        for(GLint i = 0; i < numExt; i++)
        {
            std::cout << "\t" << reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i)) << std::endl;
        }
    }
    std::cout << "==============================" << std::endl;
}

void glPrintError(void)
{
    GLenum code = glGetError();
    switch(code)
    {
        case GL_NO_ERROR:
            std::cout << "GL_NO_ERROR" << std::endl;
            break;
        case GL_INVALID_ENUM:
            std::cout << "GL_INVALID_ENUM" << std::endl;
            break;
        case GL_INVALID_VALUE:
            std::cout << "GL_INVALID_VALUE" << std::endl;
            break;
        case GL_INVALID_OPERATION:
            std::cout << "GL_INVALID_OPERATION" << std::endl;
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            std::cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << std::endl;
            break;
        case GL_OUT_OF_MEMORY:
            std::cout << "GL_OUT_OF_MEMORY" << std::endl;
            break;
        case GL_STACK_OVERFLOW:
            std::cout << "GL_STACK_OVERFLOW" << std::endl;
            break;
        case GL_STACK_UNDERFLOW:
            std::cout << "GL_STACK_UNDERFLOW" << std::endl;
            break;
        case GL_CONTEXT_LOST:
            std::cout << "GL_CONTEXT_LOST" << std::endl;
            break;
        default:
            std::cout << "UNKNOWN ERROR ENUM: " << std::hex << code << std::endl;
    }
}

void glPrintFramebufferStatus(GLenum target)
{
    GLenum status = glCheckFramebufferStatus(target);
    switch(status)
    {
        case GL_FRAMEBUFFER_COMPLETE:
            std::cout << "GL_FRAMEBUFFER_COMPLETE" << std::endl;
            break;
        case GL_FRAMEBUFFER_UNDEFINED:
            std::cout << "GL_FRAMEBUFFER_UNDEFINED" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            std::cout << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            std::cout << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            std::cout << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            std::cout << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" << std::endl;
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            std::cout << "GL_FRAMEBUFFER_UNSUPPORTED" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            std::cout << "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            std::cout << "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS" << std::endl;
            break;
        default:
            std::cout << "UNKNOWN ENUM: " << std::hex << status << std::endl;
    }
}

void glPrintShaderLog(GLuint shader)
{
    GLint maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
    if(maxLength > 0)
    {
        GLchar* infoLog = new GLchar[maxLength];
        glGetShaderInfoLog(shader, maxLength, NULL, infoLog);

        std::cout << infoLog << std::endl;
        delete[] infoLog;
    }
}

void glPrintProgramLog(GLuint program)
{
	GLint maxLength;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
	if (maxLength > 0)
	{
		GLchar *infoLog = new GLchar[maxLength];
		glGetProgramInfoLog(program, maxLength, NULL, infoLog);
        std::cout << infoLog << std::endl;
		delete[] infoLog;
	}
}

void glPrintComputeShaderInfo()
{
    std::cout << "===== BEGIN COMPUTE SHADER INFO =====" << std::endl;
    GLint value;
    GLint values[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, values);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, values + 1);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, values + 2);
    std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT = (" << values[0] << ", " << values[1] << ", " << values[2] << ")" << std::endl;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, values);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, values + 1);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, values + 2);
    std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE = (" << values[0] << ", " << values[1] << ", " << values[2] << ")" << std::endl;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &value);
    std::cout << "GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS = " << value << std::endl;
    glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &value);
    std::cout << "GL_MAX_COMPUTE_SHARED_MEMORY_SIZE = " << value << " bytes" << std::endl;
    std::cout << "=====================================" << std::endl;
}

void glPrintUniformInfo()
{
    std::cout << "===== BEGIN UNIFORM INFO =====" << std::endl;
    GLint value;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &value);
    std::cout << "GL_MAX_VERTEX_UNIFORM_COMPONENTS = " << value << std::endl;
    glGetIntegerv(GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS , &value);
    std::cout << "GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS  = " << value << std::endl;
    glGetIntegerv(GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS , &value);
    std::cout << "GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS  = " << value << std::endl;
    glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS, &value);
    std::cout << "GL_MAX_GEOMETRY_UNIFORM_COMPONENTS = " << value << std::endl;
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS , &value);
    std::cout << "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS  = " << value << std::endl;
    glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS , &value);
    std::cout << "GL_MAX_UNIFORM_LOCATIONS  = " << value << std::endl;
    std::cout << "==============================" << std::endl;
}

#endif // GLUTILS_IMPLEMENTATION
