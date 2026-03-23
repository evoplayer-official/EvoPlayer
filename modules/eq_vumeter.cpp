#include <GL/glew.h>
#include "eq_vumeter.h"
#include <cmath>
#include <QDebug>

// --- FFT Cooley-Tukey in-place (radix-2, potenza di 2) ---
static void fft(float* re, float* im, int n) {
    // Bit-reversal
    for(int i=1,j=0; i<n; i++){
        int bit = n>>1;
        for(; j&bit; bit>>=1) j^=bit;
        j^=bit;
        if(i<j){ float tr=re[i];re[i]=re[j];re[j]=tr;
                  float ti=im[i];im[i]=im[j];im[j]=ti; }
    }
    // Butterfly
    for(int len=2; len<=n; len<<=1){
        float ang = -2.0f * 3.14159265f / len;
        float wRe = cosf(ang), wIm = sinf(ang);
        for(int i=0; i<n; i+=len){
            float curRe=1.0f, curIm=0.0f;
            for(int j=0; j<len/2; j++){
                float uRe=re[i+j],       uIm=im[i+j];
                float vRe=re[i+j+len/2], vIm=im[i+j+len/2];
                float tRe=curRe*vRe - curIm*vIm;
                float tIm=curRe*vIm + curIm*vRe;
                re[i+j]        = uRe+tRe; im[i+j]        = uIm+tIm;
                re[i+j+len/2]  = uRe-tRe; im[i+j+len/2]  = uIm-tIm;
                float newRe = curRe*wRe - curIm*wIm;
                curIm       = curRe*wIm + curIm*wRe;
                curRe       = newRe;
            }
        }
    }
}

EqVuMeter::EqVuMeter(QWidget* parent) : Visual(parent) {}
EqVuMeter::~EqVuMeter() { Visual::remove(this); }

void EqVuMeter::init(GLuint shader) {
    Q_UNUSED(shader);
    static const char* vs =
        "#version 330 core\n"
        "layout(location=0) in vec2 aPos;\n"
        "void main(){ gl_Position=vec4(aPos,0,1); }\n";
    static const char* fs =
        "#version 330 core\n"
        "uniform vec3 uColor;\n"
        "out vec4 fragColor;\n"
        "void main(){ fragColor=vec4(uColor,1.0); }\n";
    auto comp=[](GLenum t,const char* s)->GLuint{
        GLuint sh=glCreateShader(t);
        glShaderSource(sh,1,&s,nullptr);
        glCompileShader(sh);
        return sh;
    };
    GLuint v=comp(GL_VERTEX_SHADER,vs);
    GLuint f=comp(GL_FRAGMENT_SHADER,fs);
    m_shader=glCreateProgram();
    glAttachShader(m_shader,v); glAttachShader(m_shader,f);
    glLinkProgram(m_shader);
    glDeleteShader(v); glDeleteShader(f);
    float verts[]={0,0, 1,0, 1,1, 0,0, 1,1, 0,1};
    glGenVertexArrays(1,&m_vao);
    glGenBuffers(1,&m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER,m_vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
    glBindVertexArray(0);
    Visual::add(this);
    m_ready=true;
    qDebug()<<"EqVuMeter: ready (FFT spectrum mode)";
}

void EqVuMeter::drawBar(float x, float y, float w, float h, float r, float g, float b) {
    glUseProgram(m_shader);
    glUniform3f(glGetUniformLocation(m_shader,"uColor"), r, g, b);
    float verts[]={x,y, x+w,y, x+w,y+h, x,y, x+w,y+h, x,y+h};
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER,m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(verts),verts);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
}

void EqVuMeter::render(int winW, int winH, int panelW, int panelH,
                        float dispX, float dispY, float dispW, float dispH,
                        float* sliderValues) {
    if(!m_ready) return;

    // --- Acquisisci PCM da libqmmp ---
    float pcmL[512]={}, pcmR[512]={};
    bool hasData = takeData(pcmL, pcmR);

    const int FFT_SIZE = 512;
    float bands[BANDS]={};

    if(hasData) {
        // --- Finestra di Hann sul segnale mono ---
        float re[FFT_SIZE]={}, im[FFT_SIZE]={};
        for(int i=0; i<FFT_SIZE; i++){
            float hann = 0.5f * (1.0f - cosf(2.0f*3.14159265f*i/(FFT_SIZE-1)));
            re[i] = (pcmL[i] + pcmR[i]) * 0.5f * hann;
            im[i] = 0.0f;
        }

        // --- FFT reale ---
        fft(re, im, FFT_SIZE);

        // --- Magnitudini (solo metà spettro utile) ---
        const int SPEC = FFT_SIZE/2;
        float mag[SPEC]={};
        for(int i=1; i<SPEC; i++){
            mag[i] = sqrtf(re[i]*re[i] + im[i]*im[i]) / (FFT_SIZE/2);
        }

        // --- Mappa su BANDS bande con scala LOGARITMICA ---
        // Frequenze: ~20Hz → ~20kHz, sample rate 44100Hz
        float freqMin = 1.0f;   // bin minimo (evita DC)
        float freqMax = (float)(SPEC - 1);
        float logMin  = log10f(freqMin + 1.0f);
        float logMax  = log10f(freqMax + 1.0f);

        for(int b=0; b<BANDS; b++){
            float t0 = (float)b     / BANDS;
            float t1 = (float)(b+1) / BANDS;
            int bin0 = (int)(powf(10.0f, logMin + t0*(logMax-logMin)) - 1.0f);
            int bin1 = (int)(powf(10.0f, logMin + t1*(logMax-logMin)) - 1.0f);
            if(bin0 < 1)    bin0 = 1;
            if(bin1 >= SPEC) bin1 = SPEC-1;
            if(bin1 <= bin0) bin1 = bin0+1;

            float sum = 0;
            int   cnt = 0;
            for(int i=bin0; i<=bin1 && i<SPEC; i++){
                sum += mag[i];
                cnt++;
            }
            bands[b] = (cnt > 0) ? sum/cnt * 6.0f : 0.0f;
            if(bands[b] > 1.0f) bands[b] = 1.0f;
        }
    }

    // --- Applica guadagno slider (10 gruppi) ---
    if(sliderValues) {
        for(int b=0; b<BANDS; b++){
            int idx = (int)(b * 10.0f / BANDS);
            if(idx > 9) idx = 9;
            float gain = (sliderValues[idx] - 0.5f) * 2.0f;
            bands[b] *= (1.0f + gain * 1.5f);
            if(bands[b] > 1.0f) bands[b] = 1.0f;
            if(bands[b] < 0.0f) bands[b] = 0.0f;
        }
    }

    // --- Smoothing reattivo + peak ---
    for(int b=0; b<BANDS; b++){
        // Salita rapida, discesa più naturale
        if(bands[b] > m_smooth[b])
            m_smooth[b] += (bands[b] - m_smooth[b]) * 0.7f;
        else
            m_smooth[b] -= 0.018f;
        if(m_smooth[b] < 0) m_smooth[b] = 0;

        if(bands[b] > m_peak[b])   m_peak[b] = bands[b];
        else                        m_peak[b] -= 0.005f;
        if(m_peak[b] < 0) m_peak[b] = 0;
    }

    // --- Setup OpenGL ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // --- Pixel → NDC ---
    float px0 = (dispX / panelW) * winW;
    float py0 = (dispY / panelH) * winH;
    float px1 = ((dispX+dispW) / panelW) * winW;
    float py1 = ((dispY+dispH) / panelH) * winH;
    float x0  = (px0/winW)*2.0f - 1.0f;
    float x1  = (px1/winW)*2.0f - 1.0f;
    float y0  = 1.0f - (py1/winH)*2.0f;
    float y1  = 1.0f - (py0/winH)*2.0f;
    float vuW = x1 - x0;
    float vuH = y1 - y0;

    float barW   = vuW / BANDS * 0.78f;
    float barGap = vuW / BANDS * 0.22f;

    const int SEGS  = 40;
    float segH    = vuH / SEGS;
    float segDraw = segH * 0.72f;

    // Azzurro ghiaccio #D6EEFF
    const float LED_R = 0.839f;
    const float LED_G = 0.933f;
    const float LED_B = 1.0f;

    for(int b=0; b<BANDS; b++){
        float bx = x0 + b*(barW+barGap);

        // Sfondo spento
        for(int s=0; s<SEGS; s++)
            drawBar(bx, y0+s*segH, barW, segDraw, 0.04f, 0.06f, 0.09f);

        // Barre accese — intensità crescente verso l'alto
        int activeSeg = (int)(m_smooth[b] * SEGS);
        if(activeSeg > SEGS) activeSeg = SEGS;
        for(int s=0; s<activeSeg; s++){
            float intensity = 0.5f + 0.5f*(float)s/SEGS;
            drawBar(bx, y0+s*segH, barW, segDraw,
                    LED_R*intensity, LED_G*intensity, LED_B*intensity);
        }

        // Peak marker bianco
        int peakSeg = (int)(m_peak[b] * SEGS);
        if(peakSeg >= SEGS) peakSeg = SEGS-1;
        if(peakSeg > 0)
            drawBar(bx, y0+peakSeg*segH, barW, segDraw, 1.0f, 1.0f, 1.0f);
    }
    glDisable(GL_BLEND);
}
