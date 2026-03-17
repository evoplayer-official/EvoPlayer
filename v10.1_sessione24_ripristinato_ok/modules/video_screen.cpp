#include "video_screen.h"
#include <QPainter>
#include <QDebug>
#include <QFileInfo>
#include <cstring>

// ─── VLC Video Callbacks ──────────────────────────────────────────────────────

void* VideoScreen::vlcLock(void* data, void** planes) {
    VideoScreen* self=static_cast<VideoScreen*>(data);
    self->m_mutex.lock();
    planes[0]=self->m_frameBack.bits();
    return nullptr;
}

void VideoScreen::vlcUnlock(void* data, void* /*picture*/, void* const* /*planes*/) {
    VideoScreen* self=static_cast<VideoScreen*>(data);
    self->m_mutex.unlock();
}

void VideoScreen::vlcDisplay(void* data, void* /*picture*/) {
    VideoScreen* self=static_cast<VideoScreen*>(data);
    {
        QMutexLocker lk(&self->m_mutex);
        self->m_frame=self->m_frameBack.copy();
    }
    emit self->frameReady();
}

// ─── VLC Audio Callbacks ──────────────────────────────────────────────────────

int VideoScreen::vlcAudioSetup(void** data, char* format, unsigned* rate, unsigned* channels) {
    VideoScreen* self=static_cast<VideoScreen*>(*data);
    memcpy(format,"S16N",4);
    if(*channels>2) *channels=2;
    self->m_audioRate=*rate;
    self->m_audioChannels=*channels;
    qDebug()<<"vlcAudioSetup rate="<<*rate<<"ch="<<*channels;
    // Avvia aplay subito nel thread principale
    QMetaObject::invokeMethod(self,[self](){
        if(self->m_aplay){ self->m_aplay->kill(); delete self->m_aplay; }
        self->m_aplay=new QProcess(self);
        QStringList args;
        args<<"-f"<<"S16_LE"
            <<"-r"<<QString::number(self->m_audioRate)
            <<"-c"<<QString::number(self->m_audioChannels);
        self->m_aplay->start("aplay",args);
        self->m_aplay->waitForStarted(500);
        qDebug()<<"aplay started";
    },Qt::BlockingQueuedConnection);
    return 0;
}

void VideoScreen::vlcAudioCleanup(void* /*data*/) {}

void VideoScreen::vlcAudioPlay(void* data, const void* samples, unsigned count, int64_t /*pts*/) {
    VideoScreen* self=static_cast<VideoScreen*>(data);
    const int16_t* in=static_cast<const int16_t*>(samples);
    int total=count*self->m_audioChannels;

    // Applica EQ con filtri IIR low-shelf (bassi) e high-shelf (acuti)
    QByteArray pcm(total*2,0);
    int16_t* out=reinterpret_cast<int16_t*>(pcm.data());
    float bassGain=0.0f; for(int b=0;b<3;b++) bassGain+=self->m_eqGains[b]; bassGain/=3.0f;
    float trebGain=0.0f; for(int b=7;b<10;b++) trebGain+=self->m_eqGains[b]; trebGain/=3.0f;
    const float alpha=0.05f; // cutoff bassa freq
    const int ch=self->m_audioChannels>0?self->m_audioChannels:2;
    for(int i=0;i<total;i++){
        int c2=i%ch;
        float x=in[i]/32768.0f;
        // low-pass per bassi
        self->m_bassState[c2]=alpha*x+(1.0f-alpha)*self->m_bassState[c2];
        float low=self->m_bassState[c2];
        float high=x-low;
        float y=x + bassGain*low + trebGain*high;
        y*=32767.0f;
        if(y>32767)  y=32767;
        if(y<-32768) y=-32768;
        out[i]=(int16_t)y;
    }

    // VuMeter
    {
        int samples2=count;
        float* fL=new float[samples2];
        float* fR=new float[samples2];
        for(int i=0;i<samples2;i++){
            fL[i]=out[i*2  ]/32768.0f;
            fR[i]=out[i*2+1]/32768.0f;
        }
        Visual::addAudio(fL,samples2,2,0,0);
        delete[] fL;
        delete[] fR;
    }

    // Buffer audio thread-safe
    {
        QMutexLocker lk(&self->m_audioMutex);
        self->m_audioBuf.append(pcm);
    }
    emit self->audioDataReady();
}

void VideoScreen::vlcAudioFlush(void* /*data*/, int64_t /*pts*/) {}
void VideoScreen::vlcAudioDrain(void* /*data*/, int64_t /*pts*/) {}

void VideoScreen::setEqGains(float* gains10) {
    for(int i=0;i<10;i++) m_eqGains[i]=gains10[i];
}

// ─── VideoScreen ──────────────────────────────────────────────────────────────

VideoScreen::VideoScreen(QObject* parent) : QObject(parent) {
    const char* args[]={"--no-xlib","--quiet"};
    m_vlc=libvlc_new(2,args);
    // Connetti il segnale audio nel thread principale
    connect(this,&VideoScreen::audioDataReady,this,[this](){
        if(!m_aplay) return;
        QMutexLocker lk(&m_audioMutex);
        if(!m_audioBuf.isEmpty()){
            m_aplay->write(m_audioBuf);
            m_audioBuf.clear();
        }
    }, Qt::QueuedConnection);
}

VideoScreen::~VideoScreen() {
    stop();
    if(m_aplay){ m_aplay->kill(); delete m_aplay; }
    if(m_vlc) libvlc_release(m_vlc);
}

void VideoScreen::loadVideo(const QString& path) {
    stop();
    if(!m_vlc) return;
    m_media=libvlc_media_new_path(m_vlc,path.toUtf8().constData());
    if(!m_media){ qWarning()<<"VideoScreen: cannot open"<<path; return; }
    m_player=libvlc_media_player_new_from_media(m_media);
    libvlc_media_release(m_media);
    m_videoW=1280; m_videoH=720;
    // Video callback
    libvlc_video_set_callbacks(m_player,vlcLock,vlcUnlock,vlcDisplay,this);
    libvlc_video_set_format(m_player,"RV32",m_videoW,m_videoH,m_videoW*4);
    m_frameBack=QImage(m_videoW,m_videoH,QImage::Format_RGB32);
    m_frameBack.fill(Qt::black);
    m_frame=m_frameBack.copy();
    // Audio callback
    libvlc_audio_set_callbacks(m_player,vlcAudioPlay,nullptr,
                               vlcAudioFlush,vlcAudioDrain,nullptr,this);
    libvlc_audio_set_format_callbacks(m_player,vlcAudioSetup,vlcAudioCleanup);
    m_loaded=true;
    m_paused=false;
    libvlc_media_player_play(m_player);
    // Info video
    QString title=QFileInfo(path).completeBaseName();
    QTimer::singleShot(800,[this,title](){
        if(!m_player) return;
        // Info
        libvlc_time_t dur=libvlc_media_player_get_length(m_player);
        qint64 durSec=dur/1000;
        QString durStr=QString("%1:%2").arg(durSec/60,2,10,QChar('0')).arg(durSec%60,2,10,QChar('0'));
        QString vCodec="N/A"; QString aCodec="N/A"; int sampleRate=0;
        libvlc_media_track_t** tracks=nullptr;
        libvlc_media_t* med=libvlc_media_player_get_media(m_player);
        if(med){
            unsigned cnt=libvlc_media_tracks_get(med,&tracks);
            for(unsigned i=0;i<cnt;i++){
                auto* t=tracks[i];
                char codec[5]={0};
                codec[0]=(t->i_codec>>0)&0xFF;
                codec[1]=(t->i_codec>>8)&0xFF;
                codec[2]=(t->i_codec>>16)&0xFF;
                codec[3]=(t->i_codec>>24)&0xFF;
                QString cs=QString(codec).trimmed().toUpper();
                if(t->i_type==libvlc_track_video && vCodec=="N/A") vCodec=cs;
                if(t->i_type==libvlc_track_audio && aCodec=="N/A"){
                    aCodec=cs; if(t->audio) sampleRate=t->audio->i_rate;
                }
            }
            if(tracks) libvlc_media_tracks_release(tracks,cnt);
        }
        emit videoInfoReady(title,durStr,25.0,m_videoW,m_videoH,vCodec,aCodec,sampleRate,0);
    });
}

void VideoScreen::stop() {
    m_loaded=false; m_paused=false;
    if(m_player){
        libvlc_media_player_stop(m_player);
        libvlc_media_player_release(m_player);
        m_player=nullptr;
    }
    if(m_aplay){ m_aplay->terminate(); delete m_aplay; m_aplay=nullptr; }
    QMutexLocker lk(&m_mutex);
    m_frame=QImage();
}

void VideoScreen::setPaused(bool p) {
    m_paused=p;
    if(m_player) libvlc_media_player_set_pause(m_player,p?1:0);
}

void VideoScreen::paint(QPainter& p, int winW, int winH,
                         int panelW, int panelH,
                         int sx, int sy, int sw, int sh) {
    QMutexLocker lk(&m_mutex);
    if(m_frame.isNull()) return;
    int dx=(int)((float)sx/panelW*winW);
    int dy=(int)((float)sy/panelH*winH);
    int dw=(int)((float)sw/panelW*winW);
    int dh=(int)((float)sh/panelH*winH);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(QRect(dx,dy,dw,dh),m_frame);
}

void VideoScreen::paintFullscreen(QPainter& p, int winW, int winH) {
    QMutexLocker lk(&m_mutex);
    if(m_frame.isNull()) return;
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(QRect(0,0,winW,winH),m_frame);
}
