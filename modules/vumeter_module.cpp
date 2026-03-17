#include <GL/glew.h>
#include "vumeter_module.h"
#include <QDebug>
#include <cmath>

VuMeterModule::VuMeterModule(QWidget *parent)
    : Visual(parent)
    , m_shader(0), m_vao(0), m_vbo(0)
{
}

VuMeterModule::~VuMeterModule() {
    Visual::remove(this);
}

void VuMeterModule::setupGeometry() {
    float verts[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void VuMeterModule::init(GLuint textShader) {
    m_shader = textShader;
    setupGeometry();
    Visual::add(this);
    m_ready = true;
}

void VuMeterModule::getLevel(float &left, float &right) {
    float pcmL[512] = {}, pcmR[512] = {};
    if (!takeData(pcmL, pcmR)) { left = right = 0.0f; return; }

    float sumL = 0, sumR = 0;
    for (int i = 0; i < 512; i++) {
        sumL += pcmL[i] * pcmL[i];
        sumR += pcmR[i] * pcmR[i];
    }
    left  = sqrtf(sumL / 512.0f) * 3.0f;
    right = sqrtf(sumR / 512.0f) * 3.0f;
    if (left  > 1.0f) left  = 1.0f;
    if (right > 1.0f) right = 1.0f;
}

void VuMeterModule::drawBar(float x, float y, float w, float h,
                             float r, float g, float b)
{
    glUseProgram(m_shader);
    glUniform3f(glGetUniformLocation(m_shader, "uTextColor"), r, g, b);

    GLuint dummyTex;
    glGenTextures(1, &dummyTex);
    glBindTexture(GL_TEXTURE_2D, dummyTex);
    unsigned char white[] = {255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, white);

    float verts[] = {
        x,   y,
        x+w, y,
        x+w, y+h,
        x,   y,
        x+w, y+h,
        x,   y+h
    };
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glDeleteTextures(1, &dummyTex);
}

void VuMeterModule::render(int winW, int winH) {
    if (!m_ready) return;

    float pcmL[512] = {}, pcmR[512] = {};
    bool hasData = takeData(pcmL, pcmR);

    // FFT semplice — 32 bande di frequenza
    const int BANDS = 32;
    float bands[BANDS] = {};

    if (hasData) {
        for (int b = 0; b < BANDS; b++) {
            int start = b * (256 / BANDS);
            int end   = start + (256 / BANDS);
            float sum = 0;
            for (int i = start; i < end; i++) {
                float re = 0, im = 0;
                for (int n = 0; n < 256; n++) {
                    float avg = (pcmL[n] + pcmR[n]) * 0.5f;
                    float angle = 2.0f * 3.14159f * i * n / 256.0f;
                    re += avg * cosf(angle);
                    im += avg * sinf(angle);
                }
                sum += sqrtf(re*re + im*im) / 256.0f;
            }
            bands[b] = sum / (256 / BANDS) * 12.0f;
            if (bands[b] > 1.0f) bands[b] = 1.0f;
        }
    }

    // decay e smoothing
    static float smoothBands[BANDS] = {};
    static float peakBands[BANDS]   = {};
    for (int b = 0; b < BANDS; b++) {
        if (bands[b] > smoothBands[b])
            smoothBands[b] += (bands[b] - smoothBands[b]) * 0.7f;
        else
            smoothBands[b] -= 0.04f;
        if (smoothBands[b] < 0) smoothBands[b] = 0;

        if (bands[b] > peakBands[b]) peakBands[b] = bands[b];
        else peakBands[b] -= 0.012f;
        if (peakBands[b] < 0) peakBands[b] = 0;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // coordinate finestra 1600x423
    float x0 = (38.0f/1600.0f)*winW,  x1 = (204.0f/1600.0f)*winW;
    float y0 = (50.0f/423.0f)*winH,  y1 = (172.0f/423.0f)*winH;
    float vuW = x1 - x0;
    float vuH = y1 - y0;

    float barW   = vuW / BANDS * 0.75f;
    float barGap = vuW / BANDS * 0.25f;

    for (int b = 0; b < BANDS; b++) {
        float bx = x0 + b * (barW + barGap);
        float bh = smoothBands[b] * vuH;
        float intensity = (float)b / BANDS;

        // sfondo scuro
        drawBar(bx, y0, barW, vuH, 0.04f, 0.06f, 0.09f);

        // barra attiva — azzurro ghiaccio
        if (bh > 1.0f) {
            float r = 0.55f + intensity * 0.29f;
            float g = 0.75f + intensity * 0.18f;
            float bl = 1.0f;
            drawBar(bx, y0, barW, bh, r, g, bl);
        }

        // peak marker bianco
        float ph = peakBands[b] * vuH;
        if (ph > 2.0f)
            drawBar(bx, y0 + ph - 2.0f, barW, 2.0f, 1.0f, 1.0f, 1.0f);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
