#pragma once
#include <QTimer>
#include <qmmp/visual.h>
#include <QProcess>
#include <QObject>
#include <QImage>
#include <QMutex>
#include <QString>
#include <vlc/vlc.h>

class VideoScreen : public QObject {
    Q_OBJECT
public:
    explicit VideoScreen(QObject* parent=nullptr);
    ~VideoScreen();

    void loadVideo(const QString& path);
    void stop();
    void setPaused(bool p);
    void setEqGains(float* gains10); // 10 bande -1.0..+1.0
    float eqGain(int band) const { return (band>=0&&band<10)?m_eqGains[band]:0.0f; }
    bool isLoaded()  const { return m_loaded; }
    bool isPaused()  const { return m_paused; }

    void paint(QPainter& p, int winW, int winH,
               int panelW, int panelH,
               int sx, int sy, int sw, int sh);
    void paintFullscreen(QPainter& p, int winW, int winH);

signals:
    void frameReady();
    void videoInfoReady(QString title, QString duration, double fps, int w, int h,
                        QString vCodec, QString aCodec, int sampleRate, int bitrate);
    void audioDataReady();

private:
    // Callback VLC
    static void* vlcLock(void* data, void** planes);
    static void  vlcUnlock(void* data, void* picture, void* const* planes);
    static void  vlcDisplay(void* data, void* picture);

    libvlc_instance_t*     m_vlc     = nullptr;
    libvlc_media_player_t* m_player  = nullptr;
    float          m_eqGains[10] = {};
    float          m_bassState[2] = {};
    float          m_trebleState[2] = {};
    QProcess*      m_aplay    = nullptr;
    QByteArray     m_audioBuf;
    QMutex         m_audioMutex;
    unsigned       m_audioRate     = 44100;
    unsigned       m_audioChannels = 2;
    // Audio callback
    static void vlcAudioPlay(void* data, const void* samples, unsigned count, int64_t pts);
    static void vlcAudioFlush(void* data, int64_t pts);
    static void vlcAudioDrain(void* data, int64_t pts);
    static int  vlcAudioSetup(void** data, char* format, unsigned* rate, unsigned* channels);
    static void vlcAudioCleanup(void* data);
    libvlc_media_t*        m_media   = nullptr;

    QImage  m_frame;
    QImage  m_frameBack;
    QMutex  m_mutex;
    bool    m_loaded = false;
    bool    m_paused = false;
    int     m_videoW = 0;
    int     m_videoH = 0;
};
