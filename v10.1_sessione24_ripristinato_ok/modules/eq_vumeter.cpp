#include <GL/glew.h>
#include "eq_vumeter.h"
#include <cmath>
#include <QDebug>

EqVuMeter::EqVuMeter(QWidget* parent) : Visual(parent) {}
EqVuMeter::~EqVuMeter() { Visual::remove(this); }

void EqVuMeter::init(GLuint shader) {
    Q_UNUSED(shader);
    // Compila shader flat dedicato
    static const char* vs = "#version 330 core\nlayout(location=0) in vec2 aPos;\nvoid main(){ gl_Position=vec4(aPos,0,1); }\n";
    static const char* fs = "#version 330 core\nuniform vec3 uTextColor;\nout vec4 fragColor;\nvoid main(){ fragColor=vec4(uTextColor,1.0); }\n";
    auto comp=[](GLenum t,const char* s)->GLuint{ GLuint sh=glCreateShader(t); glShaderSource(sh,1,&s,nullptr); glCompileShader(sh); return sh; };
    GLuint v=comp(GL_VERTEX_SHADER,vs), f=comp(GL_FRAGMENT_SHADER,fs);
    m_shader=glCreateProgram();
    glAttachShader(m_shader,v); glAttachShader(m_shader,f);
    glLinkProgram(m_shader);
    glDeleteShader(v); glDeleteShader(f);
    float verts[] = { 0,0, 1,0, 1,1, 0,0, 1,1, 0,1 };
    glGenVertexArrays(1,&m_vao);
    glGenBuffers(1,&m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER,m_vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
    glBindVertexArray(0);
    Visual::add(this);
    m_ready = true;
}

void EqVuMeter::drawBar(float x, float y, float w, float h, float r, float g, float b) {
    glUseProgram(m_shader);
    glUniform3f(glGetUniformLocation(m_shader,"uTextColor"), r, g, b);
    GLuint dummyTex;
    glGenTextures(1,&dummyTex);
    glBindTexture(GL_TEXTURE_2D,dummyTex);
    unsigned char white[] = {255};
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,GL_RED,GL_UNSIGNED_BYTE,white);
    float verts[] = { x,y, x+w,y, x+w,y+h, x,y, x+w,y+h, x,y+h };
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER,m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(verts),verts);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
    glDeleteTextures(1,&dummyTex);
}

void EqVuMeter::render(int winW, int winH, int panelW, int panelH,
                        float dispX, float dispY, float dispW, float dispH,
                        float* sliderValues) {
    if(!m_ready) return;
    float pcmL[512]={}, pcmR[512]={};
    bool hasData = takeData(pcmL, pcmR);

    const int BANDS = 64;
    float bands[BANDS] = {};
    if(hasData) {
        for(int b = 0; b < BANDS; b++) {
            int start = b * (256/BANDS);
            int end   = start + (256/BANDS);
            float sum = 0;
            for(int i = start; i < end; i++) {
                float re=0, im=0;
                for(int n=0; n<256; n++) {
                    float avg = (pcmL[n]+pcmR[n])*0.5f;
                    float angle = 2.0f*3.14159f*i*n/256.0f;
                    re += avg*cosf(angle);
                    im += avg*sinf(angle);
                }
                sum += sqrtf(re*re+im*im)/256.0f;
            }
            bands[b] = sum/(256/BANDS)*12.0f;
            if(bands[b]>1.0f) bands[b]=1.0f;
        }
    }

    // Applica guadagno slider — 10 gruppi di 6-7 bande ciascuno
    if(sliderValues) {
        for(int b=0; b<BANDS; b++) {
            int sliderIdx = (int)(b * 10.0f / BANDS);
            if(sliderIdx > 9) sliderIdx = 9;
            float gain = (sliderValues[sliderIdx] - 0.5f) * 2.0f; // -1 a +1
            bands[b] *= (1.0f + gain * 1.5f);
            if(bands[b] > 1.0f) bands[b] = 1.0f;
            if(bands[b] < 0.0f) bands[b] = 0.0f;
        }
    }

    static float smoothBands[BANDS] = {};
    static float peakBands[BANDS]   = {};
    for(int b=0; b<BANDS; b++) {
        if(bands[b] > smoothBands[b])
            smoothBands[b] += (bands[b]-smoothBands[b])*0.7f;
        else
            smoothBands[b] -= 0.04f;
        if(smoothBands[b]<0) smoothBands[b]=0;
        if(bands[b]>peakBands[b]) peakBands[b]=bands[b];
        else peakBands[b] -= 0.004f;
        if(peakBands[b]<0) peakBands[b]=0;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // Coordinate pixel → NDC
    float px0 = (dispX/panelW)*winW;
    float py0 = (dispY/panelH)*winH;
    float px1 = ((dispX+dispW)/panelW)*winW;
    float py1 = ((dispY+dispH)/panelH)*winH;
    float x0 = (px0/winW)*2.0f-1.0f;
    float x1 = (px1/winW)*2.0f-1.0f;
    float y0 = 1.0f-(py1/winH)*2.0f;
    float y1 = 1.0f-(py0/winH)*2.0f;
    float vuW = x1-x0;
    float vuH = y1-y0;

    float barW   = vuW/BANDS*0.78f;
    float barGap = vuW/BANDS*0.22f;

    const int SEGS = 40;
    float segH    = vuH / SEGS;
    float segDraw = segH * 0.72f;

    for(int b=0; b<BANDS; b++) {
        float bx = x0 + b*(barW+barGap);
        float t  = (float)b/BANDS;
        float r  = 0.85f + t*0.15f;
        float g  = 0.92f + t*0.08f;
        float bl = 1.0f;

        // Sfondo segmenti spenti
        for(int s=0; s<SEGS; s++)
            drawBar(bx, y0+s*segH, barW, segDraw, 0.04f, 0.06f, 0.09f);

        // Segmenti accesi
        int activeSeg = (int)(smoothBands[b]*SEGS);
        if(activeSeg > SEGS) activeSeg = SEGS;
        for(int s=0; s<activeSeg; s++) {
            float intensity = 0.6f + 0.4f*(float)s/SEGS;
            drawBar(bx, y0+s*segH, barW, segDraw, r*intensity, g*intensity, bl*intensity);
        }

        // Peak marker bianco
        int peakSeg = (int)(peakBands[b]*SEGS);
        if(peakSeg >= SEGS) peakSeg = SEGS-1;
        if(peakSeg > 0)
            drawBar(bx, y0+peakSeg*segH, barW, segDraw, 1.0f, 1.0f, 1.0f);
    }
    glDisable(GL_BLEND);
}
