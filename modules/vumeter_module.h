#pragma once
#include <GL/glew.h>
#include <QMatrix4x4>
#include <qmmp/visual.h>
#include <cmath>

class VuMeterModule : public Visual {
    Q_OBJECT
public:
    explicit VuMeterModule(QWidget *parent = nullptr);
    ~VuMeterModule();

    void init(GLuint textShader);
    void render(int winW=1600, int winH=423);
    void getLevel(float &left, float &right);

private:
    GLuint m_shader;
    GLuint m_vao, m_vbo;
    bool m_ready = false;
    float m_levelL = 0.0f;
    float m_levelR = 0.0f;
    float m_peakL  = 0.0f;
    float m_peakR  = 0.0f;

    void drawBar(float x, float y, float w, float h, float r, float g, float b);
    void setupGeometry();
};
