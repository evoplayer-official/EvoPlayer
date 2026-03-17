#pragma once
#include <QMainWindow>
#include "library_window.h"

class LibMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit LibMainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        m_lib = new LibraryWindow(this);
        setCentralWidget(m_lib);
    }
    // Forward metodi pubblici
    void setAudio(QmmpBridge* a)        { m_lib->setAudio(a); }
    void loadVideo(const QString& p)    { m_lib->loadVideo(p); }
    VideoScreen& videoScreen()          { return m_lib->videoScreen(); }
    void videoStop()                    { m_lib->videoStop(); }
    void videoPause()                   { m_lib->videoPause(); }
    bool isVideoPlaying() const         { return m_lib->isVideoPlaying(); }
    LibraryWindow* lib()                { return m_lib; }
private:
    LibraryWindow* m_lib = nullptr;
};
