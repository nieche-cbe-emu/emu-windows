#include "core.h"
#include "mainwindow.h"

#include <QApplication>
#include <QIcon>
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

static int selftest(const char *modulePath)
{
    auto say = [](const QString &m) { MainWindow::trace(QStringLiteral("[selftest] ") + m); };
    Core c;
    say(QStringLiteral("load"));
    if (!c.load()) {
        say(QStringLiteral("load 失败: ") + c.errorString());
        return 2;
    }
    say(QStringLiteral("ABI %1").arg(c.abiVersion()));

    say(QStringLiteral("selftest(JIT) 开始"));
    say(QStringLiteral("selftest(JIT) = %1").arg(c.selftest() ? 1 : 0));
    say(QStringLiteral("open"));
    if (!c.open(QString::fromLocal8Bit(modulePath))) {
        say(QStringLiteral("open/boot 失败: ") + c.errorString());
        return 3;
    }
    say(QStringLiteral("boot 成功 %1x%2").arg(c.size().width()).arg(c.size().height()));
    for (int i = 0; i < 60; ++i) {
        const QByteArray px = c.step();
        c.takeEvents();
        if (i < 3 || i == 59)
            say(QStringLiteral("step %1 -> %2 字节").arg(i).arg(px.size()));
    }
    say(QStringLiteral("60 帧完成"));
    c.close();
    say(QStringLiteral("closed"));
    return 0;
}

int main(int argc, char **argv)
{
#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(crashFilter);
#endif
    if (argc > 2 && QString::fromLocal8Bit(argv[1]) == QLatin1String("--selftest"))
        return selftest(argv[2]);

    if (qEnvironmentVariableIsEmpty("NIECHE_HOME")) {
        const QString root = QDir::homePath() + QStringLiteral("/.nieche-emu");
        QDir().mkpath(root);
        qputenv("NIECHE_HOME", QDir::toNativeSeparators(root).toLocal8Bit());
    }
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("NiecheEmu"));
    app.setWindowIcon(QIcon(QStringLiteral(":/app.ico")));
    const QString start = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();
    MainWindow w(start);
    w.show();
    return app.exec();
}
