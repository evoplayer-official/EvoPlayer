#pragma once
#include <GL/glew.h>
#include <QObject>
#include <QString>

class EqSlider : public QObject {
    Q_OBJECT
public:
    explicit EqSlider(QObject* parent = nullptr);
    ~EqSlider();
    void init(const QString& texPath, float panelX, float trackY, float trackH, float trackW);
    void render(int winW, int winH, int panelW, int panelH);
    void mousePressEvent(int x, int y, int winW, int winH, int panelW, int panelH);
    void mouseMoveEvent(int x, int y, int winW, int winH, int panelW, int panelH);
    void mouseReleaseEvent();
    float value() const { return m_value; }

signals:
    void valueChanged(float v);

private:
    GLuint m_tex = 0;
    GLuint m_vao = 0, m_vbo = 0;
    GLuint m_shader = 0;
    float m_panelX = 0;   // centro X corsia sulla plancia
    float m_trackY = 0;   // top corsia sulla plancia
    float m_trackH = 0;   // altezza corsia
    float m_trackW = 0;   // larghezza corsia
    float m_value = 0.5f; // 0=bottom 1=top
    bool  m_dragging = false;
    GLuint loadTexture(const QString& path);
    GLuint compileShader();
};
