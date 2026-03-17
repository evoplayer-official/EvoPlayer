#pragma once
#include <GL/glew.h>
#include <QObject>
#include <QString>

class KnobModule : public QObject {
    Q_OBJECT
public:
    explicit KnobModule(QObject* parent = nullptr);
    ~KnobModule();
    void init(const QString& texturePath, float panelX, float panelY, float size);
    void render(int windowWidth, int windowHeight, int panelW, int panelH);
    void mousePressEvent(int x, int y, int windowWidth, int windowHeight, int panelW, int panelH);
    void mouseMoveEvent(int x, int y);
    void mouseReleaseEvent();
    float value() const { return m_value; }
signals:
    void valueChanged(float value);
private:
    GLuint m_texture = 0;
    GLuint m_vao = 0, m_vbo = 0;
    GLuint m_shader = 0;
    float m_panelX = 0, m_panelY = 0;
    float m_size = 0;
    float m_angle = 135.0f;
    float m_value = 0.0f;
    bool m_dragging = false;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;
    float m_centerX = 0, m_centerY = 0;
    const float MIN_ANGLE = -135.0f;
    const float MAX_ANGLE =  135.0f;
    GLuint compileShader();
    GLuint loadTexture(const QString& path);
};
