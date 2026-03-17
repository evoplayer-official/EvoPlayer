#include <GL/glew.h>
#include "visualizer_screen.h"
#include <cmath>
#include <QDebug>

static const char* VIZ_VERT = R"(
#version 330 core
layout(location=0) in vec2 aPos;
out vec2 vUV;
void main(){
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* VIZ_FRAG = R"(
#version 330 core
in vec2 vUV;
out vec4 fragColor;
uniform sampler1D uFFT;
uniform float uTime;
uniform float uBass;
uniform float uMid;
uniform float uHigh;

#define PI 3.14159265359
#define TAU 6.28318530718

vec3 palette(float t){
    vec3 a=vec3(0.5,0.5,0.5);
    vec3 b=vec3(0.5,0.5,0.5);
    vec3 c=vec3(1.0,1.0,1.0);
    vec3 d=vec3(0.0,0.33,0.67);
    return a + b*cos(TAU*(c*t+d));
}

void main(){
    vec2 uv = vUV * 2.0 - 1.0;
    uv.x *= 1.5;
    vec3 col = vec3(0.0);
    float bass  = uBass  * 1.5;
    float mid   = uMid   * 1.2;
    float high  = uHigh  * 1.0;

    // === CERCHIO RADIALE FFT ===
    float angle = atan(uv.y, uv.x);
    float radius = length(uv);
    float norm = (angle + PI) / TAU;
    float fftVal = texture(uFFT, norm).r;
    float ring = 0.35 + fftVal * 0.25 * (1.0 + bass);
    float ringW = 0.04 + fftVal * 0.02;
    float ringMask = smoothstep(ringW, 0.0, abs(radius - ring));
    vec3 ringCol = palette(norm + uTime * 0.1) * (1.5 + bass);
    col += ringCol * ringMask * 2.0;

    // === BARRE FFT RADIALI ===
    float barAngle = floor(norm * 64.0) / 64.0;
    float barFFT = texture(uFFT, barAngle).r;
    float barLen = 0.05 + barFFT * 0.4 * (1.0 + mid * 0.5);
    float barR1 = ring + 0.02;
    float barR2 = ring + barLen;
    float inBar = step(barR1, radius) * step(radius, barR2);
    float angDiff = abs(mod(norm * 64.0, 1.0) - 0.5);
    float barMask = step(angDiff, 0.35) * inBar;
    vec3 barCol = palette(barAngle + uTime * 0.05 + barFFT * 0.3);
    col += barCol * barMask * (1.5 + high);

    // === ONDE PLASMA ===
    float plasma = sin(uv.x * 3.0 + uTime * 0.8 + bass * 2.0)
                 + sin(uv.y * 2.5 + uTime * 0.6)
                 + sin((uv.x + uv.y) * 2.0 + uTime * 0.4 + mid)
                 + sin(length(uv) * 4.0 - uTime * 1.2);
    plasma = plasma * 0.25 + 0.5;
    vec3 plasmaCol = palette(plasma + uTime * 0.02) * 0.15 * (0.5 + mid);
    col += plasmaCol * (1.0 - ringMask);

    // === GLOW CENTRALE ===
    float glow = exp(-radius * (3.0 - bass * 1.5)) * bass * 0.8;
    col += vec3(0.4, 0.7, 1.0) * glow;

    // === STELLA CENTRALE ===
    float star = max(0.0, 1.0 - abs(uv.x) * 8.0) * max(0.0, 1.0 - abs(uv.y) * 8.0);
    star += max(0.0, 1.0 - abs(uv.x + uv.y) * 6.0) * max(0.0, 1.0 - abs(uv.x - uv.y) * 6.0);
    col += vec3(0.6, 0.9, 1.0) * star * bass * 0.5;

    // Vignette
    float vig = 1.0 - smoothstep(0.6, 1.2, length(uv));
    col *= vig;

    // Tone mapping
    col = col / (col + 0.8);
    col = pow(col, vec3(0.85));

    fragColor = vec4(col, 1.0);
}
)";


static const char* VU_FRAG = R"(
#version 330 core
in vec2 vUV;
out vec4 fragColor;
uniform float uBands[8];

void main(){
    vec2 uv = vUV;
    int bar = int(uv.x * 8.0);
    float barX = fract(uv.x * 8.0);
    // Gap tra barre
    if(barX < 0.08 || barX > 0.92){
        fragColor = vec4(0.0,0.0,0.0,1.0); return;
    }
    float level = uBands[bar];
    float lit = step(uv.y, level);
    // Colore LED azzurro ghiaccio
    vec3 ledOn  = vec3(0.84, 0.93, 1.0) * (0.6 + level * 0.4);
    vec3 ledOff = vec3(0.03, 0.05, 0.08);
    // Segmenti
    float seg = fract(uv.y * 20.0);
    float segMask = step(0.1, seg) * step(seg, 0.9);
    vec3 col = mix(ledOff, ledOn, lit) * segMask;
    // Peak bianco
    float peak = step(abs(uv.y - level), 0.025) * step(0.025, level);
    col += vec3(1.0) * peak * segMask;
    fragColor = vec4(col, 1.0);
}
)";

VisualizerScreen::VisualizerScreen(QWidget* parent) : Visual(parent) {}

VisualizerScreen::~VisualizerScreen() {
    Visual::remove(this);
    if(m_fftTex) glDeleteTextures(1,&m_fftTex);
    if(m_vao)    glDeleteVertexArrays(1,&m_vao);
    if(m_vbo)    glDeleteBuffers(1,&m_vbo);
    if(m_shader) glDeleteProgram(m_shader);
}

GLuint VisualizerScreen::compileShader() {
    auto comp=[](GLenum t,const char* s)->GLuint{
        GLuint id=glCreateShader(t);
        glShaderSource(id,1,&s,nullptr);
        glCompileShader(id);
        GLint ok=0; glGetShaderiv(id,GL_COMPILE_STATUS,&ok);
        if(!ok){ char log[512]; glGetShaderInfoLog(id,512,nullptr,log); qWarning()<<"Shader:"<<log; }
        return id;
    };
    GLuint vs=comp(GL_VERTEX_SHADER,VIZ_VERT);
    GLuint fs=comp(GL_FRAGMENT_SHADER,VIZ_FRAG);
    GLuint p=glCreateProgram();
    glAttachShader(p,vs); glAttachShader(p,fs);
    glLinkProgram(p);
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

void VisualizerScreen::init() {
    m_shader=compileShader();
    // Shader oscilloscopio per modalità video
    auto compOsc=[](const char* vs, const char* fs)->GLuint{
        auto comp=[](GLenum t,const char* s)->GLuint{
            GLuint id=glCreateShader(t); glShaderSource(id,1,&s,nullptr); glCompileShader(id); return id;
        };
        GLuint v=comp(GL_VERTEX_SHADER,vs), f=comp(GL_FRAGMENT_SHADER,fs);
        GLuint p=glCreateProgram(); glAttachShader(p,v); glAttachShader(p,f);
        glLinkProgram(p); glDeleteShader(v); glDeleteShader(f); return p;
    };
    m_shaderVU=compOsc(VIZ_VERT, VU_FRAG);
    float quad[]={-1,-1, 1,-1, 1,1, -1,-1, 1,1, -1,1};
    glGenVertexArrays(1,&m_vao);
    glGenBuffers(1,&m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER,m_vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
    glBindVertexArray(0);
    // Texture 1D per FFT
    glGenTextures(1,&m_fftTex);
    glBindTexture(GL_TEXTURE_1D,m_fftTex);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    float zeros[256]={};
    glTexImage1D(GL_TEXTURE_1D,0,GL_RED,256,0,GL_RED,GL_FLOAT,zeros);
    glBindTexture(GL_TEXTURE_1D,0);
    Visual::add(this);
    m_ready=true;
    qDebug()<<"VisualizerScreen: ready";
}

void VisualizerScreen::updateFFT(float* pcmL, float* pcmR) {
    const int BANDS=256;
    float bands[BANDS]={};
    // FFT semplificata — media RMS per banda
    for(int b=0;b<BANDS;b++){
        int start=b*512/BANDS/2;
        int end=start+512/BANDS/2;
        if(end>256) end=256;
        float sum=0;
        for(int i=start;i<end;i++){
            float avg=(pcmL[i]+pcmR[i])*0.5f;
            sum+=avg*avg;
        }
        bands[b]=sqrtf(sum/(end-start+1))*8.0f;
        if(bands[b]>1.0f) bands[b]=1.0f;
    }
    for(int b=0;b<BANDS;b++){
        if(bands[b]>m_smooth[b]) m_smooth[b]+=(bands[b]-m_smooth[b])*0.6f;
        else m_smooth[b]-=0.03f;
        if(m_smooth[b]<0) m_smooth[b]=0;
        if(bands[b]>m_peak[b]) m_peak[b]=bands[b];
        else m_peak[b]-=0.003f;
        if(m_peak[b]<0) m_peak[b]=0;
    }
    glBindTexture(GL_TEXTURE_1D,m_fftTex);
    glTexSubImage1D(GL_TEXTURE_1D,0,0,BANDS,GL_RED,GL_FLOAT,m_smooth);
    glBindTexture(GL_TEXTURE_1D,0);
}

void VisualizerScreen::render(int winW, int winH, int panelW, int panelH,
                               int sx, int sy, int sw, int sh) {
    if(!m_ready) return;
    float pcmL[512]={}, pcmR[512]={};
    bool hasData=takeData(pcmL,pcmR);
    if(hasData){
        if(m_videoActive){
            // 8 bande RMS — zero FFT
            for(int b=0;b<8;b++){
                int s=b*32, e=s+32;
                float sum=0;
                for(int i=s;i<e;i++) sum+=(pcmL[i]+pcmR[i])*(pcmL[i]+pcmR[i])*0.25f;
                float rms=sqrtf(sum/32.0f)*4.0f;
                if(rms>1.0f) rms=1.0f;
                if(rms>m_vuBands[b]) m_vuBands[b]=rms;
                else m_vuBands[b]-=0.04f;
                if(m_vuBands[b]<0) m_vuBands[b]=0;
            }
        } else {
            updateFFT(pcmL,pcmR);
        }
    }
    m_time+=0.016f;

    // Calcola bass, mid, high
    float bass=0,mid=0,high=0;
    for(int i=0;i<32;i++)  bass+=m_smooth[i];
    for(int i=32;i<128;i++) mid+=m_smooth[i];
    for(int i=128;i<256;i++) high+=m_smooth[i];
    bass/=32.0f; mid/=96.0f; high/=128.0f;

    // Scissor + viewport
    int scX=(int)((float)sx/panelW*winW);
    int scY=(int)((float)(panelH-sy-sh)/panelH*winH);
    int scW=(int)((float)sw/panelW*winW);
    int scH=(int)((float)sh/panelH*winH);
    glEnable(GL_SCISSOR_TEST);
    glScissor(scX,scY,scW,scH);
    glViewport(scX,scY,scW,scH);

    GLuint activeShader = m_videoActive ? m_shaderVU : m_shader;
    glUseProgram(activeShader);
    if(m_videoActive){
        glUniform1fv(glGetUniformLocation(activeShader,"uBands"),8,m_vuBands);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D,m_fftTex);
        glUniform1i(glGetUniformLocation(activeShader,"uFFT"),0);
        glUniform1f(glGetUniformLocation(activeShader,"uTime"),m_time);
        glUniform1f(glGetUniformLocation(activeShader,"uBass"),bass);
        glUniform1f(glGetUniformLocation(activeShader,"uMid"),mid);
        glUniform1f(glGetUniformLocation(activeShader,"uHigh"),high);
    }
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
    glUseProgram(0);

    glViewport(0,0,winW,winH);
    glDisable(GL_SCISSOR_TEST);
}
