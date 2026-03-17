#pragma once
#include <GL/glew.h>
#include <QObject>
#include <QString>

class PowerButton : public QObject {
    Q_OBJECT
public:
    explicit PowerButton(QObject* parent = nullptr);
    ~PowerButton();
    void init(const QString& pathNormal, const QString& pathOn,
              float panelX, float panelY, float size);
    void render(int winW, int winH, int panelW, int panelH);
    void mousePressEvent(int x, int y, int winW, int winH, int panelW, int panelH);
    bool isOn() const { return m_isOn; }
    void mouseMoveEvent(int x, int y, int winW, int winH, int panelW, int panelH);

signals:
    void clicked();

private:
    GLuint m_texNormal = 0;
    GLuint m_texOn     = 0;
    GLuint m_vao = 0, m_vbo = 0;
    GLuint m_shader = 0;
    float m_panelX = 0, m_panelY = 0;
    float m_size = 0;
    bool m_isOn = false;
    bool m_hover = false;
    GLuint loadTexture(const QString& path);
    GLuint compileShader();
};
