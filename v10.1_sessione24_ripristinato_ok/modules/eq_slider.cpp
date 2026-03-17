#include <GL/glew.h>
#include "eq_slider.h"
#include <QImage>
#include <QtMath>
#include <QDebug>

static const char* VERT = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
uniform mat4 uTransform;
out vec2 vUV;
void main(){ vUV=aUV; gl_Position=uTransform*vec4(aPos,0,1); }
)";
static const char* FRAG = R"(
#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
out vec4 fragColor;
void main(){ fragColor=texture(uTex,vUV); }
)";

EqSlider::EqSlider(QObject* parent) : QObject(parent) {}
EqSlider::~EqSlider() {
    if(m_vao) glDeleteVertexArrays(1,&m_vao);
    if(m_vbo) glDeleteBuffers(1,&m_vbo);
    if(m_tex) glDeleteTextures(1,&m_tex);
    if(m_shader) glDeleteProgram(m_shader);
}

GLuint EqSlider::loadTexture(const QString& path) {
    QImage img(path);
    if(img.isNull()){ qWarning()<<"EqSlider: cannot load"<<path; return 0; }
    img = img.mirrored().convertToFormat(QImage::Format_RGBA8888);
    GLuint tex;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,img.width(),img.height(),0,GL_RGBA,GL_UNSIGNED_BYTE,img.bits());
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    return tex;
}

GLuint EqSlider::compileShader() {
    auto compile=[](GLenum type,const char* src)->GLuint{
        GLuint s=glCreateShader(type);
        glShaderSource(s,1,&src,nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint vs=compile(GL_VERTEX_SHADER,VERT);
    GLuint fs=compile(GL_FRAGMENT_SHADER,FRAG);
    GLuint prog=glCreateProgram();
    glAttachShader(prog,vs); glAttachShader(prog,fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

void EqSlider::init(const QString& texPath, float panelX, float trackY, float trackH, float trackW) {
    m_panelX = panelX;
    m_trackY = trackY;
    m_trackH = trackH;
    m_trackW = trackW;
    m_tex    = loadTexture(texPath);
    m_shader = compileShader();
    float h=0.5f;
    float verts[]={
        -h,-h,0,0, h,-h,1,0, h,h,1,1,
        -h,-h,0,0, h,h,1,1, -h,h,0,1
    };
    glGenVertexArrays(1,&m_vao);
    glGenBuffers(1,&m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER,m_vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glBindVertexArray(0);
}

void EqSlider::render(int winW, int winH, int panelW, int panelH) {
    if(!m_vao) return;
    float cx   = (m_panelX / panelW) * winW;
    float topY = (m_trackY / panelH) * winH;
    float botY = ((m_trackY + m_trackH) / panelH) * winH;
    float szW  = (m_trackW / panelW) * winW * 0.85f;
    float szH  = szW * 0.55f;
    float halfH = szH * 0.5f;
    float cyRaw = botY - m_value * (botY - topY);
    cyRaw = qBound(topY + halfH, cyRaw, botY - halfH);
    float cy = (1.0f - cyRaw / winH) * winH;
    float sx = szW / winW * 2.0f;
    float sy = szH / winH * 2.0f;
    float tx = (cx / winW) * 2.0f - 1.0f;
    float ty = (cy / winH) * 2.0f - 1.0f;
    float mat[16]={sx,0,0,0, 0,sy,0,0, 0,0,1,0, tx,ty,0,1};
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(m_shader);
    glUniformMatrix4fv(glGetUniformLocation(m_shader,"uTransform"),1,GL_FALSE,mat);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,m_tex);
    glUniform1i(glGetUniformLocation(m_shader,"uTex"),0);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_BLEND);
}

void EqSlider::mousePressEvent(int x, int y, int winW, int winH, int panelW, int panelH) {
    float cx  = (m_panelX / panelW) * winW;
    float topY   = (m_trackY / panelH) * winH;
    float botY   = ((m_trackY + m_trackH) / panelH) * winH;
    float hw = (m_trackW / panelW) * winW * 0.5f;
    if(x >= cx-hw && x <= cx+hw && y >= topY && y <= botY) {
        m_dragging = true;
        float v = 1.0f - (float)(y - topY) / (botY - topY);
        m_value = qBound(0.0f, v, 1.0f);
        emit valueChanged(m_value);
    }
}

void EqSlider::mouseMoveEvent(int x, int y, int winW, int winH, int panelW, int panelH) {
    if(!m_dragging) return;
    float topY = (m_trackY / panelH) * winH;
    float botY = ((m_trackY + m_trackH) / panelH) * winH;
    float v = 1.0f - (float)(y - topY) / (botY - topY);
    m_value = qBound(0.0f, v, 1.0f);
    emit valueChanged(m_value);
}

void EqSlider::mouseReleaseEvent() {
    m_dragging = false;
}
