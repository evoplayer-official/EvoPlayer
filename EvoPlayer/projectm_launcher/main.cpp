#include <QApplication>
#include <QOpenGLWidget>
#include <QSurfaceFormat>
#include <QTimer>
#include <QDebug>
#include <libprojectM/projectM.hpp>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <thread>
#include <atomic>

class ProjectMWindow : public QOpenGLWidget {
public:
    ProjectMWindow(QWidget* parent=nullptr) : QOpenGLWidget(parent){}
    ~ProjectMWindow(){
        m_running=false;
        if(m_audioThread.joinable()) m_audioThread.join();
        if(m_pm) delete m_pm;
    }
    void initializeGL() override {
            projectM::Settings s;
        s.windowWidth  = width();
        s.windowHeight = height();
        s.fps          = 60;
        s.meshX        = 32;
        s.meshY        = 24;
        s.smoothPresetDuration = 10;
        s.presetDuration       = 15;
        s.beatSensitivity      = 1.5f;
        s.aspectCorrection     = true;
        s.shuffleEnabled       = true;
        s.presetURL    = "/usr/share/projectM/presets";
        s.menuFontURL  = "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf";
        s.titleFontURL = "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf";
        m_pm=new projectM(s, projectM::FLAG_NONE);
        m_running=true;
        m_audioThread=std::thread([this](){
            pa_simple* pa=nullptr;
            pa_sample_spec ss;
            ss.format=PA_SAMPLE_FLOAT32LE;
            ss.channels=2;
            ss.rate=44100;
            int err=0;
            pa=pa_simple_new(nullptr,"EvoPlayer-Viz",PA_STREAM_RECORD,
                             nullptr,"visualization",&ss,nullptr,nullptr,&err);
            if(!pa){ qWarning()<<"PA error:"<<pa_strerror(err); return; }
            while(m_running){
                float buf[512*2];
                if(pa_simple_read(pa,buf,sizeof(buf),&err)<0) break;
                if(m_pm) m_pm->pcm()->addPCMfloat(buf,512);
            }
            pa_simple_free(pa);
        });
        m_timer=new QTimer(this);
        connect(m_timer,&QTimer::timeout,this,[this](){ update(); });
        m_timer->start(16);
    }
    void paintGL() override {
        makeCurrent();
        if(m_pm) m_pm->renderFrame();
        doneCurrent();
    }
    void resizeGL(int w,int h) override {
        if(m_pm) m_pm->projectM_resetGL(w,h);
    }
private:
    projectM*   m_pm=nullptr;
    QTimer*     m_timer=nullptr;
    std::thread m_audioThread;
    std::atomic<bool> m_running{false};
};

int main(int argc, char* argv[]) {
    setenv("QT_QPA_PLATFORM","xcb",1);
    QSurfaceFormat fmt;
    fmt.setVersion(3,3);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    fmt.setSamples(0);
    QSurfaceFormat::setDefaultFormat(fmt);
    QApplication app(argc,argv);
    ProjectMWindow w;
    int x=0,y=0,sw=591,sh=396;
    if(argc>=5){ x=atoi(argv[1]); y=atoi(argv[2]); sw=atoi(argv[3]); sh=atoi(argv[4]); }
    w.setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::Tool);
    w.resize(sw,sh);
    w.move(x,y);
    w.show();
    return app.exec();
}
