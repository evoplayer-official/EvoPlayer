#include <GL/glew.h>
#include <QWindow>
#include "snap_manager.h"
#include "library_window.h"
#include <QImage>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>
#include <QPainter>
#include <QPaintEvent>

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

LibraryWindow::LibraryWindow(QWidget* parent) : QOpenGLWidget(parent) {
    setMouseTracking(true);
}
LibraryWindow::~LibraryWindow() {
    makeCurrent();
    if(m_panelVAO)     glDeleteVertexArrays(1, &m_panelVAO);
    if(m_panelVBO)     glDeleteBuffers(1, &m_panelVBO);
    if(m_panelTexture) glDeleteTextures(1, &m_panelTexture);
    if(m_panelShader)  glDeleteProgram(m_panelShader);
    doneCurrent();
}

GLuint LibraryWindow::loadTexture(const QString& path) {
    QImage img(path);
    if(img.isNull()){ qWarning()<<"LibraryWindow: cannot load"<<path; return 0; }
    img = img.mirrored().convertToFormat(QImage::Format_RGBA8888);
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,img.width(),img.height(),0,
                 GL_RGBA,GL_UNSIGNED_BYTE,img.bits());
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    return tex;
}

GLuint LibraryWindow::compileShader() {
    auto compile=[](GLenum type,const char* src)->GLuint{
        GLuint s=glCreateShader(type);
        glShaderSource(s,1,&src,nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint vs=compile(GL_VERTEX_SHADER,PANEL_VERT);
    GLuint fs=compile(GL_FRAGMENT_SHADER,PANEL_FRAG);
    GLuint prog=glCreateProgram();
    glAttachShader(prog,vs); glAttachShader(prog,fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

void LibraryWindow::initializeGL() {
    glewExperimental=GL_TRUE;
    glewInit();
    glClearColor(0,0,0,1);
    m_panelShader=compileShader();
    float verts[]={
        -1,-1,0,0,  1,-1,1,0,  1,1,1,1,
        -1,-1,0,0,  1,1,1,1,  -1,1,0,1
    };
    glGenVertexArrays(1,&m_panelVAO);
    glGenBuffers(1,&m_panelVBO);
    glBindVertexArray(m_panelVAO);
    glBindBuffer(GL_ARRAY_BUFFER,m_panelVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glBindVertexArray(0);
    QString base=qgetenv("EVOPLAYER_BASE").isEmpty() ? QCoreApplication::applicationDirPath() + "/../" : QString::fromLocal8Bit(qgetenv("EVOPLAYER_BASE")) + "/";
    m_panelTexture=loadTexture(base+"assets/lib_panel.png");
    m_libScreen.init(m_panelShader);
    m_vizScreen.init();
    connect(&m_videoScreen,&VideoScreen::frameReady,this,[this](){ update(); });
    connect(&m_videoScreen,&VideoScreen::videoInfoReady,this,[this](QString t,QString d,double fps,int w,int h,QString vc,QString ac,int sr,int br){
        m_libScreen.setVideoInfo(t,d,fps,w,h,vc,ac,sr,br);
    });
    connect(&m_timer,&QTimer::timeout,this,[this](){
        if(m_audio){
            QStringList pl=m_audio->playlist();
            if(pl!=m_lastPlaylist){
                m_lastPlaylist=pl;
                QStringList names;
                for(auto& p:pl){
                    QFileInfo fi(p);
                    names<<fi.completeBaseName();
                }
                m_libScreen.setTrackList(names);
            }
            int idx=m_audio->currentIndex();
            if(idx>=0&&idx<pl.size()){
                QFileInfo fi(pl[idx]);
                m_libScreen.setCurrentTrack(fi.completeBaseName(),"");
                m_libScreen.setSelectedIndex(idx);
            }
        }
        update();
    });
    m_timer.start(16);
    m_glReady=true;
}

void LibraryWindow::resizeGL(int w,int h){ glViewport(0,0,w,h); }

void LibraryWindow::drawPanel() {
    glUseProgram(m_panelShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,m_panelTexture);
    glUniform1i(glGetUniformLocation(m_panelShader,"uTex"),0);
    glBindVertexArray(m_panelVAO);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
    glUseProgram(0);
}

void LibraryWindow::paintGL() {
    if(!m_glReady) return;
    glClear(GL_COLOR_BUFFER_BIT);
    drawPanel();
    // Schermo 1 — Libreria: X=65 Y=55 W=593 H=398
    m_libScreen.render(width(),height(),m_panelW,m_panelH, 65,55,593,398);
    // Schermo 3 — Visualizzatore: X=1373 Y=56 W=591 H=396
    m_vizScreen.render(width(),height(),m_panelW,m_panelH, 1373,56,591,396);

}

void LibraryWindow::setAudio(QmmpBridge* audio) {
    m_audio=audio;
}
void LibraryWindow::loadVideo(const QString& path) {
    m_vizScreen.setVideoActive(true);
    m_videoScreen.loadVideo(path);
}
void LibraryWindow::videoStop() {
    m_vizScreen.setVideoActive(false);
    m_videoScreen.stop();
}
void LibraryWindow::videoPause() {
    m_videoScreen.setPaused(!m_videoScreen.isPaused());
}
bool LibraryWindow::isVideoPlaying() const {
    return m_videoScreen.isLoaded();
}

void LibraryWindow::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::RightButton) {
        QWidget* top = window();
        if (top && top->windowHandle())
            top->windowHandle()->startSystemMove();
        return;
    }
    update();
}
void LibraryWindow::mouseMoveEvent(QMouseEvent* e) {
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
        move(pos() + e->globalPos() - m_dragPos);
        m_dragPos = e->globalPos();
    }
}
void LibraryWindow::mouseReleaseEvent(QMouseEvent* e) {
    Q_UNUSED(e);
    m_dragging = false;
    SnapManager::instance()->snap(window());
}
void LibraryWindow::wheelEvent(QWheelEvent* e) {
    if(e->angleDelta().y()>0) m_libScreen.scrollUp();
    else                      m_libScreen.scrollDown();
    update();
}

void LibraryWindow::paintEvent(QPaintEvent* e) {
    QOpenGLWidget::paintEvent(e);
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    m_videoScreen.paint(p, width(), height(), m_panelW, m_panelH, 718, 54, 594, 400);
}
