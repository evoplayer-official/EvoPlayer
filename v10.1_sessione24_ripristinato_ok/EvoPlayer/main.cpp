#include <QApplication>
#include <QTimer>
#include "ui/eq_window.h"
#include "ui/library_window.h"
#include <QSettings>
#include <QSurfaceFormat>
#include "ui/main_window.h"

int main(int argc, char *argv[])
{
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

    EqWindow eq;
    eq.setFixedSize(sz);
    eq.move(w.x(), w.y() + sz.height());
    eq.show();

    LibraryWindow lib;
    lib.setFixedSize(sz);
    lib.move(w.x(), w.y() + sz.height()*2);
    lib.show();
    QTimer::singleShot(100, [&](){ eq.setAudio(w.audio()); eq.setVideo(&lib.videoScreen()); lib.setAudio(w.audio()); });
    w.setLibraryWindow(&lib);

    return app.exec();
}
