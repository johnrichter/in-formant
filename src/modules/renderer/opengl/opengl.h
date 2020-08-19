#ifndef RENDERER_OPENGL_H
#define RENDERER_OPENGL_H

#include "../base/base.h"
#include <GL/glew.h>

namespace Module::Renderer {

    class OpenGL : public AbstractBase {
    public:
        OpenGL();
        ~OpenGL();

        void setProvider(void *provider) override; 

        void initialize() override;
        void terminate() override;

        void begin() override;
        void end() override;

        void clear() override;

        void test() override;
 
        void renderGraph(float *x, float *y, size_t count) override;
        void renderSpectrogram(float ***spectrogram, size_t *lengths, size_t count) override;

    private:
        void initMultisampling();

        GLuint createProgram(const std::string& vertexShaderFilename,
                             const std::string& fragmentShaderFilename);

        GLuint loadShader(GLenum shaderType, const std::vector<char>& code);

        static GLEWAPIENTRY void debugCallback(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar *, const void *);

        OpenGLProvider *mProvider;

        std::vector<Vertex> mVertices;
        UniformBuffer mUniforms;

        GLuint mGraphProgram;
        GLint mLocPosition;
        GLint mLocOffsetX;
        GLint mLocScaleX;

        GLuint mSpectrogramProgram;
        GLint mLocPoint;
        GLint mLocMinFreq;
        GLint mLocMaxFreq;
        GLint mLocMinGain;
        GLint mLocMaxGain;
        GLint mLocScaleType;

        GLuint mVBO;
        GLuint mVAO;

        void *mBufferData;

        GLuint mRBO;
        GLuint mFBO;
        int mMsCount;
        GLuint mMsTexture;
    };

}

#endif // RENDERER_OPENGL_H
