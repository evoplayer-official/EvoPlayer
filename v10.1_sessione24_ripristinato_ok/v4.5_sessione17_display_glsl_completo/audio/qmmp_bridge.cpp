#include "qmmp_bridge.h"
#include <qmmp/soundcore.h>
#include <qmmp/qmmp.h>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

QmmpBridge::QmmpBridge(QObject *parent)
    : QObject(parent)
    , m_currentIndex(-1)
    , m_userStopped(false)
    , m_changingTrack(false)
{
    m_core = new SoundCore(this);
    m_nextTimer = new QTimer(this);
    m_nextTimer->setSingleShot(true);
    m_nextTimer->setInterval(200);

    connect(m_core, &SoundCore::stateChanged,
            this, [this](Qmmp::State s){ onStateChanged((int)s); });
    connect(m_core, SIGNAL(elapsedChanged(qint64)),
            this, SLOT(onElapsedChanged(qint64)));
    connect(m_nextTimer, &QTimer::timeout, this, &QmmpBridge::autoNext);
}

QmmpBridge::~QmmpBridge() {}

void QmmpBridge::loadFile(const QString &path) {
    m_playlist.clear();
    m_playlist.append(path);
    m_currentIndex = 0;
    m_userStopped = false;
    m_changingTrack = true;
    m_core->play(path);
}

void QmmpBridge::loadFolder(const QString &path) {
    m_playlist.clear();
    QDir dir(path);
    QStringList filters;
    filters << "*.mp3" << "*.flac" << "*.ogg" << "*.wav"
            << "*.opus" << "*.m4a" << "*.aac" << "*.wma";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
    for (auto &f : files)
        m_playlist.append(f.absoluteFilePath());
    if (m_playlist.isEmpty()) return;
    m_currentIndex = 0;
    m_userStopped = false;
    m_changingTrack = true;
    m_core->play(m_playlist[0]);
    qDebug() << "QmmpBridge: caricati" << m_playlist.size() << "brani";
}

void QmmpBridge::playPauseToggle() {
    if (m_core->state() == Qmmp::Playing) {
        m_core->pause();
    } else if (m_core->state() == Qmmp::Paused) {
        m_core->pause();
    } else {
        if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
            m_userStopped = false;
            m_changingTrack = true;
            m_core->play(m_playlist[m_currentIndex]);
        }
    }
}

void QmmpBridge::stop() {
    m_userStopped = true;
    m_core->stop();
}

void QmmpBridge::next() {
    if (m_playlist.isEmpty()) return;
    m_currentIndex = (m_currentIndex + 1) % m_playlist.size();
    m_userStopped = false;
    m_changingTrack = true;
    m_core->play(m_playlist[m_currentIndex]);
}

void QmmpBridge::previous() {
    if (m_playlist.isEmpty()) return;
    if (elapsed() > 3000) {
        m_core->seek(0);
        return;
    }
    m_currentIndex = (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size();
    m_userStopped = false;
    m_changingTrack = true;
    m_core->play(m_playlist[m_currentIndex]);
}

QString QmmpBridge::title() const {
    return m_core->metaData(Qmmp::TITLE);
}

QString QmmpBridge::artist() const {
    return m_core->metaData(Qmmp::ARTIST);
}

qint64 QmmpBridge::elapsed() const {
    return m_core->elapsed();
}

qint64 QmmpBridge::duration() const {
    return m_core->duration();
}

bool QmmpBridge::isPlaying() const {
    return m_core->state() == Qmmp::Playing;
}

bool QmmpBridge::isPaused() const {
    return m_core->state() == Qmmp::Paused;
}

void QmmpBridge::onStateChanged(int state) {
    Qmmp::State s = (Qmmp::State)state;
    if (s == Qmmp::Playing) {
        m_changingTrack = false;
    }
    if (s == Qmmp::Stopped && !m_userStopped && !m_changingTrack) {
        m_nextTimer->start();
    }
    emit stateChanged(state);
}

void QmmpBridge::onElapsedChanged(qint64 ms) {
    emit elapsedChanged(ms);
}

void QmmpBridge::autoNext() {
    if (!m_userStopped && !m_changingTrack) {
        next();
    }
}

QString QmmpBridge::format() const {
    return "MP3";
}

int QmmpBridge::bitrate() const {
    return m_core->bitrate();
}
