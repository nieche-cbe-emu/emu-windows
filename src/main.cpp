#include "mainwindow.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <windows.h>

static LONG WINAPI crashFilter(EXCEPTION_POINTERS *ep)
{
    QString r = qEnvironmentVariable("NIECHE_HOME");
    if (r.isEmpty())
        r = QDir::homePath() + QStringLiteral("/.nieche-emu");
    QFile f(r + QStringLiteral("/emu.log"));
    if (f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
           << "  !! 未处理异常 0x"
           << QString::number((quint32)ep->ExceptionRecord->ExceptionCode, 16)
           << " 于 0x"
           << QString::number((quintptr)ep->ExceptionRecord->ExceptionAddress, 16)
           << "\n";
        ts.flush();
        f.flush();
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char **argv)
{
#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(crashFilter);
#endif
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("NiecheEmu"));
    const QString start = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();
    MainWindow w(start);
    w.show();
    return app.exec();
}
