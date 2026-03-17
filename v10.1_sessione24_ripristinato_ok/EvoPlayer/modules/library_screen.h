#pragma once
#include <GL/glew.h>
#include <QString>
#include <QStringList>
#include <vector>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H

class LibraryScreen {
public:
    LibraryScreen();
    ~LibraryScreen();
    void init(GLuint shaderProg);
    void render(int winW, int winH, int panelW, int panelH,
                int sx, int sy, int sw, int sh);
    void setTrackList(const QStringList& tracks);
    void setCurrentTrack(const QString& title, const QString& artist);
    void setVideoInfo(const QString& title, const QString& duration, double fps,
                      int w, int h, const QString& vCodec="",
                      const QString& aCodec="", int sampleRate=0, int bitrate=0);
    void scrollUp();
    void scrollDown();
    void setSelectedIndex(int idx);
    int  selectedIndex() const { return m_selectedIdx; }
    int  trackCount()    const { return m_tracks.size(); }
private:
    GLuint m_shader     = 0;
    GLuint m_textShader = 0;
    GLuint m_textVAO  = 0, m_textVBO = 0;
    GLuint m_glyphTex = 0;
    FT_Library m_ft   = nullptr;
    FT_Face    m_face = nullptr;
    QStringList m_tracks;
    QString     m_currentTitle;
    QString     m_currentArtist;
    QString     m_videoDuration;
    double      m_videoFps = 0;
    int         m_videoW = 0, m_videoH = 0;
    bool        m_videoMode = false;
    QString     m_videoVCodec;
    QString     m_videoACodec;
    int         m_videoSampleRate = 0;
    int         m_videoBitrate = 0;
    int         m_scrollOffset = 0;
    int         m_selectedIdx  = 0;
    int         m_visibleRows  = 10;
    void renderBackground(int winW, int winH, int panelW, int panelH,
                          int sx, int sy, int sw, int sh);
    void renderText(const QString& text, float x, float y,
                    float scale, float r, float g, float b, float a,
                    int winW, int winH);
    void initFreeType();
    void initGlyphTexture();
    struct Glyph {
        float tx, ty, tw, th;
        int   bx, by, bw, bh;
        int   advance;
    };
    Glyph m_glyphs[128];
    bool  m_ftReady = false;
};
