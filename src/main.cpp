#include "mainwindow.h"

#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("NiecheEmu"));

    const QString start = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();
    MainWindow w(start);
    w.show();
    return app.exec();
}
