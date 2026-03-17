#include "skin_manager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

SkinManager& SkinManager::instance() {
    static SkinManager s;
    return s;
}

bool SkinManager::load(const QString& skinPath) {
    QString ini = skinPath + "/skin.ini";
    if(!QFileInfo::exists(ini)) {
        qWarning() << "SkinManager: skin.ini non trovato in" << skinPath;
        return false;
    }
    QSettings s(ini, QSettings::IniFormat);
    m_currentPath = skinPath;

    s.beginGroup("panel");
    m_data.panelImage = skinPath + "/" + s.value("image","player_panel.png").toString();
    m_data.panelW     = s.value("width",  2030).toInt();
    m_data.panelH     = s.value("height", 537).toInt();
    s.endGroup();

    s.beginGroup("display");
    m_data.displayX = s.value("x", 301).toInt();
    m_data.displayY = s.value("y", 166).toInt();
    m_data.displayW = s.value("w", 1276).toInt();
    m_data.displayH = s.value("h", 194).toInt();
    s.endGroup();

    s.beginGroup("vumeter");
    m_data.vuX = s.value("x", 38).toInt();
    m_data.vuY = s.value("y", 50).toInt();
    m_data.vuW = s.value("w", 166).toInt();
    m_data.vuH = s.value("h", 122).toInt();
    s.endGroup();

    s.beginGroup("knob_volume");
    m_data.knobVolX    = s.value("x", 1798).toInt();
    m_data.knobVolY    = s.value("y", 329).toInt();
    m_data.knobVolSize = s.value("size", 276).toInt();
    s.endGroup();

    s.beginGroup("knob_bass");
    m_data.knobBassX    = s.value("x", 1313).toInt();
    m_data.knobBassY    = s.value("y", 440).toInt();
    m_data.knobBassSize = s.value("size", 105).toInt();
    s.endGroup();

    s.beginGroup("knob_treble");
    m_data.knobTrebleX    = s.value("x", 1522).toInt();
    m_data.knobTrebleY    = s.value("y", 440).toInt();
    m_data.knobTrebleSize = s.value("size", 105).toInt();
    s.endGroup();

    s.beginGroup("power");
    m_data.powerX    = s.value("x", 153).toInt();
    m_data.powerY    = s.value("y", 235).toInt();
    m_data.powerSize = s.value("size", 84).toInt();
    s.endGroup();

    s.beginGroup("button_rewind");
    m_data.btnRewindX = s.value("x", 400).toInt();
    m_data.btnY       = s.value("y", 437).toInt();
    s.endGroup();

    s.beginGroup("button_play");
    m_data.btnPlayX = s.value("x", 526).toInt();
    s.endGroup();

    s.beginGroup("button_pause");
    m_data.btnPauseX = s.value("x", 656).toInt();
    s.endGroup();

    s.beginGroup("button_stop");
    m_data.btnStopX = s.value("x", 784).toInt();
    s.endGroup();

    s.beginGroup("button_forward");
    m_data.btnForwardX = s.value("x", 908).toInt();
    s.endGroup();

    s.beginGroup("button_open");
    m_data.btnOpenX = s.value("x", 1034).toInt();
    s.endGroup();

    qDebug() << "SkinManager: skin caricata da" << skinPath;
    return true;
}

QStringList SkinManager::availableSkins(const QString& skinsDir) {
    QStringList list;
    QDir dir(skinsDir);
    for(auto& d : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if(QFileInfo::exists(skinsDir + "/" + d + "/skin.ini"))
            list << d;
    }
    return list;
}
