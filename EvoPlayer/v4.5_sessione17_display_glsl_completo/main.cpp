#include <QApplication>
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
    w.resize(1902, 476);
    w.setFixedSize(1902, 476);
    w.setWindowTitle("EvoPlayer");
    w.show();

    return app.exec();
}
