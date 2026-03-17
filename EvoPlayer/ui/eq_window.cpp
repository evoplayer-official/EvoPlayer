#include <GL/glew.h>
#include "eq_window.h"
#include <QImage>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QDebug>

static const char* PANEL_VERT = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
out vec2 vUV;
void main(){ vUV=aUV; gl_Position=vec4(aPos,0,1); }
)";
static const char* PANEL_FRAG = R"(
#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
out vec4 fragColor;
void main(){ fragColor = texture(uTex, vUV); }
)";

EqWindow::EqWindow(QWidget* parent) : QOpenGLWidget(parent) {}
EqWindow::~EqWindow() {
    makeCurrent();
    if(m_panelVAO) glDeleteVertexArrays(1, &m_panelVAO);
    if(m_panelVBO) glDeleteBuffers(1, &m_panelVBO);
    if(m_panelTexture) glDeleteTextures(1, &m_panelTexture);
    if(m_panelShader) glDeleteProgram(m_panelShader);
    doneCurrent();
}

GLuint EqWindow::loadTexture(const QString& path) {
    QImage img(path);
    if(img.isNull()){ qWarning() << "EqWindow: cannot load" << path; return 0; }
    img = img.mirrored().convertToFormat(QImage::Format_RGBA8888);
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,img.width(),img.height(),0,GL_RGBA,GL_UNSIGNED_BYTE,img.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

GLuint EqWindow::compileShader() {
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, PANEL_VERT);
    GLuint fs = compile(GL_FRAGMENT_SHADER, PANEL_FRAG);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

void EqWindow::initializeGL() {
    glewExperimental = GL_TRUE;
    glewInit();
    glClearColor(0,0,0,1);
    m_panelShader = compileShader();
    float verts[] = {
        -1,-1, 0,0,  1,-1, 1,0,  1,1, 1,1,
        -1,-1, 0,0,  1,1,  1,1, -1,1, 0,1
    };
    glGenVertexArrays(1, &m_panelVAO);
    glGenBuffers(1, &m_panelVBO);
    glBindVertexArray(m_panelVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_panelVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glBindVertexArray(0);

    QString base = QCoreApplication::applicationDirPath() + "/../";
    m_panelTexture = loadTexture(base + "assets/eq_panel.png");

    m_eqVuMeter.init(m_panelShader);

    m_slider1.init(base+"assets/knobs/eq_slider.png",  100, 130, 272, 47);
    connect(&m_timer, &QTimer::timeout, this, [this](){ update(); });
    m_timer.start(16);
    m_slider2.init(base+"assets/knobs/eq_slider.png",  178, 130, 272, 47);
    m_slider3.init(base+"assets/knobs/eq_slider.png",  257, 130, 272, 47);
    m_slider4.init(base+"assets/knobs/eq_slider.png",  336, 130, 272, 47);
    m_slider5.init(base+"assets/knobs/eq_slider.png",  418, 130, 273, 48);
    m_slider6.init(base+"assets/knobs/eq_slider.png", 1607, 130, 273, 48);
    m_slider7.init(base+"assets/knobs/eq_slider.png", 1684, 130, 272, 47);
    m_slider8.init(base+"assets/knobs/eq_slider.png", 1763, 130, 272, 47);
    m_slider9.init(base+"assets/knobs/eq_slider.png", 1843, 130, 272, 48);
    m_slider10.init(base+"assets/knobs/eq_slider.png",1924, 130, 272, 48);
}

void EqWindow::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void EqWindow::paintGL() {
    makeCurrent();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,0);
    glClear(GL_COLOR_BUFFER_BIT);
    drawPanel();

    float sv[10] = {
        m_slider1.value(), m_slider2.value(), m_slider3.value(),
        m_slider4.value(), m_slider5.value(), m_slider6.value(),
        m_slider7.value(), m_slider8.value(), m_slider9.value(),
        m_slider10.value()
    };
    m_eqVuMeter.render(width(), height(), m_panelW, m_panelH, 517, 85, 997, 337, sv);

    m_slider1.render(width(), height(), m_panelW, m_panelH);
    m_slider2.render(width(), height(), m_panelW, m_panelH);
    m_slider3.render(width(), height(), m_panelW, m_panelH);
    m_slider4.render(width(), height(), m_panelW, m_panelH);
    m_slider5.render(width(), height(), m_panelW, m_panelH);
    m_slider6.render(width(), height(), m_panelW, m_panelH);
    m_slider7.render(width(), height(), m_panelW, m_panelH);
    m_slider8.render(width(), height(), m_panelW, m_panelH);
    m_slider9.render(width(), height(), m_panelW, m_panelH);
    m_slider10.render(width(), height(), m_panelW, m_panelH);
}

void EqWindow::drawPanel() {
    glUseProgram(m_panelShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_panelTexture);
    glUniform1i(glGetUniformLocation(m_panelShader,"uTex"), 0);
    glBindVertexArray(m_panelVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

void EqWindow::mousePressEvent(QMouseEvent* e) {
    m_slider1.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    m_slider2.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    m_slider3.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    m_slider4.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    m_slider5.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    m_slider6.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    m_slider7.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    m_slider8.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    m_slider9.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    m_slider10.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    update();
}

void EqWindow::mouseMoveEvent(QMouseEvent* e) {
    if(e->buttons() & Qt::LeftButton) {
        m_slider1.mouseMoveEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
        m_slider2.mouseMoveEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
        m_slider3.mouseMoveEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
        m_slider4.mouseMoveEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
        m_slider5.mouseMoveEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
        m_slider6.mouseMoveEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
        m_slider7.mouseMoveEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
        m_slider8.mouseMoveEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
        m_slider9.mouseMoveEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
        m_slider10.mouseMoveEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    }
    update();
}

void EqWindow::mouseReleaseEvent(QMouseEvent* e) {
    Q_UNUSED(e);
    m_slider1.mouseReleaseEvent();
    m_slider2.mouseReleaseEvent();
    m_slider3.mouseReleaseEvent();
    m_slider4.mouseReleaseEvent();
    m_slider5.mouseReleaseEvent();
    m_slider6.mouseReleaseEvent();
    m_slider7.mouseReleaseEvent();
    m_slider8.mouseReleaseEvent();
    m_slider9.mouseReleaseEvent();
    m_slider10.mouseReleaseEvent();
}

void EqWindow::setVideo(VideoScreen* video) { m_video=video; qDebug()<<"EqWindow: setVideo="<<video; }

void EqWindow::setAudio(QmmpBridge* audio) {
    m_audio = audio;
    auto conn = [this](EqSlider& sl, int band){
        connect(&sl, &EqSlider::valueChanged, this, [this, band](float v){
            if(m_audio) m_audio->setEqBand(band, v);
            if(m_video){
                float gains[10]={0};
                gains[0]=m_slider1.value()-0.5f;
                gains[1]=m_slider2.value()-0.5f;
                gains[2]=m_slider3.value()-0.5f;
                gains[3]=m_slider4.value()-0.5f;
                gains[4]=m_slider5.value()-0.5f;
                gains[5]=m_slider6.value()-0.5f;
                gains[6]=m_slider7.value()-0.5f;
                gains[7]=m_slider8.value()-0.5f;
                gains[8]=m_slider9.value()-0.5f;
                gains[9]=m_slider10.value()-0.5f;
                m_video->setEqGains(gains);
            }
        });
    };
    conn(m_slider1,  0);
    conn(m_slider2,  1);
    conn(m_slider3,  2);
    conn(m_slider4,  3);
    conn(m_slider5,  4);
    conn(m_slider6,  5);
    conn(m_slider7,  6);
    conn(m_slider8,  7);
    conn(m_slider9,  8);
    conn(m_slider10, 9);
}
