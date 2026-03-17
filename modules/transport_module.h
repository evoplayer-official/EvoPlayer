#pragma once
#include <GL/glew.h>
#include <QMatrix4x4>
#include <functional>
#include <QProcess>
enum class PlayerState { STOPPED, PLAYING, PAUSED };
struct Button {
    float x, y, w, h;
    float pressAnim = 0.0f;
    bool hovered = false;
    int id;
};
class TransportModule {
public:
    TransportModule();
    ~TransportModule();
    void init(GLuint shaderProgram, GLuint panelShader);
    void render(const QMatrix4x4 &view, const QMatrix4x4 &proj);
    void handleMouseMove(float nx, float ny);
    void handleMousePress(float nx, float ny);
    void update(float dt);
    void setPlayerState(PlayerState state);
    std::function<void()> onPlay;
    std::function<void()> onPause;
    std::function<void()> onStop;
    std::function<void()> onNext;
    std::function<void()> onPrev;
    std::function<void()> onOpen;
private:
    GLuint m_vao, m_vbo, m_ebo;
    GLuint m_shader;
    GLuint m_panelShader;
    Button m_buttons[6];
    PlayerState m_state;
    int m_lastPressed = -1;
    GLuint m_texPressed[6];
    GLuint m_texHover[6];
    GLuint loadTexture(const QString &path);
    void drawOverlay(int btnIdx, GLuint tex);
    void setupGeometry();
    void drawButton(const Button &btn, const QMatrix4x4 &view, const QMatrix4x4 &proj);

    QVector3D buttonColor(const Button &btn) const;
};
