#pragma once
#include <QMainWindow>
#include "eq_window.h"

class EqMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit EqMainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        m_eq = new EqWindow(this);
        setCentralWidget(m_eq);
    }
    // Forward metodi pubblici
    void setAudio(QmmpBridge* a) { m_eq->setAudio(a); }
    void setVideo(VideoScreen* v) { m_eq->setVideo(v); }
    EqWindow* eq() { return m_eq; }
private:
    EqWindow* m_eq = nullptr;
};
