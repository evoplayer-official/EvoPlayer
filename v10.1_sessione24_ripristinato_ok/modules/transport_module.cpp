#include <GL/glew.h>
#include <QImage>
#include "transport_module.h"
#include <QDebug>
#include <QProcess>

static const float BTN_W = 0.085f;
static const float BTN_H = 0.140f;
static const float GAP   = 0.026f;
static const float BTN_Y = -0.425f;

TransportModule::TransportModule()
    : m_vao(0), m_vbo(0), m_ebo(0), m_shader(0)
    , m_state(PlayerState::STOPPED)
{
    // Coordinate pixel centro pulsanti nel PNG (Y dal top)
    float cx[6] = {400, 526, 656, 784, 908, 1034};
    float cy[6] = {437, 437, 437, 437, 437, 437};
    float hw = 44.0f; // half-width hit zone
    float hh = 30.0f; // half-height hit zone
    for (int i = 0; i < 6; i++) {
        m_buttons[i].id = i;
        m_buttons[i].x  = cx[i];
        m_buttons[i].y  = cy[i];
        m_buttons[i].w  = hw;
        m_buttons[i].h  = hh;
    }
}

TransportModule::~TransportModule() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
}

void TransportModule::init(GLuint shaderProgram, GLuint panelShader) {
    m_shader = shaderProgram;
    m_panelShader = panelShader;
    setupGeometry();
    for (int i = 0; i < 6; i++) m_texPressed[i] = 0;

    for (int i = 0; i < 6; i++) m_texHover[i] = 0;
    QString base = (qgetenv("EVOPLAYER_BASE").isEmpty() ? QString::fromLocal8Bit(qgetenv("HOME")) + "/EvoPlayer/" : QString::fromLocal8Bit(qgetenv("EVOPLAYER_BASE")) + "/") + "assets/buttons/";
    m_texPressed[0] = loadTexture(base + "btn_rewind_pressed.png");
    m_texHover[0]   = loadTexture(base + "btn_rewind_hover.png");
    m_texPressed[1] = loadTexture(base + "btn_play_pressed.png");
    m_texHover[1]   = loadTexture(base + "btn_play_hover.png");
    m_texPressed[2] = loadTexture(base + "btn_pause_pressed.png");
    m_texHover[2]   = loadTexture(base + "btn_pause_hover.png");
    m_texPressed[3] = loadTexture(base + "btn_stop_pressed.png");
    m_texHover[3]   = loadTexture(base + "btn_stop_hover.png");
    m_texPressed[4] = loadTexture(base + "btn_forward_pressed.png");
    m_texHover[4]   = loadTexture(base + "btn_forward_hover.png");
    m_texPressed[5] = loadTexture(base + "btn_open_pressed.png");
    m_texHover[5]   = loadTexture(base + "btn_open_hover.png");
}

void TransportModule::setupGeometry() {
    float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    unsigned int idx[] = { 0,1,2, 0,2,3 };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

QVector3D TransportModule::buttonColor(const Button &btn) const {
    // LED attivo per Play (id=1) e Pause (id=2)
    if (btn.id == 1 && m_state == PlayerState::PLAYING)
        return QVector3D(0.84f, 0.93f, 1.0f);
    if (btn.id == 2 && m_state == PlayerState::PAUSED)
        return QVector3D(0.84f, 0.93f, 1.0f);

    float base = btn.hovered ? 0.45f : 0.30f;
    float flash = btn.pressAnim * 0.6f;
    return QVector3D(base + flash, base + flash, base + flash + 0.05f);
}

void TransportModule::drawButton(const Button &btn,
    const QMatrix4x4 &view, const QMatrix4x4 &proj)
{
    QMatrix4x4 model;
    model.translate(btn.x, btn.y, 0.0f);
    model.scale(btn.w, btn.h, 1.0f);

    glUniformMatrix4fv(glGetUniformLocation(m_shader, "uModel"),
        1, GL_FALSE, model.constData());
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "uView"),
        1, GL_FALSE, view.constData());
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "uProjection"),
        1, GL_FALSE, proj.constData());

    QVector3D col = buttonColor(btn);
    glUniform3f(glGetUniformLocation(m_shader, "uColor"),
        col.x(), col.y(), col.z());
    glUniform1f(glGetUniformLocation(m_shader, "uEmission"),
        btn.pressAnim);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void TransportModule::render(const QMatrix4x4 &view, const QMatrix4x4 &proj) {
    for (int i = 0; i < 6; i++) {
        if (m_buttons[i].pressAnim > 0.01f)
            drawOverlay(i, m_texPressed[i]);
        else if (i == 1 && m_state == PlayerState::PLAYING && m_texHover[1]) {
            drawOverlay(i, m_texHover[i]);
            drawOverlay(i, m_texHover[i]);
        }
        else if (i == 2 && m_state == PlayerState::PAUSED && m_texHover[2]) {
            drawOverlay(i, m_texHover[i]);
            drawOverlay(i, m_texHover[i]);
        }
        else if (i == m_lastPressed && m_texHover[i]) {
            drawOverlay(i, m_texHover[i]);
            drawOverlay(i, m_texHover[i]);
        }
        else if (m_buttons[i].hovered && m_texHover[i])
            drawOverlay(i, m_texHover[i]);
    }
}

void TransportModule::handleMouseMove(float nx, float ny) {
    // nx, ny sono coordinate pixel dirette (Y dall alto)
    for (int i = 0; i < 6; i++) {
        Button &b = m_buttons[i];
        b.hovered = (nx >= b.x - b.w && nx <= b.x + b.w &&
                     ny >= b.y - b.h && ny <= b.y + b.h);
    }
}

void TransportModule::handleMousePress(float nx, float ny) {
    for (int i = 0; i < 6; i++) {
        Button &b = m_buttons[i];
        if (nx >= b.x - b.w && nx <= b.x + b.w &&
            ny >= b.y - b.h && ny <= b.y + b.h)
        {
            b.pressAnim = 1.0f;
            QProcess::startDetached("paplay", {(qgetenv("EVOPLAYER_BASE").isEmpty() ? QString::fromLocal8Bit(qgetenv("HOME")) + "/EvoPlayer/" : QString::fromLocal8Bit(qgetenv("EVOPLAYER_BASE")) + "/") + "assets/click.wav"});
            switch (i) {
                case 0: if (onPrev) onPrev(); break;
                case 1: if (onPlay) onPlay(); break;
                case 2: if (onPause) onPause(); break;
                case 3: if (onStop) onStop(); break;
                case 4: if (onNext) onNext(); break;
                case 5: if (onOpen) onOpen(); break;
            }
        }
    }
}

void TransportModule::update(float dt) {
    for (int i = 0; i < 6; i++) {
        if (m_buttons[i].pressAnim > 0.0f)
            m_buttons[i].pressAnim -= dt * 1.0f;
        if (m_buttons[i].pressAnim < 0.0f)
            m_buttons[i].pressAnim = 0.0f;
    }
}

void TransportModule::setPlayerState(PlayerState state) {
    m_state = state;
}

GLuint TransportModule::loadTexture(const QString &path) {
    QImage img(path);
    if (img.isNull()) { qWarning() << "Texture non trovata:" << path; return 0; }
    img = img.convertToFormat(QImage::Format_RGBA8888).mirrored();
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(),
        0, GL_RGBA, GL_UNSIGNED_BYTE, img.constBits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}



void TransportModule::drawOverlay(int btnIdx, GLuint tex) {
    if (!tex) return;
    const Button &btn = m_buttons[btnIdx];
    // Converti coordinate pixel plancia (2029x508) in NDC
    float x0 = ((btn.x - btn.w) / 2030.0f) * 2.0f - 1.0f;
    float x1 = ((btn.x + btn.w) / 2030.0f) * 2.0f - 1.0f;
    float y0 = 1.0f - ((btn.y + btn.h) / 537.0f) * 2.0f;
    float y1 = 1.0f - ((btn.y - btn.h) / 537.0f) * 2.0f;
    float verts[] = {
        x0, y0, 0.0f, 0.0f,
        x1, y0, 1.0f, 0.0f,
        x1, y1, 1.0f, 1.0f,
        x0, y1, 0.0f, 1.0f
    };
    unsigned int idx[] = { 0,1,2, 0,2,3 };
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glUseProgram(m_panelShader);
    glUniform1i(glGetUniformLocation(m_panelShader, "uPanel"), 0);
    glUniform1f(glGetUniformLocation(m_panelShader, "uAlpha"), 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}
