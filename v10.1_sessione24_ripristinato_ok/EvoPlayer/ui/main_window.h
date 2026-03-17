#pragma once
class LibraryWindow;
#include <GL/glew.h>
#include <QMainWindow>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QTimer>
#include <QElapsedTimer>
#include "audio/qmmp_bridge.h"
#include "engine/camera_controller.h"
#include "engine/model_loader.h"
#include "modules/transport_module.h"
#include "modules/display_module.h"
#include "modules/vumeter_module.h"
#include "modules/knob_module.h"
#include "modules/power_button.h"

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget();
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
private:
    GLuint m_shader;
    GLuint m_textShader;
    GLuint m_panelShader;
    GLuint m_panelTexture;
    int m_panelW = 2030;
    int m_panelH = 537;
    GLuint m_panelVAO, m_panelVBO;
    GLuint m_quadVAO, m_quadVBO, m_quadEBO;
    GLuint loadShaders(const QString &vert, const QString &frag);
    GLuint loadTexture(const QString &path);
    void setupPanel();
    void drawPanel();
    void setupQuad();
    void drawPlaceholder(const QMatrix4x4 &view, const QMatrix4x4 &proj,
                         float x, float y, float w, float h,
                         float r, float g, float b);
    void drawModel(const QMatrix4x4 &view, const QMatrix4x4 &proj);
    CameraController m_camera;
    ModelLoader      m_model;
    TransportModule  m_transport;
    DisplayModule    m_display;
    VuMeterModule    m_vuMeter;
    KnobModule       m_knobVolume;
private:
    PowerButton      m_powerButton;
    KnobModule       m_knobBass;
    KnobModule       m_knobTreble;
    QmmpBridge      *m_audio;
public:
    QmmpBridge* audio() { return m_audio; }
    void setLibraryWindow(LibraryWindow* lib){ m_lib=lib; }
private:
    LibraryWindow* m_lib = nullptr;
private:
    QTimer           m_timer;
    QElapsedTimer    m_elapsed;
    QPoint           m_dragPos;
    bool             m_dragging = false;
};

class GLWidget;
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    QmmpBridge* audio();
    void setLibraryWindow(LibraryWindow* lib);
private:
    GLWidget* m_gl = nullptr;
};
