#pragma once
#include <QObject>
#include <QStringList>
#include <QTimer>

class SoundCore;

class QmmpBridge : public QObject {
    Q_OBJECT
public:
    explicit QmmpBridge(QObject *parent = nullptr);
    ~QmmpBridge();

    void loadFile(const QString &path);
    void loadFolder(const QString &path);
    void playPauseToggle();
    void stop();
    void next();
    void previous();

    QString title() const;
    QString artist() const;
    qint64 elapsed() const;
    qint64 duration() const;
    QString format() const;
    int bitrate() const;
    bool isPlaying() const;
    bool isPaused() const;

signals:
    void stateChanged(int state);
    void elapsedChanged(qint64 ms);

private slots:
    void onStateChanged(int state);
    void onElapsedChanged(qint64 ms);
    void autoNext();

private:
    SoundCore *m_core;
    QStringList m_playlist;
    int m_currentIndex;
    bool m_userStopped;
    bool m_changingTrack;
    QTimer *m_nextTimer;
};
