#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include <GL/glew.h>
#include <opencv2/opencv.hpp>

typedef struct _INT32DTarget
{
    GLuint fbo;
    GLuint color_id_buffer;
    GLuint depth_buffer;
    int w;
    int h;

    void init()
    {
        glGenTextures(1, &color_id_buffer);
        glBindTexture(GL_TEXTURE_2D, color_id_buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenTextures(1, &depth_buffer);
        glBindTexture(GL_TEXTURE_2D, depth_buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_id_buffer, 0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_buffer, 0);
    }

    void resize(int w, int h)
    {
        this->w = w;
        this->h = h;

        glBindTexture(GL_TEXTURE_2D, color_id_buffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, w, h, 0, GL_RED_INTEGER, GL_INT, NULL);

        glBindTexture(GL_TEXTURE_2D, depth_buffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }

    void bind()
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glViewport(0, 0, w, h);
    }

    cv::Mat readback()
    {
        cv::Mat m = cv::Mat(h, w, CV_32SC1);
        glBindTexture(GL_TEXTURE_2D, color_id_buffer);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_INT, m.data);
        cv::Mat m2 = cv::Mat(h, w, CV_32SC1);
        cv::flip(m, m2, 0);
        return m2;
    }

    cv::Mat readbackDepth()
    {
        cv::Mat m = cv::Mat(h, w, CV_32FC1);
        glBindTexture(GL_TEXTURE_2D, depth_buffer);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, m.data);
        cv::Mat m2 = cv::Mat(h, w, CV_32FC1);
        cv::flip(m, m2, 0);
        return m2;
    }

    void update(cv::Mat m)
    {
        cv::Mat m2 = cv::Mat(h, w, CV_8UC4);
        cv::flip(m, m2, 0);
        glBindTexture(GL_TEXTURE_2D, color_id_buffer);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED_INTEGER, GL_INT, m2.data);
    }

    void updateDepth(cv::Mat m)
    {
        cv::Mat m2 = cv::Mat(h, w, CV_32FC1);
        cv::flip(m, m2, 0);
        glBindTexture(GL_TEXTURE_2D, depth_buffer);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_DEPTH_COMPONENT, GL_FLOAT, m2.data);
    }

    void bindClear(int val)
    {
        bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        glClearBufferiv(GL_COLOR, 0, &val);
    }

    void release()
    {
        glDeleteTextures(1, &color_id_buffer);
        glDeleteTextures(1, &depth_buffer);
        glDeleteFramebuffers(1, &fbo);
    }

} INT32DTarget;

typedef struct _RGBADTarget
{
    GLuint fbo;
    GLuint color_id_buffer;
    GLuint depth_buffer;
    int w;
    int h;

    void init()
    {
        glGenTextures(1, &color_id_buffer);
        glBindTexture(GL_TEXTURE_2D, color_id_buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenTextures(1, &depth_buffer);
        glBindTexture(GL_TEXTURE_2D, depth_buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_id_buffer, 0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_buffer, 0);
    }

    void resize(int w, int h)
    {
        this->w = w;
        this->h = h;

        glBindTexture(GL_TEXTURE_2D, color_id_buffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glBindTexture(GL_TEXTURE_2D, depth_buffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }

    void bind()
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glViewport(0, 0, w, h);
    }

    cv::Mat readback()
    {
        cv::Mat m = cv::Mat(h, w, CV_8UC4);
        glBindTexture(GL_TEXTURE_2D, color_id_buffer);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, m.data);
        cv::Mat m2 = cv::Mat(h, w, CV_8UC4);
        cv::flip(m, m2, 0);
        return m2;
    }

    cv::Mat readbackDepth()
    {
        cv::Mat m = cv::Mat(h, w, CV_32FC1);
        glBindTexture(GL_TEXTURE_2D, depth_buffer);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, m.data);
        cv::Mat m2 = cv::Mat(h, w, CV_32FC1);
        cv::flip(m, m2, 0);
        return m2;
    }

    void update(cv::Mat m)
    {
        cv::Mat m2 = cv::Mat(h, w, CV_8UC4);
        cv::flip(m, m2, 0);
        glBindTexture(GL_TEXTURE_2D, color_id_buffer);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, m2.data);
    }

    void updateDepth(cv::Mat m)
    {
        cv::Mat m2 = cv::Mat(h, w, CV_32FC1);
        cv::flip(m, m2, 0);
        glBindTexture(GL_TEXTURE_2D, depth_buffer);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_DEPTH_COMPONENT, GL_FLOAT, m2.data);
    }

    void bindClear()
    {
        bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void release()
    {
        glDeleteTextures(1, &color_id_buffer);
        glDeleteTextures(1, &depth_buffer);
        glDeleteFramebuffers(1, &fbo);
    }

} RGBADTarget;

#endif // RENDERTARGET_H
