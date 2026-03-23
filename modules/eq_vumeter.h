#pragma once
#include <GL/glew.h>
#include <qmmp/visual.h>

class EqVuMeter : public Visual {
public:
    EqVuMeter(QWidget* parent = nullptr);
    ~EqVuMeter();
    void init(GLuint shader);
    void render(int winW, int winH, int panelW, int panelH,
                float dispX, float dispY, float dispW, float dispH,
                float* sliderValues);
private:
    GLuint m_shader = 0;
    GLuint m_vao    = 0;
    GLuint m_vbo    = 0;
    bool   m_ready  = false;
    static const int BANDS = 64;
    float  m_smooth[BANDS] = {};
    float  m_peak[BANDS]   = {};
    void drawBar(float x, float y, float w, float h, float r, float g, float b);
};
