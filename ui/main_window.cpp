#include <GL/glew.h>
#include "main_window.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFileDialog>
#include "ui/library_window.h"
#include <QMenu>
#include <QSettings>
#include <QAction>
#include <QProcess>
#include <QApplication>
#include <QFile>
#include <QDialog>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QMatrix4x4>
#include <QWindow>
#include <QImage>
#include "modules/skin_manager.h"
#include "snap_manager.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    m_gl = new GLWidget(this);
    setCentralWidget(m_gl);
    m_gl->setFocus();
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
        // Snap periodico via QWindow::position()
        if (window()->windowHandle()) {
            static QPoint lastPos;
            static int stillFrames = 0;
            QPoint cur = window()->windowHandle()->position();
            if (cur != lastPos)
                qDebug() << "POS CAMBIA:" << cur;
            if (cur == lastPos) {
                stillFrames++;
                if (stillFrames == 10) {
                    qDebug() << "SNAP at" << cur;
                    SnapManager::instance()->snap(window());
                }
            } else {
                lastPos = cur;
                stillFrames = 0;
            }
        }
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

    QString base = qgetenv("EVOPLAYER_BASE").isEmpty() ? QString::fromLocal8Bit(qgetenv("HOME")) + "/EvoPlayer/" : QString::fromLocal8Bit(qgetenv("EVOPLAYER_BASE")) + "/";
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
    m_transport.onPlay  = [this](){ if(m_lib&&m_lib->isVideoPlaying()) m_lib->videoPause(); else m_audio->playPauseToggle(); };
    m_transport.onPause = [this](){ if(m_lib&&m_lib->isVideoPlaying()) m_lib->videoPause(); else m_audio->playPauseToggle(); };
    m_transport.onStop  = [this](){ if(m_lib&&m_lib->isVideoPlaying()) m_lib->videoStop(); else m_audio->stop(); };
    m_transport.onNext  = [this](){ m_audio->next(); };
    m_transport.onPrev  = [this](){ m_audio->previous(); };
    m_transport.onOpen  = [this](){
        QString dir = QFileDialog::getExistingDirectory(nullptr, "Apri cartella musica", QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
        if (!dir.isEmpty()) m_audio->loadFolder(dir);
    };

    connect(m_audio, &QmmpBridge::stateChanged, this, [this](int s){
        qDebug() << "stateChanged ricevuto:" << s;
        if (s == 0) m_transport.setPlayerState(PlayerState::PLAYING);
        else if (s == 1) m_transport.setPlayerState(PlayerState::PAUSED);
        else if (s == 3) {} // Buffering — ignora
        else m_transport.setPlayerState(PlayerState::STOPPED);
    });

    m_display.init(m_textShader, m_panelShader);
    m_vuMeter.init(m_textShader);
    m_powerButton.init(base+"assets/buttons/power_normal.png", base+"assets/buttons/power_on.png", 153, 235, 84);
    connect(&m_powerButton, &PowerButton::clicked, this, [this](){
        QMenu menu(this);
        menu.setStyleSheet(
            "QMenu { background:#1a1a1a; color:#D6EEFF; border:1px solid #334455; }"
            "QMenu::item:selected { background:#2a3a4a; }"
            "QMenu::separator { background:#334455; height:1px; margin:4px 8px; }"
        );

        // Apri file
        QAction* openFile = menu.addAction(QString::fromUtf8("🎵  Apri brano singolo"));
        connect(openFile, &QAction::triggered, this, [this](){
            QString f = QFileDialog::getOpenFileName(this, "Apri brano",
                QString(), "Audio (*.mp3 *.flac *.ogg *.wav *.opus *.m4a *.aac)");
            if(!f.isEmpty()) m_audio->loadFile(f);
        });

        // Apri cartella
        QAction* openDir = menu.addAction(QString::fromUtf8("📁  Apri cartella musica"));
        connect(openDir, &QAction::triggered, this, [this](){
            QString d = QFileDialog::getExistingDirectory(this, "Apri cartella musica");
            if(!d.isEmpty()) m_audio->loadFolder(d);
        });

        // Apri video
        QAction* openVideo = menu.addAction(QString::fromUtf8("🎬  Apri video"));
        connect(openVideo, &QAction::triggered, this, [this](){
            QString f = QFileDialog::getOpenFileName(this, "Apri video",
                QString(), "Video (*.mp4 *.mkv *.avi *.mov *.webm *.flv)");
            if(!f.isEmpty() && m_lib){
                m_audio->stop();
                m_lib->loadVideo(f);
            }
        });
        menu.addSeparator();

        // Impostazioni
        QMenu* settings = menu.addMenu(QString::fromUtf8("⚙️  Impostazioni"));
        settings->setStyleSheet(menu.styleSheet());

        // Risoluzione
        QMenu* resMenu = settings->addMenu("Risoluzione");
        resMenu->setStyleSheet(menu.styleSheet());
        QList<QPair<QString,QSize>> resolutions = {
            {"640×480",    QSize(640,169)},
            {"800×600",    QSize(800,211)},
            {"1024×768",   QSize(1024,270)},
            {"1280×720 (HD)", QSize(1280,337)},
            {"1280×800",   QSize(1280,337)},
            {"1360×768",   QSize(1360,358)},
            {"1366×768",   QSize(1366,360)},
            {"1440×900",   QSize(1440,379)},
            {"1600×900",   QSize(1600,423)},
            {"1920×1080 (Full HD)", QSize(1920,506)},
            {"2560×1440 (2K)", QSize(2560,675)},
            {"2560×1600",  QSize(2560,675)},
            {"2880×1800",  QSize(2880,759)},
            {"3440×1440 (Ultrawide)", QSize(3440,907)},
            {"3840×2160 (4K)", QSize(3840,1013)}
        };
        for(auto& r : resolutions) {
            QAction* a = resMenu->addAction(r.first);
            QSize sz = r.second;
            connect(a, &QAction::triggered, this, [this, sz](){
                window()->setFixedSize(sz);
                resize(sz);
                resizeGL(sz.width(), sz.height());
                update();
                QSettings s("EvoPlayer", "EvoPlayer");
                s.setValue("risoluzione", sz);
            });
        }

        // Skin
        QMenu* skinMenu = settings->addMenu(QString::fromUtf8("Skin"));
        skinMenu->setStyleSheet(menu.styleSheet());
        QString base2 = (qgetenv("EVOPLAYER_BASE").isEmpty() ? QString::fromLocal8Bit(qgetenv("HOME")) + "/EvoPlayer/" : QString::fromLocal8Bit(qgetenv("EVOPLAYER_BASE")) + "/");
        QString skinsDir = base2 + "skins";
        QStringList skins = SkinManager::instance().availableSkins(skinsDir);
        for(auto& sk : skins) {
            QAction* sa = skinMenu->addAction(sk);
            connect(sa, &QAction::triggered, this, [this, skinsDir, sk](){
                if(SkinManager::instance().load(skinsDir + "/" + sk)) {
                    const SkinData& sd = SkinManager::instance().data();
                    QSettings cfg("EvoPlayer","EvoPlayer");
                    cfg.setValue("skin", sk);
                    // ricarica plancia
                    m_panelTexture = loadTexture(sd.panelImage);
                    m_panelW   = sd.panelW;
                    m_panelH   = sd.panelH;
                    // ricarica knob
                    m_knobVolume.init(skinsDir+"/"+sk+"/knobs/knob_volume.png", sd.knobVolX, sd.knobVolY, sd.knobVolSize);
                    m_knobBass.init(skinsDir+"/"+sk+"/knobs/knob_bass.png",    sd.knobBassX, sd.knobBassY, sd.knobBassSize);
                    m_knobTreble.init(skinsDir+"/"+sk+"/knobs/knob_mid.png",   sd.knobTrebleX, sd.knobTrebleY, sd.knobTrebleSize);
                    // ricarica power
                    m_powerButton.init(skinsDir+"/"+sk+"/buttons/power_normal.png",
                                       skinsDir+"/"+sk+"/buttons/power_on.png",
                                       sd.powerX, sd.powerY, sd.powerSize);
                    update();
                }
            });
        }
        skinMenu->addSeparator();
        QAction* openSkin = skinMenu->addAction(QString::fromUtf8("Apri skin..."));
        connect(openSkin, &QAction::triggered, this, [this](){
            QString dir = QFileDialog::getExistingDirectory(this, QString::fromUtf8("Seleziona cartella skin"));
            if(!dir.isEmpty() && SkinManager::instance().load(dir)) {
                const SkinData& sd = SkinManager::instance().data();
                m_panelTexture = loadTexture(sd.panelImage);
                m_panelW   = sd.panelW;
                m_panelH   = sd.panelH;
                m_knobVolume.init(dir+"/knobs/knob_volume.png", sd.knobVolX, sd.knobVolY, sd.knobVolSize);
                m_knobBass.init(dir+"/knobs/knob_bass.png",     sd.knobBassX, sd.knobBassY, sd.knobBassSize);
                m_knobTreble.init(dir+"/knobs/knob_mid.png",    sd.knobTrebleX, sd.knobTrebleY, sd.knobTrebleSize);
                m_powerButton.init(dir+"/buttons/power_normal.png",
                                   dir+"/buttons/power_on.png",
                                   sd.powerX, sd.powerY, sd.powerSize);
                update();
            }
        });

        // Dispositivo audio
        QMenu* audioMenu = settings->addMenu(QString::fromUtf8("Dispositivo audio"));
        audioMenu->setStyleSheet(menu.styleSheet());
        audioMenu->addAction("Default (PipeWire/ALSA)");

        // Lingua
        QMenu* langMenu = settings->addMenu("Lingua");
        langMenu->setStyleSheet(menu.styleSheet());
        langMenu->addAction("Italiano");
        langMenu->addAction("English");

        // Sempre in primo piano
        QAction* onTop = settings->addAction(QString::fromUtf8("Sempre in primo piano"));
        onTop->setCheckable(true);
        onTop->setChecked(false);
        connect(onTop, &QAction::toggled, this, [this](bool on){
            Qt::WindowFlags flags = window()->windowFlags();
            if(on) flags |= Qt::WindowStaysOnTopHint;
            else   flags &= ~Qt::WindowStaysOnTopHint;
            window()->setWindowFlags(flags);
            window()->show();
        });

        settings->addSeparator();

        // Guida creazione skin
        QAction* guida = settings->addAction(QString::fromUtf8("\U0001F4D6  Guida creazione skin..."));
        connect(guida, &QAction::triggered, this, [this](){
            QString base = (qgetenv("EVOPLAYER_BASE").isEmpty() ? QString::fromLocal8Bit(qgetenv("HOME")) + "/EvoPlayer/" : QString::fromLocal8Bit(qgetenv("EVOPLAYER_BASE")) + "/");
            QFile fguida(base + "assets/guida_skin.md");
            QString testo;
            if(fguida.open(QIODevice::ReadOnly | QIODevice::Text))
                testo = QString::fromUtf8(fguida.readAll());
            else
                testo = "Guida non trovata in assets/guida_skin.md";

            QDialog* dlg = new QDialog(this);
            dlg->setWindowTitle(QString::fromUtf8("Guida Creazione Skin"));
            dlg->setFixedSize(600, 500);
            dlg->setStyleSheet("background:#1a1a1a; color:#D6EEFF;");
            QVBoxLayout* layout = new QVBoxLayout(dlg);
            QTextEdit* te = new QTextEdit(dlg);
            te->setMarkdown(testo);
            te->setReadOnly(true);
            te->setStyleSheet("background:#111; color:#D6EEFF; border:1px solid #334455;");
            layout->addWidget(te);
            QPushButton* btn = new QPushButton(QString::fromUtf8("Salva guida come .md"), dlg);
            btn->setStyleSheet("background:#2a3a4a; color:#D6EEFF; padding:8px; border:1px solid #334455;");
            layout->addWidget(btn);
            connect(btn, &QPushButton::clicked, dlg, [testo, this](){
                QString path = QFileDialog::getSaveFileName(this,
                    QString::fromUtf8("Salva guida"),
                    QDir::homePath() + "/EvoPlayer_guida_skin.md",
                    "Markdown (*.md)");
                if(!path.isEmpty()){
                    QFile f(path);
                    if(f.open(QIODevice::WriteOnly | QIODevice::Text))
                        f.write(testo.toUtf8());
                }
            });
            dlg->exec();
        });

        menu.addSeparator();

        // Esci
        QAction* quit = menu.addAction(QString::fromUtf8("❌  Esci"));
        connect(quit, &QAction::triggered, qApp, &QApplication::quit);

        // Mostra menu sotto il power button
        QPoint pos = mapToGlobal(QPoint(
            (int)((153.0f/m_panelW)*width()),
            (int)((235.0f/m_panelH)*height())
        ));
        menu.exec(pos);
    });
    m_knobVolume.init(base+"assets/knobs/knob_volume.png", 1798, 329, 276);
    m_knobBass.init(base+"assets/knobs/knob_bass.png", 1313, 440, 105);
    m_knobTreble.init(base+"assets/knobs/knob_mid.png", 1522, 440, 105);
    connect(&m_knobTreble, &KnobModule::valueChanged, this, [this](float v){
        m_audio->setTreble(v);
        m_display.setTreble(v);
        if(m_lib){
            float gains[10];
            for(int i=0;i<10;i++) gains[i]= (i>=7) ? v : m_lib->videoScreen().eqGain(i);
            m_lib->videoScreen().setEqGains(gains);
        }
    });
    connect(&m_knobBass, &KnobModule::valueChanged, this, [this](float v){
        m_audio->setBass(v);
        m_display.setBass(v);
        if(m_lib){
            float gains[10];
            for(int i=0;i<10;i++) gains[i]= (i<=2) ? v : m_lib->videoScreen().eqGain(i);
            m_lib->videoScreen().setEqGains(gains);
        }
    });
    QProcess::startDetached("pactl", {"set-sink-volume", "@DEFAULT_SINK@", "0%"});
    connect(&m_knobVolume, &KnobModule::valueChanged, this, [this](float v){
        m_audio->setVolume(v);
        m_display.setVolume(v);
    });
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
    m_display.renderBackground(width(), height());
    m_vuMeter.render(width(), height());
    m_powerButton.render(width(), height(), m_panelW, m_panelH);
    m_knobVolume.render(width(), height(), m_panelW, m_panelH);
    m_knobBass.render(width(), height(), m_panelW, m_panelH);
    m_knobTreble.render(width(), height(), m_panelW, m_panelH);
    m_display.render(pixelProj, width(), height(),
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
    float px = (float)e->x() * m_panelW / width();
    float py = (float)e->y() * m_panelH / height();
    m_transport.handleMouseMove(px, py);
    m_powerButton.mouseMoveEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    if(e->buttons() & Qt::LeftButton) m_knobVolume.mouseMoveEvent(e->x(), e->y());
    if(e->buttons() & Qt::LeftButton) m_knobBass.mouseMoveEvent(e->x(), e->y());
    if(e->buttons() & Qt::LeftButton) m_knobTreble.mouseMoveEvent(e->x(), e->y());
}

void GLWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::RightButton) {
        QWidget* top = window();
        if (top && top->windowHandle())
            top->windowHandle()->startSystemMove();
        return;
    }
    float px = (float)e->x() * m_panelW / width();
    float py = (float)e->y() * m_panelH / height();
    m_transport.handleMousePress(px, py);
    m_powerButton.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    m_knobVolume.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    m_knobBass.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
    m_knobTreble.mousePressEvent(e->x(), e->y(), width(), height(), m_panelW, m_panelH);
}

void GLWidget::keyPressEvent(QKeyEvent *e) {
    switch (e->key()) {
        case Qt::Key_F5: {
            qDebug() << "TEST xcb_configure prima:" << window()->pos();
            if (auto* qw = window()->windowHandle()) {
                void* conn = qw->nativeHandle();
                // testa con windowHandle position
                qw->setPosition(100, 100);
                qDebug() << "TEST xcb_configure dopo:" << window()->pos();
            }
            break;
        }
        case Qt::Key_Space: m_audio->playPauseToggle(); break;
        case Qt::Key_S:     m_audio->stop();            break;
        case Qt::Key_Left:  m_audio->previous();        break;
        case Qt::Key_Right: m_audio->next();            break;
        case Qt::Key_O: {
            QString dir = QFileDialog::getExistingDirectory(
                nullptr, "Apri cartella musica");
            if (!dir.isEmpty()) m_audio->loadFolder(dir);
            break;
        }
        default: break;
    }
}


void GLWidget::mouseReleaseEvent(QMouseEvent *) {
    m_dragging = false;
    m_knobVolume.mouseReleaseEvent();
    m_knobBass.mouseReleaseEvent();
    m_knobTreble.mouseReleaseEvent();
}

void MainWindow::setLibraryWindow(LibraryWindow* lib){ m_gl->setLibraryWindow(lib); }
QmmpBridge* MainWindow::audio() { return m_gl ? m_gl->audio() : nullptr; }
