#pragma once
#include <GL/glew.h>
#include <qmmp/visual.h>

class VisualizerScreen : public Visual {
public:
    VisualizerScreen(QWidget* parent=nullptr);
    ~VisualizerScreen();
    void init();
    void render(int winW, int winH, int panelW, int panelH,
                int sx, int sy, int sw, int sh);
private:
    GLuint m_shader  = 0;
    GLuint m_shaderVU = 0;
    GLuint m_vao     = 0;
    GLuint m_vbo     = 0;
    GLuint m_fftTex  = 0;
    bool   m_ready   = false;
    float  m_smooth[256] = {};
    float  m_peak[256]   = {};
    float  m_vuBands[8]  = {};
    float  m_vuPeak[8]   = {};
    float  m_time        = 0.0f;
    int    m_frameSkip   = 0;
    bool   m_videoActive = false;
    GLuint compileShader();
public:
    void setVideoActive(bool v){ m_videoActive=v; }
    void   updateFFT(float* pcmL, float* pcmR);
};
