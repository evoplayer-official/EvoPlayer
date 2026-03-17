#pragma once
#include <QString>
#include <QSettings>
#include <QSize>
#include <QPoint>

struct SkinData {
    // Panel
    QString panelImage;
    int panelW = 2030, panelH = 537;

    // Display
    int displayX=301, displayY=166, displayW=1276, displayH=194;

    // VU Meter
    int vuX=38, vuY=50, vuW=166, vuH=122;

    // Knob volume
    int knobVolX=1798, knobVolY=329, knobVolSize=276;

    // Knob bass
    int knobBassX=1313, knobBassY=440, knobBassSize=105;

    // Knob treble
    int knobTrebleX=1522, knobTrebleY=440, knobTrebleSize=105;

    // Power
    int powerX=153, powerY=235, powerSize=84;

    // Buttons Y
    int btnRewindX=400,  btnY=437;
    int btnPlayX=526;
    int btnPauseX=656;
    int btnStopX=784;
    int btnForwardX=908;
    int btnOpenX=1034;
};

class SkinManager {
public:
    static SkinManager& instance();
    bool load(const QString& skinPath);
    const SkinData& data() const { return m_data; }
    QString currentPath() const { return m_currentPath; }
    QStringList availableSkins(const QString& skinsDir);

private:
    SkinData m_data;
    QString m_currentPath;
};
