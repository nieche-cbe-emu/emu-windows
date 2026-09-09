#include "mainwindow.h"

#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("NiecheEmu"));
    MainWindow w;
    w.show();
    if (argc > 1)
        QMetaObject::invokeMethod(&w, "show", Qt::QueuedConnection);
    return app.exec();
}
