#include <GL/glew.h>
#include "library_screen.h"
#include <QDebug>
#include <cstring>

static const char* TXT_VERT = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
out vec2 vUV;
void main(){ vUV=aUV; gl_Position=vec4(aPos,0,1); }
)";
static const char* TXT_FRAG = R"(
#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
uniform vec4 uColor;
out vec4 fragColor;
void main(){
    float a = texture(uTex, vUV).r;
    fragColor = vec4(uColor.rgb, uColor.a * a);
}
)";

static GLuint compileTextShader(){
    auto comp=[](GLenum t,const char* s)->GLuint{
        GLuint id=glCreateShader(t);
        glShaderSource(id,1,&s,nullptr);
        glCompileShader(id);
        return id;
    };
    GLuint vs=comp(GL_VERTEX_SHADER,TXT_VERT);
    GLuint fs=comp(GL_FRAGMENT_SHADER,TXT_FRAG);
    GLuint p=glCreateProgram();
    glAttachShader(p,vs); glAttachShader(p,fs);
    glLinkProgram(p);
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

LibraryScreen::LibraryScreen(){}
LibraryScreen::~LibraryScreen(){
    if(m_glyphTex)   glDeleteTextures(1,&m_glyphTex);
    if(m_textVAO)    glDeleteVertexArrays(1,&m_textVAO);
    if(m_textVBO)    glDeleteBuffers(1,&m_textVBO);
    if(m_textShader) glDeleteProgram(m_textShader);
    if(m_face)       FT_Done_Face(m_face);
    if(m_ft)         FT_Done_FreeType(m_ft);
}

void LibraryScreen::init(GLuint /*panelShader*/){
    m_textShader = compileTextShader();

    glGenVertexArrays(1,&m_textVAO);
    glGenBuffers(1,&m_textVBO);
    glBindVertexArray(m_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER,m_textVBO);
    glBufferData(GL_ARRAY_BUFFER,6*4*sizeof(float),nullptr,GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glBindVertexArray(0);

    if(FT_Init_FreeType(&m_ft)){
        qWarning()<<"LibraryScreen: FT_Init_FreeType fail"; return;
    }
    const char* fontPath="/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf";
    if(FT_New_Face(m_ft,fontPath,0,&m_face)){
        qWarning()<<"LibraryScreen: font not found"; return;
    }
    FT_Set_Pixel_Sizes(m_face,0,28);
    m_ftReady=true;

    // Texture atlas glifi
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glGenTextures(1,&m_glyphTex);
    glBindTexture(GL_TEXTURE_2D,m_glyphTex);
    const int ATW=1024, ATH=64;
    std::vector<unsigned char> buf(ATW*ATH,0);
    int px=0;
    for(int c=32;c<128;c++){
        if(FT_Load_Char(m_face,(char)c,FT_LOAD_RENDER)) continue;
        FT_GlyphSlot g=m_face->glyph;
        int bw=g->bitmap.width, bh=g->bitmap.rows;
        if(px+bw>=ATW) break;
        for(int row=0;row<bh;row++)
            memcpy(&buf[(row*ATW)+px], g->bitmap.buffer+row*bw, bw);
        m_glyphs[c]={
            (float)px/ATW, 0.0f,
            (float)bw/ATW, (float)bh/ATH,
            g->bitmap_left, g->bitmap_top,
            bw, bh,
            (int)(g->advance.x>>6)
        };
        px+=bw+1;
    }
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,ATW,ATH,0,GL_RED,GL_UNSIGNED_BYTE,buf.data());
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D,0);
}

void LibraryScreen::renderText(const QString& text, float px, float py,
                                float scale, float r, float g, float b, float a,
                                int winW, int winH){
    if(!m_ftReady||!m_glyphTex) return;
    glUseProgram(m_textShader);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,m_glyphTex);
    glUniform1i(glGetUniformLocation(m_textShader,"uTex"),0);
    glUniform4f(glGetUniformLocation(m_textShader,"uColor"),r,g,b,a);

    float cx=px;
    for(QChar qc:text){
        int c=qc.toLatin1();
        if(c<32||c>=128) continue;
        Glyph& gl=m_glyphs[c];
        if(gl.bw==0){ cx+=gl.advance*scale*2.0f/winW; continue; }
        float x0=cx + gl.bx*scale*2.0f/winW;
        float y0=py - (gl.bh-gl.by)*scale*2.0f/winH;
        float x1=x0 + gl.bw*scale*2.0f/winW;
        float y1=y0 + gl.bh*scale*2.0f/winH;
        float v[]={
            x0,y0, gl.tx,        gl.ty+gl.th,
            x1,y0, gl.tx+gl.tw,  gl.ty+gl.th,
            x1,y1, gl.tx+gl.tw,  gl.ty,
            x0,y0, gl.tx,        gl.ty+gl.th,
            x1,y1, gl.tx+gl.tw,  gl.ty,
            x0,y1, gl.tx,        gl.ty
        };
        glBindVertexArray(m_textVAO);
        glBindBuffer(GL_ARRAY_BUFFER,m_textVBO);
        glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(v),v);
        glDrawArrays(GL_TRIANGLES,0,6);
        glBindVertexArray(0);
        cx+=gl.advance*scale*2.0f/winW;
    }
    glDisable(GL_BLEND);
    glUseProgram(0);
}

void LibraryScreen::render(int winW, int winH, int panelW, int panelH,
                            int sx, int sy, int sw, int sh){
    // Scissor clipping — testo non esce dalla cornice
    int scX=(int)((float)sx/panelW*winW);
    int scY=(int)((float)(panelH-sy-sh)/panelH*winH);
    int scW=(int)((float)sw/panelW*winW);
    int scH=(int)((float)sh/panelH*winH);
    glEnable(GL_SCISSOR_TEST);
    glScissor(scX,scY,scW,scH);

    if(!m_ftReady){ glDisable(GL_SCISSOR_TEST); return; }

    // Coordinate NDC schermo
    float ndcX0=(float)sx/panelW*2.0f-1.0f;
    float ndcY1=1.0f-(float)sy/panelH*2.0f;
    float lineH=(float)sh/panelH*2.0f/(m_visibleRows+2);
    float scale=(float)winH/panelH;

    if(m_videoMode){
        float lx=ndcX0+0.012f;
        // Titolo
        renderText(m_currentTitle, lx, ndcY1-lineH*0.7f,
                   scale, 0.84f,0.93f,1.0f,1.0f, winW,winH);
        // Separatore
        renderText("──────────────────", lx, ndcY1-lineH*1.3f,
                   scale*0.7f, 0.3f,0.5f,0.7f,0.6f, winW,winH);
        // Info
        renderText("Durata  : "+m_videoDuration, lx, ndcY1-lineH*2.0f,
                   scale, 0.55f,0.72f,0.88f,0.9f, winW,winH);
        renderText(QString("Res     : %1 x %2").arg(m_videoW).arg(m_videoH), lx, ndcY1-lineH*2.8f,
                   scale, 0.55f,0.72f,0.88f,0.9f, winW,winH);
        renderText(QString("FPS     : %1").arg(m_videoFps,0,'f',2), lx, ndcY1-lineH*3.6f,
                   scale, 0.55f,0.72f,0.88f,0.9f, winW,winH);
        if(!m_videoVCodec.isEmpty())
            renderText("V.Codec : "+m_videoVCodec, lx, ndcY1-lineH*4.4f,
                       scale, 0.55f,0.72f,0.88f,0.9f, winW,winH);
        if(!m_videoACodec.isEmpty()){
            QString aInfo=m_videoACodec;
            if(m_videoSampleRate>0) aInfo+=QString(" %1Hz").arg(m_videoSampleRate);
            renderText("A.Codec : "+aInfo, lx, ndcY1-lineH*5.2f,
                       scale, 0.55f,0.72f,0.88f,0.9f, winW,winH);
        }
        // Separatore
        renderText("──────────────────", lx, ndcY1-lineH*6.0f,
                   scale*0.7f, 0.3f,0.5f,0.7f,0.6f, winW,winH);
        renderText("[ IN RIPRODUZIONE ]", lx, ndcY1-lineH*6.8f,
                   scale*0.85f, 0.84f,0.93f,1.0f,0.7f, winW,winH);
    } else {
    if(!m_currentTitle.isEmpty()){
        renderText(">> "+m_currentTitle,
                   ndcX0+0.01f, ndcY1-lineH*0.7f,
                   scale, 0.84f,0.93f,1.0f,1.0f, winW,winH);
    }

    // Lista brani
    for(int i=0;i<m_visibleRows;i++){
        int idx=m_scrollOffset+i;
        if(idx>=(int)m_tracks.size()) break;
        bool sel=(idx==m_selectedIdx);
        float y=ndcY1-lineH*(1.9f+i);
        float cr=sel?0.84f:0.55f;
        float cg=sel?0.93f:0.72f;
        float cb=sel?1.0f :0.88f;
        float ca=sel?1.0f :0.75f;
        QString label=QString("%1  %2").arg(idx+1,3).arg(m_tracks[idx]);
        renderText(label, ndcX0+0.01f, y, scale, cr,cg,cb,ca, winW,winH);
    }

    } // fine else videoMode
    glDisable(GL_SCISSOR_TEST);
}

void LibraryScreen::setVideoInfo(const QString& title, const QString& duration, double fps,
                                  int w, int h, const QString& vCodec,
                                  const QString& aCodec, int sampleRate, int bitrate){
    m_currentTitle=title;
    m_currentArtist="";
    m_videoDuration=duration;
    m_videoFps=fps;
    m_videoW=w; m_videoH=h;
    m_videoVCodec=vCodec;
    m_videoACodec=aCodec;
    m_videoSampleRate=sampleRate;
    m_videoBitrate=bitrate;
    m_videoMode=true;
}
void LibraryScreen::setTrackList(const QStringList& t){ m_tracks=t; m_scrollOffset=0; m_selectedIdx=0; m_videoMode=false; }
void LibraryScreen::setCurrentTrack(const QString& t,const QString& a){ m_currentTitle=t; m_currentArtist=a; }
void LibraryScreen::scrollUp()  { if(m_scrollOffset>0) m_scrollOffset--; }
void LibraryScreen::scrollDown(){ if(m_scrollOffset<(int)m_tracks.size()-m_visibleRows) m_scrollOffset++; }
void LibraryScreen::setSelectedIndex(int i){ m_selectedIdx=i; }
