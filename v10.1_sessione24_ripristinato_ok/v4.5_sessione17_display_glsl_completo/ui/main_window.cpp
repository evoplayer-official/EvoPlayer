#include <GL/glew.h>
#include "main_window.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFileDialog>
#include <QFile>
#include <QDebug>
#include <QMatrix4x4>
#include <QImage>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    GLWidget *gl = new GLWidget(this);
    setCentralWidget(gl);
    gl->setFocus();
}

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_shader(0), m_textShader(0)
    , m_quadVAO(0), m_quadVBO(0), m_quadEBO(0)
    , m_panelTexture(0), m_panelShader(0)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    m_audio = new QmmpBridge(this);
    connect(&m_timer, &QTimer::timeout, this, [this](){
        m_camera.update();
        float dt = m_elapsed.elapsed() / 1000.0f;
        m_elapsed.restart();
        m_transport.update(dt);
        update();
    });
}

GLWidget::~GLWidget() {
    makeCurrent();
    if (m_quadVAO) glDeleteVertexArrays(1, &m_quadVAO);
    if (m_quadVBO) glDeleteBuffers(1, &m_quadVBO);
    if (m_quadEBO) glDeleteBuffers(1, &m_quadEBO);
    doneCurrent();
}

GLuint GLWidget::loadShaders(const QString &vertPath, const QString &fragPath) {
    auto readFile = [](const QString &path) -> QByteArray {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << "Shader non trovato:" << path;
            return {};
        }
        return f.readAll();
    };
    QByteArray vertSrc = readFile(vertPath);
    QByteArray fragSrc = readFile(fragPath);
    if (vertSrc.isEmpty() || fragSrc.isEmpty()) return 0;

    auto compile = [this](GLenum type, const QByteArray &src) -> GLuint {
        GLuint s = glCreateShader(type);
        const char *c = src.constData();
        glShaderSource(s, 1, &c, nullptr);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
            qWarning() << "Shader error:" << log;
        }
        return s;
    };

    GLuint vs = compile(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void GLWidget::setupQuad() {
    float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    unsigned int idx[] = { 0,1,2, 0,2,3 };
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glGenBuffers(1, &m_quadEBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}


GLuint GLWidget::loadTexture(const QString &path) {
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

void GLWidget::setupPanel() {
    float verts[] = {
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };
    unsigned int idx[] = { 0,1,2, 0,2,3 };
    glGenVertexArrays(1, &m_panelVAO);
    glGenBuffers(1, &m_panelVBO);
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindVertexArray(m_panelVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_panelVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void GLWidget::drawPanel() {
    glUseProgram(m_panelShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_panelTexture);
    glUniform1i(glGetUniformLocation(m_panelShader, "uPanel"), 0);
    glUniform1f(glGetUniformLocation(m_panelShader, "uAlpha"), 1.0f);
    glBindVertexArray(m_panelVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
void GLWidget::initializeGL() {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK)
        qWarning() << "GLEW:" << (const char*)glewGetErrorString(err);

    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.07f, 1.0f);

    QString base = QString::fromLocal8Bit(qgetenv("HOME")) + "/EvoPlayer/";
    m_shader     = loadShaders(base+"shaders/vertex.glsl",
                               base+"shaders/fragment.glsl");
    m_textShader = loadShaders(base+"shaders/text_vertex.glsl",
                               base+"shaders/text_fragment.glsl");
    setupQuad();
    setupPanel();

    m_panelShader = loadShaders(base+"shaders/panel_vertex.glsl",
                                base+"shaders/panel_fragment.glsl");
    m_panelTexture = loadTexture(base+"assets/player_panel.png");

    bool loaded = m_model.load(base + "assets/models/player.glb");
    if (loaded) m_model.uploadToGPU();

    m_transport.init(m_shader, m_panelShader);
    m_transport.onPlay  = [this](){ m_audio->playPauseToggle(); };
    m_transport.onPause = [this](){ m_audio->playPauseToggle(); };
    m_transport.onStop  = [this](){ m_audio->stop(); };
    m_transport.onNext  = [this](){ m_audio->next(); };
    m_transport.onPrev  = [this](){ m_audio->previous(); };
    m_transport.onOpen  = [this](){
        QString dir = QFileDialog::getExistingDirectory(this, "Apri cartella musica");
        if (!dir.isEmpty()) m_audio->loadFolder(dir);
    };

    connect(m_audio, &QmmpBridge::stateChanged, this, [this](int s){
        if (s == 2) m_transport.setPlayerState(PlayerState::PLAYING);
        else if (s == 3) m_transport.setPlayerState(PlayerState::PAUSED);
        else m_transport.setPlayerState(PlayerState::STOPPED);
    });

    m_display.init(m_textShader, m_panelShader);
    m_camera.setMousePosition(0.5f, 0.5f);
    m_elapsed.start();
    m_timer.start(16);
}

void GLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    m_camera.setViewport(w, h);
}

void GLWidget::drawPlaceholder(const QMatrix4x4 &view, const QMatrix4x4 &proj,
                                float x, float y, float w, float h,
                                float r, float g, float b)
{
    glUseProgram(m_shader);
    QMatrix4x4 model;
    model.translate(x, y, 0.0f);
    model.scale(w, h, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "uModel"),
        1, GL_FALSE, model.constData());
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "uView"),
        1, GL_FALSE, view.constData());
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "uProjection"),
        1, GL_FALSE, proj.constData());
    glUniform3f(glGetUniformLocation(m_shader, "uColor"), r, g, b);
    glUniform1f(glGetUniformLocation(m_shader, "uEmission"), 0.0f);
    glBindVertexArray(m_quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void GLWidget::drawModel(const QMatrix4x4 &view, const QMatrix4x4 &proj) {
    if (m_model.meshes().isEmpty()) return;
    glUseProgram(m_shader);
    for (const MeshData &mesh : m_model.meshes()) {
        QMatrix4x4 model;
        float scale = 0.14f;
        model.scale(scale);
        model.translate(0.0f, -2.0f, 0.0f);

        glUniformMatrix4fv(glGetUniformLocation(m_shader, "uModel"),
            1, GL_FALSE, model.constData());
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "uView"),
            1, GL_FALSE, view.constData());
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "uProjection"),
            1, GL_FALSE, proj.constData());

        QVector3D col;
        float emission = 0.0f;
        if (mesh.isEmissive) {
            col = QVector3D(0.84f, 0.93f, 1.0f);
            emission = 1.2f;
        } else {
            col = QVector3D(0.25f, 0.25f, 0.30f);
        }
        glUniform3f(glGetUniformLocation(m_shader, "uColor"),
            col.x(), col.y(), col.z());
        glUniform1f(glGetUniformLocation(m_shader, "uEmission"), emission);

        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indices.size(),
                       GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

void GLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Disegna PNG pannello come sfondo fullscreen
    glDisable(GL_DEPTH_TEST);
    drawPanel();
    glEnable(GL_DEPTH_TEST);

    // Overlay pulsanti
    m_transport.render(m_camera.viewMatrix(), m_camera.projectionMatrix());

    // Display testo pixel-space sopra il pannello
    QMatrix4x4 pixelProj;
    pixelProj.ortho(0, width(), 0, height(), -1, 1);
    m_display.renderBackground(0, 0);
    m_display.render(pixelProj,
                     m_audio->title(),
                     m_audio->artist(),
                     m_audio->elapsed(),
                     m_audio->duration(),
                     m_audio->format(),
                     m_audio->bitrate());
}

void GLWidget::mouseMoveEvent(QMouseEvent *e) {
    float nx = (float)e->x() / width();
    float ny = (float)e->y() / height();
    m_camera.setMousePosition(nx, ny);
    // passa coordinate pixel per hit detection
    float px = (float)e->x() * 2029.0f / width();
    float py = (float)e->y() * 508.0f / height();
    m_transport.handleMouseMove(px, py);
}

void GLWidget::mousePressEvent(QMouseEvent *e) {
    float px = (float)e->x() * 2029.0f / width();
    float py = (float)e->y() * 508.0f / height();
    m_transport.handleMousePress(px, py);
}

void GLWidget::keyPressEvent(QKeyEvent *e) {
    switch (e->key()) {
        case Qt::Key_Space: m_audio->playPauseToggle(); break;
        case Qt::Key_S:     m_audio->stop();            break;
        case Qt::Key_Left:  m_audio->previous();        break;
        case Qt::Key_Right: m_audio->next();            break;
        case Qt::Key_O: {
            QString dir = QFileDialog::getExistingDirectory(
                this, "Apri cartella musica");
            if (!dir.isEmpty()) m_audio->loadFolder(dir);
            break;
        }
        default: break;
    }
}
