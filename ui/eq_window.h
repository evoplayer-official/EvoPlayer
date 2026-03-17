#pragma once
#include "modules/video_screen.h"
#include <GL/glew.h>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include "audio/qmmp_bridge.h"
#include <QTimer>
#include "modules/eq_slider.h"
#include "modules/eq_vumeter.h"

class EqWindow : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit EqWindow(QWidget* parent = nullptr);
    ~EqWindow();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mousePressEvent(QMouseEvent* e) override;
    QPoint m_dragPos;
    bool   m_dragging = false;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    GLuint m_panelShader = 0;
    GLuint m_panelTexture = 0;
    GLuint m_panelVAO = 0, m_panelVBO = 0;
    int m_panelW = 2030, m_panelH = 537;
    QTimer m_timer;
    QmmpBridge* m_audio = nullptr;
    VideoScreen* m_video = nullptr;


    GLuint loadTexture(const QString& path);
    GLuint compileShader();
    void drawPanel();
public:
    void setAudio(QmmpBridge* audio);
    void setVideo(VideoScreen* video);
private:
    EqVuMeter m_eqVuMeter;
    EqSlider m_slider1;
    EqSlider m_slider2;
    EqSlider m_slider3;
    EqSlider m_slider4;
    EqSlider m_slider5;
    EqSlider m_slider6;
    EqSlider m_slider7;
    EqSlider m_slider8;
    EqSlider m_slider9;
    EqSlider m_slider10;
};
