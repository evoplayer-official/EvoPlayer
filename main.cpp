#include <QApplication>
#include <QTimer>
#include "ui/eq_main_window.h"
#include "ui/snap_manager.h"
#include "ui/lib_main_window.h"
#include <QSettings>
#include <QSurfaceFormat>
#include "ui/main_window.h"

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "xcb"); // Forza XWayland — frameless + drag
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    MainWindow w;
    QSettings s("EvoPlayer", "EvoPlayer");
    QSize sz = s.value("risoluzione", QSize(1600, 423)).toSize();
    w.resize(sz);
    w.setFixedSize(sz);
    w.setWindowTitle("EvoPlayer");
    w.show();

    EqMainWindow eq;
    eq.setFixedSize(sz);
    eq.move(w.x(), w.y() + sz.height());
    eq.show();

    LibMainWindow lib;
    lib.setFixedSize(sz);
    lib.move(w.x(), w.y() + sz.height()*2);
    lib.show();
    QTimer::singleShot(100, [&](){ eq.setAudio(w.audio()); eq.setVideo(&lib.videoScreen()); lib.setAudio(w.audio()); });
    w.setLibraryWindow(lib.lib());
    SnapManager::instance()->add(&w);
    SnapManager::instance()->add(&eq);
    SnapManager::instance()->add(&lib);

    return app.exec();
}
