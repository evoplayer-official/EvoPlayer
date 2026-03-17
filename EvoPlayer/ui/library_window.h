#pragma once
#include <GL/glew.h>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include "audio/qmmp_bridge.h"
#include <QTimer>
#include <QFileInfo>
#include "modules/library_screen.h"
#include "modules/visualizer_screen.h"
#include "modules/video_screen.h"

class LibraryWindow : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit LibraryWindow(QWidget* parent = nullptr);
    ~LibraryWindow();
    void setAudio(QmmpBridge* audio);
    void loadVideo(const QString& path);
    VideoScreen& videoScreen() { return m_videoScreen; }
    void videoStop();
    void videoPause();
    bool isVideoPlaying() const;
protected:
    void initializeGL() override;
    void paintEvent(QPaintEvent* e) override;
    void paintGL()      override;
    void resizeGL(int w, int h) override;
    void mousePressEvent(QMouseEvent* e)   override;
    void mouseMoveEvent(QMouseEvent* e)    override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e)        override;
private:
    GLuint m_panelShader  = 0;
    GLuint m_panelTexture = 0;
    GLuint m_panelVAO = 0, m_panelVBO = 0;
    int    m_panelW = 2030, m_panelH = 537;
    QTimer       m_timer;
    QmmpBridge*  m_audio = nullptr;
    GLuint loadTexture(const QString& path);
    GLuint compileShader();
    void   drawPanel();
    LibraryScreen      m_libScreen;
    VisualizerScreen   m_vizScreen;
    VideoScreen   m_videoScreen;
    QStringList   m_lastPlaylist;
    bool          m_glReady = false;
};
