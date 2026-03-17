#include <GL/glew.h>
#include "display_module.h"
#include <QDebug>
#include <QImage>
#include <QDateTime>
#include <QFile>
#include <QTime>

DisplayModule::DisplayModule()
    : m_vao(0), m_vbo(0), m_shader(0), m_panelShader(0)
    , m_texDisplay(0), m_panelVao(0), m_panelVbo(0), m_panelEbo(0)
    , m_ready(false)
{}

DisplayModule::~DisplayModule() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_panelVao) glDeleteVertexArrays(1, &m_panelVao);
    if (m_panelVbo) glDeleteBuffers(1, &m_panelVbo);
    if (m_panelEbo) glDeleteBuffers(1, &m_panelEbo);
    if (m_texDisplay) glDeleteTextures(1, &m_texDisplay);
    for (auto &c : m_chars)
        glDeleteTextures(1, &c.textureID);
    if (m_ready) {
        FT_Done_Face(m_face);
        FT_Done_FreeType(m_ft);
    }
}

GLuint DisplayModule::loadTexture(const QString &path) {
    QImage img(path);
    if (img.isNull()) { qWarning() << "DisplayModule: texture non trovata:" << path; return 0; }
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

bool DisplayModule::init(GLuint textShaderProgram, GLuint panelShaderProgram) {
    m_shader      = textShaderProgram;
    m_panelShader = panelShaderProgram;

    if (FT_Init_FreeType(&m_ft)) {
        qWarning() << "DisplayModule: FreeType init fallito";
        return false;
    }
    loadFont("/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf", 24);
    loadFontInto("/home/snorky/.fonts/DSEG7Classic-Bold.ttf", 32, m_faceDigital, m_charsDigital);

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*6*4, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // VAO pannello display PNG
    float verts[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f
    };
    unsigned int idx[] = { 0,1,2, 0,2,3 };
    glGenVertexArrays(1, &m_panelVao);
    glGenBuffers(1, &m_panelVbo);
    glGenBuffers(1, &m_panelEbo);
    glBindVertexArray(m_panelVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_panelVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_panelEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    m_texDisplay = 0; // non usata — display GLSL
    m_displayShader = loadDisplayShader();

    m_ready = true;
    return true;
}

void DisplayModule::loadFont(const QString &path, int size) {
    if (FT_New_Face(m_ft, path.toLocal8Bit().constData(), 0, &m_face)) {
        qWarning() << "DisplayModule: font non trovato:" << path;
        return;
    }
    FT_Set_Pixel_Sizes(m_face, 0, size);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(m_face, c, FT_LOAD_RENDER)) continue;
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
            m_face->glyph->bitmap.width,
            m_face->glyph->bitmap.rows,
            0, GL_RED, GL_UNSIGNED_BYTE,
            m_face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Character ch;
        ch.textureID = tex;
        ch.width     = m_face->glyph->bitmap.width;
        ch.height    = m_face->glyph->bitmap.rows;
        ch.bearingX  = m_face->glyph->bitmap_left;
        ch.bearingY  = m_face->glyph->bitmap_top;
        ch.advance   = m_face->glyph->advance.x;
        m_chars[QChar(c)] = ch;
    }
}

QString DisplayModule::formatTime(qint64 ms) {
    int s = (ms / 1000) % 60;
    int m = (ms / 60000) % 60;
    return QString("%1:%2").arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0'));
}

void DisplayModule::renderText(const QString &text, float x, float y,
                                float scale, float r, float g, float b)
{
    if (!m_ready) return;
    glUseProgram(m_shader);
    glUniform3f(glGetUniformLocation(m_shader, "uTextColor"), r, g, b);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_vao);

    for (QChar qc : text) {
        if (!m_chars.contains(qc)) continue;
        Character ch = m_chars[qc];
        float xpos = x + ch.bearingX * scale;
        float ypos = y - (ch.height - ch.bearingY) * scale;
        float w = ch.width  * scale;
        float h = ch.height * scale;
        float verts[6][4] = {
            {xpos,   ypos+h, 0,0},
            {xpos,   ypos,   0,1},
            {xpos+w, ypos,   1,1},
            {xpos,   ypos+h, 0,0},
            {xpos+w, ypos,   1,1},
            {xpos+w, ypos+h, 1,0}
        };
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DisplayModule::renderBackground(int winW, int winH) {
    (void)winW; (void)winH;
    // coordinate display in pixel (2029x508): X=230..1270, Y=60..310 dal top
    // convertiamo in NDC
    float x0 = (302.0f  / 2030.0f) * 2.0f - 1.0f;
    float x1 = (1577.0f / 2030.0f) * 2.0f - 1.0f;
    float y0 = 1.0f - (361.0f / 537.0f) * 2.0f;
    float y1 = 1.0f - (167.0f / 537.0f) * 2.0f;
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
    glUseProgram(m_displayShader);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}

void DisplayModule::renderProgressBarValue(float x, float y, float w, float h, float value) {
    int numSegments = 22;
    float segW  = (w / numSegments) * 0.75f;
    float segGap = (w / numSegments) * 0.25f;
    int filled = (int)(value * numSegments);

    glUseProgram(m_shader);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    for(int i = 0; i < numSegments; i++) {
        float sx = x + i * (segW + segGap);
        float x1 = sx, x2 = sx + segW;
        float y1 = y, y2 = y + h;

        float r, g, b;
        if(i < filled) {
            // azzurro ghiaccio come VU meter — si intensifica verso destra
            float t = (float)i / numSegments;
            r = 0.52f + t * 0.32f;
            g = 0.72f + t * 0.21f;
            b = 0.95f + t * 0.05f;
        } else {
            // sfondo scuro
            r = 0.04f; g = 0.06f; b = 0.09f;
        }

        GLuint dummyTex;
        glGenTextures(1, &dummyTex);
        glBindTexture(GL_TEXTURE_2D, dummyTex);
        unsigned char white[] = {255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, white);

        float verts[] = {
            x1,y1, 0,0,  x2,y1, 1,0,  x2,y2, 1,1,
            x1,y1, 0,0,  x2,y2, 1,1,  x1,y2, 0,1
        };
        glUniform3f(glGetUniformLocation(m_shader, "uTextColor"), r, g, b);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDeleteTextures(1, &dummyTex);
    }
    glBindVertexArray(0);
}
void DisplayModule::renderProgressBar(float x, float y, float w, float h,
                                       qint64 elapsed, qint64 duration)
{
    if (duration <= 0) return;
    float progress = (float)elapsed / (float)duration;
    if (progress > 1.0f) progress = 1.0f;

    glUseProgram(m_shader);

    // texture dummy bianca per disegnare rettangoli
    GLuint dummyTex;
    glGenTextures(1, &dummyTex);
    glBindTexture(GL_TEXTURE_2D, dummyTex);
    unsigned char white[] = {255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, white);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Barra segmentata stile equalizzatore
    int numSegments = 36;
    float segW = (w / numSegments) * 0.78f;
    float segGap = (w / numSegments) * 0.22f;
    int activeSegs = (int)(progress * numSegments);

    for (int s = 0; s < numSegments; s++) {
        float sx = x + s * (segW + segGap);
        float sy = y;

        if (s < activeSegs) {
            // segmento attivo — azzurro LED ghiaccio
            glUniform3f(glGetUniformLocation(m_shader, "uTextColor"), 0.84f, 0.93f, 1.0f);
        } else {
            // segmento inattivo — scuro quasi invisibile
            glUniform3f(glGetUniformLocation(m_shader, "uTextColor"), 0.08f, 0.12f, 0.18f);
        }

        float verts[6][4] = {
            {sx,      sy,   0,0}, {sx+segW, sy,   1,0}, {sx+segW, sy+h, 1,1},
            {sx,      sy,   0,0}, {sx+segW, sy+h, 1,1}, {sx,      sy+h, 0,1}
        };
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glDeleteTextures(1, &dummyTex);
}

void DisplayModule::render(const QMatrix4x4 &proj, int winW, int winH,
                            const QString &title,
                            const QString &artist,
                            qint64 elapsed,
                            qint64 duration,
                            const QString &format,
                            int bitrate)
{
    if (!m_ready) return;
    float sx = winW  / 1600.0f;
    float sy = winH  / 423.0f;

    // 2. Testo — proiezione pixel space
    glDisable(GL_DEPTH_TEST);
    glUseProgram(m_shader);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "uProjection"),
        1, GL_FALSE, proj.constData());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // colore base azzurro #D6EEFF
    float R = 0.84f, G = 0.93f, B = 1.0f;

    // Titolo — grande, in alto a sinistra
    QString t = title.isEmpty() ? "--- EvoPlayer ---" : title;
    renderTextWith(t, 261*sx, 225*sy, 1.0f*sx, R, G, B, m_charsDigital);

    // Artista — sotto, più piccolo
    if (!artist.isEmpty())
        renderTextWith(artist, 261*sx, 198*sy, 0.78f*sx, R, G, B, m_charsDigital);

    // Orologio digitale — in alto a destra
    QString clock = QTime::currentTime().toString("HH:mm:ss");
    renderTextWith(clock, 1000*sx, 225*sy, 1.2f*sx, R, G, B, m_charsDigital);

    // Tempo + Formato + Bitrate — stessa riga
    QString timeStr = formatTime(elapsed) + " / " + formatTime(duration);
    QString fmtStr  = format.isEmpty() ? "" : "   " + format +
                      (bitrate > 0 ? QString(" • %1 kbps").arg(bitrate) : "");
    renderTextWith(timeStr + fmtStr, 261*sx, 175*sy, 0.72f*sx, R, G, B, m_charsDigital);

    // Barra progresso
    renderProgressBar(261*sx, 148*sy, 934*sx, 11*sy, elapsed, duration);

    // Barre V/B/A a destra sotto orologio
    float bx = 709.0f*sx;
    float barX = 729.0f*sx;
    float barW = 220.0f*sx;
    float barH = 4.0f*sy;

    // V - Volume
    renderTextWith("V", bx, 232*sy, 0.55f*sx, R, G, B, m_charsDigital);
    renderProgressBarValue(barX, 232*sy, barW, barH, m_volume);

    // B - Bassi
    renderTextWith("B", bx, 210*sy, 0.55f*sx, R, G, B, m_charsDigital);
    renderProgressBarValue(barX, 210*sy, barW, barH, m_bass);

    // A - Alti
    renderTextWith("A", bx, 188*sy, 0.55f*sx, R, G, B, m_charsDigital);
    renderProgressBarValue(barX, 188*sy, barW, barH, m_treble);

    glDisable(GL_BLEND);
}

GLuint DisplayModule::loadDisplayShader() {
    // Vertex
    QFile vf("/home/snorky/EvoPlayer/shaders/display_vertex.glsl");
    vf.open(QIODevice::ReadOnly);
    QByteArray vsrc = vf.readAll();
    const char* vptr = vsrc.constData();
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vptr, nullptr);
    glCompileShader(vert);

    // Fragment
    QFile ff("/home/snorky/EvoPlayer/shaders/display_fragment.glsl");
    ff.open(QIODevice::ReadOnly);
    QByteArray fsrc = ff.readAll();
    const char* fptr = fsrc.constData();
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fptr, nullptr);
    glCompileShader(frag);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

void DisplayModule::loadFontInto(const QString &path, int size, FT_Face &face, QMap<QChar, Character> &chars) {
    if (FT_New_Face(m_ft, path.toLocal8Bit().constData(), 0, &face)) {
        qWarning() << "DisplayModule: font non trovato:" << path;
        return;
    }
    FT_Set_Pixel_Sizes(face, 0, size);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0, GL_RED, GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Character ch;
        ch.textureID = tex;
        ch.width     = face->glyph->bitmap.width;
        ch.height    = face->glyph->bitmap.rows;
        ch.bearingX  = face->glyph->bitmap_left;
        ch.bearingY  = face->glyph->bitmap_top;
        ch.advance   = face->glyph->advance.x;
        chars[QChar(c)] = ch;
    }
}

void DisplayModule::renderTextWith(const QString &text, float x, float y,
                                    float scale, float r, float g, float b,
                                    QMap<QChar, Character> &chars)
{
    if (!m_ready) return;
    glUseProgram(m_shader);
    glUniform3f(glGetUniformLocation(m_shader, "uTextColor"), r, g, b);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_vao);

    for (QChar qc : text) {
        if (!chars.contains(qc)) continue;
        Character ch = chars[qc];
        float xpos = x + ch.bearingX * scale;
        float ypos = y - (ch.height - ch.bearingY) * scale;
        float w = ch.width  * scale;
        float h = ch.height * scale;
        float verts[6][4] = {
            {xpos,   ypos+h, 0,0},
            {xpos,   ypos,   0,1},
            {xpos+w, ypos,   1,1},
            {xpos,   ypos+h, 0,0},
            {xpos+w, ypos,   1,1},
            {xpos+w, ypos+h, 1,0}
        };
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
