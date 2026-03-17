#pragma once
#include <GL/glew.h>
#include <QMatrix4x4>
#include <QString>
#include <QMap>
#include <ft2build.h>
#include FT_FREETYPE_H

struct Character {
    GLuint textureID;
    int width, height;
    int bearingX, bearingY;
    unsigned int advance;
};

class DisplayModule {
public:
    DisplayModule();
    ~DisplayModule();

    bool init(GLuint textShaderProgram, GLuint panelShaderProgram);

    void renderBackground(int winW, int winH);
    void render(const QMatrix4x4 &proj, int winW, int winH,
                const QString &title,
                const QString &artist,
                qint64 elapsed,
                qint64 duration,
                const QString &format,
                int bitrate);

public:
    void setVolume(float v) { m_volume = v; }
    void setBass(float v)   { m_bass = v; }
    void setTreble(float v) { m_treble = v; }
private:
    float m_volume = 0.0f;
    float m_bass   = 0.5f;
    float m_treble = 0.5f;
    FT_Library m_ft;
    FT_Face    m_face;
    FT_Face    m_faceDigital;
    QMap<QChar, Character> m_charsDigital;
    QMap<QChar, Character> m_chars;
    GLuint m_vao, m_vbo;
    GLuint m_shader;
    GLuint m_panelShader;
    GLuint m_texDisplay;
    GLuint m_displayShader;
    GLuint m_panelVao, m_panelVbo, m_panelEbo;
    bool m_ready;

    void loadFont(const QString &path, int size);
    void loadFontInto(const QString &path, int size, FT_Face &face, QMap<QChar, Character> &chars);
    void renderText(const QString &text, float x, float y,
                    float scale, float r, float g, float b);
    void renderTextWith(const QString &text, float x, float y,
                    float scale, float r, float g, float b,
                    QMap<QChar, Character> &chars);

    void renderProgressBarValue(float x, float y, float w, float h, float value);
    void renderProgressBar(float x, float y, float w, float h,
                           qint64 elapsed, qint64 duration);
    QString formatTime(qint64 ms);
    GLuint loadTexture(const QString &path);
    GLuint loadDisplayShader();
};
